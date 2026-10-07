#pragma once
#include <cstdint>
#include <opensim/core/values.hpp>
#include <opensim/scene/scene.hpp>

namespace opensim::physics {
class World {
    scene::Document starting_document_;
    scene::Snapshot snapshot_;
    double fixed_dt_;
    std::uint64_t step_index_;
    World(scene::Document document, scene::Snapshot snapshot, double dt, std::uint64_t index);

  public:
    [[nodiscard]] static core::Result<World> create(const scene::Document &, double fixed_dt);
    [[nodiscard]] core::Result<World> step() const;
    [[nodiscard]] core::Result<World> reset() const;
    [[nodiscard]] scene::Snapshot snapshot() const { return snapshot_; }
    [[nodiscard]] double fixed_dt() const { return fixed_dt_; }
    [[nodiscard]] std::uint64_t step_index() const { return step_index_; }
};
} // namespace opensim::physics
