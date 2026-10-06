# SCENE-002 — Implement particle document validation and snapshots

- Stage: 1
- Status, priority, ownership: authoritative in [registry](../registry.json), initially BLOCKED, P2, unowned.
- Dependencies: SCENE-001, CORE-002, MATH-002
- Blocks: PHY-002, REN-003
- External gates: None

## Objective
Implement the canonical Stage 1 data types, validation and snapshot value semantics.

## Allowed files/modules
- `src/scene/`
- `tests/unit/scene/`
- `tests/contract/scene/`

Registry/report updates go through the coordinator. A new dependency or required edit
outside these paths needs a scoped follow-up or reviewed scope amendment first.

## Relevant specifications/interfaces
- [spec/architecture.md](../../spec/architecture.md)
- [spec/interfaces.md](../../spec/interfaces.md)
- [spec/testing.md](../../spec/testing.md)
- [tasks/items/SCENE-001.md](../../tasks/items/SCENE-001.md)
- [tasks/items/CORE-002.md](../../tasks/items/CORE-002.md)
- [tasks/items/MATH-002.md](../../tasks/items/MATH-002.md)

Read the merged outputs of dependency tasks as well as these baseline references.

## Requirements and acceptance criteria
Implement accepted minimal schema and command atomicity; no persistence or physics runtime; validation must report object-specific diagnostics.

Every clause above requires evidence; the reviewer checks each in the completion report.

## Required tests/checks
Run scene contract vectors: valid document, duplicate IDs, nonfinite fields, invalid mass, failed edit leaves original intact, copied snapshot remains unchanged.

Run existing affected checks and `python tools/tasks.py validate` as applicable.
For documentation tasks, record reviewed examples and checklist outcomes.

## Deliverables
Canonical scene module and validation/contract tests.

## Notes/assumptions
Only the small Stage 1 scope is authorized. No speculative SimPHY behavior.
Contract tasks produce reviewed specifications; downstream implementation owns C++
headers and must conform to those specifications. Missing decisions are blockers,
not permission to assume another agent's unpublished work.

## Completion report
Initially incomplete. Coordinator records [report](../reports/SCENE-002.md) before REVIEW,
using [template](../templates/completion.md). No report exists until work is submitted.
