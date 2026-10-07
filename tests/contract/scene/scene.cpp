#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <opensim/scene/scene.hpp>
using namespace opensim::scene;
using opensim::core::Code;
namespace {
ParticleFields fields() { return {1, {1, 2}, {3, 4}, 0.5}; }
Document example() {
    return *Document::create(1, {0, -9.8}, {{{9}, fields()}, {{2}, fields()}}, {9}).value();
}
const double not_a_number = std::numeric_limits<double>::quiet_NaN(),
             inf = std::numeric_limits<double>::infinity();
} // namespace
TEST_CASE("S01-S08 authored validation and diagnostics") {
    REQUIRE(Document::create(1, {0, -9.8}, {}, {0}).has_value());
    const auto d = example();
    CHECK(d.particles()[0].id.value == 2);
    CHECK(d.particles()[1].id.value == 9);
    CHECK(Document::create(1, {0, 0}, {{{0}, fields()}}, {0}).error()->code == Code::invalid_data);
    CHECK(Document::create(1, {0, 0}, {{{2}, fields()}, {{2}, fields()}}, {2}).error()->code ==
          Code::duplicate_id);
    for (double bad : {0.0, -1.0, not_a_number, inf}) {
        auto f = fields();
        f.mass = bad;
        auto r = Document::create(1, {0, 0}, {{{2}, f}}, {2});
        REQUIRE(r.error());
        CHECK(r.error()->code == Code::invalid_data);
        CHECK(r.error()->entity->value == 2);
        CHECK(r.error()->path == "particles.mass");
        f = fields();
        f.display_radius = bad;
        CHECK(Document::create(1, {0, 0}, {{{2}, f}}, {2}).error()->path ==
              "particles.display_radius");
    }
    for (double bad : {not_a_number, inf}) {
        CHECK(Document::create(1, {bad, 0}, {}, {0}).error()->path == "gravity.x");
        CHECK(Document::create(1, {0, bad}, {}, {0}).error()->path == "gravity.y");
        for (int component = 0; component < 4; ++component) {
            auto f = fields();
            if (component == 0)
                f.initial_position.x = bad;
            if (component == 1)
                f.initial_position.y = bad;
            if (component == 2)
                f.initial_velocity.x = bad;
            if (component == 3)
                f.initial_velocity.y = bad;
            auto r = Document::create(1, {0, 0}, {{{2}, f}}, {2});
            REQUIRE(r.error());
            CHECK(r.error()->entity->value == 2);
        }
    }
    CHECK(Document::create(2, {0, 0}, {}, {0}).error()->code == Code::unsupported_feature);
    CHECK(Document::create(1, {0, 0}, {{{9}, fields()}}, {8}).error()->code == Code::invalid_data);
}
TEST_CASE("S09-S15 and S21 edits are atomic and deterministic") {
    const auto d = example(), before = d;
    const auto added = add_particle(d, fields());
    REQUIRE(added.value());
    CHECK(added.value()->affected_id->value == 10);
    CHECK(added.value()->document.high_water().value == 10);
    auto invalid = fields();
    invalid.mass = 0;
    CHECK(add_particle(d, invalid).error());
    CHECK(add_particle(d, fields()).value()->affected_id->value == 10);
    const auto exhausted =
        Document::create(1, {0, 0}, {}, {std::numeric_limits<std::uint64_t>::max()});
    CHECK(add_particle(*exhausted.value(), fields()).error()->code == Code::id_exhausted);
    auto changed = fields();
    changed.initial_position = {8, 7};
    auto replaced = replace_particle(d, {2}, changed);
    REQUIRE(replaced.value());
    CHECK(replaced.value()->document.particles()[0].fields == changed);
    CHECK(replaced.value()->document.particles()[1] == d.particles()[1]);
    CHECK(replace_particle(d, {2}, invalid).error());
    const auto removed = remove_particle(d, {9});
    REQUIRE(removed.value());
    CHECK(removed.value()->document.high_water().value == 9);
    CHECK(add_particle(removed.value()->document, fields()).value()->affected_id->value == 10);
    for (auto id : {opensim::core::EntityId{0}, opensim::core::EntityId{3}}) {
        const auto code = id.valid() ? Code::missing_reference : Code::invalid_argument;
        CHECK(remove_particle(d, id).error()->code == code);
        CHECK(replace_particle(d, id, fields()).error()->code == code);
    }
    const auto gravity = set_gravity(d, {1, 2});
    REQUIRE(gravity.value());
    CHECK((gravity.value()->document.gravity() == opensim::math::Vec2{1, 2}));
    CHECK(gravity.value()->document.particles()[0] == d.particles()[0]);
    CHECK_FALSE(gravity.value()->affected_id);
    CHECK(set_gravity(d, {not_a_number, 0}).error());
    CHECK(d == before);
    CHECK(add_particle(example(), fields()).value()->document == added.value()->document);
}
TEST_CASE("S16-S18 snapshots own state and validate publication") {
    const auto retained = [] {
        auto d = example();
        return *Snapshot::initial(d).value();
    }();
    CHECK(retained.time() == 0);
    CHECK(retained.particles()[0].id.value == 2);
    const auto d = example();
    auto samples =
        std::vector<ParticleSample>(retained.particles().begin(), retained.particles().end());
    samples[0].position = {8, 9};
    const auto later = Snapshot::create(d, 1, samples);
    REQUIRE(later.value());
    samples[0].position = {99, 99};
    CHECK((later.value()->particles()[0].position == opensim::math::Vec2{8, 9}));
    CHECK((retained.particles()[0].position == opensim::math::Vec2{1, 2}));
    // Scene's reset building block, not a claim that physics stepping exists.
    CHECK(*Snapshot::initial(d).value() == retained);
    CHECK(Snapshot::create(d, -1, samples).error());
    CHECK(Snapshot::create(d, not_a_number, samples).error());
    samples[1].id = samples[0].id;
    CHECK(Snapshot::create(d, 1, samples).error()->code == Code::duplicate_id);
}
TEST_CASE("S19-S20 world-space packet validation") {
    const auto packet = DrawPacket::create(
        {Circle{{1, 2}, 0.5, {1, 0, 0, 1}}, Line{{0, 0}, {1, 0}, 0.1, {1, 0, 0, 1}}});
    REQUIRE(packet.value());
    REQUIRE(packet.value()->primitives().size() == 2);
    CHECK((std::get<Circle>(packet.value()->primitives()[0]).center == opensim::math::Vec2{1, 2}));
    CHECK(DrawPacket::create({Circle{{not_a_number, 0}, 1, {1, 0, 0, 1}}}).error());
    for (double bad : {0.0, -1.0, not_a_number, inf}) {
        CHECK(DrawPacket::create({Circle{{0, 0}, bad, {1, 0, 0, 1}}}).error());
        CHECK(DrawPacket::create({Line{{0, 0}, {1, 0}, bad, {1, 0, 0, 1}}}).error());
    }
    for (double bad : {-0.1, 1.1, not_a_number, inf})
        CHECK(DrawPacket::create({Circle{{0, 0}, 1, {bad, 0, 0, 1}}}).error());
    CHECK(DrawPacket::create({Line{{0, 0}, {0, 0}, 1, {1, 0, 0, 1}}}).has_value());
}
