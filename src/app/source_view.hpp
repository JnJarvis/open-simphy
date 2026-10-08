#pragma once
#include "sdl_host.hpp"
#include "text.hpp"
#include <opensim/compat/mechanical_source.hpp>
#include <opensim/physics/mechanism.hpp>
namespace opensim::app {
// SDL ownership remains private to application composition.
class SourceView {
    compat::MechanicalSource source_;
    std::shared_ptr<physics::Mechanism> world_;
    struct DecodedImage {
        int width, height;
        std::vector<std::uint8_t> pixels;
    };
    std::map<std::string, Texture> images_;
    std::map<std::string, DecodedImage> decoded_;
    std::unique_ptr<TextRenderer> text_;
    math::Vec2 camera_;
    double scale_, remainder_ = 0;
    bool running_ = false;
    float list_scroll_ = 0;
    std::optional<core::EntityId> selected_, dragging_;
    Uint8 pan_button_ = 0;
    math::Vec2 pan_pointer_{};
    std::string status_ =
        "Experimental simulation: collision sounds and some scripted events are not supported yet.";
    void replace_world();
    math::Vec2 local(math::Vec2, const Workspace &) const;
    void drag(math::Vec2);

  public:
    explicit SourceView(compat::MechanicalSource);
    bool event(const SDL_Event &, Host &);
    void zoom_at(double, math::Vec2, const Workspace &);
    void tick(double);
    void paint(SDL_Renderer *, renderer::Extent, float);
    scene::MechanismSnapshot snapshot() const { return world_->snapshot(); }
    const scene::Mechanism &definition() const { return source_.definition(); }
    void reset();
    math::Vec2 camera() const { return camera_; }
    double zoom() const { return scale_; }
    const std::vector<compat::SourceWidget> &widgets() const { return source_.widgets(); }
    bool running() const { return running_; }
};
} // namespace opensim::app
