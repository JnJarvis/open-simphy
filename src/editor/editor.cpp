#include <algorithm>
#include <cmath>
#include <opensim/editor/editor.hpp>
namespace opensim::editor {
namespace {
using core::Code;
core::Diagnostic error(Code code, const char *path, std::optional<core::EntityId> id = {}) {
    return {code, core::Severity::error, std::string("Invalid editor operation: ") + path, id,
            path};
}
std::optional<core::Diagnostic> validate(View v) {
    if (!v.width || !v.height || v.width > 4096 || v.height > 4096 ||
        std::uint64_t(v.width) * v.height > 4194304)
        return error(Code::invalid_argument, "view.extent");
    if (!math::finite(v.center) || !std::isfinite(v.pixels_per_meter) ||
        v.pixels_per_meter < 1.0 / 1024 || v.pixels_per_meter > 4096)
        return error(Code::invalid_argument, "view.camera");
    return {};
}
bool pointer_valid(math::Vec2 p) {
    return math::finite(p) && std::abs(p.x) <= 1048576 && std::abs(p.y) <= 1048576;
}
const scene::Particle *find(const scene::Document &doc, core::EntityId id) {
    for (const auto &p : doc.particles())
        if (p.id == id)
            return &p;
    return nullptr;
}
} // namespace
core::Diagnostic State::gesture_error() const {
    return error(Code::invalid_argument, "gesture", selected_);
}
core::Result<State> State::create(const scene::Document &doc, View view) {
    if (doc.particles().size() > 4096)
        return core::Result<State>::failure(error(Code::invalid_argument, "editor.primitives"));
    if (const auto e = validate(view))
        return core::Result<State>::failure(*e);
    return core::Result<State>::success(State(doc, view));
}
core::Result<State> State::select(std::optional<core::EntityId> id) const {
    if (drag_)
        return core::Result<State>::failure(gesture_error());
    if (id && !id->valid())
        return core::Result<State>::failure(error(Code::invalid_argument, "selection", id));
    if (id && !find(document_, *id))
        return core::Result<State>::failure(error(Code::missing_reference, "selection", id));
    auto next = *this;
    next.selected_ = id;
    return core::Result<State>::success(std::move(next));
}
core::Result<State> State::set_view(View view) const {
    if (drag_)
        return core::Result<State>::failure(gesture_error());
    if (const auto e = validate(view))
        return core::Result<State>::failure(*e);
    auto next = *this;
    next.view_ = view;
    return core::Result<State>::success(std::move(next));
}
core::Result<math::Vec2> State::world_at(math::Vec2 p) const {
    if (!pointer_valid(p))
        return core::Result<math::Vec2>::failure(error(Code::invalid_argument, "pointer"));
    const math::Vec2 offset{(p.x - double(view_.width) / 2) / view_.pixels_per_meter,
                            (double(view_.height) / 2 - p.y) / view_.pixels_per_meter};
    const auto world = view_.center + offset;
    if (!math::finite(offset) || !math::finite(world))
        return core::Result<math::Vec2>::failure(error(Code::invalid_data, "pointer.geometry"));
    return core::Result<math::Vec2>::success(world);
}
core::Result<std::optional<core::EntityId>> State::hit_test(math::Vec2 pointer) const {
    using Result = core::Result<std::optional<core::EntityId>>;
    if (!pointer_valid(pointer))
        return Result::failure(error(Code::invalid_argument, "pointer"));
    if (pointer.x < 0 || pointer.y < 0 || pointer.x >= view_.width || pointer.y >= view_.height)
        return Result::success({});
    std::optional<core::EntityId> hit;
    for (const auto &p : document_.particles()) {
        const auto delta = p.fields.initial_position - view_.center;
        const auto scaled = delta * view_.pixels_per_meter;
        const math::Vec2 center{double(view_.width) / 2 + scaled.x,
                                double(view_.height) / 2 - scaled.y};
        const auto radius = p.fields.display_radius * view_.pixels_per_meter;
        if (!math::finite(delta) || !math::finite(scaled) || !math::finite(center) ||
            std::abs(center.x) > 1048576 || std::abs(center.y) > 1048576 ||
            !std::isfinite(radius) || radius <= 0 || radius > 1048576)
            return Result::failure(error(Code::invalid_data, "picking.geometry", p.id));
        if (std::hypot(pointer.x - center.x, pointer.y - center.y) <= std::max(radius, 6.0))
            hit = p.id;
    }
    return Result::success(hit);
}
core::Result<State> State::finish(const scene::Edit &edit,
                                  std::optional<core::EntityId> selection) const {
    if (edit.document == document_)
        return core::Result<State>::success(*this);
    auto next = *this;
    next.undo_.push_back({{document_, selected_}, {edit.document, selection}});
    if (next.undo_.size() > 64)
        next.undo_.erase(next.undo_.begin());
    next.redo_.clear();
    next.document_ = edit.document;
    next.selected_ = selection;
    return core::Result<State>::success(std::move(next));
}
core::Result<State> State::add(scene::ParticleFields fields) const {
    if (drag_)
        return core::Result<State>::failure(gesture_error());
    if (document_.particles().size() >= 4096)
        return core::Result<State>::failure(error(Code::invalid_argument, "editor.primitives"));
    const auto edit = scene::add_particle(document_, fields);
    if (edit.error())
        return core::Result<State>::failure(*edit.error());
    return finish(*edit.value(), edit.value()->affected_id);
}
core::Result<State> State::replace(core::EntityId id, scene::ParticleFields fields) const {
    if (drag_)
        return core::Result<State>::failure(gesture_error());
    const auto edit = scene::replace_particle(document_, id, fields);
    if (edit.error())
        return core::Result<State>::failure(*edit.error());
    return finish(*edit.value(), selected_);
}
core::Result<State> State::erase(core::EntityId id) const {
    if (drag_)
        return core::Result<State>::failure(gesture_error());
    const auto edit = scene::remove_particle(document_, id);
    if (edit.error())
        return core::Result<State>::failure(*edit.error());
    return finish(*edit.value(), selected_ == id ? std::nullopt : selected_);
}
core::Result<State> State::set_gravity(math::Vec2 g) const {
    if (drag_)
        return core::Result<State>::failure(gesture_error());
    const auto edit = scene::set_gravity(document_, g);
    if (edit.error())
        return core::Result<State>::failure(*edit.error());
    return finish(*edit.value(), selected_);
}
core::Result<State> State::travel(bool forward) const {
    if (drag_)
        return core::Result<State>::failure(gesture_error());
    const auto &source = forward ? redo_ : undo_;
    if (source.empty())
        return core::Result<State>::failure(error(Code::invalid_argument, "history"));
    const auto &entry = forward ? source.back().after : source.back().before;
    const auto &d = entry.document;
    const auto restored = scene::Document::create(d.revision(), d.gravity(),
                                                  {d.particles().begin(), d.particles().end()},
                                                  std::max(document_.high_water(), d.high_water()));
    if (restored.error())
        return core::Result<State>::failure(*restored.error());
    if (entry.selection && !find(*restored.value(), *entry.selection))
        return core::Result<State>::failure(
            error(Code::internal_error, "history", entry.selection));
    auto next = *this;
    if (forward) {
        next.undo_.push_back(next.redo_.back());
        next.redo_.pop_back();
    } else {
        next.redo_.push_back(next.undo_.back());
        next.undo_.pop_back();
    }
    next.document_ = *restored.value();
    next.selected_ = entry.selection;
    return core::Result<State>::success(std::move(next));
}
core::Result<State> State::begin_drag(math::Vec2 pointer) const {
    if (drag_)
        return core::Result<State>::failure(gesture_error());
    if (!selected_)
        return core::Result<State>::failure(error(Code::invalid_argument, "selection"));
    const auto world = world_at(pointer);
    if (world.error())
        return core::Result<State>::failure(*world.error());
    auto next = *this;
    next.drag_.emplace(Drag{*world.value(), find(document_, *selected_)->fields, document_});
    return core::Result<State>::success(std::move(next));
}
core::Result<State> State::update_drag(math::Vec2 pointer) const {
    if (!drag_)
        return core::Result<State>::failure(gesture_error());
    const auto world = world_at(pointer);
    if (world.error()) {
        auto e = *world.error();
        if (e.path == "pointer.geometry")
            e.entity = selected_;
        return core::Result<State>::failure(std::move(e));
    }
    const auto delta = *world.value() - drag_->start;
    auto fields = drag_->fields;
    fields.initial_position = fields.initial_position + delta;
    if (!math::finite(delta) || !math::finite(fields.initial_position))
        return core::Result<State>::failure(
            error(Code::invalid_data, "pointer.geometry", selected_));
    const auto edit = scene::replace_particle(document_, *selected_, fields);
    if (edit.error())
        return core::Result<State>::failure(*edit.error());
    auto next = *this;
    next.drag_->preview = edit.value()->document;
    return core::Result<State>::success(std::move(next));
}
core::Result<State> State::commit_drag() const {
    if (!drag_)
        return core::Result<State>::failure(gesture_error());
    auto next = *this;
    next.drag_.reset();
    return next.finish({drag_->preview, selected_}, selected_);
}
core::Result<State> State::cancel_drag() const {
    if (!drag_)
        return core::Result<State>::failure(gesture_error());
    auto next = *this;
    next.drag_.reset();
    return core::Result<State>::success(std::move(next));
}
} // namespace opensim::editor
