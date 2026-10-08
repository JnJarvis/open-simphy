#pragma once
#include <opensim/core/values.hpp>
#include <opensim/math/math.hpp>
#include <string>
#include <vector>
namespace opensim::scene {
struct RigidFixture {
    math::Vec2 center{};
    double radius = .5;
    std::vector<math::Vec2> vertices;
    double friction = .3, restitution = 0;
    bool sensor = false;
    std::uint64_t category = 1, mask = ~std::uint64_t{0};
};
struct CircleBody {
    core::EntityId id;
    std::string name;
    math::Vec2 center{}, velocity{};
    double angle = 0, angular_velocity = 0, radius = .5, mass = 1, inertia = .125;
    double friction = .3, restitution = 1, damping = 0, angular_damping = 0, gravity_scale = 1;
    bool fixed_rotation = false, static_body = false, sensor = false;
    std::uint64_t category = 1, mask = ~std::uint64_t{0};
    std::vector<RigidFixture> fixtures;
};
struct DistanceLink {
    core::EntityId id, body_a, body_b;
    math::Vec2 local_a{}, local_b{};
    double length = 1;
    bool collide_connected = false;
};
struct HingeLink {
    core::EntityId id, body_a, body_b;
    math::Vec2 local_a{}, local_b{};
    double reference = 0, lower = 0, upper = 0, speed = 0, max_torque = 0;
    bool limit = false, motor = false, collide_connected = false;
};
struct WindingLink {
    core::EntityId id, body_a, body_b;
    math::Vec2 local_a{}, local_b{};
    double radius_a = 0, radius_b = 0;
    bool collide_connected = false;
};
struct AppliedForce {
    core::EntityId body;
    math::Vec2 force{}, local_point{};
    bool wrapped = false;
    double torque_arm = 0;
};
enum class MaterialMixer { geometric_mean, minimum, maximum };
struct Mechanism {
    math::Vec2 gravity{0, -9.8};
    double fixed_dt = 1.0 / 60;
    std::vector<CircleBody> bodies;
    std::vector<DistanceLink> links;
    std::vector<HingeLink> hinges;
    std::vector<WindingLink> windings;
    MaterialMixer friction_mixer = MaterialMixer::geometric_mean;
    MaterialMixer restitution_mixer = MaterialMixer::maximum;
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
