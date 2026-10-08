# INT-006 — Measured rendering and collision corrections

User authorized performance and collision corrections after approving INT-005.
Affected-contract review: private app presentation may bypass the hidden particle
Frame when the imported mechanism owns the full viewport. Canonical renderer and
particle ports remain unchanged. Frame timing benchmarks cover the actual native
source view. SDL remains exclusively in app.

Baseline independent five-circle test fails: incoming velocity 2 m/s leaves the
striker moving backwards at -0.30555 rather than stopping. Ideal suspended cradle
return amplitude is 0.24216 versus required >0.33560 after one release. The prior
native test only required some other ball motion and was inadequate.

Evaluate bounded contact-refresh microsteps rather than one Box2D frame with eight
internal substeps: Box2D updates manifolds/restitution once per world step. Multiple
short steps refresh impact velocities through close chains. This is a private
solver-policy correction, not a new scene schema or filename-specific simulation.
Retain circle materials, mass, filters, sensors and distance links. Commit the
policy only after independent chain, pendulum and analytic checks pass. Record any
additional contact policy and its evidence here before adopting it.

Eight complete world microsteps pass both independent chain regressions (3,757
assertions), preserving one-/two-ball output and return swings without changing
materials or adding special cradle logic. Adopt this contact-refresh policy; the
canonical fixed_dt/snapshot/public APIs are unchanged. Pairwise restitution/mass,
static/filter/sensor and oblique analytic cases also verify general circle contacts.
