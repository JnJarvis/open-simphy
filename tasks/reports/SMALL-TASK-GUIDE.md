# Strict implementation packet for a later worker

This is guidance, not an assignment. No model is authorized to self-assign blocked
work, approve its own changes, or take over an active claim. The user has not yet
switched workers. Coordinator selects an unclaimed READY task and fills the packet
below from merged contracts. Smaller models should implement bounded, fully specified
pieces; architecture, numeric guarantees, dependency decisions and final review stay
with the maintainer/current coordinator unless explicitly handed over.

## Coordinator fills every field before dispatch

- Exact task ID, owner identity, baseline commit, isolated branch/worktree.
- Allowed paths copied from registry; one concrete output objective.
- Required reads with exact accepted contract version/commit.
- Named functions/types and expected results; no missing design decisions.
- Test vector IDs, exact fixtures/units/tolerances and expected error codes.
- Exact configure/build/test/format commands and required native CI jobs.
- Known traps and what must remain invariant.
- Report destination, reviewer and explicit completion boundary (REVIEW, not DONE).

Do not divide one registered task among concurrent workers without coordinator
scoping separate IDs and paths first. Do not rely on another worker's unpublished code.

## Worker procedure

1. Read instructions and confirm registry READY/claim acknowledgement. Inspect git
   status; do not reset user changes. Verify baseline contains accepted dependencies.
2. Restate the objective, paths and vector list briefly. If any decision is missing,
   ask coordinator; do not guess, add a fallback or weaken a test.
3. Implement the smallest conforming value/function. No UI wiring, dependency updates,
   unrelated refactors, new public options or speculative generalized frameworks.
4. Test both normal and failure cases. Verify actual observations against independent
   expected data, not values copied from current implementation output.
5. Run required checks. A compile-only pass is incomplete. Record failures and fixes.
6. Inspect changed-path list; revert only your own unintended edits safely. Submit
   branch/commit/report. Do not merge, change claims, fake reviewer identity or mark DONE.

## Project-specific traps

- Fixed gravity uses OLD velocity in position drift. Whole step must fail without
  changing any particle if the last particle overflows. Never hide failure with clamp.
- Time is successful step count times fixed dt; no wall-clock reads in physics.
- Result<T> has const pointer accessors and deleted assignment. Do not edit core
  to make a downstream convenience compile; use a new local result/value.
- All input objects own data. A span/string_view is borrowed and cannot outlive it.
- Renderer owns projection/rasterization. App owns SDL window/events/presentation.
  No native handles or SDL headers in scene/math/physics or renderer public headers.
- Width/height/stride products need checked arithmetic. Do not cast huge floats to
  integers before finite/range checks. Texture destination pitch can exceed width*4.
- Respect approved pixel-center/alpha rules; no approximate screenshot assertions
  substituted for exact tiny raster examples. Do not invent REN-002 decisions.
- Failed tests cannot be skipped, tolerated or assigned looser bounds to pass.
- Tests-off/headless must not fetch SDL. Build scope belongs to BUILD-003, not renderer.

## Suitable later packets

After dependencies are accepted and coordinator scopes the task: add specified
edge-case tests, implement a single frozen mapping/validation helper, or write demo
instructions from verified commands. PHY-002 transactional state, renderer clipping/
blending, native lifecycle integration and any public contract change should receive
maintainer implementation or particularly close review. These suggestions do not
create new task IDs or authorize extra files.

## Required return format

Task / baseline / branch / commit; changed paths; vector-by-vector evidence;
commands and actual outputs/counts; failures fixed; skipped checks and reasons;
assumptions/limitations; follow-up proposals. State REVIEW requested. Stop dependent
work if blocked and report the exact unmet criterion to the coordinator.
