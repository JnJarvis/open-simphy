# INT-009 — Exact pieces and elastic constraints

Affected-contract review before consumers: scene owns additive DistanceLink spring
stiffness (N/m), damping ratio and optional minimum/maximum length bounds (m).
Rigid defaults preserve existing callers. SpringJoint is a Hooke spring using
explicit stiffness; distance frequency input derives stiffness from reduced mass.
Rope is a non-spring unilateral bounded distance, free inside its interval.
WeldLink owns COM-local anchors, B-A reference angle (radians), frequency (Hz) and
damping ratio; zero frequency is a rigid weld. Body_b zero is fixed world ground.
No dependency changes; the mature backend remains private to physics.

Compatibility decomposes bounded simple polygons deterministically into triangles,
retaining original occupied area rather than replacing them with a convex hull.
Remove duplicate closing/collinear vertices only when the occupied boundary is
unchanged. Reject crossing/touching non-neighbor edges, holes and degenerate pieces.
All triangles retain source material/filter/sensor; explicit mass/COM/inertia remain
unchanged. Rendering/picking consume the same owning fixture geometry.

Validation must precede backend creation; source scripts preserve authored elastic
parameters through collect/reset. Test analytic spring motion, damping and rope
slack, weld anchor/angle response, independent polygon coverage and notch collision.
This increment cannot silently omit other controllers, fields, widgets or domains.

Presentation uses one body-local texture frame across its pieces and omits shared
internal triangle edges. A decomposition does not repeat textures or draw false
internal boundaries. Spring links draw coils, welds mark their anchor.

Approved numerical public-API probe: SpringJoint(mA=mB=2,k=8,b=.6)
reports k=8, frequency .4501581581 and damping coefficient .6; a 1ms step with
relative velocity 1 changes velocities by -.0003,+.0003. Its frequency setter
leaves k unchanged. Weld angles .2,.6 give reference -.4, confirming source A-B.
Canonical damping_coefficient defaults -1 (use damping_ratio); nonnegative values
select direct N*s/m damping, preserving the source SpringJoint's coefficient.
Backend spring Hertz/damping refresh from anchor effective mass each microstep,
including rotational lever arms, so stiffness retains N/m off center.

An explicit spring flag distinguishes a zero-stiffness/free spring from a rigid rod.
Zero stiffness with direct damping is a pure axial damper: implicit impulse
J=-b*v_rel*dt/(1+b*inverse_effective_mass*dt), with equal opposite anchor impulses.
This avoids silently turning k=0 into a rigid constraint or ignoring its damping.

Real Body and joints text body has 79 authored fixtures. Extend per-body budget
64 ->256, retain aggregate4096, validate both source and canonical boundaries.
ParticleSystem/tracers are required behaviors, so the loader explicitly rejects
them until their runtime exists instead of accidentally playing only rigid bodies.

App drag must distinguish elastic links from rods: springs extend freely, slack
rope endpoints clamp to active distance bounds, rigid rods retain length. Native
--smoke-elastic exercises actual mouse events and validates both before playback.
