# Test registration and evidence conventions

Tests are headless. All eight categories in spec/testing.md are registered through
tests/CMakeLists.txt: unit, contract, integration, regression, reference,
serialization, compatibility and malformed. A module adds a CMakeLists.txt under
tests/<category>/<module>/ and calls opensim_add_test with CATEGORY, explicit
SOURCES and required LIBRARIES. New category/module directories are discovered on
configure; source files are never globbed into targets. Empty categories add no tests.

Use globally unique target names. Test-only numeric support is available by linking
opensim_test_support and including numeric.hpp. It adds no production dependency.
OPENSIM_CHECK_NEAR evaluates arguments once, reports actual/expected and tolerances,
and rejects nonfinite values and invalid tolerances. Absolute tolerance is nonnegative;
relative tolerance is in [0,1]. Acceptance is absolute error <= absolute tolerance
OR relative error <= relative tolerance, using max magnitude scaling. Opposite
extreme finite values cannot pass from arithmetic overflow. Exact IDs/bytes use
exact assertions. Each numeric test states units and a justified tolerance; no
universal physics tolerance is supplied.

After configure/build, discover using `ctest --preset headless-debug -N` and run
categories with `ctest --preset headless-debug -L '^unit$' --no-tests=error` or
`-L '^contract$'`. Use --output-on-failure for diagnosis. Empty categories must not
be presented as coverage. Run the full suite before submission.

The support unit sample tests tolerance boundaries and invalid values. Its hidden
failure probe is executed by a separate checker that requires failure and all four
numeric context values. The contract example is explicitly synthetic, exercising
shared vectors without claiming production conformance. Real contract tasks must
exercise both the consumer double and reviewed implementation on shared vectors.

Fixture conventions are in tests/fixtures/README.md. The sample metadata validator
checks the exact data hash and six damaged records. Preserve fixture bytes when
moving between platforms. Completion reports record commit, platform/tool versions,
commands, counts, failures, skips, fixture provenance and limitations. Never replace
an expected result merely to make a test pass.
