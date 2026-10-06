#pragma once

#include <catch2/catch_test_macros.hpp>
#include <cmath>

namespace opensim::test {
// Test-only comparison; tolerances are supplied by the owning specification.
inline bool within_tolerance(double actual, double expected, double absolute, double relative) {
    if (!std::isfinite(actual) || !std::isfinite(expected) || !std::isfinite(absolute) ||
        !std::isfinite(relative) || absolute < 0 || relative < 0 || relative > 1) {
        return false;
    }
    const double scale = std::fmax(std::abs(actual), std::abs(expected));
    const double difference = std::abs(actual - expected);
    return difference <= absolute ||
           (scale > 0 && std::abs(actual / scale - expected / scale) <= relative);
}
} // namespace opensim::test

#define OPENSIM_CHECK_NEAR(actual, expected, absolute, relative)                                   \
    do {                                                                                           \
        const double actual_value = (actual);                                                      \
        const double expected_value = (expected);                                                  \
        const double absolute_tolerance = (absolute);                                              \
        const double relative_tolerance = (relative);                                              \
        CAPTURE(actual_value, expected_value, absolute_tolerance, relative_tolerance);             \
        CHECK(opensim::test::within_tolerance(actual_value, expected_value, absolute_tolerance,    \
                                              relative_tolerance));                                \
    } while (false)
