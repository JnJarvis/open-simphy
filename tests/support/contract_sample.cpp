#include "numeric.hpp"
#include <array>

TEST_CASE("synthetic contract vectors run without production modules") {
    // Demonstrates shared vectors, not a simulator contract or implementation.
    const auto identity_double = [](double value) { return value; };
    for (const double value : std::array{0.0, -2.0, 4.0}) {
        OPENSIM_CHECK_NEAR(identity_double(value), value, 0, 0);
    }
}
