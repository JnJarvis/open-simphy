# Rigid mechanism extension

SI, binary64 canonical values, +y up, radians. Fixture: circle(center,radius) or
strictly convex CCW polygon(3..8 vertices); material, sensor, category/mask per
fixture. Max 256 fixtures/body, 4096 total; local geometry <=1000. A body's COM-local
fixture values preserve its explicit mass/inertia and rotation. Existing empty
fixture vector means one centered circle. Hinge: COM-local anchors, optional angular
limits/motor; body_b zero denotes ground. Winding: signed radii and endpoint anchors,
no-slip cable constraint with equal/opposite linear impulses and angular reactions.
Limits: existing 256 bodies/1024 total joints and numeric envelopes apply.

Runtime force command is world vector plus COM-local point; wrapped forces use a
constant signed torque arm independent of body angle. Material command updates
all fixtures of the addressed body. Both validate entirely before mutation.
Expression evaluation is compat-only, solver has no script/UI dependencies.
Nested source dialog/panel layouts expose text/button/slider controls; numeric
sliders bind their named scalar, and friction expressions update contacts live.

MaterialMixer selects geometric mean/minimum/maximum independently for friction and restitution. Defaults retain the original circle backend. Import preserves coeffMixer indices 0/1/2.

INT-009 additive constraints: DistanceLink stiffness defaults zero (rigid rod),
damping_ratio defaults zero. Positive stiffness enables a Hooke spring, with damping
2*zeta*sqrt(k*reduced_mass). Optional limit/minimum (including zero)/maximum allows slack rope bounds;
limits disabled by default. WeldLink fixes coincident anchors and relative B-A angle;
frequency zero is rigid, positive Hz softens rotation with damping_ratio. Total joint
budget remains 1024 including welds. Simple source polygons are decomposed into
area-preserving convex fixtures before canonical validation; canonical polygons
remain strictly convex 3..8, with existing fixture budgets. No hull approximation.

DistanceLink damping_coefficient=-1 selects the ratio; nonnegative N*s/m values
override the ratio. Source SpringJoint uses the direct coefficient, even though
its saved field is named DampingRatio; the public API numeric vector is in INT-009
RFC/report. Effective anchor mass includes rotational lever arms each microstep.

DistanceLink spring=false preserves rigid defaults. Explicit spring=true allows
zero stiffness (free link), optionally with direct viscous damping. Positive
stiffness enables spring behavior even when the flag is false.

INT-010 additive ports and slide constraints are reviewed in
rfcs/INT-010-driven-mechanics.md. SlideLink distinguishes free rotation line
constraints from rotation locked prismatic constraints, preserving COM-local
anchors and A-local unit axis. BodyUpdate validates an entire batch before changing
any bodies, retaining the world and its constraints. set_time changes only the
finite nonnegative simulation clock. Reset retains its original definition.
