# SCENE-002 implementation evidence

Owner: codex-worker. Branch: codex/scene-002. Commit: 4ea2598.
Status: REVIEW; local and native CI passed; independent review/merge pending.

## Scope and behavior

Changed paths: src/scene/CMakeLists.txt, src/scene/README.md,
src/scene/include/opensim/scene/scene.hpp, src/scene/scene.cpp,
tests/contract/scene/{CMakeLists.txt,scene.cpp},
tests/unit/scene/{CMakeLists.txt,header.cpp,scene.cpp}.
Only allowed task paths changed. Public dependencies: reviewed core and math only.
No OS, graphics, persistence, physics runtime, or third-party dependency introduced.

Document construction checks revision, finite fields, positive mass/radius,
unique/nonzero IDs and high-water consistency; particles are sorted by ID.
Commands produce new documents, preserving originals on success and failure.
Add returns the fresh ID; replace/remove require existing IDs; remove retains
high-water. Snapshot construction owns samples, validates them against the starting
document and returns const access. Initial snapshots restore initial values/time 0.
Drawing packets own ordered, validated circle/line primitives in world coordinates.
Diagnostics carry entity IDs and field paths where applicable.

## Checks and results

Windows MSVC 19.44, CMake 4.4.2, Ninja 1.13, warnings as errors, Catch2 3.7.1
from BUILD-002's previously hash-verified offline source:

- cmake --preset headless-debug -DOPENSIM_WARNINGS_AS_ERRORS=ON
  -DOPENSIM_CATCH2_SOURCE_DIR=<verified source>: PASS.
- cmake --build --preset headless-debug --parallel 4: PASS.
- ctest --preset headless-debug --output-on-failure: 23/23 PASS, including existing
  core/math/support/build checks and five new scene cases.
- clang-format 18.1.8 check: PASS for 17 files before the final test-variable rename;
  renamed file reformatted with the same executable. Staged whitespace check PASS.
- Public header compiled in a dedicated translation unit with no other includes.

Contract cases cover S01-S21, with S17 limited to the initial-snapshot/reset building
block. Actual stepping/pause/reset lifecycle remains PHY/INT responsibility.
Invalid cases include mass/radius/nonfinite fields, duplicate IDs, invalid references,
exhaustion, invalid snapshots, packet values and failed edit atomicity. Unit cases
also reject runtime entity-set changes and invalid sample fields.
Initial compile caught a test constant named nan conflicting with the C library
function; renamed to not_a_number and rebuilt. No production workaround was needed.

CI: https://github.com/JnJarvis/open-simphy/actions/runs/37552678480
for exact head 4ea259842dab4d6bd19ef14ed93826f627b3b5e0. All eight jobs completed
successfully: Windows MSVC, Linux GCC/Clang, macOS AppleClang, Debug and Release.
Each job includes formatting, build, full CTest and tests-disabled configuration.

## Review notes and limits

Public API is the first implementation of accepted scene v1; contract semantics
were not edited. Snapshot::create takes the starting document to enforce run IDs;
mass, radius and gravity stay tied to that run. It allows finite dynamic positions
and velocities without choosing an integrator. Document and snapshot spans borrow
from their owning object; owning copies survive producer destruction. Result's
const payload API entails extra copies; optimize only with measured need and review.
No interactive app yet. Next work follows the coordinator handoff report.
