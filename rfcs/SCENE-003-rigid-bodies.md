# SCENE-003 — Compound rigid-body model

Status: PROPOSED for affected-contract review, 2026-10-08.

The current app opens source archives but cannot simulate their mechanical bodies.
A display circle on a particle is not a collision fixture. Introduce a separate
owning rigid-body document and snapshot, with a precise center-of-mass convention,
so compound fixtures and rotation survive translation. The exact producer contract
is [rigid-scene.md](../spec/contracts/rigid-scene.md).

Evidence: COMPAT-003's real archive inspection, research/FILE_FORMAT.md sections
5.1–5.5, and INT-004's polygon-centroid regression. Source polygons already express
vertices in body-local coordinates; LocalCenter is not an additional translation.
Body transforms and source angular velocities require explicit degree-to-radian
conversion. Syntax evidence does not establish solver or mass-mode equivalence.

## Affected-contract review

| Owner | Proposed effect / review checkpoint |
|---|---|
| scene | Add rigid.hpp/BodyDocument/BodySnapshot; keep particle Document/Snapshot unchanged. Validate ownership, IDs, compound fixtures and center-of-mass convention. |
| collision | Consume body-local circle/convex polygon geometry and immutable poses. Produce fixture-keyed manifolds; never integrate or alter documents. |
| physics | Add a separate rigid world; particle World's exact gravity update remains intact. Integrate center-of-mass position, retain body-origin pose, solve angular contacts. Collision/solver contract follows separately. |
| constraints | Reference body IDs and local anchors; transform anchors with body origin, torque lever arms with world center of mass. Future joint contract required. |
| renderer | Add a rigid snapshot entry point; actual transformed fixture geometry, not particle display radii. Particle frame byte contract stays intact. |
| editor | Body edits are transactions over BodyDocument; select/pick fixture geometry, translate/rotate all fixtures together. New history/picking contract required. |
| compat | Translate circles and validated convex polygons exactly; bake fixture geometry once. Explicit source mass/COM may map only under a reviewed translation profile. Unknown fields/modes/scripts prevent Play. |
| serialization | No reinterpretation of .osim v1. Body persistence requires a new native revision and migrations before Save is exposed. |
| app | Add rigid workspace via a later integration task after producers merge. Retain source archive and capability report; failed translation keeps previous state. |

No new dependency edges or third-party physics library are introduced by this
producer. Solver choice remains separately reviewable. A later solver must prove
resting contact, restitution, friction and angular momentum under stated tolerances;
a successful import or screenshot is not a numerical reference.

## Implementation sequence

1. Review this producer contract, then implement rigid document/snapshot validation
   with the listed independent geometry vectors.
2. Review and implement circle/convex contact queries and a rigid world, including
   stable resting contacts and reset. No source runs without capability assessment.
3. Add exact supported body translation and real-fixture rendering/editing in an
   integration task. Demonstrate a script-free body scene with collisions.
4. Add distance/revolute/spring and prismatic/line joints, then widen tested import
   profiles. Scripts, planes/rings, other domains and unsupported settings stay
   explicit until their behavior is implemented and verified.

Review is required before producer code under AGENTS.md's shared-interface rule.
This RFC does not claim rigid bodies or contacts already work.
