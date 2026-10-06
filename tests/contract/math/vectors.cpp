#include "numeric.hpp"
#include <numbers>
#include <opensim/math/math.hpp>
using namespace opensim::math;
namespace {
void check_vector(Vec2 actual, Vec2 expected) {
    // Accepted MATH-001 tolerances for small analytic examples, in caller units.
    OPENSIM_CHECK_NEAR(actual.x, expected.x, 1e-12, 1e-12);
    OPENSIM_CHECK_NEAR(actual.y, expected.y, 1e-12, 1e-12);
}
} // namespace
TEST_CASE("M01-M09 vector numeric contract") {
    CHECK((Vec2{1, 2} + Vec2{3, -4} == Vec2{4, -2}));
    CHECK((Vec2{1, 2} - Vec2{3, -4} == Vec2{-2, 6}));
    CHECK((-Vec2{1, 2} == Vec2{-1, -2}));
    CHECK((2 * Vec2{1, -3} == Vec2{2, -6}));
    CHECK(dot({1, 2}, {3, 4}) == 11);
    CHECK(cross({1, 0}, {0, 1}) == 1);
    OPENSIM_CHECK_NEAR(magnitude({3, 4}), 5, 1e-12, 1e-12);
    REQUIRE(normalized({3, 4}));
    check_vector(*normalized({3, 4}), {0.6, 0.8});
    const auto nan = std::numeric_limits<double>::quiet_NaN(),
               inf = std::numeric_limits<double>::infinity();
    CHECK_FALSE(normalized({0, 0}));
    CHECK_FALSE(normalized({nan, 1}));
    CHECK_FALSE(normalized({inf, 0}));
    CHECK(magnitude({1e308, 0}) == 1e308);
    REQUIRE(normalized({1e-300, 0}));
    check_vector(*normalized({1e-300, 0}), {1, 0});
    CHECK(finite({1, 2}));
    CHECK_FALSE(finite({nan, 2}));
    CHECK_FALSE(finite({1, inf}));
    CHECK(near(0, 5e-13, 1e-12, 0));
    CHECK_FALSE(near(0, 2e-12, 1e-12, 0));
    CHECK(near(1e6, 1e6 + 0.5, 0, 1e-6));
    CHECK_FALSE(near(inf, inf, 1, 0));
    CHECK_FALSE(near(1, 1, -1, 0));
    CHECK_FALSE(near(1, 1, 0, 2));
}
TEST_CASE("M10-M18 transform and equality contract") {
    const auto identity = Transform2::identity();
    REQUIRE(identity.point({3, 4}));
    check_vector(*identity.point({3, 4}), {3, 4});
    check_vector(*identity.direction({3, 4}), {3, 4});
    const auto t = Transform2::create({2, 3}, std::numbers::pi / 2);
    REQUIRE(t);
    check_vector(*t->point({1, 0}), {2, 4});
    check_vector(*t->direction({1, 0}), {0, 1});
    const auto a = Transform2::create({2, 0}, std::numbers::pi / 2),
               b = Transform2::create({1, 0}, 0);
    const auto combined = compose(*a, *b);
    REQUIRE(combined);
    check_vector(*combined->point({1, 0}), {2, 2});
    check_vector(*compose(*b, *a)->point({1, 0}), {3, 1});
    const auto inv = inverse(*t);
    REQUIRE(inv);
    check_vector(*inv->point(*t->point({3, -4})), {3, -4});
    const auto inf = std::numeric_limits<double>::infinity();
    CHECK_FALSE(Transform2::create({0, 0}, inf));
    CHECK_FALSE(Transform2::create({inf, 0}, 0));
    const auto big = Transform2::create({1e308, 0}, 0);
    CHECK_FALSE(compose(*big, *big));
    auto copy = *t;
    copy = identity;
    CHECK(copy.angle() == 0);
    CHECK(t->angle() == std::numbers::pi / 2);
    CHECK((Vec2{0, -0.0} == Vec2{0, 0}));
    CHECK_FALSE((Vec2{1, 1 + 1e-10} == Vec2{1, 1}));
    CHECK_FALSE(t->point({inf, 0}));
    CHECK_FALSE(t->direction({0, inf}));
}
