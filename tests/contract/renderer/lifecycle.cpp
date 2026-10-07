#include "fixtures.hpp"
#include "recording.hpp"
using namespace fixture;
template <class Backend> void consumer_harness() {
    Backend backend;
    std::optional<renderer::Frame> good;
    {
        const auto s = snapshot({{{1}, {1, {0, 0}, {2, 3}, 1}}});
        const auto before = s;
        const auto p = packet({circle()});
        const auto result = backend.render(s, p, {{0, 0}, 1}, {4, 4});
        REQUIRE(result.has_value());
        good.emplace(*result.value());
        REQUIRE(s == before);
        REQUIRE(backend.last->snapshot == s);
        REQUIRE(backend.last->packet.primitives().size() == 1);
        REQUIRE(backend.last->camera == renderer::Camera{{0, 0}, 1});
        REQUIRE(backend.last->extent == renderer::Extent{4, 4});
    }
    const auto saved = *good;
    const std::vector<std::uint8_t> old(saved.bytes().begin(), saved.bytes().end());
    REQUIRE(backend.last->snapshot.particles().size() == 1);
    REQUIRE(std::get<scene::Circle>(backend.last->packet.primitives()[0]).center ==
            math::Vec2{-1.5, 1.5});
    const auto resized = backend.render(snapshot(), packet(), {{0, 0}, 1}, {8, 4});
    REQUIRE(resized.has_value());
    REQUIRE(backend.last->extent == renderer::Extent{8, 4});
    REQUIRE(backend.last->snapshot.particles().empty());
    const core::Diagnostic failure{
        core::Code::internal_error, core::Severity::error, "Injected", {}, "test"};
    backend.fail_next = failure;
    const auto failed = backend.render(snapshot(), packet(), {{0, 0}, 1}, {4, 4});
    if (failed.has_value())
        good.emplace(*failed.value());
    REQUIRE(failed.error());
    REQUIRE(*failed.error() == failure);
    REQUIRE(std::vector<std::uint8_t>(good->bytes().begin(), good->bytes().end()) == old);
    REQUIRE(std::vector<std::uint8_t>(saved.bytes().begin(), saved.bytes().end()) == old);
    REQUIRE(good->width() == saved.width());
    REQUIRE(good->height() == saved.height());
    REQUIRE(backend.render(snapshot(), packet(), {{0, 0}, 1}, {4, 4}).has_value());
}
TEST_CASE("R13 R22 shared consumer ownership and failure conformance") {
    consumer_harness<test_renderer::RecordingRenderer>();
    consumer_harness<test_renderer::BackendWrapper>();
}
TEST_CASE("Real failure leaves retained frame intact after helper destruction") {
    const auto saved = [] {
        test_renderer::BackendWrapper backend;
        const auto result = backend.render(snapshot(), packet({circle()}), {{0, 0}, 1}, {4, 4});
        REQUIRE(result.has_value());
        return *result.value();
    }();
    const auto failure = renderer::render(snapshot(), packet(), {{0, 0}, 1}, {0, 0});
    REQUIRE(failure.error());
    golden(saved, {0});
}
