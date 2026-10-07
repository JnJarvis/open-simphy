#pragma once
#include <cmath>
#include <cstdint>
#include <opensim/core/values.hpp>

namespace opensim::physics::detail {
inline constexpr std::uint64_t max_step_count = 9007199254740991ULL;
struct StepTime {
    std::uint64_t index;
    double time;
};
// Private pure seam: tests exercise the counter limit without public state injection.
[[nodiscard]] inline core::Result<StepTime> next_time(std::uint64_t index, double old_time,
                                                      double dt) {
    if (index >= max_step_count)
        return core::Result<StepTime>::failure({core::Code::invalid_data,
                                                core::Severity::error,
                                                "Step count exhausted",
                                                {},
                                                "step_index"});
    if (!std::isfinite(dt) || dt <= 0 || dt > 1)
        return core::Result<StepTime>::failure({core::Code::invalid_argument,
                                                core::Severity::error,
                                                "Fixed timestep must be finite and in (0,1]",
                                                {},
                                                "fixed_dt"});
    const auto next = index + 1;
    const double time = static_cast<double>(next) * dt;
    if (!std::isfinite(old_time) || old_time < 0 || !std::isfinite(time) || time <= old_time)
        return core::Result<StepTime>::failure({core::Code::invalid_data,
                                                core::Severity::error,
                                                "Simulation time must advance finitely",
                                                {},
                                                "time"});
    return core::Result<StepTime>::success({next, time});
}
} // namespace opensim::physics::detail
