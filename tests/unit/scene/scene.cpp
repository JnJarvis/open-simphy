#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <opensim/scene/scene.hpp>
using namespace opensim::scene;
TEST_CASE("snapshot rejects entity-set changes and invalid fields") {
    const auto d = Document::create(1, {0, 0}, {{{1}, {1, {0, 0}, {0, 0}, 0.5}}}, {5});
    REQUIRE(d.value());
    const ParticleSample valid{{1}, 1, {0, 0}, {0, 0}, 0.5};
    CHECK(Snapshot::create(*d.value(), 1, {}).error());
    for (int field = 0; field < 5; ++field) {
        auto bad = valid;
        if (field == 0)
            bad.id = {2};
        if (field == 1)
            bad.mass = 2;
        if (field == 2)
            bad.display_radius = 1;
        if (field == 3)
            bad.position.x = std::numeric_limits<double>::infinity();
        if (field == 4)
            bad.velocity.y = std::numeric_limits<double>::quiet_NaN();
        CHECK(Snapshot::create(*d.value(), 1, {bad}).error());
    }
    const auto removed = remove_particle(*d.value(), {1});
    REQUIRE(removed.value());
    CHECK(removed.value()->document.particles().empty());
    CHECK(add_particle(removed.value()->document, {1, {0, 0}, {0, 0}, 1})
              .value()
              ->affected_id->value == 6);
}
