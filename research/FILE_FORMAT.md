# SimPHY `.ssim` File Format (v4.2)

Primary evidence: writer `org.shikhar.simphy.j.b`, loader `org.shikhar.simphy.j.c`
(both decompiled); cross-checked against all 67 bundled examples
(`research/evidence/xml_inventory.json`, `archive_manifest.json`).

The preceding source inspection is reported by the research provider. This
repository supplies the inventory/manifest and six extracted examples, but not
the decompiled source or all original archives. This is a working survey, not a
complete validated schema. See [evidence review](EVIDENCE_REVIEW.md).

## 1. Container

`.ssim` is a **ZIP archive** (PK magic). Fixed member `simulation.xml`; optional
embedded assets referenced by name inside the XML:
- images → `Textures` (png/jpg), `Files`
- audio → `Sounds` (`*.wav`/`*.mp3`)
- fonts → `Fonts`
- `screenshot.png` (auto-generated preview, in every bundled example)
- 3D models (e.g. `jeep.glb`) when a simulation embeds GLB assets

`archive_manifest.json` lists per-file entries, sizes, sha256.

## 2. Root element

```xml
<Simulation xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
            xsi:noNamespaceSchemaLocation="http://www.simphy.com"
            version="4.2"
            title="..." author="..." access="0" price="0.0"
            categories="3d,free" speed="1.0">
```
- `access` — gating level (see `dialog.open.invalidaccess` upgrade message).
- `price`, `categories`, `author` — simulation "store" metadata.
- `speed` — default playback time scale.

Top-level children (order as written):
`System`, `Description`, `Camera`, `Textures`, `Animations`, `Sounds`, `Fonts`,
`Files`, `Fields`, `Rays`, `World`, `LabTable`, `AssessmentQuestions`.

### System (machine that saved; informational)
`JavaVersion, JavaVendor, OperatingSystem, Architecture, NumberOfCpus, Locale`.

### Description
CDATA free text (can hold rich notes / LaTeX).

### Camera
```xml
<Camera>
  <Scale>#1.0</Scale>                 <!-- '#' prefix = special parse marker -->
  <CameraBodyID>12</CameraBodyID>      <!-- optional: camera follows body 12 -->
  <AxisBodyID>7</AxisBodyID>           <!-- optional: custom axis reference body -->
  <Translation x=".." y=".."/>
</Camera>
```

### Textures / Animations / Sounds / Fonts / Files
- `<Texture name width height wrap filter/>` — image resource.
- `<Animation name texture params><TextureRegion u1 v1 u2 v2/…></Animation>` —
  sprite-sheet animation; `params` = comma list (e.g. frame rate). A body/brush can
  reference an `Animation` to render animated sprites.
- `<Sound name soundPath/>`.
- `<Font …/>` (via `FontResource.toXML()`), `<File name filePath/>`.

## 3. Fields (top-level, act on bodies)
```xml
<Fields>
  <Field xsi:type="ElectricField|MagneticField|GravitationalField|BuoyancyField"
         name=".." enabled="true" color=".." zOrder="0">
    <xExprForce><![CDATA[expr(x,y)]]></xExprForce>
    <yExprForce><![CDATA[expr(x,y)]]></yExprForce>
    <FieldColor r g b a/>
    <FieldEnabled>boolExpr</FieldEnabled>       <!-- optional enable expression -->
    <FieldParams>…</FieldParams>                <!-- BuoyancyField only -->
    <ConvexBounds>…</ConvexBounds>              <!-- optional spatial region -->
  </Field>
</Fields>
```
Force fields are **expression-driven** (evaluated per body). Observed types in
examples: `ElectricField`, `MagneticField`; code adds `GravitationalField`,
`BuoyancyField`.

## 4. Rays
```xml
<Rays><Ray Name="..">
  <Start x y/><Direction>angle</Direction><Length>..</Length>
  <IgnoreSensors>..</IgnoreSensors><TestAll>..</TestAll>
</Ray></Rays>
```
Persisted raycasts (query objects).

## 5. World

```xml
<World>
  <Name>..</Name>
  <WorldBrush name params/>                     <!-- optional default sprite/brush -->
  <Settings>…</Settings>                         <!-- solver params, see §6 -->
  <Preferences>…</Preferences>                   <!-- see §7 -->
  <BroadphaseDetector>DynamicAABBTree|Sap|…</BroadphaseDetector>
  <NarrowphaseDetector>Gjk|Sat|…</NarrowphaseDetector>
  <ManifoldSolver>ClippingManifoldSolver|…</ManifoldSolver>
  <TimeOfImpactDetector>ConservativeAdvancement|…</TimeOfImpactDetector>
  <Gravity x="0" y="-9.8"/>
  <AirFriction linear quadratic windVelocity="x,y"/>   <!-- optional -->
  <gridsize>0.25</gridsize>
  <ConvexBounds>…</ConvexBounds>                 <!-- world AABB bounds -->
  <Bodies>…</Bodies>
  <Groups><Group>id,…</Group></Groups>           <!-- COMBody group ids -->
  <Joints>…</Joints>
  <Shapes>…</Shapes>                              <!-- 2D geometry, if active -->
  <Circuit><![CDATA[…]]></Circuit>               <!-- circuit, if active -->
  <Tracers>…</Tracers>
  <BodyControllers>…</BodyControllers>
  <ParticleSystem [attrs]><ParticleEmitter …/></ParticleSystem>
  <KeyBoardControllers><KeyController>…</KeyController></KeyBoardControllers>
  <ScriptManager><Script><![CDATA[JS]]></Script></ScriptManager>
  <GuiManager>
    <GuiXML><![CDATA[widget XML]]></GuiXML>
    <Tracer…/> <KeyController…/> <Widget…/> <Grapher…/> <Timer…/>
    <QuickEditDialog…/> <LabTable…/> <LabColumn…/>
  </GuiManager>
</World>
```
Detector values are **class simple names** of dyn4j algorithms; the four are
user-selectable in the "Simulation/advanced settings" dialog (evidence E006/E007/E008).

### 5.1 Bodies
Element is `<Body>` or `<PlaneBody>`. Attributes: `Id` (UUID), `Name`,
`strokeWidth`, optional `drawPattern`/`fillPattern` (DrawMode), `useRK4="true"`.

Children:
- `OutlineColor`, `FillColor` (r,g,b,a)
- `Transform` > `Translation`(x,y) + `Rotation`(degrees)
- `Mass` > `LocalCenter`, `Type` ∈ {`NORMAL`,`FIXED_ANGULAR_VELOCITY`,
  `FIXED_LINEAR_VELOCITY`,`INFINITE`}, `Mass`, `Inertia`, `Explicit`
- `Brush` (sprite/animation ref)
- `Fixtures` > one or more `Fixture` (see §5.2)
- `Velocity`(x,y), `AngularVelocity`(deg)
- `AccumulatedForce`(x,y), `AccumulatedTorque`
- Optional flags (only written if non-default): `AutoSleep`,`Asleep`,`Active`,
  `Bullet`,`DynamicallAddedBody`,`Renderable`,`BorderDrawn`,`renderFilled`,
  `FbdDrawn`,`Opacity`,`ZOrder`,`Charge`,`Touchable`,`Immortal`,`Killer`,
  `AttractionG`,`LinearDamping`,`AngularDamping`,`GravityScale`,`ScaleX`,`ScaleY`,
  `BodyText`(CDATA)

`COMBody` is serialized by inlining its member bodies.

### 5.2 Fixture
```xml
<Fixture Id=".." Name="..">
  <Shape Id xsi:type="Circle|Rectangle|Plane|Ellipse|Triangle|Polygon|Slice|Link|Segment|Parabola|Capsule|Ring|Convex">
    <LocalCenter x y/>
    <!-- geometry per type, see §5.3 -->
  </Shape>
  <Filter xsi:type="DefaultFilter|CategoryFilter">
    <PartOfGroups><Group1 Value="1"/>… or <All Value="2147483647"/></PartOfGroups>
    <CollideWithGroups>…</CollideWithGroups>
  </Filter>
  <Sensor>..</Sensor><Sticky>..</Sticky>
  <Density>..</Density>      <!-- only if != 1.0 -->
  <Friction>..</Friction>    <!-- only if != 0.2 -->
  <Restitution>..</Restitution> <!-- only if != 0.0 -->
</Fixture>
```
Collision filtering = 32-bit **category/mask** (`CategoryFilter`), groups named
`Group1..Group31` or `All`.

### 5.3 Shape geometry per `xsi:type`
| Type | Child elements |
|---|---|
| `Circle` | `Radius` |
| `Ring` | `Radius` |
| `Rectangle` | `Width`,`Height`,`LocalRotation` |
| `Ellipse` | `Width`,`Height`,`LocalRotation` |
| `Capsule` | `Width`(length),`Height`(=2·capRadius),`LocalRotation` |
| `Parabola` | `Width`,`Height`,`LocalRotation` |
| `Slice` | `Radius`,`Theta`,`LocalRotation` |
| `Plane` | `planeSize` attr, `LocalRotation` |
| `Triangle`,`Polygon`,`Link`,`Segment` | repeated `<Vertex x y/>` |
| `Convex` | (convex hull) |

Observed in 67 examples: Circle, Rectangle, Triangle, Polygon, Ring, Capsule.
Loader also accepts the rest.

### 5.4 Joints
```xml
<Joint Id Name xsi:type="…" zOrder="..">
  <BodyId1>..</BodyId1><BodyId2>..</BodyId2>
  <CollisionAllowed>bool</CollisionAllowed>
  <JointRenderable>..</JointRenderable><Opacity>..</Opacity><JointSize>..</JointSize>
  <DynamicallyAddedJoint>..</DynamicallyAddedJoint>
  <JointColor r g b a/>
  <!-- type-specific children, §5.5 -->
</Joint>
```
`xsi:type` observed in examples: `DistanceJoint, LineJoint, PrismaticJoint,
RevoluteJoint, RopeJoint, SpindleJoint, SpringJoint, WeldJoint, WheelJoint`.
Writer supports (full set, see PHYSICS_2D §4): `AngleJoint, DistanceJoint,
FrictionJoint, PinJoint, PrismaticJoint, PulleyJoint, RevoluteJoint, RopeJoint,
SpindleJoint, WeldJoint, WheelJoint, SpringJoint, TorsionalSpringJoint,
PathJoint, LineJoint, SpringRopeJoint`.

Type-specific elements (abridged):
- **DistanceJoint**: `Anchor1,Anchor2, Frequency, DampingRatio, Distance`
- **RevoluteJoint / WheelJoint / PrismaticJoint / AngleJoint / RopeJoint /
  PulleyJoint / WeldJoint / FrictionJoint / PinJoint / SpringJoint /
  TorsionalSpringJoint**: anchors + limits (`LowerLimit`,`UpperLimit`,
  `LimitEnabled`), motors (`MotorSpeed`,`MaximumMotorTorque/Force`,`MotorEnabled`),
  `ReferenceAngle`, `Ratio`(angle/pulley), `SpringConstant`,`NaturalAngle`,
  `MaximumForce/Torque`, `Target`(pin).
- **PathJoint**: `xExpr`,`yExpr`(CDATA exprs),`PathJointOffset`,`AlignBodyToPath`,`PathJointMu`
- **LineJoint**: `LocalAnchor`,`LineAngle`,`LineJointOffset`,`LineJointMu`,`LimitEnabled`,`LowerLimit`,`UpperLimit`
- **SpringRopeJoint**: `Anchor1,Anchor2,SpringConstant,DampingRatio,SpringNaturalLength,RopeLength`

### 5.5 Tracers
```xml
<Tracer>
  <TracerBodyID>bodyId|groupId</TracerBodyID>
  <TracerPoint x y/><TracerColor r g b a/>
  <TracerWidth>..</TracerWidth><TracerCount>..</TracerCount>
  <TracerInterval>..</TracerInterval><TracerMode>..</TracerMode>
  <TracerShowVel/ShowAcc/ShowAngularVel>bool</…>
  <VelocityDisplaySettings>b,b,b,b</VelocityDisplaySettings>
</Tracer>
```

### 5.6 BodyControllers
```xml
<BodyController type="ForceController|TorqueController|VelocityController|
  AccelerationController|PositionController|AngleController|AngVelocityController|
  PropertyController|TogglePropertiesController|SpringController|PathController|
  ValuePropertiesController" name=".." enabled="boolExpr" bodyid="..">
  <xExpr><![CDATA[expr]]></xExpr><yExpr><![CDATA[expr]]></yExpr>
  <!-- ForceController: ExtForcePoint + ForceMode -->
  <!-- PropertyController: ForceMode -->
  <!-- PathController: ForceMode (1 if ideal) -->
  <!-- SpringController: <SpringParams px py dx dy length k damping/> -->
</BodyController>
```
Controllers are **expression-driven** (x/y as functions of `t`, position, etc.).

### 5.7 ParticleSystem (SPH fluids)
```xml
<ParticleSystem density radius gravityScale maxCount particeSystemIteration
                strictContactCheck destroyByAge renderMode particleRenderScale zOrder>
  <ParticleEmitter type="ParticleEmitter|ParticleGroupCreater" name bodyid enabled
     <!-- ParticleEmitter: --> emitRate lifeTime maxParticleCount speed angle range
                                emitFromOrigin flags
     <!-- ParticleGroupCreater: --> stride flags groupFlags
  />
</ParticleSystem>
```

### 5.8 KeyBoardControllers
```xml
<KeyController>
  <ControllerBodyID>..</ControllerBodyID>
  <ControllerAccs>..</ControllerAccs><ControllerKeys>..</ControllerKeys>
</KeyController>
```
Keyboard-driven body (arcade controls).

### 5.9 ScriptManager / GuiManager
- `<ScriptManager><Script><![CDATA[JS]]></Script></ScriptManager>` — the embedded
  JavaScript (Rhino). This is where 3D scenes, games, custom behavior live.
- `<GuiManager>` holds `<GuiXML>` (the widget tree as CDATA XML, see
  GUI_WIDGETS.md) plus runtime widgets: `Tracer`, `KeyController`, `Widget`,
  `Grapher`, `Timer`, `QuickEditDialog`, `LabTable`, `LabColumn`.

## 6. Settings (solver) — `<World><Settings>`
The supplied inventory lists 17 direct child fields; all six inspected XML
fixtures contain these 17 fields. This establishes sample coverage, not a rule
that every writer/version always emits every field:
`StepFrequency`, `MaximumTranslation`, `MaximumRotation`(deg),
`ContinuousCollisionDetectionMode` ∈ {`NONE`,`BULLETS_ONLY`,`ALL`}, `AutoSleep`,
`SleepTime`, `SleepLinearVelocity`, `SleepAngularVelocity`(deg),
`VelocitySolverIterations`, `PositionSolverIterations`, `WarmStartDistance`,
`RestitutionVelocity`, `LinearTolerance`, `AngularTolerance`(deg),
`MaximumLinearCorrection`, `MaximumAngularCorrection`(deg), `Baumgarte`.

`StepFrequency` is serialized as frequency-like values `60.0`, `90.0`, `144.0`,
or `180.0` in the supplied inventory, not reciprocal values. The six local XML
files contain 60, 144, or 180. The earlier "stored as 1/Hz" statement was incorrect
for this evidence. Interpretation as Hz is consistent with those values and the
reported UI; `dt = 1 / frequency` is the corresponding seconds-per-step conversion,
not the stored value. Loader conversion, speed scaling, invalid-value handling and
defaults still need direct behavioral or source verification. Do not apply the
reciprocal twice or treat an example's setting as a universal default.

## 7. Preferences — `<World><Preferences>`
Renders/axis/display options, all always written:
`BackColor,ForeColor,SelectionColor, drawgrid,gridcolor,drawruler,draworigin,
drawbodycenter,drawbounds,drawvelocities,drawpositions,randomcolor,
randomFillColor,randomDrawColor,MinDisplayForce,MinDisplayVelocity,ImpulseScale,
VelocityScale,FBDForceNames,FBDForceColors,ThemeColors,FBDForceStates,
ForceDisplaySettings,Precision,drawElectricFieldLinesForCharge,
ElectricFieldLinesColor,customAxis,axisOrigin,axisAngle,trigAxis,axisFlipped,
showAxis,allowAxisRotation,allowCameraRotation,allowCameraAtCOMonly,coeffMixer,
gravityRendered,simulationLocked,windowWidth,windowHeight,
fontName,fontSize,fontStyle,fontName_gui,fontSize_gui,fontStyle_gui`.
- `coeffMixer` = `frictionMixer,restitutionMixer` (0/1/2 indices).
- `FBDForceNames` = comma list (e.g. `Mg,f,N,T,R,kx,Fe,F,Fps,Fcf,Fcori`).

## 8. Shapes (2D constructive geometry)
```xml
<Shapes>
  <GeometrySettings>…</GeometrySettings>
  <Shape2D Id Name Class="Circle2D|Line2D|…" Parents="id,id|X-AXIS,Y-AXIS"
           Params=";joined" DrawPattern DrawColor FillColor Touchable Visible
           OffsetPixels DisplayInfoFormat ShowEqn StrokeWidth>
    <ExtraShapeData><![CDATA[…]]></ExtraShapeData>   <!-- optional -->
  </Shape2D>
</Shapes>
```
`Class` = geometry class simple name; `Parents` reference other shape ids or the
sentinels `X-AXIS`/`Y-AXIS`. See GEOMETRY.md for the class catalogue.

## 9. Circuit
```xml
<Circuit><![CDATA[<serialized circuit token stream>]]></Circuit>
```
The inspected Potentiometer example contains newline-delimited records in CDATA,
with `$`, `v`, `w`, `r`, `p`, `)`, `s`, and `%` record tokens. Element class names
are not the record identifiers in that fixture. See [CIRCUITS.md §4](CIRCUITS.md#4-serialization)
for the observed record structure and unresolved grammar; a complete decoder
cannot yet be specified from this one example.

## 10. LabTable / AssessmentQuestions (top-level)
```xml
<LabTable chartSettings="…">
  <LabColumn id text formula format data="v1,v2,…" />   <!-- one per column -->
</LabTable>
<AssessmentQuestions>
  <AssessmentQuestion type="BOOLEAN|NUMERIC|SELECT|DESCRIPTION" options="a,b,c">
    <![CDATA[question text]]>
  </AssessmentQuestion>
</AssessmentQuestions>
```
See LAB_ASSESSMENT.md.

## 11. Loader behavior (j/c)
- SAX-based, case-insensitive element match, tolerant of unknown tags.
- Rebuilds bodies by Id, then wires joints/tracers/controllers by `*BodyID`.
- `CameraBodyID`/`AxisBodyID` attach camera/axis to restored bodies.
- Unknown `AssessmentQuestion type` → skipped with a log line.

## 12. Versioning & forward-compat
- Writer always emits full `<Settings>`+`<Preferences>`; optional flags only when
  non-default (keeps files small).
- `access`/`price`/upgrade path (E013 "save upgrade required") implies a
  freemium gate on some simulations.
