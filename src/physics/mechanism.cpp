#include <algorithm>
#include <box2d/box2d.h>
#include <cmath>
#include <map>
#include <numbers>
#include <opensim/physics/mechanism.hpp>
namespace opensim::physics {
namespace {
b2Vec2 native(math::Vec2 v) { return {static_cast<float>(v.x), static_cast<float>(v.y)}; }
math::Vec2 value(b2Vec2 v) { return {v.x, v.y}; }
float mix_min(float a, int, float b, int) { return std::min(a, b); }
float mix_max(float a, int, float b, int) { return std::max(a, b); }
float mix_mean(float a, int, float b, int) { return std::sqrt(a * b); }
b2FrictionCallback *mixer(scene::MaterialMixer mode) {
    switch (mode) {
    case scene::MaterialMixer::minimum:
        return mix_min;
    case scene::MaterialMixer::maximum:
        return mix_max;
    default:
        return mix_mean;
    }
}
core::Result<void> bad(const char *m) {
    return core::Result<void>::failure(
        {core::Code::invalid_argument, core::Severity::error, m, {}, "physics.mechanism"});
}
} // namespace
struct Mechanism::Impl {
    scene::Mechanism initial;
    b2WorldId world{};
    b2BodyId ground{};
    std::map<core::EntityId, b2BodyId> bodies;
    std::uint64_t steps = 0;
    bool poisoned = false;
    scene::MechanismSnapshot published;
    std::vector<scene::AppliedForce> applied;
    struct WindingState {
        scene::WindingLink link;
        double constant, a, b, previous_a, previous_b, phi;
    };
    std::vector<WindingState> windings;
    static double unwrap(double x, double previous) {
        return std::remainder(x - previous, 2 * std::numbers::pi);
    }
    static double cross(b2Vec2 a, b2Vec2 b) { return double(a.x) * b.y - double(a.y) * b.x; }
    void winding_step(double dt) {
        for (auto &w : windings) {
            auto a = bodies.at(w.link.body_a), b = bodies.at(w.link.body_b);
            auto p = b2Body_GetPosition(a), q = b2Body_GetPosition(b);
            const double dx = q.x - p.x, dy = q.y - p.y, d = std::hypot(dx, dy);
            const double delta = w.link.radius_a - w.link.radius_b;
            if (d <= std::abs(delta) + 1e-5) {
                poisoned = true;
                return;
            }
            const double phi = std::atan2(dy, dx) + std::asin(delta / d);
            w.phi += unwrap(phi, w.phi);
            const double aa = b2Rot_GetAngle(b2Body_GetRotation(a)),
                         bb = b2Rot_GetAngle(b2Body_GetRotation(b));
            w.a += unwrap(aa, w.previous_a);
            w.b += unwrap(bb, w.previous_b);
            w.previous_a = aa;
            w.previous_b = bb;
            b2Vec2 n{float(std::cos(phi)), float(std::sin(phi))};
            const auto va = b2Body_GetLinearVelocity(a), vb = b2Body_GetLinearVelocity(b);
            const auto ma = b2Body_GetMassData(a), mb = b2Body_GetMassData(b);
            const double ia = b2Body_GetType(a) == b2_dynamicBody ? 1.0 / ma.mass : 0;
            const double ib = b2Body_GetType(b) == b2_dynamicBody ? 1.0 / mb.mass : 0;
            const double ja = ma.rotationalInertia > 0 ? 1.0 / ma.rotationalInertia : 0;
            const double jb = mb.rotationalInertia > 0 ? 1.0 / mb.rotationalInertia : 0;
            const double k = ia + ib + w.link.radius_a * w.link.radius_a * ja +
                             w.link.radius_b * w.link.radius_b * jb;
            if (k <= 0)
                continue;
            const double error = std::sqrt(d * d - delta * delta) + delta * w.phi +
                                 w.link.radius_b * w.b - w.link.radius_a * w.a - w.constant;
            const double velocity = double(n.x) * (vb.x - va.x) + double(n.y) * (vb.y - va.y) +
                                    w.link.radius_b * b2Body_GetAngularVelocity(b) -
                                    w.link.radius_a * b2Body_GetAngularVelocity(a);
            const double impulse = -(velocity + std::clamp(.1 * error / dt, -10.0, 10.0)) / k;
            b2Body_ApplyLinearImpulseToCenter(a, {-float(impulse * n.x), -float(impulse * n.y)},
                                              true);
            b2Body_ApplyLinearImpulseToCenter(b, {float(impulse * n.x), float(impulse * n.y)},
                                              true);
            b2Body_ApplyAngularImpulse(a, float(-impulse * w.link.radius_a), true);
            b2Body_ApplyAngularImpulse(b, float(impulse * w.link.radius_b), true);
        }
    }
    explicit Impl(const scene::Mechanism &m) : initial(m) {
        auto w = b2DefaultWorldDef();
        w.gravity = native(m.gravity);
        w.frictionCallback = mixer(m.friction_mixer);
        w.restitutionCallback = mixer(m.restitution_mixer);
        w.enableSleep = false;
        w.enableContinuous = true;
        w.restitutionThreshold = .01f;
        w.maximumLinearSpeed = 10000;
        world = b2CreateWorld(&w);
        auto g = b2DefaultBodyDef();
        ground = b2CreateBody(world, &g);
        for (const auto &b : m.bodies) {
            auto d = b2DefaultBodyDef();
            d.type = b.static_body ? b2_staticBody : b2_dynamicBody;
            d.position = native(b.center);
            d.rotation = b2MakeRot(static_cast<float>(b.angle));
            d.linearVelocity = native(b.velocity);
            d.angularVelocity = static_cast<float>(b.angular_velocity);
            d.linearDamping = static_cast<float>(b.damping);
            d.angularDamping = static_cast<float>(b.angular_damping);
            d.gravityScale = static_cast<float>(b.gravity_scale);
            d.fixedRotation = b.fixed_rotation;
            d.isBullet = !b.static_body;
            auto id = b2CreateBody(world, &d);
            bodies.emplace(b.id, id);
            auto shape = b2DefaultShapeDef();
            shape.material.friction = static_cast<float>(b.friction);
            shape.material.restitution = static_cast<float>(b.restitution);
            shape.filter.categoryBits = b.category;
            shape.filter.maskBits = b.mask;
            shape.isSensor = b.sensor;
            if (b.fixtures.empty()) {
                b2Circle circle{{0, 0}, static_cast<float>(b.radius)};
                b2CreateCircleShape(id, &shape, &circle);
            } else
                for (const auto &f : b.fixtures) {
                    shape.material.friction = float(f.friction);
                    shape.material.restitution = float(f.restitution);
                    shape.filter.categoryBits = f.category;
                    shape.filter.maskBits = f.mask;
                    shape.isSensor = f.sensor;
                    if (f.vertices.empty()) {
                        b2Circle circle{native(f.center), float(f.radius)};
                        b2CreateCircleShape(id, &shape, &circle);
                    } else {
                        b2Vec2 vertices[8];
                        for (std::size_t i = 0; i < f.vertices.size(); ++i)
                            vertices[i] = native(f.vertices[i]);
                        auto hull = b2ComputeHull(vertices, int(f.vertices.size()));
                        auto polygon = b2MakePolygon(&hull, 0);
                        b2CreatePolygonShape(id, &shape, &polygon);
                    }
                }
            if (!b.static_body)
                b2Body_SetMassData(id, {static_cast<float>(b.mass),
                                        {0, 0},
                                        b.fixed_rotation ? 0.0f : static_cast<float>(b.inertia)});
        }
        for (const auto &b : m.bodies)
            published.bodies.push_back({b.id, b.center, b.velocity, b.angle, b.angular_velocity});
        for (const auto &j : m.links) {
            auto d = b2DefaultDistanceJointDef();
            d.bodyIdA = bodies.at(j.body_a);
            d.bodyIdB = j.body_b.valid() ? bodies.at(j.body_b) : ground;
            d.localAnchorA = native(j.local_a);
            d.localAnchorB = native(j.local_b);
            d.length = static_cast<float>(j.length);
            d.collideConnected = j.collide_connected;
            b2CreateDistanceJoint(world, &d);
        }
        for (const auto &j : m.hinges) {
            auto d = b2DefaultRevoluteJointDef();
            d.bodyIdA = bodies.at(j.body_a);
            d.bodyIdB = j.body_b.valid() ? bodies.at(j.body_b) : ground;
            d.localAnchorA = native(j.local_a);
            d.localAnchorB = native(j.local_b);
            d.referenceAngle = float(j.reference);
            d.lowerAngle = float(j.lower);
            d.upperAngle = float(j.upper);
            d.enableLimit = j.limit;
            d.enableMotor = j.motor;
            d.motorSpeed = float(j.speed);
            d.maxMotorTorque = float(j.max_torque);
            d.collideConnected = j.collide_connected;
            b2CreateRevoluteJoint(world, &d);
        }
        for (const auto &j : m.windings) {
            const auto a = bodies.at(j.body_a), b = bodies.at(j.body_b);
            auto p = b2Body_GetPosition(a), q = b2Body_GetPosition(b);
            const double dx = q.x - p.x, dy = q.y - p.y, dist = std::hypot(dx, dy),
                         delta = j.radius_a - j.radius_b;
            const double phi = std::atan2(dy, dx) + std::asin(delta / dist);
            const double aa = b2Rot_GetAngle(b2Body_GetRotation(a)),
                         bb = b2Rot_GetAngle(b2Body_GetRotation(b));
            windings.push_back({j,
                                std::sqrt(dist * dist - delta * delta) + delta * phi +
                                    j.radius_b * bb - j.radius_a * aa,
                                aa, bb, aa, bb, phi});
            if (!j.collide_connected) {
                auto filter = b2DefaultFilterJointDef();
                filter.bodyIdA = a;
                filter.bodyIdB = b;
                b2CreateFilterJoint(world, &filter);
            }
        }
    }
    ~Impl() {
        if (B2_IS_NON_NULL(world))
            b2DestroyWorld(world);
    }
};
Mechanism::Mechanism(const scene::Mechanism &m) : impl_(std::make_unique<Impl>(m)) {}
Mechanism::~Mechanism() = default;
core::Result<std::shared_ptr<Mechanism>> Mechanism::create(const scene::Mechanism &m) {
    const auto valid = scene::validate(m);
    if (valid.error())
        return core::Result<std::shared_ptr<Mechanism>>::failure(*valid.error());
    for (const auto &body : m.bodies)
        for (const auto &fixture : body.fixtures)
            if (!fixture.vertices.empty()) {
                b2Vec2 vertices[8];
                for (std::size_t i = 0; i < fixture.vertices.size(); ++i)
                    vertices[i] = native(fixture.vertices[i]);
                auto hull = b2ComputeHull(vertices, int(fixture.vertices.size()));
                if (hull.count != int(fixture.vertices.size()) || !b2ValidateHull(&hull))
                    return core::Result<std::shared_ptr<Mechanism>>::failure(
                        {core::Code::invalid_argument, core::Severity::error,
                         "Polygon loses geometry at backend precision", body.id,
                         "physics.mechanism"});
            }
    return core::Result<std::shared_ptr<Mechanism>>::success(
        std::shared_ptr<Mechanism>(new Mechanism(m)));
}
scene::MechanismSnapshot Mechanism::snapshot() const { return impl_->published; }
core::Result<void> Mechanism::step() {
    if (impl_->poisoned || impl_->steps >= 9007199254740991ULL)
        return bad("Reset invalid or exhausted mechanism runtime");
    for (unsigned substep = 0; substep < 8; ++substep) {
        for (const auto &f : impl_->applied) {
            const auto id = impl_->bodies.at(f.body);
            if (f.wrapped) {
                b2Body_ApplyForceToCenter(id, native(f.force), true);
                b2Body_ApplyTorque(id, float(f.torque_arm * std::hypot(f.force.x, f.force.y)),
                                   true);
            } else
                b2Body_ApplyForce(id, native(f.force),
                                  b2Body_GetWorldPoint(id, native(f.local_point)), true);
        }
        b2World_Step(impl_->world, static_cast<float>(impl_->initial.fixed_dt / 8), 1);
        impl_->winding_step(impl_->initial.fixed_dt / 8);
        if (impl_->poisoned)
            return bad("Winding geometry exhausted; reset required");
    }
    ++impl_->steps;
    scene::MechanismSnapshot next;
    next.time = double(impl_->steps) * impl_->initial.fixed_dt;
    for (const auto &b : impl_->initial.bodies) {
        const auto id = impl_->bodies.at(b.id);
        next.bodies.push_back(
            {b.id, value(b2Body_GetPosition(id)), value(b2Body_GetLinearVelocity(id)),
             b2Rot_GetAngle(b2Body_GetRotation(id)), b2Body_GetAngularVelocity(id)});
    }
    for (const auto &b : next.bodies)
        if (!math::finite(b.center) || !math::finite(b.velocity) || !std::isfinite(b.angle) ||
            !std::isfinite(b.angular_velocity)) {
            impl_->poisoned = true;
            return bad("Nonfinite mechanism runtime; reset required");
        }
    impl_->published = std::move(next);
    return core::Result<void>::success();
}
core::Result<void> Mechanism::reset() {
    auto next = std::make_unique<Impl>(impl_->initial);
    impl_ = std::move(next);
    return core::Result<void>::success();
}
core::Result<void> Mechanism::forces(const std::vector<scene::AppliedForce> &forces) {
    if (impl_->poisoned || forces.size() > 1024)
        return bad("Invalid force budget/runtime");
    for (const auto &f : forces)
        if (!impl_->bodies.contains(f.body) || !math::finite(f.force) ||
            !math::finite(f.local_point) || std::abs(f.force.x) > 1e9 ||
            std::abs(f.force.y) > 1e9 || std::abs(f.local_point.x) > 1000 ||
            std::abs(f.local_point.y) > 1000 || !std::isfinite(f.torque_arm) ||
            std::abs(f.torque_arm) > 1000)
            return bad("Invalid applied force");
    impl_->applied = forces;
    return core::Result<void>::success();
}
core::Result<void> Mechanism::friction(core::EntityId id, double mu) {
    if (impl_->poisoned || !impl_->bodies.contains(id) || !std::isfinite(mu) || mu < 0 || mu > 1e6)
        return bad("Invalid friction update");
    auto body = impl_->bodies.at(id);
    const int n = b2Body_GetShapeCount(body);
    std::vector<b2ShapeId> shapes(static_cast<std::size_t>(n));
    b2Body_GetShapes(body, shapes.data(), n);
    for (auto shape : shapes)
        b2Shape_SetFriction(shape, float(mu));
    return core::Result<void>::success();
}
core::Result<void> Mechanism::relocate(core::EntityId id, math::Vec2 p) {
    if (impl_->poisoned || !math::finite(p) || std::abs(p.x) > 10000 || std::abs(p.y) > 10000)
        return bad("Invalid drag position or runtime");
    for (const auto &b : impl_->initial.bodies)
        if (b.id == id) {
            if (b.static_body)
                return bad("Cannot drag a static body");
            auto native_id = impl_->bodies.at(id);
            b2Body_SetTransform(native_id, native(p), b2Body_GetRotation(native_id));
            b2Body_SetLinearVelocity(native_id, {0, 0});
            b2Body_SetAngularVelocity(native_id, 0);
            for (auto &sample : impl_->published.bodies)
                if (sample.id == id) {
                    sample.center = p;
                    sample.velocity = {};
                    sample.angular_velocity = 0;
                }
            return core::Result<void>::success();
        }
    return bad("Drag body not found");
}
} // namespace opensim::physics
