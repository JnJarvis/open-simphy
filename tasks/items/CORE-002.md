# CORE-002 — Implement stable ID and diagnostic values

- Stage: 1
- Status, priority, ownership: authoritative in [registry](../registry.json), initially BLOCKED, P2, unowned.
- Dependencies: CORE-001, BUILD-002, TEST-001
- Blocks: SCENE-002
- External gates: None

## Objective
Implement only the accepted core value primitives.

## Allowed files/modules
- `src/core/`
- `tests/unit/core/`
- `tests/contract/core/`

Registry/report updates go through the coordinator. A new dependency or required edit
outside these paths needs a scoped follow-up or reviewed scope amendment first.

## Relevant specifications/interfaces
- [spec/architecture.md](../../spec/architecture.md)
- [spec/interfaces.md](../../spec/interfaces.md)
- [spec/testing.md](../../spec/testing.md)
- [tasks/items/CORE-001.md](../../tasks/items/CORE-001.md)
- [tasks/items/BUILD-002.md](../../tasks/items/BUILD-002.md)
- [tasks/items/TEST-001.md](../../tasks/items/TEST-001.md)

Read the merged outputs of dependency tasks as well as these baseline references.

## Requirements and acceptance criteria
Match core contract; keep allocation scope explicit and expose no mutable global service; supply module build registration under existing conventions.

Every clause above requires evidence; the reviewer checks each in the completion report.

## Required tests/checks
Run all core contract vectors; test valid/invalid and duplicate IDs according to allocation policy, structured error propagation and value ownership.

Run existing affected checks and `python tools/tasks.py validate` as applicable.
For documentation tasks, record reviewed examples and checklist outcomes.

## Deliverables
Core public headers, implementation if needed, unit and conformance tests.

## Notes/assumptions
Only the small Stage 1 scope is authorized. No speculative SimPHY behavior.
Contract tasks produce reviewed specifications; downstream implementation owns C++
headers and must conform to those specifications. Missing decisions are blockers,
not permission to assume another agent's unpublished work.

## Completion report
Initially incomplete. Coordinator records [report](../reports/CORE-002.md) before REVIEW,
using [template](../templates/completion.md). No report exists until work is submitted.
