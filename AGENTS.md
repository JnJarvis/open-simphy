# Agent operating rules

This is initially a planning repository. Do not implement simulator functionality
as part of the initial planning phase. Future explicitly assigned backlog tasks
authorize only their specified implementation scope.

Read README.md, tasks/README.md, the assigned task, and its referenced specs before
editing. Select only READY tasks and claim through the canonical coordinator.
Use an isolated branch/checkout per task; the registry, claims, and completion
reports are owned by the coordinator, not copied back from worker branches.

- Stay inside the task's allowed paths. No unrelated fixes or opportunistic cleanup.
- Follow spec/architecture.md dependency boundaries. Document new dependencies.
- Shared interface changes require an RFC and affected-contract review. Never
  silently change headers, data semantics, file schemas, or test expectations.
- Use agreed contracts and local test doubles, never another agent's unfinished code.
- Add the task's required tests and keep existing applicable tests passing.
  Compilation alone is not DONE. Do not invent test results.
- Record assumptions, commands, results, limitations, and changed paths.
- Create a follow-up proposal in the completion report for discovered work rather
  than expanding scope. Coordinator assigns IDs and updates the registry.
- If blocked, stop dependent work, record the blocker, and ask the coordinator to
  move the task to BLOCKED. Do not weaken an acceptance criterion to unblock it.
- Only integration tasks own src/app/. Feature tasks do not wire the application.
- Submit implementation and documentation for REVIEW. DONE requires review,
  required checks, and merge into the shared baseline, not just a local commit.
- Never take over an abandoned claim automatically. Coordinator verifies the old
  worker has stopped before reassignment. Avoid concurrent writes to shared paths.

No proprietary SimPHY binaries or unlicensed fixtures belong in this repository.
Do not reverse engineer SimPHY during foundation work; await supplied research.
