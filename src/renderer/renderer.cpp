#include "raster.hpp"
#include <algorithm>
#include <cmath>
#include <type_traits>

namespace opensim::renderer {
namespace {
constexpr double geometry_limit = 1048576;
core::Diagnostic error(core::Code code, const char *path, const char *message,
                       std::optional<core::EntityId> id = std::nullopt) {
    return {code, core::Severity::error, message, id, std::string(path)};
}
bool valid_size(double size) { return std::isfinite(size) && size > 0 && size <= geometry_limit; }
std::uint32_t clipped(double value, std::uint32_t dimension) {
    return static_cast<std::uint32_t>(std::clamp(value, 0.0, double(dimension)));
}
detail::Box bounds(math::Vec2 a, math::Vec2 b, double r, Extent extent) {
    return {clipped(std::floor(std::min(a.x, b.x) - r), extent.width),
            clipped(std::floor(std::min(a.y, b.y) - r), extent.height),
            clipped(std::ceil(std::max(a.x, b.x) + r), extent.width),
            clipped(std::ceil(std::max(a.y, b.y) + r), extent.height)};
}
std::array<std::uint8_t, 4> quantize(scene::Color color) {
    const auto q = [](double c) { return static_cast<std::uint8_t>(std::floor(255 * c + 0.5)); };
    return {q(color.r), q(color.g), q(color.b), q(color.a)};
}
bool covered(const detail::Shape &shape, math::Vec2 p) {
    const auto d = shape.b - shape.a;
    const auto length_squared = math::dot(d, d);
    auto nearest = shape.a;
    if (length_squared != 0) {
        const auto numerator = math::dot(p - shape.a, d);
        // Compare before division: a subnormal denominator must not overflow u.
        const auto u =
            numerator <= 0 ? 0 : (numerator >= length_squared ? 1 : numerator / length_squared);
        nearest = shape.a + u * d;
    }
    const auto delta = p - nearest;
    return math::dot(delta, delta) <= shape.radius * shape.radius;
}
} // namespace
namespace detail {
std::optional<math::Vec2> project(math::Vec2 point, Camera camera, Extent extent) {
    const auto delta = point - camera.center;
    if (!math::finite(delta))
        return std::nullopt;
    const auto scaled = delta * camera.pixels_per_meter;
    if (!math::finite(scaled))
        return std::nullopt;
    const math::Vec2 p{double(extent.width) / 2 + scaled.x, double(extent.height) / 2 - scaled.y};
    if (!math::finite(p) || std::abs(p.x) > geometry_limit || std::abs(p.y) > geometry_limit)
        return std::nullopt;
    return p;
}
core::Result<std::vector<Shape>> preflight(const scene::Snapshot &snapshot,
                                           const scene::DrawPacket &packet, Camera camera,
                                           Extent extent) {
    using Result = core::Result<std::vector<Shape>>;
    const auto pixels = std::uint64_t(extent.width) * extent.height;
    if (extent.width == 0 || extent.height == 0 || extent.width > 4096 || extent.height > 4096 ||
        pixels > 4194304)
        return Result::failure(
            error(core::Code::invalid_argument, "extent", "Frame dimensions exceed limits"));
    if (!math::finite(camera.center) || !std::isfinite(camera.pixels_per_meter) ||
        camera.pixels_per_meter <= 0)
        return Result::failure(error(core::Code::invalid_argument, "camera",
                                     "Camera must be finite with positive scale"));
    if (snapshot.particles().size() > 4096 || packet.primitives().size() > 4096 ||
        snapshot.particles().size() + packet.primitives().size() > 4096)
        return Result::failure(
            error(core::Code::invalid_argument, "primitives", "Too many primitives"));
    std::vector<Shape> shapes;
    shapes.reserve(snapshot.particles().size() + packet.primitives().size());
    std::uint64_t visits = 0;
    const auto append = [&](math::Vec2 a, math::Vec2 b, double size, bool line, scene::Color color,
                            std::optional<core::EntityId> id) -> std::optional<core::Diagnostic> {
        const auto pa = project(a, camera, extent), pb = project(b, camera, extent);
        const auto projected_size = size * camera.pixels_per_meter;
        if (!pa || !pb || !valid_size(projected_size))
            return error(core::Code::invalid_data, "geometry", "Projected geometry exceeds limits",
                         id);
        const auto radius = line ? projected_size / 2 : projected_size;
        const auto box = bounds(*pa, *pb, radius, extent);
        visits += std::uint64_t(box.x1 - box.x0) * (box.y1 - box.y0);
        shapes.push_back({*pa, *pb, radius, quantize(color), box});
        return std::nullopt;
    };
    for (const auto &particle : snapshot.particles()) {
        if (const auto failure =
                append(particle.position, particle.position, particle.display_radius, false,
                       {64.0 / 255, 192.0 / 255, 1, 1}, particle.id))
            return Result::failure(*failure);
    }
    for (const auto &primitive : packet.primitives()) {
        const auto failure = std::visit(
            [&](const auto &value) {
                if constexpr (std::is_same_v<std::decay_t<decltype(value)>, scene::Circle>)
                    return append(value.center, value.center, value.radius, false, value.color,
                                  std::nullopt);
                else
                    return append(value.from, value.to, value.width, true, value.color,
                                  std::nullopt);
            },
            primitive);
        if (failure)
            return Result::failure(*failure);
    }
    if (visits > 67108864)
        return Result::failure(
            error(core::Code::invalid_argument, "work_budget", "Candidate pixel budget exceeded"));
    return Result::success(std::move(shapes));
}
} // namespace detail
core::Result<Frame> render(const scene::Snapshot &snapshot, const scene::DrawPacket &packet,
                           Camera camera, Extent extent) {
    const auto prepared = detail::preflight(snapshot, packet, camera, extent);
    if (prepared.error())
        return core::Result<Frame>::failure(*prepared.error());
    std::vector<std::uint8_t> bytes(std::size_t(extent.width) * extent.height * 4);
    for (std::size_t i = 0; i < bytes.size(); i += 4) {
        bytes[i] = 16;
        bytes[i + 1] = 20;
        bytes[i + 2] = 28;
        bytes[i + 3] = 255;
    }
    for (const auto &shape : *prepared.value()) {
        for (auto y = shape.box.y0; y < shape.box.y1; ++y) {
            for (auto x = shape.box.x0; x < shape.box.x1; ++x) {
                if (!covered(shape, {double(x) + 0.5, double(y) + 0.5}))
                    continue;
                const auto offset = (std::size_t(y) * extent.width + x) * 4;
                const std::uint32_t alpha = shape.color[3];
                for (std::size_t c = 0; c < 3; ++c)
                    bytes[offset + c] = static_cast<std::uint8_t>(
                        (shape.color[c] * alpha + bytes[offset + c] * (255 - alpha) + 127) / 255);
            }
        }
    }
    return core::Result<Frame>::success(detail::FrameBuilder::make(extent, std::move(bytes)));
}
} // namespace opensim::renderer
