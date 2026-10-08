#pragma once
#include <algorithm>
#include <cmath>
#include <opensim/renderer/renderer.hpp>

namespace opensim::app {
// App-private geometry shared by native presentation and input routing.
struct Workspace {
    float scale, width, height, left, right;
    std::uint32_t x, y;
    renderer::Extent canvas;
    static Workspace layout(renderer::Extent output, float display_scale) {
        const float scale = std::isfinite(display_scale) && display_scale > 0
                                ? std::max(1.0f, display_scale)
                                : 1.0f;
        const float width = static_cast<float>(output.width) / scale;
        const float height = static_cast<float>(output.height) / scale;
        const float left = std::min(260.0f, width * .40f);
        const float right = left;
        const auto x =
            std::min(output.width, static_cast<std::uint32_t>(std::ceil((left + 64) * scale)));
        const auto y = std::min(output.height, static_cast<std::uint32_t>(std::ceil(112 * scale)));
        const auto end_x = output.width;
        const auto end_y =
            output.height -
            std::min(output.height, static_cast<std::uint32_t>(std::ceil(32 * scale)));
        return {scale, width, height, left,
                right, x,     y,      {end_x > x ? end_x - x : 0, end_y > y ? end_y - y : 0}};
    }
    bool contains(math::Vec2 point) const {
        return point.x >= x && point.y >= y && point.x < double(x) + canvas.width &&
               point.y < double(y) + canvas.height;
    }
    math::Vec2 local(math::Vec2 point) const { return {point.x - x, point.y - y}; }
    float button_width() const { return std::min(82.0f, std::max(0.0f, width - 24) / 9); }
    float property_y() const { return 172 + list_height() + 46; }
    float property_height() const { return std::max(0.0f, height - 58 - property_y()); }
    float list_height() const { return std::max(0.0f, std::min(96.0f, height * .15f)); }
};
} // namespace opensim::app
