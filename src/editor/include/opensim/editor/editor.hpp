#pragma once
#include <opensim/scene/scene.hpp>
namespace opensim::editor {
struct View {
    math::Vec2 center;
    double pixels_per_meter;
    std::uint32_t width, height;
    bool operator==(const View &) const = default;
};
class State {
    struct Entry {
        scene::Document document;
        std::optional<core::EntityId> selection;
    };
    struct Transition {
        Entry before, after;
    };
    struct Drag {
        math::Vec2 start;
        scene::ParticleFields fields;
        scene::Document preview;
    };
    scene::Document document_;
    View view_;
    std::optional<core::EntityId> selected_;
    std::vector<Transition> undo_, redo_;
    std::optional<Drag> drag_;
    State(scene::Document document, View view) : document_(std::move(document)), view_(view) {}
    core::Result<State> finish(const scene::Edit &, std::optional<core::EntityId>) const;
    core::Result<State> travel(bool forward) const;
    core::Diagnostic gesture_error() const;

  public:
    static core::Result<State> create(const scene::Document &, View);
    const scene::Document &document() const { return document_; }
    const scene::Document &display_document() const { return drag_ ? drag_->preview : document_; }
    std::optional<core::EntityId> selected() const { return selected_; }
    View view() const { return view_; }
    bool dragging() const { return drag_.has_value(); }
    bool can_undo() const { return !undo_.empty(); }
    bool can_redo() const { return !redo_.empty(); }
    core::Result<State> select(std::optional<core::EntityId>) const;
    core::Result<State> set_view(View) const;
    core::Result<math::Vec2> world_at(math::Vec2) const;
    core::Result<std::optional<core::EntityId>> hit_test(math::Vec2) const;
    core::Result<State> add(scene::ParticleFields) const;
    core::Result<State> replace(core::EntityId, scene::ParticleFields) const;
    core::Result<State> erase(core::EntityId) const;
    core::Result<State> set_gravity(math::Vec2) const;
    core::Result<State> undo() const { return travel(false); }
    core::Result<State> redo() const { return travel(true); }
    core::Result<State> begin_drag(math::Vec2) const;
    core::Result<State> update_drag(math::Vec2) const;
    core::Result<State> commit_drag() const;
    core::Result<State> cancel_drag() const;
};
} // namespace opensim::editor
