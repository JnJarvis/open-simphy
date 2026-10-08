# Native particle document codec v1

Status: PROPOSED under IO-001. Requires independent review before implementation.
Owner: serialization. Consumers: app and tests. Allowed internal dependencies:
core, math, scene. No filesystem, SDL, editor, physics, clocks or platform types.

## Scope and owning port

Persist the accepted scene v1 authoring document, in SI units with +y up. Preserve
schema revision, gravity, every particle field, IDs and high-water mark. No runtime
time/position, solver cache, selection, history, camera, DPI or drag preview is saved.
Display radius remains presentation data, not collision geometry. This is an
independent native format, not a SimPHY format or compatibility promise.

Proposed header: `opensim/serialization/document.hpp`, namespace
`opensim::serialization`, target `opensim::serialization`:

```cpp
[[nodiscard]] core::Result<std::vector<std::byte>> encode(const scene::Document &);
[[nodiscard]] core::Result<scene::Document> decode(std::span<const std::byte>);
```

Both calls are synchronous and stateless. Inputs are borrowed only for the call;
success and diagnostics own all their data. Failure returns one error and no partial
document/bytes; neither call mutates inputs or application state. Independent calls
may run concurrently with immutable inputs. Allocation failures may propagate
`std::bad_alloc`; do not catch them to allocate a diagnostic. No public mutable
document, stream, callback or configurable parser limits in v1.

## Encoding and versioning

Suggested native extension `.osim`; recognition uses bytes, never the extension.
Every integer is unsigned and little-endian, independent of host byte order.
Every scalar is the little-endian 64-bit IEEE-754 representation of the accepted
binary64 value. Emit/read individual bytes with checked offsets; never serialize
struct memory, enum ordinals, pointer values, compiler padding or native `long`.
Require eight-bit bytes and the already required binary64 representation at build
time. Scalar conversion preserves bits (for example via `std::bit_cast`), not
decimal conversion or floating-point arithmetic.

The fixed header is 48 bytes. There is no BOM, padding, compression or checksum.

| Offset | Bytes | Field |
|---|---|---|
| 0 | 8 | Magic: `4f 50 53 49 4d 44 4f 43` (ASCII `OPSIMDOC`) |
| 8 | 4 | Wire format version, exactly 1 |
| 12 | 4 | Scene schema revision, exactly 1 |
| 16 | 4 | Particle count N, 0 through 4096 |
| 20 | 4 | Reserved, exactly zero |
| 24 | 8 | Document high-water mark |
| 32 | 8 | Gravity x |
| 40 | 8 | Gravity y |

Each particle record is exactly 56 bytes, beginning at `48 + 56*i` for
zero-based index i. No per-record tags or omitted/default fields:

| Relative offset | Bytes | Field |
|---|---|---|
| 0 | 8 | EntityId |
| 8 | 8 | Mass |
| 16 | 8 | Initial position x |
| 24 | 8 | Initial position y |
| 32 | 8 | Initial velocity x |
| 40 | 8 | Initial velocity y |
| 48 | 8 | Display radius |

Length is exactly `48 + 56*N`, maximum **229424 bytes**. No trailing bytes, second
document, extension chunks or comments. Count is checked before multiplication,
allocation or record access. Codec supports only 4096 particles even if a valid
scene Document contains more; this is a codec limit, not a change to scene validity.

Wire version and scene revision are distinct. Either unsupported value fails;
do not attempt v1 decoding of a future version or guess a migration. Future fields
require a new wire version with a reviewed migration. Existing v1 bytes retain
their meaning. Reserved nonzero is unsupported, not silently ignored. No optional
fields or lossy partial loading. There are no migrations in this first version.

The lack of checksum means arbitrary corruption that still forms valid data may
be accepted. Structural/semantic validation detects invalid data, not all damage;
make no integrity, authenticity, crash-durability or recovery guarantee from this
codec. A later checksum/version proposal must retain explicit v1 decoding.

## Numeric and identity invariants

Encode validated particles in ascending numeric EntityId order. Decode permits
arbitrary input order and delegates normalization and semantic validation to
`scene::Document::create`. IDs are full uint64, never passed through double; values
above 2^53 and UINT64_MAX must survive exactly. Duplicate/zero IDs and a watermark
below the maximum ID are rejected by scene. An empty document may retain any
watermark, including UINT64_MAX. Never rebuild watermark from only visible IDs.

All finite scalar bit patterns accepted by scene round-trip exactly, including
signed zero and subnormals where valid. Mass/radius must remain strictly positive.
NaN/infinities and zero/negative mass/radius are rejected through scene validation.
No rounding, epsilon, clamping, unit conversion or implicit gravity default.
Codec tests use bit comparisons, because ordinary equality cannot distinguish
signed zeros. No new scalar policy is imposed on scene or math.

For supported documents D, `decode(encode(D))` preserves scalar bits, IDs,
watermark, revision and normalized order. Repeated encode of D returns identical
bytes across the supported platforms. For accepted bytes B, `encode(decode(B))`
normalizes record order; byte identity with B is required only when B was already
in ascending-ID order. No arithmetic is performed on scalar payloads by the codec.

## Bounded parsing and diagnostic precedence

Check these stages in order, without reading fields before their bytes exist:

1. Input size greater than 229424: invalid_data / `file.size`.
2. Input size less than 48: invalid_data / `file.size`.
3. Magic mismatch: invalid_data / `file.magic`.
4. Wire version not 1: unsupported_feature / `file.version`.
5. Scene revision not 1: unsupported_feature / `schema_revision`.
6. Reserved nonzero: unsupported_feature / `file.reserved`.
7. Count greater than 4096: invalid_data / `file.count`.
8. Size not exactly 48+56*N: invalid_data / `file.size`.
9. Read bounded records, then call Document::create with decoded revision, gravity,
   particles and supplied high-water. Propagate its entire diagnostic unchanged.

Codec-origin errors have severity error, no entity, explanatory owned message and
the stable code/path above. Scene owns semantic error precedence; tests isolate
each invalid semantic field. Structural stages have the explicit order above.
Encode rejects count >4096 with invalid_argument / `file.count`, no entity, before
allocating output. Other scene-invalid inputs cannot enter through Document's
public constructor. No new core code or reinterpretation of existing scene paths.

The codec allocates O(N) data, reads only within the span, and performs no unbounded
recursion, compression, external lookup or input-directed allocation. Scene's
existing O(N log N) sorting is allowed. Input bytes and a bounded decoded vector
may coexist; do not promise an exact heap-byte budget. File adapters must also cap
reads rather than reading an unlimited file before invoking this bounded codec.

## Proposed app transaction (separate integration task)

Only the app coordinates editor, World and file adapter. No dependencies are added
between serialization and editor/physics/platform. The codec never opens a path.

Open is permitted only in Authoring mode with no active drag or uncommitted numeric
text edit. Reject otherwise before I/O (`invalid_argument/app.mode`, then
`invalid_argument/app.gesture`, then `invalid_argument/app.property`). UI explains
Reset/finish/cancel as appropriate. No implicit gesture commit or snapshot adoption.
The integration task must provide explicit user confirmation before discarding
unsaved authoring data; cancellation does nothing. Serialize operations on the app
owner thread, including any confirmation, so approval cannot apply to stale edits.

Read bounded bytes, decode, create a fresh editor State using the current validated
View, and prepare a World at dt=1/128. Prepare all owning copies and the saved-byte
baseline before replacing any app state. Any failed stage preserves the original
document, history, selection, view, World, path, saved baseline and last good frame.
Successful Open publishes the complete candidate in stopped Authoring mode at t=0,
empty selection/history/gesture, zero accumulated time and a new document-session
identity. Clear the old scene's retained frame so it cannot masquerade as the new
document. Subsequent render failure reports an error, not a silent rollback of load.

New session watermark comes from the loaded file; do not merge it with the previous
document's watermark. Numeric IDs can repeat across sessions. App must invalidate
old selections/handles/history instead of treating equal IDs as the same entity.
This does not weaken monotonic IDs within an editor session. Reopening the same
file also establishes a new session. View is retained; it is not persisted.

Save is allowed in Authoring or Simulation, and always serializes the committed
authoring document. Reject active drag or uncommitted text using the paths above.
It never serializes advanced runtime positions, resets the simulation, or clears
history. Encode fully before touching the destination. Save As path changes only
after successful replacement. Cancellation/encode/write/replace failure preserves
the previous saved baseline/path and must not report successful save.

Dirty state compares the current canonical encoded authoring bytes with the last
successful load/save baseline; use full comparison, not hash alone or Document's
ordinary floating equality. No baseline means unsaved. Loading unsorted input uses
re-encoded canonical bytes as baseline. View/selection/playback are not dirty;
watermark changes are dirty, even after undo restores visible content. Never mark
an unencodable document clean. Confirming discard does not itself clear dirty state.

File adapters require a separate reviewed platform contract: bounded reads even if
file size changes, exclusive temporary creation in the destination directory,
checked full write/flush/close and a platform-appropriate atomic replacement with
failure cleanup that cannot delete the original destination. No delete-then-rename
fallback. Only report save success after replacement. Permissions, links, locks,
overwrite confirmation, paths and durability details belong to that contract and
native adapter tests. Do not claim crash-safe persistence from portable streams
or this proposal. No filesystem adapter is authorized by IO-001 alone.

## Independent conformance vectors

Synthetic data only. All integer/byte/bit assertions exact; no float tolerances.
Implementers must use literal byte specimens, not only encoder-to-decoder tests.

| ID | Input/action | Expected |
|---|---|---|
| F01 | Empty revision1, H0, gravity(+0,-1) | Exact 48-byte specimen below |
| F02 | One particle ID1 H1, mass2, position(1,-2), velocity(0,.5), radius.25, gravity(0,-1) | Length104; exact record below |
| F03 | Decode F02 then destroy/overwrite input buffer | Independent values remain valid |
| F04 | Encode/decode ID9007199254740993 and H18446744073709551615 | Both exact; later scene add returns id_exhausted |
| F05 | Empty H17, then round-trip and scene add | Next ID18, not1 |
| F06 | Two records IDs9,2 in input, H10 | Decode order2,9; re-encode sorted; H10 preserved |
| F07 | Finite minimum subnormal, maximum finite, signed zeros in permitted fields | Exact bits preserved; choose positive mass/radius |
| F08 | Independently encoded NaN/+inf/-inf in each scalar location | Scene invalid_data, correct scene field/entity; no document |
| F09 | ID0; duplicate ID2; H1 with ID2 | Scene invalid_data/particles.id; duplicate_id/particles.id; invalid_data/high_water |
| F10 | Zero/negative mass or radius, including -0 | Scene invalid_data with affected field/entity |
| F11 | Every proper byte prefix of F02, including empty | file.size; no partial record access |
| F12 | F01 plus one trailing byte; concatenated F01 twice | file.size |
| F13 | F02 with each magic byte corrupted in turn | file.magic |
| F14 | Version0 or2 with otherwise valid F01 | unsupported_feature/file.version |
| F15 | Scene revision0 or2 | unsupported_feature/schema_revision |
| F16 | Reserved1 or0x80000000 | unsupported_feature/file.reserved |
| F17 | Count4097 orUINT32_MAX in otherwise 48-byte header | file.count before size arithmetic/allocation |
| F18 | Valid4096 records, IDs1..4096 | Exact229424 bytes, successful round-trip |
| F19 | Valid scene4097 particles to encode; decode229425 bytes | invalid_argument/file.count; invalid_data/file.size |
| F20 | Count2 with exactly104 bytes | file.size, no second-record read |
| F21 | Bad magic and version2; valid magic/version2 with excessive count | file.magic; file.version (precedence) |
| F22 | Change a valid positive mass bit to another valid value | May decode; no checksum/integrity claim |
| F23 | Same document encoded repeatedly/on all native CI platforms | Same literal bytes, unaffected by locale |
| F24 | Future app: Open invalid file or inject editor/World preparation failure | Entire previous app state/path/baseline retained |
| F25 | Future app: successful Open when old session H100, file H3 | New history/selection empty, t0, next add ID4 |
| F26 | Future app: save while paused/running at t>0 | Saves authoring initial fields; runtime/history unchanged |
| F27 | Future app: cancelled Open/Save As; write/replace failure | Old document/path/baseline retained; destination preserved on reported pre-replacement failure |
| F28 | Future app: add, undo; change view/playback only | Watermark keeps first dirty; second does not change dirty state |
| F29 | Future app: load unsorted valid bytes then no edit | Clean against canonical encoded baseline |
| F30 | Future app: active drag/text or Simulation Open | Reject pre-I/O with specified app diagnostic; no side effects |

F01, six rows of eight bytes (48 total):

```text
4f 50 53 49 4d 44 4f 43
01 00 00 00 01 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 f0 bf
```

F02 uses F01 header with count1 at offset16 and H1 at offset24, then this record:

```text
01 00 00 00 00 00 00 00
00 00 00 00 00 00 00 40
00 00 00 00 00 00 f0 3f
00 00 00 00 00 00 00 c0
00 00 00 00 00 00 00 00
00 00 00 00 00 00 e0 3f
00 00 00 00 00 00 d0 3f
```

## Delivery gates

After independent acceptance, scope codec implementation and headless CMake/test
registration together; require F01-F23, header isolation and native compiler matrix.
Separately scope platform file contract/adapters and app integration with F24-F30,
failure-injected file replacement and actual native save/reopen smoke. File pickers
and polished UI remain integration choices. Do not add persistence commands to
today's executable until these prerequisites are reviewed and merged.
