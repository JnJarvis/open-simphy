#include <catch2/catch_test_macros.hpp>
#include <opensim/math/math.hpp>
using namespace opensim::math;
TEST_CASE("nonfinite and overflow never become checked success") {
    const auto max = std::numeric_limits<double>::max(),
               nan = std::numeric_limits<double>::quiet_NaN();
    CHECK_FALSE(normalized({max, max}));
    CHECK_FALSE(near(max, -max, 0, 1));
    CHECK_FALSE(near(1, 1, nan, 0));
    CHECK_FALSE(near(1, 1, 0, -1));
    CHECK_FALSE(near(1, 1, 0, nan));
    CHECK_FALSE(near(Vec2{1, 2}, Vec2{1, 3}, 0, 0));
    CHECK_FALSE(Transform2::create({nan, 0}, 0));
    CHECK_FALSE(Transform2::create({0, 0}, nan));
    const auto angle = Transform2::create({0, 0}, max);
    REQUIRE(angle);
    CHECK_FALSE(compose(*angle, *angle));
    const auto translated = Transform2::create({max, 0}, 0);
    REQUIRE(translated);
    CHECK_FALSE(translated->point({max, 0}));
    const auto unwrapped = Transform2::create({0, 0}, 100);
    REQUIRE(unwrapped);
    CHECK(unwrapped->angle() == 100);
}
