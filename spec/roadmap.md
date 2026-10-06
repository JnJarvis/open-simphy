# Staged roadmap

Stage completion is a reviewed milestone, not simply all tasks with that stage
number being DONE. Only Stage 0 and the early Stage 1 slice are scheduled now.

| Stage | Objective and prerequisites | Major deliverables and exit criteria | Parallel work |
|---|---|---|---|
| 0 Foundation | Planning baseline accepted | Build/coding policy, working CMake/CI test scaffold, frozen core/math/scene/step/render contracts, task workflow checks. Exit: clean checkout builds and tests on selected platform, contract checks pass, ownership established. | Build policy, core contract, math contract, research intake, render decision |
| 1 Minimal engine | Stage 0 contracts and build baseline | Free particle under uniform gravity, scene validation, immutable snapshot, minimal renderer and app loop. Exit: documented demo starts, advances at fixed dt, pauses/resets, and renders known motion; analytic and integration tests pass. No collisions yet. | Math/core implementation, scene validation, physics step, render component after respective gates |
| 2 Basic editor | Stage 1 usable slice | Create/select/transform objects, properties, camera, document reset and play/pause. Exit: scripted user workflow produces validated state; invalid edits are atomic and reported. | Tools, property model, camera, native-document design after contract decisions |
| 3 Core physics | Stable scene/physics contracts; explicit scope decision | Rigid bodies, collision, contact solve, friction, restitution, force/impulse/torque and basic constraints. Exit: selected analytic/contact reference suite, regression fixtures and stability bounds pass. | Geometry, force models, constraints and solver tasks against accepted contact contracts |
| 4 Visualization and analysis | Snapshot/time/quantity contracts | Vectors, trajectories, measurements, graph data and collection. Exit: known reference scenes produce correct values/units and playback-aligned plots. | Overlay generation, sampling and graph presentation |
| 5 Advanced features | Research-informed feature priorities and Stage 3 stability | Select springs, motors, joints, ropes or other justified capabilities. Exit: each selected feature has model/physics/edit/render/IO tasks and explicit integration evidence. | Independent feature modules after contracts; no speculative full backlog |
| 6 Compatibility | Research evidence and native model/schema ready | Dedicated parser, translator, diagnostics, evidence-backed fixture harness. Exit: declared import subset passes; unsupported input is never silently discarded. Export only if separately specified and tested. | Research normalization, parser, translators and fixture validation by format/version |
| 7 Feature parity | Versioned capability matrix and reference evidence | Close prioritized behavioral, UI/workflow and file compatibility gaps. Exit: declared target version/scope matrix satisfied; exclusions published; no unqualified parity claims. | Independent matrix rows after shared contract gates |
| 8 Stabilization/release | Declared scope and licensing settled | Performance budgets, UX checks, fuzzing, docs, packages and platform CI. Exit: reproducible release candidate meets published quality gates and known limitations. | Packaging, accessibility/UX, performance, docs and platform tests |

Stage 6 research preparation may run from Stage 0. Implementation follows evidence
and dependencies, not a calendar. Stages 4–6 may overlap where contracts exist.
Stages 2–8 are planning envelopes; decompose them when their prerequisites arrive.
