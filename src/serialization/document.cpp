#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <cstdint>
#include <limits>
#include <opensim/serialization/document.hpp>

namespace opensim::serialization {
namespace {
static_assert(CHAR_BIT == 8 && sizeof(double) == 8 && std::numeric_limits<double>::is_iec559 &&
              std::numeric_limits<double>::digits == 53);
constexpr std::size_t header_size = 48, record_size = 56, max_count = 4096;
constexpr std::size_t max_size = header_size + record_size * max_count;
constexpr std::array magic{std::byte{0x4f}, std::byte{0x50}, std::byte{0x53}, std::byte{0x49},
                           std::byte{0x4d}, std::byte{0x44}, std::byte{0x4f}, std::byte{0x43}};
core::Diagnostic error(core::Code code, const char *path) {
    return {code,
            core::Severity::error,
            "Invalid native document field: " + std::string(path),
            {},
            path};
}
// Callers establish the complete fixed-size layout before accessing any field.
std::uint64_t read(std::span<const std::byte> bytes, std::size_t offset, std::size_t width) {
    std::uint64_t value = 0;
    for (std::size_t i = 0; i < width; ++i)
        value |= std::uint64_t{std::to_integer<unsigned char>(bytes[offset + i])} << (8 * i);
    return value;
}
void write(std::span<std::byte> bytes, std::size_t offset, std::uint64_t value, std::size_t width) {
    for (std::size_t i = 0; i < width; ++i)
        bytes[offset + i] = static_cast<std::byte>((value >> (8 * i)) & 0xff);
}
double scalar(std::span<const std::byte> bytes, std::size_t offset) {
    return std::bit_cast<double>(read(bytes, offset, 8));
}
void scalar(std::span<std::byte> bytes, std::size_t offset, double value) {
    write(bytes, offset, std::bit_cast<std::uint64_t>(value), 8);
}
} // namespace
core::Result<std::vector<std::byte>> encode(const scene::Document &document) {
    if (document.particles().size() > max_count)
        return core::Result<std::vector<std::byte>>::failure(
            error(core::Code::invalid_argument, "file.count"));
    std::vector<std::byte> bytes(header_size + record_size * document.particles().size());
    std::copy(magic.begin(), magic.end(), bytes.begin());
    write(bytes, 8, 1, 4);
    write(bytes, 12, document.revision(), 4);
    write(bytes, 16, document.particles().size(), 4);
    write(bytes, 24, document.high_water().value, 8);
    scalar(bytes, 32, document.gravity().x);
    scalar(bytes, 40, document.gravity().y);
    std::size_t offset = header_size;
    for (const auto &particle : document.particles()) {
        write(bytes, offset, particle.id.value, 8);
        scalar(bytes, offset + 8, particle.fields.mass);
        scalar(bytes, offset + 16, particle.fields.initial_position.x);
        scalar(bytes, offset + 24, particle.fields.initial_position.y);
        scalar(bytes, offset + 32, particle.fields.initial_velocity.x);
        scalar(bytes, offset + 40, particle.fields.initial_velocity.y);
        scalar(bytes, offset + 48, particle.fields.display_radius);
        offset += record_size;
    }
    return core::Result<std::vector<std::byte>>::success(std::move(bytes));
}
core::Result<scene::Document> decode(std::span<const std::byte> bytes) {
    using Result = core::Result<scene::Document>;
    if (bytes.size() > max_size || bytes.size() < header_size)
        return Result::failure(error(core::Code::invalid_data, "file.size"));
    if (!std::equal(magic.begin(), magic.end(), bytes.begin()))
        return Result::failure(error(core::Code::invalid_data, "file.magic"));
    if (read(bytes, 8, 4) != 1)
        return Result::failure(error(core::Code::unsupported_feature, "file.version"));
    if (read(bytes, 12, 4) != 1)
        return Result::failure(error(core::Code::unsupported_feature, "schema_revision"));
    if (read(bytes, 20, 4) != 0)
        return Result::failure(error(core::Code::unsupported_feature, "file.reserved"));
    const auto count = read(bytes, 16, 4);
    if (count > max_count)
        return Result::failure(error(core::Code::invalid_data, "file.count"));
    if (bytes.size() != header_size + record_size * count)
        return Result::failure(error(core::Code::invalid_data, "file.size"));
    std::vector<scene::Particle> particles;
    particles.reserve(static_cast<std::size_t>(count));
    for (std::size_t offset = header_size; offset < bytes.size(); offset += record_size)
        particles.push_back({{read(bytes, offset, 8)},
                             {scalar(bytes, offset + 8),
                              {scalar(bytes, offset + 16), scalar(bytes, offset + 24)},
                              {scalar(bytes, offset + 32), scalar(bytes, offset + 40)},
                              scalar(bytes, offset + 48)}});
    return scene::Document::create(1, {scalar(bytes, 32), scalar(bytes, 40)}, std::move(particles),
                                   {read(bytes, 24, 8)});
}
} // namespace opensim::serialization
