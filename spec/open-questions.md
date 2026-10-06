# Open decisions and blockers

| Question | Owner / resolution | Blocks |
|---|---|---|
| C++ version, compiler, CMake baseline, test framework, dependency acquisition and style | BUILD-001 evidence-based ADR | BUILD-002 and compiled contract conformance |
| First supported OS and CI runner; later platform matrix | BUILD-001; request maintainer preference if material | Build CI and packaging guarantees |
| Minimal rendering backend and headless testing route | REN-001 ADR; avoid choosing full UI framework yet | REN-002 |
| Project name, license, contribution policy | Maintainer decision, recorded in ADR before external release | Distribution; not local foundation work |
| Who is the canonical coordinator/reviewer? | Maintainer assigns one identity before claims; an agent is acceptable | Safe multi-agent claims and DONE transitions |
| Native format encoding/version/migrations | Future IO design task after canonical model settles | Native persistence implementation |
| Integrator, timestep and reproducibility guarantee | PHY-001 specifies tiny first slice; later solver ADR | PHY-002; no global solver choice yet |
| Full UI toolkit, undo/redo and live edits | Stage 2 design tasks | Editor implementation |
| SimPHY versions, dimensions, format semantics, feature coverage and tolerances | Supplied research mapped by COMPAT-002 | Detailed compatibility implementation and parity claims |
| Compatibility export and lossless round trips | Evidence plus explicit target-scope decision | Any export guarantee |

No external libraries are selected in this planning baseline. A 2D free-particle
slice is a reversible scope choice, not a feature-parity assumption. Legal review
may be needed for distribution decisions; this plan makes no legal conclusions.
