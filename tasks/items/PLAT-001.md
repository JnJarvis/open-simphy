# PLAT-001 - Specify bounded reads and safe file replacement

Read README.md, tasks/README.md, spec/architecture.md, spec/interfaces.md,
spec/testing.md, spec/contracts/core.md and spec/contracts/serialization.md.
Dependency: IO-001, accepted and merged. No dependency on unfinished IO-002 code.

Scope a platform-owned portable file port for future Save/Open integration. Define
paths, read limits, owning results, replacement commit boundaries, failure/cleanup
semantics, supported filesystem assumptions and deterministic fault-injection
vectors. Explicitly address uncertain OS replacement outcomes rather than claiming
that every failed native call leaves a destination untouched. Review app/IO-001
effects in an RFC. Consult primary OS documentation and preserve source links.

Allowed paths: spec/contracts/files.md; rfcs/PLAT-001-files.md.
No file-adapter or app implementation in this task. No new core enums or dependency
edges. Acceptance: complete API/behavior proposal, consumer review, native-platform
implementation/test requirements, validation precedence and reference checks.
Submit REVIEW; native adapters require accepted contract and scoped follow-up.
