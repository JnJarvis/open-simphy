#include "session.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <limits>
using namespace opensim;
namespace {
app::Session session() {
    const auto s = app::Session::create();
    REQUIRE(s.has_value());
    return *s.value();
}
struct Recorder {
    std::optional<scene::Snapshot> snapshot;
    renderer::Extent extent{};
    unsigned calls = 0;
    core::Result<renderer::Frame> operator()(const scene::Snapshot &s, const scene::DrawPacket &p,
                                             renderer::Camera c, renderer::Extent e) {
        snapshot = s;
        extent = e;
        ++calls;
        return renderer::render(s, p, c, e);
    }
};
struct Presenter {
    unsigned calls = 0;
    bool fail = false;
    std::vector<std::uint8_t> bytes;
    core::Result<void> operator()(const renderer::Frame &frame) {
        ++calls;
        if (fail)
            return core::Result<void>::failure({core::Code::internal_error,
                                                core::Severity::error,
                                                "Injected host failure",
                                                {},
                                                "host"});
        REQUIRE(frame.stride() == frame.width() * 4);
        bytes.assign(frame.bytes().begin(), frame.bytes().end());
        return core::Result<void>::success();
    }
};
} // namespace
TEST_CASE("Initial advance pause reset and analytic trajectory") {
    auto s = session();
    const auto initial = s.snapshot();
    REQUIRE_FALSE(s.running());
    REQUIRE(s.tick(1).has_value());
    REQUIRE(s.snapshot() == initial);
    REQUIRE(s.advance().has_value());
    REQUIRE(s.snapshot().time() == 1.0 / 128);
    REQUIRE(s.reset().has_value());
    REQUIRE(s.snapshot() == initial);
    s.set_running(true);
    for (int i = 0; i < 128; ++i)
        REQUIRE(s.tick(1.0 / 128).has_value());
    const auto advanced = s.snapshot();
    REQUIRE(advanced.time() == 1);
    for (std::size_t i = 0; i < initial.particles().size(); ++i) {
        const auto &a = initial.particles()[i];
        const auto &b = advanced.particles()[i];
        REQUIRE(std::abs(b.position.x - (a.position.x + a.velocity.x)) <= 1e-12);
        REQUIRE(std::abs(b.position.y - (a.position.y + a.velocity.y - 4.90625)) <= 1e-12);
        REQUIRE(std::abs(b.velocity.y - (a.velocity.y - 9.8125)) <= 1e-12);
    }
    s.set_running(false);
    REQUIRE(s.tick(10).has_value());
    REQUIRE(s.snapshot() == advanced);
    REQUIRE(s.reset().has_value());
    REQUIRE_FALSE(s.running());
    REQUIRE(s.snapshot() == initial);
    s.set_running(true);
    REQUIRE(s.reset().has_value());
    REQUIRE(s.running());
}
TEST_CASE("Fixed-step accumulation is bounded and pause discards backlog") {
    auto s = session();
    s.set_running(true);
    REQUIRE(s.tick(1.0 / 256).has_value());
    REQUIRE(s.snapshot().time() == 0);
    REQUIRE(s.tick(1.0 / 256).has_value());
    REQUIRE(s.snapshot().time() == 1.0 / 128);
    REQUIRE(s.tick(100).has_value());
    REQUIRE(s.snapshot().time() == 33.0 / 128);
    const auto before = s.snapshot();
    REQUIRE(s.tick(-1).error());
    REQUIRE(s.tick(std::numeric_limits<double>::infinity()).error());
    REQUIRE(s.tick(std::numeric_limits<double>::quiet_NaN()).error());
    REQUIRE(s.snapshot() == before);
    REQUIRE(s.tick(1.0 / 256).has_value());
    s.set_running(false);
    s.set_running(true);
    REQUIRE(s.tick(1.0 / 256).has_value());
    REQUIRE(s.snapshot() == before);
}
TEST_CASE("Composition forwards current snapshots and preserves frames on failure") {
    auto s = session();
    Recorder render;
    Presenter present;
    REQUIRE(s.draw({64, 48}, render, present).has_value());
    REQUIRE(render.calls == 1);
    REQUIRE(present.calls == 1);
    REQUIRE(*render.snapshot == s.snapshot());
    REQUIRE(present.bytes.size() == 64 * 48 * 4);
    REQUIRE(s.advance().has_value());
    REQUIRE(s.draw({80, 48}, render, present).has_value());
    REQUIRE(*render.snapshot == s.snapshot());
    REQUIRE(render.extent == renderer::Extent{80, 48});
    const auto before = s.snapshot();
    const auto saved = present.bytes;
    REQUIRE(s.draw({0, 0}, render, present).has_value());
    REQUIRE(render.calls == 2);
    s.set_running(true);
    present.fail = true;
    const auto failed = s.draw({64, 48}, render, present);
    REQUIRE(failed.error());
    REQUIRE(failed.error()->path == "host");
    REQUIRE_FALSE(s.running());
    REQUIRE(s.snapshot() == before);
    REQUIRE(std::vector<std::uint8_t>(s.last_frame()->bytes().begin(),
                                      s.last_frame()->bytes().end()) == saved);
    present.fail = false;
    const auto invalid = s.draw({4097, 1}, render, present);
    REQUIRE(invalid.error());
    REQUIRE(invalid.error()->path == "extent");
    REQUIRE(present.calls == 3);
    REQUIRE(s.snapshot() == before);
    REQUIRE(s.draw({64, 48}, render, present).has_value());
}
