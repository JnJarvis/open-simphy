# SimPHY 3D Physics (Oimo.js port) + 3D Scene

Evidence: `org/simphy/phys3d/**` (~170 classes, Oimo.js port),
`org/shikhar/simphy/gfx/canvas/scene3d/**` (`Scene3D`, `Threed`, `world3d.*`),
`org/simphy/threed/**` (~266 classes, THREE.js-style scene graph), and the
"3D Physics Demo" / "Scene3D Demo" `.ssim` examples (all script-built).

## 1. Two independent 3D stacks
- **Physics:** `org.simphy.phys3d` — a Java port of **Oimo.js** (Box2D-like 3D
  engine). Own math (`Vec3`,`Mat3`,`Quat`), own collision (GJK/EPA, BVH/bruteforce
  broadphase), own constraint solvers (PGS + Direct MassMatrix).
- **Graphics:** `org.simphy.threed` — a **THREE.js-style** scene graph
  (Object3D, BufferGeometry, materials, lights, cameras, controls, loaders) with a
  software/JOGL renderer. Exposed to scripts as `THREE`/`Threed`.

The 3D **physics world** (`World3D`) bridges Oimo `RigidBody`s to `threed`
`Object3D` meshes. **3D scenes are created by embedded JavaScript**, not by the
XML schema (see FILE_FORMAT §5.9). No 3D-specific XML tags exist.

## 2. 3D physics engine (`org.simphy.phys3d`)
### Rigid bodies
`RigidBody` + `RigidBodyConfig`: `position`(Vec3), `rotation`(Mat3),
`linearVelocity`, `angularVelocity`, `type`, `massType` (dynamic/kinematic/
static/fixed), `autoSleep`, `linearDamping`, `angularDamping`.
`RigidBodyType`/`MassType` enums; `Shape` + `ShapeConfig`: `position`,`rotation`,
`friction`,`restitution`,`density`,`geometry`,`collisionGroup`,`collisionMask`,
`contactCallback`.

### Geometry (`collision/geometry`)
`BoxGeometry, SphereGeometry, PlaneGeometry, CylinderGeometry, ConeGeometry,
CapsuleGeometry, ConvexGeometry, ConvexHullGeometry, TriangleGeometry,
HeightFieldGeometry, ParticleGeometry, Aabb`.

### Collision
- Broadphase: `BruteForceBroadPhase`, `BvhBroadPhase` (multiple insertion
  strategies).
- Narrowphase detectors: sphere/box/capsule/convex/plane/heightfield/particle
  combos + generic `GjkEpaDetector` (GJK + EPA polyhedron, caching, logging).

### Constraints / joints (`dynamics/constraint/joint`)
`DistanceJoint, PointToPoint(Joint), PrismaticJoint, RevoluteJoint,
CylindricalJoint, SphericalJoint, UniversalJoint, WheelJoint, WeldJoint,
LineJoint, SpringJoint, GenericJoint, RagdollJoint`.
Supporting: `SpringDamper` (frequency/dampingRatio), `RotationalLimitMotor`,
`TranslationalLimitMotor`, `BasisTracker`.
Solvers: `PgsContactConstraintSolver` + `PgsJointConstraintSolver` (default) and
`DirectJointConstraintSolver` (MassMatrix). `ConstraintSolverType` selects.

### Vehicles
`RaycastVehicle` + `WheelInfo` (suspension, friction sliders, connection) —
used by the car demos.

### World
`World` (gravity, `TimeStep`, `ContactManager`, `Island`, broadphase, solver
type), `AirFriction` (linear/quadratic + wind), raycast/convexCast/aabbTest
queries with callbacks.

## 3. 3D scene bridge (`scene3d.world3d`)
`World3D` (extends a `threed` `Group`) key API (all script-callable):
- **Bodies:** `addSphere, addBox, addCylinder, addCone, addCapsule,
  addConvexHull, addHeightField, addPlane, addGear, addArrow, addRagdoll,
  addParticleSystem, mergeBodies, clearAll, remove, getBodyByName`.
- **Joints:** `addUniversalJoint, addGenericJoint, addPrismaticJoint,
  addLineJoint, addRevoluteJoint, addCylindricalJoint, addSphericalJoint,
  addRagdollJoint, addWeldJoint, addDistanceJoint, addSpringJoint,
  addWheelJoint` (each with optional `SpringDamper` / limit motors).
- **Factory helpers:** `createSpringDamper, createRotationalLimitMotor,
  createTranslationalLimitMotor`.
- **World controls:** `setGravity, getGravity, setAirFriction, getAirFriction,
  removeAirFriction, setSolverIterations`.
- **Queries:** `rayCast, convexCast, aabbTest` (callback to a JS function).
- **Debug/viz:** `toggleDebug`, `setShowVelocities/AngularVelocities/Forces/
  GravityForces`, scales + min thresholds + max arrow length.
- **Vehicles:** `createRaycastVehicle(body)`, `removeRaycastVehicle`.
- Input: `handleKeyEvent` (WASD/drive), `handleMouseEvent` (raycast select/drag).

`PhysicsBody3D` (per-body API): position/rotation/orientation (quaternion),
translate/rotate, linear/angular velocity, `applyForce/Torque/LinearImpulse/
AngularImpulse`, `setRotationFactor` (per-axis lock), inertia matrix,
`charge`, `friction`, `restitution`, sleep, local/world point conversion,
kinetic/translational/rotational energy, `clone`, `reset`, `setType`.

`Vehicle3D`, `Tracer3D`, `PhysicsParticleSystem3D`, joint *wrappers* (per-type
rendering: cylindrical/prismatic/revolute/spherical/spring/universal/wheel).

## 4. THREE.js-style graphics (`org.simphy.threed`)
Full scene-graph port: `Object3D`, `BufferGeometry` (+ primitives & `CSG`),
`Mesh`/materials (`MeshLambert/Phong/Standard/Physical/Basic/Toon/Normal/
Matcap/Depth/Distance/RawShader/Shader/Sprite/Line…`), lights (`Ambient,
Hemisphere, Directional(+Shadow), Point(+Shadow), Spot(+Shadow), RectArea`),
cameras (`Perspective, Orthographic`), `Texture`/cube/data textures, `Raycaster`,
`AnimationMixer`/clips, `Controls` (`Orbit, Trackball, FirstPerson, Drag,
Transform`), objects (`Sprite, InstancedMesh, SkinnedMesh, Skeleton, Reflector,
Refractor, Sky, Water, BitMapTextMesh`), math (`Vector2/3/4, Matrix3/4,
Quaternion, Euler, Color, Box3, Plane, Ray, Sphere, Frustum, Line3`), loaders
(SVG, Collada, GLTF/GLB, PLY), fog, shadows, tone mapping, color space.
Script exposes constants & factories via `THREE` and `Threed.createX()` helpers.

## 5. How 3D simulations are stored & run
1. `.ssim` contains `simulation.xml` (usually a minimal/empty 2D world) +
   embedded model assets (`.glb`, textures) + `screenshot.png`.
2. `<ScriptManager><Script>` JS builds the scene at load:
   `Threed.createScene()` → `scene.setWorld3D(Threed.createWorld())`, add
   lights/cameras/bodies/joints, `Widgets.loadFromXml(…)` for UI, then step the
   world each frame.
3. 3D car demos use `RaycastVehicle` + GLB models; heightfield demos use
   `addHeightField`.

## Cross-refs
ARCHITECTURE §4, FILE_FORMAT §5.9, SCRIPTING (Threed/THREE API), FEATURES.md
(SIMPHY-PHY-3xx).
