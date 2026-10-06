# ADR 0001 — Independent model and contract-first development

Status: ACCEPTED for this planning baseline, 2026-10-06.

Context: many agents must develop independently before empirical SimPHY research
arrives. A shared external-file-shaped model would couple all modules to unknowns.

Decision: use the dependency DAG in spec/architecture.md, an independent canonical
model, immutable snapshots, explicit app composition, native/compat codec separation,
and contract tasks before implementation. Adopt a 2D free-particle first slice.
Use JSON registry plus Markdown tasks and one canonical claim coordinator.

Consequences: contracts and integration are explicit gates; not every task can be
READY immediately. Remote workers require a single coordination authority. Full
compatibility, graphics framework, native encoding and solver choices remain open.

Alternatives rejected: shared mutable application objects across modules; duplicating
SimPHY file structures as the internal model; distributed uncoordinated claims;
large speculative feature backlog. Supersede via reviewed RFC, never silently edit.
