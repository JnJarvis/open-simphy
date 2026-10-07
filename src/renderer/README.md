# CPU renderer

Implements accepted `spec/contracts/renderer.md`. Public entry point:
`opensim::renderer::render(snapshot, packet, camera, extent)` in
`<opensim/renderer/renderer.hpp>`, linked through `opensim::renderer`.
Frames own immutable RGBA bytes; width/height/stride/bytes expose the result.
No initialization, surface, timer, SDL or physics dependency is present.

The entire request is projected and checked before pixel storage is allocated.
Clipped boxes bound both work estimates and loops. Circles and round capsules use
pixel-center coverage; overlays composite in packet order over particles. A failed
request returns its diagnostic and cannot replace a caller's earlier frame.
Allocation exceptions propagate. Resize is simply a new extent on the next call.

`raster.hpp` is private: preflight permits testing exact limits without painting
large images. FrameBuilder is a private construction seam used only after
preflight and by the consumer-owned test recorder for its independent fixture.
Neither is exported as a supported public API.

Tests contain synthetic, hand-specified fixtures from REN-002 conformance vectors;
no external images or SimPHY material. CPU smoke is headless. Native presentation
and window lifecycle checks belong to INT-001.
