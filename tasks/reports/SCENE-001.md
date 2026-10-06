# SCENE-001 submission

Owner: codex-worker. Coordinator: codex-coordinator. Date: 2026-10-06.
Branch: task/SCENE-001. Implementation commit: 11da2ec.
Status: DONE; user approved and contract merged.

## Deliverable and scope

Only [spec/contracts/scene.md](../../.worktrees/SCENE-001/spec/contracts/scene.md)
changed. Defines particle authoring documents, atomic commands, immutable runtime
snapshots and backend-neutral drawing packets. Uses accepted core/math contracts;
adds no external dependency, solver implementation or file encoding.

## Author acceptance checklist

- IDs, mass, position/velocity, gravity and time: fields, units and finite-value
  requirements explicitly stated.
- Validation: duplicate/zero IDs, high-water marks, invalid mass/radius, unsupported
  revisions and nonfinite fields have diagnostic rules.
- Editing: add/replace/remove/set-gravity are atomic; failures consume no IDs.
- Ordering/reset: ascending EntityId order, retained high-water after removal,
  time-zero reset from the app-retained starting document.
- Snapshot isolation: owning immutable values survive producer changes/destruction.
- Presentation: circles/lines in world units with validated RGBA; no native types,
  graphics resources or solver caches.
- Conformance examples S01-S21 cover valid/invalid scenes, duplicate IDs, nonfinite
  data, invalid mass, command failure, exhaustion, snapshot isolation, reset and
  world-coordinate packets. Reviewed by author against the stated rules.

## Checks and limitations

`python tools/tasks.py validate`: PASS, 17 tasks. Existing workflow unittest suite:
PASS, 6 tests. `git diff --cached --check`: PASS. Documentation-only task: examples
are specification vectors, not claims of passing C++ runtime tests. Independent
affected-contract review is pending before any consumer may implement this proposal.

Assumption: the approved first slice contains free particles only. Particle radius
is presentation data, not collision geometry. Future entity kinds require reviewed
contract extensions. PHY/INT will verify the runtime reset lifecycle; SCENE-002
will own document/snapshot implementation and conformance tests after prerequisites.

## Acceptance and merge

Independent reviewer: user. Approval: "looks good" in response to the explicit
scene contract approval request and comparison link. Accepted commit: 11da2ec.
Merged baseline: 0c793b164b95f4029df8292e469e0598e4ebb18d.
This acceptance supersedes pending-review statements above. Contract status metadata
was updated to ACCEPTED without changing its semantics.
