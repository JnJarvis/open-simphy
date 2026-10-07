# BUILD-003 implementation evidence

Owner: codex-worker. Branch: codex/build-003.
Implementation: 39d4b51b9686705b36e61cf76b7e2118f146fea3.
Status: submitted for independent review; not merged.

Changed paths: CMakeLists.txt, .github/workflows/build.yml,
cmake/{OpenSimSDL.cmake,README.md,SDL.md}, cmake/checks/CMakeLists.txt,
cmake/checks/{verify_renderer_build.py,verify_sdl_rejections.py},
cmake/checks/sdl_probe/{CMakeLists.txt,main.cpp}. All within assigned scope.

Renderer registration joins the headless topological module list, preserving absent
module handling and the explicit missing-app error. SDL setup is inert until called
by an integration target. The public setup macro enables C in the caller directory
and delegates acquisition to a private function: a function-only entry point lost
the language configuration outside its scope. No production interfaces changed.

SDL is pinned to release-3.2.28, archive SHA256
5c908f07a93087037d3319bc5d820ba8df14ac98bdbdf24af9084ef460ce01b1.
The later app links SDL3::SDL3-static privately. Unused subsystems are disabled;
video/render remain enabled. Explicit offline sources validate version/layout and
retain caller provenance responsibility; local archives validate the exact hash.
The license path and integration call sequence are documented in cmake/SDL.md.

## Observed verification

Local Windows MSVC 19.44, CMake 4.4.2, Ninja 1.13.0:

- `python cmake/checks/verify_renderer_build.py . cmake`: PASS; isolated mock
  renderer registers with tests/app disabled and no dependency acquisition or C compiler.
- `python cmake/checks/verify_sdl_rejections.py . cmake`: PASS; invalid source
  version and archive hash fail with the expected diagnostics.
- `cmake -S . -B build/no-tests -G Ninja -DOPENSIM_BUILD_TESTS=OFF
  -DOPENSIM_BUILD_APP=OFF -DFETCHCONTENT_FULLY_DISCONNECTED=ON`, then build:
  PASS. No Catch2, Python discovery or SDL required.
- Configure `cmake/checks/sdl_probe` into `build/sdl-probe` using Ninja Debug,
  build, then `ctest --test-dir build/sdl-probe --output-on-failure`: PASS, 1/1,
  for pinned network acquisition. Reconfigure with OPENSIM_SDL_SOURCE_DIR pointing
  to the acquired source, rebuild with final reduced subsystem options and retest:
  PASS, 1/1. SDL_GetVersion agrees with 3.2.28.
- Formatting with clang-format 18.1.8: PASS, 25 owned C++ files. Whitespace: PASS.

Native CI run https://github.com/JnJarvis/open-simphy/actions/runs/37568944027
at the exact implementation commit above: all eight MSVC/GCC/Clang/AppleClang
Debug/Release jobs completed successfully. These run CMake 3.28.4, Ninja 1.11.1.4,
full existing headless CTest checks including the new renderer fixture, formatting,
and tests-off builds. MSVC Debug additionally passed rejection checks and the
pinned-download static SDL version probe. Headless configurations do not acquire SDL.

## Limits and follow-up

No renderer or interactive application is implemented by this task. Native window
smoke testing belongs to INT-001; renderer behavior belongs to REN-003. Those
already-scoped tasks remain dependent on accepted prerequisites. No additional
follow-up proposal is needed. Independent review and merge remain outstanding.
