# BUILD-004 completion report

Owner: codex-worker. Branch: codex/build-004.
Commit: 3e2970b6fab4921f014921128ff547af23cce312.
Status: submitted for independent REVIEW; not merged.

Changed paths: .github/workflows/desktop.yml, cmake/checks/verify_desktop.py,
cmake/README.md, README.md. No application/domain/contract changes.

Separate desktop CI adds Windows MSVC14.44, Linux GCC13 and macOS Xcode16.4,
each Debug (tests ON) and Release (tests OFF). Existing eight headless jobs are
untouched. Exact prior CMake3.28.4/Ninja1.11.1.4 packages and SDL3.2.28 hash remain.
Linux X11/Mesa development packages are runner prerequisites drawn from the pinned
SDL source README-linux.md, documented as OS-managed rather than pinned libraries.
No release artifacts are distributed; licensing/high-DPI limits remain explicit.

Verification checks native executable presence, exact pinned SDL license SHA256,
expected app/tests cache settings, no Catch2/Python discovery for tests-off builds,
and expected code1 plus diagnostic from each deliberate failure flag. Child-only
SDL dummy/software drivers and 30-second per-process timeout prevent UI waits.
These are build/failure checks, not visual/native-window smoke or desktop support.
README now accurately describes the working demo and development boundaries.

Observed local commands:
- python cmake/checks/verify_desktop.py --self-test: PASS, 2 tests, including missing
  executable/notice, altered notice, wrong config, leaked Catch2/Python, wrong
  exit/diagnostic and propagated timeout. Fixtures synthetic, never executed.
- python cmake/checks/verify_desktop.py <INT-001/build/demo> --tests ON: PASS against
  the previously built actual Windows executable and pinned SDL notice.
- git diff --cached --check: PASS.

Native desktop run https://github.com/JnJarvis/open-simphy/actions/runs/37619394668
at exact commit above: all SIX jobs PASS. Includes actual native compilation/link,
Debug app-enabled full headless suites, Release tests-off dependency checks, pinned
notice validation and dummy-driver failure probes on each OS/configuration.
Existing headless run https://github.com/JnJarvis/open-simphy/actions/runs/37619394487
at same commit: all EIGHT jobs PASS, including formatting and existing suites.
No compile-only claim is substituted for execution: probes/tests ran successfully.

INT-001 high-DPI/mixed-scale evidence is still outstanding; Linux/macOS interactive
window smoke remains unperformed. Neither is weakened by passing these jobs.
Independent reviewer/outcome: pending. Merged baseline: none. No new follow-up.
