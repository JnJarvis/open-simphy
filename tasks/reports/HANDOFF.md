# Development handoff

Repository: https://github.com/JnJarvis/open-simphy.git
Canonical local checkout: C:/Users/Dell4/Desktop/open-simphy.
Updated 2026-10-06. The user may temporarily use another AI due to usage limits.

## Current state

Approved and merged: build policy/scaffold, core/math/scene contracts, test utilities,
core IDs/diagnostics and 2D math. All their required native platform jobs passed.
There is no runnable simulator yet. Preserve the modular architecture and Windows
desktop priority with portable Linux/macOS headless foundations.

SCENE-002 implementation is on codex/scene-002 at 4ea2598, pushed to origin.
Local worktree: .worktrees/SCENE-002. Report: tasks/reports/SCENE-002.md.
It has validated documents, atomic edits, snapshots and drawing packets. Local
Windows Debug: 23/23 tests pass. CI run 37552678480 is the authoritative native
verification for that commit. All eight jobs passed; task submitted for REVIEW.
No scene implementation merge approval has been received. Check the canonical
registry for subsequent changes. Scene editing by this worker is now stopped;
the coordinator may verify this state before assigning review corrections.

## Resume safely

Read AGENTS.md, README.md, tasks/README.md, registry.json, the assigned task and
referenced contracts. The canonical coordinator identity is codex-coordinator;
worker identity on existing tasks is codex-worker. Claims/status/reports live only
in the canonical checkout. Do not copy stale registries back from task branches.
Do not take over an existing claim while this worker is active. This worker stops
scene edits after the final review submission; coordinator must verify before any
reassignment. Never auto-expire claims or weaken required tests.

Finish review of SCENE-002 after its native checks pass. User approval is still
required under the repository review workflow; merge and record the real merge
commit before DONE. Do not consume its branch as an unmerged dependency.

## Shortest authorized route to the first demo

1. PHY-001 (READY): specify fixed-step particle port and update equations, dt limits,
   failure atomicity, reset and analytic reference bounds. Owns only
   spec/contracts/physics.md. Requires review before implementation.
2. REN-001 (READY): select a minimal render route from primary docs, document
   platform/surface ownership and headless tests. Owns render policy and task ADR.
   Do not choose a full editor toolkit or introduce a renderer-to-platform edge.
3. REN-002 after REN-001 merge: freeze render port using accepted scene packets.
4. PHY-002 after PHY-001 and SCENE-002 merge: uniform-gravity stepper and tests.
5. REN-003 after REN-002 and SCENE-002 merge: minimal particle renderer.
6. INT-001 after PHY-002/REN-003 merge: app composition, private host adapter,
   advance/pause/reset controls, headless lifecycle tests, documented visual demo.
   Only integration tasks own src/app/. No speculative collision/full-editor work.

COMPAT-001 is independently READY but not needed for the first runnable slice.
Raw unlicensed research fixtures stay ignored; research/incoming/ is untracked
pre-existing content. Do not delete or publish it opportunistically.

## Tooling notes

Windows compiler: VS2022 Community, developer shell, MSVC 19.44.
Shared commands: cmake --preset headless-debug; cmake --build --preset headless-debug;
ctest --preset headless-debug --output-on-failure. Release equivalents exist.
Use isolated task branches/worktrees and explicit allowed paths. Native CI covers
Windows, Linux GCC/Clang and macOS Debug/Release. macos-15 works; the original
macos-15-arm64 label stalled and was replaced at the user's explicit request.
Catch2 3.7.1 and build tools are pinned. clang-format 18.1.8 required.
Tests-off configurations must not fetch test libraries or require Python.
Git credentials already work locally; never print tokens. No GitHub CLI installed.
GitHub API can read public run/job status. Do not claim that passing CI is review.

Recent interfaces: core Result<T> has const nullable value/error accessors and no
assignment; copy/move construction is supported. Test-only numerical helpers live
in tests/support. Each module owns its target and public include directory, and
tests/<category>/<module>/CMakeLists.txt registers through opensim_add_test.
