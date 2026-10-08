#pragma once
#include <cstddef>
#include <opensim/scene/scene.hpp>
#include <span>
#include <vector>

namespace opensim::serialization {
[[nodiscard]] core::Result<std::vector<std::byte>> encode(const scene::Document &);
[[nodiscard]] core::Result<scene::Document> decode(std::span<const std::byte>);
} // namespace opensim::serialization
