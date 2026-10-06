# TEST-001 implementation evidence

Owner: codex-worker. Branch: codex/test-001. Commit: 99c7fca.
Status: REVIEW. Local and native CI verified; independent review pending.

## Scope and acceptance

Changed paths: tests/CMakeLists.txt, tests/fixtures/README.md,
spec/test-evidence.md, tests/support/{CMakeLists.txt,numeric.hpp,numeric_tests.cpp,
contract_sample.cpp,verify_diagnostics.py,fixture_metadata.py,sample.json,
sample.metadata.json}. No production interfaces or architectural dependencies changed.

All eight categories have module registration entry points using the existing
opensim_add_test hook. Unit and contract samples demonstrate discovery without
production dependencies. Numeric assertions report actual/expected and absolute/
relative tolerances; invalid tolerance and nonfinite inputs are rejected. The
failure verifier executes a hidden failing assertion and checks exit 42 and all
four context values. Fixture documentation covers origin, permission, hash,
version, expected results, units and tolerance. The synthetic sample's exact
SHA-256 is verified, and six damaged metadata records are rejected.

## Observed checks

Windows MSVC 19.44, CMake 4.4.2, Ninja 1.13; Debug with warnings as errors and the
previously hash-verified Catch2 3.7.1 offline source:

- cmake --preset headless-debug -DOPENSIM_WARNINGS_AS_ERRORS=ON
  -DOPENSIM_CATCH2_SOURCE_DIR=<BUILD-002 verified source>: PASS.
- cmake --build --preset headless-debug --parallel 4: PASS.
- ctest --preset headless-debug --output-on-failure: 12/12 PASS.
- ctest --preset headless-debug -N: 12 discovered.
- ctest --preset headless-debug -L '^unit$' --no-tests=error: 2/2 PASS.
- ctest --preset headless-debug -L '^contract$' --no-tests=error: 2/2 PASS.
- python cmake/checks/check_format.py with clang-format 18.1.8: PASS, four files.
- git diff --cached --check: PASS.

Initial diagnostic verification incorrectly expected exit 1; actual Catch2 3.7.1
assertion failure returns 42. Corrected the checker and reran full/category suites.
Initial fixture normalization command had a quoting error; corrected before final
checks. Sample bytes omit a trailing newline to preserve hash across Git line-ending
conversion. Metadata validator is intentionally sample-specific, not a file schema.

No simulator functionality or external compatibility is demonstrated by synthetic
vectors. Each module must supply justified numeric tolerances and real contract
vectors. Review and baseline merge remain required before dependent implementations.

Native CI run https://github.com/JnJarvis/open-simphy/actions/runs/37528087385:
all eight jobs completed successfully for 99c7fca (MSVC, GCC, Clang, AppleClang;
Debug and Release). Each job runs formatting, full CTest and tests-disabled build.
