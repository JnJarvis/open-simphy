# Architecture and module map

## Principles

The canonical scene is independent of SimPHY and of all UI, graphics, and file
libraries. Physics runs headlessly at a fixed step. Presentation consumes immutable
snapshots; editing produces validated commands. The app composes modules and owns
lifetimes. Module dependencies refer to stable public contracts, not private source.

Initial scope is a deliberately small 2D simulation slice. This is not a claim
about SimPHY dimensionality or capabilities. Expansion requires evidence and an RFC.

## Allowed dependency DAG

An entry lists all permitted direct internal dependencies. Nothing may depend on
app. New edges require an ADR; transitive availability does not permit direct use.

| Module | Responsibility / boundary | Does not own | Allowed dependencies | Tests it owns |
|---|---|---|---|---|
| core | IDs, result/error values, version primitives | math, logging service, global state | none | ID and error invariants |
| math | 2D vectors, transforms and numeric helpers | scene objects, solver policy | none | algebra, finite values, tolerances |
| scene | Canonical authoring data, validation, edit commands, immutable snapshots | runtime solver caches, graphics, file parsing | core, math | validation, commands, snapshot isolation |
| collision | Shapes, broad/narrow-phase queries, contact data | time stepping, UI, impulse solve | core, math, scene | geometric/reference and degeneracy cases |
| constraints | Constraint descriptions and row generation | global step ownership, UI | core, math, scene | Jacobian/reference and invalid-input tests |
| physics | Runtime world, fixed-step integration, contact/constraint solving | file I/O, presentation, authoring history | core, math, scene, collision, constraints | analytic/reference, determinism and stability |
| renderer | Draw immutable scene and overlay packets through a backend | simulation, edits, native windows | core, math, scene | packet semantics, headless contract and backend smoke |
| editor | Selection and tools that emit scene commands; editor view state | solver, direct file access, app wiring | core, math, scene | command behavior and view-state tests |
| visualization | Produce overlay packets from snapshots and recorded values | graphics backend, stepping | core, math, scene | vector/trajectory/measurement geometry |
| analysis | Sample streams, quantities, graph series and exports as data | graph widgets, renderer, solver mutation | core, math, scene | units, sampling, missing-data and series tests |
| serialization | Versioned native document codec | SimPHY schemas, platform paths | core, math, scene | round trips, migrations, malformed-input limits |
| compat | External bytes → external representation → canonical translation and report | native codec, editor, solver, inferred undocumented behavior | core, math, scene | evidence-backed translations, diagnostics, hostile input |
| platform | Files, clocks, input events, host surfaces behind adapters | domain policy or scene | core | adapter behavior and platform smoke |
| app | Composition, event routing, document/runtime ownership and startup | module algorithms | all above | end-to-end lifecycle and feature integration |

Renderer packet and sampling contracts live in public scene contract headers so
visualization, analysis, and renderer never depend on one another. They are data
contracts only; SCENE-001 must avoid placing presentation behavior in scene.
Platform surface handles are opaque values owned by the host, passed by app.
Backend-specific includes stay private. Collision and constraints are optional
physics dependencies until Stage 3; Stage 1 does not need placeholder solvers.

## Ownership and flow

Authoring document → validated commands → canonical document. App constructs a
runtime world from a document, steps it, and publishes an immutable snapshot.
Reset reconstructs from the retained starting document. Simulation does not mutate
the saved authoring document. Editor changes while running are initially rejected;
later live-edit semantics require an explicit design decision.

App forwards snapshots to renderer, visualization, and analysis. It combines
overlay packets with rendering input and displays graph series through editor UI.
No consumer calls back into physics. File adapters supply bounded bytes to codecs;
successful decoded data is validated before app replaces a document.

## Repository boundaries

Each src/<module>/ eventually contains include/opensim/<module>/, private source,
and a module-owned CMakeLists.txt. Tests follow tests/<category>/<module>/.
Root build files belong to BUILD tasks; public headers belong to contract tasks;
src/app/ belongs to INT tasks. Directory placeholders confer no implementation.
Build checks should enforce the DAG and catch domain imports of UI/backend headers.

Stable contracts are merged before parallel consumers start. Test doubles belong
to the consumer's test directory, with shared conformance vectors under
tests/contract/<module>/. Do not require live renderer or UI infrastructure for
physics, serialization, or compatibility tests.
