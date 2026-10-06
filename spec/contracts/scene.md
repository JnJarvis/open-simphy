# Minimal scene and snapshot contract v1

Status: ACCEPTED under SCENE-001 following independent user review, 2026-10-06.
Owner: scene. Dependencies: accepted core v1 and math v1 contracts only.
Consumers: physics, renderer, editor, native serialization and compatibility.

## Scope and ownership

The first scene contains free point particles under one uniform gravity vector.
It has no collision shapes, joints, scripts, native resource handles, solver caches,
or file-format-specific fields. An application can produce a useful demonstration
with this model without committing the project to a full rigid-body representation.

Authoring state is separate from runtime state. The application retains the initial
document while physics copies the data it needs. Stepping never mutates the saved
document. All objects own their values; public types contain no references to an
editor, renderer, platform adapter or physics world. No OS-specific types or branches.

## Canonical authoring document

`Document` has schema revision 1, gravity Vec2, a collection of Particle definitions,
and an ID high-water mark. The revision names this in-memory contract; it is not a
SimPHY version or a native-file encoding. An unsupported revision is rejected with
`unsupported_feature`, not guessed or silently migrated.

`Particle` has EntityId, mass in kilograms, initial position in meters, initial
velocity in meters per second, and a positive display radius in meters. The radius
is presentation size only, not collision geometry or an inertia calculation.
There is no body orientation/angular velocity in this first particle slice.
All numeric fields use the accepted math scalar representation.

The document explicitly supplies all these fields. No field secretly reads a global
gravity setting, wall clock, OS configuration or graphics DPI. A future app demo may
choose gravity (0,-9.8) and a display radius, but those are explicit authoring values,
not inferred SimPHY defaults. An empty particle list is a valid document.

Validation rules:

1. Revision must be 1. Gravity components, positions and velocities must be finite.
2. IDs must be nonzero and unique across the document. Check duplicates separately
   from the allocator; reserve_through does not prove uniqueness.
3. Mass and display radius must be finite and strictly positive. Static bodies and
   zero-mass/infinite-mass conventions are outside this slice; do not encode them
   through infinity, zero or negative numbers.
4. High-water mark is at least the largest particle ID, or 0 when no ID has ever
   been issued. An empty document may retain a larger mark after deletion. A raw
   import constructor may calculate the maximum once, but must not rewind a
   supplied higher mark or invent ID reuse.
5. Normalize the validated particle collection to ascending numeric EntityId order.
   Input order has no physical meaning. Do not depend on hash-table iteration.
6. Future arithmetic overflow is not ruled out merely by finite authoring data;
   physics must validate its computed results and report failure transactionally.

Document construction/validation returns core Result<Document>, failing without
publishing partially validated state. An error carries the affected EntityId where
available and logical field path (for example `particles.mass`, `gravity.x`,
`schema_revision`), with the matching core code. For duplicate IDs use duplicate_id;
for nonfinite/invalid authored data use invalid_data. Diagnostic text is explanatory,
not a stable parsing API. Validation ordering for multiple invalid fields is not
specified; tests should isolate each failure rather than require one arbitrary order.

## Commands and atomic editing

The minimum scene operations are add particle, replace an existing particle's
editable fields, remove particle, and set gravity. They operate on a validated
Document and return a new validated Document plus the affected ID when applicable,
using core Result. Inputs do not mutate the original object. The owner replaces
the old document only on success; this also makes test doubles and editor commands
independent of any physics implementation.

Add supplies mass, initial position/velocity and display radius; the document's
allocator chooses a fresh ID. On success high-water advances once. On failure
(including invalid fields or exhausted IDs), no ID is consumed and no state changes.
Replace/remove require an existing nonzero ID; absent IDs return missing_reference,
and invalid zero IDs return invalid_argument. Replace cannot change identity.
Remove retains the high-water mark. Set gravity changes only gravity. A failed
operation leaves particle values, order and high-water mark exactly unchanged.

Commands are synchronous and deterministic for the same input document. One app
owner serializes edits; scene has no threads, event queue, globals or observers.
Rejecting editing while running is app policy from spec/architecture.md; the scene
module has no hidden dependency on playback state. Undo/redo is future editor work.

## Runtime snapshot

`Snapshot` owns a finite nonnegative simulation time in seconds, the finite gravity
vector for that run, and particle samples in ascending ID order. Each sample owns
ID, positive finite mass, finite position and velocity, and positive finite display
radius. The IDs match the run's initial document for this slice; runtime creation
and deletion are not supported yet. Time is simulation time, not frame count or
wall-clock time. Physics receives dt explicitly; snapshot publication does not
advance time.

Consumers receive immutable access to a completed snapshot value. They may retain
an owning copy after the producer changes or is destroyed. Neither producer nor
consumer can mutate the other party's data through a shared mutable pointer/view.
The C++ representation may return a value consumed as const or an owning immutable
handle; it must satisfy these ownership semantics without holding a physics lock.

An initial snapshot copies initial positions/velocities and has time 0. Reset
reconstructs runtime from the app-retained starting document; the next snapshot
again has those initial values and time 0. Reset does not mutate authoring state,
allocate new document IDs, or reuse IDs from a different document. Pause publishes
no changed time/state unless the app explicitly requests another step.

Scene validates snapshot data independently of physics. How the integrator reaches
the next state, steps fail, and elapsed time is accumulated belongs to PHY-001.
No integrator or contact-solver decision is made by this contract.

## Presentation data

Backend-neutral overlays are a value list of Circle and Line primitives, in supplied
draw order. Circle: center Vec2 and positive radius. Line: two Vec2 endpoints and
positive width. Positions, radii and widths are world units, not physical pixels.
Both have RGBA channels, finite doubles in [0,1], interpreted as straight-alpha
sRGB display values. A zero-length line is allowed and may draw no visible segment.
Primitive values own their data and contain no resources, callbacks or native handles.

Primitives must be finite and valid before rendering. Scene can validate packet
values without loading a renderer. Layering is the packet's sequence order; no
physics entity ID is allocated for an overlay. Visualization produces overlays,
renderer consumes them, and app combines them without a visualization→renderer edge.
Particle drawing itself consumes the snapshot's position/radius. Camera transforms,
zoom, pixels and DPI belong to the renderer/editor contract, not these packets.

## Conformance vectors for SCENE-002 and later consumers

| ID | Setup/action | Expected |
|---|---|---|
| S01 | Revision 1, gravity (0,-9.8), empty particles, high-water 0 | Valid empty document |
| S02 | Particles IDs 9 and 2, masses 1 and 2, finite states/radii; mark 9 | Valid output ordered 2,9 |
| S03 | Particle ID 0; two particles ID 2 | invalid_data for zero; duplicate_id for duplicate |
| S04 | Mass 0, negative, NaN or infinity (separate cases) | invalid_data with entity and mass path |
| S05 | Radius 0, negative, NaN or infinity | invalid_data with radius path |
| S06 | Nonfinite gravity/position/velocity, one field per case | invalid_data with relevant path |
| S07 | Revision 2 | unsupported_feature; original input unchanged |
| S08 | Existing ID 9 with high-water 8 | invalid_data; no silently accepted allocator collision |
| S09 | Add valid particle to mark 9 | Fresh ID 10, sorted output, mark 10; input unchanged |
| S10 | Add invalid mass to mark 9, then valid add to same original | First fails without consuming ID; second returns ID 10 |
| S11 | Add with high-water max-uint64 | id_exhausted, document unchanged |
| S12 | Replace ID 2 position; invalid replacement of ID 2 | First changes only supplied editable fields; second leaves document unchanged |
| S13 | Remove ID 9 then add | High-water remains 9 after removal; next ID 10, never 9 |
| S14 | Replace/remove absent ID 3; replace/remove ID 0 | missing_reference; invalid_argument, respectively |
| S15 | Set valid gravity; then request nonfinite gravity | Valid command changes only gravity; invalid request has no effect |
| S16 | Snapshot initial document; change/destroy producer | Retained snapshot data remains usable and unchanged |
| S17 | Reset after a stepped run | Time 0 and exact original ID/mass/position/velocity/radius; authoring unchanged |
| S18 | Publish snapshot at negative/NaN time or with duplicate IDs | Rejected by snapshot validator |
| S19 | Circle center (1,2), radius 0.5; line from (0,0) to (1,0), width 0.1; RGBA (1,0,0,1) | Valid world-space packets; no y flip or pixel conversion |
| S20 | Packet NaN coordinate, radius/width <=0, or color channel outside [0,1] | invalid_data; no backend needed to test |
| S21 | Independent equal input documents receive same commands | Equal resulting data/order; no shared hidden state |

Numeric values copied without arithmetic compare exactly; later renderer/physics
calculations use tolerances from their own contracts. Scene itself does not step
physics or rasterize packets to satisfy these tests. S17 is a lifecycle expectation
for PHY/INT tests as well as the initial-snapshot factory. The authoring/sample
representations remain separate even if their first fields look similar.

## Review boundary

Changing supported entity kinds, field semantics, edit atomicity, ordering or
snapshot lifetime requires contract review. New physics/3D/circuit/geometry work
can extend the model through separate contracts; it must not encode its runtime
caches or an external file schema into this first canonical document.
