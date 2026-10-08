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

TEST_CASE("Five elastic circles transfer one and two incoming velocities",
          "[unit][mechanism][cradle]") {
    for (unsigned count : {1u, 2u}) {
        scene::Mechanism m;
        m.gravity = {0, 0};
        m.fixed_dt = 1.0 / 60;
        for (unsigned i = 0; i < 5; ++i) {
            scene::CircleBody b;
            b.id = {i + 1};
            b.radius = .5;
            b.mass = 1;
            b.inertia = .125;
            b.center = {double(i) * 1.01, 0};
            b.velocity = {i < count ? 2.0 : 0.0, 0};
            b.friction = 0;
            b.restitution = 1;
            b.fixed_rotation = true;
            m.bodies.push_back(b);
        }
        auto w = make(m);
        double overlap = 0;
        for (int i = 0; i < 60; ++i) {
            REQUIRE(w->step().has_value());
            auto s = w->snapshot();
            for (unsigned j = 1; j < 5; ++j)
                overlap = std::max(overlap, 1 - (s.bodies[j].center.x - s.bodies[j - 1].center.x));
        }
        auto s = w->snapshot();
        double momentum = 0, energy = 0;
        for (unsigned i = 0; i < 5; ++i) {
            INFO("incoming=" << count << " circle=" << i << " vx=" << s.bodies[i].velocity.x
                             << " overlap=" << overlap);
            const double expected = i >= 5 - count ? 2.0 : 0.0;
            REQUIRE(s.bodies[i].velocity.x == Catch::Approx(expected).margin(.04));
            momentum += s.bodies[i].velocity.x;
            energy += math::dot(s.bodies[i].velocity, s.bodies[i].velocity) / 2;
        }
        REQUIRE(momentum == Catch::Approx(2 * count).margin(.01));
        REQUIRE(energy == Catch::Approx(2 * count).margin(.08));
        REQUIRE(overlap < .005);
    }
}
TEST_CASE("Suspended cradle transfers the released ball count and returns",
          "[unit][mechanism][cradle]") {
    for (unsigned count : {1u, 2u}) {
        scene::Mechanism m;
        m.fixed_dt = 1.0 / 60;
        constexpr double length = 3, theta = .5;
        for (unsigned i = 0; i < 5; ++i) {
            scene::CircleBody b;
            b.id = {i + 1};
            b.radius = .5;
            b.mass = 1;
            b.inertia = .125;
            b.friction = 0;
            b.fixed_rotation = true;
            const double x = double(i) * 1.01;
            b.center =
                i < count ? math::Vec2{x - length * std::sin(theta), length * (1 - std::cos(theta))}
                          : math::Vec2{x, 0};
            m.bodies.push_back(b);
            m.links.push_back({{i + 1}, {i + 1}, {}, {}, {x, length}, length, false});
        }
        auto w = make(m);
        double outgoing[5]{}, returning[5]{};
        double initial_energy = count * 9.8 * length * (1 - std::cos(theta));
        double max_energy = 0, max_overlap = 0;
        for (int k = 0; k < 300; ++k) {
            REQUIRE(w->step().has_value());
            auto s = w->snapshot();
            double energy = 0;
            for (unsigned i = 0; i < 5; ++i) {
                auto &b = s.bodies[i];
                const double dx = b.center.x - double(i) * 1.01;
                if (k < 130)
                    outgoing[i] = std::max(outgoing[i], dx / length);
                if (k > 130)
                    returning[i] = std::max(returning[i], -dx / length);
                energy += .5 * math::dot(b.velocity, b.velocity) + 9.8 * b.center.y;
                REQUIRE(std::hypot(dx, b.center.y - length) == Catch::Approx(length).margin(.005));
                if (i) {
                    auto d = b.center - s.bodies[i - 1].center;
                    max_overlap = std::max(max_overlap, 1 - std::hypot(d.x, d.y));
                }
            }
            max_energy = std::max(max_energy, energy);
        }
        for (unsigned i = 0; i < 5; ++i) {
            INFO("count=" << count << " ball=" << i << " outgoing=" << outgoing[i]
                          << " returning=" << returning[i] << " Emax=" << max_energy
                          << " E0=" << initial_energy << " overlap=" << max_overlap);
            if (i >= 5 - count)
                REQUIRE(outgoing[i] > .85 * std::sin(theta));
            else
                REQUIRE(outgoing[i] < .12);
            if (i < count)
                REQUIRE(returning[i] > .7 * std::sin(theta));
        }
        REQUIRE(max_energy < initial_energy * 1.04);
        REQUIRE(max_overlap < .005);
    }
}

TEST_CASE("Circle contacts honor restitution, mass, static bodies and filtering",
          "[unit][mechanism][collision]") {
    scene::Mechanism m;
    m.gravity = {0, 0};
    m.fixed_dt = 1.0 / 120;
    scene::CircleBody a;
    a.id = {1};
    a.radius = .5;
    a.mass = 1;
    a.inertia = .125;
    a.center = {-1.5, 0};
    a.velocity = {3, 0};
    a.friction = 0;
    a.restitution = .5;
    auto b = a;
    b.id = {2};
    b.center = {0, 0};
    b.velocity = {};
    b.mass = 3;
    b.inertia = .375;
    m.bodies = {a, b};
    auto w = make(m);
    for (int i = 0; i < 70; ++i)
        REQUIRE(w->step().has_value());
    auto s = w->snapshot();
    REQUIRE(s.bodies[0].velocity.x == Catch::Approx(-.375).margin(.01));
    REQUIRE(s.bodies[1].velocity.x == Catch::Approx(1.125).margin(.01));
    REQUIRE(s.bodies[0].velocity.x + 3 * s.bodies[1].velocity.x == Catch::Approx(3).margin(.003));
    m.bodies[1].static_body = true;
    m.bodies[0].restitution = .8;
    m.bodies[1].restitution = .8;
    w = make(m);
    for (int i = 0; i < 70; ++i)
        REQUIRE(w->step().has_value());
    s = w->snapshot();
    REQUIRE(s.bodies[0].velocity.x == Catch::Approx(-2.4).margin(.01));
    REQUIRE(s.bodies[1].center == math::Vec2{0, 0});
    for (bool sensor : {false, true}) {
        m.bodies[1].sensor = sensor;
        m.bodies[0].mask = sensor ? ~std::uint64_t{0} : 0;
        w = make(m);
        for (int i = 0; i < 70; ++i)
            REQUIRE(w->step().has_value());
        s = w->snapshot();
        REQUIRE(s.bodies[0].velocity.x == Catch::Approx(3).margin(.001));
        REQUIRE(s.bodies[0].center.x > 0);
    }
}
TEST_CASE("Oblique elastic circle impact matches analytic normal impulse",
          "[unit][mechanism][collision]") {
    scene::Mechanism m;
    m.gravity = {};
    m.fixed_dt = 1.0 / 120;
    scene::CircleBody a;
    a.id = {1};
    a.radius = .5;
    a.mass = 1;
    a.inertia = .125;
    a.center = {-1.5, 0};
    a.velocity = {2, .5};
    a.friction = 0;
    auto b = a;
    b.id = {2};
    b.center = {0, 0};
    b.velocity = {};
    m.bodies = {a, b};
    // Earliest root of |(-1.5,0)+(2,.5)t|^2=1.
    const double t = (6 - std::sqrt(14.75)) / 8.5;
    const math::Vec2 n{1.5 - 2 * t, -.5 * t};
    const auto transferred = n * math::dot(a.velocity, n), remaining = a.velocity - transferred;
    auto w = make(m);
    for (int i = 0; i < 90; ++i)
        REQUIRE(w->step().has_value());
    auto s = w->snapshot();
    REQUIRE(s.bodies[0].velocity.x == Catch::Approx(remaining.x).margin(.02));
    REQUIRE(s.bodies[0].velocity.y == Catch::Approx(remaining.y).margin(.02));
    REQUIRE(s.bodies[1].velocity.x == Catch::Approx(transferred.x).margin(.02));
    REQUIRE(s.bodies[1].velocity.y == Catch::Approx(transferred.y).margin(.02));
    auto total = s.bodies[0].velocity + s.bodies[1].velocity;
    REQUIRE(total.x == Catch::Approx(2).margin(.003));
    REQUIRE(total.y == Catch::Approx(.5).margin(.003));
    REQUIRE(math::dot(s.bodies[0].velocity, s.bodies[0].velocity) +
                math::dot(s.bodies[1].velocity, s.bodies[1].velocity) ==
            Catch::Approx(4.25).margin(.03));
}

namespace {
scene::RigidFixture box(double x, double y, double mu = 0) {
    scene::RigidFixture f;
    f.vertices = {{-x, -y}, {x, -y}, {x, y}, {-x, y}};
    f.friction = mu;
    return f;
}
scene::Mechanism friction_world(double mu) {
    scene::Mechanism m;
    m.gravity = {0, -10};
    scene::CircleBody ground;
    ground.id = {1};
    ground.static_body = true;
    ground.center = {0, -.5};
    ground.fixtures = {box(20, .5, mu)};
    scene::CircleBody block;
    block.id = {2};
    block.center = {0, .51};
    block.fixed_rotation = true;
    block.fixtures = {box(.5, .5, mu)};
    m.bodies = {ground, block};
    return m;
}
} // namespace
TEST_CASE("Static friction threshold and kinetic deceleration match Coulomb reference",
          "[unit][rigid][friction]") {
    auto w = make(friction_world(.5));
    for (int i = 0; i < 120; ++i)
        REQUIRE(w->step().has_value());
    REQUIRE(w->forces({{{2}, {4, 0}, {}, false, 0}}).has_value());
    for (int i = 0; i < 60; ++i)
        REQUIRE(w->step().has_value());
    REQUIRE(std::abs(w->snapshot().bodies[1].velocity.x) < .01);
    REQUIRE(std::abs(w->snapshot().bodies[1].center.x) < .01);
    REQUIRE(w->forces({{{2}, {6, 0}, {}, false, 0}}).has_value());
    for (int i = 0; i < 60; ++i)
        REQUIRE(w->step().has_value());
    REQUIRE(w->snapshot().bodies[1].velocity.x == Catch::Approx(1).margin(.06));
    REQUIRE(w->forces({}).has_value());
    for (int i = 0; i < 6; ++i)
        REQUIRE(w->step().has_value());
    REQUIRE(w->snapshot().bodies[1].velocity.x == Catch::Approx(.5).margin(.08));
    REQUIRE(w->friction({1}, 0).has_value());
    REQUIRE(w->friction({2}, 0).has_value());
    const double v = w->snapshot().bodies[1].velocity.x;
    for (int i = 0; i < 60; ++i)
        REQUIRE(w->step().has_value());
    REQUIRE(w->snapshot().bodies[1].velocity.x == Catch::Approx(v).margin(.03));
    REQUIRE(w->friction({2}, -1).error());
    REQUIRE(w->forces({{{99}, {1, 0}, {}, false, 0}}).error());
}
TEST_CASE("Rigid angular force and compound COM use explicit inertia", "[unit][rigid]") {
    scene::Mechanism m;
    m.gravity = {0, 0};
    scene::CircleBody b;
    b.id = {1};
    b.mass = 2;
    b.inertia = 4;
    b.fixtures = {box(.5, .5), box(.25, .25)};
    b.fixtures[1].vertices.clear();
    b.fixtures[1].center = {2, 0};
    b.fixtures[1].radius = .25;
    m.bodies = {b};
    auto w = make(m);
    REQUIRE(w->forces({{{1}, {2, 0}, {0, 1}, false, 0}}).has_value());
    // Rotation changes the fixed body point's moment arm; wrapped mode holds torque -2 Nm.
    REQUIRE(w->forces({{{1}, {2, 0}, {}, true, -1}}).has_value());
    for (int i = 0; i < 60; ++i)
        REQUIRE(w->step().has_value());
    auto s = w->snapshot();
    REQUIRE(s.bodies[0].velocity.x == Catch::Approx(1).margin(.002));
    REQUIRE(s.bodies[0].angular_velocity == Catch::Approx(-.5).margin(.002));
    REQUIRE(s.bodies[0].center.y == Catch::Approx(0).margin(.001));
    REQUIRE(w->reset().has_value());
    REQUIRE(w->snapshot().bodies[0].center == b.center);
    REQUIRE(w->snapshot().bodies[0].angle == 0);
}
TEST_CASE("Revolute constraint retains anchor under off-center force", "[unit][rigid]") {
    scene::Mechanism m;
    m.gravity = {0, 0};
    scene::CircleBody b;
    b.id = {1};
    b.fixtures = {box(.5, .5)};
    m.bodies = {b};
    m.hinges.push_back({{1}, {1}, {}, {}, {}, 0, 0, 0, 0, 0, false, false, false});
    auto w = make(m);
    REQUIRE(w->forces({{{1}, {5, 0}, {0, 1}, false, 0}}).has_value());
    for (int i = 0; i < 120; ++i)
        REQUIRE(w->step().has_value());
    REQUIRE(std::hypot(w->snapshot().bodies[0].center.x, w->snapshot().bodies[0].center.y) < .005);
    REQUIRE(std::abs(w->snapshot().bodies[0].angle) > .1);
}
TEST_CASE("Winding transfers spool rotation into linear impulse with torque reaction",
          "[unit][rigid][winding]") {
    scene::Mechanism m;
    m.gravity = {0, 0};
    m.fixed_dt = .001;
    scene::CircleBody a;
    a.id = {1};
    a.radius = .1;
    a.inertia = 1;
    auto b = a;
    b.id = {2};
    b.center = {3, 0};
    b.angular_velocity = 1;
    m.bodies = {a, b};
    m.windings.push_back({{1}, {1}, {2}, {}, {0, 1}, 0, -3 / std::sqrt(10.0), true});
    auto w = make(m);
    REQUIRE(w->step().has_value());
    auto s = w->snapshot();
    // Independently measured public-API numerical vector (see RFC), with short-step drift bound.
    REQUIRE(s.bodies[0].velocity.x == Catch::Approx(-.310344827586).margin(.02));
    REQUIRE(s.bodies[0].velocity.y == Catch::Approx(-.103448275862).margin(.02));
    REQUIRE(s.bodies[1].angular_velocity == Catch::Approx(.689655172414).margin(.03));
    REQUIRE(s.bodies[0].velocity.x + s.bodies[1].velocity.x == Catch::Approx(0).margin(.00001));
    for (int i = 0; i < 2000; ++i)
        REQUIRE(w->step().has_value());
    const auto after = w->snapshot();
    REQUIRE(std::abs(after.bodies[1].angular_velocity) < 1.01);
    REQUIRE(w->reset().has_value());
    REQUIRE(w->snapshot().bodies[1].angular_velocity == 1);
}
TEST_CASE("Polygon contacts exchange velocity and sensors/filter bypass response",
          "[unit][rigid]") {
    for (int mode = 0; mode < 3; ++mode) {
        scene::Mechanism m;
        m.gravity = {0, 0};
        scene::CircleBody a;
        a.id = {1};
        a.fixed_rotation = true;
        a.center = {-2, 0};
        a.velocity = {2, 0};
        auto f = box(.5, .5);
        f.restitution = 1;
        if (mode == 1)
            f.sensor = true;
        if (mode == 2)
            f.mask = 0;
        a.fixtures = {f};
        auto b = a;
        b.id = {2};
        b.center = {0, 0};
        b.velocity = {};
        b.fixtures[0].sensor = false;
        b.fixtures[0].mask = ~std::uint64_t{0};
        m.bodies = {a, b};
        auto w = make(m);
        for (int i = 0; i < 90; ++i)
            REQUIRE(w->step().has_value());
        auto s = w->snapshot();
        if (mode == 0) {
            REQUIRE(s.bodies[0].velocity.x == Catch::Approx(0).margin(.03));
            REQUIRE(s.bodies[1].velocity.x == Catch::Approx(2).margin(.03));
        } else {
            REQUIRE(s.bodies[0].velocity.x == Catch::Approx(2).margin(.001));
            REQUIRE(s.bodies[1].velocity.x == Catch::Approx(0).margin(.001));
        }
    }
}

TEST_CASE("Inclined triangle contact holds above threshold and slides below it",
          "[unit][rigid][friction]") {
    scene::Mechanism m;
    m.gravity = {0, -10};
    scene::CircleBody wedge;
    wedge.id = {1};
    wedge.static_body = true;
    scene::RigidFixture plane;
    plane.vertices = {{-5, -2}, {5, -2}, {5, 3}};
    plane.friction = .7;
    wedge.fixtures = {plane};
    wedge.radius = 6;
    scene::CircleBody block;
    block.id = {2};
    block.fixed_rotation = true;
    block.angle = std::atan(.5);
    block.center = {0, .5 + .25 * std::sqrt(1.25)};
    block.fixtures = {box(.25, .25, .7)};
    m.bodies = {wedge, block};
    auto w = make(m);
    for (int i = 0; i < 60; ++i)
        REQUIRE(w->step().has_value());
    REQUIRE(std::abs(w->snapshot().bodies[1].velocity.x) < .01);
    REQUIRE(w->friction({1}, .1).has_value());
    REQUIRE(w->friction({2}, .1).has_value());
    for (int i = 0; i < 60; ++i)
        REQUIRE(w->step().has_value());
    // a=g(sin(theta)-mu*cos(theta)); ax=-a*cos(theta)=-3.2 m/s^2.
    REQUIRE(w->snapshot().bodies[1].velocity.x == Catch::Approx(-3.2).margin(.12));
    REQUIRE(w->snapshot().bodies[1].velocity.y == Catch::Approx(-1.6).margin(.12));
}

TEST_CASE("Contact friction mixers preserve independent surface coefficients",
          "[unit][rigid][friction]") {
    for (auto mode : {scene::MaterialMixer::minimum, scene::MaterialMixer::geometric_mean,
                      scene::MaterialMixer::maximum}) {
        auto m = friction_world(.5);
        m.bodies[0].fixtures[0].friction = 0;
        m.friction_mixer = mode;
        auto w = make(m);
        for (int i = 0; i < 60; ++i)
            REQUIRE(w->step().has_value());
        REQUIRE(w->forces({{{2}, {4, 0}, {}, false, 0}}).has_value());
        for (int i = 0; i < 60; ++i)
            REQUIRE(w->step().has_value());
        if (mode == scene::MaterialMixer::maximum)
            REQUIRE(std::abs(w->snapshot().bodies[1].velocity.x) < .01);
        else
            REQUIRE(w->snapshot().bodies[1].velocity.x == Catch::Approx(4).margin(.02));
    }
}

TEST_CASE("Hinge angular limits stop a motor at the declared bound", "[unit][rigid]") {
    scene::Mechanism m;
    m.gravity = {0, 0};
    scene::CircleBody b;
    b.id = {1};
    b.fixtures = {box(.5, .5)};
    m.bodies = {b};
    // Canonical hinge angle is body_b minus body_a; a ground endpoint means -body_a.angle.
    m.hinges.push_back({{1}, {1}, {}, {}, {}, 0, -.2, .3, 1, 5, true, true, false});
    auto w = make(m);
    for (int i = 0; i < 180; ++i)
        REQUIRE(w->step().has_value());
    REQUIRE(w->snapshot().bodies[0].angle == Catch::Approx(-.3).margin(.02));
    REQUIRE(std::abs(w->snapshot().bodies[0].angular_velocity) < .02);
}
