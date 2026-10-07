#include "session.hpp"
#include <catch2/catch_test_macros.hpp>
#include <limits>
using namespace opensim;
TEST_CASE("E03 DPI physical mapping at 100 125 150 200 percent and asymmetric ratios") {
    for (const auto size : std::vector<renderer::Extent>{
             {800, 600}, {1000, 750}, {1200, 900}, {1600, 1200}, {1600, 900}}) {
        const auto p = app::physical_pointer({250, 50}, {800, 600}, size);
        REQUIRE(p.has_value());
        REQUIRE(p.value()->x == 250.0 * size.width / 800);
        REQUIRE(p.value()->y == 50.0 * size.height / 600);
        const auto d = scene::Document::create(1, {0, 0}, {}, {});
        REQUIRE(d.has_value());
        const auto editor =
            editor::State::create(*d.value(), {{0, 0}, 100, size.width, size.height});
        REQUIRE(editor.has_value());
        const auto world = editor.value()->world_at(*p.value());
        REQUIRE(world.has_value());
        REQUIRE(world.value()->x == (p.value()->x - size.width / 2.0) / 100);
    }
    const auto e03 = app::physical_pointer({250, 50}, {400, 300}, {800, 600});
    REQUIRE(e03.has_value());
    REQUIRE(*e03.value() == math::Vec2{500, 100});
    REQUIRE(app::physical_pointer({1, 1}, {0, 300}, {800, 600}).error());
    REQUIRE(app::physical_pointer({1, 1}, {400, 300}, {0, 0}).error());
    REQUIRE(app::physical_pointer({std::numeric_limits<double>::max(), 1}, {1, 1}, {4096, 4096})
                .error());
}
TEST_CASE("E27-E29 authoring mode rollback and resize cancellation") {
    auto created = app::Session::create();
    REQUIRE(created.has_value());
    auto s = *created.value();
    REQUIRE(s.apply(s.editing().select(core::EntityId{1})).has_value());
    const auto before = s.editing().document();
    const auto world = s.snapshot();
    const auto candidate = s.editing().add({1, {0, 0}, {0, 0}, .2});
    REQUIRE(candidate.has_value());
    const auto failed = s.apply_with(candidate, [](const scene::Document &, double) {
        return core::Result<physics::World>::failure({core::Code::internal_error,
                                                      core::Severity::error,
                                                      "Injected construction failure",
                                                      {},
                                                      "test"});
    });
    REQUIRE(failed.error());
    REQUIRE(s.editing().document() == before);
    REQUIRE(s.snapshot() == world);
    REQUIRE_FALSE(s.editing().can_undo());
    REQUIRE(s.apply(s.editing().begin_drag({360, 300})).has_value());
    REQUIRE(s.apply(s.editing().update_drag({400, 320})).has_value());
    REQUIRE(s.resize({1500, 1000}).has_value());
    REQUIRE_FALSE(s.editing().dragging());
    REQUIRE(s.editing().document() == before);
    REQUIRE(s.set_running(true).has_value());
    REQUIRE(s.tick(1.0 / 128).has_value());
    REQUIRE(s.set_running(false).has_value());
    const auto stepped = s.snapshot();
    REQUIRE(s.apply(candidate).error()->path == "app.mode");
    REQUIRE(s.snapshot() == stepped);
    REQUIRE(s.reset().has_value());
    REQUIRE(s.authoring());
    REQUIRE_FALSE(s.running());
    REQUIRE(s.apply(candidate).has_value());
    REQUIRE(s.snapshot().time() == 0);
    REQUIRE(s.snapshot().particles().size() == 4);
}
