# Interface and integration rules

These are architectural requirements, not completed C++ APIs. CORE-001, MATH-001,
and SCENE-001 produce the first frozen contracts; consumers remain BLOCKED until
those contracts are reviewed and merged.

Each contract specifies: version, owner, types and operations, pre/postconditions,
units, coordinate convention, finite-value policy, errors, lifetime/ownership,
thread assumptions, determinism expectations, and executable conformance examples.

Initial defaults: SI units; radians; 2D right-handed coordinates with +y up;
screen conversion belongs to presentation. Physics time is simulation time,
not wall-clock time. Fixed step is provided by app; physics never reads a clock.
No promise of bitwise cross-platform determinism. Specify tolerance-based reference
agreement; document any same-build deterministic stepping guarantee.

Contract inventory:

| Producer | Consumers | Required semantics |
|---|---|---|
| core | all | stable nonzero IDs, invalid sentinel, structured diagnostics |
| math | scene/domain/presentation | vectors, transforms, finite values, unit conventions |
| scene | physics/editor/IO/compat | document schema, validation, command atomicity, version |
| scene snapshot | renderer/visualization/analysis | copied immutable values, object IDs, simulation time |
| scene presentation data | renderer/visualization | world-coordinate primitive packets, no backend objects |
| physics port | app/test doubles | construct, step, snapshot, reset; explicit failure behavior |
| renderer port | app/test doubles | consume snapshot/packets, resource lifecycle, resize |
| serialization port | app | bounded bytes, native schema version, transactional decode |
| compat port | app | translated scene plus unsupported/loss/error report and provenance |
| platform ports | app | file bytes, input, time, surface lifecycle; injected adapters |

Stage 0 freezes only contracts needed for the first slice. Later port definitions
are requirements, not permission to implement speculative systems.

To change a frozen contract, create rfcs/<task-id>-<topic>.md, enumerate consumers,
describe migration and tests, and obtain review from affected module owners and
integration owner. Record acceptance in adr/ and create contract/migration tasks.
Pause affected claims before a breaking merge. No unilateral header edits.

Integration tasks consume only reviewed, merged producers. They own composition,
integration fixtures, and lifecycle checks. Integration failures produce scoped
follow-up work; app must not absorb missing domain behavior as a workaround.
