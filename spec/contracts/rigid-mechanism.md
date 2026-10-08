# Rigid mechanism extension

SI, binary64 canonical values, +y up, radians. Fixture: circle(center,radius) or
strictly convex CCW polygon(3..8 vertices); material, sensor, category/mask per
fixture. Max 64 fixtures/body, 4096 total; local geometry <=1000. A body's COM-local
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
