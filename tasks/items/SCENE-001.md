# SCENE-001 — Specify minimal document and snapshot contracts

- Stage: 0
- Status, priority, ownership: authoritative in [registry](../registry.json), initially BLOCKED, P1, unowned.
- Dependencies: CORE-001, MATH-001
- Blocks: PHY-001, REN-002, SCENE-002
- External gates: None

## Objective
Define minimal particle authoring, runtime snapshots and presentation data independently of file formats.

## Allowed files/modules
- `spec/contracts/scene.md`

Registry/report updates go through the coordinator. A new dependency or required edit
outside these paths needs a scoped follow-up or reviewed scope amendment first.

## Relevant specifications/interfaces
- [spec/architecture.md](../../spec/architecture.md)
- [spec/interfaces.md](../../spec/interfaces.md)
- [spec/testing.md](../../spec/testing.md)
- [tasks/items/CORE-001.md](../../tasks/items/CORE-001.md)
- [tasks/items/MATH-001.md](../../tasks/items/MATH-001.md)

Read the merged outputs of dependency tasks as well as these baseline references.

## Requirements and acceptance criteria
Specify IDs, mass, position/velocity, gravity, time, validation and edit atomicity; define immutable snapshot and simple draw packets; no solver caches or backend types; document ID ordering and reset semantics.

Every clause above requires evidence; the reviewer checks each in the completion report.

## Required tests/checks
Review valid/invalid scenes, duplicate IDs, nonfinite fields, invalid mass, snapshot isolation and coordinate examples; supply conformance vectors for each rule.

Run existing affected checks and `python tools/tasks.py validate` as applicable.
For documentation tasks, record reviewed examples and checklist outcomes.

## Deliverables
Reviewed scene schema/command/snapshot contract and test vectors.

## Notes/assumptions
Only the small Stage 1 scope is authorized. No speculative SimPHY behavior.
Contract tasks produce reviewed specifications; downstream implementation owns C++
headers and must conform to those specifications. Missing decisions are blockers,
not permission to assume another agent's unpublished work.

## Completion report
Initially incomplete. Coordinator records [report](../reports/SCENE-001.md) before REVIEW,
using [template](../templates/completion.md). No report exists until work is submitted.
