# Contributing

Follow AGENTS.md and the task workflow. Keep each branch focused on one claimed
task. Use task ID prefixes in commits and review titles. Prefer small, reviewable
changes, explicit ownership, and tests of observable behavior.

Before implementation, complete the relevant contract task. Draft documents are
not stable interfaces. Accepted contracts must define invariants, errors, units,
ownership, threading, and test examples. Do not select a library merely because it
is familiar; BUILD-001 and REN-001 document justified choices.

Until BUILD-001 lands, no C++ standard, compiler matrix, formatting program, test
framework, package manager, or graphics/UI framework is mandated. Proposed coding
rules: RAII ownership, no hidden global mutable state, explicit error boundaries,
no renderer/UI types in domain interfaces, and deterministic headless testing.
BUILD-001 turns these into concrete checked rules.

The reviewer checks scope, contracts, tests, documentation, and license/provenance.
Completion reports use tasks/templates/completion.md. Report skipped tests with
reasons; required skipped checks prevent DONE. Numeric tests must state tolerances
and their rationale. Update specifications through the RFC process, not by quietly
changing implementation expectations.

Review and merge are serialized by a designated integration owner (human or agent).
Different completed modules may be reviewed independently, but consumers only use
the merged baseline. A coordinator can be an agent; it does not need a custom service.
