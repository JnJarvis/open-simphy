# COMPAT-004 — SSIM opening and rigid-body migration

Status: PROPOSED for affected-contract review, 2026-10-08.
Product direction: an open-source replacement capable of opening existing SimPHY
projects. The current particle workspace is a foundation, not the product scope.
Preserve the accepted module DAG; prioritize compatibility over cosmetic UI work.

## Evidence and limits

COMPAT-003 independently inspected 67 installed ZIP archives with simulation.xml,
serialized versions 4.0/4.1/4.2, and verified hashes against the supplied manifest.
See research/SSIM_INSPECTION.md and evidence/ssim-inspection.json. This establishes
containers and syntax, not behavioral equivalence. FILE_FORMAT.md sections 5.1–5.5
report body transforms, mass/inertia, compound fixtures, filters and joints.
EVIDENCE_REVIEW.md distinguishes supplied reports from direct observations.
Directly inspected extracted examples include Newton_Cradle (DistanceJoint),
Resonance_in_Action (LineJoint/SpringJoint), and Rotation_Toppling (Plane,
Rectangle/Circle). These observations justify model expansion, not guessed defaults.
Existing broad COMPAT-001/002 research gates remain unchanged. This narrow RFC
uses the separately reviewed COMPAT-003 evidence; no global parity gate is waived.

## Proposed pipeline and ownership

platform bounded file read -> compat archive/XML source document -> compat
capability assessment and translation -> validated scene -> app atomic replacement.
compat depends only on core/math/scene internally; ZIP/XML libraries remain private
implementation dependencies, pinned and license-reviewed before implementation.
Choose maintained archive/XML libraries in the parser implementation task; do not
write a bespoke compression decoder or let third-party nodes escape public APIs.
Raw source representation belongs to compat, not the canonical simulation model.
No toolkit, external schema retrieval or script execution in parsing/translation.

The import source document owns the original bounded archive bytes, member table,
XML representation, external identity strings and asset references. Original data
survives unsupported features for inspection and future translation. This does not
promise an editable lossless export; original archives are never overwritten.
IDs map deterministically to canonical IDs, retaining source-to-canonical mappings.
Reject duplicate/dangling identities; never merge different UUIDs by truncation.

## Opening behavior

Open .ssim through a real file action (later also command line/drop). Parse and
validate into temporary state. A failure leaves the active document, camera,
selection, history, dirty state and runtime untouched. Offer unsaved-change handling
before replacement once persistent editing is wired. Cancel is not a parse error.

Successful parsing yields one of these explicit states:
- Supported: required geometry/physics/settings translate completely into an
  available engine profile. Open paused; Reset restores the imported authored state.
- Preview only: retain source, show supported geometry/assets plus a conspicuous
  feature report. Disable Play and simulation edits if required behavior is missing.
  Missing visuals must be listed; show placeholders where location is known.
- Rejected: corrupt/unsafe/ambiguous input; no replacement of active state.

A parse success is never called simulation compatibility. Every unsupported
required field, body type, shape, joint, script or solver setting prevents a
supported classification. Unknown fields are conservatively unresolved until
classified; do not ignore them because one example seems to work. Informational
metadata may be retained without preventing playback only under a reviewed allowlist.
Report version, source member and XML location, object ID/name, feature, reason and
severity. Group repeated issues without hiding affected object counts/identities.
Changing body geometry into particles, disabling collisions, or dropping joints to
make a scene runnable is prohibited. Preview is interim functionality, not acceptance
of the user's drop-in replacement requirement.

## Bounded archive and asset contract requirements

Initial budgets match the independently tested inspection envelope: 64 MiB input,
4096 entries, 128 MiB total expansion, 8 MiB XML, depth 128, 200000 XML elements,
1000:1 member expansion ratio. Enforce during streaming, not just declared sizes.
Require exactly one root simulation.xml; validate CRC, lengths and compression.
Initially allow stored/deflate; reject encryption, links, duplicate/ambiguous member
names, absolute paths, parent traversal, drive paths and backslash aliases.
No extraction to disk, external URLs or schema/DTD/entity expansion. Reject ZIP64
until separately covered. Asset references resolve only within the member table;
missing references produce diagnostics. Image decoding also needs dimension/pixel
budgets independent of archive size. Account for parser allocations and report
budget exhaustion; numeric fields must reject NaN/infinity/overflow.
Never infer .sim is .ssim from its suffix; additional formats get their own evidence.

## Affected-contract review: required migration

scene: introduce real 2D rigid bodies and compound fixtures alongside existing
particles. Body pose includes translation and radians rotation; velocity includes
angular velocity. Define local center of mass, mass/inertia policy, body mobility,
forces/torques, gravity scale and collision participation. Fixtures carry local
geometry/transform, density, friction, restitution, sensors and filtering.
First shapes: circles and convex polygons (rectangles are exact polygons). Plane,
concave, ring and other geometry must retain their identity and be unsupported
until an exact model or explicitly reviewed decomposition exists. Do not fill holes.
Specify fixed-linear/fixed-angular modes from evidence before translating them.

physics/collision/constraints: rigid-body integration, broad/narrow phase, contact
manifolds, impulses, friction/restitution and stability/CCD policy precede runnable
collision imports. Joint definitions carry validated body references and local
anchors. Implement distance/revolute first, then springs and prismatic/line support
as evidence allows. Solver names in XML do not imply equivalent native behavior.

renderer/editor: render/pick actual transformed fixtures, rotate/translate bodies,
edit physical properties atomically, preserve undo/reset and imported identities.
Publish immutable rigid-body snapshots; keep viewport/grid app-owned. Assets and
render style have distinct status from physical behavior.

serialization: retain .osim v1 particle decoding and exact existing fixtures. Add
an explicitly versioned native schema for bodies, fixtures, joints and assets;
never reinterpret v1 fields or silently save a body scene through a particle codec.
Define migration and resource ownership before enabling Save for imported scenes.
The custom format remains independent of SimPHY XML and designed for bounded,
fast native decoding; benchmark real body/asset workloads after the schema exists.

app/platform: portable read/dialog adapter and staged document load; app composes
capability report, preview mode and runnable mode. Linux/macOS reuse importer and
engine; native file UI stays behind the platform boundary.

All above are proposed changes, not accepted replacement contracts. Each affected
owner must review before producer implementation; existing particle contracts/tests
remain valid. No new dependency edges or silent schema changes are authorized here.

## Script and wider parity policy

Preserve scripts as data. Nonempty or unresolved scripts make initial imports
preview-only; do not assume they are cosmetic. Later compatibility requires a
scoped execution environment, supported API/lifecycle mapping and behavioral tests.
3D, optics, circuits, geometry, graphs/widgets and assessments remain product goals,
with explicit capability rows and dedicated domain models. They cannot be represented
by pretending all source content is a 2D mechanical scene.

## Acceptance for later implementations

Container: malformed, truncated, CRC, duplicate/path, missing XML, XML entity/depth,
resource exhaustion and cancellation tests; independently authored ZIP fixtures.
Translation: exact transforms/units/IDs, multiple fixtures, filters, settings,
assets, references and unsupported-field tests; no partial publication.
Mechanics: analytic free-body/rotation, contact/resting/stacking/restitution/friction,
joint length/limits and reset tests with stated tolerances, not visual similarity.
Real corpus: local supplied archives identified by SHA-256, version and all feature
statuses. Record parse/preview/run separately. Original samples stay outside Git
unless rights are established; use synthetic equivalents for redistributable CI.
Behavioral parity requires recorded SimPHY reference conditions and measurements;
the current syntax research alone is insufficient. No unqualified compatibility
claim based on a screenshot or successful ZIP extraction.
