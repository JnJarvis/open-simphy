# Minimal portable rendering policy

Status: PROPOSED under REN-001; requires independent review before REN-002.
Decision: a portable CPU raster backend owned by renderer produces an owning RGBA8
frame. A thin app-private SDL3 host displays it and handles events. Windows is the
first desktop smoke target. Linux/macOS renderer math and raster tests stay headless.
This selects a small demo route, not an editor toolkit or permanent GPU architecture.

## Alternatives considered

| Route | Platforms/license | Setup, CI and ownership consequence |
|---|---|---|
| CPU raster + SDL3 host (selected) | SDL supports Windows/Linux/macOS; zlib | Pure renderer output can be tested without display or SDL. App owns SDL window/texture/event loop. Modest software rendering cost and a small explicit raster implementation. |
| SDL3 drawing directly in renderer | Same host support/license | Less CPU raster code, but renderer needs a carefully typed host-surface bridge and SDL-backed tests; GPU/backend pixel variation complicates image assertions. Viable later backend. |
| GLFW + OpenGL | Windows/Linux/macOS, zlib/libpng | Explicit window ownership, but needs graphics loader, shader/context lifecycle and framebuffer tests for this small 2D slice. |
| raylib | Multiple desktop targets, zlib/libpng | Convenient drawing, but bundled graphics/window lifecycle needs more care to keep app composition and renderer ownership separate. Broader than required. |

These are engineering tradeoffs, not performance measurements. CPU rendering is
for the bounded particle demo, not a promised scalable full simulator backend.
The owning-frame interface permits later replacing the raster backend without
changing physics, scene values or file formats. No new internal DAG edge is added.

## Dependency and host boundary

renderer depends only on core/math/scene and standard library. It takes an immutable
snapshot, validated scene packet, camera and output dimensions; returns
Result<Frame>. Frame owns width/height/stride and row-major RGBA bytes. No filesystem,
wall clocks, SDL headers, native handles, callbacks to app or platform dependency.

SDL belongs only to the private INT-001 host adapter. Native window, SDL_Renderer
and SDL_Texture have one main-thread owner. App asks renderer for a full frame,
then copies rows into a streaming SDL_PIXELFORMAT_RGBA32 texture using lock/unlock.
Use the returned destination pitch, not an assumed width*4. SDL describes locked
texture storage as write-only; copy every row and never depend on previous contents.
Set texture blending to NONE for an already composited opaque frame. Clear the
window backbuffer, copy texture, present once. Check all bool/pointer results;
convert failures to visible app diagnostics and unwind resources with RAII.
Destroy texture before SDL_Renderer, then window, then quit SDL. Do not keep locked
pointers after unlock. No UI API calls from physics or worker threads.

The app-private host performs presentation only: it must not implement circle/line
rasterization or world-to-pixel mapping. Those belong to renderer. This keeps the
architecture's graphics backend private to renderer while the native presentation
surface stays in the host adapter; no platform surface is needed by the CPU backend.

Selected dependency: SDL 3.2.28, release tag release-3.2.28, resolved Git identity
7f3ae3d57459e59943a4ecfefc8f6277ec6bf540. Source archive:
https://codeload.github.com/libsdl-org/SDL/tar.gz/refs/tags/release-3.2.28
SHA256: 5c908f07a93087037d3319bc5d820ba8df14ac98bdbdf24af9084ef460ce01b1.
Archive downloaded/hash checked on 2026-10-06; not compiled by this policy task.
This is a deliberately selected release, not a claim that it is the newest version.
Retain the release's LICENSE.txt with distributed dependency notices. Project
licensing remains a separate unresolved release decision.

Build acquisition belongs to a scoped BUILD follow-up, before INT-001: pinned
FetchContent or equivalent immutable archive/hash, explicit offline source option,
no arbitrary installed-version fallback. Link SDL privately to app. Prefer the
static SDL target for the first local demo, with upstream's transitive system libs;
disable SDL tests/examples and unused subsystems through documented options only.
Use upstream CMake integration. Do not put SDL fetching in a public header or in
headless configuration. SDL requires C language enabled by that build task.

## Camera and concrete scenario

Camera carries finite world center (cx,cy) in meters and finite positive scale s
in physical pixels per meter. For output size W,H physical pixels, map:

```text
px = W/2 + (x-cx)*s
py = H/2 - (y-cy)*s
pixel radius = world_radius*s
pixel line width = world_width*s
```

No integer truncation until clipping/coverage. Pixel centers are (i+0.5,j+0.5),
origin top-left. REN-002 must freeze exact coverage, blending and bounds before
REN-003 code. Proposed baseline: filled circles, round-ended line segments,
one sample per pixel (no anti-aliasing), opaque dark background, particles first
in ascending ID order, then overlay primitives in packet order. A zero-length
line draws a disk of half the line width, consistent with an allowed scene value.
No text/fonts, selection handles, grid, 3D, textures from disk or editor widgets.

Example: W=800,H=600, center=(0,0), scale=100. World (1,2) maps to (500,100),
radius 0.5 maps to 50 pixels. Line (0,0)->(1,0), width 0.1 maps from (400,300)
to (500,300), width 10 pixels. Point (0,-1) maps below center to (400,400).
These are projection expectations, not golden raster output before REN-002.

Physical output size comes from SDL_GetRenderOutputSize, not window coordinate
size. Disable SDL logical scaling for one-to-one texture presentation. A resize
recreates the streaming texture and asks renderer for the new dimensions, retaining
world center and scale. Content extent changes rather than stretching physics.
Skip drawing while minimized/zero output; continue polling events, do not allocate
zero-size textures or treat minimization as lost simulation state.

At high DPI, scaling remains explicitly physical pixels/meter in this first slice;
this may change apparent physical size across monitors. App can deliberately change
scale on a DPI transition later, but must never change world state. Test a move
between monitors and verify queried output size, no stretching, preserved state.

## Failure and bounded work

REN-002 must specify concrete maximum frame allocation, primitive count and work
budget. Reject invalid/nonfinite camera or transformed coordinates before any
floating-to-integer cast. Clip in floating coordinates to bounded output dimensions;
never loop over unbounded world radius/line extent. Check dimension/stride/byte
products before allocation. Both huge finite inputs and NaN/inf need tests.
Build a new frame locally; failure must preserve any previously displayed frame.
No partial frame publication or silently skipped invalid primitive. A render error
does not advance or reset physics. Report rejected resource limits clearly.

Color input is straight-alpha sRGB scene data. REN-002 must explicitly choose
whether compositing occurs in display-encoded values or linear light, with golden
blend examples; do not let the host/GPU choose by accident. Frame channels are
explicit bytes RGBA in memory, never a host-endian uint32 pixel assumption.

## Testing and build ownership

Required all-platform headless renderer tests: projection/y flip; particle radius;
ordered overlays; viewport clipping; circle/line coverage including zero length;
alpha examples; copied frame isolation; invalid values and resource limits;
resize dimensions with no world mutation. Use tiny synthetic fixtures with
provenance and independently calculated expected pixels. No GPU screenshot tolerance
may substitute for deterministic CPU raster tests. No physics stepping in renderer.

Existing headless presets currently omit renderer. A scoped BUILD follow-up must
include the portable renderer when its implementation exists while keeping SDL/app
optional, and add the SDL acquisition described above. REN-003 must not modify
root build files outside its scope or hide tests behind OPENSIM_BUILD_APP=ON.
Coordinator assigns this build task before renderer implementation is scheduled.

INT-001 owns Windows visual smoke evidence: launch at normal and high DPI; verify
orientation/circles/lines; resize/minimize/restore; advance/pause/reset; close cleanly;
surface creation and texture failure diagnostics where injectable. Renderer CI
success is not native window smoke evidence. Linux/macOS desktop smoke is required
before declaring those desktop ports supported; headless results alone do not.
Future ports should reuse the same SDL adapter with platform packaging adjustments,
not add branches to scene/math/physics.

## Primary references checked 2026-10-06

- [SDL release](https://github.com/libsdl-org/SDL/releases/tag/release-3.2.28),
  [license](https://www.libsdl.org/license.php),
  [platforms](https://wiki.libsdl.org/SDL3/README-platforms),
  [CMake integration](https://wiki.libsdl.org/SDL3/README-cmake).
- [Streaming texture lock/pitch](https://wiki.libsdl.org/SDL3/SDL_LockTexture),
  [pixel formats](https://wiki.libsdl.org/SDL3/SDL_PixelFormat),
  [texture upload caveats](https://wiki.libsdl.org/SDL3/SDL_UpdateTexture).
- [Physical render size](https://wiki.libsdl.org/SDL3/SDL_GetRenderOutputSize),
  [window/DPI distinction](https://wiki.libsdl.org/SDL3/SDL_CreateWindow),
  [presentation lifecycle](https://wiki.libsdl.org/SDL3/SDL_RenderPresent).
- [GLFW overview/license](https://www.glfw.org/),
  [raylib overview](https://www.raylib.com/), [raylib license](https://www.raylib.com/license.html).

These sources establish available APIs and dependencies, not application build or
smoke results. No external API code is implemented by this planning task.
