# SimPHY 4.2 — Research Map (decompilation-based feature survey)

Purpose: a supplied feature survey of **SimPHY 4.2.2412.330**
so an independently implemented, open-source compatible application can be
built. Focus is the **executable / decompiled source**, not GUI screenshots
(see `evidence/` for the few captures taken).

## How to read this
1. **Start here** (`README.md`) — what SimPHY is, how the evidence was gathered.
2. **`FEATURES.md`** — the master checklist with **stable IDs** (the contract).
3. Per-subsystem detail docs (the "why" and full API/param lists).
4. **`evidence/`** — raw artifacts (XML inventory, archive manifest, screenshots).

Evidence labels and verification limits are defined in
[EVIDENCE_REVIEW.md](EVIDENCE_REVIEW.md). The feature index now distinguishes
reported findings (R), inspected examples (E), supplied inventory/manifest
evidence (M), inference (I), and unknowns (U). These are evidence classifications,
not compatibility test results. No entry currently has measured runtime evidence.
Legacy "OBSERVED" labels in subsystem prose mean reported observations by the
research provider unless the review explicitly identifies a checked artifact.

## What SimPHY is
A **Java Swing** desktop **physics simulator & authoring tool** (Windows install
at `C:\Users\Dell4\AppData\Local\SimPHY\`). In one app it combines:
- a **2D rigid-body physics engine** (modified **dyn4j** / Box2D v2 port),
- a **3D physics engine + scene graph** (Oimo.js port + THREE.js-style renderer),
- a **circuit simulator** (DC/AC, analog + digital),
- **geometrical optics** (ray tracing, lenses, mirrors, dispersion),
- a **dependent 2D constructive geometry** system (GeoGebra-like),
- **SPH fluid/particle** simulation,
- an embedded **JavaScript engine** with a large API (build UIs, 2D/3D scenes,
  circuits, games),
- **live graphing**, a **virtual lab table** (formulas, statistics, regression),
  **assessment** questions, snapshots, and import/export (incl. →Java code).

Simulations are saved as **`.ssim`** (a ZIP containing `simulation.xml` +
assets). 67 example simulations are bundled and were used as the primary
file-format evidence.

## Evidence & method
| Source | What it gave us |
|---|---|
| `simphy.jar` (7.8 MB, ~2192 classes) decompiled with **CFR 0.152** → `decomp/src/` (1592 `.java`) | Engine, all subsystems, persistence, UI, tool catalogue |
| `scripts.jar` → `decomp/src_scripts/` (`simphy.script.*`) | Full JS API surface + bundled JS engine |
| `resources/languages/messages_english.properties` (3564 lines) | Every menu/tool/dialog/tooltip string → tool & feature index |
| 67 bundled `.ssim` examples | `xml_inventory.json` (all XML elements/attrs/types), `archive_manifest.json` (hashes, entries) |
| `Major changes` file in jar | dyn4j modification changelog |

Decompiler: `cfr.jar` (v0.152). Decompiled trees: `decomp/src/`,
`decomp/src_scripts/`. Extracted jar resources: `decomp/simphy/`.
Extracted example fixtures: `research/fixtures/extracted/`.

These source-tree paths describe the provider's research environment; `decomp/`
is not supplied in this repository. The local package contains manifests for 67
archives and six extracted XML projects, not all 67 original archives. Code-derived
claims therefore remain reported findings, not independently inspected source.

## Documents
| File | Covers |
|---|---|
| `ARCHITECTURE.md` | Runtime, package map, libraries, 2D/3D/circuit/geometry split, step pipeline |
| `FILE_FORMAT.md` | Working `.ssim`/XML survey; incomplete semantics and version coverage are identified |
| `PHYSICS_2D.md` | dyn4j engine, solver settings, bodies, shapes, all joints, fields, controllers, SPH, tracers, dyn4j patches |
| `PHYSICS_3D.md` | Oimo 3D engine, 3D joints/geometry, World3D API, THREE scene, script-driven 3D |
| `CIRCUITS.md` | MNA solver, all circuit elements + params, AC/DC, logic gates, serialization |
| `OPTICS.md` | Ray tracing, all optical elements + params, dispersion, image construction |
| `GEOMETRY.md` | Dependent 2D geometry, full object catalogue, conics, curves, vectors, measures |
| `SCRIPTING.md` | JS engine, all global objects & APIs, 2D/3D/circuit/geometry/canvas/widget APIs |
| `GUI_WIDGETS.md` | Scripted widget system, `GuiXML`, action/perform callbacks |
| `GRAPHING_DATA.md` | Live grapher, plottable quantities, lab table, statistics, regression, export |
| `LAB_ASSESSMENT.md` | Virtual lab, CSV, assessment question types, snapshots |
| `EDITOR_TOOLS.md` | All 47 tools, 6 toolbar tabs, menus, context menus, shortcuts, preferences |
| `FEATURES.md` | **Master feature map with namespaced stable IDs** |
| `EVIDENCE_REVIEW.md` | Corrections, evidence distinctions, verification gaps and ID migration |

## Evidence artifacts
- `evidence/xml_inventory.json` — every XML element path + observed attributes/
  values + `xsi:type` sets across all 67 examples (759 KB).
- `evidence/archive_manifest.json` — per-`.ssim` sha256, size, ZIP entries.
- `evidence/E00x-*.jpg` — targeted UI captures (settings, collision, constraint,
  file/snapshot/simulations/view menus, upgrade dialog).
- `evidence/inventory_examples.py` — script that generated the inventory.
- `fixtures/extracted/` — unzipped example contents (XML + assets).

## Version / identity
- **App:** reported SimPHY 4.2.2412.330; manifest versions: 57 × 4.0, 3 × 4.1,
  and 7 × 4.2. The six locally extracted XML projects are five 4.0 and one 4.1.
- **Main class:** `org.shikhar.simphy.SplashWindow`.
- **Engine:** modified dyn4j (2D), Oimo.js port (3D), THREE-style scene (3D gfx).
- **JS:** self-contained ECMAScript engine (`simphy.script.js.*`) + `RhinoJsProvider`.
- **Math/expr:** exp4j. **UI:** FlatLaf, RSTA. **Graphs:** XChart. **LaTeX:** JLaTeXMath.
- **Author/brand:** SWE Studio (maheshkurmi).

## Stable ID namespaces
Research identifiers use `SIMPHY-<category>-<number>`, for example
`SIMPHY-PHY-001` and `SIMPHY-FILE-018`. Categories retain their original meanings:
PHY physics, OBJ objects, EDITOR tools, UI widgets, VIS rendering, ANALYSIS data,
FILE persistence, SCRIPT scripting, OTHER miscellaneous. Defined in FEATURES.md.

The migration preserves every category and number: an old research identifier X
maps to `SIMPHY-X`. Old unqualified identifiers are historical research aliases
only; they must never be used as task references. Repository task IDs and future
`REQ-COMPAT-###` requirement IDs remain unchanged. Findings, requirements, and tasks
are separate records linked explicitly, not interchangeable identifiers.

## Status
The supplied survey is broad enough for planning. Completeness and runtime parity
are not established. See EVIDENCE_REVIEW.md and the remaining open items in
FEATURES.md before treating a finding as an implementation requirement.
