# Fixed-step particle physics contract v1

Status: PROPOSED under PHY-001; independent review required before PHY-002.
Owner: physics. Direct dependencies: core, math, scene only for this slice.
Consumers: app and its test doubles. No collisions, forces other than uniform
gravity, joints, runtime entity creation, solver framework or wall-clock access.

## Port and ownership

The following names and semantics are the implementation target:

| Operation | Result and behavior |
|---|---|
| World::create(const scene::Document&, double fixed_dt) | core::Result<World>; copy starting document, validate dt, initial snapshot at time 0, step_index 0 |
| world.step() const | core::Result<World>; new world exactly one configured step later, original unchanged |
| world.snapshot() const | owning scene::Snapshot value; publication does not advance anything |
| world.reset() const | core::Result<World>; new world from retained starting document and same dt, index/time 0 |
| world.fixed_dt() const | configured seconds as double |
| world.step_index() const | uint64 count of successful steps since construction/reset |

No public default constructor may produce an invalid World. Copies own state or
share only immutable state; neither world can mutate another's document/snapshot.
No references into caller-owned documents. No mutable static/global state or hidden
threads. Main-thread-only rules belong to presentation, not this module.
Result<T>'s existing const payload API requires copying successful World values;
do not change core accessors to save copies. The initial small slice accepts this
cost. No public inheritance/plugin framework or callback is needed.

An explicit fixed_dt is supplied at construction and never read from a clock.
It must be finite and 0 < fixed_dt <= 1 second. No arbitrary positive lower cutoff:
subnormal positive values are admitted, subject to the arithmetic checks below.
Zero, negative, NaN, infinity and values >1 fail with invalid_argument, path
`fixed_dt`, no entity. The upper bound is a deliberate demo policy, not a stability
claim for future collision/constraint solvers. App chooses 1/128 second initially.
Changing dt requires a new World; do not silently change it mid-run.

App implements pause by not calling step. Single-step always invokes it exactly
once. Reset is independent of pause state. Scene commands edit authoring data;
they never alter a running World. App must recreate a world for a changed document.

## Numeric update, one transaction per step

All components are binary64 and use SI units and +y up. For each particle with old
position p, old velocity v, gravity g, and h=fixed_dt, evaluate componentwise:

```text
velocity_delta = g * h
drift          = v * h
acceleration   = velocity_delta * (0.5 * h)
next_position  = (p + drift) + acceleration
next_velocity  = v + velocity_delta
```

Use old v for drift. Computing drift from next_velocity would implement a different
method. Mass is positive scene data but cancels from uniform gravitational
acceleration; neither multiply by mass nor divide g by mass. Keep ID/mass/radius
unchanged. Apply the same rule in ascending ID order, independent of original
import order. No clamping, damping, epsilon insertion, collision floor or forces.

This is the constant-acceleration kinematic step (equivalently the position update
of velocity Verlet with constant acceleration). It is exact in real arithmetic for
this model. This choice does not freeze a general-purpose integrator for later
models, which require their own contract review.

Let n be current step_index. Maximum successful count is 2^53-1 =
9007199254740991. Reject a step at that count with invalid_data, path `step_index`.
This is before increment, so no integer wrap and all converted indices are exact.
Compute candidate_time = double(n+1) * h, not repeated time += h. Require finite
candidate_time > old snapshot time; otherwise invalid_data at `time`. A very large
index can fail the progress check earlier due to rounding; do not force progress
with nextafter or adjust dt. Snapshot time is rounded n*h, not frame/wall time.

Check every intermediate vector and final vector for finite components. If any
is nonfinite, return invalid_data with the particle's ID and the corresponding
`particles.velocity_delta`, `particles.drift`, `particles.acceleration`,
`particles.position` or `particles.velocity` path. Nonfinite p+drift is a position
failure even if later cancellation might have produced a finite mathematical sum.
Do not algebraically rearrange to bypass a required rejection. Underflow to finite
zero follows ordinary binary64 behavior; nonzero displacement is not guaranteed.

Build all candidate samples locally, validate using Snapshot::create(starting_doc,
candidate_time,samples), and only then construct success World. Propagate any scene
Diagnostic intact. A failure on the last particle leaves every old particle, time,
step_index and retained document unchanged. No partial update may escape. Allocation
exceptions remain exceptions and also cannot mutate the original const World.

## Accuracy and reproducibility

For N real-arithmetic steps, T=N*h:

```text
v_N = v_0 + N*g*h
p_N = p_0 + N*h*v_0 + (N*N/2)*g*h*h
    = p_0 + T*v_0 + 0.5*g*T*T
```

Derivation: v_k=v_0+k*g*h. Sum h*v_k + 0.5*g*h*h for k=0..N-1;
sum k=N(N-1)/2, giving N^2/2. There is no truncation error for constant g.
Forward/semi-implicit Euler would instead have a position bias of magnitude
0.5*abs(g)*T*h in a component; the reference test must distinguish this error.

Rounding still accumulates. Reference acceptance below is limited to the supplied
ordinary-magnitude examples, N<=1000 and T<=10; it is not a universal arbitrary-input
bound. For normal, nonoverflowing intermediates a conservative first-order guide is
gamma(8N) * (abs(p0)+T*abs(v0)+T*T*abs(g)+1) for position and
gamma(8N) * (abs(v0)+T*abs(g)+1) for velocity, where
u=2^-53 and gamma(k)=k*u/(1-k*u). The extra operations budget covers velocity-error
propagation into position and rounded products. Use the independently checked
absolute thresholds in the table, not a dynamically enlarged tolerance based on
the computed result. Extreme/subnormal inputs test safety, not this accuracy claim.

Use exact integer ID/index comparisons and exact reset copies. Cross-platform
analytic comparisons use explicit absolute bounds; no bitwise floating agreement
between compilers/CPUs is promised. Repeated identical input and step count in the
same build/environment must give equal snapshot data. No fast-math or explicit FMA
reassociation; ordinary compiler differences are covered by reference tolerances.

## Required PHY-002 vectors

All positions in meters, velocities m/s, h and times seconds; mass kg. Unspecified
mass=1 and display_radius=0.1. Compare untouched copied fields exactly.

| ID | Setup/action | Expected |
|---|---|---|
| P01 | Empty validated document, h=1/128 | create succeeds at index/time 0; step succeeds at index 1/time h, still empty |
| P02 | h=0,-1,NaN,+infinity,1.0001 (separate); h=1 and smallest positive subnormal | former invalid_argument/fixed_dt; latter create successfully |
| P03 | p=(0,10), v=(2,3), g=(0,-8), h=1/4, one step | p=(0.5,10.5), v=(2,1), time=0.25, index=1; exact binary values |
| P04 | Same initial data, 4 steps | p=(2,9), v=(2,-5), time=1; exact binary values |
| P05 | p=(1,-2), v=(-3,4), g=(0,0), h=1/8, 8 steps | p=(-2,2), v unchanged, time=1; exact |
| P06 | p=(0,10), v=(2,3), g=(0,-8), h=1/128, 128 steps | p=(2,9), v=(2,-5); abs error <=1e-11, relative=0 |
| P07 | p=(0,10), v=(2,2), g=(0,-9.8), h=0.01, 100 steps | p=(2,7.1), v=(2,-7.8), T=1; abs error <=1e-11 for position/velocity, time <=1e-15 |
| P08 | p=(0,10), v=(2,2), g=(0,-9.8), h=0.01, 1000 steps | p=(20,-460), v=(2,-96), T=10; abs error <=1e-8 for position/velocity, time <=1e-14 |
| P09 | Two particles same initial state, masses 1 and 100 | equal trajectories; own IDs/masses/radii unchanged |
| P10 | Retain original World and copied snapshot; step and destroy new world | original/index/time and retained snapshot unchanged; input document unchanged |
| P11 | Step multiple times, reset, then repeat same count | initial values/IDs and index/time 0 after reset; replay agrees with first run |
| P12 | Valid first particle; second p.x=max-double, v.x=max-double, g=0, h=1 | entire step fails invalid_data on second ID/particles.position; both old particles/time/index unchanged |
| P13 | p.x=-max-double, v.x=max-double, g.x=max-double, h=1, all y=0 | next_position.x=max/2 finite; velocity overflow fails with particles.velocity and ID; no published candidate |
| P14 | Same data/steps in two independently created worlds | equal same-build snapshots, no shared mutable/global state |
| P15 | snapshot called repeatedly without step | no change in time/state/index (pause building block) |

Counter-limit logic is checked in a pure internal helper or narrowly scoped test
seam; never run 2^53 steps or expose arbitrary-state restore in the public API.
Test values n=max_count and an invalid/nonincreasing candidate time directly through
that helper. Public constructors still enforce valid state.

## Strict implementation checkpoints

1. Register only src/physics and owned tests; include public header alone.
2. Implement construction and initial/reset snapshot before stepping.
3. Implement the time/index helper and scalar/vector update with explicit checks.
4. Add the transactional step using local candidate storage; run P12/P13 first.
5. Run all vectors and independent analytic tests on native Debug/Release matrix.
6. Submit report with actual commands/results. Do not wire src/app or silently
   add a fallback integrator. Stop and report any missing contract decision.

Consumer doubles use the same lifecycle vectors (initial/pause/reset/one-step)
in consumer-owned tests; a lookup-table double is not evidence of numeric accuracy.
Numeric reference expectations must come from the formula above, never sampled
implementation output. A future physics contract change requires normal review.
