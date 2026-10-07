#include "step_time.hpp"
#include <catch2/catch_test_macros.hpp>
#include <limits>
using namespace opensim::physics::detail;
TEST_CASE("bounded tick time never wraps or stagnates") {
    const auto end = next_time(max_step_count, 0, 1);
    REQUIRE(end.error());
    CHECK(end.error()->path == "step_index");
    CHECK(next_time(std::numeric_limits<std::uint64_t>::max(), 0, 1).error()->path == "step_index");
    const auto last = next_time(max_step_count - 1, static_cast<double>(max_step_count - 1), 1);
    REQUIRE(last.value());
    CHECK(last.value()->index == max_step_count);
    CHECK(last.value()->time == static_cast<double>(max_step_count));
    CHECK(next_time(3, 1, 0.25).error()->path == "time");
    CHECK(next_time(0, std::numeric_limits<double>::infinity(), 0.25).error()->path == "time");
    CHECK(next_time(0, 0, 0).error()->path == "fixed_dt");
    const auto tiny = next_time(0, 0, std::numeric_limits<double>::denorm_min());
    REQUIRE(tiny.value());
    CHECK(tiny.value()->time > 0);
    const auto hundred = next_time(99, 0.99, 0.01);
    REQUIRE(hundred.value());
    CHECK(hundred.value()->time == 1);
}
