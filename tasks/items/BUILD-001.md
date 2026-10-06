# BUILD-001 — Select foundation toolchain and coding policy

- Stage: 0
- Status, priority, ownership: authoritative in [registry](../registry.json), initially READY, P1, unowned.
- Dependencies: None
- Blocks: REN-001, BUILD-002
- External gates: None

## Objective
Choose the smallest supported C++ build and test baseline with a reproducible acquisition policy.

## Allowed files/modules
- `spec/build-policy.md`
- `adr/BUILD-001-*.md`
- `rfcs/BUILD-001-*.md`

Coordinator scope amendment, 2026-10-06: include the technology-choice RFC required
by rfcs/README.md. The user authorized foundation development with Windows first
and straightforward later Linux/macOS support; the module dependency DAG is unchanged.

Registry/report updates go through the coordinator. A new dependency or required edit
outside these paths needs a scoped follow-up or reviewed scope amendment first.

## Relevant specifications/interfaces
- [spec/architecture.md](../../spec/architecture.md)
- [spec/interfaces.md](../../spec/interfaces.md)
- [spec/testing.md](../../spec/testing.md)
- [CONTRIBUTING.md](../../CONTRIBUTING.md)

Read the merged outputs of dependency tasks as well as these baseline references.

## Requirements and acceptance criteria
Record C++ standard, compiler/platform matrix, CMake minimum, test framework, dependency pinning, formatting, warnings and CI strategy; justify choices and list exact clean-checkout commands; resolve exception/error and naming rules consistently with core requirements.

Every clause above requires evidence; the reviewer checks each in the completion report.

## Required tests/checks
Review every choice against headless testing and dependency-license constraints; verify selected tool versions are available using authoritative documentation and record sources; no simulator build required.

Run existing affected checks and `python tools/tasks.py validate` as applicable.
For documentation tasks, record reviewed examples and checklist outcomes.

## Deliverables
Accepted toolchain ADR and build policy.

## Notes/assumptions
Only the small Stage 1 scope is authorized. No speculative SimPHY behavior.
Contract tasks produce reviewed specifications; downstream implementation owns C++
headers and must conform to those specifications. Missing decisions are blockers,
not permission to assume another agent's unpublished work.

## Completion report
Initially incomplete. Coordinator records [report](../reports/BUILD-001.md) before REVIEW,
using [template](../templates/completion.md). No report exists until work is submitted.
