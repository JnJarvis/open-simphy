#include "session.hpp"
#include <algorithm>
#include <cmath>
namespace opensim::app {
core::Result<Session> Session::create() {
    const auto document = scene::Document::create(1, {0, -9.8125},
                                                  {{{1}, {1, {-3, 1}, {2, 5}, .18}},
                                                   {{2}, {2, {0, 2}, {.5, 2}, .25}},
                                                   {{3}, {.5, {3, 1}, {-1, 4}, .14}}},
                                                  {3});
    if (document.error())
        return core::Result<Session>::failure(*document.error());
    const auto world = physics::World::create(*document.value(), 1.0 / 128);
    if (world.error())
        return core::Result<Session>::failure(*world.error());
    const auto overlays =
        scene::DrawPacket::create({scene::Line{{-5, 0}, {5, 0}, .0125, {.25, .35, .45, 1}},
                                   scene::Line{{0, -3}, {0, 3}, .0125, {.25, .35, .45, 1}}});
    if (overlays.error())
        return core::Result<Session>::failure(*overlays.error());
    return core::Result<Session>::success(Session(*world.value(), *overlays.value()));
}
core::Result<void> Session::advance() {
    const auto next = world_.step();
    if (next.error()) {
        set_running(false);
        return core::Result<void>::failure(*next.error());
    }
    world_ = *next.value();
    return core::Result<void>::success();
}
core::Result<void> Session::reset() {
    const auto next = world_.reset();
    if (next.error())
        return core::Result<void>::failure(*next.error());
    world_ = *next.value();
    accumulated_ = 0;
    return core::Result<void>::success();
}
core::Result<void> Session::tick(double elapsed) {
    if (!std::isfinite(elapsed) || elapsed < 0)
        return core::Result<void>::failure({core::Code::invalid_argument,
                                            core::Severity::error,
                                            "Elapsed time must be finite and nonnegative",
                                            {},
                                            "elapsed"});
    if (!running_)
        return core::Result<void>::success();
    // Bound catch-up after stalls. Excess wall time is discarded, never a larger physics dt.
    accumulated_ += std::min(elapsed, .25);
    unsigned count = 0;
    while (accumulated_ >= world_.fixed_dt() && count < 32) {
        const auto result = advance();
        if (result.error())
            return result;
        accumulated_ -= world_.fixed_dt();
        ++count;
    }
    return core::Result<void>::success();
}
} // namespace opensim::app
