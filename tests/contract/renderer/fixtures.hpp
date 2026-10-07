#pragma once
#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <opensim/renderer/renderer.hpp>
namespace fixture {
using namespace opensim;
inline scene::Snapshot snapshot(std::vector<scene::Particle> particles = {}) {
    const core::EntityId high{particles.empty() ? 0 : particles.back().id.value};
    const auto doc = scene::Document::create(1, {0, -9.8}, std::move(particles), high);
    REQUIRE(doc.has_value());
    const auto value = scene::Snapshot::initial(*doc.value());
    REQUIRE(value.has_value());
    return *value.value();
}
inline scene::DrawPacket packet(std::vector<scene::Primitive> values = {}) {
    const auto result = scene::DrawPacket::create(std::move(values));
    REQUIRE(result.has_value());
    return *result.value();
}
inline renderer::Frame draw(std::vector<scene::Primitive> values = {},
                            renderer::Extent extent = {4, 4}) {
    const auto result =
        renderer::render(snapshot(), packet(std::move(values)), {{0, 0}, 1}, extent);
    REQUIRE(result.has_value());
    return *result.value();
}
using Pixel = std::array<std::uint8_t, 4>;
inline constexpr Pixel background{16, 20, 28, 255}, white{255, 255, 255, 255};
inline Pixel pixel(const renderer::Frame &f, unsigned x, unsigned y) {
    const auto i = std::size_t(y) * f.stride() + x * 4;
    return {f.bytes()[i], f.bytes()[i + 1], f.bytes()[i + 2], f.bytes()[i + 3]};
}
inline void golden(const renderer::Frame &f, std::vector<unsigned> lit, Pixel color = white) {
    for (unsigned y = 0; y < f.height(); ++y)
        for (unsigned x = 0; x < f.width(); ++x) {
            const auto i = y * f.width() + x;
            const auto expected =
                std::find(lit.begin(), lit.end(), i) != lit.end() ? color : background;
            REQUIRE(pixel(f, x, y) == expected);
        }
}
inline scene::Circle circle(double radius = 0.5, scene::Color color = {1, 1, 1, 1}) {
    return {{-1.5, 1.5}, radius, color};
}
} // namespace fixture
