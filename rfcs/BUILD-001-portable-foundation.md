# RFC BUILD-001 — Portable foundation toolchain

Status: PROPOSED, awaiting independent review.
Author: codex-worker. Date: 2026-10-06.
Affected owners: build/integration and future module implementers.

## Problem and evidence

The user authorized foundation development on Windows with straightforward later
Linux/macOS development. The existing DAG isolates platform services but does not
yet specify build tools, compiler checks, dependency acquisition or coding rules.

## Proposal and alternatives

Adopt spec/build-policy.md: conservative C++20, CMake/Ninja shared headless presets,
test-only Catch2, explicit compiler/platform matrix and private platform/backend
adapters. Retain current module ownership and dependency directions.

C++17 is viable but unnecessary for the selected modern compiler set; C++23 and
named modules add avoidable compiler/library variation. A Windows-only solution
would not exercise the future ports. A large package-manager bootstrap is not
needed for one test dependency. A custom assertion framework would create its own
maintenance burden. Full UI/graphics toolkit selection remains REN-001 work.

## Impact and migration

No existing C++ ABI, file schema or implementation changes. BUILD-002 and REN-001
remain blocked until this task is reviewed and merged. CORE-001/MATH-001 can be
reviewed independently; they use no native types or third-party libraries.
No runtime platform abstraction or speculative backend is implemented by this RFC.

## Acceptance and review

Reviewer checks tool versions/sources, Windows-first/native-headless distinction,
dependency/license policy, domain/platform boundaries, compiler warning scope and
the exact acceptance commands. BUILD-002 must execute the future scaffold checks;
this policy does not claim those results. The retained architecture is the accepted
ADR 0001; this RFC adds implementation policy, not new dependency edges.

Independent review: pending. Decision: pending.
Decision record: adr/BUILD-001-portable-toolchain.md (proposed until review).
