#pragma once
#include <cstdint>
#include <map>
#include <opensim/core/values.hpp>
#include <opensim/math/math.hpp>
#include <span>
#include <string>
#include <vector>
namespace opensim::compat {
inline constexpr std::size_t max_ssim_bytes = 64 * 1024 * 1024;
struct Outline {
    std::string body;
    std::vector<math::Vec2> points;
    bool closed = true;
};
struct Project {
    std::vector<std::uint8_t> source;
    std::string title, version;
    std::vector<std::string> members, diagnostics;
    std::map<std::string, std::size_t> elements, shapes, joints;
    std::vector<Outline> outlines;
    std::size_t scripts = 0, preview_points = 0, omitted_outlines = 0;
};
core::Result<Project> inspect_ssim(std::span<const std::uint8_t> bytes);
} // namespace opensim::compat
