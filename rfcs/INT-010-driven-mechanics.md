# INT-010 — Driven mechanisms

Affected-contract review before consumers (coordinator, 2026-10-08): scene,
physics, compat and app retain their existing dependency boundaries. Additive
ports below affect only the experimental mechanism profile, never native v1.

`SlideLink` defines COM-local anchors and a unit axis in A's local frame. A may
be ground (zero ID, anchors/axis then world coordinates); B is explicit. Line
mode constrains normal translation and leaves relative rotation free. Prismatic
mode additionally locks B-A rotation to `reference` radians. Translation is the
anchor difference projected onto the rotating A axis, in meters. Optional finite
ordered limits and a prismatic motor use m/s and N. Disabled source limits may
contain infinity; translate only enabled limits. Joint IDs share the existing
1024 aggregate budget and collision policy is explicit. A line motor is rejected.

`BodyUpdate` contains optional COM position, velocity, angle, angular velocity,
mass, inertia, gravity scale, linear/angular damping and fixture material values.
`update(vector<BodyUpdate>)` validates the whole batch against canonical envelopes
and body mobility before any mutation, including duplicate IDs. Static pose
changes are allowed, nonzero static velocities are not. Each field leaves others
unchanged; mass updates retain explicit COM/inertia. Snapshot refreshes immediately;
constraints and contacts remain in the same runtime. Reset restores the original
definition. No geometry scaling, hidden body replacement or solver rebuild.

`set_time(seconds)` changes the published simulation clock with a finite nonnegative
value, then each step adds fixed_dt. It leaves body/joint state intact; reset clears
it. No host clock dependency. Source clock callbacks must use this port.

Compatibility mapping requires public numeric frame/unit evidence. LineJoint
public probes recorded in INT-009 confirm local anchor in A origin frame and local
axis angle in radians; convert anchor to COM once. Nonzero source offset/friction
remain unsupported until measured. Prismatic import/controller units are separate
evidence gates; canonical backend support alone never claims file compatibility.

Required evidence: free axis/free rotation, translated/rotated/off-center frames,
reaction torque, bounds/motor, atomic rejected batches, reset/time, driven springs,
existing collisions, real source controls and files. Charge/field implementations
follow as separate scoped work; no charged file may run while charge is omitted.

SourceWidget adds visible/enabled flags, inherited through containers. Hidden
sliders retain their scalar binding; app skips their rendering and hit testing.
Finite hidden layout bounds may be nonpositive since no display rectangle is
used. Public Gui.create("slider") numerical probe confirms source defaults
minimum=0, maximum=10, value=5; explicit attributes override those values.

SourceControls adds optional time command (0..1e9 seconds). Source clock setters
stage a finite command; successful controls evaluation consumes it once. Creation
preflight restores the staged command so app construction receives it. Expressions
in the same batch see the requested time. App applies time before forces/step and
also when constructing the replacement runtime after a source reset callback.
World.getSimulationTime reads the interpreter's current simulation time; neither
source nor physics accesses a host clock. Failed evaluation never publishes a batch.

Approved prismatic public probes: constructor world anchors(1,2)/(3,4), world
axis(0,1), body angles .3/.7 return the same world anchors/axis and reference-.4.
getJointTranslation reports +2 but getJointSpeed for B velocity(0,2) reports -2.
Actual limit response uses A-B: B initial y2 with source limits0..1 goes to .005,
limits-3..-1 leave y2, limits-1..3 go to1.005. Positive source motor moves B negative.
Translate actual response, not that inconsistent getter: canonical B-A limits
are -sourceUpper..-sourceLower, motor speed=-sourceSpeed, reference=-sourceReference.
When source B is fixtureless ground swap endpoints/anchors, reverse signs again,
and account for any original ground rotation absent from canonical ground. Axis
comes from source world getAxis and is rotated into canonical A's frame once.

User confirms actual Resonance runs in original SimPHY despite its stale LineJoint
A UUID. Narrow compatibility inference: an unresolved LineJoint A may alias the
unique fixtureless INFINITE body named FixedAnchorBody only in its zero origin/
rotation/COM frame. Missing B, other joint families, ambiguous/transformed defaults
still fail. This represents the built-in ground reference, never a missing dynamic
body. Record this inference explicitly, then check real driven behavior.

SourceControls adds a vector of BodyUpdate commands, coalesced per ID in controller
order. ValueProperties indices9/10 are COM coordinates (m),11/12 velocity(m/s),
matching public property names, source world axes and measured COM setter semantics.
13/14 currently accept only exact zero, whose conversion is unambiguous regardless
of the still unmeasured angular expression units; nonzero requests remain explicit
unsupported errors. This enables zero rotation locks without guessing other units.
Apply updates atomically before forces/stepping. Property controllers operate on
the actual current snapshot, never retained startup coordinates or rebuilt worlds.
