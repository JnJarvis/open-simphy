# IO-001 RFC: bounded native authoring documents

Status: ACCEPTED by user, 2026-10-07. Native format remains separate from the required SimPHY-compatible import support.
See the [complete format and vectors](../spec/contracts/serialization.md).

## Problem and proposed decision

The editor can author a scene but cannot preserve work between launches. Saving
raw C++ objects would encode platform layout; saving runtime snapshots would lose
initial conditions and ID allocation state. A failed Open must not destroy the
current session, and a failed Save must not truncate the previous file.

Propose a small fixed binary v1 codec for the existing particle document. A 48-byte
header and 56-byte records preserve full uint64 IDs and binary64 bits with explicit
little-endian order. Limit to4096 particles /229424 bytes, reject unknown versions,
reserved fields and trailing bytes, and reuse scene validation. The codec is pure
bounded bytes-to-values; app and later platform adapters own replacement and files.

## Alternatives and limits

- JSON is easier to inspect, but requires a parser/dependency decision plus precise
  duplicate-key, numeric-token and uint64 conventions. Binary v1 avoids rounding IDs
  through double and makes exact float and byte-size tests straightforward. This is
  a project choice, not a claim that JSON cannot preserve these values. A future
  explicit text export can coexist without changing v1 meaning.
- Raw struct dumps are rejected because padding, alignment and endianness vary.
- Archives, compression, chunk extensions, scene graphs and asset bundles add
  resource/security/versioning complexity unrelated to the current particle model.
- No checksum is included. Validation rejects malformed or semantically invalid
  input; valid-looking bit corruption may survive. Integrity/recovery needs a later
  version proposal. The format is neither authenticated nor encrypted.
- No automatic migration or downgrade. Future versions must have explicit reviewed
  readers/translators; unsupported content fails without partial loading.
- Persisting history/camera/runtime would complicate session identity and model
  evolution. v1 saves authored physics values and allocation watermark only.

## Affected-contract review

| Owner/consumer | Existing contract | Proposed effect |
|---|---|---|
| core | uint64 ID, owning Result/Diagnostic | Unchanged; no new code or persisted enum ordinal |
| math | binary64 scalars, finite scene policy | Unchanged; codec bit preservation is stronger than numeric equality |
| scene | revision1, normalized IDs, validated values and high-water | Unchanged; decode calls existing create and propagates errors |
| serialization | architectural placeholder | New owning codec API, fixed versioned bytes and resource limits |
| editor | monotonic IDs within session; create clears history | Unchanged API; explicit successful Open starts a new session |
| physics | World from authoring document at app dt | Unchanged; candidate World created before app replaces state |
| renderer | immutable snapshots, retained output in app | Unchanged; successful Open invalidates old scene frame in app |
| app | owns editor/World, no persistence | New transactional Open/Save, dirty baseline, discard confirmation and path ownership |
| platform | architecture permits bounded file adapters | Requires separate file-port contract/native implementation before app persistence |
| build/tests | explicit target allowlist, headless modules | Later codec task adds serialization headlessly and F01-F23 checks |
| compat | evidence-gated external translation | Unchanged; native bytes establish no external compatibility evidence |

No dependency edge changes or third-party dependency. Proposed app behavior needs
integration-owner review as part of acceptance. Platform requirements here are
constraints on a future contract, not an unreviewed file API. No existing C++ header,
scene semantic, physics equation or test expectation changes in IO-001.

## Critical review points

Verify little-endian offsets and literal specimens independently; maximum length;
strict size/version/count precedence; exact signed-zero/subnormal/large-ID round
trips; empty high-water persistence; normalization versus byte identity; rejection
without publishing partial values. Runtime tests are required in implementation,
not claimed by this documentation task.

For app review, verify failure retains all original state, Save uses committed
authoring data even in Simulation, Open starts a new ID namespace, dirty comparison
includes watermark, pending edits cannot be silently lost, and saved path/baseline
changes only after successful replacement. Atomic replacement and crash durability
are different guarantees; native file adapter behavior must be specified and tested.

## Sequencing after acceptance

Coordinator records acceptance in an ADR and scopes: native codec implementation
with headless build registration; file-adapter contract then native adapters; and
an integration task consuming only their accepted merged outputs. Implementing
codec code does not by itself enable Save/Open. Review each change independently.
No current format users require migration: this is the first native-file proposal.
