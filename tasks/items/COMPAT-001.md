# COMPAT-001 — Prepare research intake templates

- Stage: 0
- Status, priority, ownership: authoritative in [registry](../registry.json), initially READY, P1, unowned.
- Dependencies: None
- Blocks: COMPAT-002
- External gates: None

## Objective
Prepare empty evidence and requirement templates so future research can be consumed without inventing observations.

## Allowed files/modules
- `research/templates/`
- `research/README.md`

Registry/report updates go through the coordinator. A new dependency or required edit
outside these paths needs a scoped follow-up or reviewed scope amendment first.

## Relevant specifications/interfaces
- [spec/architecture.md](../../spec/architecture.md)
- [spec/interfaces.md](../../spec/interfaces.md)
- [spec/testing.md](../../spec/testing.md)
- [spec/research-intake.md](../../spec/research-intake.md)

Read the merged outputs of dependency tasks as well as these baseline references.

## Requirements and acceptance criteria
Include source/version/platform/confidence, permission/hash, contradictory findings, requirement-task-test links and matrix state; include one clearly synthetic structural example only.

Every clause above requires evidence; the reviewer checks each in the completion report.

## Required tests/checks
Walk a synthetic finding through requirement, task and test references; verify UNKNOWN cannot be confused with PASS and provenance fields are present.

Run existing affected checks and `python tools/tasks.py validate` as applicable.
For documentation tasks, record reviewed examples and checklist outcomes.

## Deliverables
Finding, fixture and requirement templates plus intake instructions.

## Notes/assumptions
Only the small Stage 1 scope is authorized. No speculative SimPHY behavior.
Contract tasks produce reviewed specifications; downstream implementation owns C++
headers and must conform to those specifications. Missing decisions are blockers,
not permission to assume another agent's unpublished work.

## Completion report
Initially incomplete. Coordinator records [report](../reports/COMPAT-001.md) before REVIEW,
using [template](../templates/completion.md). No report exists until work is submitted.
