#pragma once
#include <memory>
#include <opensim/scene/mechanism.hpp>
namespace opensim::physics {
class Mechanism {
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit Mechanism(const scene::Mechanism &);

  public:
    ~Mechanism();
    static core::Result<std::shared_ptr<Mechanism>> create(const scene::Mechanism &);
    scene::MechanismSnapshot snapshot() const;
    core::Result<void> step();
    core::Result<void> reset();
    core::Result<void> forces(const std::vector<scene::AppliedForce> &);
    core::Result<void> friction(core::EntityId, double);
    core::Result<void> relocate(core::EntityId, math::Vec2);
};
} // namespace opensim::physics
