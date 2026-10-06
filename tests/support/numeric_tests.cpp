#include "numeric.hpp"
#include <limits>

TEST_CASE("numeric tolerance handles finite boundaries") {
    using opensim::test::within_tolerance;
    CHECK(within_tolerance(0.125, 0, 0.125, 0));
    CHECK_FALSE(within_tolerance(0.25, 0, 0.125, 0));
    CHECK(within_tolerance(8, 7, 0, 0.125));
    CHECK_FALSE(within_tolerance(8, 6, 0, 0.125));
    CHECK_FALSE(within_tolerance(1, 1, -1, 0));
    CHECK_FALSE(within_tolerance(1, 1, 0, 2));
    CHECK_FALSE(within_tolerance(std::numeric_limits<double>::infinity(), 1, 0, 0));
    CHECK_FALSE(within_tolerance(1, std::numeric_limits<double>::quiet_NaN(), 0, 0));
    const auto largest = std::numeric_limits<double>::max();
    CHECK_FALSE(within_tolerance(largest, -largest, 0, 1));
    OPENSIM_CHECK_NEAR(1.0, 1.0, 0, 0);
}

TEST_CASE("numeric diagnostic probe", "[.diagnostic-probe]") {
    OPENSIM_CHECK_NEAR(2.0, 1.0, 0.125, 0.0625);
}
