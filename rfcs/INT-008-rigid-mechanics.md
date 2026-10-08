# INT-008 — Rigid fixture and friction-control extension

User-authorized compatibility extension, 2026-10-08. Affected-contract review before
consumers: scene validates geometry and owns values; physics privately owns Box2D;
compat interprets bounded source data and expressions; app alone connects controls,
snapshots and rendering. No new dependency edges, no native-v1 schema change.

Extend Mechanism additively: CircleBody retains its centered-circle meaning when
fixtures is empty. Explicit fixtures override only geometry/material/filter; in
that profile radius is a conservative picking bound, center remains COM. All
fixture coordinates are COM-local. Existing circle callers and snapshots retain
behavior. New hinge and winding constraints have owning SI/radian values.
This experimental extension does not replace the broader proposed rigid document.

Forces and per-body material updates are validated runtime commands, applied before
each fixed step without reconstructing the world. Slider expressions run in the
existing bounded JS context with no host I/O. Pose/reset actions remain staged and
reconstruct a validated runtime. Unsupported domains and expressions still fail.

Evidence: real Static and Kinetic Friction archive, supplied PHYSICS_2D/SCRIPTING/
GUI_WIDGETS; public SimPHY force-point documentation; user-approved public API and
numerical probes. No proprietary implementation or fixtures are copied. Winding
constraint uses a physical no-slip cable velocity constraint and torque reaction,
not a distance rod. Source winding radii derive from signed anchor lever arms;
reference probe at centers (0,0),(3,0), anchors (0,0),(3,1) gives radius2
-0.9486832980505138 and response va=(-.3103448276,-.1034482759),
vb=(.3103448276,.1034482759), wb=.6896551724 for initial wb=1.
Backend tolerances/solver are independently selected; exact SimPHY numeric parity
is not claimed. Planes are finite static half-space boxes within supported bounds.

Review checklist: existing initial-circle/reset semantics preserved; convex input
validated before backend hull creation; per-fixture material/filter retained; source
COM offset baked once; required controller behavior never silently omitted; failed
imports/actions retain the active world. Tests must establish these before REVIEW.

Native evidence found a 7555x4087 JPEG (86.jpg) in the real friction archive.
App image policy reviewed before the decoder change: <=16MiB encoded assets,
<=8192 per dimension and <=32Mi source pixels, decoded sequentially with bounded
transient buffers; retain <=2048 per side and <=16Mi aggregate pixels. Bilinear
resampling is app-private and does not alter source bytes, canonical physics or
scene contracts. A failed image still leaves the previous app document intact.

Hinge sign probe: a.angle=.2,b.angle=.6 gives source reference=-.4 and
angle=0; setting reference=0 gives angle=-.4. A positive source motor speed 2
produces omega_a=1,omega_b=-1 for equal inertias. Canonical Box2D angle is B-A,
so import negates reference and motor speed and maps limits [lo,hi] to [-hi,-lo].

COM pose probe: local COM (.5,0), setRotation(pi/2) retains world COM(.5,0)
and changes origin to(.5,-.5); setPosition(2,3) sets world COM(2,3), origin(2,2.5).
Import XML origin+rotated local COM once; script setPosition addresses COM and
setRotation preserves COM. Add independent fixture/pose conformance vectors.
