# BUILD-002 local verification report

Owner: codex-worker. Coordinator: codex-coordinator. Date: 2026-10-06.
Branch: task/BUILD-002. Implementation commit: c91c571 (includes 5cc122e).
Status: DONE. Required native CI passed; user approved and implementation merged.

## Deliverables and scope

Portable C++20 CMake scaffold, explicit module dependency registration, category
test hooks, Debug/Release presets, pinned Catch2 acquisition with offline override,
formatting check, and an eight-job native CI matrix. No simulator functionality.
Developer instructions: [cmake/README.md](../../.worktrees/BUILD-002/cmake/README.md).

Changed paths: CMakeLists.txt, CMakePresets.json, .clang-format,
.github/workflows/build.yml, cmake/OpenSimDependencies.cmake,
cmake/OpenSimModules.cmake, cmake/OpenSimTests.cmake, cmake/README.md,
cmake/checks/CMakeLists.txt, cmake/checks/check_format.py,
cmake/checks/module_contract/CMakeLists.txt, cmake/checks/scaffold.cpp,
cmake/checks/verify_failure.cmake, cmake/checks/verify_module.cmake.
All are within the task's allowed paths. Architecture boundaries are unchanged.

## Commands and observed results

Windows VS2022 developer environment: MSVC 19.44.35219.0, toolset 14.44.35207.

- `cmake --preset headless-debug`, build preset and CTest: PASS, 8/8.
- Release configure/build/CTest with the downloaded Catch2 source override: PASS, 8/8.
- Minimum tools: CMake 3.28.4 and explicitly selected Ninja 1.11.1
  (`1.11.1.git.kitware.jobserver-1`), Debug configure/build/CTest: PASS, 8/8.
  Command used `--preset headless-debug -B build/minimum-debug`,
  `-DCMAKE_MAKE_PROGRAM=<local ninja.exe>`, `-DOPENSIM_WARNINGS_AS_ERRORS=ON`,
  `-DOPENSIM_CATCH2_SOURCE_DIR=build/headless-debug/_deps/catch2-src`, followed by
  `--build build/minimum-debug --parallel 4` and
  `ctest --test-dir build/minimum-debug --output-on-failure --no-tests=error`.
- Initial Debug/Release used CMake 4.4.2 and Ninja 1.13.0; minimum tools were
  installed only beneath the ignored task build directory.
- Tests disabled: configure/build PASS without fetching Catch2 or requiring Python.
- Invalid offline source: expected configure rejection observed.
- App enabled before app implementation: expected clear configure rejection observed.
- clang-format 18.1.8 via `python cmake/checks/check_format.py`: PASS, one C++ file.
- Workflow YAML parsed; eight matrix entries and immutable action pins checked.
- `python tools/tasks.py validate`: PASS, 17 tasks.
- `python -m unittest discover -s tests -p test_task_workflow.py -v`: PASS, 6 tests.
- `git diff --cached --check`: PASS before implementation commit.

The eight CTest cases cover the C++20 scaffold, deliberate failure reporting,
valid/forbidden/unknown/missing module dependencies, registry validation and task
workflow tests. The failure-reporting case runs a deliberately failing nested
CTest invocation and verifies its nonzero status and diagnostic; outer suite passes.

Catch2 3.7.1 archive was downloaded with TLS and verified SHA256
`c991b247a1a0d7bb9c39aa35faf0fe9e19764213f28ffba3109388e62ee0269c`.
Release reused that local source with explicit version checking. Catch2 is test-only.

## Original blocker (resolved)

Native Linux GCC/Clang and macOS AppleClang jobs are authored but NOT executed.
There is no configured Git remote or supplied native runner. The accepted
spec/build-policy.md and BUILD-002 required checks make those results necessary
before DONE. Independent review and baseline merge also remain outstanding.
Windows results do not establish Linux/macOS portability. Downstream implementation
tasks remain blocked. Coordinator follow-up: obtain repository/runner details,
run the matrix, address failures within scope, then submit for independent review.
No acceptance criterion has been relaxed.

## GitHub verification update

User supplied https://github.com/JnJarvis/open-simphy.git and authorized publication
in context of the repository setup request. Baseline master, task/BUILD-002 and
task/SCENE-001 were pushed successfully. Native run for implementation 5cc122e:
https://github.com/JnJarvis/open-simphy/actions/runs/37522163848.
Windows MSVC Debug/Release and Linux GCC/Clang Debug/Release all completed with
success (six jobs). Both macOS jobs remain queued at this report update; no macOS
success is claimed. The missing-remote blocker is resolved; the remaining gate is
macOS execution, followed by independent review and merge.
## Runner correction requested by user

User explicitly requested cancelling the queued run and using macos-15 or
macos-latest. Run 37522163848 is confirmed cancelled. Task commit c91c571 changes
only the AppleClang runner label to macos-15, retaining Xcode 16.4 and both presets.
YAML validation confirms all eight jobs remain; git diff whitespace check passed.
Replacement run: https://github.com/JnJarvis/open-simphy/actions/runs/37526635119.
Its results are pending; previous successful jobs are evidence for 5cc122e only.

## Final native CI verification

Run 37526635119 completed successfully at exact head
`c91c5719ce063b36708fe820def74a57824f6fd6`. GitHub's run and jobs APIs confirm
all eight jobs completed with success: MSVC, GCC, Clang and AppleClang, each in
Debug and Release. No failed steps were reported. Each workflow job includes
formatting, configure, build, CTest and the tests-disabled build.

The native CI blocker is resolved. Submitted for independent review; no reviewer
approval or merge is claimed. Downstream tasks remain gated on reviewed merge.

## Acceptance and merge

Independent reviewer: user. Approval: "looks good", in response to the build
comparison and explicit merge approval request. Accepted implementation: c91c571.
Merged into master as 7425dd6b6005adaa15405ddb760493cb4afa39ce.
This acceptance supersedes the pending-review status recorded above.

