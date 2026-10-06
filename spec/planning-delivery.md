# Planning baseline delivery

This phase creates documentation, task infrastructure and empty module/test
directories. It does not implement a simulator or claim SimPHY compatibility.

| Requested deliverable | Location |
|---|---|
| Architecture and module/dependency map | spec/architecture.md |
| Stages and exit criteria | spec/roadmap.md |
| Repository layout | README.md and module/test placeholders |
| IDs, statuses, ownership and task loop | tasks/README.md |
| Task/completion templates | tasks/templates/ |
| Machine-readable registry | tasks/registry.json |
| Agent rules | AGENTS.md |
| Interfaces and integration rules | spec/interfaces.md |
| Testing strategy | spec/testing.md |
| RFC/decision process | rfcs/README.md, rfcs/TEMPLATE.md, adr/0001-modular-planning-baseline.md |
| Initial detailed backlog | tasks/items/ (17 tasks) |
| Dependency graph and parallel work | tasks/graph.md |
| Open decisions/blockers | spec/open-questions.md |
| Future research intake | spec/research-intake.md |

Four READY tasks have independent scopes: BUILD-001, CORE-001, MATH-001,
COMPAT-001. REN-001 is blocked on the platform/build policy. COMPAT-002 has an
external research gate. Later feature work is intentionally not expanded yet.

Before dispatching workers, appoint a canonical coordinator, commit the planning
baseline to Git, and give workers isolated checkouts of that baseline. There is no
remote configured and no license selected. Do not have separate clones grant claims.
The initial registry is unclaimed, and no implementation task is marked DONE.

Validation commands:

```text
python tools/tasks.py validate
python tools/tasks.py ready
python -m unittest discover -s tests -p test_task_workflow.py -v
```

The coordination tests cover duplicate claims, blocked dependencies, review/report
requirements, independent reviewers, dependency unlocking, cycle detection, scope
overlap, explicit blockers and lock cleanup. Simulator tests are future task work.
