# CORE-001 submission

Owner: codex-worker. Coordinator: codex-coordinator. Date: 2026-10-06.
Branch: task/CORE-001. Implementation commit: 519d15f.
Status: submitted for independent review; not frozen or merged.

Deliverable/only changed path:
[spec/contracts/core.md](../../.worktrees/CORE-001/spec/contracts/core.md).

## Acceptance evidence

- ID width/sentinel/range: unsigned 64-bit, zero invalid, max explicitly specified.
- Uniqueness/allocation: document-scoped monotonic high-water allocator, explicit
  reserve/overflow behavior, no global state; scene owns duplicate validation.
- Diagnostics: stable spelled codes, severity, owned UTF-8 text/entity/field context;
  no OS types or logging dependency.
- Results: exclusive success/failure, safe inactive-branch queries, void success,
  ownership/lifetimes, copy/move behavior and expected versus exceptional failure.
- Threading: allocator confined to document owner; immutable values freely copied.
- Examples: C01–C14 map identity, overflow, reserve, duplicate responsibility,
  propagation, lifetime, result state and invalid severity to required tests.

## Checks and results

Author review of C01–C08: zero sentinel and max overflow cannot be confused with
valid IDs; high-water never rewinds; repeated reserve is deliberately not duplicate
validation; independent documents do not share a hidden allocator. C09–C14 cover
zero payload versus invalid ID, safe inactive access, lifetime and preserved errors.
This is specification review by the author, not independent acceptance.

- `python tools/tasks.py validate`: PASS, 17 tasks.
- `python -m unittest discover -s tests -p test_task_workflow.py -v`: PASS, 6 tests.
- `git diff --cached --check` in task checkout before commit: PASS.

No C++ implementation/test results claimed. CORE-002 will implement executable
conformance tests and public-header checks against the reviewed contract.

Assumptions: in-process value semantics; no stable binary ABI; external UUIDs need
an explicit compatibility mapping; diagnostics are data, never UI services.
Follow-up: scene must enforce uniqueness across entity kinds and retain allocator
state appropriately; those are existing scene-task responsibilities.

Independent reviewer/outcome: pending.
Merged baseline commit: none; branch is available for review.
