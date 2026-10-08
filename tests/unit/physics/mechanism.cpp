#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <opensim/physics/mechanism.hpp>
using namespace opensim;
namespace {
std::shared_ptr<physics::Mechanism> make(const scene::Mechanism &m) {
    auto r = physics::Mechanism::create(m);
    REQUIRE(r.value());
    return *r.value();
}
} // namespace
TEST_CASE("Circular backend free motion, reset and isolation", "[unit][mechanism]") {
    scene::Mechanism m;
    m.gravity = {0, -8};
    m.fixed_dt = 1.0 / 120;
    scene::CircleBody b;
    b.id = {1};
    b.center = {0, 10};
    b.velocity = {2, 3};
    m.bodies.push_back(b);
    auto w = make(m);
    auto original = w->snapshot();
    for (int i = 0; i < 120; ++i)
        REQUIRE(w->step().has_value());
    auto s = w->snapshot();
    REQUIRE(s.time == 1);
    REQUIRE(s.bodies[0].center.x == Catch::Approx(2).margin(.001));
    // Semi-implicit integration at eight substeps: exact trajectory y=9,
    // bounded O(h/substeps) position error .5*8*(1/120)/8=.004167.
    REQUIRE(s.bodies[0].center.y == Catch::Approx(9).margin(.006));
    REQUIRE(s.bodies[0].velocity.y == Catch::Approx(-5).margin(.001));
    REQUIRE(original.bodies[0].center == math::Vec2{0, 10});
    REQUIRE(w->reset().has_value());
    REQUIRE(w->snapshot().time == 0);
    REQUIRE(w->snapshot().bodies[0].center == b.center);
    REQUIRE(w->relocate({1}, {1, 8}).has_value());
    REQUIRE(w->snapshot().bodies[0].center == math::Vec2{1, 8});
    REQUIRE(w->snapshot().bodies[0].velocity == math::Vec2{});
    REQUIRE(w->relocate({99}, {0, 0}).error());
}
TEST_CASE("Elastic equal circles transfer velocity and conserve momentum", "[unit][mechanism]") {
    scene::Mechanism m;
    m.gravity = {0, 0};
    m.fixed_dt = 1.0 / 120;
    scene::CircleBody a;
    a.id = {1};
    a.center = {-1.5, 0};
    a.velocity = {2, 0};
    a.friction = 0;
    scene::CircleBody b = a;
    b.id = {2};
    b.center = {0, 0};
    b.velocity = {0, 0};
    m.bodies = {a, b};
    auto w = make(m);
    for (int i = 0; i < 90; ++i)
        REQUIRE(w->step().has_value());
    auto s = w->snapshot();
    REQUIRE(s.bodies[0].velocity.x == Catch::Approx(0).margin(.02));
    REQUIRE(s.bodies[1].velocity.x == Catch::Approx(2).margin(.02));
    REQUIRE(s.bodies[0].velocity.x + s.bodies[1].velocity.x == Catch::Approx(2).margin(.002));
    REQUIRE(std::pow(s.bodies[0].velocity.x, 2) + std::pow(s.bodies[1].velocity.x, 2) ==
            Catch::Approx(4).margin(.08));
}
TEST_CASE("Distance suspension remains bounded during pendulum motion", "[unit][mechanism]") {
    scene::Mechanism m;
    scene::CircleBody b;
    b.id = {1};
    b.center = {1.8, .6};
    b.fixed_rotation = true;
    b.damping = .1;
    m.bodies = {b};
    m.links.push_back({{1}, {1}, {}, {}, {0, 3}, 3, false});
    auto w = make(m);
    for (int i = 0; i < 600; ++i) {
        REQUIRE(w->step().has_value());
        const auto p = w->snapshot().bodies[0].center;
        REQUIRE(std::hypot(p.x, p.y - 3) == Catch::Approx(3).margin(.01));
    }
}
