# Task workflow

Registry JSON is authoritative for status, priority, owner, dependencies, blockers,
and scope. Markdown describes work and acceptance. IDs are immutable, never reused.
Prefixes: CORE, MATH, SCENE, PHY, COLL, CONSTR, REN, EDT, VIS, ANA, IO, COMPAT,
PLAT, TEST, BUILD, INT. P0 is urgent, P1 foundation, P2 normal, P3 optional.

## Status meanings

| Status | Meaning |
|---|---|
| BACKLOG | Not sufficiently scoped/approved to schedule; dependency completion alone does not promote it |
| BLOCKED | Scoped task missing a merged prerequisite, external gate, or explicit blocker |
| READY | Fully scoped; every dependency DONE and merged, every gate satisfied, no explicit blocker; independently completable |
| IN_PROGRESS | Claimed by one owner in canonical registry; implementation underway |
| REVIEW | Owner submitted deliverables, report and required test evidence; awaiting reviewer/merge |
| DONE | Reviewer verified every acceptance criterion, tests and documentation; outputs merged into shared baseline |

Permitted transitions: BACKLOG → BLOCKED/READY after scoping review; BLOCKED → READY
on refresh only when all gates clear; READY → IN_PROGRESS by claim; IN_PROGRESS →
REVIEW by submission; REVIEW → DONE by reviewer after merge; REVIEW → IN_PROGRESS
for corrections; any unfinished task → BLOCKED with reason. DONE is immutable;
regressions create follow-ups, or coordinator explicitly reopens it and invalidates
dependent readiness. Never refresh an in-progress task out from under its worker.

## Coordination and independent work

Maintainer appoints one coordinator/reviewer and records its identity via
`python tools/tasks.py coordinator <identity>` in the canonical checkout. Any agent
can fill this role. The initial four READY tasks require no unresolved implementation
decision; claiming still requires a configured coordinator.

All claims/status writes must run against this one checkout. A directory lock
serializes updates and atomic replacement prevents partial JSON writes. It is not
a distributed lock across clones. Remote agents send claim requests to the designated
coordinator, who runs the same command and acknowledges ownership before work starts.
Workers use separate worktrees/branches from the merged baseline; do not concurrently
edit a shared source checkout. Keep registry and report commits coordinator-owned.

Example:

```text
python tools/tasks.py ready
python tools/tasks.py claim CORE-001 agent-a
# agent implements within allowed scope and submits branch + completion evidence
# coordinator stores report as tasks/reports/CORE-001.md
python tools/tasks.py review CORE-001 agent-a
# reviewer verifies and merges outputs; update report with review and merged commit
python tools/tasks.py done CORE-001 reviewer-b <merged-commit>
python tools/tasks.py ready
```

DONE automatically refreshes newly unblocked tasks. The tool validates dependency
cycles, reverse edges and missing specification/task files. Human/agent review
still verifies semantics, scope, evidence, and merge ancestry; the script is not an
authorization service or a substitute for review. Do not pass a fabricated commit.

Reports are required before REVIEW. The reviewer must differ from the worker.
The designated coordinator executes DONE on the reviewer's behalf if needed.
Registry ownership persists through REVIEW and DONE for auditability.

Use `block ID reason` for discovered blockers and `unblock ID` only after recording
resolution evidence in the task/report. Use `resume ID` for review corrections.
For abandonment, coordinator confirms worker stopped, uses `block`, then edits owner
to null under the coordination lock before clearing the blocker. There is no timed
claim expiry. A crashed tool may leave tasks/.registry-lock; inspect the holder and
verify no writer remains before manually removing it. Never delete a live lock.

Only the coordinator may scope new tasks, edit dependencies/gates, or amend paths.
For administrative edits, create the same lock directory, edit the registry, run
validation, then remove the lock. Recompute blocks when dependencies change.
`gate research_available <evidence-path>` requires a real supplied evidence file
and intake review, not a placeholder. Readiness refresh never invents research.

Tasks must normally fit one reviewable change. Split broad work before READY.
App-wide wiring always gets an INT task. Shared interface work always precedes
consumers. Two tasks with overlapping writable scope must not be claimed concurrently;
split their paths or serialize them. Initial scopes are deliberately disjoint.

Do not start a task until every required dependency artifact is accessible in the
worker's baseline. No cherry-picking unreviewed work to evade a BLOCKED state.
