# BUILD-002 — Create CMake and CI test scaffold

- Stage: 0
- Status, priority, ownership: authoritative in [registry](../registry.json), initially BLOCKED, P1, unowned.
- Dependencies: BUILD-001
- Blocks: TEST-001, CORE-002, MATH-002
- External gates: None

## Objective
Create a minimal build/test scaffold according to the accepted toolchain policy.

## Allowed files/modules
- `CMakeLists.txt`
- `CMakePresets.json`
- `cmake/`
- `.github/workflows/`
- `.clang-format`
- `.gitignore`

Registry/report updates go through the coordinator. A new dependency or required edit
outside these paths needs a scoped follow-up or reviewed scope amendment first.

## Relevant specifications/interfaces
- [spec/architecture.md](../../spec/architecture.md)
- [spec/interfaces.md](../../spec/interfaces.md)
- [spec/testing.md](../../spec/testing.md)
- [CONTRIBUTING.md](../../CONTRIBUTING.md)
- [tasks/items/BUILD-001.md](../../tasks/items/BUILD-001.md)

Read the merged outputs of dependency tasks as well as these baseline references.

## Requirements and acceptance criteria
Provide module registration convention and category test hooks, developer presets, selected CI jobs and clean-checkout instructions in cmake/README.md; empty modules must not require unfinished source.

Every clause above requires evidence; the reviewer checks each in the completion report.

## Required tests/checks
Configure and build from a clean directory; run the registered scaffold check through CTest locally and on required CI targets; verify intentionally failed check is surfaced by the harness.

Run existing affected checks and `python tools/tasks.py validate` as applicable.
For documentation tasks, record reviewed examples and checklist outcomes.

## Deliverables
Reproducible build scaffold and CI configuration.

## Notes/assumptions
Only the small Stage 1 scope is authorized. No speculative SimPHY behavior.
Contract tasks produce reviewed specifications; downstream implementation owns C++
headers and must conform to those specifications. Missing decisions are blockers,
not permission to assume another agent's unpublished work.

## Completion report
Initially incomplete. Coordinator records [report](../reports/BUILD-002.md) before REVIEW,
using [template](../templates/completion.md). No report exists until work is submitted.
