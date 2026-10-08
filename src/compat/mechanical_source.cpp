#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <locale>
#include <miniz.h>
#include <numbers>
#include <opensim/compat/mechanical_source.hpp>
#include <pugixml.hpp>
#include <quickjs.h>
#include <sstream>
#include <stdexcept>
namespace opensim::compat {
namespace {
struct Value {
    JSContext *context;
    JSValue value;
    Value(JSContext *c, JSValue v) : context(c), value(v) {}
    ~Value() { JS_FreeValue(context, value); }
    Value(const Value &) = delete;
};
Value field(JSContext *c, JSValueConst v, const char *name) {
    return {c, JS_GetPropertyStr(c, v, name)};
}
std::string string(JSContext *c, JSValueConst v) {
    const char *s = JS_ToCString(c, v);
    if (!s)
        throw std::runtime_error("Invalid script string");
    std::string result(s);
    JS_FreeCString(c, s);
    return result;
}
double number(JSContext *c, JSValueConst v) {
    double n = 0;
    if (JS_ToFloat64(c, &n, v) || !std::isfinite(n))
        throw std::runtime_error("Nonfinite script number");
    return n;
}
double num(JSContext *c, JSValueConst v, const char *key) {
    auto f = field(c, v, key);
    return number(c, f.value);
}
std::uint64_t bits_value(JSContext *c, JSValueConst v, const char *key) {
    const double n = num(c, v, key);
    if (n < 0 || n > 9007199254740991.0 || std::floor(n) != n)
        throw std::runtime_error("Invalid integer filter bits");
    return static_cast<std::uint64_t>(n);
}
math::Vec2 vec(JSContext *c, JSValueConst v) { return {num(c, v, "x"), num(c, v, "y")}; }
JSValue vector(JSContext *c, math::Vec2 v) {
    auto o = JS_NewObject(c);
    JS_SetPropertyStr(c, o, "x", JS_NewFloat64(c, v.x));
    JS_SetPropertyStr(c, o, "y", JS_NewFloat64(c, v.y));
    return o;
}
void put(JSContext *c, JSValue o, const char *key, double n) {
    JS_SetPropertyStr(c, o, key, JS_NewFloat64(c, n));
}
double scalar(pugi::xml_node n, double fallback = 0) {
    if (!n)
        return fallback;
    std::istringstream in(n.child_value());
    in.imbue(std::locale::classic());
    double v = 0;
    if (!(in >> v) || !std::isfinite(v) || (in >> std::ws && !in.eof()))
        throw std::runtime_error("Invalid numeric source field");
    return v;
}
math::Vec2 point(pugi::xml_node n) {
    if (!n)
        throw std::runtime_error("Missing source vector");
    math::Vec2 v{};
    for (const auto *key : {"x", "y"}) {
        std::istringstream in(n.attribute(key).value());
        in.imbue(std::locale::classic());
        double x = 0;
        if (!(in >> x) || !std::isfinite(x) || (in >> std::ws && !in.eof()))
            throw std::runtime_error("Invalid source vector");
        if (key[0] == 'x')
            v.x = x;
        else
            v.y = x;
    }
    return v;
}
math::Vec2 rotate(math::Vec2 v, double a) {
    return {v.x * std::cos(a) - v.y * std::sin(a), v.x * std::sin(a) + v.y * std::cos(a)};
}
JSValue color(JSContext *c, pugi::xml_node n) {
    double values[4]{1, 1, 1, 1};
    const char *keys[] = {"r", "g", "b", "a"};
    for (unsigned i = 0; i < 4; ++i) {
        if (n && n.attribute(keys[i])) {
            std::istringstream in(n.attribute(keys[i]).value());
            in.imbue(std::locale::classic());
            if (!(in >> values[i]) || (in >> std::ws && !in.eof()))
                throw std::runtime_error("Invalid source color");
        }
        if (!std::isfinite(values[i]) || values[i] < 0 || values[i] > 1)
            throw std::runtime_error("Invalid source color");
    }
    auto a = JS_NewArray(c);
    for (unsigned i = 0; i < 4; ++i)
        JS_SetPropertyUint32(c, a, i, JS_NewFloat64(c, values[i]));
    return a;
}
scene::Color rgba(JSContext *c, JSValueConst v) {
    double out[4]{};
    for (unsigned i = 0; i < 4; ++i) {
        Value n(c, JS_GetPropertyUint32(c, v, i));
        double x = number(c, n.value);
        if (x < 0 || x > 1)
            throw std::runtime_error("Invalid script color");
        out[i] = x;
    }
    return {out[0], out[1], out[2], out[3]};
}
std::vector<std::uint8_t> member(const Project &p, const std::string &name, std::size_t limit) {
    mz_zip_archive z{};
    if (!mz_zip_reader_init_mem(&z, p.source.data(), p.source.size(), 0))
        throw std::runtime_error("Source ZIP unavailable");
    struct End {
        mz_zip_archive *z;
        ~End() { mz_zip_reader_end(z); }
    } end{&z};
    int index = mz_zip_reader_locate_file(&z, name.c_str(), nullptr, 0);
    mz_zip_archive_file_stat s{};
    if (index < 0 || !mz_zip_reader_file_stat(&z, static_cast<mz_uint>(index), &s) ||
        s.m_uncomp_size > limit)
        throw std::runtime_error("Missing or oversized source member: " + name);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(s.m_uncomp_size));
    if (!mz_zip_reader_extract_to_mem(&z, static_cast<mz_uint>(index), bytes.data(), bytes.size(),
                                      0))
        throw std::runtime_error("Source member CRC failure");
    return bytes;
}
// Deterministic ear clipping preserves the authored occupied boundary. No hull.
std::vector<std::vector<math::Vec2>> pieces(std::vector<math::Vec2> v) {
    constexpr double eps = 1e-10;
    const auto cross = [](math::Vec2 a, math::Vec2 b, math::Vec2 c) {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    };
    if (v.size() > 64 || v.size() < 3)
        throw std::runtime_error("Polygon vertex budget (3..64)");
    for (auto q : v)
        if (!math::finite(q) || std::abs(q.x) > 1000 || std::abs(q.y) > 1000)
            throw std::runtime_error("Polygon outside local envelope");
    if (v.front() == v.back())
        v.pop_back();
    const auto on = [&](math::Vec2 a, math::Vec2 b, math::Vec2 q) {
        return std::abs(cross(a, b, q)) <= eps && q.x >= std::min(a.x, b.x) - eps &&
               q.x <= std::max(a.x, b.x) + eps && q.y >= std::min(a.y, b.y) - eps &&
               q.y <= std::max(a.y, b.y) + eps;
    };
    for (std::size_t i = 0; i < v.size(); ++i) {
        auto a = v[i], b = v[(i + 1) % v.size()];
        if (a == b)
            throw std::runtime_error("Duplicate polygon edge");
        for (std::size_t j = i + 1; j < v.size(); ++j) {
            if (j == i + 1 || (i == 0 && j + 1 == v.size()))
                continue;
            auto c = v[j], d = v[(j + 1) % v.size()];
            const double ab = cross(a, b, c), ac = cross(a, b, d), cd = cross(c, d, a),
                         ce = cross(c, d, b);
            if (((ab > eps && ac < -eps) || (ab < -eps && ac > eps)) &&
                ((cd > eps && ce < -eps) || (cd < -eps && ce > eps)))
                throw std::runtime_error("Self-intersecting polygon");
            if (on(a, b, c) || on(a, b, d) || on(c, d, a) || on(c, d, b))
                throw std::runtime_error("Touching polygon boundary or hole");
        }
    }
    bool changed = true;
    while (changed && v.size() > 3) {
        changed = false;
        for (std::size_t i = 0; i < v.size(); ++i) {
            if (on(v[(i + v.size() - 1) % v.size()], v[(i + 1) % v.size()], v[i])) {
                v.erase(v.begin() + static_cast<std::ptrdiff_t>(i));
                changed = true;
                break;
            }
        }
    }
    double area = 0;
    for (std::size_t i = 0; i < v.size(); ++i)
        area += v[i].x * v[(i + 1) % v.size()].y - v[i].y * v[(i + 1) % v.size()].x;
    if (std::abs(area) <= eps)
        throw std::runtime_error("Degenerate polygon");
    if (area < 0)
        std::reverse(v.begin(), v.end());
    bool convex = v.size() <= 8;
    for (std::size_t i = 0; i < v.size(); ++i)
        convex = convex && cross(v[i], v[(i + 1) % v.size()], v[(i + 2) % v.size()]) > eps;
    if (convex)
        return {v};
    std::vector<std::vector<math::Vec2>> out;
    while (v.size() > 3) {
        bool ear = false;
        for (std::size_t i = 0; i < v.size(); ++i) {
            const auto before = (i + v.size() - 1) % v.size(), after = (i + 1) % v.size();
            auto a = v[before], b = v[i], c = v[after];
            if (cross(a, b, c) <= eps)
                continue;
            bool occupied = false;
            for (std::size_t k = 0; k < v.size(); ++k) {
                if (k == before || k == i || k == after)
                    continue;
                occupied = occupied || (cross(a, b, v[k]) >= -eps && cross(b, c, v[k]) >= -eps &&
                                        cross(c, a, v[k]) >= -eps);
            }
            if (occupied)
                continue;
            out.push_back({a, b, c});
            v.erase(v.begin() + static_cast<std::ptrdiff_t>(i));
            ear = true;
            break;
        }
        if (!ear)
            throw std::runtime_error("Polygon cannot be decomposed without losing geometry");
    }
    if (cross(v[0], v[1], v[2]) <= eps)
        throw std::runtime_error("Degenerate polygon piece");
    out.push_back(v);
    return out;
}
const char *bridge = R"JS(
class Vector2 {
 constructor(x=0,y=0){this.x=x;this.y=y;}
 add(x,y){if(typeof x==='object'){this.x+=x.x;this.y+=x.y;}else{this.x+=x;this.y+=y;}return this;}
 sum(x,y){return new Vector2(this.x,this.y).add(x,y);}
 difference(v){return new Vector2(this.x-v.x,this.y-v.y);}
 getMagnitude(){return Math.hypot(this.x,this.y);}
}
class Color {
 constructor(name){const colors={white:[1,1,1,1],pink:[1,.68,.76,1],yellow:[1,1,0,1],red:[1,0,0,1],green:[0,.5,0,1],orange:[1,.647,0,1],black:[0,0,0,1],blue:[0,0,1,1]};if(!colors[name])throw Error('Unsupported color '+name);this.rgba=colors[name];}
}
let __bodies=__seed.bodies, __links=__seed.links, __next=__bodies.length+1, __joint=__links.reduce((n,j)=>Math.max(n,j.id),0)+1;
const __initial=JSON.parse(JSON.stringify(__seed.bodies));
class Body {
 constructor(d){this.d=d;}
 setFillColor(c){this.d.fill=c.rgba.slice();}
 setPosition(v,y){const p=typeof v==='object'?v:{x:v,y:y};this.d.center={x:p.x,y:p.y};this.d.velocity={x:0,y:0};}
 getPosition(){return new Vector2(this.d.center.x,this.d.center.y);}
 setRotation(a){if(!Number.isFinite(a))throw Error('Invalid angle');this.d.angle=a;this.d.angular=0;}
 reset(){const initial=__initial.find(b=>b.id===this.d.id);if(!initial)throw Error('Missing initial body');Object.assign(this.d,JSON.parse(JSON.stringify(initial)));}
 getVelocity(){return new Vector2(this.d.velocity.x,this.d.velocity.y);}
}
function __local(body,p){const x=p.x-body.d.center.x,y=p.y-body.d.center.y,c=Math.cos(body.d.angle),s=Math.sin(body.d.angle);return {x:c*x+s*y,y:-s*x+c*y};}
const World={
 clear(){__bodies=__bodies.filter(b=>!b.spawned);__links=__links.filter(j=>!j.spawned);},
 getBody(name){const b=__bodies.find(b=>b.name===name);if(!b)throw Error('Missing body '+name);return new Body(b);},
 createCopy(body){if(__bodies.length>=256)throw Error('Body budget');const b=JSON.parse(JSON.stringify(body.d));b.id=__next++;b.spawned=true;__bodies.push(b);return new Body(b);},
 addDistanceJoint(a,b,p,q){if(__links.length>=1024)throw Error('Joint budget');const j={id:__joint++,a:a.d.id,b:b?b.d.id:0,pa:__local(a,p),pb:b?__local(b,q):{x:q.x,y:q.y},length:Math.hypot(p.x-q.x,p.y-q.y),color:[1,.647,0,1],spawned:true};__links.push(j);return {setColor(c){j.color=c.rgba.slice();}};}
};
const Resources={getSound(name){return {isPlaying(){return false;},play(){throw Error('Collision audio unsupported in experimental mechanics');}};}};
// No host modules, I/O, network, native handles or clocks are installed.
Date=undefined;
var sin=Math.sin,cos=Math.cos,tan=Math.tan,sqrt=Math.sqrt,abs=Math.abs,min=Math.min,max=Math.max,pi=Math.PI;
Math.random=()=>{throw Error('Random script API not supported');};
)JS";
} // namespace
struct MechanicalSource::Impl {
    JSRuntime *runtime = nullptr;
    JSContext *context = nullptr;
    std::chrono::steady_clock::time_point deadline{};
    scene::Mechanism definition;
    struct Controller {
        core::EntityId body;
        std::string enabled, x, y, friction;
        math::Vec2 point{};
        int mode = 0;
        double initial_angle = 0;
    };
    std::vector<Controller> controllers;
    std::map<core::EntityId, BodyStyle> styles;
    std::map<core::EntityId, scene::Color> joint_colors;
    std::map<std::string, std::vector<std::uint8_t>> images;
    std::vector<SourceWidget> widgets;
    math::Vec2 camera{};
    double scale = 45;
    bool poisoned = false, ready = false;
    Impl() {
        runtime = JS_NewRuntime();
        if (!runtime)
            throw std::runtime_error("Script runtime allocation failed");
        JS_SetMemoryLimit(runtime, 64 * 1024 * 1024);
        JS_SetMaxStackSize(runtime, 512 * 1024);
        context = JS_NewContext(runtime);
        if (!context) {
            JS_FreeRuntime(runtime);
            runtime = nullptr;
            throw std::runtime_error("Script context allocation failed");
        }
        JS_SetInterruptHandler(
            runtime,
            [](JSRuntime *, void *p) {
                auto &i = *static_cast<Impl *>(p);
                return std::chrono::steady_clock::now() > i.deadline ? 1 : 0;
            },
            this);
    }
    ~Impl() {
        if (context)
            JS_FreeContext(context);
        if (runtime)
            JS_FreeRuntime(runtime);
    }
    JSValue eval(std::string_view code) {
        deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
        auto v = JS_Eval(context, code.data(), code.size(), "source-script", JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(v)) {
            Value e(context, JS_GetException(context));
            throw std::runtime_error("Script: " + string(context, e.value));
        }
        return v;
    }
    void collect() {
        scene::Mechanism next = definition;
        next.bodies.clear();
        next.links.clear();
        std::map<core::EntityId, BodyStyle> next_styles;
        std::map<core::EntityId, scene::Color> next_colors;
        Value bodies(context, eval("__bodies"));
        const double count = num(context, bodies.value, "length");
        if (count < 0 || count > 256)
            throw std::runtime_error("Body budget");
        for (unsigned i = 0; i < static_cast<unsigned>(count); ++i) {
            Value v(context, JS_GetPropertyUint32(context, bodies.value, i));
            scene::CircleBody b;
            const double id = num(context, v.value, "id");
            if (id < 1 || id > 9007199254740991.0 || std::floor(id) != id)
                throw std::runtime_error("Invalid script body ID");
            b.id = {static_cast<std::uint64_t>(id)};
            {
                auto n = field(context, v.value, "name");
                b.name = string(context, n.value);
            }
            auto p = field(context, v.value, "center");
            b.center = vec(context, p.value);
            auto speed = field(context, v.value, "velocity");
            b.velocity = vec(context, speed.value);
            b.angle = num(context, v.value, "angle");
            b.angular_velocity = num(context, v.value, "angular");
            b.radius = num(context, v.value, "radius");
            b.mass = num(context, v.value, "mass");
            b.inertia = num(context, v.value, "inertia");
            b.friction = num(context, v.value, "friction");
            b.restitution = num(context, v.value, "restitution");
            b.damping = num(context, v.value, "damping");
            b.angular_damping = num(context, v.value, "angularDamping");
            b.gravity_scale = num(context, v.value, "gravityScale");
            b.fixed_rotation = num(context, v.value, "fixed") != 0;
            b.static_body = num(context, v.value, "static") != 0;
            b.category = bits_value(context, v.value, "category");
            b.mask = bits_value(context, v.value, "mask");
            b.sensor = num(context, v.value, "sensor") != 0;
            auto fs = field(context, v.value, "fixtures");
            const double nf = num(context, fs.value, "length");
            if (nf < 0 || nf > 256)
                throw std::runtime_error("Fixture budget");
            for (unsigned k = 0; k < static_cast<unsigned>(nf); ++k) {
                Value fv(context, JS_GetPropertyUint32(context, fs.value, k));
                scene::RigidFixture f;
                auto cp = field(context, fv.value, "center");
                f.center = vec(context, cp.value);
                f.radius = num(context, fv.value, "radius");
                f.friction = num(context, fv.value, "friction");
                f.restitution = num(context, fv.value, "restitution");
                f.sensor = num(context, fv.value, "sensor") != 0;
                f.category = bits_value(context, fv.value, "category");
                f.mask = bits_value(context, fv.value, "mask");
                auto vs = field(context, fv.value, "vertices");
                const double nv = num(context, vs.value, "length");
                if (nv < 0 || nv > 8)
                    throw std::runtime_error("Polygon vertex budget");
                for (unsigned t = 0; t < static_cast<unsigned>(nv); ++t) {
                    Value q(context, JS_GetPropertyUint32(context, vs.value, t));
                    f.vertices.push_back(vec(context, q.value));
                }
                b.fixtures.push_back(std::move(f));
            }
            BodyStyle style;
            auto fill = field(context, v.value, "fill");
            style.fill = rgba(context, fill.value);
            auto line = field(context, v.value, "outline");
            style.outline = rgba(context, line.value);
            auto image = field(context, v.value, "image");
            style.image = string(context, image.value);
            if (b.name.size() > 4096 || style.image.size() > 4096 ||
                (ready && !style.image.empty() && !images.contains(style.image)))
                throw std::runtime_error("Script name/image is unavailable or exceeds bounds");
            next_styles.emplace(b.id, std::move(style));
            next.bodies.push_back(std::move(b));
        }
        Value links(context, eval("__links"));
        const double n = num(context, links.value, "length");
        if (n < 0 || n > 1024)
            throw std::runtime_error("Joint budget");
        for (unsigned i = 0; i < static_cast<unsigned>(n); ++i) {
            Value v(context, JS_GetPropertyUint32(context, links.value, i));
            const auto identifier = [&](const char *name) {
                double x = num(context, v.value, name);
                if (x < 0 || x > 9007199254740991.0 || std::floor(x) != x)
                    throw std::runtime_error("Invalid script joint ID");
                return core::EntityId{static_cast<std::uint64_t>(x)};
            };
            scene::DistanceLink j;
            j.id = identifier("id");
            j.body_a = identifier("a");
            j.body_b = identifier("b");
            auto a = field(context, v.value, "pa");
            j.local_a = vec(context, a.value);
            auto b = field(context, v.value, "pb");
            j.local_b = vec(context, b.value);
            j.length = num(context, v.value, "length");
            const auto optional = [&](const char *key, double fallback) {
                auto value = field(context, v.value, key);
                return JS_IsUndefined(value.value) ? fallback : number(context, value.value);
            };
            j.stiffness = optional("stiffness", 0);
            j.spring = optional("elastic", 0) != 0;
            j.damping_ratio = optional("damping", 0);
            j.damping_coefficient = optional("coefficient", -1);
            j.minimum = optional("minimum", .01);
            j.maximum = optional("maximum", 20000);
            j.limit = optional("limit", 0) != 0;
            j.collide_connected = optional("collision", 0) != 0;
            auto c = field(context, v.value, "color");
            next_colors.emplace(j.id, rgba(context, c.value));
            next.links.push_back(j);
        }
        const auto valid = scene::validate(next);
        if (valid.error())
            throw std::runtime_error(valid.error()->message);
        for (const auto &c : controllers)
            if (std::none_of(next.bodies.begin(), next.bodies.end(),
                             [&](const auto &b) { return b.id == c.body; }))
                throw std::runtime_error("Action removed a required controlled body");
        definition = std::move(next);
        styles = std::move(next_styles);
        joint_colors = std::move(next_colors);
    }
};
MechanicalSource::MechanicalSource(std::shared_ptr<Impl> p) : impl_(std::move(p)) {}
core::Result<MechanicalSource> MechanicalSource::create(const Project &project) {
    try {
        auto p = std::make_shared<Impl>();
        auto bytes = member(project, "simulation.xml", 8 * 1024 * 1024);
        pugi::xml_document xml;
        if (!xml.load_buffer(bytes.data(), bytes.size(), pugi::parse_default, pugi::encoding_utf8))
            throw std::runtime_error("Invalid source XML");
        auto root = xml.child("Simulation"), world = root.child("World");
        if (world.child("Circuit") || world.child("Geometry") ||
            world.child("Fields").first_child() || world.child("Controllers").first_child())
            throw std::runtime_error("Other domains/fields/controllers require source preview");
        if (world.child("ParticleSystem").first_child() || world.child("Tracers").first_child())
            throw std::runtime_error("Particle systems/tracers require their runtime features");
        p->definition.gravity = point(world.child("Gravity"));
        std::string mix = world.child("Preferences").child("coeffMixer").child_value();
        if (!mix.empty()) {
            for (auto &ch : mix)
                if (ch == ',')
                    ch = ' ';
            std::istringstream in(mix);
            int friction = 0, restitution = 0;
            if (!(in >> friction >> restitution) || friction < 0 || friction > 2 ||
                restitution < 0 || restitution > 2 || (in >> std::ws && !in.eof()))
                throw std::runtime_error("Invalid coefficient mixer");
            p->definition.friction_mixer = static_cast<scene::MaterialMixer>(friction);
            p->definition.restitution_mixer = static_cast<scene::MaterialMixer>(restitution);
        }
        p->definition.fixed_dt = 1.0 / scalar(world.child("Settings").child("StepFrequency"), 60);
        auto camera = root.child("Camera");
        if (camera.child("Translation")) {
            auto t = point(camera.child("Translation"));
            p->camera = {-t.x, -t.y};
        }
        std::string scale = camera.child("Scale").child_value();
        if (!scale.empty() && scale.front() == '#')
            scale.erase(0, 1);
        if (!scale.empty()) {
            std::istringstream in(scale);
            in.imbue(std::locale::classic());
            if (!(in >> p->scale) || !std::isfinite(p->scale) || p->scale < 1 || p->scale > 4096)
                throw std::runtime_error("Invalid source camera");
        }
        Value seed(p->context, JS_NewObject(p->context));
        Value bodies(p->context, JS_NewArray(p->context)),
            links(p->context, JS_NewArray(p->context));
        std::map<std::string, unsigned> ids;
        std::map<unsigned, math::Vec2> centers;
        std::map<unsigned, double> angles;
        unsigned index = 0;
        for (auto body : world.child("Bodies").children()) {
            if (std::string(body.name()) != "Body" && std::string(body.name()) != "PlaneBody")
                throw std::runtime_error("Non-circular body type");
            const std::string key = body.attribute("Id").value();
            if (key.empty() || ids.contains(key))
                throw std::runtime_error("Duplicate source body ID");
            auto mass = body.child("Mass");
            std::string mode = mass.child("Type").child_value();
            auto fixtures = body.child("Fixtures");
            if (!fixtures.first_child()) {
                if (mode != "INFINITE")
                    throw std::runtime_error("Dynamic body has no fixtures");
                ids[key] = 0;
                continue;
            }
            if (index >= 256)
                throw std::runtime_error("Body budget");
            auto fixture = fixtures.first_child();

            if (mode != "NORMAL" && mode != "INFINITE" && mode != "FIXED_ANGULAR_VELOCITY")
                throw std::runtime_error("Unsupported source mass mode");
            auto com = point(mass.child("LocalCenter"));

            double angle =
                scalar(body.child("Transform").child("Rotation")) * std::numbers::pi / 180;
            auto origin = point(body.child("Transform").child("Translation")),
                 offset = rotate(com, angle);
            math::Vec2 center{origin.x + offset.x, origin.y + offset.y};
            double angular = scalar(body.child("AngularVelocity")) * std::numbers::pi / 180;
            if (mode == "FIXED_ANGULAR_VELOCITY" && std::abs(angular) > 1e-12)
                throw std::runtime_error("Prescribed nonzero angular velocity unsupported");
            if ((body.child("AccumulatedForce") &&
                 point(body.child("AccumulatedForce")) != math::Vec2{}) ||
                scalar(body.child("AccumulatedTorque")) != 0 || scalar(body.child("Charge")) != 0 ||

                body.child("Active").text().as_bool(true) == false)
                throw std::runtime_error("Unsupported force/filter/charge/inactive state");
            Value v(p->context, JS_NewObject(p->context));
            unsigned id = ++index;
            ids[key] = id;
            centers[id] = center;
            angles[id] = angle;
            put(p->context, v.value, "id", id);
            JS_SetPropertyStr(p->context, v.value, "name",
                              JS_NewString(p->context, body.attribute("Name").value()));
            JS_SetPropertyStr(p->context, v.value, "center", vector(p->context, center));
            JS_SetPropertyStr(p->context, v.value, "velocity",
                              vector(p->context, point(body.child("Velocity"))));
            put(p->context, v.value, "angle", angle);
            put(p->context, v.value, "angular", mode == "FIXED_ANGULAR_VELOCITY" ? 0 : angular);
            const auto bits = [](pugi::xml_node group) {
                std::uint64_t out = 0;
                for (auto n : group.children()) {
                    std::istringstream in(n.attribute("Value").value());
                    std::uint64_t value = 0;
                    if (!(in >> value) || value > 2147483647ULL || (in >> std::ws && !in.eof()))
                        throw std::runtime_error("Invalid source category filter");
                    out |= value;
                }
                return out;
            };
            Value fs(p->context, JS_NewArray(p->context));
            unsigned fi = 0;
            double bound = 1e-4;
            for (auto fx : fixtures.children("Fixture")) {
                if (fi >= 256)
                    throw std::runtime_error("Fixture budget");
                auto sh = fx.child("Shape");
                const std::string type = sh.attribute("xsi:type").value();
                const auto lc = point(sh.child("LocalCenter"));
                const math::Vec2 fc{lc.x - com.x, lc.y - com.y};
                std::vector<math::Vec2> vertices;
                double radius = scalar(sh.child("Radius"), .5);
                if (type == "Rectangle" || type == "Plane") {
                    const double width = type == "Plane" ? sh.attribute("planeSize").as_double(700)
                                                         : scalar(sh.child("Width"));
                    const double height = type == "Plane" ? 50 : scalar(sh.child("Height"));
                    if (!std::isfinite(width) || width <= 0 || width > 1000 ||
                        !std::isfinite(height) || height <= 0 || height > 1000)
                        throw std::runtime_error("Invalid rectangle/plane size");
                    const double y = type == "Plane" ? -height / 2 : 0;
                    const double rot = scalar(sh.child("LocalRotation")) * std::numbers::pi / 180;
                    for (auto q : std::vector<math::Vec2>{{-width / 2, y - height / 2},
                                                          {width / 2, y - height / 2},
                                                          {width / 2, y + height / 2},
                                                          {-width / 2, y + height / 2}}) {
                        q = rotate(q, rot);
                        vertices.push_back({q.x + fc.x, q.y + fc.y});
                    }
                    if (type == "Plane" && mode != "INFINITE")
                        throw std::runtime_error("Dynamic plane unsupported");
                } else if (type == "Polygon" || type == "Triangle") {
                    for (auto q : sh.children("Vertex")) {
                        if (vertices.size() >= 64)
                            throw std::runtime_error("Polygon vertex budget (3..64)");
                        auto pt = point(q);
                        vertices.push_back({pt.x - com.x, pt.y - com.y});
                    }
                    if (vertices.size() < 3 || vertices.size() > 64)
                        throw std::runtime_error("Polygon vertex budget (3..64)");
                } else if (type != "Circle")
                    throw std::runtime_error("Unsupported rigid shape: " + type);
                auto filter = fx.child("Filter");
                const std::string ft = filter.attribute("xsi:type").value();
                if (filter && ft != "CategoryFilter" && ft != "DefaultFilter")
                    throw std::runtime_error("Unsupported collision filter");
                const auto category =
                    ft == "CategoryFilter" ? bits(filter.child("PartOfGroups")) : 1;
                const auto mask = ft == "CategoryFilter" ? bits(filter.child("CollideWithGroups"))
                                                         : 2147483647ULL;
                if (fx.child("Sticky").text().as_bool(false))
                    throw std::runtime_error("Sticky fixture unsupported");
                const auto geometry =
                    vertices.empty() ? std::vector<std::vector<math::Vec2>>{{}} : pieces(vertices);
                for (const auto &piece : geometry) {
                    if (fi >= 256)
                        throw std::runtime_error("Decomposed fixture budget");
                    vertices = piece;
                    Value f(p->context, JS_NewObject(p->context)),
                        vs(p->context, JS_NewArray(p->context));
                    for (unsigned k = 0; k < vertices.size(); ++k) {
                        JS_SetPropertyUint32(p->context, vs.value, k,
                                             vector(p->context, vertices[k]));
                        bound = std::max(bound, std::hypot(vertices[k].x, vertices[k].y));
                    }
                    if (vertices.empty())
                        bound = std::max(bound, std::hypot(fc.x, fc.y) + radius);
                    JS_SetPropertyStr(p->context, f.value, "vertices",
                                      JS_DupValue(p->context, vs.value));
                    JS_SetPropertyStr(p->context, f.value, "center", vector(p->context, fc));
                    put(p->context, f.value, "radius", radius);
                    put(p->context, f.value, "friction", scalar(fx.child("Friction"), .2));
                    put(p->context, f.value, "restitution", scalar(fx.child("Restitution")));
                    put(p->context, f.value, "sensor",
                        fx.child("Sensor").text().as_bool(false) ? 1 : 0);
                    put(p->context, f.value, "category", double(category));
                    put(p->context, f.value, "mask", double(mask));
                    JS_SetPropertyUint32(p->context, fs.value, fi++,
                                         JS_DupValue(p->context, f.value));
                }
            }
            JS_SetPropertyStr(p->context, v.value, "fixtures", JS_DupValue(p->context, fs.value));
            JS_SetPropertyStr(p->context, v.value, "com", vector(p->context, com));
            put(p->context, v.value, "radius", bound);
            put(p->context, v.value, "category", 1);
            put(p->context, v.value, "mask", 2147483647);
            put(p->context, v.value, "sensor", 0);
            put(p->context, v.value, "mass", scalar(mass.child("Mass")));
            put(p->context, v.value, "inertia", scalar(mass.child("Inertia")));
            put(p->context, v.value, "friction", scalar(fixture.child("Friction"), .2));
            put(p->context, v.value, "restitution", scalar(fixture.child("Restitution"), 0));
            put(p->context, v.value, "damping", scalar(body.child("LinearDamping")));
            put(p->context, v.value, "angularDamping", scalar(body.child("AngularDamping")));
            put(p->context, v.value, "gravityScale", scalar(body.child("GravityScale"), 1));
            put(p->context, v.value, "fixed", mode == "FIXED_ANGULAR_VELOCITY" ? 1 : 0);
            put(p->context, v.value, "static", mode == "INFINITE" ? 1 : 0);
            JS_SetPropertyStr(p->context, v.value, "fill",
                              color(p->context, body.child("FillColor")));
            JS_SetPropertyStr(p->context, v.value, "outline",
                              color(p->context, body.child("OutlineColor")));
            std::string brush = body.child("Brush").attribute("name").value();
            if (brush.rfind("Pattern_", 0) == 0)
                brush.clear();
            JS_SetPropertyStr(p->context, v.value, "image",
                              JS_NewString(p->context, brush.c_str()));
            JS_SetPropertyUint32(p->context, bodies.value, id - 1,
                                 JS_DupValue(p->context, v.value));
        }
        if (!index)
            throw std::runtime_error(
                "No supported 2D rigid bodies; scene needs another simulation domain");
        unsigned joint = 0, distance_index = 0;
        for (auto node : world.child("Joints").children("Joint")) {
            if (node.child("DynamicallyAddedJoint").text().as_bool(false))
                continue;
            const std::string jt = node.attribute("xsi:type").value();
            if (jt != "DistanceJoint" && jt != "RevoluteJoint" && jt != "SpindleJoint" &&
                jt != "SpringJoint" && jt != "RopeJoint" && jt != "WeldJoint")
                throw std::runtime_error("Unsupported joint profile: " + jt);
            const double source_frequency = scalar(node.child("Frequency"));
            if (source_frequency < 0 || (source_frequency != 0 && jt != "DistanceJoint" &&
                                         jt != "SpringJoint" && jt != "WeldJoint"))
                throw std::runtime_error("Unsupported elastic parameters for joint: " + jt);
            auto ia = ids.find(node.child("BodyId1").child_value()),
                 ib = ids.find(node.child("BodyId2").child_value());
            if (ia == ids.end() || ib == ids.end() || ia->second == 0)
                throw std::runtime_error("Unresolved authored distance joint");
            auto a = point(node.child((jt == "RevoluteJoint" || jt == "WeldJoint") ? "Anchor"
                                                                                   : "Anchor1")),
                 b = point(node.child((jt == "RevoluteJoint" || jt == "WeldJoint") ? "Anchor"
                                                                                   : "Anchor2"));
            const auto wa = a, wb = b;
            auto ca = centers.at(ia->second);
            a = rotate({a.x - ca.x, a.y - ca.y}, -angles.at(ia->second));
            if (ib->second) {
                auto cb = centers.at(ib->second);
                b = rotate({b.x - cb.x, b.y - cb.y}, -angles.at(ib->second));
            }
            if (jt == "WeldJoint") {
                scene::WeldLink w;
                w.id = {3073ULL + joint++};
                w.body_a = {ia->second};
                w.body_b = {ib->second};
                w.local_a = a;
                w.local_b = b;
                w.reference = -scalar(node.child("ReferenceAngle"));
                w.frequency = scalar(node.child("Frequency"));
                w.damping_ratio = scalar(node.child("DampingRatio"));
                w.collide_connected = node.child("CollisionAllowed").text().as_bool(false);
                p->definition.welds.push_back(w);
                continue;
            }
            if (jt == "RevoluteJoint") {
                scene::HingeLink h;
                h.id = {1025ULL + joint++};
                h.body_a = {ia->second};
                h.body_b = {ib->second};
                h.local_a = a;
                h.local_b = b;
                h.reference = -scalar(node.child("ReferenceAngle"));
                h.lower = -scalar(node.child("UpperLimit"));
                h.upper = -scalar(node.child("LowerLimit"));
                h.limit = node.child("LimitEnabled").text().as_bool(false);
                h.motor = node.child("MotorEnabled").text().as_bool(false);
                h.speed = -scalar(node.child("MotorSpeed"));
                h.max_torque = scalar(node.child("MaximumMotorTorque"));
                h.collide_connected = node.child("CollisionAllowed").text().as_bool(false);
                p->definition.hinges.push_back(h);
                continue;
            }
            if (jt == "SpindleJoint") {
                if (!ib->second)
                    throw std::runtime_error("Winding needs two explicit bodies");
                const double length = std::hypot(wb.x - wa.x, wb.y - wa.y);
                if (length < .01)
                    throw std::runtime_error("Invalid winding anchors");
                const math::Vec2 n{(wb.x - wa.x) / length, (wb.y - wa.y) / length};
                auto spool_a = centers.at(ia->second), spool_b = centers.at(ib->second);
                scene::WindingLink w;
                w.id = {2049ULL + joint++};
                w.body_a = {ia->second};
                w.body_b = {ib->second};
                w.local_a = a;
                w.local_b = b;
                w.radius_a = (wa.x - spool_a.x) * n.y - (wa.y - spool_a.y) * n.x;
                w.radius_b = (wb.x - spool_b.x) * n.y - (wb.y - spool_b.y) * n.x;
                w.collide_connected = node.child("CollisionAllowed").text().as_bool(false);
                p->definition.windings.push_back(w);
                continue;
            }
            Value v(p->context, JS_NewObject(p->context));
            put(p->context, v.value, "id", ++joint);
            put(p->context, v.value, "a", ia->second);
            put(p->context, v.value, "b", ib->second);
            const double length = scalar(node.child(jt == "SpringJoint" ? "distance" : "Distance"),
                                         std::hypot(wb.x - wa.x, wb.y - wa.y));
            put(p->context, v.value, "length", length);
            double stiffness = jt == "SpringJoint" ? scalar(node.child("SpringConstant")) : 0;
            const double frequency = scalar(node.child("Frequency"));
            if (jt == "DistanceJoint" && frequency > 0) {
                const auto inv = [&](unsigned body_id) {
                    if (!body_id)
                        return 0.0;
                    Value body_value(p->context,
                                     JS_GetPropertyUint32(p->context, bodies.value, body_id - 1));
                    return num(p->context, body_value.value, "static")
                               ? 0.0
                               : 1.0 / num(p->context, body_value.value, "mass");
                };
                const double inverse_mass = inv(ia->second) + inv(ib->second);
                if (inverse_mass <= 0)
                    throw std::runtime_error("Spring requires dynamic mass");
                stiffness =
                    4 * std::numbers::pi * std::numbers::pi * frequency * frequency / inverse_mass;
            }
            put(p->context, v.value, "stiffness", stiffness);
            put(p->context, v.value, "elastic", jt == "SpringJoint" || stiffness > 0 ? 1 : 0);
            put(p->context, v.value, "damping",
                jt == "SpringJoint" ? 0 : scalar(node.child("DampingRatio")));
            if (jt == "SpringJoint")
                put(p->context, v.value, "coefficient", scalar(node.child("DampingRatio")));
            put(p->context, v.value, "collision",
                node.child("CollisionAllowed").text().as_bool(false) ? 1 : 0);
            if (jt == "RopeJoint") {
                // Independent lower/upper limit flags select the active unilateral bounds.
                const bool lower = node.child("LowerLimitEnabled").text().as_bool(false);
                const bool upper = node.child("UpperLimitEnabled").text().as_bool(true);
                put(p->context, v.value, "limit", 1);
                put(p->context, v.value, "minimum", lower ? scalar(node.child("LowerLimit")) : .01);
                put(p->context, v.value, "maximum",
                    upper ? scalar(node.child("UpperLimit"), length) : 20000);
            }
            JS_SetPropertyStr(p->context, v.value, "pa", vector(p->context, a));
            JS_SetPropertyStr(p->context, v.value, "pb", vector(p->context, b));
            JS_SetPropertyStr(p->context, v.value, "color",
                              color(p->context, node.child("JointColor")));
            JS_SetPropertyUint32(p->context, links.value, distance_index++,
                                 JS_DupValue(p->context, v.value));
        }
        JS_SetPropertyStr(p->context, seed.value, "bodies", JS_DupValue(p->context, bodies.value));
        JS_SetPropertyStr(p->context, seed.value, "links", JS_DupValue(p->context, links.value));
        Value global(p->context, JS_GetGlobalObject(p->context));
        JS_SetPropertyStr(p->context, global.value, "__seed", JS_DupValue(p->context, seed.value));
        pugi::xml_document gui;
        const auto source = world.child("GuiManager").child("GuiXML").child_value();
        if (*source && !gui.load_string(source))
            throw std::runtime_error("Invalid GUI XML");
        double row = 16;
        std::function<void(pugi::xml_node, unsigned)> widgets = [&](pugi::xml_node parent,
                                                                    unsigned depth) {
            if (depth > 16)
                throw std::runtime_error("Widget nesting budget");
            for (auto node : parent.children()) {
                const std::string kind = node.name();
                if (kind == "dialog" || kind == "panel") {
                    widgets(node, depth + 1);
                    continue;
                }
                if (kind != "button" && kind != "textarea" && kind != "label" && kind != "slider")
                    throw std::runtime_error("Unsupported embedded widget: " + kind);
                if (p->widgets.size() >= 64)
                    throw std::runtime_error("Widget budget");
                SourceWidget w;
                w.button = kind == "button";
                w.slider = kind == "slider";
                w.text = node.attribute("text").value();
                w.action = node.attribute("action").value();
                w.name = node.attribute("name").value();
                if (w.text.size() > 65536 || w.action.size() > 4096 || w.name.size() > 128)
                    throw std::runtime_error("Widget text budget");
                w.position = {16, row};
                w.size = {300, w.button || w.slider ? 36.0 : 80.0};
                std::string bounds = node.attribute("rectbounds").value();
                if (!bounds.empty()) {
                    for (auto &ch : bounds)
                        if (ch == ',')
                            ch = ' ';
                    std::istringstream in(bounds);
                    in.imbue(std::locale::classic());
                    if (!(in >> w.position.x >> w.position.y >> w.size.x >> w.size.y))
                        throw std::runtime_error("Invalid widget bounds");
                }
                if (!math::finite(w.position) || !math::finite(w.size) || w.size.x <= 0 ||
                    w.size.y <= 0 || w.size.x > 4096 || w.size.y > 4096 ||
                    std::abs(w.position.x) > 10000 || std::abs(w.position.y) > 10000)
                    throw std::runtime_error("Invalid widget bounds");
                row += w.size.y + 8;
                if (w.slider) {
                    if (w.name.empty() ||
                        !(std::isalpha(static_cast<unsigned char>(w.name[0])) ||
                          w.name[0] == '_') ||
                        !std::all_of(w.name.begin(), w.name.end(),
                                     [](unsigned char c) { return std::isalnum(c) || c == '_'; }))
                        throw std::runtime_error("Invalid slider variable");
                    w.minimum = node.attribute("minimum").as_double(0);
                    w.maximum = node.attribute("maximum").as_double(1);
                    w.value = node.attribute("value").as_double(0);
                    if (!std::isfinite(w.minimum) || !std::isfinite(w.maximum) ||
                        !std::isfinite(w.value) || w.minimum >= w.maximum || w.value < w.minimum ||
                        w.value > w.maximum || std::abs(w.minimum) > 1e9 ||
                        std::abs(w.maximum) > 1e9)
                        throw std::runtime_error("Invalid slider range");
                    JS_SetPropertyStr(p->context, global.value, w.name.c_str(),
                                      JS_NewFloat64(p->context, w.value));
                }
                if (w.button && w.action.empty() && !w.name.empty())
                    w.action = w.name + "_onClick()";
                p->widgets.push_back(std::move(w));
            }
        };
        widgets(gui.child("desktop"), 0);
        for (auto node : world.child("BodyControllers").children("BodyController")) {
            if (p->controllers.size() >= 1024)
                throw std::runtime_error("Controller budget");
            const auto id = ids.find(node.attribute("bodyid").value());
            if (id == ids.end() || !id->second)
                throw std::runtime_error("Unresolved controller body");
            Impl::Controller c;
            c.body = {id->second};
            c.initial_angle = angles.at(id->second);
            const std::string kind = node.attribute("type").value();
            if (kind == "ForceController") {
                c.enabled = node.attribute("enabled").value();
                if (c.enabled.empty())
                    c.enabled = "true";
                c.x = node.child("xExpr").child_value();
                c.y = node.child("yExpr").child_value();
                c.point = point(node.child("ExtForcePoint"));
                const double mode_value = scalar(node.child("ForceMode"));
                if (mode_value < 0 || mode_value > 2 || std::floor(mode_value) != mode_value)
                    throw std::runtime_error("Invalid force mode");
                c.mode = static_cast<int>(mode_value);
                if (c.mode < 0 || c.mode > 2)
                    throw std::runtime_error("Unsupported force mode");
            } else if (kind == "ValuePropertiesController") {
                std::istringstream fields(node.attribute("enabled").value());
                std::string expr;
                unsigned i = 0;
                while (std::getline(fields, expr, ';')) {
                    if (!expr.empty() && expr != "null") {
                        if (i != 4)
                            throw std::runtime_error("Unsupported value property index: " +
                                                     std::to_string(i));
                        c.friction = expr;
                    }
                    ++i;
                }
                if (i > 15)
                    throw std::runtime_error("Property expression budget");
            } else
                throw std::runtime_error("Unsupported controller: " + kind);
            if (c.enabled.size() + c.x.size() + c.y.size() + c.friction.size() > 4096)
                throw std::runtime_error("Controller expression budget");
            p->controllers.push_back(std::move(c));
        }
        { Value boot(p->context, p->eval(bridge)); }
        const std::string script = world.child("ScriptManager").child("Script").child_value();
        if (script.size() > 1024 * 1024)
            throw std::runtime_error("Script byte budget");
        if (!script.empty()) {
            Value run(p->context, p->eval(script));
        }
        p->collect();
        for (const auto &[id, style] : p->styles) {
            static_cast<void>(id);
            if (!style.image.empty() && !p->images.contains(style.image))
                p->images.emplace(style.image, member(project, style.image, 16 * 1024 * 1024));
        }
        p->ready = true;
        MechanicalSource candidate(p);
        scene::MechanismSnapshot initial;
        for (const auto &b : p->definition.bodies)
            initial.bodies.push_back({b.id, b.center, b.velocity, b.angle, b.angular_velocity});
        const auto controls = candidate.controls(initial);
        if (controls.error())
            throw std::runtime_error(controls.error()->message);
        return core::Result<MechanicalSource>::success(MechanicalSource(std::move(p)));
    } catch (const std::exception &e) {
        return core::Result<MechanicalSource>::failure({core::Code::unsupported_feature,
                                                        core::Severity::error,
                                                        e.what(),
                                                        {},
                                                        "compat.mechanism"});
    }
}
const scene::Mechanism &MechanicalSource::definition() const { return impl_->definition; }
const std::map<core::EntityId, BodyStyle> &MechanicalSource::styles() const {
    return impl_->styles;
}
const std::map<core::EntityId, scene::Color> &MechanicalSource::joint_colors() const {
    return impl_->joint_colors;
}
const std::map<std::string, std::vector<std::uint8_t>> &MechanicalSource::images() const {
    return impl_->images;
}
const std::vector<SourceWidget> &MechanicalSource::widgets() const { return impl_->widgets; }
math::Vec2 MechanicalSource::camera_center() const { return impl_->camera; }
double MechanicalSource::camera_scale() const { return impl_->scale; }
core::Result<void> MechanicalSource::apply_action(std::string_view action) {
    try {
        if (impl_->poisoned || action.size() > 4096)
            throw std::runtime_error("Reopen source after failed script action");
        Value result(impl_->context, impl_->eval(action));
        impl_->collect();
        return core::Result<void>::success();
    } catch (const std::exception &e) {
        impl_->poisoned = true;
        return core::Result<void>::failure({core::Code::unsupported_feature,
                                            core::Severity::error,
                                            e.what(),
                                            {},
                                            "compat.action"});
    }
}
core::Result<void> MechanicalSource::set_slider(std::size_t index, double value) {
    if (index >= impl_->widgets.size() || !impl_->widgets[index].slider || !std::isfinite(value) ||
        value < impl_->widgets[index].minimum || value > impl_->widgets[index].maximum)
        return core::Result<void>::failure({core::Code::invalid_argument,
                                            core::Severity::error,
                                            "Invalid slider value",
                                            {},
                                            "compat.controls"});
    auto &w = impl_->widgets[index];
    Value global(impl_->context, JS_GetGlobalObject(impl_->context));
    JS_SetPropertyStr(impl_->context, global.value, w.name.c_str(),
                      JS_NewFloat64(impl_->context, value));
    w.value = value;
    return core::Result<void>::success();
}
core::Result<SourceControls> MechanicalSource::controls(const scene::MechanismSnapshot &state) {
    try {
        if (impl_->poisoned)
            throw std::runtime_error("Reopen source after failed controller");
        SourceControls result;
        Value global(impl_->context, JS_GetGlobalObject(impl_->context));
        put(impl_->context, global.value, "T", state.time);
        put(impl_->context, global.value, "t", state.time);
        const auto expression = [&](const std::string &code) {
            Value n(impl_->context, impl_->eval("(" + code + ")"));
            return number(impl_->context, n.value);
        };
        for (const auto &c : impl_->controllers) {
            if (!c.friction.empty()) {
                const double mu = expression(c.friction);
                if (mu < 0 || mu > 1e6)
                    throw std::runtime_error("Invalid friction expression");
                result.friction[c.body] = mu;
                continue;
            }
            Value enabled(impl_->context, impl_->eval("Boolean(" + c.enabled + ")"));
            if (!JS_ToBool(impl_->context, enabled.value))
                continue;
            scene::AppliedForce f;
            f.body = c.body;
            f.force = {expression(c.x), expression(c.y)};
            f.local_point = c.point;
            auto body = std::find_if(state.bodies.begin(), state.bodies.end(),
                                     [&](const auto &b) { return b.id == c.body; });
            if (body == state.bodies.end())
                throw std::runtime_error("Missing controlled body");
            if (c.mode == 1)
                f.force = rotate(f.force, body->angle);
            if (c.mode == 2) {
                f.wrapped = true;
                auto point = rotate(c.point, c.initial_angle);
                const double length = std::hypot(f.force.x, f.force.y);
                f.torque_arm =
                    length > 0 ? (point.x * f.force.y - point.y * f.force.x) / length : 0;
            }
            if (std::abs(f.force.x) > 1e9 || std::abs(f.force.y) > 1e9)
                throw std::runtime_error("Force expression envelope");
            result.forces.push_back(f);
        }
        return core::Result<SourceControls>::success(std::move(result));
    } catch (const std::exception &e) {
        impl_->poisoned = true;
        return core::Result<SourceControls>::failure({core::Code::unsupported_feature,
                                                      core::Severity::error,
                                                      e.what(),
                                                      {},
                                                      "compat.controls"});
    }
}
} // namespace opensim::compat
