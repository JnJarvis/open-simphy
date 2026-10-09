#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <limits>
#include <numbers>
#include <opensim/physics/mechanism.hpp>
using namespace opensim;
namespace {
scene::Mechanism charged(double q = 1) {
    scene::Mechanism m;
    m.gravity = {};
    m.fixed_dt = .002;
    scene::CircleBody b;
    b.id = {1};
    b.charge = q;
    b.radius = .01;
    m.bodies = {b};
    return m;
}
std::shared_ptr<physics::Mechanism> runtime(const scene::Mechanism &m) {
    auto result = physics::Mechanism::create(m);
    REQUIRE(result.has_value());
    return *result.value();
}
double speed(math::Vec2 v) { return std::hypot(v.x, v.y); }
} // namespace
TEST_CASE("Electric fields have SI acceleration and zero charge or disabled fields stay inert",
          "[unit][INT-011]") {
    for (double q : {-2.0, 0.0, 2.0}) {
        auto m = charged(q);
        m.bodies[0].mass = 2;
        scene::ElectromagneticField f;
        f.electric = {3, 4};
        m.fields = {f};
        auto w = runtime(m);
        for (int i = 0; i < 500; ++i)
            REQUIRE(w->step().has_value());
        const auto s = w->snapshot().bodies[0];
        REQUIRE(s.velocity.x == Catch::Approx(1.5 * q).margin(.001));
        REQUIRE(s.velocity.y == Catch::Approx(2 * q).margin(.001));
        REQUIRE(s.center.x == Catch::Approx(.75 * q).margin(.002));
        REQUIRE(s.center.y == Catch::Approx(q).margin(.002));
        f.enabled = false;
        REQUIRE(w->fields({f}).has_value());
        const auto before = w->snapshot().bodies[0].velocity;
        REQUIRE(w->step().has_value());
        REQUIRE(w->snapshot().bodies[0].velocity == before);
    }
}
TEST_CASE("Magnetic motion preserves energy and follows signed analytic radius and frequency",
          "[unit][INT-011]") {
    for (double q : {-1.0, 1.0}) {
        auto m = charged(q);
        m.bodies[0].velocity = {4, 0};
        scene::ElectromagneticField f;
        f.magnetic = 2;
        m.fields = {f};
        auto w = runtime(m);
        for (int i = 0; i < 500; ++i) {
            REQUIRE(w->step().has_value());
            const auto s = w->snapshot().bodies[0];
            const double omega = 2 * q, t = w->snapshot().time;
            REQUIRE(speed(s.velocity) == Catch::Approx(4).margin(.001));
            REQUIRE(s.velocity.x == Catch::Approx(4 * std::cos(omega * t)).margin(.002));
            REQUIRE(s.velocity.y == Catch::Approx(-4 * std::sin(omega * t)).margin(.002));
            REQUIRE(s.center.x == Catch::Approx(4 * std::sin(omega * t) / omega).margin(.003));
            REQUIRE(s.center.y ==
                    Catch::Approx(4 * (std::cos(omega * t) - 1) / omega).margin(.003));
            REQUIRE(std::hypot(s.center.x, s.center.y + 4 / omega) ==
                    Catch::Approx(2).margin(.003));
        }
    }
}
TEST_CASE("Coulomb signs inverse square and static reactions are equal opposite",
          "[unit][INT-011]") {
    for (double sign : {-1.0, 1.0})
        for (double r : {1.0, 2.0, 4.0}) {
            auto m = charged(1e-5);
            m.bodies[0].static_body = true;
            auto b = m.bodies[0];
            b.id = {2};
            b.center = {r, 0};
            b.charge *= sign;
            m.bodies.push_back(b);
            auto w = runtime(m);
            REQUIRE(w->step().has_value());
            auto forces = w->electromagnetic_forces();
            REQUIRE(forces.size() == 2);
            REQUIRE(forces[1].coulomb.x == Catch::Approx(sign * .9 / (r * r)).margin(1e-12));
            REQUIRE(forces[0].coulomb.x == -forces[1].coulomb.x);
            REQUIRE(w->snapshot().bodies[1].center == b.center);
        }
    auto m = charged(1e-5);
    auto b = m.bodies[0];
    b.id = {2};
    b.center = {2, 0};
    m.bodies.push_back(b);
    auto w = runtime(m);
    for (int i = 0; i < 500; ++i)
        REQUIRE(w->step().has_value());
    const auto s = w->snapshot();
    REQUIRE(s.bodies[0].velocity.x + s.bodies[1].velocity.x == Catch::Approx(0).margin(1e-6));
    REQUIRE(s.bodies[0].center.x < 0);
    REQUIRE(s.bodies[1].center.x > 2);
}
TEST_CASE("Coulomb circular orbit has bounded electrostatic energy", "[unit][INT-011]") {
    auto m = charged(1e-4);
    m.bodies[0].static_body = true;
    auto b = m.bodies[0];
    b.id = {2};
    b.static_body = false;
    b.charge = -1e-4;
    b.center = {2, 0};
    b.velocity = {0, std::sqrt(45.0)};
    m.bodies.push_back(b);
    auto w = runtime(m);
    for (int i = 0; i < 1000; ++i) {
        REQUIRE(w->step().has_value());
        const auto s = w->snapshot().bodies[1];
        const double r = speed(s.center), v = speed(s.velocity);
        REQUIRE(r == Catch::Approx(2).margin(.003));
        REQUIRE(.5 * v * v - 90 / r == Catch::Approx(-22.5).margin(.01));
    }
}
TEST_CASE("Charge updates and field regions are transactional and reset retains original charge",
          "[unit][INT-011]") {
    auto m = charged();
    auto w = runtime(m);
    scene::BodyUpdate a;
    a.body = {1};
    a.charge = 2;
    auto invalid = a;
    invalid.body = {99};
    REQUIRE(w->update({a, invalid}).error());
    scene::ElectromagneticField f;
    f.electric = {3, 0};
    REQUIRE(w->fields({f}).has_value());
    REQUIRE(w->step().has_value());
    REQUIRE(w->electromagnetic_forces()[0].electric.x == 3);
    REQUIRE(w->update({a}).has_value());
    auto bad_field = f;
    bad_field.magnetic = std::numeric_limits<double>::infinity();
    REQUIRE(w->fields({bad_field}).error());
    REQUIRE(w->step().has_value());
    REQUIRE(w->electromagnetic_forces()[0].electric.x == 6);
    REQUIRE(w->reset().has_value());
    REQUIRE(w->fields({f}).has_value());
    REQUIRE(w->step().has_value());
    REQUIRE(w->electromagnetic_forces()[0].electric.x == 3);
    a.charge = std::numeric_limits<double>::quiet_NaN();
    REQUIRE(w->update({a}).error());
}
TEST_CASE("Fields test COM against translated rotated regions and singular charges fail explicitly",
          "[unit][INT-011]") {
    auto m = charged();
    m.bodies[0].static_body = true;
    scene::ElectromagneticField f;
    f.electric = {1, 0};
    const double c = std::sqrt(.5);
    for (const auto p :
         {math::Vec2{-5, -.5}, math::Vec2{5, -.5}, math::Vec2{5, .5}, math::Vec2{-5, .5}})
        f.vertices.push_back({2 + c * (p.x - p.y), 3 + c * (p.x + p.y)});
    m.fields = {f};
    auto w = runtime(m);
    for (const auto p : {math::Vec2{5, 6}, math::Vec2{5, 0}}) {
        scene::BodyUpdate u;
        u.body = {1};
        u.center = p;
        REQUIRE(w->update({u}).has_value());
        REQUIRE(w->step().has_value());
        REQUIRE(w->electromagnetic_forces()[0].electric.x == (p.y == 6 ? 1 : 0));
    }
    f.vertices.clear();
    f.center = {2, 3};
    f.radius = 2;
    REQUIRE(w->fields({f}).has_value());
    scene::BodyUpdate u;
    u.body = {1};
    u.center = math::Vec2{4, 3};
    REQUIRE(w->update({u}).has_value());
    REQUIRE(w->step().has_value());
    REQUIRE(w->electromagnetic_forces()[0].electric.x == 1);
    auto b = m.bodies[0];
    b.id = {2};
    m.bodies.push_back(b);
    auto singular = runtime(m);
    REQUIRE(singular->step().error());
    REQUIRE(singular->snapshot().time == 0);
    REQUIRE(singular->step().error());
    REQUIRE(singular->reset().has_value());
}
