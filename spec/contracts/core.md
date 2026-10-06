# Core values contract v1

Status: ACCEPTED under CORE-001 following independent user review, 2026-10-06.
Owner: core. Consumers: scene and subsequent domain/application modules.
No internal or external library dependencies beyond the C++ standard library.

## Entity identity

`EntityId` is an unsigned 64-bit value. Zero is the invalid sentinel. Valid values
range from 1 through 18446744073709551615. Equality and ordering compare values;
there is no pointer identity, platform UUID API, OS handle or dependence on `long`.

Uniqueness is per canonical document, across its entity kinds. The document owns
one ID allocator; separate documents may have equal IDs. A consumer addressing
multiple documents must also carry document identity, not compare entity IDs alone.
Core provides values/allocation, not an entity registry. Scene rejects duplicate
IDs when constructing or validating a document. Copying a value preserves its ID;
duplicating a scene entity obtains a new ID and remaps references in scene logic.

External format IDs (including SimPHY string UUIDs) are translator-owned keys.
They map to internal IDs; never truncate or hash a UUID into an unchecked integer.
Preserving an external ID belongs to compatibility metadata, not core identity.

Allocator state is a high-water mark (largest assigned/reserved ID), initially 0.
`next()` succeeds with high-water+1 and advances it, unless already at the maximum;
then it returns `id_exhausted`, preserves state and never wraps to 0.
`reserve_through(id)` advances the high-water mark to max(current,id) and returns
success for nonzero IDs, or `invalid_argument` for 0 without changing state.
Reserving an already reserved number is allowed and does not prove uniqueness.
Restoring a document reserves through its largest validated existing ID before
creating new entities. Deleted IDs are not reused by that allocator; resetting
a simulation is not an excuse to reset its document's allocator.

Core does not persist allocator memory; native serialization later defines its
encoding. ID values copy/move freely, outlive their source, and are immutable by
convention once attached to an entity. Allocators are thread-confined; the owning
document serializes mutations. Independent allocators may be used concurrently.

## Structured diagnostics and results

`Diagnostic` owns a stable machine-readable code, severity, UTF-8 explanatory
message, optional EntityId, and optional logical field path such as `body.mass`.
It owns its text, not string views into temporary input. OS error text can be
context, but no Windows errno/handle or native exception type is a public code.
Diagnostics do not display UI, write files, translate strings, or emit logs.

Initial stable codes: `invalid_argument`, `id_exhausted`, `duplicate_id`,
`missing_reference`, `unsupported_feature`, `invalid_data`, `internal_error`.
They have stable spelled names for tests; C++ enum ordinals and memory layout are
not a serialization format. New codes require normal contract review. Severity is
`info`, `warning`, or `error`; diagnostics as standalone values may use any severity.

`Result<T>` is exactly one of success containing T, or failure containing one
error-severity Diagnostic. It never contains both and never reports success after
discarding a required value. `Result<void>` uses an explicit success without a
payload. Accessors permit inspecting the state and safely querying either branch;
querying the inactive branch returns no value, not a dangling reference. No default
construction that accidentally represents successful uninitialized data.

Copying a result copies its payload/diagnostic if T is copyable; move-only T gives
a move-only result. Diagnostic propagation preserves code/entity/path and message.
Any view/reference into a result is valid only while that owning result remains
alive and unmodified. Public APIs mark result-returning operations nodiscard.

Expected invalid inputs return failures; resource exhaustion from allocation may
throw the standard allocation exception. Do not translate bad_alloc into an
allocating Diagnostic or mark operations owning strings unconditionally noexcept.
Invalid result use must not depend on a debug-only assertion for safety. No logging
framework, exception hierarchy, or generic global error registry is introduced.

## Conformance vectors (CORE-002 implements these)

| ID | Input/action | Expected outcome |
|---|---|---|
| C01 | EntityId 0; 1; max | 0 invalid; 1/max valid on every target |
| C02 | Copy ID 42; compare to 42 and 43 | Equal to 42 only; ordered before 43 |
| C03 | Fresh allocator, next twice | Success 1 then 2 |
| C04 | Reserve 10, next; reserve 5, next | 11 then 12; never rewind |
| C05 | Reserve 0 on high-water 7, next | invalid_argument then success 8 |
| C06 | Reserve max-1, next twice | Success max then id_exhausted; state remains max |
| C07 | Two independent allocators each next | Each returns 1; no hidden global coupling |
| C08 | Reserve same ID twice | Allowed; scene must separately reject duplicate entities |
| C09 | Success with integer 0 | Success payload 0, no failure branch; 0 payload is not ID validation |
| C10 | Failure invalid_argument, entity 42, path body.mass | No success branch; code/context/message survive copying and source destruction |
| C11 | Propagate C10 across a result boundary | Identical diagnostic, no silent relabeling or lost entity/path |
| C12 | Void success; failure; querying inactive branch | Explicit state; inactive accessor reports no value |
| C13 | Move-only payload ownership | Cannot copy; moving transfers payload without dangling diagnostic text |
| C14 | Standalone warning Diagnostic passed as failure | Failure construction rejects non-error severity as invalid_argument, not success |

C14 avoids creating an invalid result: a checked failure factory returns the
standard invalid_argument error diagnostic when given a non-error severity.
Scene tests, not core alone, must assert duplicate IDs cannot enter a document.

## Portability and evolution

Values have no units and no clock access. Public headers compile in isolation on
MSVC, GCC and AppleClang. Tests use exact integer/branch comparisons, not binary
layout assumptions. No cross-DLL ABI stability is promised at this stage. Future
ID-width, result-semantic or code changes require an RFC and affected-consumer review.
