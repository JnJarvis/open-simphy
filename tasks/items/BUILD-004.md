# BUILD-004 — Validate desktop application builds across native platforms

Status/owner: canonical registry. Dependencies: BUILD-003, REN-003

## Allowed paths

- `.github/workflows/desktop.yml`
- `cmake/checks/verify_desktop.py`
- `cmake/README.md`
- `README.md`

## Authorized scope and acceptance

Add separate native Windows, Linux and macOS Debug/Release application build jobs using pinned existing tools and SDL. Compile the user-approved merged INT-001 code (0591745), without treating its remaining high-DPI validation as complete. Existing headless jobs stay unchanged. Check executable/license presence, tests-off exclusion of Catch2/Python discovery, and deterministic host failure injection using SDL dummy/software adapters with a timeout. Dummy checks are not native-window smoke. Refresh the planning-era README using verified project state. No distribution artifacts or release/license claims. Required: native six-job results, checker positive/negative fixtures, existing eight headless jobs, registry/format checks. No app or domain edits; report any discovered defects as follow-ups.

Read README.md, tasks/README.md and these references before editing:

- spec/architecture.md
- spec/interfaces.md
- spec/testing.md
- spec/contracts/scene.md
- spec/contracts/renderer.md
- spec/contracts/physics.md

Submit evidence to coordinator for independent review; no automatic merge.
