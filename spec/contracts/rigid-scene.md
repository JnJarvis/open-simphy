# Rigid scene producer contract v1

Status: PROPOSED under SCENE-003. Owner scene; dependencies core/math only.
Additive public header `opensim/scene/rigid.hpp`; accepted particle contracts and
native .osim v1 are unchanged. No external schema or native handles in these types.

## Owning values and coordinates

Use binary64 SI: meters, seconds, kilograms, radians, kg*m^2. +y up, positive
rotation counterclockwise. `Pose { Vec2 origin; double angle; }` locates the body
coordinate origin, not its center of mass. `Mass { double kilograms, inertia;
Vec2 local_center; }` describes inertia about the body's center of mass.

For body-local point q, world point = origin + R(angle)*q. World COM is
origin + R(angle)*local_center. `Velocity { Vec2 linear; double angular; }`
stores world COM velocity and radians/second, not origin velocity. A solver that
integrates COM c must publish origin = c - R(angle)*local_center. Contact velocity
at world point p is linear + angular * (-(p-c).y, (p-c).x).
These equations prevent compound objects rotating around the wrong point.

`CircleShape { Vec2 center; double radius; }` and `ConvexShape { vector<Vec2>
vertices; }` are body-local. `Shape` is their variant. A rectangle is four exact
vertices. Geometry incorporates fixture-local translations/rotations exactly once;
there is no separate fixture pose or polygon centroid offset.

`Fixture { uint32_t key; Shape shape; double density, friction, restitution;
bool sensor; uint32_t category_bits, mask_bits; }`. Keys are nonzero, unique within
a body, stable across edits and snapshots, and canonicalized ascending. An
identified fixture is (body EntityId, fixture key). Density is kg/m^2, friction
is dimensionless >=0, restitution lies in [0,1]. Sensors report overlaps without
contact impulses. Collision pair eligibility requires both mask/category tests
nonzero; same-body fixture pairs never collide. No group-index policy yet.

`Mobility` is dynamic/static/kinematic. `Body { EntityId id; Mobility mobility;
Pose initial_pose; Velocity initial_velocity; Mass mass; vector<Fixture> fixtures;
double gravity_scale, linear_damping, angular_damping; }`. Mass is explicit and
authoritative, not silently recomputed from density. Dynamic mass and inertia are
strictly positive. Static/kinematic values are retained but do not grant finite
inverse mass/inertia; both must be nonnegative. Static initial velocity is zero.
Kinematic velocity is prescribed. Gravity scale is finite (including negative);
damping coefficients are finite and nonnegative, in 1/seconds. Their discrete
application is owned by the separately reviewed rigid physics contract.

Source FIXED_LINEAR/FIXED_ANGULAR mass modes are not aliases for these mobility
values. Mass computation, accumulated forces/torques, sleep, CCD, joints, scripts,
charge and unsupported source settings need reviewed mappings before a source
project is runnable. The original source representation preserves them meanwhile.

## Document and publication ports

`BodyDocument::create(unsigned revision, Vec2 gravity, vector<Body>, EntityId
high_water)` returns Result<BodyDocument>. Only revision 1 supported. Accessors:
revision(), gravity(), bodies() as span<const Body>, high_water(); exact equality.
Private constructor; immutable public API; copies own data or share only immutable
storage. Empty body collection is valid; every body has at least one fixture.
ID uniqueness/high-water validation follows the accepted scene particle policy.
Bodies sort by ascending ID; fixtures sort by key. Preserve vertex order.

`BodySample { EntityId id; Pose pose; Velocity velocity; }` stores runtime state.
`BodySnapshot::create(const BodyDocument&, double time, vector<BodySample>)`
returns Result<BodySnapshot>; samples contain exactly one matching body ID each,
no duplicates or extras. It owns the definitions and samples needed for rendering,
never borrows the document. Accessors time(), gravity(), bodies() and samples();
both spans canonicalized by body ID. No solver caches or clock access.

Time is finite and nonnegative. Poses/velocities/COM must be finite, including
computed world fixture coordinates. Angles are retained in radians without hidden
normalization. Static runtime samples must exactly match their initial pose/zero
velocity. A snapshot does not step the solver or change authoring values.

## Validation and limits

Maximum 4096 bodies, 16384 total fixtures, 64 fixtures/body, 3–64 vertices/convex.
Check budgets before sorting/geometry work. Positions, local centers, vertices and
radii are bounded in absolute magnitude by 1e9 meters. Circle radius >0. All
numeric inputs must be finite; material rules above apply. Reject invalid enum
values, zero/duplicate keys and absent fixtures. No default inferred source values.

Convex vertices must be distinct, strictly convex, CCW and non-self-intersecting;
reject collinear edges, clockwise winding, repeated endpoints and concavity.
Use binary64 signed cross products, require strict positive turns and every other
vertex strictly left of every edge. Check finite intermediates. Do not reorder,
compute a hull, decompose, clamp, insert tolerances or silently repair input.
Near-degenerate positive values are accepted only when all arithmetic remains
finite and the strict tests succeed; this is validation, not solver stability.

Report unsupported revision as unsupported_feature/revision; invalid numeric or
geometry input as invalid_argument, duplicate IDs as duplicate_id. Diagnostic
entity is the affected body where known; paths identify bodies.<id>.fixtures.<key>
and the failing field. Missing/duplicate snapshot samples use missing_reference/
duplicate_id. Unsupported budgets use invalid_argument with a limits path.
Validation order: revision/budgets, document globals/IDs, ascending body scalar
fields, ascending fixtures and shape fields, then snapshot identity/time/values.
A failing operation never publishes partial documents or snapshots. Allocation
failures may remain exceptions; input objects are unchanged.

## Independent acceptance vectors for implementation

- Radius-1 circle, body origin (3,4), angle pi/2, local center (2,0): circle center
  transforms to (3,6), absolute coordinate error <=1e-12.
- COM (3,6), local_center (2,0), angle pi/2 yields origin (3,4). For linear (1,2),
  angular 3 and world point (4,6), contact velocity is (1,5), <=1e-12.
- CCW rectangle [(-1,-2),(1,-2),(1,2),(-1,2)] accepted; reverse winding,
  repeated point, zero-length edge, collinear triple, bow-tie and concave polygon
  each rejected. No correction of any vector.
- Two fixtures on one body retain independent keys and local geometry. Polygon
  vertices with a nonzero centroid must not receive an extra centroid translation.
- Dynamic zero/negative mass or inertia rejected; static nonzero initial velocity
  rejected; NaN/Inf in each numeric field rejected. Negative gravity scale accepted.
- Two fixture filters (category=1, mask=2) and (category=2, mask=1) eligible;
  changing second mask to 4 makes the pair ineligible. Sensors remain sensors.
- Every bound tested at boundary and one beyond; duplicate/zero body/fixture IDs,
  incorrect high-water and invalid mobility rejected without input mutation.
- Unsorted bodies/fixtures canonicalize deterministically; retained copied document
  and snapshot remain valid after input destruction. Unsorted samples canonicalize;
  duplicate/missing/extra IDs and static drift rejected transactionally.
- Public header compiles alone; full existing particle/physics/renderer/native-codec
  vectors still pass. No app, ZIP/XML or third-party physics dependency enters scene.

Producer validation tests establish this contract. They do not establish collisions
or executable SimPHY compatibility. Later consumer tasks require their own tests.
