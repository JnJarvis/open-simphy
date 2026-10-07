# Build the portable foundation

The scaffold builds a C++20 test executable and validates the build/coordination
infrastructure. The engine/application modules are still placeholders. A passing
scaffold is not a running simulator or a supported desktop release.

## Tools

Use CMake 3.28+, Ninja 1.11+, and Python 3.11+ when tests are enabled. The validation
matrix is VS 2022 MSVC 14.44 on Windows x64, GCC 13 and Clang 18 on Linux x64,
and Xcode 16.4 AppleClang on macOS arm64. See spec/build-policy.md for ownership
rules and which platform evidence is required.

Windows: install the C++ desktop tools and a Windows SDK through Visual Studio
Installer; open Developer PowerShell for VS 2022 with x64 host/target. CMake/Ninja
must be on PATH. No installation path is embedded in tracked presets.

Linux: install GCC 13 or Clang 18 and their C++ standard library development files
through the distribution's package manager. Select CC/CXX before first configure
(for example CC=gcc-13 and CXX=g++-13); use a fresh build directory when changing
compilers. macOS: select Xcode 16.4 with the DEVELOPER_DIR environment variable or
Xcode's command-line-tools setting. Select the compiler's native architecture.

CI installs exact CMake 3.28.4, Ninja 1.11.1.4 and clang-format 18.1.8 Python wheels
with `python -m pip install cmake==3.28.4 ninja==1.11.1.4 clang-format==18.1.8`.
These are development tools; the application has no Python runtime dependency.

## Configure, build, test

From this checkout's root, use the same commands on all three platforms:

```text
cmake --preset headless-debug
cmake --build --preset headless-debug --parallel 4
ctest --preset headless-debug --output-on-failure --no-tests=error
cmake --preset headless-release
cmake --build --preset headless-release --parallel 4
ctest --preset headless-release --output-on-failure --no-tests=error
```

Use `-DOPENSIM_WARNINGS_AS_ERRORS=ON` on configure to reproduce CI warning policy.
`cmake --list-presets` lists available options; optional `windows-vs` configure and
`windows-vs-debug` build/test presets use the Visual Studio generator. Tests and
test binaries remain under the build tree. Test output is retained by CTest in
`build/<preset>/Testing/Temporary/LastTest.log`.

`build.scaffold` compiles and runs C++20/binary64 checks. `build.failure_reporting`
runs an intentionally failing Catch2 assertion through a separate nested CTest
invocation, verifies its failure message and exit status, then passes the outer
test. A nested LastTest.log therefore deliberately contains a failure. Module
checks exercise an allowed edge plus forbidden, unknown and missing module cases.
Registry and task-workflow checks use the selected Python interpreter.

## Dependency pin and offline use

Only test builds fetch Catch2. OpenSimDependencies.cmake pins the official 3.7.1
archive by SHA-256:
`c991b247a1a0d7bb9c39aa35faf0fe9e19764213f28ffba3109388e62ee0269c`.
Upstream: https://codeload.github.com/catchorg/Catch2/tar.gz/refs/tags/v3.7.1
License: BSL-1.0, retained in the fetched source's LICENSE.txt. Catch2 is not vendored
or added to runtime targets. Downloads verify TLS and content hash; do not disable
either to work around a proxy/network failure.

For offline tests, obtain and checksum that archive on a connected machine,
extract it locally, then configure with:

```text
cmake --preset headless-debug -DOPENSIM_CATCH2_SOURCE_DIR=/absolute/path/to/Catch2-3.7.1
```

Quote the entire `-D...` argument when its path contains spaces. This override is
printed at configure and must have version 3.7.1; record its checksum/source in test
evidence. A version header alone does not authenticate an arbitrary source tree.
Reconfigure with `-DOPENSIM_CATCH2_SOURCE_DIR=` to return to the locked download.

Engine-only configuration has no Python/test/graphics dependency:

```text
cmake -S . -B build/no-tests -G Ninja -DOPENSIM_BUILD_TESTS=OFF -DOPENSIM_BUILD_APP=OFF
cmake --build build/no-tests
```

At this stage there are no engine targets, so that build correctly reports no work.
Setting OPENSIM_BUILD_APP=ON fails clearly until INT-001 supplies the application.

## Module and test registration

Each src/<module>/CMakeLists.txt calls `opensim_add_module`, explicitly listing
SOURCES and/or HEADERS plus PUBLIC_DEPS/PRIVATE_DEPS as bare module names. It creates
opensim_<module> and opensim::<module>, exposes only that module's include/ path,
and validates declared direct edges against the architecture. Example for a future
compiled scene module (these files are not created by the scaffold):

```cmake
opensim_add_module(scene
    SOURCES scene.cpp
    HEADERS include/opensim/scene/scene.hpp
    PUBLIC_DEPS core math)
```

Core and math cannot depend on each other or platform. Private source directories
are never exported. Warning flags affect first-party compiled targets only; Catch2
keeps its own settings. Header-only modules propagate cxx_std_20 and public headers.
Registration does not scan arbitrary code for undeclared includes; isolated-header
tests and review remain required when real module implementations arrive.

TEST-001 owns tests/CMakeLists.txt and the test helpers. Its later category files
can call the supplied hook:

```cmake
opensim_add_test(scene_contract_tests
    CATEGORY contract
    SOURCES scene_contract_tests.cpp
    LIBRARIES opensim::scene)
```

Cases are discovered through Catch2 with a target-specific prefix and category
label. Valid categories are unit, contract, integration, regression, reference,
serialization, compatibility and malformed. Unknown categories fail configure.
Missing placeholder module/test CMakeLists are skipped; actual sources are never
globbed into build targets. Headless configuration includes the portable renderer
when registered, and omits editor/platform/app. It does not acquire SDL.

## CI and validation limits

.github/workflows/build.yml defines Debug and Release jobs for all four compiler
families, with versioned runner labels and immutable action commits. The workflow
does not upload or publish application artifacts and needs only contents:read.
Missing compiler/Xcode images must fail visibly rather than silently skip a job.
The macOS runner label may require repository/account eligibility; if unavailable,
provide a native runner with the same toolchain through a reviewed workflow change.

Native jobs have not run merely because this file exists. Record actual workflow
URLs and results in the BUILD-002 completion report before marking the task DONE.
The same applies to Windows CI once a remote is configured. Local Windows results
cannot substitute for the required Linux/macOS execution evidence.

Sources consulted for CI configuration:
[MSVC action inputs](https://github.com/ilammy/msvc-dev-cmd/tree/v1.13.0) and
[macOS arm64 image](https://github.com/actions/runner-images/blob/main/images/macos/macos-15-arm64-Readme.md).


## Native desktop verification (BUILD-004)

The separate desktop workflow compiles the user-approved merged demo on
Windows MSVC 14.44, Linux GCC 13 and macOS Xcode 16.4, each Debug/Release.
Debug enables and runs all headless tests in the app build; Release disables
tests and checks no Python discovery or Catch2 acquisition occurred. Python
runs the CI verifier; it is not a tests-off application configure dependency.
The original eight headless jobs remain unchanged.

All jobs use the existing pinned SDL acquisition and CMake 3.28.4/Ninja 1.11.1.4
packages. Linux installs video development packages from Ubuntu 24.04 repositories
(X11/XRandR/cursor/input/screensaver, xkbcommon and Mesa GL/EGL), based on SDL
3.2.28's README-linux.md. These are runner system build prerequisites, not vendored
or version-pinned project libraries; apt package versions can change with updates.
No new production library API or architecture dependency is introduced.

`python cmake/checks/verify_desktop.py build/desktop --tests ON` (or OFF) verifies
executable and exact SDL 3.2.28 LICENSE.txt SHA256, configuration/dependency layout,
and two expected diagnostic/exit-code pairs, each with a 30-second timeout.
Its `--self-test` covers absent/modified outputs, unexpected test dependencies,
wrong exit codes/messages and timeout propagation using synthetic fixtures.

Failure probes set SDL_VIDEODRIVER=dummy and SDL_RENDER_DRIVER=software only in
child-process environments. They never count as native window, GPU, high-DPI,
resize or visual smoke evidence. No release binaries are uploaded by this workflow.
Interactive hardware validation remains in docs/demo.md and the INT-001 report.
