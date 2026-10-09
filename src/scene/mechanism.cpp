#include <algorithm>
#include <cmath>
#include <opensim/scene/mechanism.hpp>
#include <set>
namespace opensim::scene {
namespace {
bool point(math::Vec2 v) {
    return math::finite(v) && std::abs(v.x) <= 10000 && std::abs(v.y) <= 10000;
}
bool range(double v, double low, double high) { return std::isfinite(v) && v >= low && v <= high; }
core::Result<void> bad(const char *message, core::EntityId id = {}) {
    return core::Result<void>::failure({core::Code::invalid_argument, core::Severity::error,
                                        message, id.valid() ? std::optional{id} : std::nullopt,
                                        "mechanism"});
}
} // namespace
core::Result<void> validate(const Mechanism &m) {
    if (m.bodies.size() > 256 ||
        m.links.size() + m.hinges.size() + m.windings.size() + m.welds.size() + m.slides.size() >
            1024 ||
        !point(m.gravity) || !range(m.fixed_dt, .001, 1.0 / 30))
        return bad("Mechanism bounds or timestep invalid");
    const auto valid_mixer = [](MaterialMixer v) {
        return v == MaterialMixer::geometric_mean || v == MaterialMixer::minimum ||
               v == MaterialMixer::maximum;
    };
    if (!valid_mixer(m.friction_mixer) || !valid_mixer(m.restitution_mixer))
        return bad("Invalid material mixer");
    std::set<core::EntityId> ids, joints;
    std::size_t fixture_count = 0;
    for (const auto &b : m.bodies) {
        if (!b.id.valid() || !ids.insert(b.id).second)
            return bad("Duplicate or zero body ID", b.id);
        if (!point(b.center) || !point(b.velocity) || !range(b.angle, -1e6, 1e6) ||
            !range(b.angular_velocity, -1000, 1000) || !range(b.radius, 1e-4, 1000) ||
            b.radius == 0 || !range(b.mass, 0, 1e12) || !range(b.inertia, 0, 1e12) ||
            (!b.static_body && (b.mass < 1e-6 || b.inertia < 1e-9)) || !range(b.friction, 0, 1e6) ||
            !range(b.restitution, 0, 1) || !range(b.damping, 0, 1e6) ||
            !range(b.angular_damping, 0, 1e6) || !range(b.gravity_scale, -100, 100) ||
            (b.static_body && (b.velocity != math::Vec2{} || b.angular_velocity != 0)))
            return bad("Invalid circular body", b.id);
        fixture_count += b.fixtures.size();
        if (b.fixtures.size() > 256 || fixture_count > 4096)
            return bad("Fixture budget", b.id);
        for (const auto &f : b.fixtures) {
            if (!point(f.center) || !range(f.radius, 1e-4, 1000) || !range(f.friction, 0, 1e6) ||
                !range(f.restitution, 0, 1))
                return bad("Invalid fixture material or circle", b.id);
            if (f.vertices.empty())
                continue;
            if (f.vertices.size() < 3 || f.vertices.size() > 8)
                return bad("Convex fixture requires 3..8 vertices", b.id);
            for (std::size_t i = 0; i < f.vertices.size(); ++i) {
                auto a = f.vertices[i], z = f.vertices[(i + 1) % f.vertices.size()];
                if (!point(a) || std::abs(a.x) > 1000 || std::abs(a.y) > 1000)
                    return bad("Fixture outside local envelope", b.id);
                for (std::size_t k = 0; k < f.vertices.size(); ++k) {
                    if (k == i || k == (i + 1) % f.vertices.size())
                        continue;
                    auto q = f.vertices[k];
                    if ((z.x - a.x) * (q.y - a.y) - (z.y - a.y) * (q.x - a.x) <= 1e-10)
                        return bad("Fixture must be strictly convex CCW", b.id);
                }
            }
        }
    }
    for (const auto &j : m.links) {
        const auto dynamic = [&](core::EntityId id) {
            const auto found = std::find_if(m.bodies.begin(), m.bodies.end(),
                                            [&](const auto &b) { return b.id == id; });
            return found != m.bodies.end() && !found->static_body;
        };
        if (!dynamic(j.body_a) && !dynamic(j.body_b))
            return bad("Distance joint needs a dynamic endpoint", j.id);
        if (!j.id.valid() || !joints.insert(j.id).second || !ids.contains(j.body_a) ||
            (j.body_b.valid() && !ids.contains(j.body_b)) || j.body_a == j.body_b ||
            !point(j.local_a) || !point(j.local_b) || !range(j.length, .01, 20000) ||
            j.length == 0 || !range(j.stiffness, 0, 1e9) || !range(j.damping_ratio, 0, 10) ||
            !(j.damping_coefficient == -1 || range(j.damping_coefficient, 0, 1e9)) ||
            !range(j.minimum, 0, 20000) || !range(j.maximum, j.minimum, 20000))
            return bad("Invalid distance joint", j.id);
    }
    const auto endpoints = [&](core::EntityId a, core::EntityId b, core::EntityId id) {
        const auto dyn = [&](core::EntityId x) {
            for (const auto &body : m.bodies)
                if (body.id == x)
                    return !body.static_body;
            return false;
        };
        return id.valid() && joints.insert(id).second && ids.contains(a) &&
               (!b.valid() || ids.contains(b)) && a != b && (dyn(a) || dyn(b));
    };
    for (const auto &j : m.slides) {
        // The axis is owned by A; ground is therefore permitted at A, unlike
        // distance/hinge ports which conventionally place ground at B.
        const auto dynamic = [&](core::EntityId id) {
            for (const auto &b : m.bodies)
                if (b.id == id)
                    return !b.static_body;
            return false;
        };
        if (!j.id.valid() || !joints.insert(j.id).second ||
            (j.body_a.valid() && !ids.contains(j.body_a)) || !ids.contains(j.body_b) ||
            j.body_a == j.body_b || (!dynamic(j.body_a) && !dynamic(j.body_b)) ||
            !point(j.local_a) || !point(j.local_b) || !math::finite(j.axis) ||
            std::abs(std::hypot(j.axis.x, j.axis.y) - 1) > 1e-9 || !range(j.reference, -1e6, 1e6) ||
            !range(j.lower, -20000, 20000) || !range(j.upper, j.lower, 20000) ||
            !range(j.speed, -10000, 10000) || !range(j.max_force, 0, 1e9) ||
            (j.motor && !j.lock_rotation))
            return bad("Invalid slide joint", j.id);
    }
    for (const auto &j : m.welds)
        if (!endpoints(j.body_a, j.body_b, j.id) || !point(j.local_a) || !point(j.local_b) ||
            !range(j.reference, -1e6, 1e6) || !range(j.frequency, 0, 1000) ||
            !range(j.damping_ratio, 0, 10))
            return bad("Invalid weld", j.id);
    for (const auto &j : m.hinges)
        if (!endpoints(j.body_a, j.body_b, j.id) || !point(j.local_a) || !point(j.local_b) ||
            !range(j.reference, -1e6, 1e6) || !range(j.lower, -1e6, 1e6) ||
            !range(j.upper, j.lower, 1e6) || !range(j.speed, -1000, 1000) ||
            !range(j.max_torque, 0, 1e12))
            return bad("Invalid hinge", j.id);
    for (const auto &j : m.windings)
        if (!endpoints(j.body_a, j.body_b, j.id) || !point(j.local_a) || !point(j.local_b) ||
            !range(j.radius_a, -1000, 1000) || !range(j.radius_b, -1000, 1000))
            return bad("Invalid winding", j.id);
    for (const auto &j : m.windings) {
        const CircleBody *a = nullptr, *b = nullptr;
        for (const auto &body : m.bodies) {
            if (body.id == j.body_a)
                a = &body;
            if (body.id == j.body_b)
                b = &body;
        }
        if (!a || !b ||
            std::hypot(b->center.x - a->center.x, b->center.y - a->center.y) <=
                std::abs(j.radius_a - j.radius_b) + 1e-5)
            return bad("Degenerate winding geometry", j.id);
    }
    return core::Result<void>::success();
}
} // namespace opensim::scene
