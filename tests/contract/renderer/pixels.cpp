#include "fixtures.hpp"
using namespace fixture;
TEST_CASE("R01 empty RGBA frame") {
    const auto f = draw({}, {2, 2});
    REQUIRE(f.width() == 2);
    REQUIRE(f.height() == 2);
    REQUIRE(f.stride() == 8);
    REQUIRE(f.bytes().size() == 16);
    golden(f, {});
}
TEST_CASE("R03-R07 exact circles clipping and capsules") {
    golden(draw({scene::Circle{{0, 0}, 1, {1, 1, 1, 1}}}), {5, 6, 9, 10});
    golden(draw({circle()}), {0});
    golden(draw({circle(1)}), {0, 1, 4});
    golden(draw({scene::Line{{-1.5, 1.5}, {0.5, 1.5}, 1, {1, 1, 1, 1}}}), {0, 1, 2});
    golden(draw({scene::Line{{-1.5, 1.5}, {-1.5, 1.5}, 1, {1, 1, 1, 1}}}), {0});
    golden(draw({scene::Line{{-2.5, 1.5}, {-0.5, -0.5}, 1, {1, 1, 1, 1}}}), {4, 9});
}
TEST_CASE("R08-R11 integer alpha ordering and particles") {
    const auto red = circle(.5, {1, 0, 0, .5}), blue = circle(.5, {0, 0, 1, .5});
    golden(draw({red}), {0}, {136, 10, 14, 255});
    golden(draw({red, blue}), {0}, {68, 5, 135, 255});
    golden(draw({blue, red}), {0}, {132, 5, 71, 255});
    golden(draw({circle(.5, {1, 1, 1, 0})}), {});
    golden(draw({circle()}), {0});
    const auto s = snapshot({{{1}, {1, {-1.5, 1.5}, {0, 0}, .5}}});
    const auto p = renderer::render(s, packet(), {{0, 0}, 1}, {4, 4});
    REQUIRE(p.has_value());
    golden(*p.value(), {0}, {64, 192, 255, 255});
    const auto overlay = renderer::render(s, packet({circle()}), {{0, 0}, 1}, {4, 4});
    REQUIRE(overlay.has_value());
    golden(*overlay.value(), {0});
}
TEST_CASE("R12 resize projects again without stretching") {
    const auto s = snapshot({{{1}, {1, {0, 0}, {0, 0}, 1}}});
    const auto before = s;
    const auto small = renderer::render(s, packet(), {{0, 0}, 1}, {4, 4});
    const auto wide = renderer::render(s, packet(), {{0, 0}, 1}, {8, 4});
    REQUIRE(small.has_value());
    REQUIRE(wide.has_value());
    golden(*small.value(), {5, 6, 9, 10}, {64, 192, 255, 255});
    golden(*wide.value(), {11, 12, 19, 20}, {64, 192, 255, 255});
    REQUIRE(s == before);
}
TEST_CASE("R21 fully clipped shapes have no pixels") {
    golden(draw({scene::Circle{{100, 100}, 1, {1, 1, 1, 1}},
                 scene::Line{{-100, -100}, {-50, -50}, 1, {1, 1, 1, 1}}}),
           {});
}
