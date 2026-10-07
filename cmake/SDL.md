# Optional SDL host setup

Root CMake includes OpenSimSDL without downloads or enabling C. INT-001's app
CMakeLists must call `opensim_setup_sdl()` before creating/linking its executable,
then `target_link_libraries(<app-target> PRIVATE SDL3::SDL3-static)`.
The setup entry is a macro so enable_language(C) remains in the calling directory
scope; acquisition is a private function. Invoke in a common parent directory of
every target needing SDL, not from inside another CMake function. It is repeatable
within that directory. There is no public SDL dependency on engine/renderer targets.

Selected SDL3.2.28 archive/hash match accepted render policy. HTTPS and SHA256 checks
are mandatory. `OPENSIM_SDL_SOURCE_DIR` optionally selects extracted offline source;
version header, root CMakeLists and license must exist and version must be 3.2.28.
The caller supplies source provenance; version checking is not source authentication.
`OPENSIM_SDL_ARCHIVE` alternatively selects a local archive and checks the exact
pinned hash before extraction. Do not combine both options. Arbitrary FetchContent
source override is rejected. No system-installed/latest-version fallback exists.

Static SDL, video and render are enabled; tests/examples and audio/camera/joystick/
haptic/hidapi/power/sensor/dialog/GPU subsystems are disabled with upstream options.
SDL's render backends and transitive system libraries remain upstream-managed.
Keep `${OPENSIM_SDL_LICENSE_FILE}` (the downloaded source LICENSE.txt) with dependency
notices in any distribution. No third-party warnings are promoted by our targets.

Standalone build-only probe (no window, no SDL initialization):

```text
cmake -S cmake/checks/sdl_probe -B build/sdl-probe -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/sdl-probe --parallel 3
ctest --test-dir build/sdl-probe --output-on-failure --no-tests=error
python cmake/checks/verify_sdl_rejections.py . cmake
```

For offline validation add `-DOPENSIM_SDL_SOURCE_DIR=<verified extracted source>`
on configure. Use Windows developer environment or appropriate compiler toolchain.
Only the Windows Debug CI job runs this separate probe; all eight headless engine
jobs remain SDL-free. App ON still fails clearly until INT-001 supplies the app.
The headless renderer registration fixture uses synthetic modules and tests-off
configuration; it proves registration without relying on unfinished renderer code.
Actual renderer pixel tests arrive with REN-003, native window smoke with INT-001.
