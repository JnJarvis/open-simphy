#pragma once
#include "sdl_host.hpp"
#include "session.hpp"
#include "source_project.hpp"
#include "text.hpp"
namespace opensim::app {
class EditorUI {
    mutable std::unique_ptr<TextRenderer> text_;
    void text(SDL_Renderer *, float, float, const std::string &, float width = 100000) const;
    std::optional<compat::Project> source_;
    float source_scroll_ = 0;
    void paint_source(SDL_Renderer *, renderer::Extent, float) const;
    int field_ = -1;
    float panel_scroll_ = 0, scene_scroll_ = 0;
    bool help_ = false;
    std::string buffer_, message_ = "Click a particle to select; drag to move. Scroll to zoom.";
    bool replace_text_ = true;
    math::Vec2 last_pointer_{};
    void report(const core::Result<void> &);
    void edit_field(int, Session &, Host &);
    void end_text(Host &);
    void action(int, Session &, Host &);

  public:
    bool open_file(const std::string &, Session &);
    void poll_open(Session &, Host &);
    std::string source_title() const { return source_ ? source_->title : std::string{}; }
    bool source_open() const { return source_.has_value(); }
    bool event(const SDL_Event &, Session &, Host &);
    void paint(SDL_Renderer *, const Session &, renderer::Extent, float scale) const;
    const std::string &message() const { return message_; }
};
} // namespace opensim::app
