# Foundation build and portability policy

Status: ACCEPTED under BUILD-001, 2026-10-06, following user review.
Owner: build/integration. See [RFC](../rfcs/BUILD-001-portable-foundation.md) and
[decision record](../adr/BUILD-001-portable-toolchain.md). This document supplements
the existing module DAG; it does not introduce new module dependencies.

## Target and language baseline

Windows x64 is the first interactive development target. Portable headless
components must also build on Linux x64 and macOS arm64 from the first compiled
foundation. A successful headless job does not declare a supported desktop port.
macOS x64 builds are a later matrix extension; never encode pointer-size or CPU
assumptions that prevent them.

Use C++20 in standard-conforming mode, no compiler extensions. Avoid C++ named
modules, coroutine-based infrastructure, std::format and experimental library
features in the initial foundation. Ordinary headers and compiled libraries keep
the three compiler families usable without a new module build system.

| Target | Initial validation compiler | Build configuration | Evidence required |
|---|---|---|---|
| Windows x64 | Visual Studio 2022 MSVC 19.44 / v143 | Debug and Release | Local configure/build/CTest and CI when a remote exists |
| Linux x64 | GCC 13, plus Clang 18 compiler coverage | Debug and Release | Native runner headless build/CTest |
| macOS arm64 | AppleClang from Xcode 16.4 | Debug and Release | Native runner headless build/CTest |

These are selected validation versions, not claims that older compilers cannot
work or that these jobs have already passed. CI records full compiler, standard
library, OS and architecture versions. Runner/tool availability must be checked
when BUILD-002 is implemented; a missing selected image is a recorded blocker or
reviewed version adjustment, not a silently omitted job. Never use `-march=native`
or `/arch:AVX*` in the portable baseline.

## Build system

- CMake minimum 3.28; presets schema 6 suffices. Prefer the same `headless-debug`
  and `headless-release` Ninja presets on all hosts. Ninja minimum 1.11.
- Windows uses a Developer PowerShell/command prompt so the selected MSVC compiler
  and SDK are available. No hardcoded installation paths in tracked presets.
- A Windows Visual Studio 2022 generator preset is a useful optional convenience,
  not the only supported build route. Avoid shell-specific build scripts.
- Each module owns its CMake target and public include directory. Use target-scoped
  compile features, definitions, warnings, include paths and link dependencies.
  Set `CXX_EXTENSIONS OFF`; do not set global compiler flags for downstream users.
- Internal targets use `opensim_<module>` and aliases `opensim::<module>`.
  Only declared DAG edges may appear in target dependencies. A consumer cannot
  access another module's private include directory through transitive exposure.
- Discover module CMakeLists only from an explicit ordered allowlist matching the
  DAG; skip genuinely empty placeholders. Never glob source files into a target.
- Engine/core tests configure without renderer, editor, app, native window or
  graphics packages. BUILD-002 supplies `OPENSIM_BUILD_APP=OFF` and
  `OPENSIM_BUILD_TESTS=ON` in headless presets. Tests disabled means no Catch2
  download or Python requirement for building engine libraries.
- Keep output below `build/<preset>/`; no generated source, absolute machine path,
  cached dependency, or IDE-user settings in version control.

## Platform boundaries and portability acceptance

| Concern | Owning boundary | Portable consumers receive |
|---|---|---|
| Windows handles, Cocoa objects, X11/Wayland objects | platform adapter; temporary Stage 1 app-private host adapter | Opaque host surface/lifecycle through app composition |
| File dialogs, paths, application-data locations, filesystem operations | platform adapter | Explicit byte buffers/results; codecs do not open files |
| Wall clocks, timers, event loop, keyboard/mouse input | platform adapter/app | Simulation dt and normalized input values; engine never polls OS |
| Graphics device/API objects | private renderer backend | Snapshot and primitive data contracts |
| Native error values | adapter | Portable diagnostic code and contextual text |

No `windows.h`, Cocoa/Objective-C, X11/Wayland, platform-specific graphics headers,
native handles or OS macros in core, math, scene, physics, collision, constraints,
analysis, visualization, serialization or compat public interfaces. No OS-specific
branches inside their algorithms. Adapter factories are composed by app; engines
do not locate adapters or depend on a global service registry.

The current DAG deliberately gives renderer/editor no dependency on platform.
App mediates the two sides. Future platform ports must obey that edge direction;
do not smuggle a platform include into a domain header. A Stage 1 app-private host
adapter may become a platform implementation later without changing domain data.

Use fixed-width integers for identities and persisted fields, `double` for initial
simulation scalars, and `size_t` only for in-memory sizes. Never serialize struct
memory, `long`, pointer values, native enums, host-endian binary or platform paths.
UTF-8 is the interchange encoding for text; OS adapters perform native conversion.
Asset references use document-relative logical names, not drive letters. UTF-8,
case-sensitive names, spaces, read-only files and missing assets need adapter/IO
tests when those modules are implemented. Keep display scale separate from world
units, so high-DPI screens and macOS scaling do not affect physics.

Floating-point tests use explicit tolerances. No fast-math flags or cross-platform
bitwise determinism promise. Stable object iteration and fixed dt will support
repeatable reference tests without binding math to an OS or graphics API.

BUILD-002 must make these boundaries visible in target layout; later module tasks
add checks as code appears. Before any domain component is DONE, its public headers
must compile in isolation and its headless tests must not require an OS adapter.
Record native Linux/macOS results before claiming those targets tested; a Windows
cross-compile alone is not equivalent to a native run.

## Tests and dependencies

Use CTest for test orchestration and Catch2 3.7.1 for C++ assertions/discovery.
Catch2 is test-only; no dependency is added to a runtime public interface. TEST-001
owns category conventions and helpers; BUILD-002 supplies registration hooks.
Use Python 3.11+ only for existing repository coordination checks.

BUILD-002 records the exact Catch2 source archive URL plus SHA-256 (or full immutable
upstream commit) in one dependency declaration. A version tag alone is not a lock.
Permit a documented local source override for offline use; show the override in
configure output and record its provenance. Never silently fall back to a different
installed version. No package manager is mandated for the initial single dependency.
New dependencies require purpose, version/hash, license and supported-platform notes.
Catch2's BSL-1.0 notice must accompany any vendored copy; keep notices when packaging.
No package installation or downloads happen when simply importing a public header.

## Coding and checking rules

- Namespace `opensim::<module>`; PascalCase types, snake_case functions/variables,
  lowercase snake_case filenames. Header guard or pragma once is allowed; no
  implementation definitions in public headers unless inline/template by design.
- RAII; values and unique ownership by default; no owning raw pointers. Borrowed
  views state lifetime and invalidation. No hidden mutable global state.
- Public domain operations report expected invalid-input failures as values.
  Exceptions are enabled for standard-library allocation and unexpected failures;
  do not catch-and-ignore them or mark allocating operations `noexcept`. App/host
  boundaries handle exceptional failures; no exception may cross a C callback.
- Use `[[nodiscard]]` for result values. The core result contract defines diagnostics;
  math keeps its zero-dependency policy through optional checked operations.
- MSVC: `/W4 /permissive- /utf-8 /EHsc`; GCC/Clang: `-Wall -Wextra -Wpedantic`.
  Warnings-as-errors is a project CI option, applied to owned targets only. Do not
  modify third-party targets or expose warning flags to downstream consumers.
- clang-format 18.1.8, LLVM base, 4-space indent, 100-column limit; BUILD-002 writes
  the configuration. Formatting checks are separate from compiler semantics.
- No runtime assertions as the only validation of external input. Test assertions
  must remain active in Release. Sanitizers are added once compatible runner jobs
  are selected; do not prescribe MSVC and Clang sanitizer flags interchangeably.

## Commands BUILD-002 will make available

These are the proposed clean-checkout acceptance commands, not an assertion that
the scaffold exists yet. Run from the repository root after installing the named
tools and, on Windows, entering the selected compiler's developer shell:

```text
cmake --preset headless-debug
cmake --build --preset headless-debug
ctest --preset headless-debug --output-on-failure --no-tests=error
cmake --preset headless-release
cmake --build --preset headless-release
ctest --preset headless-release --output-on-failure --no-tests=error
python tools/tasks.py validate
python -m unittest discover -s tests -p test_task_workflow.py -v
```

Linux selects GCC/Clang through `CC`/`CXX` before the first configure. macOS uses
the selected Xcode developer directory. Compiler changes require a new build
directory, not reuse of a different compiler's cache. Configure/build/test commands
are identical across shells. BUILD-002 documents acquisition and offline operation.

CI starts with versioned Windows, Ubuntu and macOS images, an explicit compiler
selection, and the same headless presets. Pin third-party actions by immutable
commit and dependencies by hash. Keep desktop smoke tests separate. With no remote
or runner currently configured, authoring a workflow is not evidence it ran.
BUILD-002's required platform jobs prevent DONE until their results exist.

## Sources and local inspection

Consulted 2026-10-06:

- [CMake presets](https://cmake.org/cmake/help/v3.28/manual/cmake-presets.7.html):
  shared configure/build/test presets, local overrides and schema compatibility.
- [MSVC standard mode](https://learn.microsoft.com/en-us/cpp/build/reference/std-specify-language-standard-version?view=msvc-170)
  and [conformance](https://learn.microsoft.com/en-us/cpp/overview/visual-cpp-language-conformance?view=msvc-170).
- [GCC C++ status](https://gcc.gnu.org/projects/cxx-status.html) and
  [Clang C++ status](https://clang.llvm.org/cxx_status.html): feature-level support,
  not a blanket guarantee for every C++20 library facility.
- [Catch2 3.7.1 release](https://github.com/catchorg/Catch2/releases/tag/v3.7.1).
- [Catch2 license at that tag](https://raw.githubusercontent.com/catchorg/Catch2/v3.7.1/LICENSE.txt).
- [Xcode 16.4 release notes](https://developer.apple.com/documentation/xcode-release-notes/xcode-16_4-release-notes).
- [clang-format 18.1.8 options](https://releases.llvm.org/18.1.8/tools/clang/docs/ClangFormatStyleOptions.html).

Local inspection found VS 2022 Community with MSVC toolset 14.44.35207, CMake
4.4.2, Ninja 1.13.0.git.kitware.jobserver-pipe-1 and Python 3.11.9. CMake's Python
launcher initially failed under the sandbox while inspecting cmake-gui.exe;
`cmake --version` succeeded with approved execution outside it. Compiler installation
discovery is not a successful C++ compilation. Linux/macOS and formatting tools
have not been run in this Windows environment.
