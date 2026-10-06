# Minimal 2D numeric contract v1

Status: ACCEPTED under MATH-001 following independent user review, 2026-10-06.
Owner: math. Consumers: scene and subsequent domain/presentation modules.
Dependencies: standard library only; no core diagnostics, scene or OS types.

## Values, units and coordinates

Scalar is IEEE-754 binary64 (`double` with 53 significant bits). Unsupported scalar
representation is a compile-time diagnostic, not an implicit float fallback.
Vec2 is two scalar values x,y with value ownership, no heap allocation and no
references to another module. x points right and y up; positive planar rotation is
counterclockwise. Angles are radians. Position is meters, time seconds and mass
kilograms where the caller assigns those meanings; Vec2 itself has no implicit
unit conversion. Screen coordinates and degrees are explicitly converted at
presentation/import boundaries. No universal dynamic units framework is required.

Signed zero compares equal to zero. Equality compares components exactly; it is
not approximate and is not appropriate for analytic floating-point comparisons.
NaN and infinity may exist as raw scalar/Vec2 values so input can be inspected,
but a validated scene/transform must reject them before simulation.

## Required minimal operations

Vec2 supports addition, subtraction, unary negation, multiplication by scalar,
dot product, scalar 2D cross product (`a.x*b.y - a.y*b.x`), magnitude, finite-value
predicate, checked normalization and componentwise approximate comparison.
Vector division is not required; avoid exposing divide-by-zero policy accidentally.
Magnitude uses a scaled/hypot-style calculation so avoidable intermediate square
overflow/underflow does not destroy otherwise representable results.

Raw algebra follows double arithmetic and does not throw domain errors, clamp,
invent epsilon offsets, or silently replace nonfinite results with zero. A finite
input can still overflow in dot/add/cross; callers must check the result before
placing it in validated domain state. Fast-math is prohibited for these contracts.

`normalized(v)` is checked and returns an optional value. It succeeds for finite,
nonzero vectors whose magnitude is representable and nonzero, returning v/norm.
It returns no value for zero vectors, nonfinite inputs or nonrepresentable norm.
No arbitrary small-vector cutoff: (1e-300,0) normalizes to (1,0) on the selected
binary64 implementation. Values flushed to zero by unsupported floating modes
do not satisfy this contract. No exception or dependency on core is needed.

Scalar `near(a,b,abs_tol,rel_tol)` is false for any nonfinite argument, negative
tolerance, or rel_tol > 1. Otherwise it means
`abs(a-b) <= max(abs_tol, rel_tol * max(abs(a),abs(b)))`.
Require finite nonnegative abs_tol and rel_tol in [0,1]. Vec2 near compares both
components using that rule. The caller supplies tolerances; no hidden default is
part of the public contract. Relative tolerance does not help at zero; use an
explicit absolute tolerance. Overflow in a-b yields false against a finite bound.

## Rigid transform semantics

Transform2 contains translation Vec2 and rotation angle; no scale/shear/projective
matrix yet. Construction is checked and rejects nonfinite components/angle.
Transform identity is translation (0,0), angle 0. Angles need not be normalized;
the library must not silently change a caller's stored angle by wrapping it.

For rotation R(theta), `R(x,y) = (cos(theta)*x - sin(theta)*y,
sin(theta)*x + cos(theta)*y)`. Applying a transform to a point is `R*p + t`;
applying it to a direction is `R*v`, without translation. Both operations are
checked, returning no value for nonfinite input or result.

`compose(A,B)` means apply B first, then A. Translation is `R_A*t_B + t_A`;
angle is theta_A + theta_B. Reject a nonfinite computed translation or angle.
Inverse has angle -theta and translation `R(-theta)*(-t)`; it is likewise checked.
Transforms and operations are pure values: no shared caches, mutable globals,
thread affinity, graphics matrices or native handles.

No bitwise agreement across math libraries or CPUs is promised. Extremely large
finite angles may suffer argument-reduction error; no unbounded-angle accuracy
guarantee is made. Reference examples use ordinary angles with explicit tolerance.
Scene validation later chooses supported physical ranges rather than math guessing
them or silently clamping inputs.

## Conformance vectors (MATH-002 implements these)

Unless marked exact, scalar/vector comparisons below use abs_tol=1e-12 and
rel_tol=1e-12. These small analytic examples justify that tolerance for ordinary
double arithmetic; it is not a universal physics error budget.

| ID | Input/action | Expected |
|---|---|---|
| M01 | (1,2)+(3,-4); (1,2)-(3,-4); -(1,2) | (4,-2); (-2,6); (-1,-2), exact |
| M02 | 2*(1,-3); dot((1,2),(3,4)); cross((1,0),(0,1)) | (2,-6); 11; 1, exact |
| M03 | magnitude(3,4); normalized(3,4) | 5; (0.6,0.8) |
| M04 | normalized(0,0), normalized(NaN,1), normalized(infinity,0) | All absent |
| M05 | magnitude(1e308,0); normalized(1e-300,0) | 1e308; (1,0), without squaring overflow/underflow |
| M06 | finite(1,2); finite(NaN,2); finite(1,infinity) | true; false; false, exact |
| M07 | near(0,5e-13,1e-12,0); near(0,2e-12,1e-12,0) | true; false |
| M08 | near(1e6,1e6+0.5,0,1e-6) | true via relative tolerance |
| M09 | near(infinity,infinity,1,0); near(1,1,-1,0); near(1,1,0,2) | All false; invalid comparison parameters |
| M10 | identity applied to point/direction (3,4) | Both (3,4) |
| M11 | translation (2,3), angle pi/2: apply point (1,0), direction (1,0) | (2,4); (0,1) |
| M12 | A: translation (2,0), angle pi/2; B: translation (1,0), angle 0; compose(A,B) applied to (1,0) | (2,2); reverse order would produce (3,1) |
| M13 | inverse(T) applied after T from M11, point (3,-4) | (3,-4) |
| M14 | construct transform with nonfinite angle or translation | Absent |
| M15 | compose translations (1e308,0) and (1e308,0) | Absent due to nonfinite sum |
| M16 | Copy transform, replace the copy, inspect original | Original unchanged; no shared ownership |
| M17 | (0,-0) == (0,0); (1,1+1e-10) == (1,1) | true; false (exact comparison) |
| M18 | checked transform of nonfinite point/direction | Absent |

Tests also compile public headers alone and repeat the same vectors for each
selected compiler/platform. Future matrix/quaternion/3D operations are separate
tasks. They must not change 2D units, transform composition order or tolerance
semantics without a reviewed contract version change.
