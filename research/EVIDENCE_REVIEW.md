# Research corrections and evidence review — 2026-10-06

Scope: correct the supplied research after comparison with its local artifacts.
The project's architecture, roadmap, contracts, task IDs and registry are unchanged.
This is a documentation correction requested directly by the user, not completion
of COMPAT-001 or COMPAT-002. No simulator or compatibility implementation is delivered.

## Evidence categories

| Code | Meaning | What it does not establish |
|---|---|---|
| R | Finding reported by the original research provider; underlying source type may be mixed or unclassified | Independent source inspection or runtime verification |
| E | Direct inspection of a supplied extracted XML example; exact claim and location below | Original archive authenticity, all-version behavior, or runtime semantics |
| M | A supplied inventory/manifest records the claim | Independent re-extraction of all archives or a runtime test |
| I | Interpretation or inference, with rationale | Verified behavior |
| U | Missing or conflicting evidence | Presence, absence, or compatibility |

The original O label combined decompiled source, files and UI observations.
Former O entries now use R unless this review identifies inspected evidence for
the row's claim. This preserves reported findings without falsely upgrading them.
I and U are retained. Embedded OBSERVED labels in subsystem prose are historical
provider assertions governed by this policy; they do not override the index.

For future refinements, record the source kind separately: code, XML, archive,
screenshot, or runtime experiment. Include an exact source locator, version,
method, expected/actual result and uncertainty. A runtime-verification record must
also give initial state, actions, timestep, measurements and tolerances. No such
numerical runtime record is supplied by this correction pass. None of R/E/M/I/U
means compatibility PASS or a DONE task.

## Checked claims and corrections

| Finding | Evidence and locator | Result and limits |
|---|---|---|
| SIMPHY-FILE-001 | evidence/archive_manifest.json, all 67 records | M: records list ZIP magic and simulation.xml members; original archives were not independently inspected |
| SIMPHY-FILE-016 | fixtures/extracted/Charge_oscillation/simulation.xml, /Simulation/World/ScriptManager/Script | E: embedded JavaScript text exists; not executed and API semantics unverified |
| SIMPHY-FILE-018 | evidence/xml_inventory.json paths under /Simulation/World/Settings; all six extracted XMLs at the same path | E + M: 17 named fields, not 18; no universal writer guarantee inferred |
| SIMPHY-FILE-021 | fixtures/extracted/Potentiometer_Experiment/simulation.xml, /Simulation/World/Circuit | E: line-based token payload, not class-name-prefixed records; detailed semantics remain unknown |
| StepFrequency encoding | inventory /Simulation/World/Settings/StepFrequency values; six extracted XMLs | E + M: values are 60/90/144/180 in inventory and 60/144/180 in local fixtures. Reciprocal-storage claim removed. Hz interpretation is consistent with reported UI, not a measured clock test |

The settings fields are StepFrequency, MaximumTranslation, MaximumRotation,
ContinuousCollisionDetectionMode, AutoSleep, SleepTime, SleepLinearVelocity,
SleepAngularVelocity, VelocitySolverIterations, PositionSolverIterations,
WarmStartDistance, RestitutionVelocity, LinearTolerance, AngularTolerance,
MaximumLinearCorrection, MaximumAngularCorrection, and Baumgarte.

The six inspected projects are Charge_oscillation, Newton_Cradle,
Potentiometer_Experiment, Prism_Dispersion, Resonance_in_Action, and Rotation_Toppling.
Newton_Cradle is version 4.1; the other five are 4.0. The archive manifest reports
57 version-4.0, three version-4.1, and seven version-4.2 archives. A 4.2 document
title must not imply that the six inspected fixtures were written by 4.2.

For circuit records, CIRCUITS.md now documents sample lexical structure and
explicitly leaves token meanings, flags, type-specific fields, defaults, escaping,
and version rules unresolved. Supplying a plausible but untested full grammar
would not fix the evidence gap. The source-provider assertion about reflection may
describe another format; it is not used as evidence of this payload's encoding.

## Identifier migration

All 209 feature IDs are qualified as `SIMPHY-<old-ID>`, preserving the original
category and numeric suffix. This is a reversible one-to-one mapping, for example
old research PHY-001 → SIMPHY-PHY-001. All research Markdown references use the
qualified namespace. Repository tasks PHY-001 and PHY-002 retain their existing
meaning. Future REQ-COMPAT identifiers link to these findings and to separate tasks.
Historical unqualified IDs in this migration explanation are not live references.

## Remaining verification work

- Separate source-derived observations from example/UI observations per finding;
  most rows currently have only the provider's subsystem-level references.
- Supply reproducible behavioral measurements for integrators, collisions, joints,
  scripting callbacks and reset/timing semantics before specifying numerical parity.
- Complete the circuit payload grammar using attributable fixtures or provider
  evidence; the current single example is insufficient for all element types.
- Verify missing-field defaults and version differences, particularly 4.2 files.
- Record fixture permission/provenance before redistribution. Manifest hashes identify
  archives but do not establish redistribution permission or authenticate extracted
  XML that cannot be compared with its original archive here.

The decompiled source trees referred to by the provider are not in this repository.
This review did not obtain binaries, decompile code, run SimPHY, or execute embedded
scripts. Reported 3D, circuit, optics and scripting capabilities remain available
as planning evidence; no architecture expansion was made.

## Completion and verification

Changes: corrected FILE_FORMAT.md, PHYSICS_2D.md and CIRCUITS.md; qualified research
IDs and references; updated README.md/FEATURES.md evidence language; added this log.
Raw fixture XML, images, inventories and archive manifests were not edited.

Checks: parse all six XMLs with DTDs prohibited; compare their settings field sets
against the inventory; check serialized frequencies and the circuit record tokens;
verify 209 unique namespaced research IDs with no task-ID collisions; confirm no
old O evidence rows remain; validate the task registry; compare SHA-256 hashes of
spec/, adr/, rfcs/ and tasks/ before and after to confirm the plan is unchanged.
These are document/data consistency checks, not simulator tests or runtime parity
evidence. No task is marked DONE and no independent review/merge is claimed.
