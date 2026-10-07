#pragma once
#include <cstdint>
#include <opensim/scene/scene.hpp>
#include <span>
#include <vector>

namespace opensim::renderer {
struct Camera {
    math::Vec2 center;
    double pixels_per_meter;
    bool operator==(const Camera &) const = default;
};
struct Extent {
    std::uint32_t width, height;
    bool operator==(const Extent &) const = default;
};
namespace detail {
struct FrameBuilder;
}
class Frame {
    Extent extent_;
    std::vector<std::uint8_t> bytes_;
    Frame(Extent extent, std::vector<std::uint8_t> bytes)
        : extent_(extent), bytes_(std::move(bytes)) {}
    friend struct detail::FrameBuilder;

  public:
    [[nodiscard]] std::uint32_t width() const { return extent_.width; }
    [[nodiscard]] std::uint32_t height() const { return extent_.height; }
    [[nodiscard]] std::uint32_t stride() const { return extent_.width * 4; }
    [[nodiscard]] std::span<const std::uint8_t> bytes() const { return bytes_; }
};
[[nodiscard]] core::Result<Frame> render(const scene::Snapshot &, const scene::DrawPacket &, Camera,
                                         Extent);
} // namespace opensim::renderer
