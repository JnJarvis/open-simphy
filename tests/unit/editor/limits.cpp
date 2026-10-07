#include "../../contract/editor/fixture.hpp"
#include <limits>
using namespace test_editor;
TEST_CASE("E02 E04-E09 checked mapping and deterministic picking") {
    auto s = state();
    REQUIRE(take(s.world_at({500, 100})) == math::Vec2{1, 2});
    REQUIRE(take(s.world_at({400, 400})) == math::Vec2{0, -1});
    auto other = take(s.set_view({{2, -1}, 3, 800, 600}));
    const auto p = take(other.world_at({100, 200}));
    REQUIRE(p.x == -98);
    REQUIRE(std::abs(p.y - 32.333333333333336) < 1e-12);
    auto overlap = state({{{2}, fields()}, {{9}, fields()}});
    REQUIRE(take(overlap.hit_test({400, 300})) == core::EntityId{9});
    auto tiny = fields();
    tiny.display_radius = .01;
    auto pick = state({{{1}, tiny}});
    REQUIRE(take(pick.hit_test({406, 300})) == core::EntityId{1});
    REQUIRE_FALSE(take(pick.hit_test({406.25, 300})));
    REQUIRE_FALSE(take(pick.hit_test({-1, 300})));
    REQUIRE_FALSE(take(pick.hit_test({800, 300})));
    REQUIRE(pick.world_at({-1, 300}).has_value());
    const double nan = std::numeric_limits<double>::quiet_NaN(),
                 inf = std::numeric_limits<double>::infinity();
    for (const auto v : std::vector<editor::View>{{{0, 0}, 1, 0, 1},
                                                  {{0, 0}, 1, 4097, 1},
                                                  {{0, 0}, 1, 4096, 4096},
                                                  {{nan, 0}, 1, 800, 600},
                                                  {{0, 0}, 0, 800, 600},
                                                  {{0, 0}, 1.0 / 2048, 800, 600},
                                                  {{0, 0}, 8192, 800, 600},
                                                  {{0, 0}, inf, 800, 600}})
        REQUIRE(s.set_view(v).error());
    REQUIRE(s.hit_test({nan, 0}).error()->path == "pointer");
    REQUIRE(s.world_at({inf, 0}).error()->path == "pointer");
    const auto max = std::numeric_limits<double>::max();
    auto huge = take(state({{{1}, fields({max, 0})}}).set_view({{-max, 0}, 1, 800, 600}));
    REQUIRE(huge.hit_test({400, 300}).error()->path == "picking.geometry");
    REQUIRE(huge.hit_test({400, 300}).error()->entity == core::EntityId{1});
}
TEST_CASE("E25 particle capacity never consumes an ID") {
    std::vector<scene::Particle> particles;
    for (std::uint64_t i = 1; i <= 4096; ++i)
        particles.push_back({{i}, fields()});
    auto s = state(particles);
    REQUIRE(s.add(fields()).error()->path == "editor.primitives");
    REQUIRE(s.document().high_water() == core::EntityId{4096});
    particles.push_back({{4097}, fields()});
    const auto d = take(scene::Document::create(1, {0, 0}, particles, {4097}));
    REQUIRE(editor::State::create(d, {{0, 0}, 1, 800, 600}).error()->path == "editor.primitives");
}
