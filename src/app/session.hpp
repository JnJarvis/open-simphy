#pragma once
#include <opensim/physics/world.hpp>
#include <opensim/renderer/renderer.hpp>
#include <optional>
namespace opensim::app {
// Private composition policy, not a new domain interface.
class Session {
    physics::World world_;
    scene::DrawPacket overlays_;
    bool running_ = false;
    double accumulated_ = 0;
    std::optional<renderer::Frame> last_frame_;
    Session(physics::World world, scene::DrawPacket overlays)
        : world_(std::move(world)), overlays_(std::move(overlays)) {}

  public:
    static core::Result<Session> create();
    [[nodiscard]] scene::Snapshot snapshot() const { return world_.snapshot(); }
    [[nodiscard]] bool running() const { return running_; }
    [[nodiscard]] const std::optional<renderer::Frame> &last_frame() const { return last_frame_; }
    void set_running(bool value) {
        running_ = value;
        accumulated_ = 0;
    }
    [[nodiscard]] core::Result<void> advance();
    [[nodiscard]] core::Result<void> reset();
    [[nodiscard]] core::Result<void> tick(double elapsed);
    template <class Render, class Present>
    core::Result<void> draw(renderer::Extent extent, Render &&render, Present &&present) {
        if (extent.width == 0 || extent.height == 0)
            return core::Result<void>::success();
        const auto frame =
            render(world_.snapshot(), overlays_, renderer::Camera{{0, 0}, 80}, extent);
        if (frame.error()) {
            set_running(false);
            return core::Result<void>::failure(*frame.error());
        }
        const auto shown = present(*frame.value());
        if (shown.error()) {
            set_running(false);
            return core::Result<void>::failure(*shown.error());
        }
        last_frame_.emplace(*frame.value());
        return core::Result<void>::success();
    }
};
} // namespace opensim::app
