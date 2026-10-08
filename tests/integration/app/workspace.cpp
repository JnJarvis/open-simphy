#include "workspace.hpp"
#include "grid.hpp"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <limits>
using namespace opensim;
TEST_CASE("Workspace viewport excludes controls at every supported display scale") {
    for (const auto scale : {1.0f, 1.25f, 1.5f, 2.0f})
        for (const auto extent : {renderer::Extent{1440, 900}, renderer::Extent{800, 680}}) {
            const auto l = app::Workspace::layout(extent, scale);
            REQUIRE(l.canvas.width > 0);
            REQUIRE(l.canvas.height > 0);
            CHECK(l.x + l.canvas.width <= extent.width);
            CHECK(l.y + l.canvas.height <= extent.height);
            CHECK(l.contains({double(l.x), double(l.y)}));
            CHECK_FALSE(l.contains({double(l.x) - 1, double(l.y)}));
            CHECK_FALSE(l.contains({double(l.x), double(l.y) - 1}));
            CHECK_FALSE(l.contains({double(l.x + l.canvas.width), double(l.y)}));
            CHECK_FALSE(l.contains({double(l.x), double(l.y + l.canvas.height)}));
            CHECK(l.local({double(l.x) + 40, double(l.y) + 25}) == math::Vec2{40, 25});
            CHECK(l.local({double(l.x) - 10, double(l.y) - 20}) == math::Vec2{-10, -20});
            CHECK(12 + 9 * l.button_width() <= l.width);
        }
}
TEST_CASE("Workspace compact mode and empty output are explicit") {
    CHECK(app::Workspace::layout({1440, 900}, 1.5f).left == 260);
    CHECK(app::Workspace::layout({800, 680}, 1.5f).left > 0);
    CHECK(app::Workspace::layout({0, 0}, 1).canvas == renderer::Extent{0, 0});
    CHECK(app::Workspace::layout({10, 10}, 1).canvas.height == 0);
    CHECK(app::Workspace::layout({800, 680}, std::numeric_limits<float>::quiet_NaN()).scale == 1);
}

TEST_CASE("Grid follows camera coordinates across pan and zoom without covering objects") {
    for (double scale : {1.0 / 1024, 1.0, 80.0, 4096.0})
        for (double center : {-123.25, 0.0, 456.125}) {
            const auto ticks = app::grid_ticks(center, scale, 2048);
            REQUIRE_FALSE(ticks.empty());
            CHECK(ticks.size() <= 512);
            double previous = -1;
            for (auto t : ticks) {
                CHECK(t.pixel > previous);
                CHECK(t.pixel >= 0);
                CHECK(t.pixel < 2048);
                CHECK(t.pixel == Catch::Approx((t.world - center) * scale + 1024));
                previous = t.pixel;
            }
        }
    CHECK(app::grid_ticks(0, 0, 100).empty());
    CHECK(app::grid_ticks(std::numeric_limits<double>::infinity(), 80, 100).empty());
    std::uint8_t background[]{16, 20, 28, 255}, particle[]{64, 192, 255, 255};
    app::grid_pixel(background, true);
    app::grid_pixel(particle, true);
    CHECK(background[0] == 76);
    CHECK(particle[0] == 64);
    CHECK(particle[1] == 192);
}
