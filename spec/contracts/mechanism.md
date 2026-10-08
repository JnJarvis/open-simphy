# Circular mechanism contract — experimental v1

Reviewed for the user-authorized INT-005 continuation before consumers. Independent
of SimPHY; additive to particle v1 and broader proposed SCENE-003 body producer.
Units are binary64 SI/+y up/radians. Circle.center is world COM and fixture center;
this profile only accepts one centered circle per body. Import bakes source pose
and requires fixture center=source local COM. Fixed-angular mobility maps to a
circle with fixed_rotation; source angular motion must be negligible in that mode.
Static fixtureless source anchors become canonical ground endpoints, not particles.

scene::Mechanism contains explicit gravity, fixed_dt, vector<CircleBody>,
vector<DistanceLink>. CircleBody has canonical EntityId/name, center/velocity,
angle/angular_velocity, radius/mass/inertia, friction/restitution/damping,
gravity_scale, fixed_rotation/static_body, sensor and uint64 category/mask bits.
DistanceLink IDs are nonzero/unique, body_a required, body_b=0 denotes fixed ground;
local_a/local_b are COM-local anchors (local_b world coordinates for ground),
length is explicit >=.01 meters, above the backend minimum, collide_connected is explicit.

validate(Mechanism) returns Result<void>. Maximum 256 circles/1024 links; all values
finite; position/velocity/local anchors <=10000 absolute, radius in [1e-4,1000],
dynamic mass>=1e-6/inertia>=1e-9 and both <=1e12, materials friction>=0/restitution in[0,1],
damping>=0, abs(gravity_scale)<=100, dt in [1/1000,1/30]. IDs unique in each
collection, references valid and distinct, at least one endpoint dynamic, angle bounded abs<=1e6, angular speed
abs<=1000. Static velocity must be zero. Body and constraint order retained;
canonical IDs deterministically assigned by source traversal/creation order.
No guessed force/controller/unknown-body-mode translation. Failure never publishes
an invalid definition. Source settings not reproduced exactly are explicitly reported.

physics::Mechanism owns a private Box2D3.1.1 solver; create validates and converts
to float only at this backend boundary. It uses fixed dt split into eight complete world microsteps (each refreshes contacts
and restitution), no sleep,
continuous collision handling, explicit imported mass/inertia and circle materials.
Float backend is intentional and not a change to canonical binary64 values.
Snapshot owns time and circle samples (IDs/center/velocity/angle/angular velocity).
step advances once; reset reconstructs the retained definition; relocate(id,world
center) stops that body's velocity and rejects missing/static bodies/nonfinite or
out-of-envelope points before mutation. A failed step poisons the runtime until
reset; no invalid snapshot is published. The accepted particle value World remains
unchanged. No clock/files/toolkit types in scene or physics APIs.

compat::MechanicalSource is a staged owning source interpreter with definition,
styles/assets/widgets/camera and apply_action. Script bridge supports Vector2,
Color, World.getBody/createCopy/clear/addDistanceJoint, fill/position/velocity
access and joint colors. Unknown API/error/time/budget violation fails without
replacing the app's active world. GUI supports textareas/buttons with script actions;
no evaluation of arbitrary paths or host I/O. inspect_ssim continues never to run
scripts. MechanicalSource construction is a separate explicit app composition step.
Memory64MiB, stack512KiB, execution100ms per eval, source script <=1MiB,
asset <=16MiB, decoded image <=4096x4096/16Mi pixels, aggregate images16Mi pixels.
Only referenced image assets loaded. Unknown actions fail and require reopening
before retry, preserving previous published physics/visuals. Audio and event
callbacks remain unsupported and conspicuous in the experimental status.

Required independent evidence: analytic gravity at zero damping; equal-mass elastic
circle velocity transfer; constrained pendulum length/energy bounds; reset exact
initial publication; drag/release; script-generated bodies/colors/constraints;
unknown API/loop timeout/over-limit rollback; assets/gui actions; actual seven-file
corpus classification and native visual comparison without importing fixtures into Git.

INT-008 reviewed additive extension: explicit fixture vector overrides geometry/material/filter for general rigid bodies; empty retains the centered circle contract. See rigid-mechanism.md and rfcs/INT-008-rigid-mechanics.md. Snapshots retain COM pose/velocity semantics. Particle/native-v1 contracts remain unchanged.
