#pragma once
#include <opensim/editor/editor.hpp>
#include <opensim/physics/world.hpp>
#include <opensim/renderer/renderer.hpp>
#include <optional>
namespace opensim::app {
core::Result<math::Vec2> physical_pointer(math::Vec2 pointer, math::Vec2 window,
                                          renderer::Extent output);
class Session {
    physics::World world_;
    editor::State editor_;
    bool authoring_ = true, running_ = false;
    double accumulated_ = 0;
    std::optional<renderer::Frame> last_frame_;
    Session(physics::World world, editor::State state)
        : world_(std::move(world)), editor_(std::move(state)) {}

  public:
    static core::Result<Session> create();
    scene::Snapshot snapshot() const;
    bool running() const { return running_; }
    bool authoring() const { return authoring_; }
    const editor::State &editing() const { return editor_; }
    const std::optional<renderer::Frame> &last_frame() const { return last_frame_; }
    core::Result<void> set_running(bool);
    core::Result<void> advance();
    core::Result<void> reset();
    core::Result<void> tick(double);
    core::Result<void> resize(renderer::Extent);
    core::Result<void> cancel_drag();
    core::Result<void> navigate(editor::View);
    core::Result<void> apply(const core::Result<editor::State> &);
    template <class Factory>
    core::Result<void> apply_with(const core::Result<editor::State> &candidate, Factory factory) {
        if (!authoring_)
            return core::Result<void>::failure({core::Code::invalid_argument,
                                                core::Severity::error,
                                                "Reset to edit the initial scene",
                                                {},
                                                "app.mode"});
        if (candidate.error())
            return core::Result<void>::failure(*candidate.error());
        if (candidate.value()->document() != editor_.document()) {
            const auto prepared = factory(candidate.value()->document(), 1.0 / 128);
            if (prepared.error())
                return core::Result<void>::failure(*prepared.error());
            // Allocate/copy before publishing either half of the transaction.
            auto next_editor = *candidate.value();
            auto next_world = *prepared.value();
            editor_ = std::move(next_editor);
            world_ = std::move(next_world);
        } else {
            auto next_editor = *candidate.value();
            editor_ = std::move(next_editor);
        }
        return core::Result<void>::success();
    }
    template <class Render, class Present>
    core::Result<void> draw(renderer::Extent extent, Render &&render, Present &&present) {
        if (!extent.width || !extent.height)
            return core::Result<void>::success();
        const auto sized = resize(extent);
        if (sized.error())
            return sized;
        const auto view = editor_.view();
        const auto overlay = overlays();
        if (overlay.error())
            return core::Result<void>::failure(*overlay.error());
        const auto frame = render(snapshot(), *overlay.value(),
                                  renderer::Camera{view.center, view.pixels_per_meter}, extent);
        if (frame.error()) {
            running_ = false;
            accumulated_ = 0;
            return core::Result<void>::failure(*frame.error());
        }
        const auto shown = present(*frame.value());
        if (shown.error()) {
            running_ = false;
            accumulated_ = 0;
            return core::Result<void>::failure(*shown.error());
        }
        auto published = *frame.value();
        last_frame_.emplace(std::move(published));
        return core::Result<void>::success();
    }

  private:
    core::Result<scene::DrawPacket> overlays() const;
};
} // namespace opensim::app
