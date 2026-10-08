#pragma once
#include "workspace.hpp"
#include <SDL3/SDL.h>
#include <functional>
#include <memory>
#include <mutex>
#include <opensim/renderer/renderer.hpp>
#include <optional>
#include <string>
namespace opensim::app {
struct QuitSDL {
    ~QuitSDL() { SDL_Quit(); }
};
using Window = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
using Device = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;
using Texture = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;
struct OpenRequest {
    std::mutex mutex;
    bool pending = false;
    std::optional<std::string> path;
};
class Host {
    std::shared_ptr<OpenRequest> open_ = std::make_shared<OpenRequest>();
    QuitSDL quit_;
    Window window_{nullptr, SDL_DestroyWindow};
    Device device_{nullptr, SDL_DestroyRenderer};
    Texture texture_{nullptr, SDL_DestroyTexture};
    renderer::Extent texture_extent_{};

  public:
    explicit Host(bool fail_window = false);
    [[nodiscard]] SDL_Window *window() const { return window_.get(); }
    [[nodiscard]] renderer::Extent extent() const;
    renderer::Extent canvas_extent() const;
    Workspace workspace() const;
    core::Result<void> present(const renderer::Frame &, const char *capture = nullptr,
                               bool fail_texture = false,
                               const std::function<void(SDL_Renderer *)> &paint = {},
                               renderer::Camera camera = {{0, 0}, 80});
    void request_open();
    std::optional<std::string> take_open();
    void title(const std::string &text);
};
void require_sdl(bool ok, const char *operation);
} // namespace opensim::app
