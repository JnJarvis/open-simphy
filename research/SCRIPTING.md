# SimPHY JavaScript API (Scripting)

Evidence: `org/shikhar/simphy/scripting/**` (host), `scripts.jar` → `simphy.script/**`
(API objects), `RhinoJsProvider`/`ScriptManager` (engine), examples with embedded
`<Script>` JS.

## 1. Engine
- A **Rhino-style ECMAScript engine** is bundled and re-skinned: `scripts.jar`
  contains the full ECMA-3/5 builtin layer under `simphy.script.js.ecma.api`
  (`JSArray, JSObject, JSMath, JSDate, JSRegExp, JSON, Function, String, Number,
  Boolean, Error, Undefined`) — i.e. a self-contained JS runtime with the usual
  builtins.
- Host bridge: `ScriptManager` creates a scope with global bindings
  (`ScriptManager.java:142`):
  - `App` (application/host services)
  - `World` → `WorldManager` (the main 2D API, ~150 public methods)
  - `Console` (print/ANSI console), `Actions` (timeline animations),
  - `Widgets` (GUI builder), `Geometry` (2D constructive geometry),
  - `Circuit` (circuit editing), `Resources` (images/sounds/fonts/files),
  - `Preferences`, `Threed` (3D factory), `TwoD` (2D canvas factory)
  - Constructor types exposed: `Vector2, Color, Path2D, Image, Transform, THREE`.
- Scripts can be **embedded in the file** (`<ScriptManager><Script>`), opened in
  the in-app **Script Editor** (Ctrl+`), run per-frame, on events, or one-shot.
  `ScriptsPanel` supports an external `scripts-src.zip`/`scripts.jar` library.
- `App.readHttpRequest` / `browseURL` allow network fetches from scripts.

## 2. `World` (WorldManager) — main 2D API (selection)
- **Lifecycle:** `update(dt)`, `clear()`, `clearAll()`, `resetTracers`,
  `setSimulationTime`, `getSimulationTime`, `loadNextSimulation`,
  `loadPrevSimulation`, `saveToJson()`, `loadFromJson(json)` (full state round-trip).
- **Bodies:** `createBody()`, `addBody`, `getBody(name)`, `getAllBodies()`,
  `removeBody`, `removeAllBodies`, `createCopy`,
  factories: `addPlane, addDisc, addRing, addRectangle, addPolygon, addCapsule,
  addSlice, addEllipse, addEdge, addPath`, `mergeBodies(...)`, `splitBody`.
- **Groups:** `createGroup()` (COMBody), `removeGroup`.
- **Joints (add…):** `addSpringJoint, addSpringRopeJoint, addDistanceJoint,
  addRopeJoint, addRevoluteJoint, addTorsionalSpringJoint, addPrismaticJoint,
  addWheelJoint, addWeldJoint, addDiscPulleyJoint, addSpindleJoint,
  addIdealPulleyJoint, addMotorJoint, addAngleJoint, addFrictionJoint,
  addPinJoint`; accessors `getJoint(name)`, `getAllJoints`, `removeJoint(s)`.
- **Fields:** `addField(name, ...)`, `getField`, `removeField(s)`,
  `getElectricFieldAt(pt)`, `getMagneticFieldAt(pt)`, `getElectricPotentialAt(pt)`.
- **Controllers:** `addController(body, type, xExpr, yExpr)`, `getController`,
  `removeController(s)`.
- **Tracers:** `addTracer(body, pt)`, `getTracer(s)`, `removeTracer(s)`,
  `resetTracer(s)`.
- **Graphs:** `addGraph(obj)`, `getGraph(obj)`, `resetGraphs`, `resetAllGraphs`.
- **Timers:** `addTimer()`, `resetAllTimers`.
- **Spatial queries:** `getBodyAt(pt)`, `getJointAt(pt)`,
  `screenToWorld`, `worldToScreen`, `getVectorInCameraFrame`,
  `getPointInCameraFrame`, `getScreenWidth/Height`, `setMinMaxField`.
- **Camera:** `getCamera()`, `setCameraBody(body, lock)`, `getCameraBody`,
  `zoomToFit`.
- **Particles:** `getParticleSystem()`, `getParticleEmitters()`.
- **Gravity:** `setGravity(x,y)`, `getGravity()`.

## 3. `simphy.script.physics2d.*` (returned object wrappers)
- `Body`: full property surface (position, velocity, mass, inertia, friction,
  restitution, charge, forces/torques, impulses, `translate`, `rotate`,
  `applyLinearImpulse`, `setLocalCenterOfMass`, `getWorldPt`, `getBodyPt`,
  `kineticEnergy`, `linearKE`/`rotationalKE`, `gravitationalPE`, `momentum`,
  `angularMomentum`, `acceleration`, `linearDamping`, `angularDamping`,
  `gravityScale`, `isSensor/isKiller/isImmortal`, `fbdDrawn`, `zOrder`,
  `opacity`, `strokeColor/fillColor`, `image`, `userData`, `getFixture(s)`).
- `COMBody` (group), `Joint` + all joint wrappers (`AngleJoint, DistanceJoint,
  FrictionJoint, MotorJoint, PinJoint, PrismaticJoint, PulleyJoint,
  RevoluteJoint, RopeJoint, SpindleJoint, SpringJoint, WeldJoint, WheelJoint`)
  with `LimitState`.
- `Field`, `BodyController`, `Grapher`, `Timer`, `Tracer`, `WorldCamera`.
- `actions/*`: `Action`, `Actionable`, `MultiAction`, `TemporalAction`
  (timeline/animation API for 2D bodies).

## 4. `simphy.script.geom.*` (2D geometry wrappers)
`Shape2D, Point2D, Line2D, Circle2D, Conic2D, Curve2D, DynamicCurve2D,
FunctionExplicit2D, ParametricCurve2D, PolarCurve2D` — script access to the
constructive-geometry objects (create, get/set params, parents, points).

## 5. `simphy.script.widgets.*` (GUI builder)
`Widget, ContainerWidget, Panel, Dialog, Desktop, SplitPane, TabbedPane,
MenuBar, Menu, ItemWidget, SelectableItemWidget, ActionItemWidget, Label,
Button, ToggleButton, CheckBox, TextField, PasswordField, TextArea,
ComboBox, ListBox, Slider, SpinBox, ProgressBar, Table, TreeView, Plotter`.
`Widgets.loadFromXml(...)` builds a whole UI from an XML string; scripts bind
`onAction`/`onMouse` handlers to widgets. (See GUI_WIDGETS.md.)

## 6. `simphy.script.canvas.*` (2D canvas drawing)
`Scene, Canvas, Context, Image, ImageData, Path2D, Gradient, FillPattern,
Transform, Color, Vector2, TextMetrics, MouseEvent, KeyEvent` — an HTML5-
canvas-like 2D drawing API for custom overlays.

## 7. 3D (`Threed` / `THREE` / `simphy.script.canvas.threed.*`)
- `Threed` factory: `createScene()`, `createWorld()` (→ `World3D`), `createLight`,
  geometry/material/mesh helpers.
- `THREE`: namespace object exposing constants & classes.
- Full `threed` scene graph (see PHYSICS_3D §4): `Scene3D, Object3D, Mesh,
  BufferGeometry, Geometries, Materials, Lights, Camera, Texture, Raycaster,
  Controls (Orbit/Trackball/FirstPerson/Drag/Transform)`, loaders, `math/*`
  (Vector2/3/4, Matrix3/4, Quaternion, Euler, Color, Box3, Plane, Ray, Sphere,
  Frustum, Line3), `objects/*` (Sprite, InstancedMesh, SkinnedMesh, Skeleton,
  Reflector, Refractor, Sky, Water, BitMapTextMesh), `lights/*`, `materials/*`.

## 8. `simphy.script.circuit.*`
`Circuit` (manager) + `CircuitElement` — create/connect/read circuit elements
programmatically; attach graphs to elements.

## 9. `App` (host services) — selection
`getVersion()`, `getPixelScalingFactor()`, `isPaused()/setPaused()`,
`setCursor/getCursor`, `browseURL`, `readHttpRequest`, `readInteger/readString/
readDouble/readExpression/readChoice/readFromList` (blocking user input),
`showErrorMessageBox/showWarningMessageBox/showOkCancelBox/showYesNoBox/
showYesNoCancelBox/showToast`, `roundoff`, `format`, `clearAllTimers`,
`setInterval/setTimeout/clearTimeout`, `setGlobalVariable/getGlobalVariable`,
`evaluateExpression`.

## 10. `Resources` (OBSERVED API)
`getFile(name)`, `getAllFiles()`, `getFont(name)`, `getAllFonts()`,
`getAllImages()`, `getAllAnimations()`, `getImage(name)`, `getAnimation(name)`,
`playAnimation(name, ...)` (3 overloads: at point / at x,y / in a canvas
`Context`), `getAllSounds()`, `getSound(name)`, `getSound(x,y,z)` (synthesized
tone), `playSound(name[, loop])`, `stopAllSounds()`.
So **`Audio`** and **`File`** are obtained via `Resources`, not bound as
top-level globals (top-level globals are App/World/Console/Actions/Widgets/
Geometry/Circuit/Resources/Preferences/Threed/TwoD + types Vector2/Color/
Path2D/Image/Transform/THREE).

## 11. `Console`
`print/log`, ANSI-colored output (`DragonConsole`/`ScriptingConsole`), console
widget, expression input.

## 12. `Actions`
Timeline animation API (`Actionable`, `TemporalAction`, `MultiAction`) — schedule
position/scale/color/opacity keyframes for 2D bodies & canvas.

## 13. `ParticleSystem` / `ParticleEmitter` / `ParticleGroup`
Script control of SPH fluids (emitters, groups, buffers, render settings).

## 14. `Preferences`
Read/write SimPHY preferences from scripts.

## Notes
- `World.saveToJson()/loadFromJson()` serializes **bodies, joints, camera,
  tracers, fields** (not 3D/circuit) — used for checkpoints & sharing state.
- `ScriptGenerater` (1010L) auto-generates a **standalone dyn4j Java program**
  from a simulation (File → Export → Java).
- `LimitState` models joint limits; `Filter`/`Bounds`/`Matrix4` are utility types.

## Cross-refs
ARCHITECTURE §3, FILE_FORMAT §5.9, PHYSICS_3D §5, GUI_WIDGETS, FEATURES.md
(SIMPHY-SCRIPT-###).
