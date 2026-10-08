# Portable bounded file transactions v1

Status: PROPOSED under PLAT-001. No implementation authorization until review.
Owner: platform; consumer: app. Internal dependency: core only. No codec, scene,
editor or physics dependency; byte limits come from callers, not a native schema.

## Port, ownership and supported scope

Proposed header `opensim/platform/files.hpp`, namespace `opensim::platform`:

```cpp
enum class WriteDisposition { not_committed, committed, indeterminate };
struct WriteOutcome {
    WriteDisposition disposition;
    std::optional<core::Diagnostic> diagnostic;
    std::vector<std::filesystem::path> recovery_files;
};
[[nodiscard]] core::Result<std::vector<std::byte>>
read_file(const std::filesystem::path &, std::size_t byte_limit);
[[nodiscard]] WriteOutcome
replace_file(const std::filesystem::path &, std::span<const std::byte>);
```

Calls are synchronous. Inputs are borrowed for the call; results own bytes, paths
and diagnostics. Independent paths can be operated on concurrently; app serializes
operations on a document. No global current-directory changes, UI, logging, retries
of uncertain commits, file watching or background threads. No public OS handle.
Use the standard filesystem path's native representation without lossy conversion;
the app converts picker paths at its boundary, never through the locale ANSI codepage.

v1 targets regular local files in user-selected, trusted directories on supported
native filesystems. No network/remote durability, device/FIFO/directory content,
hostile directory mutation, concurrent-writer conflict detection or filesystem
snapshot guarantee. Native implementations document tested filesystem/OS coverage.
Unsupported replacement semantics must fail before touching the destination rather
than fall back to truncation or copy/delete. Native locks/permissions are respected.

Require absolute paths with a nonempty final filename, no embedded native NUL and
no final `.` or `..`. Do not expand environment variables, `~`, URLs, shortcuts or
relative paths. Parent directories must exist; never create them implicitly.
No hardcoded MAX_PATH truncation. Unsupported OS path lengths fail explicitly.
Final-component symlinks/reparse points and nonregular files are rejected for read
and replacement; do not follow them silently. Parent links may resolve normally in
the trusted-directory scope; this API is not a sandbox/security boundary. Recheck
opened source type, not only a racy pre-open status. Read opens should not hang on a
FIFO while discovering it is not regular. Saving replaces the named directory entry,
so another hard link to the old file retains the old data; no in-place mutation.

## Read transaction

`byte_limit` must be 1..67108864 inclusive (64 MiB resource ceiling for this port).
The native codec consumer passes229424; an eventual other format supplies its own
reviewed bound, or a separately reviewed streaming API when this port is unsuitable.
This port limit is not a final product import-size limit.

Validate path, then limit, then inspect/open the regular file. Reject known length
greater than limit before reading, but never trust a size hint alone. Read until
EOF with total allocation bounded by limit plus a one-byte overflow probe and fixed
bookkeeping. Handle partial reads; an interrupt may retry without resetting offset.
If one extra byte exists, return file.size error without successful partial bytes.
Zero-byte EOF is valid; codec decides whether an empty document is meaningful.
Read/open/close failure produces failure and no partial result. Source is unchanged.

Concurrent in-place modification is outside the consistency guarantee: a successful
read is the bytes observed through the opened handle, not a filesystem snapshot.
A rename by another writer may let an already-open reader finish the old file.
Do not describe either case as guaranteed newest data. Same-app writes are serialized.

## Replacement transaction

The caller authorizes replacement at this exact path, whether currently absent or
present. There is no compare-and-swap or create-only mode. The app owns overwrite
confirmation; other processes may still race it, so no conflict-detection claim.
Reject bytes >64 MiB before I/O; zero bytes is allowed at this byte port.

1. Validate path/size and supported destination kind. Establish parent directory.
2. Exclusively create a fresh regular temporary file in that same directory. Never
   reuse or truncate a guessed existing temporary name. Bound name collision retries
   to16; exhausted retries fail without changing destination. Reserve recovery-path
   and diagnostic storage before any operation that may change the destination.
3. Write all bytes, treating short writes as progress and zero-progress/error as
   failure. Flush file data through the native adapter and check close before commit.
   Failure here is not_committed: destination content has not been touched.
4. Perform one supported native same-filesystem replacement. Never delete/truncate
   the destination first; never use a cross-volume copy fallback. The successful
   replacement is the logical commit point. Post-commit bookkeeping must not throw
   an allocation exception that hides a completed replacement.
5. Clean up only temporary entries this operation demonstrably owns. Never delete
   the requested destination as cleanup or attempt blind rollback after commit.

The adapter must distinguish these outcomes:

| Disposition | Required meaning | Diagnostic / recovery |
|---|---|---|
| not_committed | This operation did not replace or remove the old destination entry/content | Error diagnostic; list any owned temp retained after failed cleanup |
| committed | Replacement completed with requested bytes at commit point | No error; optional warning for residual cleanup; report retained owned files |
| indeterminate | Native replacement was attempted but its failure semantics cannot prove old target intact or completed replacement | Error diagnostic; retain available recovery files, no blind delete/retry |

Without concurrent actors, not_committed preserves previous destination bytes (or
absence). It does not promise access-time/metadata identity. committed guarantees
the publication point, not indefinite immunity from other writers or power loss.
At most two recovery paths (new temporary and optional original backup) are returned.
They are informational absolute paths; caller must not automatically delete/open
them as another document. A path is reported only for an artifact still known to
exist; failure to inspect leaves that candidate conservatively retained/reported
with an explanatory diagnostic. No directory-wide wildcard cleanup.

Implementation must prepare outcome storage before commit and retain enough error
context to return a valid outcome even when post-commit memory allocation fails.
Ordinary bad_alloc may propagate only before any destination-changing operation;
RAII cleanup must preserve the destination and not throw while unwinding.
Not_committed and indeterminate always have error-severity diagnostics. Committed
has either no diagnostic or warning severity. Returned outcome invariant violations
are implementation bugs, never interpreted by app as successful save.

Do not preserve or copy arbitrary alternate data streams, executable content or
unrelated metadata as project bytes. New-file permissions come from normal local
creation policy; implementation must specify destination permission/ACL handling
before native adapter review and must not silently bypass permission failures.
This v1 contract promises byte publication, not identical inode/file-ID/timestamps,
extended attributes or portable ACL semantics.

## Atomic visibility, uncertainty and durability

For supported ordinary local filesystems, successful publication must use native
replacement semantics that do not expose a partially written destination. This is
distinct from crash durability and from guarantees about failed native operations.
No process-crash/power-loss recovery or directory-sync durability promise in v1.
Flush before commit is still required to detect ordinary write failures early.

Microsoft documents ReplaceFileW errors where the original file is renamed or no
longer at its original path. Its WRITE_THROUGH flag is unsupported. Therefore an
implementation cannot map every zero return to not_committed or delete every temp
on failure. Native strategy must classify documented effects or return indeterminate.
[ReplaceFileW documentation](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-replacefilew).

POSIX rename provides replacement semantics and operates on the final symbolic
link itself. Its I/O-error case must not be treated as proof that nothing changed.
Implementation review must account for the selected OS/filesystem rather than
assuming a C++ stream rename wrapper proves all required guarantees.
[POSIX rename specification](https://pubs.opengroup.org/onlinepubs/9799919799/functions/rename.html).

Native adapter implementation tasks select and justify the concrete OS strategy,
including absent destination, sharing modes, metadata, cleanup and fault outcomes.
Private OS wrappers expose deterministic injection points to tests; do not expose
handles or a generic syscall framework in this public API. Require Windows, Linux
and macOS tests; mocked success alone is insufficient.

## Diagnostics and precedence

Diagnostic entity is absent; paths below are stable, messages are owned explanatory
text and may contain native error context. No new core enum is needed.

| Condition | Code / path |
|---|---|
| empty/relative/NUL/final dot path | invalid_argument / file.path |
| read limit0 or >64 MiB; write span >64 MiB | invalid_argument / file.limit |
| directory, special file, final link/reparse point, unsupported replacement semantics | unsupported_feature / file.kind |
| open/inspect failure (including missing read source or parent) | invalid_data / file.open |
| source exceeds read limit | invalid_data / file.size |
| read or read-close error | invalid_data / file.read |
| exclusive temp creation/retry failure | invalid_data / file.create |
| failed/zero-progress write | invalid_data / file.write |
| native flush failure | invalid_data / file.flush |
| pre-commit write-close failure | invalid_data / file.close |
| replacement error or indeterminate result | invalid_data / file.replace |
| cleanup warning after committed result | invalid_data / file.cleanup (warning) |

Path validation precedes limit validation, then I/O stages in order. First failure
is primary; a subsequent cleanup failure is additional explanatory context and
recovery path, not permission to overwrite the primary code/path. Core allows a
standalone warning Diagnostic; no warning is put in a failed core::Result.

## App handoff and IO-001 clarification

Read failures preserve the whole app session per IO-001. Before calling replace,
app prepares encoded bytes, candidate saved baseline/path and result-handling
storage. Only committed advances baseline/path and marks successful save.
Cleanup warning after commit is shown separately without pretending save failed.
Not_committed retains dirty state and old baseline/path. Indeterminate also retains
them, reports that disk state is uncertain, and offers explicit recovery/inspection;
it must not promise the old disk file is intact or automatically retry Save.

An explicit later retry/Save As is a new user action, not recovery hidden in the
adapter. These are proposed refinements to IO-001's generic write/replace failure
handling and require affected app/serialization review in the accompanying RFC.
No auto-save, destructive cleanup, backup retention policy or GUI implemented here.

## Required conformance and native evidence

Use synthetic temporary directories owned by each test. Never use personal files.

| ID | Operation / injected event | Expected |
|---|---|---|
| P01 | Empty, relative, embedded NUL, trailing separator/dot paths | file.path before any I/O |
| P02 | Limit0 /67108865; oversized write span | file.limit before any I/O |
| P03 | Missing read file/parent; denied open | file.open; no successful bytes or replacement |
| P04 | Read empty/exact limit/limit+1 bytes | Success empty/exact; file.size for over-limit |
| P05 | Source grows beyond size hint; short reads | Probe enforces bound; concatenated complete bytes on success |
| P06 | Read/close failure after a prefix | file.read; no partial payload |
| P07 | Directory/FIFO/device/final link/reparse point | file.kind; no blocking special-file read or followed target |
| P08 | Save absent destination, then replace existing | Exact supplied bytes at requested path; committed |
| P09 |16 exclusive temp-name collisions | file.create; no truncation/deletion of collisions or target |
| P10 | Write fails after prefix or reports zero progress | not_committed/file.write; old file exact |
| P11 | Short writes then success | All bytes written once in order; committed |
| P12 | Flush then close failure separately | not_committed/file.flush or file.close; old file exact |
| P13 | Replacement failure known unchanged | not_committed/file.replace; old target intact |
| P14 | Replacement failure with uncertain or changed native state | indeterminate/file.replace; recovery retained, no auto retry |
| P15 | Cleanup failure before commit | Primary error kept; temp path reported, target untouched |
| P16 | Cleanup failure after successful commit | committed plus warning; app may mark saved |
| P17 | Allocation failure before temp/commit; post-commit outcome path | No destination change before commit; no hidden completed save from later allocation throw |
| P18 | Unicode/spaces and supported long native paths | Exact selected path; no locale/truncation substitution |
| P19 | Hard-linked target; locked/read-only target | Replace named entry only; native permission/sharing failure explicit |
| P20 | Private file wrapper fault sequence | Exactly one commit attempt; cleanup never targets destination |
| P21 | Native reader during repeated successful replacement | Complete old or new bytes, never partial content on declared supported filesystem |
| P22 | App outcomes committed/not_committed/indeterminate | Baseline/path updated only for committed; uncertainty visible |

Future implementation evidence must record native OS/filesystem, commands, injected
stages and actual results. Test private wrappers against the same outcomes and run
real native temporary-file checks on all three systems. No current runtime results
or universal filesystem guarantee are supplied by this documentation task.
