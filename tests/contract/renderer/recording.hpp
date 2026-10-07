#pragma once
#include "raster.hpp"
#include <optional>
namespace test_renderer {
using namespace opensim;
struct Request {
    scene::Snapshot snapshot;
    scene::DrawPacket packet;
    renderer::Camera camera;
    renderer::Extent extent;
};
struct RecordingRenderer {
    std::optional<Request> last;
    std::optional<core::Diagnostic> fail_next;
    core::Result<renderer::Frame> render(const scene::Snapshot &s, const scene::DrawPacket &p,
                                         renderer::Camera c, renderer::Extent e) {
        last.emplace(Request{s, p, c, e});
        if (fail_next) {
            auto failure = std::move(*fail_next);
            fail_next.reset();
            return core::Result<renderer::Frame>::failure(std::move(failure));
        }
        // Independently specified fixture; does not execute the raster backend.
        return core::Result<renderer::Frame>::success(
            renderer::detail::FrameBuilder::make({1, 1}, {11, 22, 33, 255}));
    }
};
struct BackendWrapper {
    std::optional<Request> last;
    std::optional<core::Diagnostic> fail_next;
    core::Result<renderer::Frame> render(const scene::Snapshot &s, const scene::DrawPacket &p,
                                         renderer::Camera c, renderer::Extent e) {
        last.emplace(Request{s, p, c, e});
        if (fail_next) {
            auto failure = std::move(*fail_next);
            fail_next.reset();
            return core::Result<renderer::Frame>::failure(std::move(failure));
        }
        return renderer::render(s, p, c, e);
    }
};
} // namespace test_renderer
