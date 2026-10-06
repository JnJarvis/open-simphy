# MATH-002 — Implement minimal vector and transform operations

- Stage: 1
- Status, priority, ownership: authoritative in [registry](../registry.json), initially BLOCKED, P2, unowned.
- Dependencies: MATH-001, BUILD-002, TEST-001
- Blocks: SCENE-002
- External gates: None

## Objective
Implement only the vector/transform operations needed by the initial slice.

## Allowed files/modules
- `src/math/`
- `tests/unit/math/`
- `tests/contract/math/`

Registry/report updates go through the coordinator. A new dependency or required edit
outside these paths needs a scoped follow-up or reviewed scope amendment first.

## Relevant specifications/interfaces
- [spec/architecture.md](../../spec/architecture.md)
- [spec/interfaces.md](../../spec/interfaces.md)
- [spec/testing.md](../../spec/testing.md)
- [tasks/items/MATH-001.md](../../tasks/items/MATH-001.md)
- [tasks/items/BUILD-002.md](../../tasks/items/BUILD-002.md)
- [tasks/items/TEST-001.md](../../tasks/items/TEST-001.md)

Read the merged outputs of dependency tasks as well as these baseline references.

## Requirements and acceptance criteria
Conform to numeric policy; no scene/renderer includes; document invalid-number and zero-vector behavior.

Every clause above requires evidence; the reviewer checks each in the completion report.

## Required tests/checks
Run accepted math vectors including identity, rotation, transform composition, zero handling and nonfinite input; verify tolerances are explicit.

Run existing affected checks and `python tools/tasks.py validate` as applicable.
For documentation tasks, record reviewed examples and checklist outcomes.

## Deliverables
Minimal math module and numeric tests.

## Notes/assumptions
Only the small Stage 1 scope is authorized. No speculative SimPHY behavior.
Contract tasks produce reviewed specifications; downstream implementation owns C++
headers and must conform to those specifications. Missing decisions are blockers,
not permission to assume another agent's unpublished work.

## Completion report
Initially incomplete. Coordinator records [report](../reports/MATH-002.md) before REVIEW,
using [template](../templates/completion.md). No report exists until work is submitted.
