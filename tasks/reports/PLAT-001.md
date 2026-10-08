# PLAT-001 portable file contract evidence

Status: REVIEW, not accepted/merged. Worker codex-worker.
Branch codex/plat-001, worktree .worktrees/PLAT-001.
Commit6b5d6f4e42880f26368da57cec1b5fbae27fd022.

Delivered spec/contracts/files.md and rfcs/PLAT-001-files.md, both within scope.
Proposes bounded owning reads, same-directory exclusive temporary writes, explicit
commit/uncertainty outcomes, owned recovery paths, native path/type/permission
boundaries, exception/cleanup rules and22 conformance vectors. App/IO-001 RFC
explicitly distinguishes uncertain disk state from retained in-memory baseline.
No core enum, accepted API, domain dependency or runtime implementation changed.

Checks2026-10-07:22 unique P01-P22 vectors, relative links, balanced code fences,
64 MiB arithmetic and staged whitespace PASS. Primary source review: Microsoft
ReplaceFileW full documentation and POSIX.1-2024 rename indexed specification.
Direct POSIX open returned403; search retrieval supplied the primary specification
including replacement visibility and EIO exception. Sources linked in both artifacts.
Do not claim native adapter tests from this documentation review.

Affected consumers reviewed: core, platform, serialization, app, domain modules,
compat and build/tests. Independent acceptance is still pending. The uncertain
replacement result is an explicit IO-001 app-policy refinement requiring review.
Task consumes accepted IO-001 only, not unfinished IO-002 implementation.

Follow-up proposal: scope native implementation with concrete OS primitives,
permission/ACL policy and testable filesystem coverage, headless platform build
registration, fault injection plus real native temporary-file tests. Then app
integration. Unsupported filesystem behavior must fail explicitly; no generic
fallback that deletes the original file first. No crash-recovery guarantee.
