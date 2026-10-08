# SCENE-003 — Rigid body producer contract

Submitted REVIEW, 2026-10-08. Worker codex-worker; codex/scene-003, 8e461c5.

Concrete additive contract and affected-module RFC prepared for bodies, compound
circle/convex fixtures, center-of-mass versus body-origin pose, angular velocity,
explicit mass/inertia, sensors/filtering/materials, immutable document/snapshot
publication, validation budgets and deterministic identity. Includes independent
transform/contact-velocity/geometry/ownership/error vectors for producer tests.
Existing particle headers, update equations and .osim v1 remain intact.

Read README, task workflow/task/spec references and source-format evidence. Review
checked geometry conventions against the INT-004 centroid defect, SI/radian
math contract, public scene ownership and module DAG. Affected review table covers
scene/collision/constraints/physics/renderer/editor/compat/serialization/app.
No runtime implementation or fabricated solver test results. git diff --check PASS.

Changed: rfcs/SCENE-003-rigid-bodies.md, spec/contracts/rigid-scene.md.
Compare: https://github.com/JnJarvis/open-simphy/compare/master...codex/scene-003

Requires independent affected-contract approval before producer code. Follow-ups:
implement validated rigid model, review/implement contact manifolds and rigid
solver, then integrate exact supported SSIM body profiles. No claim that real
source simulations already run. Unknown source modes/settings remain unsupported.
