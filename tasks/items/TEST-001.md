# TEST-001 — Add test registration and fixture conventions

- Stage: 0
- Status, priority, ownership: authoritative in [registry](../registry.json), initially BLOCKED, P1, unowned.
- Dependencies: BUILD-002
- Blocks: CORE-002, MATH-002
- External gates: None

## Objective
Establish reusable test registration and evidence conventions without production helpers.

## Allowed files/modules
- `tests/CMakeLists.txt`
- `tests/support/`
- `tests/fixtures/README.md`
- `spec/test-evidence.md`

Registry/report updates go through the coordinator. A new dependency or required edit
outside these paths needs a scoped follow-up or reviewed scope amendment first.

## Relevant specifications/interfaces
- [spec/architecture.md](../../spec/architecture.md)
- [spec/interfaces.md](../../spec/interfaces.md)
- [spec/testing.md](../../spec/testing.md)
- [CONTRIBUTING.md](../../CONTRIBUTING.md)
- [tasks/items/BUILD-002.md](../../tasks/items/BUILD-002.md)

Read the merged outputs of dependency tasks as well as these baseline references.

## Requirements and acceptance criteria
Register the test categories from spec/testing.md via existing build hooks; document fixture provenance and numeric assertions; demonstrate unit and contract category discovery with test-only sample data.

Every clause above requires evidence; the reviewer checks each in the completion report.

## Required tests/checks
Run test discovery and category filtering; demonstrate failure reports include expected/actual values and tolerance; validate one sample fixture metadata record.

Run existing affected checks and `python tools/tasks.py validate` as applicable.
For documentation tasks, record reviewed examples and checklist outcomes.

## Deliverables
Test harness helpers, category registration and fixture evidence guide.

## Notes/assumptions
Only the small Stage 1 scope is authorized. No speculative SimPHY behavior.
Contract tasks produce reviewed specifications; downstream implementation owns C++
headers and must conform to those specifications. Missing decisions are blockers,
not permission to assume another agent's unpublished work.

## Completion report
Initially incomplete. Coordinator records [report](../reports/TEST-001.md) before REVIEW,
using [template](../templates/completion.md). No report exists until work is submitted.
