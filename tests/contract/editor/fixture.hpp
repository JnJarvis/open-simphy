#pragma once
#include <catch2/catch_test_macros.hpp>
#include <opensim/editor/editor.hpp>
namespace test_editor {
using namespace opensim;
template <class T> T take(const core::Result<T> &r) {
    REQUIRE(r.has_value());
    return *r.value();
}
inline scene::ParticleFields fields(math::Vec2 p = {0, 0}) { return {1, p, {0, 0}, .1}; }
inline editor::State state(std::vector<scene::Particle> particles = {}, core::EntityId high = {}) {
    for (const auto &p : particles)
        high = std::max(high, p.id);
    return take(editor::State::create(take(scene::Document::create(1, {0, -9.8}, particles, high)),
                                      {{0, 0}, 100, 800, 600}));
}
} // namespace test_editor
