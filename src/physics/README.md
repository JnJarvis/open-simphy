# Uniform-gravity runtime

World owns its starting document, current validated snapshot, fixed timestep and
successful step count. create rejects invalid dt; step/reset return a new World.
The original is never mutated, including a failure on the final particle. Snapshot
returns an owning value. App pauses by not stepping and replaces its world only
after checking Result::value(). Gravity affects every mass equally.

Position uses old velocity plus half the constant-acceleration displacement;
velocity uses g*dt. Every intermediate is finite-checked in accepted contract order.
Time is count*dt, with bounded index and strict time-progress checks. Underflow
follows ordinary binary64 behavior. No collisions, clocks, graphics or implicit
dt changes. This method is specific to free particles, not a general solver promise.

The private step_time.hpp helper permits testing count/time limits without billions
of iterations or an unsafe public restore API. Tests include accepted exact and
analytic vectors, late failure, mass independence, reset, replay and value lifetime.
Copying is deliberately simple for the small demo; no core API changes or mutable
shared caches were introduced to optimize it.
