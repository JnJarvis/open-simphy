# EDT-001 RFC: transactional particle authoring and history

Status: ACCEPTED by user, 2026-10-07; editor and app mode/reset semantics approved.
This is a new editor contract and an explicit proposal for future app behavior,
not approval to implement or mutate existing producer APIs.

## Problem and proposal

The demo can simulate a fixed scene but cannot author one. Adding UI gestures
directly to the physics World would blur document/runtime ownership. Naive undo
by restoring old documents would also rewind their ID watermark and reuse IDs
following a history branch. Incremental dragging would make positions depend on
input event rate. The proposed [editor contract](../spec/contracts/editor.md)
resolves these before code: owning editor state, checked mapping, deterministic
picking, baseline-relative drag previews, bounded history and session-monotonic IDs.

## Affected-contract review

| Owner/consumer | Existing guarantee | Proposed impact / review requirement |
|---|---|---|
| core | EntityId stable; checked Result with const payload | unchanged; no mutable Result accessor or new error enum |
| math | finite binary64 Vec2 operations | unchanged; editor adds checked composition, not new math policy |
| scene | validated immutable documents, monotonic high-water within commands | unchanged; editor normalizes historical Document using existing create and session H |
| physics | fixed-dt World from retained document; reset independent of pause policy | unchanged; app mode policy changes, World API does not |
| renderer | world-to-physical-pixel projection; scene snapshots/packets | unchanged; app copies editor View fields into renderer Camera/Extent, no new dependency |
| editor (new) | no accepted port yet | new v1 authoring/history/gesture contract and resource limits require approval |
| app | INT-001 reset preserves play/pause; no authoring mode | future Reset enters stopped Authoring mode; no edit in paused Simulation; integration-owner approval required |
| build/test | editor currently omitted with APP=OFF | later BUILD task must register editor headlessly; do not hide editor tests behind SDL |
| future serialization/compat | IDs/high-water matter for document validation | no file schema added; future save/load contract must preserve watermark and define new-session boundaries |

No domain-to-UI edge, toolkit dependency, native format, scene field, pixel rule,
or numerical physics update changes. No current executable behavior changes in
this RFC. The authoring mode distinction applies only when separately scoped app
integration is implemented after approval.

## Decisions and tradeoffs

- Six-pixel minimum picking radius improves small-target selection without changing
  raster geometry. Highest ID wins overlap to match current draw order. Overlays
  cannot steal selection. Screen coordinates are physical pixels exactly once.
- Authoring edits are allowed only in Authoring mode, not paused advanced runtime.
  Reset is the explicit return to the initial authored state. This avoids deciding
  how to turn a moving snapshot into a new initial document silently.
- Full immutable document positions for up to64 history transitions are simple and
  bounded for <=4096 particles. A command-delta optimization can come later while
  retaining exact ownership/atomicity tests. No promised memory-byte budget.
- Session ID watermark survives undo/redo and dropped redo branches. Undo restores
  content and selection but does not restore the old allocation state. This is
  intentional; equality tests must distinguish content from monotonic watermark.
- View and selection operations are not undoable; recorded edit transitions restore
  their own selection on undo/redo. No-op document edits preserve redo branches.
- Drag uses a frozen camera and baseline document; resize/DPI/focus/play/reset cancel
  the gesture before changing coordinate context. The end pointer is not cumulatively
  integrated. Invalid updates retain the last valid preview.
- Editor view scale bounds are interaction limits, not changes to renderer validity.
  Scene-valid extreme geometry may still fail picking/rendering with diagnostics.

Rejected for this slice: editing a live World, adopting runtime state on pause,
restoring stale allocator marks, silently clamping invalid numbers, global undo,
platform-native event types in editor, and choosing a full widget toolkit here.

## Consumer review checklist

Independent review must explicitly confirm the new editor port and app mode/reset
change, identity normalization, no-op/redo rules, history cap, drag cancellation,
pick ordering/minimum radius, physical-coordinate conversion, diagnostics and all
E01-E30 vectors. Verify scene create can raise high_water while restoring fewer
particles (accepted scene v1 already permits this). Verify renderer/physics/core
headers and tests are untouched. Review BUILD and INT follow-up scope before
implementation; no worker may approve its own interface change.

## Migration and acceptance

No migration code is required now: no editor has shipped and existing interfaces
remain unchanged. After approval, record acceptance in an ADR/contract status via
coordinator, scope implementation tasks, and require native headless contract tests
and app lifecycle tests. App integration must have independent review and real
Windows gesture/property smoke. Linux/macOS desktop and mixed-DPI claims require
actual evidence; no contract review substitutes for hardware validation.
