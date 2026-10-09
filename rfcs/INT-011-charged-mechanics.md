# INT-011 — Charged mechanics contract review

Coordinator affected-contract review, 2026-10-08, before consumers: scene owns
additive SI data; physics owns forces/integration and owning output; compat owns
source expressions and microcoulomb conversion; app composes/draws. No new
dependency, native v1 change, toolkit or script type in domain interfaces.

CircleBody and BodyUpdate add charge in coulombs, finite |q|<=1e6. Initial/reset
charge is retained separately from live charge. Mechanism adds Coulomb constant
(default9e9, finite0..1e12) and up to64 ElectromagneticField records. Each record
contains world electric vector(N/C), out-of-plane magnetic scalar(T), enabled flag
and optional convex polygon or circle region. Empty vertices/radius0 means global;
polygons use world CCW vertices (3..64), circles explicit world center/radius.
Canonical region membership tests body COM against the actual region, inclusive
at the boundary. Public probes: circle radius.1 at y15 remains inside the region
0..15, but y15.05 is outside despite overlapping fixtures. A rotated10x1 rectangle
accepts(3,3), rejects(3,-3) despite AABB overlap, and rejects(3.9,3.9).
Nonfinite/out-of-envelope fields or regions fail validation atomically.

Physics evaluates equal/opposite COM Coulomb forces once per unordered charged
pair per existing microstep, including static charge sources. No torque from COM
charge, no force softening/capping; coincident nonzero charge centers or force
envelope exhaustion fail explicitly and poison the runtime until reset. Coulomb
constant0 disables interaction. Electric force=qE; magnetic force=q(vy*B,-vx*B).
Magnetic integration uses an exact-length Cayley velocity rotation per microstep
to avoid artificial magnetic energy gain. Contacts/joints remain in the same world.
Outputs report physical force vectors, not the numerical rotation impulse.
Runtime fields(vector) and charge updates validate all commands before publishing.

Source charge conversion1e-6 is measured with public Electric/Magnetic APIs;
source K=.009 maps to9e9. Public ConvexBounds(shape translated to y7.5) reports
translation y7.5 directly, and ElectricField on a dynamic body returns force in
the translated region. Serialized shape center and Translation that agree are
the same frame, not two translations. Other source bounds transforms remain
evidence gated. Nonzero source singularity policy remains unverified; canonical
explicit failure does not claim singular-source parity.

Further trace/widget/source command contracts must be reviewed here before those
consumers. Backend availability alone cannot classify the actual charged demos
as runnable while their required callbacks, force displays or tracers are absent.

Source bridge review before consumers: body charge is stored in source microcoulombs
and converted once on collection/live commands. setCharge stages finite commands
for controls, never replaces the live world. SourceWidget adds checkbox flag and
selected state via its scalar value. Slider/checkbox actions receive a bounded
widget object and typed value, invoke explicit action or named onChange callback.
setFbdDrawn updates visible style metadata; force diagrams consume the owning
physics electromagnetic force output. Unsupported callbacks/APIs fail explicitly.
Standalone public Body.reset needs the application singleton. Until an original-app
observation establishes whether it preserves changed charge, the bridge rejects
reset of a body with changed charge. Unchanged charge is unambiguous. Never guess
the reset policy or classify the charged demo as fully verified while this gate remains.
