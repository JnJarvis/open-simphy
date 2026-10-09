#pragma once
#include <memory>
#include <opensim/compat/ssim.hpp>
#include <opensim/scene/mechanism.hpp>
#include <opensim/scene/scene.hpp>
namespace opensim::compat {
struct BodyStyle {
    scene::Color fill{1, 1, 1, 1}, outline{224.0 / 255, 224.0 / 255, 224.0 / 255, 1};
    std::string image;
};
struct SourceWidget {
    std::string text, action;
    math::Vec2 position{}, size{150, 32};
    bool button = false, slider = false;
    bool visible = true, enabled = true;
    std::string name;
    double minimum = 0, maximum = 1, value = 0;
};
struct SourceControls {
    std::optional<double> time;
    std::vector<scene::BodyUpdate> updates;
    std::vector<scene::AppliedForce> forces;
    std::map<core::EntityId, double> friction;
};
class MechanicalSource {
    struct Impl;
    std::shared_ptr<Impl> impl_;
    explicit MechanicalSource(std::shared_ptr<Impl>);

  public:
    static core::Result<MechanicalSource> create(const Project &);
    const scene::Mechanism &definition() const;
    const std::map<core::EntityId, BodyStyle> &styles() const;
    const std::map<core::EntityId, scene::Color> &joint_colors() const;
    const std::map<std::string, std::vector<std::uint8_t>> &images() const;
    const std::vector<SourceWidget> &widgets() const;
    math::Vec2 camera_center() const;
    double camera_scale() const;
    core::Result<void> apply_action(std::string_view);
    core::Result<void> set_slider(std::size_t, double);
    core::Result<SourceControls> controls(const scene::MechanismSnapshot &);
};
} // namespace opensim::compat
