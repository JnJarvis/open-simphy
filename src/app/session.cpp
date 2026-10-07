#include "session.hpp"
#include <algorithm>
#include <cmath>
namespace opensim::app {
core::Result<math::Vec2> physical_pointer(math::Vec2 p, math::Vec2 window,
                                          renderer::Extent output) {
    const auto invalid = [] {
        return core::Result<math::Vec2>::failure({core::Code::invalid_argument,
                                                  core::Severity::error,
                                                  "Invalid pointer/output dimensions",
                                                  {},
                                                  "app.pointer"});
    };
    if (!math::finite(p) || !math::finite(window) || window.x <= 0 || window.y <= 0 ||
        !output.width || !output.height)
        return invalid();
    const math::Vec2 mapped{p.x * (double(output.width) / window.x),
                            p.y * (double(output.height) / window.y)};
    if (!math::finite(mapped))
        return invalid();
    return core::Result<math::Vec2>::success(mapped);
}
core::Result<Session> Session::create() {
    const auto doc = scene::Document::create(1, {0, -9.8125},
                                             {{{1}, {1, {-3, 1}, {2, 5}, .18}},
                                              {{2}, {2, {0, 2}, {.5, 2}, .25}},
                                              {{3}, {.5, {3, 1}, {-1, 4}, .14}}},
                                             {3});
    if (doc.error())
        return core::Result<Session>::failure(*doc.error());
    const auto state = editor::State::create(*doc.value(), {{0, 0}, 80, 1200, 760});
    if (state.error())
        return core::Result<Session>::failure(*state.error());
    const auto world = physics::World::create(*doc.value(), 1.0 / 128);
    if (world.error())
        return core::Result<Session>::failure(*world.error());
    return core::Result<Session>::success(Session(*world.value(), *state.value()));
}
scene::Snapshot Session::snapshot() const {
    if (authoring_)
        return *scene::Snapshot::initial(editor_.display_document()).value();
    return world_.snapshot();
}
core::Result<void> Session::apply(const core::Result<editor::State> &candidate) {
    return apply_with(candidate, physics::World::create);
}
core::Result<void> Session::cancel_drag() {
    if (!editor_.dragging())
        return core::Result<void>::success();
    const auto next = editor_.cancel_drag();
    if (next.error())
        return core::Result<void>::failure(*next.error());
    auto published = *next.value();
    editor_ = std::move(published);
    return core::Result<void>::success();
}
core::Result<void> Session::navigate(editor::View view) {
    auto candidate = editor_;
    if (candidate.dragging()) {
        const auto cancelled = candidate.cancel_drag();
        if (cancelled.error())
            return core::Result<void>::failure(*cancelled.error());
        candidate = *cancelled.value();
    }
    const auto next = candidate.set_view(view);
    if (next.error())
        return core::Result<void>::failure(*next.error());
    auto published = *next.value();
    editor_ = std::move(published);
    return core::Result<void>::success();
}
core::Result<void> Session::resize(renderer::Extent size) {
    if (!size.width || !size.height)
        return cancel_drag();
    auto view = editor_.view();
    if (view.width == size.width && view.height == size.height)
        return core::Result<void>::success();
    view.width = size.width;
    view.height = size.height;
    return navigate(view);
}
core::Result<void> Session::set_running(bool value) {
    if (value && authoring_) {
        auto candidate = editor_;
        if (candidate.dragging()) {
            const auto c = candidate.cancel_drag();
            if (c.error())
                return core::Result<void>::failure(*c.error());
            candidate = *c.value();
        }
        const auto prepared = physics::World::create(candidate.document(), 1.0 / 128);
        if (prepared.error())
            return core::Result<void>::failure(*prepared.error());
        auto next = *prepared.value();
        editor_ = std::move(candidate);
        world_ = std::move(next);
        authoring_ = false;
    }
    running_ = value;
    accumulated_ = 0;
    return core::Result<void>::success();
}
core::Result<void> Session::advance() {
    if (authoring_) {
        const auto start = set_running(true);
        if (start.error())
            return start;
        running_ = false;
    }
    const auto next = world_.step();
    if (next.error()) {
        running_ = false;
        return core::Result<void>::failure(*next.error());
    }
    world_ = *next.value();
    return core::Result<void>::success();
}
core::Result<void> Session::reset() {
    auto candidate = editor_;
    if (candidate.dragging()) {
        const auto c = candidate.cancel_drag();
        if (c.error())
            return core::Result<void>::failure(*c.error());
        candidate = *c.value();
    }
    const auto next = physics::World::create(candidate.document(), 1.0 / 128);
    if (next.error())
        return core::Result<void>::failure(*next.error());
    auto world = *next.value();
    editor_ = std::move(candidate);
    world_ = std::move(world);
    authoring_ = true;
    running_ = false;
    accumulated_ = 0;
    return core::Result<void>::success();
}
core::Result<void> Session::tick(double elapsed) {
    if (!std::isfinite(elapsed) || elapsed < 0)
        return core::Result<void>::failure({core::Code::invalid_argument,
                                            core::Severity::error,
                                            "Elapsed time must be finite and nonnegative",
                                            {},
                                            "elapsed"});
    if (!running_)
        return core::Result<void>::success();
    accumulated_ += std::min(elapsed, .25);
    unsigned count = 0;
    while (accumulated_ >= world_.fixed_dt() && count < 32) {
        const auto r = advance();
        if (r.error())
            return r;
        accumulated_ -= world_.fixed_dt();
        ++count;
    }
    return core::Result<void>::success();
}
core::Result<scene::DrawPacket> Session::overlays() const {
    const double width = 1 / editor_.view().pixels_per_meter;
    std::vector<scene::Primitive> primitives{
        scene::Line{{-5, 0}, {5, 0}, width, {.25, .35, .45, 1}},
        scene::Line{{0, -3}, {0, 3}, width, {.25, .35, .45, 1}}};
    if (authoring_ && editor_.selected())
        for (const auto &p : editor_.display_document().particles())
            if (p.id == editor_.selected()) {
                const auto c = p.fields.initial_position;
                const auto r = p.fields.display_radius + 4 * width;
                const math::Vec2 a{c.x - r, c.y - r}, b{c.x + r, c.y - r}, d{c.x - r, c.y + r},
                    e{c.x + r, c.y + r};
                const scene::Color color{1, .75, .2, 1};
                for (auto line : std::vector<scene::Line>{{a, b, width, color},
                                                          {b, e, width, color},
                                                          {e, d, width, color},
                                                          {d, a, width, color}})
                    primitives.push_back(line);
            }
    return scene::DrawPacket::create(std::move(primitives));
}
} // namespace opensim::app
