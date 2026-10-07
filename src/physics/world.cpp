#include "step_time.hpp"
#include <cmath>
#include <opensim/math/math.hpp>
#include <opensim/physics/world.hpp>
#include <utility>
#include <vector>

namespace opensim::physics {
namespace {
core::Diagnostic overflow(core::EntityId id, const char *path) {
    return {core::Code::invalid_data, core::Severity::error, "Nonfinite particle step result", id,
            path};
}
} // namespace
World::World(scene::Document document, scene::Snapshot snapshot, double dt, std::uint64_t index)
    : starting_document_(std::move(document)), snapshot_(std::move(snapshot)), fixed_dt_(dt),
      step_index_(index) {}
core::Result<World> World::create(const scene::Document &document, double dt) {
    if (!std::isfinite(dt) || dt <= 0 || dt > 1)
        return core::Result<World>::failure({core::Code::invalid_argument,
                                             core::Severity::error,
                                             "Fixed timestep must be finite and in (0,1]",
                                             {},
                                             "fixed_dt"});
    const auto initial = scene::Snapshot::initial(document);
    if (initial.error())
        return core::Result<World>::failure(*initial.error());
    return core::Result<World>::success(World(document, *initial.value(), dt, 0));
}
core::Result<World> World::reset() const { return create(starting_document_, fixed_dt_); }
core::Result<World> World::step() const {
    const auto clock = detail::next_time(step_index_, snapshot_.time(), fixed_dt_);
    if (clock.error())
        return core::Result<World>::failure(*clock.error());
    std::vector<scene::ParticleSample> samples;
    samples.reserve(snapshot_.particles().size());
    for (const auto &old : snapshot_.particles()) {
        const auto delta = snapshot_.gravity() * fixed_dt_;
        if (!math::finite(delta))
            return core::Result<World>::failure(overflow(old.id, "particles.velocity_delta"));
        const auto drift = old.velocity * fixed_dt_;
        if (!math::finite(drift))
            return core::Result<World>::failure(overflow(old.id, "particles.drift"));
        const auto acceleration = delta * (0.5 * fixed_dt_);
        if (!math::finite(acceleration))
            return core::Result<World>::failure(overflow(old.id, "particles.acceleration"));
        const auto drifted = old.position + drift;
        if (!math::finite(drifted))
            return core::Result<World>::failure(overflow(old.id, "particles.position"));
        const auto position = drifted + acceleration;
        if (!math::finite(position))
            return core::Result<World>::failure(overflow(old.id, "particles.position"));
        const auto velocity = old.velocity + delta;
        if (!math::finite(velocity))
            return core::Result<World>::failure(overflow(old.id, "particles.velocity"));
        samples.push_back({old.id, old.mass, position, velocity, old.display_radius});
    }
    const auto candidate =
        scene::Snapshot::create(starting_document_, clock.value()->time, std::move(samples));
    if (candidate.error())
        return core::Result<World>::failure(*candidate.error());
    return core::Result<World>::success(
        World(starting_document_, *candidate.value(), fixed_dt_, clock.value()->index));
}
} // namespace opensim::physics
