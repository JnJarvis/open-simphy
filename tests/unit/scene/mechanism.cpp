#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <opensim/scene/mechanism.hpp>
using namespace opensim;
TEST_CASE("Mechanism validates solver envelope and references", "[unit][mechanism]") {
    scene::Mechanism m;
    scene::CircleBody b;
    b.id = {1};
    m.bodies.push_back(b);
    REQUIRE(scene::validate(m).has_value());
    SECTION("nonfinite and underflow inputs") {
        m.bodies[0].radius = 1e-50;
        REQUIRE(scene::validate(m).error());
        m.bodies[0].radius = .5;
        m.bodies[0].mass = 1e-50;
        REQUIRE(scene::validate(m).error());
        m.bodies[0].mass = 1;
        m.bodies[0].center.x = std::numeric_limits<double>::quiet_NaN();
        REQUIRE(scene::validate(m).error());
    }
    SECTION("identity and budget") {
        m.bodies.push_back(b);
        REQUIRE(scene::validate(m).error());
        m.bodies.resize(257);
        REQUIRE(scene::validate(m).error());
    }
    SECTION("distance reference and static endpoints") {
        m.links.push_back({{1}, {1}, {}, {}, {0, 3}, 3, false});
        REQUIRE(scene::validate(m).has_value());
        m.links[0].body_a = {2};
        REQUIRE(scene::validate(m).error());
        m.links[0].body_a = {1};
        m.bodies[0].static_body = true;
        REQUIRE(scene::validate(m).error());
    }
}

TEST_CASE("Rigid fixtures reject concavity, winding degeneracy and invalid materials",
          "[unit][rigid]") {
    scene::Mechanism m;
    scene::CircleBody b;
    b.id = {1};
    scene::RigidFixture f;
    f.vertices = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
    b.fixtures = {f};
    m.bodies = {b};
    REQUIRE(scene::validate(m).has_value());
    SECTION("concavity") {
        m.bodies[0].fixtures[0].vertices[2] = {0, 0};
        REQUIRE(scene::validate(m).error());
    }
    SECTION("clockwise") {
        std::reverse(m.bodies[0].fixtures[0].vertices.begin(),
                     m.bodies[0].fixtures[0].vertices.end());
        REQUIRE(scene::validate(m).error());
    }
    SECTION("degenerate") {
        m.bodies[0].fixtures[0].vertices[2] = {1, -1};
        REQUIRE(scene::validate(m).error());
    }
    SECTION("material") {
        m.bodies[0].fixtures[0].friction = -1;
        REQUIRE(scene::validate(m).error());
    }
    SECTION("winding geometry") {
        auto other = b;
        other.id = {2};
        m.bodies.push_back(other);
        m.windings.push_back({{1}, {1}, {2}, {}, {}, 0, 1});
        REQUIRE(scene::validate(m).error());
    }
}
