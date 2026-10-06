# CORE-001 — Specify IDs and diagnostics contracts

- Stage: 0
- Status, priority, ownership: authoritative in [registry](../registry.json), initially READY, P1, unowned.
- Dependencies: None
- Blocks: CORE-002, SCENE-001
- External gates: None

## Objective
Freeze a small language-neutral contract for identifiers and structured errors before consumer code exists.

## Allowed files/modules
- `spec/contracts/core.md`

Registry/report updates go through the coordinator. A new dependency or required edit
outside these paths needs a scoped follow-up or reviewed scope amendment first.

## Relevant specifications/interfaces
- [spec/architecture.md](../../spec/architecture.md)
- [spec/interfaces.md](../../spec/interfaces.md)
- [spec/testing.md](../../spec/testing.md)

Read the merged outputs of dependency tasks as well as these baseline references.

## Requirements and acceptance criteria
Define ID width and invalid value, uniqueness scope, allocation ownership, diagnostic codes/severity/context, result semantics and threading/lifetime rules; include success and failure examples without a logging framework.

Every clause above requires evidence; the reviewer checks each in the completion report.

## Required tests/checks
Review invalid/duplicate ID examples and error propagation examples; map every invariant to a conformance test vector.

Run existing affected checks and `python tools/tasks.py validate` as applicable.
For documentation tasks, record reviewed examples and checklist outcomes.

## Deliverables
Reviewed core contract and embedded test vectors.

## Notes/assumptions
Only the small Stage 1 scope is authorized. No speculative SimPHY behavior.
Contract tasks produce reviewed specifications; downstream implementation owns C++
headers and must conform to those specifications. Missing decisions are blockers,
not permission to assume another agent's unpublished work.

## Completion report
Initially incomplete. Coordinator records [report](../reports/CORE-001.md) before REVIEW,
using [template](../templates/completion.md). No report exists until work is submitted.
