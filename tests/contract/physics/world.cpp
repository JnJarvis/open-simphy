#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <opensim/physics/world.hpp>
#include <type_traits>
using namespace opensim;
namespace {
scene::Document document(math::Vec2 gravity = {0, -8}, math::Vec2 p = {0, 10},
                         math::Vec2 v = {2, 3}) {
    return *scene::Document::create(1, gravity, {{{1}, {1, p, v, 0.1}}}, {1}).value();
}
physics::World advance(physics::World world, unsigned steps) {
    for (unsigned i = 0; i < steps; ++i) {
        const auto next = world.step();
        REQUIRE(next.value());
        world = *next.value();
    }
    return world;
}
} // namespace
TEST_CASE("P01-P05 construction and exact gravity steps") {
    static_assert(!std::is_default_constructible_v<physics::World>);
    const auto empty = scene::Document::create(1, {0, -8}, {}, {0});
    REQUIRE(empty.value());
    const auto world = physics::World::create(*empty.value(), 1.0 / 128);
    REQUIRE(world.value());
    CHECK(world.value()->step_index() == 0);
    CHECK(world.value()->snapshot().time() == 0);
    const auto stepped = world.value()->step();
    REQUIRE(stepped.value());
    CHECK(stepped.value()->step_index() == 1);
    CHECK(stepped.value()->snapshot().time() == 1.0 / 128);
    CHECK(stepped.value()->snapshot().particles().empty());
    for (double dt : {0., -1., std::numeric_limits<double>::quiet_NaN(),
                      std::numeric_limits<double>::infinity(), 1.0001}) {
        const auto invalid = physics::World::create(document(), dt);
        REQUIRE(invalid.error());
        CHECK(invalid.error()->code == core::Code::invalid_argument);
        CHECK(invalid.error()->path == "fixed_dt");
        CHECK_FALSE(invalid.error()->entity);
    }
    CHECK(physics::World::create(document(), 1).has_value());
    CHECK(
        physics::World::create(document(), std::numeric_limits<double>::denorm_min()).has_value());
    const auto initial = physics::World::create(document(), 0.25);
    REQUIRE(initial.value());
    const auto one = advance(*initial.value(), 1).snapshot();
    CHECK((one.particles()[0].position == math::Vec2{0.5, 10.5}));
    CHECK((one.particles()[0].velocity == math::Vec2{2, 1}));
    CHECK(one.time() == 0.25);
    const auto four = advance(*initial.value(), 4).snapshot();
    CHECK((four.particles()[0].position == math::Vec2{2, 9}));
    CHECK((four.particles()[0].velocity == math::Vec2{2, -5}));
    const auto inertial = physics::World::create(document({0, 0}, {1, -2}, {-3, 4}), 0.125);
    REQUIRE(inertial.value());
    const auto coast = advance(*inertial.value(), 8).snapshot();
    CHECK((coast.particles()[0].position == math::Vec2{-2, 2}));
    CHECK((coast.particles()[0].velocity == math::Vec2{-3, 4}));
}
TEST_CASE("P09-P11 P14-P15 mass independence ownership reset and pause") {
    const auto d = scene::Document::create(
        1, {0, -8}, {{{2}, {100, {0, 10}, {2, 3}, 0.2}}, {{1}, {1, {0, 10}, {2, 3}, 0.1}}}, {2});
    REQUIRE(d.value());
    const auto original_document = *d.value();
    const auto initial = physics::World::create(*d.value(), 0.125);
    REQUIRE(initial.value());
    const auto retained = initial.value()->snapshot();
    const auto later = advance(*initial.value(), 8);
    const auto samples = later.snapshot();
    CHECK(samples.particles()[0].position == samples.particles()[1].position);
    CHECK(samples.particles()[0].velocity == samples.particles()[1].velocity);
    CHECK(samples.particles()[0].id.value == 1);
    CHECK(samples.particles()[1].id.value == 2);
    CHECK(samples.particles()[0].mass == 1);
    CHECK(samples.particles()[1].mass == 100);
    CHECK(samples.particles()[0].display_radius == 0.1);
    CHECK(samples.particles()[1].display_radius == 0.2);
    CHECK(initial.value()->snapshot() == retained);
    CHECK(initial.value()->step_index() == 0);
    CHECK(*d.value() == original_document);
    const auto reset = later.reset();
    REQUIRE(reset.value());
    CHECK(reset.value()->snapshot() == retained);
    CHECK(reset.value()->step_index() == 0);
    CHECK(reset.value()->fixed_dt() == 0.125);
    CHECK(advance(*reset.value(), 8).snapshot() == later.snapshot());
    const auto independent = physics::World::create(original_document, 0.125);
    REQUIRE(independent.value());
    CHECK(advance(*independent.value(), 8).snapshot() == later.snapshot());
    CHECK(later.snapshot() == later.snapshot());
    CHECK(later.step_index() == 8);
    const auto surviving = [] {
        auto local = physics::World::create(document(), 0.25);
        return local.value()->snapshot();
    }();
    CHECK(surviving.time() == 0);
    CHECK(surviving.particles()[0].id.value == 1);
}
TEST_CASE("P12-P13 late overflow rejects whole step without mutation") {
    const double max = std::numeric_limits<double>::max();
    const auto d = scene::Document::create(
        1, {0, 0}, {{{1}, {1, {0, 0}, {1, 1}, 0.1}}, {{2}, {1, {max, 0}, {max, 0}, 0.1}}}, {2});
    REQUIRE(d.value());
    const auto world = physics::World::create(*d.value(), 1);
    REQUIRE(world.value());
    const auto before = world.value()->snapshot();
    const auto failed = world.value()->step();
    REQUIRE(failed.error());
    CHECK(failed.error()->code == core::Code::invalid_data);
    CHECK(failed.error()->entity->value == 2);
    CHECK(failed.error()->path == "particles.position");
    CHECK(world.value()->snapshot() == before);
    CHECK(world.value()->step_index() == 0);
    CHECK(world.value()->reset().value()->snapshot() == before);
    const auto velocity = physics::World::create(document({max, 0}, {-max, 0}, {max, 0}), 1);
    REQUIRE(velocity.value());
    const auto failed_velocity = velocity.value()->step();
    REQUIRE(failed_velocity.error());
    CHECK(failed_velocity.error()->path == "particles.velocity");
    CHECK(failed_velocity.error()->entity->value == 1);
    CHECK(velocity.value()->step_index() == 0);
    CHECK(velocity.value()->snapshot().time() == 0);
}
