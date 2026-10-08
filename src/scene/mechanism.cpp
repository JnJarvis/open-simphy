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
    if (m.bodies.size() > 256 || m.links.size() > 1024 || !point(m.gravity) ||
        !range(m.fixed_dt, .001, 1.0 / 30))
        return bad("Mechanism bounds or timestep invalid");
    std::set<core::EntityId> ids, joints;
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
            !point(j.local_a) || !point(j.local_b) || !range(j.length, .01, 20000) || j.length == 0)
            return bad("Invalid distance joint", j.id);
    }
    return core::Result<void>::success();
}
} // namespace opensim::scene
