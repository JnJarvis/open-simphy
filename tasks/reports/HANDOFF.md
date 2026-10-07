# Development handoff

Repository: https://github.com/JnJarvis/open-simphy.git
Canonical local checkout: C:/Users/Dell4/Desktop/open-simphy.
Updated 2026-10-06. User has NOT switched workers yet. Keep this ready for possible
later small-task work with their chosen model (Qwen 3.8 27B). Do not delegate or
reassign claims from this document alone. See SMALL-TASK-GUIDE.md in this directory.

## Current state

Approved and merged: build policy/scaffold, core/math/scene contracts, test utilities,
core IDs/diagnostics, 2D math and scene implementation. All required native jobs passed.
There is no runnable simulator yet. Preserve the modular architecture and Windows
desktop priority with portable Linux/macOS headless foundations.

SCENE-002 merged as 70db740 after explicit user approval. Local suite: 23/23 PASS;
native run 37552678480: eight jobs PASS. See tasks/reports/SCENE-002.md.

PHY-001 proposal: codex/phy-001 at c398c22, .worktrees/PHY-001.
REN-001 proposal: codex/ren-001 at 05b1cf1, .worktrees/REN-001.
Both are now accepted and merged after explicit user approval: physics merge
4c96e65, rendering merge e6f19c1. Read their accepted files from master.
BUILD-003 closes the headless-renderer/optional-SDL gap and is submitted for review.
Use the registry for any later changes; current worker retains coordinator ownership.

PHY-002 implementation is approved and merged as c8e4f09; local suite 28/28 PASS,
native run 37555585323 passed all eight jobs. It advances particles in tests,
but no interactive application exists. REN-002 raster contract is approved and
merged as c2c24b5, with independently checked pixel goldens. Read accepted files
from master. BUILD-003 is the next prerequisite for renderer implementation. Prioritize difficult
correctness/lifecycle decisions before smaller tasks, as requested by the user.

## Resume safely

Read AGENTS.md, README.md, tasks/README.md, registry.json, the assigned task and
referenced contracts. The canonical coordinator identity is codex-coordinator;
worker identity on existing tasks is codex-worker. Claims/status/reports live only
in the canonical checkout. Do not copy stale registries back from task branches.
Do not take over an existing claim while this worker is active. This worker stops
scene edits after the final review submission; coordinator must verify before any
reassignment. Never auto-expire claims or weaken required tests.

PHY-001/REN-001/PHY-002/REN-002 reviews are complete. BUILD-003 is owned by
codex-worker, branch codex/build-003 at 39d4b51, worktree .worktrees/BUILD-003.
All eight native CI jobs passed (run 37568944027), including the Windows SDL probe;
local offline-source probe also passed. See BUILD-003.md for evidence. Await
independent approval and merge before REN-003. Continue using
isolated branches; do not take over claims automatically.

## Shortest authorized route to the first demo

1. PHY-001 (DONE): specifies fixed-step particle port and update equations, dt limits,
   failure atomicity, reset and analytic reference bounds. Owns only
   spec/contracts/physics.md. Accepted and available for implementation.
2. REN-001 (DONE): selects a minimal render route from primary docs, documents
   platform/surface ownership and headless tests. Owns render policy and task ADR.
   Do not choose a full editor toolkit or introduce a renderer-to-platform edge.
3. REN-002 after REN-001 merge: freeze render port using accepted scene packets.
4. PHY-002 after PHY-001 and SCENE-002 merge: uniform-gravity stepper and tests.
5. BUILD-003 after REN-001 merge: headless renderer and optional SDL setup.
6. REN-003 after REN-002, BUILD-003 and SCENE-002 merge: particle renderer.
7. INT-001 after PHY-002/REN-003/BUILD-003 merge: app composition, private host adapter,
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
