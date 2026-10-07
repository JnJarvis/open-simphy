#include "sdl_host.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace opensim::app {
namespace {
std::string message(const char *operation) {
    return std::string(operation) + ": " + SDL_GetError();
}
core::Result<void> failure(const char *operation) {
    return core::Result<void>::failure(
        {core::Code::internal_error, core::Severity::error, message(operation), {}, "host"});
}
} // namespace
void require_sdl(bool ok, const char *operation) {
    if (!ok)
        throw std::runtime_error(message(operation));
}
Host::Host(bool fail_window) {
    require_sdl(SDL_Init(SDL_INIT_VIDEO), "Initialize video");
    if (fail_window)
        throw std::runtime_error("Create window: injected failure");
    window_.reset(SDL_CreateWindow("Open Simphy", 1200, 760,
                                   SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY));
    require_sdl(bool(window_), "Create window");
    require_sdl(SDL_SetWindowMinimumSize(window_.get(), 800, 680), "Set minimum editor size");
    device_.reset(SDL_CreateRenderer(window_.get(), nullptr));
    require_sdl(bool(device_), "Create renderer");
    require_sdl(
        SDL_SetRenderLogicalPresentation(device_.get(), 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED),
        "Disable logical scaling");
}
renderer::Extent Host::extent() const {
    if (SDL_GetWindowFlags(window_.get()) & SDL_WINDOW_MINIMIZED)
        return {0, 0};
    int w = 0, h = 0;
    require_sdl(SDL_GetRenderOutputSize(device_.get(), &w, &h), "Query output size");
    if (w <= 0 || h <= 0)
        return {0, 0};
    return {static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h)};
}
renderer::Extent Host::canvas_extent() const {
    const auto size = extent();
    if (!size.width || !size.height)
        return {0, 0};
    const float scale = std::max(1.0f, SDL_GetWindowDisplayScale(window_.get()));
    const auto panel = static_cast<std::uint32_t>(std::ceil(280 * scale));
    return {size.width > panel ? size.width - panel : 0, size.height};
}
void Host::title(const std::string &text) {
    require_sdl(SDL_SetWindowTitle(window_.get(), text.c_str()), "Set title");
}
core::Result<void> Host::present(const renderer::Frame &frame, const char *capture,
                                 bool fail_texture,
                                 const std::function<void(SDL_Renderer *)> &paint) {
    const renderer::Extent size{frame.width(), frame.height()};
    if (fail_texture)
        return core::Result<void>::failure({core::Code::internal_error,
                                            core::Severity::error,
                                            "Create texture: injected failure",
                                            {},
                                            "host"});
    if (!texture_ || texture_extent_ != size) {
        Texture next(SDL_CreateTexture(device_.get(), SDL_PIXELFORMAT_RGBA32,
                                       SDL_TEXTUREACCESS_STREAMING, static_cast<int>(size.width),
                                       static_cast<int>(size.height)),
                     SDL_DestroyTexture);
        if (!next)
            return failure("Create texture");
        if (!SDL_SetTextureBlendMode(next.get(), SDL_BLENDMODE_NONE))
            return failure("Disable blending");
        texture_ = std::move(next);
        texture_extent_ = size;
    }
    void *pixels = nullptr;
    int pitch = 0;
    if (!SDL_LockTexture(texture_.get(), nullptr, &pixels, &pitch))
        return failure("Lock texture");
    if (!pixels || pitch < 0 || static_cast<unsigned>(pitch) < frame.stride()) {
        SDL_UnlockTexture(texture_.get());
        return core::Result<void>::failure({core::Code::internal_error,
                                            core::Severity::error,
                                            "Invalid texture pitch",
                                            {},
                                            "host"});
    }
    for (std::uint32_t y = 0; y < frame.height(); ++y)
        std::memcpy(static_cast<std::uint8_t *>(pixels) + std::size_t(y) * pitch,
                    frame.bytes().data() + std::size_t(y) * frame.stride(), frame.stride());
    SDL_UnlockTexture(texture_.get());
    const SDL_FRect destination{0, 0, static_cast<float>(frame.width()),
                                static_cast<float>(frame.height())};
    if (!SDL_SetRenderDrawColor(device_.get(), 16, 20, 28, 255) ||
        !SDL_RenderClear(device_.get()) ||
        !SDL_RenderTexture(device_.get(), texture_.get(), nullptr, &destination))
        return failure("Copy frame");
    if (paint)
        paint(device_.get());
    if (capture) {
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(
            SDL_RenderReadPixels(device_.get(), nullptr), SDL_DestroySurface);
        if (!surface || !SDL_SaveBMP(surface.get(), capture))
            return failure("Capture frame");
    }
    if (!SDL_RenderPresent(device_.get()))
        return failure("Present frame");
    return core::Result<void>::success();
}
} // namespace opensim::app
