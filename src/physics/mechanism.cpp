#include <algorithm>
#include <box2d/box2d.h>
#include <cmath>
#include <map>
#include <opensim/physics/mechanism.hpp>
namespace opensim::physics {
namespace {
b2Vec2 native(math::Vec2 v) { return {static_cast<float>(v.x), static_cast<float>(v.y)}; }
math::Vec2 value(b2Vec2 v) { return {v.x, v.y}; }
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
    explicit Impl(const scene::Mechanism &m) : initial(m) {
        auto w = b2DefaultWorldDef();
        w.gravity = native(m.gravity);
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
            b2Circle circle{{0, 0}, static_cast<float>(b.radius)};
            b2CreateCircleShape(id, &shape, &circle);
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
    return core::Result<std::shared_ptr<Mechanism>>::success(
        std::shared_ptr<Mechanism>(new Mechanism(m)));
}
scene::MechanismSnapshot Mechanism::snapshot() const { return impl_->published; }
core::Result<void> Mechanism::step() {
    if (impl_->poisoned || impl_->steps >= 9007199254740991ULL)
        return bad("Reset invalid or exhausted mechanism runtime");
    double remaining = impl_->initial.fixed_dt;
    double radius = 1000;
    for (const auto &b : impl_->initial.bodies)
        if (!b.sensor)
            radius = std::min(radius, b.radius);
    unsigned substeps = 0;
    while (remaining > impl_->initial.fixed_dt * 1e-10) {
        if (++substeps > 4096) {
            impl_->poisoned = true;
            return bad("Continuous collision step budget exceeded; reset required");
        }
        double speed = 0;
        for (const auto &[id, body] : impl_->bodies) {
            static_cast<void>(id);
            const auto v = b2Body_GetLinearVelocity(body);
            speed = std::max(speed, std::hypot(double(v.x), double(v.y)));
        }
        speed += std::hypot(impl_->initial.gravity.x, impl_->initial.gravity.y) *
                 impl_->initial.fixed_dt * 100;
        const double h = std::min(
            {remaining, impl_->initial.fixed_dt / 8, .25 * radius / std::max(speed, 1e-12)});
        b2World_Step(impl_->world, static_cast<float>(h), 1);
        remaining -= h;
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
core::Result<void> Mechanism::relocate(core::EntityId id, math::Vec2 p) {
    if (impl_->poisoned || !math::finite(p) || std::abs(p.x) > 10000 || std::abs(p.y) > 10000)
        return bad("Invalid drag position or runtime");
    for (const auto &b : impl_->initial.bodies)
        if (b.id == id) {
            if (b.static_body)
                return bad("Cannot drag a static body");
            const auto original = value(b2Body_GetPosition(impl_->bodies.at(id)));
            const auto delta = p - original;
            const double travel = math::dot(delta, delta);
            double fraction = 1;
            if (!b.sensor && travel > 0) {
                for (const auto &other : impl_->initial.bodies) {
                    if (other.id == id || other.sensor || !(b.category & other.mask) ||
                        !(other.category & b.mask))
                        continue;
                    bool connected = false;
                    for (const auto &j : impl_->initial.links)
                        if (!j.collide_connected && ((j.body_a == id && j.body_b == other.id) ||
                                                     (j.body_b == id && j.body_a == other.id)))
                            connected = true;
                    if (connected)
                        continue;
                    const auto center = value(b2Body_GetPosition(impl_->bodies.at(other.id)));
                    const auto offset = original - center;
                    const double radius_sum = b.radius + other.radius;
                    const double c = math::dot(offset, offset) - radius_sum * radius_sum;
                    const double approach = math::dot(offset, delta);
                    if (c < -1e-5 && approach < 0) {
                        fraction = 0;
                        continue;
                    }
                    const double disc = approach * approach - travel * c;
                    if (approach >= 0 || disc < 0)
                        continue;
                    const double contact = (-approach - std::sqrt(disc)) / travel;
                    if (contact >= -1e-6 && contact <= fraction)
                        fraction = std::max(0.0, contact - 1e-5 / std::sqrt(travel));
                }
            }
            p = original + delta * fraction;
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
