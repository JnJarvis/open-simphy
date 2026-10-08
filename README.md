# Open Simphy - C++ physics workspace

A modular, independent open-source simulator project with a working Windows
workspace preview. The current slice models free particles under uniform gravity,
with a real editor, toolbar, scene list, viewport and property inspector.
The native document codec exists. The app opens real `.ssim` archives and now has
an experimental circular-mechanism path: the supplied Newton Cradle reconstructs
script-created balls, textures, distance joints and controls, with drag, collisions,
playback and reset. Other bodies/joints, event scripts, sound, optics, circuits and
3D remain incomplete. See [SSIM opening](docs/ssim.md). This is an early application,
not a finished product or a claim of full SimPHY compatibility.

## Try the demo

See [demo instructions](docs/demo.md) for build commands, controls and the exact
scene. On Windows, use a Visual Studio 2022 Developer PowerShell:

```text
cmake -S . -B build/demo -G Ninja -DCMAKE_BUILD_TYPE=Debug -DOPENSIM_BUILD_APP=ON
cmake --build build/demo --parallel 4
ctest --test-dir build/demo --output-on-failure
./build/demo/src/app/opensim_demo.exe
```

Space plays/pauses, Right Arrow advances once, R resets, Escape closes. It starts
paused. Particles may fall out of view; the axes are reference lines, not walls.

The engine, renderer and app controller have native Windows/Linux/macOS headless
checks. A separate [desktop workflow](.github/workflows/desktop.yml) builds the
SDL executable on all three systems in Debug and Release. Those jobs check
packaging inputs and failure paths with a dummy video driver, not interactive
window behavior. Windows smoke has passed at normal and150% display scaling. Mixed-monitor
checks remain outstanding; Linux/macOS desktop support is not yet
validated. See [integration evidence](tasks/reports/INT-001.md).

## Headless development

```text
cmake --preset headless-debug
cmake --build --preset headless-debug
ctest --preset headless-debug --output-on-failure
```

C++20, CMake 3.28+, Ninja 1.11+; exact CI toolchain versions are in
[build policy](spec/build-policy.md). Catch2 3.7.1 is test-only. SDL 3.2.28 is
app-private; headless builds do not acquire it. Tests-disabled builds do not
require Catch2 or Python. Dependency downloads are pinned; see
[build notes](cmake/README.md) and [offline SDL setup](cmake/SDL.md).

## Architecture and contributing

Scene, math and physics have no native window dependencies. Renderer produces
owning RGBA frames; app owns SDL events and presentation. The modular boundary is
intended to keep later Linux/macOS desktop work in platform integration and
packaging rather than physics algorithms.

Read [AGENTS.md](AGENTS.md), [CONTRIBUTING.md](CONTRIBUTING.md),
[architecture](spec/architecture.md), [task workflow](tasks/README.md), and the
[roadmap](spec/roadmap.md). Select only READY tasks and claim through the canonical
coordinator. The [registry](tasks/registry.json) is authoritative; independent
review and merge are required before DONE. Some planning documents describe
historical initial state; consult current task reports for actual delivery evidence.

Research intake and compatibility are separate, evidence-gated work. No proprietary
SimPHY binaries or unlicensed fixtures belong here. Project release licensing is
still unresolved; this development demo is not a release package. Retain SDL's
notice with any authorized future distribution.
