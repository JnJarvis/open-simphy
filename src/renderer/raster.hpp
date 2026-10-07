#pragma once
#include <array>
#include <opensim/renderer/renderer.hpp>
namespace opensim::renderer::detail {
struct FrameBuilder {
    static Frame make(Extent extent, std::vector<std::uint8_t> bytes) {
        return Frame(extent, std::move(bytes));
    }
};
struct Box {
    std::uint32_t x0, y0, x1, y1;
};
struct Shape {
    math::Vec2 a, b;
    double radius;
    std::array<std::uint8_t, 4> color;
    Box box;
};
[[nodiscard]] std::optional<math::Vec2> project(math::Vec2, Camera, Extent);
[[nodiscard]] core::Result<std::vector<Shape>> preflight(const scene::Snapshot &,
                                                         const scene::DrawPacket &, Camera, Extent);
} // namespace opensim::renderer::detail
