#pragma once
#include "sdl_host.hpp"
#include "session.hpp"
namespace opensim::app {
class EditorUI {
    int field_ = -1;
    float panel_scroll_ = 0;
    std::string buffer_, message_ = "Click a particle to select; drag to move. Scroll to zoom.";
    bool replace_text_ = true;
    math::Vec2 last_pointer_{};
    void report(const core::Result<void> &);
    void edit_field(int, Session &, Host &);
    void end_text(Host &);

  public:
    bool event(const SDL_Event &, Session &, Host &);
    void paint(SDL_Renderer *, const Session &, renderer::Extent, float scale) const;
    const std::string &message() const { return message_; }
};
} // namespace opensim::app
