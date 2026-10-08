#pragma once
#include <filesystem>
#include <fstream>
#include <opensim/compat/ssim.hpp>
namespace opensim::app {
inline std::filesystem::path utf8_path(std::string_view text) {
    std::u8string value;
    for (unsigned char c : text)
        value.push_back(static_cast<char8_t>(c));
    return std::filesystem::path(value);
}
inline core::Result<compat::Project> read_project(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    const auto fail = [](const char *message) {
        return core::Result<compat::Project>::failure(
            {core::Code::invalid_data, core::Severity::error, message, {}, "app.open"});
    };
    if (!input)
        return fail("Cannot open project file");
    const auto length = input.tellg();
    if (length <= 0 || length > static_cast<std::streamoff>(compat::max_ssim_bytes))
        return fail("Project file size limit");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char *>(bytes.data()), length))
        return fail("Could not read complete project");
    return compat::inspect_ssim(bytes);
}
} // namespace opensim::app
