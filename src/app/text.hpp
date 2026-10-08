#pragma once
#include <SDL3/SDL.h>
#include <memory>
#include <string_view>
namespace opensim::app {
// Owns display-resolution glyph textures; lifetime ends before the SDL renderer.
class TextRenderer {
    struct Impl;
    std::unique_ptr<Impl> impl_;

  public:
    explicit TextRenderer(SDL_Renderer *);
    ~TextRenderer();
    void draw(float x, float y, std::string_view text, float width);
};
} // namespace opensim::app
