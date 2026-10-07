# BUILD-003 — Prepare portable renderer builds and optional SDL host dependency

Status/owner: canonical registry. Dependencies: BUILD-002 and accepted REN-001.
Blocks REN-003 and INT-001. Do not begin before the render policy is merged.

## Allowed paths

CMakeLists.txt, CMakePresets.json, cmake/, .github/workflows/.
No src/app, production renderer or existing contract changes.

## Objective and exact scope

Make the standard-library renderer part of the headless module list, once its
CMakeLists exists, without enabling platform/editor/app or requiring SDL. Existing
placeholder handling remains. Preserve explicit topological ordering and DAG checks.

Provide a CMake function for optional SDL setup which INT-001 can call from its
app CMakeLists. Function definition must not download/configure SDL. Invocation
enables C if needed and acquires the exact version/hash from accepted render policy.
Offer an explicit validated offline source override; no silent system fallback.
Link upstream's static target privately from the later app target. Do not fabricate
an app or change the existing clear error when OPENSIM_BUILD_APP=ON without src/app.
Document the exact call sequence/target name for INT-001. No convenience aliases
for unreviewed production APIs or uncontrolled latest-version dependencies.

## Required evidence

- All existing native headless Debug/Release checks pass with no SDL acquisition.
- Tests-off headless build still needs neither Catch2 nor Python nor SDL.
- Isolated CMake probe under cmake/checks exercises the SDL setup function and
  compiles/links a minimal SDL version query on Windows; this is build verification,
  not an interactive app. Valid pinned acquisition and offline source both tested.
- Bad source version/hash rejected. License notice path documented.
- Configure representative renderer registration using a test-only module fixture
  without consuming unfinished REN-003 code. Tests are not hidden behind APP=ON.
- Record exact versions, commands, results and changed paths. Submit for independent
  review; native window smoke belongs to INT-001, not this task.

Read README.md, tasks/README.md, spec/architecture.md, spec/interfaces.md,
spec/testing.md, spec/build-policy.md and the merged spec/render-policy.md first.
No weakening of native CI requirements or edits outside allowed paths.
