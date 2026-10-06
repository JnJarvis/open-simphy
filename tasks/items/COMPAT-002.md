# COMPAT-002 — Normalize supplied SimPHY capability research

- Stage: 0
- Status, priority, ownership: authoritative in [registry](../registry.json), initially BLOCKED, P1, unowned.
- Dependencies: COMPAT-001
- Blocks: No initial tasks
- External gates: research_available

## Objective
Turn the externally supplied map into traceable requirements and follow-up proposals.

## Allowed files/modules
- `research/normalized/`
- `research/requirements.json`
- `research/intake-report.md`

Registry/report updates go through the coordinator. A new dependency or required edit
outside these paths needs a scoped follow-up or reviewed scope amendment first.

## Relevant specifications/interfaces
- [spec/architecture.md](../../spec/architecture.md)
- [spec/interfaces.md](../../spec/interfaces.md)
- [spec/testing.md](../../spec/testing.md)
- [spec/research-intake.md](../../spec/research-intake.md)
- [tasks/items/COMPAT-001.md](../../tasks/items/COMPAT-001.md)

Read the merged outputs of dependency tasks as well as these baseline references.

## Requirements and acceptance criteria
Requires research_available gate; preserve provenance/version/uncertainty, separate syntax/model/behavior observations, flag contradictions, and propose small implementation tasks only for supported findings.

Every clause above requires evidence; the reviewer checks each in the completion report.

## Required tests/checks
Check unique requirement IDs and resolvable finding/fixture references; review uncertain and unsupported examples; ensure no PASS entries lack implementation and test evidence.

Run existing affected checks and `python tools/tasks.py validate` as applicable.
For documentation tasks, record reviewed examples and checklist outcomes.

## Deliverables
Normalized research matrix and scoped task proposals; no compatibility code.

## Notes/assumptions
Only the small Stage 1 scope is authorized. No speculative SimPHY behavior.
Contract tasks produce reviewed specifications; downstream implementation owns C++
headers and must conform to those specifications. Missing decisions are blockers,
not permission to assume another agent's unpublished work.

## Completion report
Initially incomplete. Coordinator records [report](../reports/COMPAT-002.md) before REVIEW,
using [template](../templates/completion.md). No report exists until work is submitted.
