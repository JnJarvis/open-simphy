# SCENE-003 — Compound rigid-body producer contract

User prioritizes real SSIM simulation over particle-only architecture. Define the
next canonical body model precisely before its consumers. Keep the accepted
particle APIs and .osim v1 schema intact. No app wiring or solver implementation.

Allowed: rfcs/SCENE-003-rigid-bodies.md; spec/contracts/rigid-scene.md.
Dependencies: COMPAT-004, SCENE-002, PHY-002. Read the task spec_refs plus README
and tasks/README. Include an affected-contract review for scene, collision,
constraints, physics, renderer, editor, serialization and compatibility.

Define units, mass/inertia and center-of-mass convention, circle/convex geometry,
compound fixtures, filters/sensors/materials, ownership, validation, failure
atomicity, deterministic IDs, immutable runtime samples and numeric limits.
Keep unknown source mass modes/settings unsupported. Define required independent
geometry and analytic vectors for later implementation. Submit review; producer
implementation follows approval rather than silently replacing particle contracts.
