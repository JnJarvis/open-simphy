# Testing strategy

BUILD-001 chooses the test/build stack and compiler matrix; BUILD-002 implements
it. TEST-001 supplies category registration and evidence conventions. Tests must
be headless by default; optional graphics tests are separately labeled.

| Category / location | Required evidence |
|---|---|
| unit | Small deterministic behavior, edge cases, invalid inputs; owned by module |
| contract | Shared invariants plus a consumer double and real implementation using the same vectors |
| integration | Composition, lifecycle, failure propagation, reset and module interactions |
| regression | Minimal fixture for each fixed defect, linked to task ID |
| reference | Analytic solutions or attributable external reference with dt, units and tolerances |
| serialization | Native round trip, schema versions/migrations, invalid values and no partial mutation |
| compatibility | Evidence ID → requirement → fixture → expected canonical output/behavior |
| malformed | Truncation, oversized counts, invalid encodings, nonfinite values, recursive/deep structures; bounded execution |

Fixture metadata records origin, license/permission, hash, version, expected result,
units and tolerances. Do not check in unknown-rights binaries. Synthetic fixtures
must be labeled synthetic; they do not establish external compatibility.

Physics checks separate integrator correctness from accuracy: assert a discrete
step against the defined update equation, then compare trajectory error against an
analytic solution with stated timestep/error bounds. Add energy/momentum tests only
where model assumptions justify conservation. Never bless current output as truth.

CI gates: build, category tests, task-registry validation, and contract/dependency
checks when their tasks land. New failures block DONE. Required platform checks
cannot be silently skipped. Sanitizers and fuzzing are introduced after toolchain
selection; record budgets and reproducible seeds. Performance thresholds follow
measured baselines, not arbitrary values chosen during planning.

Documentation-only tasks require structured review/checklists, link/schema checks
where relevant, and recorded evidence, not fabricated runtime tests.
