#pragma once
#include <opensim/core/values.hpp>
#include <opensim/math/math.hpp>
#include <string>
#include <vector>
namespace opensim::scene {
struct CircleBody {
    core::EntityId id;
    std::string name;
    math::Vec2 center{}, velocity{};
    double angle = 0, angular_velocity = 0, radius = .5, mass = 1, inertia = .125;
    double friction = .3, restitution = 1, damping = 0, angular_damping = 0, gravity_scale = 1;
    bool fixed_rotation = false, static_body = false, sensor = false;
    std::uint64_t category = 1, mask = ~std::uint64_t{0};
};
struct DistanceLink {
    core::EntityId id, body_a, body_b;
    math::Vec2 local_a{}, local_b{};
    double length = 1;
    bool collide_connected = false;
};
struct Mechanism {
    math::Vec2 gravity{0, -9.8};
    double fixed_dt = 1.0 / 60;
    std::vector<CircleBody> bodies;
    std::vector<DistanceLink> links;
};
core::Result<void> validate(const Mechanism &);
struct CircleSample {
    core::EntityId id;
    math::Vec2 center, velocity;
    double angle, angular_velocity;
};
struct MechanismSnapshot {
    double time = 0;
    std::vector<CircleSample> bodies;
};
} // namespace opensim::scene
