# BUILD-001 submission

Owner: codex-worker. Coordinator: codex-coordinator. Date: 2026-10-06.
Branch: task/BUILD-001. Implementation commit: 00188d1.
Status: accepted by the user and merged, 2026-10-06.

## Deliverables and scope

- [Build policy](../../.worktrees/BUILD-001/spec/build-policy.md)
- [Technology RFC](../../.worktrees/BUILD-001/rfcs/BUILD-001-portable-foundation.md)
- [Proposed ADR](../../.worktrees/BUILD-001/adr/BUILD-001-portable-toolchain.md)

Only these three files changed on the task branch. RFC path scope was explicitly
added by the coordinator before work. Existing architectural DAG remains intact.

## Acceptance evidence

| Criterion | Evidence in build policy |
|---|---|
| C++ standard/compiler/platform matrix | C++20, selected MSVC/GCC/Clang/AppleClang matrix, Windows interactive priority and native headless distinction |
| CMake minimum and clean-checkout commands | 3.28, shared Ninja presets and explicit configure/build/CTest commands |
| Test framework | CTest plus test-only Catch2 3.7.1; no engine runtime dependency |
| Dependency acquisition/pinning | Exact hash/commit required in BUILD-002; explicit offline source override and provenance |
| Formatting/warnings | clang-format 18.1.8 and scoped compiler flags; no flags leaked to third-party targets |
| CI strategy | Native three-platform headless jobs; recorded toolchain versions; no claim of completed external runs |
| Errors/naming | Expected failures as result values; exceptional allocation remains exceptional; namespace/type/member/file rules |
| Portability | OS types confined to adapters; byte-based codec interfaces; UTF-8, fixed-width IDs, no native-layout persistence or OS assumptions in math |
| Headless suitability | Engine tests configure without graphics/window packages; tests can be disabled without Catch2/Python |
| Dependency license | Catch2 tagged BSL-1.0 text checked; attribution policy recorded; project license remains open |

Official version/conformance references are linked in the policy. These sources
verify the selected releases/features exist, not that this application builds yet.

## Checks and results

- `vswhere.exe -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`:
  found VS 2022 Community; installed toolset directory 14.44.35207.
- `cmake --version`: CMake 4.4.2; required approved execution outside sandbox after
  the Python launcher hit access denied reading a companion executable.
- `ninja --version`: 1.13.0.git.kitware.jobserver-pipe-1.
- `python --version`: 3.11.9.
- `python tools/tasks.py validate`: PASS, 17 tasks.
- `python -m unittest discover -s tests -p test_task_workflow.py -v`: PASS, 6 tests.
- `git diff --cached --check` in the task checkout before commit: PASS.
- Author checklist against the task: every policy item above is specified.

No C++ build required by this policy task; none claimed. Linux/macOS native builds,
Catch2 acquisition, formatting execution and CI are not performed in this task.
BUILD-002 cannot be DONE until its own required build/platform evidence exists.

## Assumptions and follow-ups

Windows-first choice follows user approval; later desktop Linux/macOS support must
not require engine rewrites. No framework or backend chosen ahead of REN-001.
BUILD-002 must verify runner availability and lock the dependency by immutable
identity before implementation is accepted. No remote is configured yet.

Independent reviewer: user. Approval: "it all looks good. continue building".
Outcome: ACCEPTED; acceptance metadata recorded in task commit 50ee649.
Merged baseline commit: 9021c9f.
