# MATH-001 — Specify 2D numeric contracts

- Stage: 0
- Status, priority, ownership: authoritative in [registry](../registry.json), initially READY, P1, unowned.
- Dependencies: None
- Blocks: MATH-002, SCENE-001
- External gates: None

## Objective
Define the initial vector and transform operations without choosing a third-party math library.

## Allowed files/modules
- `spec/contracts/math.md`

Registry/report updates go through the coordinator. A new dependency or required edit
outside these paths needs a scoped follow-up or reviewed scope amendment first.

## Relevant specifications/interfaces
- [spec/architecture.md](../../spec/architecture.md)
- [spec/interfaces.md](../../spec/interfaces.md)
- [spec/testing.md](../../spec/testing.md)

Read the merged outputs of dependency tasks as well as these baseline references.

## Requirements and acceptance criteria
Specify scalar representation, SI/radian conventions, coordinate handedness, finite-value handling, zero normalization, transform order, equality/tolerance policy and required minimal operations.

Every clause above requires evidence; the reviewer checks each in the completion report.

## Required tests/checks
Work through zero vector, axis rotation, composition order and invalid-number examples; record exact expected values or justified tolerances.

Run existing affected checks and `python tools/tasks.py validate` as applicable.
For documentation tasks, record reviewed examples and checklist outcomes.

## Deliverables
Reviewed math contract and test vectors.

## Notes/assumptions
Only the small Stage 1 scope is authorized. No speculative SimPHY behavior.
Contract tasks produce reviewed specifications; downstream implementation owns C++
headers and must conform to those specifications. Missing decisions are blockers,
not permission to assume another agent's unpublished work.

## Completion report
Initially incomplete. Coordinator records [report](../reports/MATH-001.md) before REVIEW,
using [template](../templates/completion.md). No report exists until work is submitted.
