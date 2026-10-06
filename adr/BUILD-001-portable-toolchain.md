# ADR BUILD-001 — Portable C++ foundation

Status: ACCEPTED following user review of RFC BUILD-001.
Date: 2026-10-06. Author: codex-worker.

Context: Windows development must not force later Linux/macOS engine rewrites.

Decision: adopt spec/build-policy.md, C++20/CMake 3.28+/Ninja 1.11+,
Catch2 3.7.1 for tests only, and clang-format 18.1.8. Use target-scoped dependencies
and a native headless compiler matrix across the three platforms. Keep OS types
inside platform/app host adapters and renderer backend details private. Preserve
the existing module dependency DAG and the small 2D first milestone.

Consequences: native Linux/macOS checks are needed before declaring portable
components tested, while desktop packaging/UX support remains later work. CI
needs remote runners that are not yet configured. Dependency acquisition is pinned
and supports explicit offline source overrides. No GUI framework is chosen here.

Alternatives and acceptance evidence are in rfcs/BUILD-001-portable-foundation.md.
Reviewer: user ("it all looks good. continue building").
Accepted decision date: 2026-10-06. Supersedes: none.
