#include <algorithm>
#include <box2d/box2d.h>
#include <cmath>
#include <map>
#include <numbers>
#include <opensim/physics/mechanism.hpp>
#include <set>
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
    double time_origin = 0;
    bool poisoned = false;
    scene::MechanismSnapshot published;
    std::vector<scene::AppliedForce> applied;
    struct WindingState {
        scene::WindingLink link;
        double constant, a, b, previous_a, previous_b, phi;
    };
    std::vector<WindingState> windings;
    struct ElasticState {
        scene::DistanceLink link;
        b2JointId joint;
    };
    std::vector<ElasticState> elastic;
    void spring_parameters(double dt) {
        for (const auto &spring : elastic) {
            auto a = bodies.at(spring.link.body_a),
                 b = spring.link.body_b.valid() ? bodies.at(spring.link.body_b) : ground;
            auto pa = b2Body_GetWorldPoint(a, native(spring.link.local_a)),
                 pb = b2Body_GetWorldPoint(b, native(spring.link.local_b));
            const double dx = pb.x - pa.x, dy = pb.y - pa.y, length = std::hypot(dx, dy);
            if (length < 1e-6)
                continue;
            const auto inverse = [&](b2BodyId id, b2Vec2 p) {
                if (b2Body_GetType(id) != b2_dynamicBody)
                    return 0.0;
                auto c = b2Body_GetWorldCenterOfMass(id);
                const double lever = ((p.x - c.x) * dy - (p.y - c.y) * dx) / length;
                const double inertia = b2Body_GetRotationalInertia(id), mass = b2Body_GetMass(id);
                return (mass > 0 ? 1.0 / mass : 0) + (inertia > 0 ? lever * lever / inertia : 0);
            };
            const double inv = inverse(a, pa) + inverse(b, pb);
            if (inv <= 0)
                continue;
            if (spring.link.stiffness == 0) {
                if (spring.link.damping_coefficient > 0) {
                    auto va = b2Body_GetWorldPointVelocity(a, pa),
                         vb = b2Body_GetWorldPointVelocity(b, pb);
                    const double speed = ((vb.x - va.x) * dx + (vb.y - va.y) * dy) / length;
                    const double coefficient = spring.link.damping_coefficient;
                    const double impulse = -coefficient * speed * dt / (1 + coefficient * inv * dt);
                    b2Vec2 vector{float(impulse * dx / length), float(impulse * dy / length)};
                    b2Body_ApplyLinearImpulse(b, vector, pb, true);
                    b2Body_ApplyLinearImpulse(a, {-vector.x, -vector.y}, pa, true);
                }
                continue;
            }
            b2DistanceJoint_SetSpringHertz(
                spring.joint,
                float(std::sqrt(spring.link.stiffness * inv) / (2 * std::numbers::pi)));
            const double ratio =
                spring.link.damping_coefficient >= 0
                    ? spring.link.damping_coefficient / (2 * std::sqrt(spring.link.stiffness / inv))
                    : spring.link.damping_ratio;
            b2DistanceJoint_SetSpringDampingRatio(spring.joint, float(ratio));
        }
    }
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
            d.enableSpring = j.spring || j.stiffness > 0 || j.damping_coefficient >= 0;
            if (j.stiffness > 0) {
                const auto inverse = [](b2BodyId id) {
                    const auto mass = b2Body_GetMass(id);
                    return b2Body_GetType(id) == b2_dynamicBody && mass > 0 ? 1.0 / mass : 0;
                };
                const double inverse_mass = inverse(d.bodyIdA) + inverse(d.bodyIdB);
                d.hertz = float(std::sqrt(j.stiffness * inverse_mass) / (2 * std::numbers::pi));
                d.dampingRatio = float(j.damping_ratio);
            }
            // Box2D disables limits when its spring flag is false. A zero-Hz spring
            // supplies no bilateral force but allows the unilateral rope bounds.
            if (j.limit) {
                d.enableSpring = true;
                d.enableLimit = true;
                d.minLength = float(j.minimum);
                d.maxLength = float(j.maximum);
            }
            d.collideConnected = j.collide_connected;
            auto joint = b2CreateDistanceJoint(world, &d);
            if (j.spring || j.stiffness > 0 || j.damping_coefficient >= 0)
                elastic.push_back({j, joint});
        }
        for (const auto &j : m.slides) {
            const auto a = j.body_a.valid() ? bodies.at(j.body_a) : ground;
            const auto b = bodies.at(j.body_b);
            if (j.lock_rotation) {
                auto d = b2DefaultPrismaticJointDef();
                d.bodyIdA = a;
                d.bodyIdB = b;
                d.localAnchorA = native(j.local_a);
                d.localAnchorB = native(j.local_b);
                d.localAxisA = native(j.axis);
                d.referenceAngle = float(j.reference);
                d.enableLimit = j.limit;
                d.lowerTranslation = float(j.lower);
                d.upperTranslation = float(j.upper);
                d.enableMotor = j.motor;
                d.motorSpeed = float(j.speed);
                d.maxMotorForce = float(j.max_force);
                d.collideConnected = j.collide_connected;
                b2CreatePrismaticJoint(world, &d);
            } else {
                auto d = b2DefaultWheelJointDef();
                d.bodyIdA = a;
                d.bodyIdB = b;
                d.localAnchorA = native(j.local_a);
                d.localAnchorB = native(j.local_b);
                d.localAxisA = native(j.axis);
                d.enableSpring = false;
                d.enableMotor = false;
                d.enableLimit = j.limit;
                d.lowerTranslation = float(j.lower);
                d.upperTranslation = float(j.upper);
                d.collideConnected = j.collide_connected;
                b2CreateWheelJoint(world, &d);
            }
        }
        for (const auto &j : m.welds) {
            auto d = b2DefaultWeldJointDef();
            d.bodyIdA = bodies.at(j.body_a);
            d.bodyIdB = j.body_b.valid() ? bodies.at(j.body_b) : ground;
            d.localAnchorA = native(j.local_a);
            d.localAnchorB = native(j.local_b);
            d.referenceAngle = float(j.reference);
            d.angularHertz = float(j.frequency);
            d.angularDampingRatio = float(j.damping_ratio);
            d.collideConnected = j.collide_connected;
            b2CreateWeldJoint(world, &d);
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
        impl_->spring_parameters(impl_->initial.fixed_dt / 8);
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
    next.time = impl_->time_origin + double(impl_->steps) * impl_->initial.fixed_dt;
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
core::Result<void> Mechanism::set_time(double time) {
    if (impl_->poisoned || !std::isfinite(time) || time < 0 || time > 1e9)
        return bad("Invalid simulation time");
    impl_->steps = 0;
    impl_->time_origin = time;
    impl_->published.time = time;
    return core::Result<void>::success();
}
core::Result<void> Mechanism::update(const std::vector<scene::BodyUpdate> &updates) {
    if (impl_->poisoned || updates.size() > 256)
        return bad("Invalid body update budget/runtime");
    if (updates.empty())
        return core::Result<void>::success();
    // Validate a complete prospective definition before any backend operation.
    auto candidate = impl_->initial;
    std::set<core::EntityId> seen;
    for (const auto &u : updates) {
        auto found = std::find_if(candidate.bodies.begin(), candidate.bodies.end(),
                                  [&](const auto &b) { return b.id == u.body; });
        if (found == candidate.bodies.end() || !seen.insert(u.body).second)
            return bad("Missing or duplicate updated body");
        auto &b = *found;
        if (u.center)
            b.center = *u.center;
        if (u.velocity)
            b.velocity = *u.velocity;
        if (u.angle)
            b.angle = *u.angle;
        if (u.angular_velocity)
            b.angular_velocity = *u.angular_velocity;
        if (u.mass)
            b.mass = *u.mass;
        if (u.inertia)
            b.inertia = *u.inertia;
        if (u.gravity_scale)
            b.gravity_scale = *u.gravity_scale;
        if (u.damping)
            b.damping = *u.damping;
        if (u.angular_damping)
            b.angular_damping = *u.angular_damping;
        if (u.friction)
            b.friction = *u.friction;
        if (u.restitution)
            b.restitution = *u.restitution;
        if (b.fixed_rotation && u.angular_velocity && *u.angular_velocity != 0)
            return bad("Angular velocity conflicts with fixed rotation");
        for (auto &f : b.fixtures) {
            if (u.friction)
                f.friction = *u.friction;
            if (u.restitution)
                f.restitution = *u.restitution;
        }
    }
    auto valid = scene::validate(candidate);
    if (valid.error())
        return valid;
    for (const auto &u : updates) {
        const auto id = impl_->bodies.at(u.body);
        if (u.center || u.angle)
            b2Body_SetTransform(id, u.center ? native(*u.center) : b2Body_GetPosition(id),
                                u.angle ? b2MakeRot(float(*u.angle)) : b2Body_GetRotation(id));
        if (u.velocity)
            b2Body_SetLinearVelocity(id, native(*u.velocity));
        if (u.angular_velocity)
            b2Body_SetAngularVelocity(id, float(*u.angular_velocity));
        if (u.mass || u.inertia) {
            auto data = b2Body_GetMassData(id);
            if (u.mass)
                data.mass = float(*u.mass);
            if (u.inertia && !b2Body_IsFixedRotation(id))
                data.rotationalInertia = float(*u.inertia);
            b2Body_SetMassData(id, data);
        }
        if (u.gravity_scale)
            b2Body_SetGravityScale(id, float(*u.gravity_scale));
        if (u.damping)
            b2Body_SetLinearDamping(id, float(*u.damping));
        if (u.angular_damping)
            b2Body_SetAngularDamping(id, float(*u.angular_damping));
        if (u.friction || u.restitution) {
            const int count = b2Body_GetShapeCount(id);
            std::vector<b2ShapeId> shapes(static_cast<std::size_t>(count));
            b2Body_GetShapes(id, shapes.data(), count);
            for (auto shape : shapes) {
                if (u.friction)
                    b2Shape_SetFriction(shape, float(*u.friction));
                if (u.restitution)
                    b2Shape_SetRestitution(shape, float(*u.restitution));
            }
        }
        for (auto &sample : impl_->published.bodies)
            if (sample.id == u.body) {
                sample.center = value(b2Body_GetPosition(id));
                sample.velocity = value(b2Body_GetLinearVelocity(id));
                sample.angle = b2Rot_GetAngle(b2Body_GetRotation(id));
                sample.angular_velocity = b2Body_GetAngularVelocity(id);
            }
    }
    return core::Result<void>::success();
}
} // namespace opensim::physics
