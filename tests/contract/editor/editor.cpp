#include "fixture.hpp"
#include <limits>
using namespace test_editor;
TEST_CASE("E01 E10 selection and empty history") {
    auto s = state();
    REQUIRE(s.document().particles().empty());
    REQUIRE_FALSE(s.selected());
    REQUIRE_FALSE(s.can_undo());
    REQUIRE_FALSE(s.can_redo());
    REQUIRE_FALSE(s.dragging());
    REQUIRE(s.undo().error()->path == "history");
    REQUIRE(s.redo().error()->path == "history");
    REQUIRE(s.select(core::EntityId{}).error()->code == core::Code::invalid_argument);
    REQUIRE(s.select(core::EntityId{7}).error()->code == core::Code::missing_reference);
    s = take(s.add(fields()));
    s = take(s.select({}));
    REQUIRE_FALSE(s.selected());
    s = take(s.select(core::EntityId{1}));
    REQUIRE(s.selected() == core::EntityId{1});
}
TEST_CASE("E11-E16 E30 history branches never rewind IDs") {
    auto base = state({}, {4});
    auto added = take(base.add(fields()));
    REQUIRE(added.selected() == core::EntityId{5});
    auto undone = take(added.undo());
    REQUIRE(undone.document().high_water() == core::EntityId{5});
    auto redo = take(undone.redo());
    REQUIRE(redo.selected() == core::EntityId{5});
    auto branch = take(undone.add(fields()));
    REQUIRE(branch.selected() == core::EntityId{6});
    REQUIRE_FALSE(branch.can_redo());
    auto deleted = take(redo.erase({5}));
    REQUIRE_FALSE(deleted.selected());
    auto restored = take(deleted.undo());
    REQUIRE(restored.selected() == core::EntityId{5});
    REQUIRE_FALSE(take(restored.redo()).selected());
    auto bad = fields();
    bad.mass = -1;
    REQUIRE(undone.add(bad).error());
    REQUIRE(undone.can_redo());
    auto noop = take(undone.set_gravity(undone.document().gravity()));
    REQUIRE(noop.can_redo());
    noop = take(noop.select({}));
    noop = take(noop.set_view(noop.view()));
    REQUIRE(noop.can_redo());
    auto original = state({{{1}, fields()}}, {2});
    auto changed = take(original.replace({1}, fields({1, 2})));
    auto back = take(changed.undo());
    REQUIRE(take(back.replace({1}, fields())).can_redo());
    REQUIRE(back.replace({1}, bad).error());
    REQUIRE(back.can_redo());
    const core::EntityId max{std::numeric_limits<std::uint64_t>::max()};
    auto full = state({{max, fields()}}, max);
    auto removed = take(full.erase(max));
    auto returned = take(removed.undo());
    REQUIRE(returned.document().high_water() == max);
    REQUIRE(returned.add(fields()).error()->code == core::Code::id_exhausted);
}
TEST_CASE("E17 history capacity drops oldest transition") {
    auto s = state();
    for (int i = 1; i <= 65; ++i)
        s = take(s.set_gravity({double(i), 0}));
    for (int i = 0; i < 64; ++i)
        s = take(s.undo());
    REQUIRE(s.document().gravity() == math::Vec2{1, 0});
    REQUIRE_FALSE(s.can_undo());
    for (int i = 0; i < 64; ++i)
        s = take(s.redo());
    REQUIRE(s.document().gravity() == math::Vec2{65, 0});
}
TEST_CASE("E18-E24 E26 drag preview transaction and ownership") {
    auto base = take(state({{{1}, fields({1, 2})}}).select(core::EntityId{1}));
    auto gesture = take(base.begin_drag({500, 100}));
    auto updated = take(gesture.update_drag({550, 125}));
    REQUIRE(updated.document() == base.document());
    REQUIRE(updated.display_document().particles()[0].fields.initial_position ==
            math::Vec2{1.5, 1.75});
    auto via = take(take(gesture.update_drag({600, 100})).update_drag({550, 125}));
    REQUIRE(via.display_document() == updated.display_document());
    REQUIRE_FALSE(updated.can_undo());
    auto cancelled = take(updated.cancel_drag());
    REQUIRE(cancelled.document() == base.document());
    REQUIRE_FALSE(cancelled.can_undo());
    auto committed = take(updated.commit_drag());
    REQUIRE(committed.can_undo());
    REQUIRE_FALSE(committed.dragging());
    REQUIRE(take(committed.undo()).document() == base.document());
    REQUIRE_FALSE(take(gesture.commit_drag()).can_undo());
    REQUIRE_FALSE(take(take(updated.update_drag({500, 100})).commit_drag()).can_undo());
    REQUIRE(updated.update_drag({std::numeric_limits<double>::quiet_NaN(), 0}).error()->path ==
            "pointer");
    REQUIRE(updated.update_drag({1048577, 0}).error());
    REQUIRE(updated.undo().error()->path == "gesture");
    REQUIRE(updated.redo().error()->path == "gesture");
    REQUIRE(updated.add(fields()).error()->path == "gesture");
    REQUIRE(updated.replace({1}, fields()).error()->path == "gesture");
    REQUIRE(updated.erase({1}).error()->path == "gesture");
    REQUIRE(updated.set_gravity({0, 0}).error()->path == "gesture");
    REQUIRE(updated.set_view(base.view()).error()->path == "gesture");
    REQUIRE(updated.select({}).error()->path == "gesture");
    REQUIRE(updated.begin_drag({0, 0}).error()->path == "gesture");
    REQUIRE(base.commit_drag().error());
    REQUIRE(base.cancel_drag().error());
    REQUIRE(base.update_drag({0, 0}).error());
    REQUIRE(state().begin_drag({0, 0}).error()->path == "selection");
    const auto retained = [&] {
        auto copy = updated;
        return copy;
    }();
    updated = cancelled;
    REQUIRE(retained.dragging());
    REQUIRE(retained.display_document() == via.display_document());
}
