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
