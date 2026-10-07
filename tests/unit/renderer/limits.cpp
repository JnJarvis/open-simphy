#include "../../contract/renderer/fixtures.hpp"
#include "raster.hpp"
#include <limits>
using namespace fixture;
using opensim::renderer::detail::preflight;
TEST_CASE("R02 projection scale and y orientation") {
    const auto a = renderer::detail::project({1, 2}, {{0, 0}, 100}, {800, 600});
    const auto b = renderer::detail::project({0, -1}, {{0, 0}, 100}, {800, 600});
    REQUIRE(a == math::Vec2{500, 100});
    REQUIRE(b == math::Vec2{400, 400});
    const auto prepared = preflight(snapshot(),
                                    packet({scene::Circle{{1, 2}, .5, {1, 1, 1, 1}},
                                            scene::Line{{0, 0}, {1, 0}, .1, {1, 1, 1, 1}}}),
                                    {{0, 0}, 100}, {800, 600});
    REQUIRE(prepared.has_value());
    REQUIRE(prepared.value()->at(0).radius == 50);
    REQUIRE(prepared.value()->at(1).radius == 5);
}
TEST_CASE("R14-R16 extent and camera limits") {
    const auto s = snapshot();
    const auto p = packet();
    for (const auto extent : std::vector<renderer::Extent>{
             {0, 1}, {1, 0}, {4097, 1}, {1, 4097}, {4096, 4096}, {2049, 2048}}) {
        const auto f = renderer::render(s, p, {{0, 0}, 1}, extent);
        REQUIRE(f.error());
        REQUIRE(f.error()->code == core::Code::invalid_argument);
        REQUIRE(f.error()->path == "extent");
        REQUIRE(f.error()->severity == core::Severity::error);
    }
    REQUIRE(preflight(s, p, {{0, 0}, 1}, {2048, 2048}).has_value());
    const double nan = std::numeric_limits<double>::quiet_NaN(),
                 inf = std::numeric_limits<double>::infinity();
    for (const auto c : std::vector<renderer::Camera>{{{nan, 0}, 1},
                                                      {{0, inf}, 1},
                                                      {{inf, 0}, 1},
                                                      {{0, nan}, 1},
                                                      {{0, 0}, 0},
                                                      {{0, 0}, -1},
                                                      {{0, 0}, inf},
                                                      {{0, 0}, nan}}) {
        const auto f = renderer::render(s, p, c, {4, 4});
        REQUIRE(f.error());
        REQUIRE(f.error()->code == core::Code::invalid_argument);
        REQUIRE(f.error()->path == "camera");
    }
}
TEST_CASE("R17-R18 invalid projected geometry retains diagnostic identity") {
    const auto max = std::numeric_limits<double>::max();
    const auto s = snapshot({{{7}, {1, {max, 0}, {0, 0}, 1}}});
    const auto f = renderer::render(s, packet(), {{-max, 0}, 1}, {4, 4});
    REQUIRE(f.error());
    REQUIRE(f.error()->entity == core::EntityId{7});
    REQUIRE(f.error()->code == core::Code::invalid_data);
    REQUIRE(f.error()->path == "geometry");
    for (const auto &primitive : std::vector<scene::Primitive>{
             scene::Circle{{0, 0}, max, {1, 1, 1, 0}}, scene::Circle{{1048577, 0}, 1, {1, 1, 1, 0}},
             scene::Line{{0, 0}, {max, 0}, 1, {1, 1, 1, 0}},
             scene::Line{{0, 0}, {0, 0}, max, {1, 1, 1, 0}}}) {
        const auto result = renderer::render(snapshot(), packet({primitive}), {{0, 0}, 1}, {4, 4});
        REQUIRE(result.error());
        REQUIRE(result.error()->path == "geometry");
        REQUIRE_FALSE(result.error()->entity);
    }
    const auto tiny = std::numeric_limits<double>::denorm_min();
    const auto result = renderer::render(
        snapshot(), packet({scene::Circle{{0, 0}, tiny, {1, 1, 1, 1}}}), {{0, 0}, .5}, {4, 4});
    REQUIRE(result.error());
    REQUIRE(result.error()->path == "geometry");
    const auto overflow = renderer::render(s, packet(), {{0, 0}, 2}, {4, 4});
    REQUIRE(overflow.error());
    REQUIRE(overflow.error()->path == "geometry");
}
TEST_CASE("R19-R20 primitive and exact candidate work budgets") {
    std::vector<scene::Primitive> many(4097, scene::Circle{{100, 100}, 1, {1, 1, 1, 0}});
    const auto count = preflight(snapshot(), packet(many), {{0, 0}, 1}, {4, 4});
    REQUIRE(count.error());
    REQUIRE(count.error()->path == "primitives");
    many.resize(4096);
    REQUIRE(preflight(snapshot(), packet(many), {{0, 0}, 1}, {4, 4}).has_value());
    const auto s = snapshot({{{1}, {1, {0, 0}, {0, 0}, 1}}});
    REQUIRE(preflight(s, packet(many), {{0, 0}, 1}, {4, 4}).error()->path == "primitives");
    std::vector<scene::Primitive> big(16, scene::Circle{{0, 0}, 2048, {1, 1, 1, 0}});
    REQUIRE(preflight(snapshot(), packet(big), {{0, 0}, 1}, {2048, 2048}).has_value());
    big.push_back(big.front());
    const auto budget = preflight(snapshot(), packet(big), {{0, 0}, 1}, {2048, 2048});
    REQUIRE(budget.error());
    REQUIRE(budget.error()->code == core::Code::invalid_argument);
    REQUIRE(budget.error()->path == "work_budget");
}
