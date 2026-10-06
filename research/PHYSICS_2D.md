# SimPHY 2D Physics (dyn4j-based)

Evidence: `org/dyn4j/**` (engine), `org/shikhar/simphy/dynamics/**` (wrappers),
`decomp/simphy/Major changes` (patch changelog), writer `j/b`, examples.

## 1. Engine provenance
2D dynamics = **dyn4j** (Java port of Box2D v2), heavily modified. The jar ships a
`Major changes` file documenting the patches. Key additions/changes:

- **New shapes:** `Ring`, `Parabola` (plus SimPHY-level `Slice`, `Capsule`,
  `HalfEllipse`, `Convex`, `Link`, `Spline`, `Spline2D`).
- **New mass type:** `Y_MOTION_ONLY` (search in `Body`, `Island`,
  `SequentialImpulse`, `TimeOfImpactSolver`, all joints).
- **New joints:** `SpindleJoint`, `TorsionalSpringJoint`.
- Stability rework of `DistanceJoint`/`RopeJoint` (renamed to
  `get/setNaturalLength`); new intuitive constructors + axis for `PrismaticJoint`
  /`WheelJoint`; `translateLocalAnchor1/2`.
- **Joints are renderable:** `renderer, color, opacity, size`, `setBody1/2`,
  `setColor`, `setVisible`.
- **Collision detection listener:** `DefaultContactManager.updateAndNotify()` +
  `CollisionListener` (exposes begin/persist/end to SimPHY `CollisionProcessor`).
- `CategoryFilter`: `category`/`mask` no longer final; `setCategory()`, `setMask()`.
- `CoefficientMixer`: `DEFAULT_MIXER` mixes **friction = geometric mean** `√(μ1·μ2)` and **restitution = max** `max(e1,e2)`. The user can set friction and restitution mixers **independently** to Geometric Mean / Minimum / Maximum (prefs `coeffMixer=frictionIdx,restitutionIdx`, idx 0=GM,1=min,2=max).
- `Body`: `getWorld()`, `isColliding()`, `CollisionN`, `gravityScale`, renamed
  `getMassData`/`setMassData`, default angular damping 0, `width`/`height` fields,
  `applyLinearImpulse`/`applyAngularImpulse` renames.
- `Fixture`: `setShape()`, **sticky** (`isSticky`/`setSticky`).
- `Mass.create()` honours mixed mass-type fixtures.
- `Vector2`: `approxEqual`, `getRotated`, `getAngleWithXAxis`, `addScaled`.
- `AABB`: `clone`, `expandToFit(Vector2)`, `overLaps` returns false if degenerate.
- `Island` solver: `averageVelocity`; gravity applied in `Island.applyGravity`.
- **Particles** integrated into `Settings` (see §6).

## 2. Solver & world settings (user-tunable, saved in XML)
Reported example/UI settings (not established universal defaults; evidence E006):
StepFrequency 60 Hz in one example; the supplied XML inventory records frequency
values 60, 90, 144 and 180, not their reciprocals. Seconds per step would be
`1 / frequency`; actual loader/clock behavior remains to be verified (FILE_FORMAT §6).
Other reported settings: Velocity/Position solver iterations 10, Baumgarte 0.2,
RestitutionVelocity 0.3, LinearTolerance 0.005, AngularTolerance 2°,
MaxLinearCorrection 0.2, MaxAngularCorrection 6°, WarmStartDistance 0.01,
MaxTranslation 2.0, MaxRotation 90°, ContinuousCollisionDetectionMode ALL,
AutoSleep false (examples vary), SleepTime 0.5, SleepLinearVelocity 0.01,
SleepAngularVelocity 2°.

User-selectable algorithm components (class names in XML, dialog E007/E008):
- **Broadphase:** `DynamicAABBTree` (default), `Sap`, `BruteForceBroadphase`, `DefaultBvhBroadphase`…
- **Narrowphase:** `Gjk` (default), `Sat`.
- **Manifold solver:** `ClippingManifoldSolver`, `SimplexManifoldSolver`…
- **Time-of-impact (CCD):** `ConservativeAdvancement`, `Discrete`…

## 3. Bodies & fixtures
`SimphyBody` wraps a dyn4j `Body`; extra state: `name`, `selected`,
`useRK4` (optional 4th-order Runge–Kutta integration), `scaleX/scaleY` (visual),
`drawPattern/fillPattern` (DrawMode: solid/dashed/dotted), `strokeWidth`,
`opacity`, `zOrder`, `charge` (electric), `attractionG` (pairwise gravity constant),
`isKiller`/`isImmortal` (particle/lifetime semantics), `isSensor`, `isFbdDrawn`
(force diagram), `borderDrawn`, `renderFilled`, `bodyText`, `image`/`brush`
(sprite), `userData`.

- **COMBody** = a *group* of bodies moving as one (rigid composite); serialized by
  inlining members; has a `groupId`.
- **PlaneBody** = static infinite-plane body (one `Plane` fixture); always
  `INFINITE` mass, `Immortal`.
- Mass types: `NORMAL`, `FIXED_LINEAR_VELOCITY`, `FIXED_ANGULAR_VELOCITY`,
  `INFINITE` (static). UI "Restrict Motion" uses `LineJoint` for X/Y lock and
  `FIXED_ANGULAR_VELOCITY` for rotation lock; "Set mass infinite" → `INFINITE`.
- Fixtures carry `Density` (default 1), `Friction` (default 0.2), `Restitution`
  (default 0), `Sensor`, `Sticky`, and a `CategoryFilter` (32 group bits).
- Air resistance: world-level `AirFriction` (linear + quadratic drag, optional wind
  vector) — see FILE_FORMAT §5.

## 4. Joints (2D) — full supported set
dyn4j joints (in `org/dyn4j/dynamics/joint`): `AngleJoint, DistanceJoint,
FrictionJoint, MotorJoint, PinJoint, PrismaticJoint, PulleyJoint,
RevoluteJoint, RopeJoint, SpindleJoint, SpringJoint, TorsionJoint,
TorsionalSpringJoint, WeldJoint, WheelJoint`.
SimPHY-specific (in `org.shikhar.simphy.dynamics`): `LineJoint`, `PathJoint`,
`SpringRopeJoint`.

Behavioral notes (from UI help strings, OBSERVED):
- **Distance:** rigid rod; Frequency+DampingRatio make it a spring/damper.
- **Rope:** distance joint w/ min & max limits, no spring.
- **Spring:** unconstrained, F=−kx−bv.
- **TorsionalSpring:** rotational, T=−kθ−bω.
- **Spindle:** pulley where body 1 is a spool; thread infinitely wound about COM.
- **Pulley:** block-and-tackle; ratio ≠ 1 gives mechanical advantage.
- **Wheel:** vehicle wheel = distance(0)+prismatic+revolute.
- **Prismatic:** slide along axis, limits + motor.
- **Revolute:** hinge, angle limits + motor.
- **Weld:** fixed relative pose, optional rotational spring/damper.
- **Motor:** drives relative motion vs another (often infinite) body.
- **Mouse:** spring/damper to a point (one body).
- **Path:** body follows a parametric path `x(t),y(t)` (expression-driven).
- **Line:** constrains body to a line (used by "Restrict X/Y Motion").
- **SpringRope:** combined spring (stretch) + rope (max length).

Each joint: `BodyId1/2`, `CollisionAllowed`, renderable/opacity/size/color,
`zOrder`, optional `ReferenceAngle`, motors, limits.

## 5. Body controllers (per-body, expression-driven)
In `dynamics/controllers` (13): `ForceController` (applies F, mode + optional
application point), `TorqueController`, `VelocityController`,
`AccelerationController`, `PositionController`, `AngleController`,
`AngVelocityController`, `PropertyController` (drives a body property),
`TogglePropertiesController` (toggles visibility/properties by boolean expr),
`SpringController` (spring to a point: px,py,dx,dy,length,k,damping),
`PathController` (follows path, ideal option), `ValuePropertiesController`.
Plus `KeyBoardBodyController` (keyboard → acceleration/velocity).
Each has an `enable` boolean expression and optional `xExpr`/`yExpr` evaluated
per step (variables: time `t`, body position/velocity, etc.).

## 6. Fields (region/expression forces)
`Field` base (name, enabled, color, zOrder, `xExprForce`/`yExprForce`, optional
`FieldEnabled` expr, optional `ConvexBounds` region). Subtypes:
- `ElectricField` — force on bodies with `charge`; field lines renderable.
- `MagneticField` — velocity-dependent (Lorentz) force.
- `GravitationalField` — additional gravity field.
- `BuoyancyField` — Archimedes buoyancy (params: fluid level/density).
World queries (script API): `getElectricFieldAt`, `getMagneticFieldAt`,
`getElectricPotentialAt`.

## 7. Particle systems (SPH fluids) — `org.dyn4j.particle`
Full **Smoothed-Particle-Hydrodynamics** integrated into dyn4j `Settings`:
`ParticleSystem` (density, radius, gravityScale, maxCount,
particleSystemIteration, strictContactCheck, destroyByAge, renderMode,
particleRenderScale, zOrder), `ParticleEmitter` (emitRate, lifeTime,
maxParticleCount, speed, angle, range, emitFromOrigin, flags) and
`ParticleGroupCreater` (stride, flags, groupFlags). Supports particle groups,
lifetimes, destruction, body contact, Voronoi diagram rendering, stuck-particle
detection. Script API `ParticleSystem` exposes buffers, density, gravityScale,
damping, radius, pressure iterations, collision energy. This powers the
"Liquify" feature (water/powder/elastic/solid/temperature/wall — see EDITOR_TOOLS).

## 8. Tracers
`Tracer` attaches to a body point, records trajectory; width, max count,
sample interval, mode, optional velocity/accel/angular-velocity display with
per-component toggles. Batch-managed; resettable per-body/all.

## 9. Raycasts
`SimphyRay` (name, start, direction, length, ignoreSensors, testAll) — persisted
query rays (used for e.g. laser/optical-style queries in 2D).

## 10. Integration & stepping
- Fixed-step with user time scale; optional per-body **RK4** (`useRK4`).
- `WorldStepListener` orchestrates per-tick: forces, fields, controllers, tracers,
  cameras, particles, collision events.
- Convex world `ConvexBounds` (AABB) culls/limits the active region.

## Cross-refs
FILE_FORMAT §5 (persistence), PHYSICS_3D (separate Oimo engine), SCRIPTING (§World
API), FEATURES.md (SIMPHY-PHY-###/SIMPHY-OBJ-### ids).
