# SimPHY 4.2 — Application Architecture

Evidence base: decompiled `simphy.jar` (1592 classes) + `scripts.jar`, install at
`C:\Users\Dell4\AppData\Local\SimPHY\`, plus 67 bundled `.ssim` examples
(`research/evidence/xml_inventory.json`).

## 1. Platform & runtime

- **Language:** Java (Swing desktop app). Decompiled with CFR 0.152.
- **Launcher:** `SimPHY.exe` (native wrapper) + `SimPHY (Console).exe`; real entry
  `org.shikhar.simphy.SplashWindow`. Bundled JRE present in install dir.
- **Version:** app **4.2.2412.330** (build string in filename); XML schema
  `version="4.2"` written by current build (`p_0.a()`), examples carry `4.0`/`4.1`.
- **i18n:** `resources/languages/messages_*.properties` (English default + others).
  All UI strings are resource keys — the English properties file is an effective
  feature index (see `research/EDITOR_TOOLS.md`).

## 2. Package map (simphy.jar)

| Package | Role |
|---|---|
| `org.shikhar.simphy` (root) | Main frame `e_0` (13k lines), `WorldCamera`, world wrapper `l_0`, camera/axis globals `c` |
| `org.shikhar.simphy.dynamics` | 2D body wrappers (`SimphyBody`, `COMBody`, `PlaneBody`), `ConvexBounds`, `SimphyRay`, custom joints (`LineJoint`,`PathJoint`,`SpringRopeJoint`), `WorldStepListener`, `CollisionProcessor` |
| `org.shikhar.simphy.dynamics.controllers` | 13 body controllers (see PHYSICS_2D §5) |
| `org.shikhar.simphy.dynamics.fields` | `Field`, `ElectricField`, `MagneticField`, `GravitationalField`, `BuoyancyField` |
| `org.shikhar.simphy.dynamics.tracers` | `Tracer` (trajectory recording) |
| `org.shikhar.simphy.gfx.canvas.scene2d` | 2D canvas, rendering, actions/animation API (`Action2D`,`Actions2D`,`TemporalAction2D`) |
| `org.shikhar.simphy.gfx.canvas.scene3d` | 3D scene (`Scene3D`), `Threed` factory, `world3d.*` (Oimo physics bridge) |
| `org.shikhar.simphy.circuit` | Full circuit simulator (see CIRCUITS.md) |
| `org.shikhar.simphy.geom` | 2D constructive geometry + optics (see GEOMETRY.md, OPTICS.md) |
| `org.shikhar.simphy.gui` | Widget system (`Gui`,`GuiManager`,`SimphyWidget`), `Grapher`, `NavToolBar`, `AppToolBar`, `Calculator`, `LabTable` host |
| `org.shikhar.simphy.lab` | Virtual laboratory table + assessment questions |
| `org.shikhar.simphy.scripting` | JS engine host: `WorldManager`, `ScriptManager`, `RhinoJsProvider`, `DragonConsole`, script→code `ScriptGenerater` |
| `org.shikhar.simphy.renderer` | Texture/font/file/brush (sprite-animation) managers |
| `org.shikhar.simphy.audio` | `SoundManager`, audio playback |
| `org.shikhar.simphy.j` | **Persistence**: `b` (XML writer), `c` (SAX loader) — see FILE_FORMAT.md |
| `org.dyn4j` | 2D physics engine (modified Box2D port) — see PHYSICS_2D.md |
| `org.simphy.phys3d` | 3D physics engine (Oimo.js port, ~170 classes) — see PHYSICS_3D.md |
| `org.simphy.threed` | THREE.js-style WebGL-like 3D scene graph (~266 classes, CPU/software renderer) |

## 3. Bundled third-party libraries (in-jar or separate)

- **dyn4j** (2D physics, heavily patched — see "Major changes" in jar)
- **Oimo.js port** → `org.simphy.phys3d` (3D physics)
- **THREE.js-style scene graph** → `org.simphy.threed` (3D graphics)
- **FlatLaf** (Look & Feel), **JOGL/OpenGL** (2D canvas + 3D), **XChart** (graphs),
  **JLaTeXMath** (LaTeX rendering), **exp4j** (expression eval), **Rhino** (JS engine,
  patched `RhinoJsProvider`), **ANTLR** + exp4j (math expression parser),
  **svgSalamander** (SVG), **RSTA UI** widgets, GraalJS (also present).

## 4. Two physics worlds, one scene model

- **2D world** (`l_0` wraps a dyn4j `World`) is the default simulation. Everything the
  user creates with the Mechanics toolbar lives here. Persisted to XML.
- **3D world** (`World3D` wraps Oimo `World`) is **created at runtime by scripts**
  via `Threed.createWorld()` / `Threed.createScene()`. 3D content is **not** in the
  XML schema — it is reconstructed by the embedded `<ScriptManager><Script>` JS.
  (OBSERVED: all "3D Physics Demo" `.ssim` files build their scene purely in JS.)
- **Circuits** run in a parallel `CircuitManager`/`CirSim` loop (not dyn4j).
- **Optics/Geometry** are pure-geometry (`ShapesManager`), evaluated per-frame.

## 5. Event/step pipeline

- `WorldStepListener.updatePerformed(step, world)` runs each physics tick: applies
  forces/torques, evaluates fields & controllers, updates tracers, cameras, particles.
- `CollisionProcessor` implements dyn4j's contact callbacks (sensed/begin/persist/end/
  preSolve/postSolve) to expose collision events to scripts & UI.
- Simulation clock is user-scalable (`speed` attribute, playback speed control).

## 6. Build/tooling artifacts found

- `decomp/simphy/Major changes` — 127-line changelog of dyn4j modifications.
- `decomp/simphy/release_4.1.xml` — release notes.
- `CLASSES.html` — class index.

## Cross-refs
See: FILE_FORMAT.md, PHYSICS_2D.md, PHYSICS_3D.md, CIRCUITS.md, OPTICS.md,
GEOMETRY.md, SCRIPTING.md, GUI_WIDGETS.md, LAB_ASSESSMENT.md, EDITOR_TOOLS.md,
GRAPHING_DATA.md, FEATURES.md (master checklist).
