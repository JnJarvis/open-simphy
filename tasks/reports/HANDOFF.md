# Development handoff

Repository: https://github.com/JnJarvis/open-simphy.git
Canonical local checkout: C:/Users/Dell4/Desktop/open-simphy.
Updated 2026-10-07. User has NOT switched workers yet. Keep this ready for possible
later small-task work with their chosen model (Qwen 3.8 27B). Do not delegate or
reassign claims from this document alone. See SMALL-TASK-GUIDE.md in this directory.

## Current state

Approved and merged: build policy/scaffold, core/math/scene contracts, test utilities,
core IDs/diagnostics, 2D math and scene implementation. All required native jobs passed.
The runnable demo is merged as 0591745 after explicit user approval. Its existing
local executable remains under .worktrees/INT-001/build/demo/src/app/.
See docs/demo.md in that worktree. Preserve the modular architecture and Windows
desktop priority with portable Linux/macOS headless foundations.

SCENE-002 merged as 70db740 after explicit user approval. Local suite: 23/23 PASS;
native run 37552678480: eight jobs PASS. See tasks/reports/SCENE-002.md.

PHY-001 proposal: codex/phy-001 at c398c22, .worktrees/PHY-001.
REN-001 proposal: codex/ren-001 at 05b1cf1, .worktrees/REN-001.
Both are now accepted and merged after explicit user approval: physics merge
4c96e65, rendering merge e6f19c1. Read their accepted files from master.
BUILD-003 closes the headless-renderer/optional-SDL gap and is approved and merged.
Use the registry for any later changes; current worker retains coordinator ownership.

PHY-002 implementation is approved and merged as c8e4f09; local suite 28/28 PASS,
native run 37555585323 passed all eight jobs. It advances particles in tests,
and are composed in the new INT-001 demo branch. REN-002 raster contract is approved and
merged as c2c24b5, with independently checked pixel goldens. Read accepted files
from master. BUILD-003 and REN-003 are merged. Prioritize difficult
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
local offline-source probe also passed. See BUILD-003.md for evidence. User approved;
merged as e336d1d. REN-003 is implemented by codex-worker on codex/ren-003
at a742d17, worktree .worktrees/REN-003. Local 40/40 tests passed; native CI
run 37570462632 passed all eight jobs. User approved; merged as 72d3158.
INT-001 is owned by codex-worker on codex/int-001 at a067a00. Local app build and
43/43 tests passed; real Windows window smoke passed on one 100% display.
High-DPI/mixed-scale validation remains unavailable and is required before final
task completion; user approved merging with that known limitation. Registry remains
BLOCKED on the hardware check, retaining its owner.
Native headless CI run 37571375012 passed all eight jobs. See INT-001.md.
Do not reassign the claim or mark DONE without remaining evidence and review. Continue using
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


## Current review queue (2026-10-07)

BUILD-004: codex/build-004 at 3e2970b, .worktrees/BUILD-004. Six native
Windows/Linux/macOS desktop Debug/Release builds passed (run37619394668),
plus all eight existing headless jobs (run37619394487). Checker and README
updates ready for independent review; not merged. See BUILD-004.md.

EDT-001: codex/edt-001 at 037b2d1, .worktrees/EDT-001. PROPOSED editor contract
and affected-consumer RFC ready for review; no editor code yet. Covers transactional
history/monotonic IDs, selection, checked picking, baseline-relative drag, bounded
resource use and future app Authoring/Simulation modes. All30 specification vectors
listed; arithmetic/link/consistency checks recorded in EDT-001.md. Do not implement
or treat this contract as accepted until independent approval and merge. Future
implementation must receive explicit IDs/scopes, especially shared editor headers.
COMPAT-001 remains READY/unclaimed for the later small-task worker.
