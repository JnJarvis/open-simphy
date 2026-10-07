# Particle editor contract v1

Status: PROPOSED under EDT-001, 2026-10-07. Requires independent approval before
implementation. Companion: [affected-contract RFC](../../rfcs/EDT-001-editor.md).
Owner: editor. Allowed dependencies: core, math, scene. No renderer, physics,
SDL, native handles, UI toolkit, clocks, filesystem or app dependency.

## Scope and ownership

Headless authoring of the existing free-particle scene: single selection, add,
replace properties, delete, gravity edit, transactional translation drag, camera,
and bounded undo/redo. No collision shapes, rotation, multi-selection, snapping,
copy/paste, persistence, widgets, scripting or SimPHY behavior. Display radius
remains a visual radius, never a collision shape. No scene schema changes.

`State` is an owning value. No invalid public default construction. All operations
are const and return core::Result<State> (queries use the result types below).
Success owns independent state or immutable shared values; failure cannot change
document, selection, history, view, drag or ID allocation in the original. Publish
only after every validation succeeds. Allocation failures may propagate bad_alloc;
never allocate a diagnostic from a bad_alloc handler. One app owner serializes
commands. No callbacks, observers, hidden global allocator or background thread.

Public accessors expose const document, optional selected EntityId, View by value,
bool dragging, bool can_undo/can_redo, and const display_document. The document is
committed authoring state; display_document is the latest valid drag preview while
dragging, otherwise the committed document. Borrowed access is valid only for the
owning State lifetime; callers retain State/Document copies when needed.

## Value types and operations

`View` contains math::Vec2 center (meters), double pixels_per_meter (physical
pixels/meter), and uint32 width,height (physical output pixels). These are editor
values, not renderer::Camera/Extent types. App copies the equivalent fields into
the accepted renderer types. No dependency edge or replacement renderer API.

`State::create(const scene::Document&, View)` returns Result<State>, initializes
no selection, empty history, no drag, and session ID watermark from Document.
Maximum 4096 particles. History capacity is fixed at 64 committed transitions
(total undo plus redo); it is not user-configurable in v1. Copies retain their own
history and session watermark. New editor session on explicit document load is
outside this task; create is the only history/identity reset boundary.

| Operation | Inputs | Success |
|---|---|---|
| select | optional EntityId | Select existing ID, or clear with nullopt; no history |
| hit_test | math::Vec2 physical pointer | Result<optional<EntityId>>, query only |
| world_at | math::Vec2 physical pointer | Result<Vec2>, checked inverse mapping |
| set_view | View | Replace validated view only; no document/history changes |
| add | scene::ParticleFields | Validated fresh particle; select new ID; one transition |
| replace | EntityId, ParticleFields | Replace all editable fields of existing particle; one transition |
| erase | EntityId | Remove particle; clear selection only if removed ID was selected |
| set_gravity | Vec2 | Update gravity through scene command; one transition |
| undo / redo | none | Restore document and selection for adjacent history transition |
| begin_drag | Vec2 physical pointer | Capture selected particle, starting pointer/world position and view |
| update_drag | Vec2 physical pointer | Replace preview only from captured baseline, no history |
| commit_drag | none | Publish latest preview as one transition, or finish an unchanged drag |
| cancel_drag | none | Discard preview; committed state/history unchanged |

Naming is the implementation target, not a public header already available.
Implementation must not add a public mutable document/history accessor. Successful
add/replace/erase/gravity edits use existing scene operations, not duplicate scene
validation. Scene diagnostics propagate unchanged. Editor checks precede the scene
call only for editor resource/state/precondition requirements described below.

## View, pointer coordinates and safe picking

View dimensions each 1..4096, product <=4,194,304 using uint64 multiplication.
Center finite; scale finite in [1/1024,4096]. This is an editor interaction policy;
renderer still accepts its broader positive scale domain. Zero/minimized output
is handled by app suppressing interaction; no successful zero-extent View exists.
The app updates View when a nonzero output size is available, after canceling a
pending gesture on resize or DPI change.

App converts window-relative pointer coordinates to physical output pixels using
independent ratios output_width/window_width and output_height/window_height.
Require positive finite dimensions and finite multiplication/division results.
No global DPI factor, integer rounding, extra half-pixel offset or y flip in app.
A pointer position is continuous; pixel-center sampling belongs to rasterization.
Pass checked physical coordinates to editor. Native types stay private to app.

For pointer (px,py), require finite components with abs <=1,048,576. Out-of-viewport
coordinates are accepted during captured dragging. Inverse map, in this order:

```
x_offset = (px - double(width)/2) / pixels_per_meter
y_offset = (double(height)/2 - py) / pixels_per_meter
world = center + (x_offset,y_offset)
```

Check every computed component finite. No clamping, epsilon, hidden snapping or
DPI conversion in editor. Failure returns invalid_data/pointer.geometry.

Hit testing uses committed authoring positions and radii, never a runtime snapshot.
Project every particle using the exact operation order in renderer v1, including
finite difference/product checks and abs(projected x/y)<=1,048,576. Projected
radius must be finite, >0 and <=1,048,576. Validate all particles before selecting,
even if a later shape visually wins; malformed projection cannot be silently skipped.

For each projected center, accept a hit when `hypot(dx,dy)<=max(radius_px,6)`.
This is a six-physical-pixel minimum pick radius, not expanded visible geometry.
The subtraction is finite given bounds; hypot avoids squaring overflow. Exact
boundary is included, no epsilon. If multiple particles hit, choose the greatest
EntityId (last particle drawn by ascending-ID renderer). Do not choose nearest
center, hash order, smallest radius, or overlay. Scene overlays are not selectable.
No hit returns successful nullopt and does not change selection.

`hit_test` rejects pointers outside [0,width) x [0,height) as successful no-hit,
after pointer validation and before projecting particles. This prevents clicks in
window chrome from selecting hidden objects. world_at and active drag updates
permit out-of-viewport pointer coordinates within the stated limit.

## Atomic history and identity

A committed transition records before/after authored values and before/after
selection. Camera and active gesture are never history entries. A successful
content-changing command pushes one undo transition, drops the oldest when
capacity would exceed 64, and clears redo. Undo moves the newest undo transition
to redo; redo reverses that move. Undo/redo never issue a new ID.

Crucial: history must not rewind ID allocation. Maintain an editor-session watermark
H initialized from the starting Document.high_water. Every successful add advances
H to the issued ID. Before restoring any historical document, reconstruct it with
its saved revision/gravity/particles and high_water=max(H,saved_high_water), using
Document::create. Publish the reconstructed result and keep H. Historical content
may match an older state while its watermark is intentionally larger.

Example: start H=4; add ID5; undo; add again => ID6, never ID5. Redo of the old add
restores ID5 without allocating only if no branching edit cleared redo. Undoing
delete restores that original entity identity. At uint64 maximum, undo/delete do
not replenish IDs; add still reports id_exhausted. No change to scene's allocator
or Document contract is needed. Do not save raw old Documents as undo results
without this watermark normalization.

A replace with exactly equal ParticleFields or gravity edit with exactly equal
Vec2 is a no-op success: do not push history, clear redo or change selection.
A drag whose final fields exactly equal the baseline is likewise no-op. This uses
existing value equality (so +0/-0 compare equal), not epsilon. Select/set_view
never clear redo. A failed command never clears redo or consumes an ID.
Undo/redo restore the transition's recorded selection (overriding later selection
changes). It must exist in the restored document or be nullopt; a missing selected
ID is an internal_error/history diagnostic, never an unchecked stale reference.
Empty undo/redo are invalid_argument/history failures, not silent successes.

The 4096-particle cap is checked before adding and on create; replacement/removal
cannot increase count. Capacity bounds retained committed documents to at most
65 logical history positions, plus bounded gesture state; implementation may
share immutable storage, but must not retain dropped redo/oldest states forever.
No physics snapshots/frames enter editor history.

## Drag transaction

begin_drag requires a selected entity and no active drag. Capture baseline document,
selected fields, View and world_at(start_pointer). Committed document remains
unchanged; initial preview is the baseline. No scene command is yet committed.
Selection has already been performed separately by the app.

Each update maps the current pointer using the captured View and evaluates:

```
delta = current_world_pointer - start_world_pointer
candidate_position = baseline.initial_position + delta
```

Check differences/sums finite, then call scene::replace_particle on the BASELINE,
changing only initial_position. Never accumulate deltas onto the previous preview;
that would introduce event-rate-dependent drift. A failed update returns failure
and preserves the previous valid preview and active gesture in the original State.
A successful update stores the new preview but does not mutate committed document,
history, selected identity, mass, velocity, radius or watermark.

commit_drag publishes the latest valid preview in one undo transition and ends the
gesture. No update followed by commit is a no-op. cancel_drag ends the gesture and
reverts display_document to committed state with no history entry. With no active
gesture, commit/update/cancel fail invalid_argument/gesture. While active, select,
set_view, add, replace, erase, set_gravity, undo, redo and begin_drag fail
invalid_argument/gesture. Queries remain available; hit_test uses committed data.
The app cancels before focus loss, resize/DPI change, playback start or reset;
failed cancel is a diagnostic, not permission to leave a stale captured view.

## Diagnostics and validation precedence

One error-severity Diagnostic per failure; no partial new State. Message text is
explanatory. Unless propagated from scene, use these stable code/path pairs:

| Condition | Code / path | Entity |
|---|---|---|
| particle count >4096 | invalid_argument / editor.primitives | none |
| invalid View dimensions/product | invalid_argument / view.extent | none |
| nonfinite center or out-of-range scale | invalid_argument / view.camera | none |
| pointer nonfinite or exceeds component bound | invalid_argument / pointer | none |
| inverse-map or drag arithmetic nonfinite | invalid_data / pointer.geometry | selected for drag, otherwise none |
| particle projection invalid | invalid_data / picking.geometry | affected ID |
| select zero ID | invalid_argument / selection | zero ID |
| select absent nonzero ID | missing_reference / selection | requested ID |
| begin_drag without selection | invalid_argument / selection | none |
| illegal operation for gesture state | invalid_argument / gesture | selected if any |
| empty undo/redo | invalid_argument / history | none |
| inconsistent retained selection | internal_error / history | affected ID |

State/gesture preconditions are checked before operation-specific numeric inputs.
Create checks particle cap, then View extent, then camera. Picking checks pointer,
then viewport no-hit, then all projected particles, then resolves topmost hit.
Do not prescribe precedence between independent invalid scene fields; scene's
existing policy remains. Tests isolate each numeric violation.

## App handoff and playback transaction (future integration scope)

App owns editor State and physics World separately. Editor never sees a World or
calls render/step. Runtime output while playing/paused is the World's snapshot;
authoring output is Snapshot::initial(display_document). Selection overlays are
scene packets composed by app; renderer semantics are unchanged.

Use explicit Authoring and Simulation modes. In Authoring, world time is zero and
the displayed state is editor content/preview. Play constructs a new World from
committed document at dt=1/128 and enters running Simulation. Single-step from
Authoring constructs that World and steps once, entering paused Simulation.
Pausing Simulation does NOT turn a time>0 snapshot into editable authoring data.
Reset stops simulation and returns to Authoring at time zero, preserving document,
history, view and selection. This is an intentional CHANGE from INT-001's current
reset-preserves-playback behavior and requires the RFC's app-consumer approval.
No integration implementation may silently change today's demo during EDT-001.

Authoring edits, undo/redo and drag are rejected by app in Simulation mode even
when paused (invalid_argument/app.mode); prompt the user to Reset before editing.
View navigation may be allowed in Simulation but must not touch the document or
world. App normalizes input once; editor maps pointer, renderer maps geometry.

When an authoring edit succeeds, prepare a candidate editor State and, if app keeps
a prepared World, create it from the candidate committed document BEFORE publishing
either. On failure, retain both old editor and old World; no half-applied edit.
Preview updates never rebuild/advance World. Play/reset cancel a gesture first and
prepare the complete new app state before publication. App owns this transaction,
not a callback/plugin framework in editor. Geometry render limits can fail after a
valid scene edit; report the error and retain the last good frame, not silently
clamp physical data. Valid authored data and renderability are separate contracts.

## Independent conformance vectors

Synthetic in-memory fixtures; meters/seconds/kilograms and physical pixels as above.
Exact equality for copied fields, IDs, selection, statuses and dyadic results.
For specified non-dyadic inverse-map examples only, absolute tolerance 1e-12;
never apply a tolerance to identity/history/atomicity. No external fixture files.

| ID | Setup/action | Expected |
|---|---|---|
| E01 | create empty valid document H0, View800x600 center0 scale100 | empty, unselected, no undo/redo/drag |
| E02 | View800x600 center0 scale100, world_at(500,100)/(400,400) | (1,2)/(0,-1) |
| E03 | window400x300 to output800x600; pointer250,50 | app physical500,100 -> world1,2 (no half-pixel shift) |
| E04 | pointer100,200; center(2,-1), s3, W800 H600 | world(-98,32.333333333333336), abs1e-12 on y |
| E05 | dims0,4097,4096x4096; NaN center; scale0,1/2048,8192 | view.extent or view.camera; prior state intact |
| E06 | same-center particles IDs2,9 radius.1, s100, pointer at center | select query returns9; no mutation |
| E07 | projected center400,300 radius1; pointers406,300 and406.25,300 | first hits via six-pixel minimum, second no hit |
| E08 | pointer(-1,300) or(800,300) for W800 | no hit; world_at remains defined within component bound |
| E09 | valid scene particle at max-double, camera=-max-double | picking.geometry with that ID, no skipped particle |
| E10 | select existing/clear; select ID0/absent ID7 | success/no history; invalid_argument or missing_reference/selection |
| E11 | start H4, add valid -> undo -> add valid | ID5 then ID6, redo cleared, H6 |
| E12 | add ID5 -> undo -> redo | ID5 restored, H5, no fresh allocation |
| E13 | delete selected ID5 -> undo -> redo | selection none ->5 ->none; H never decreases |
| E14 | after undo, invalid mass replacement or failed add | state/history/redo/H unchanged; scene diagnostic propagated |
| E15 | after undo, equal replacement/equal gravity; select/set_view | no history push and redo retained |
| E16 | Hmax, remove/undo/add | restored same ID; add id_exhausted; watermark remains max |
| E17 | 65 distinct gravity edits from empty doc | only last64 undoable; oldest retained content is after edit1 |
| E18 | start particle(1,2), pointer500,100, View800x600 s100; update550,125 | preview(1.5,1.75), committed(1,2), other fields exact |
| E19 | same gesture update600,100 then550,125 | same preview as E18, independent of intermediate event |
| E20 | E18 then cancel; E18 then commit then undo | cancel no history; commit one entry; undo restores(1,2) |
| E21 | begin then commit without update; move away then exactly back | no history/redo change |
| E22 | update NaN/huge pointer after valid preview | diagnostic; last valid preview and committed state retained |
| E23 | active drag then undo/add/set_view/select/begin | gesture failure, complete state unchanged |
| E24 | commit/cancel/update with no drag; begin with no selection | gesture/selection failure as specified |
| E25 | 4096 particles then add; create4097 | editor.primitives, no consumed ID |
| E26 | copy State including history/active drag; destroy original | retained independent values/preview/history valid |
| E27 | future app: Play, pause at t>0, attempt edit | app.mode, document/world unchanged; Reset enables Authoring |
| E28 | future app: candidate editor succeeds, injected World creation fails | neither editor nor World published; old history retained |
| E29 | resize/DPI/focus loss during drag | app cancels, then applies new View; no stale gesture commit |
| E30 | undo restores earlier content after larger H existed | all fields/selection restored, high_water rebased to session H |

## Implementation boundaries after approval

Coordinator scopes separate tasks: (1) editor state/commands/history, (2) checked
view/picking/drag against the same contract, (3) build registration for headless
editor target, and (4) app input/property presentation and mode transaction.
Serialize any tasks that share public headers; do not allocate overlapping writable
paths to simultaneous workers. UI toolkit/property-entry design belongs to the
integration scope and must be settled before that task becomes READY. No automatic
promotion based only on this PROPOSED document. INT-001's pending high-DPI evidence
remains separate; this contract neither supplies it nor waives it.
