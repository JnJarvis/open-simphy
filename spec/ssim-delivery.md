# Compatibility-first delivery sequence

Proposed 2026-10-08 under COMPAT-004. See
[SSIM migration RFC](../rfcs/COMPAT-004-ssim-opening.md).
This refines scheduling inside the existing architecture and staged roadmap.
Full SimPHY replacement remains the destination; particle-only editing is not done.

| Order | Reviewable work | User-visible result / acceptance |
|---|---|---|
| 1 | Review source-document/import contract and rigid-body scene migration | Agreed representation for actual .ssim content; no geometry-to-particle shortcut |
| 2 | Bounded ZIP/XML reader, feature report, platform read and Open integration | Open an actual archive, identify all unsupported content, preserve current document on failure; preview is explicitly non-playable when incomplete |
| 3 | Rigid-body scene/snapshot/editor/renderer implementation | Circles and compound convex shapes with pose, rotation and physical properties display and edit correctly |
| 4 | Collision/contact solver, friction/restitution and rigid-body integration | First genuinely playable imported 2D collision scenes, with measured reference tests and declared supported profile |
| 5 | Distance/revolute constraints, springs, prismatic/line semantics | Progress toward pendulums, cradles and spring experiments; no claim that these work until every required feature, including scripts, is supported |
| 6 | Native body/asset format, save/reopen and source preservation | Imported editable projects persist efficiently in the independent native format; existing particle files remain readable |
| 7 | Evidence-led scripting, remaining joints/shapes, fields, plotting/widgets, optics/circuits/geometry/3D | Expand the versioned parity matrix until the declared drop-in scope is met |

Work on the difficult shared model/solver boundaries before low-risk UI polish.
Parser work and rigid-body design can progress independently after their respective
contracts are reviewed. Native IO platform work can also proceed without physics,
but app integration waits for merged producers. Implementation tasks must get exact
allowed paths, contract versions, fixtures and acceptance checks when scheduled.
These rows are proposals, not READY implementation claims or fictional completion.

## Required progress reporting

Maintain per-format/version capability rows with separate states for container read,
geometry preview, editable translation, simulation behavior and round-trip saving.
For each sampled archive, show bodies/fixtures/joints/assets/scripts discovered,
unsupported feature details and actual validation evidence. Never report a project
as supported simply because simulation.xml parsed.

The current 67-archive corpus covers .ssim, serialized versions 4.0/4.1/4.2.
.sim and every other SimPHY-openable format remain explicit goals with separately
identified evidence and loaders. No extension aliasing is assumed.

## Next hard deliverable

The migration RFC is ready for affected-contract review. Its first implementation
slice must open real .ssim archives and report their features, while the rigid-body
contracts unlock genuine scene translation. A playable milestone requires the
collision solver and complete required-feature coverage; a static viewer alone
is not a drop-in replacement.
