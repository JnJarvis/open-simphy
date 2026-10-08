# PLAT-001 RFC: explicit file replacement outcomes

Status: PROPOSED, requires independent affected-consumer review.
Full [file contract](../spec/contracts/files.md).

## Why this boundary matters

IO-001 separates a pure codec from file access and requires retaining the original
document when loading fails. Disk replacement adds a different hazard: a native
operation can fail after changing names. Treating every failure as untouched would
mislead users and could delete the only remaining recovery copy during cleanup.

Propose bounded read and replacement ports owned by platform with core-only
dependencies. Replace reports not_committed, committed or indeterminate; it never
falls back to truncating the target. Explicit outcomes prevent a generic Result<void>
from incorrectly equating every failure with rollback. Limit and path rules remain
independent of native scene format and eventual SimPHY-compatible import parsers.

## Evidence and decisions

Microsoft's documented ReplaceFileW failure modes include renamed/missing original
paths; its write-through flag is not supported. The design therefore requires
classification and preserved recovery files when outcome is uncertain.
[Microsoft source](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-replacefilew).

POSIX rename's replacement behavior does not justify universal rollback claims for
all failure classes. The implementation must review native errors and cannot infer
crash durability from successful namespace replacement.
[POSIX source](https://pubs.opengroup.org/onlinepubs/9799919799/functions/rename.html).

These are source-informed design requirements, not claims of tested adapters.
Reject generic delete-then-rename, cross-volume copy fallback and hidden retry of
uncertain operations. A trusted local-directory scope avoids claiming sandboxing
against malicious path mutation. Concurrent external writes and power-loss recovery
remain outside v1; do not present those exclusions as full product requirements.

## Affected consumers and migration

| Consumer/owner | Effect |
|---|---|
| core | No new enum/code/Result semantics; standalone warning allowed after committed cleanup issue |
| platform | New file port and owned outcome; private native strategy/injection seams follow acceptance |
| serialization | Pure codec unchanged; byte cap supplied by app, not imported into platform |
| app | Refine IO-001 save results: only committed updates baseline; uncertain disk state needs explicit messaging/recovery |
| editor/physics/renderer | No API, state or dependency change |
| compat | Future importer may consume bounded file bytes; format inventory and larger/streaming inputs need separate scope |
| build/test | Native file tests require headless registration of platform when implemented, separately scoped |

IO-001 says save failures retain baseline/path and must not report success. Preserve
that rule for not_committed/indeterminate. Explicitly clarify that disk rollback
cannot be promised for indeterminate native failures; committed with a cleanup
warning remains a successful save. No published code consumes a previous file port,
so no implementation migration or accepted-header change is required now.

## Acceptance and follow-up scope

Review owning API, path/limit rules, supported filesystem assumptions, commit
boundary, recovery lifetime, exception handling, error precedence and P01-P22.
Require integration-owner approval of the IO-001 outcome clarification. After
acceptance, coordinator scopes portable transaction orchestration, native adapters,
tests/build registration and then app integration using accepted merged code.
Implementation review must settle permission/ACL details and actual OS primitives;
no adapter task becomes READY while those implementation decisions are ambiguous.
