#include <chrono>
#include <cmath>
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
let __bodies=__seed.bodies, __links=__seed.links, __next=__bodies.length+1, __joint=__links.length+1;
class Body {
 constructor(d){this.d=d;}
 setFillColor(c){this.d.fill=c.rgba.slice();}
 setPosition(v){this.d.center={x:v.x,y:v.y};this.d.velocity={x:0,y:0};}
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
Math.random=()=>{throw Error('Random script API not supported');};
)JS";
} // namespace
struct MechanicalSource::Impl {
    JSRuntime *runtime = nullptr;
    JSContext *context = nullptr;
    std::chrono::steady_clock::time_point deadline{};
    scene::Mechanism definition;
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
            auto c = field(context, v.value, "color");
            next_colors.emplace(j.id, rgba(context, c.value));
            next.links.push_back(j);
        }
        const auto valid = scene::validate(next);
        if (valid.error())
            throw std::runtime_error(valid.error()->message);
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
        p->definition.gravity = point(world.child("Gravity"));
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
            if (std::string(body.name()) != "Body")
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
            if (fixture.next_sibling())
                throw std::runtime_error("Compound fixtures require full rigid profile");
            auto shape = fixture.child("Shape");
            if (std::string(shape.attribute("xsi:type").value()) != "Circle")
                throw std::runtime_error("Non-circle shape requires source preview");
            if (mode != "NORMAL" && mode != "INFINITE" && mode != "FIXED_ANGULAR_VELOCITY")
                throw std::runtime_error("Unsupported source mass mode");
            auto local = point(shape.child("LocalCenter"));
            auto com = point(mass.child("LocalCenter"));
            if (std::hypot(local.x - com.x, local.y - com.y) > 1e-10)
                throw std::runtime_error("Off-center circle requires full rigid profile");
            double angle =
                scalar(body.child("Transform").child("Rotation")) * std::numbers::pi / 180;
            auto origin = point(body.child("Transform").child("Translation")),
                 offset = rotate(local, angle);
            math::Vec2 center{origin.x + offset.x, origin.y + offset.y};
            double angular = scalar(body.child("AngularVelocity")) * std::numbers::pi / 180;
            if (mode == "FIXED_ANGULAR_VELOCITY" && std::abs(angular) > 1e-12)
                throw std::runtime_error("Prescribed nonzero angular velocity unsupported");
            if ((body.child("AccumulatedForce") &&
                 point(body.child("AccumulatedForce")) != math::Vec2{}) ||
                scalar(body.child("AccumulatedTorque")) != 0 || scalar(body.child("Charge")) != 0 ||
                fixture.child("Filter") || fixture.child("Sensor") ||
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
            put(p->context, v.value, "radius", scalar(shape.child("Radius")));
            put(p->context, v.value, "mass", scalar(mass.child("Mass")));
            put(p->context, v.value, "inertia", scalar(mass.child("Inertia")));
            put(p->context, v.value, "friction", scalar(fixture.child("Friction"), .3));
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
            JS_SetPropertyStr(
                p->context, v.value, "image",
                JS_NewString(p->context, body.child("Brush").attribute("name").value()));
            JS_SetPropertyUint32(p->context, bodies.value, id - 1,
                                 JS_DupValue(p->context, v.value));
        }
        if (!index)
            throw std::runtime_error("No circular mechanical bodies");
        unsigned joint = 0;
        for (auto node : world.child("Joints").children("Joint")) {
            if (node.child("DynamicallyAddedJoint").text().as_bool(false))
                continue;
            if (std::string(node.attribute("xsi:type").value()) != "DistanceJoint" ||
                scalar(node.child("Frequency")) != 0)
                throw std::runtime_error("Unsupported joint profile");
            auto ia = ids.find(node.child("BodyId1").child_value()),
                 ib = ids.find(node.child("BodyId2").child_value());
            if (ia == ids.end() || ib == ids.end() || ia->second == 0)
                throw std::runtime_error("Unresolved authored distance joint");
            auto a = point(node.child("Anchor1")), b = point(node.child("Anchor2"));
            auto ca = centers.at(ia->second);
            a = rotate({a.x - ca.x, a.y - ca.y}, -angles.at(ia->second));
            if (ib->second) {
                auto cb = centers.at(ib->second);
                b = rotate({b.x - cb.x, b.y - cb.y}, -angles.at(ib->second));
            }
            Value v(p->context, JS_NewObject(p->context));
            put(p->context, v.value, "id", ++joint);
            put(p->context, v.value, "a", ia->second);
            put(p->context, v.value, "b", ib->second);
            put(p->context, v.value, "length", scalar(node.child("Distance")));
            JS_SetPropertyStr(p->context, v.value, "pa", vector(p->context, a));
            JS_SetPropertyStr(p->context, v.value, "pb", vector(p->context, b));
            JS_SetPropertyStr(p->context, v.value, "color",
                              color(p->context, node.child("JointColor")));
            JS_SetPropertyUint32(p->context, links.value, joint - 1,
                                 JS_DupValue(p->context, v.value));
        }
        JS_SetPropertyStr(p->context, seed.value, "bodies", JS_DupValue(p->context, bodies.value));
        JS_SetPropertyStr(p->context, seed.value, "links", JS_DupValue(p->context, links.value));
        Value global(p->context, JS_GetGlobalObject(p->context));
        JS_SetPropertyStr(p->context, global.value, "__seed", JS_DupValue(p->context, seed.value));
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
        pugi::xml_document gui;
        const auto source = world.child("GuiManager").child("GuiXML").child_value();
        if (*source && !gui.load_string(source))
            throw std::runtime_error("Invalid GUI XML");
        for (auto node : gui.child("desktop").children()) {
            const std::string kind = node.name();
            if (kind != "button" && kind != "textarea")
                throw std::runtime_error("Unsupported embedded widget");
            if (p->widgets.size() >= 64)
                throw std::runtime_error("Widget budget");
            SourceWidget w;
            w.button = kind == "button";
            w.text = node.attribute("text").value();
            w.action = node.attribute("action").value();
            if (w.text.size() > 65536 || w.action.size() > 4096)
                throw std::runtime_error("Widget text budget");
            std::string bounds = node.attribute("rectbounds").value();
            for (auto &ch : bounds)
                if (ch == ',')
                    ch = ' ';
            std::istringstream in(bounds);
            in.imbue(std::locale::classic());
            if (!(in >> w.position.x >> w.position.y >> w.size.x >> w.size.y) ||
                !math::finite(w.position) || !math::finite(w.size) || w.size.x <= 0 ||
                w.size.y <= 0 || std::abs(w.position.x) > 10000 || std::abs(w.position.y) > 10000 ||
                w.size.x > 4096 || w.size.y > 4096)
                throw std::runtime_error("Invalid widget bounds");
            p->widgets.push_back(std::move(w));
        }
        p->ready = true;
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
} // namespace opensim::compat
