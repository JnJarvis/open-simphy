# Open physics simulator — planning repository

An independent, open-source C++ simulator project, aiming eventually at broad
SimPHY feature and project-file compatibility. No compatibility is claimed and no
simulator is implemented. SimPHY research is being produced separately.

## Start here

1. Read [AGENTS.md](AGENTS.md) and [CONTRIBUTING.md](CONTRIBUTING.md).
2. Read [architecture](spec/architecture.md), [roadmap](spec/roadmap.md), and
   [task workflow](tasks/README.md).
3. Run `python tools/tasks.py ready`, select one task, and read its Markdown file.
4. Claim through the shared coordination checkout:
   `python tools/tasks.py claim BUILD-001 agent-name`.
5. Implement only that task in an isolated checkout; submit evidence for review.
6. The integration owner reviews and merges, then marks DONE and refreshes readiness.

See [initial backlog and dependency graph](tasks/graph.md), [open questions](spec/open-questions.md),
[testing](spec/testing.md), [interfaces](spec/interfaces.md), and
[research intake](spec/research-intake.md).

The registry is `tasks/registry.json`; descriptions are in `tasks/items/`.
The coordination script uses only Python 3's standard library. It is planning
infrastructure, not a build dependency of the future C++ application.

Only one canonical coordination checkout may grant claims. Separate clones must
not self-assign from stale registries. See the workflow for distributed operation.

## Layout

`spec/` holds specifications, `adr/` accepted decisions, `rfcs/` proposed changes,
`tasks/` work and reports, `research/` future evidence, `src/` module placeholders,
`tests/` test categories and fixture provenance, and `tools/` repository utilities.

The project name, license, supported platforms, and build stack remain open.
Do not distribute a release until the license and dependency policy are resolved.

The [planning delivery index](spec/planning-delivery.md) maps all requested
deliverables and lists the one-time setup before dispatching workers.
