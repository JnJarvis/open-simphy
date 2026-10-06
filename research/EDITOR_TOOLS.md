# SimPHY Editor Tools, Menus & Shortcuts

Evidence: `toolbar.*` / `menu.*` / `menu.context.*` / `toolbar.preferences.*`
strings in `messages_english.properties`, main frame `e_0`, `AppToolBar`,
`NavToolBar`, `ScriptsPanel`.

## 1. Main simulation toolbar (Mechanics) — 47 tools, indexed -1..45
Labels come from `toolbar.info.tglEditBody{N}` (exact, OBSERVED):

| # | Tool |
|---|---|
| -1 | **Hand/Pan** — drag to move world; wheel zoom |
| 0 | **Select** — select item(s) to remove / inspect |
| 1 | **Frame of reference** — attach a reference frame to an object |
| 2 | **Graph** — generate plot for an object |
| 3 | **Free-body diagram** — enable/disable FBD |
| 4 | **Disc** — press-drag to create a disc |
| 5 | **Ring** — press-drag to create a ring |
| 6 | **Rectangle** — press-drag to create a rectangle |
| 7 | **Line segment (rod)** — press-drag to create a rod |
| 8 | **Gear** — press-drag; tooth size from toolbar spinners |
| 9 | **Static object** — press-drag to create a static body |
| 10 | **Polygon** — click to add vertices; right-click/first-vertex to close |
| 11 | **Distance joint** — press-drag between two objects |
| 12 | **Revolute (hinge) joint** — click on an object |
| 13 | **Spring** — press-drag to connect two objects |
| 14 | **Rope** — press-drag to connect two objects |
| 15 | **Prismatic joint** — press-drag between two objects |
| 16 | **Chain** — press-drag to create a chain |
| 17 | **Pulley joint** — press-drag; mouse point sets the pulley |
| 18 | **Force** — press-drag on an object to apply a force |
| 19 | **Impulse** — press-drag on an object to apply an impulse |
| 20 | **Tracer** — click an object to add a trajectory tracer |
| 21 | **Text/Font body** — pick font + text → glyph bodies |
| 22 | **Intersection** — click overlapping area to create a new object from the overlap |
| 23 | **Pie/Slice** — press-drag to create a slice |
| 24 | **Ellipse** — press-drag to create an ellipse |
| 25 | **Weld (overlap)** — click overlapping objects to weld |
| 26 | **Freehand shape** — press-drag to create a freehand shape |
| 26.5 | **Path (polyline)** — click to add vertices; right-click to finish |
| 27 | **Wheel joint** — press-drag; final point is the axle |
| 28 | **Parabola** — press-drag to create a parabola |
| 29 | **Triangle** — press-drag to create a triangle |
| 30 | **Capsule** — press-drag to create a capsule |
| 31 | **Gesture** — draw objects & joints with mouse gestures |
| 32 | **Slice** — press-drag over objects to slice them |
| 33 | **Ruler** — press-drag to create a ruler (measure length) |
| 34 | **Protractor** — press-drag to create a protractor (measure angle) |
| 35 | **Measure** — measure distances and angles |
| 36 | **Axis** — click then drag to create a custom axis |
| 37 | **Gravity** — click then drag to change gravity |
| 38 | **Zoom** — press-drag to zoom the camera |
| 39 | **Pan camera** — press-drag to pan the camera |
| 40 | **Draw** — drag & draw on the canvas (custom 2D) |
| 41 | **Constructive geometry** — press-move-press to create dependent geometry |
| 42 | **Path joint** — click body, set two anchor points, then path |
| 43 | **Circuit** — drag & draw to create circuits |
| 44 | **Optics** — click-move-click to create optical elements |
| 45 | **Mechanics (multi)** — drag & draw to create mechanics objects |

### Paint toolbar (`toolbar.paint.button*`)
Pencil, Pen/Brush, Eraser, Clear All, Thickness selector.

### Simulation control bar (top)
Start/Play, Step, Stop, Reset, FPS readout, Zoom In/Out, To Origin,
Mouse-location readout, Timer, New, Open, Save, Print, Undo, Redo, Cut, Copy,
Paste, Preferences, Tools, Pen, Help, Script Editor, Calculator, Table, Info
Mode, Toolbars, Delete, Add Image Body, Add Animation, Add Sound.

## 2. Toolbar tabs (6)
**Mechanics**, **Circuits**, **Optics**, **Geometry**, **Tools**, **Settings**.
- **Circuits** tab: element groups *Elements / Devices / Electronics / Tools*
  (see CIRCUITS.md).
- **Optics** tab: optical elements (see OPTICS.md).
- **Geometry** tab: constructive-geometry objects (see GEOMETRY.md).
- **Tools** tab: rulers, protractor, measure, axis, gravity, camera, draw,
  gesture, etc.
- **Settings** tab: display/physics option toggles.

## 3. Menu bar
- **File:** New, Save, Save As…, Download Simulations…, Open…, Import…,
  Insert…, Export… (→ **Java** code), Open Recent, Reload Simulation, Exit.
- **Snapshot:** Take Snapshot, Clear All Snapshots, Last Run (auto/manual,
  timestamped).
- **Simulations:** (recent / bundled / downloaded list).
- **View:** Preferences, Objects Table, Status Bar, Script Editor, Fullscreen,
  Look & Feel, Language.
- **Help:** Help Topics, Visit Homepage, Check for Updates, About, Language
  Settings, **Activate License / Deactivate License Key** (licensing).

## 4. Context menus (right-click)
- **On body:** edit properties (mass, friction, restitution, charge, damping,
  mass type, render flags, FBD, restrict motion X/Y/rotation, set mass
  infinite), attach tools (force/torque/velocity/acceleration/position/angle/
  path/spring controllers — 11+), plot graph, add tracer, FBD, delete, rename,
  group/ungroup, image/animation, script actions.
- **On joint:** edit params (limits, motor, spring constant), display plot,
  delete.
- **On bounds/field:** edit field expression, enable, color, region.
- **World (empty):** paste, add body/joint/field, preferences.
- **Animation folder (tree):** import image → "Create body from image contour"
  (image → body).
- **Widgets/Animations/Sound/Files/Fonts:** add/delete resources.
- **Assessment:** manage questions.

## 5. Keyboard shortcuts (OBSERVED)
- **Ctrl+Z / Ctrl+Y** — Undo / Redo
- **Ctrl+N** — New, **Ctrl+O** — Open, **Ctrl+S** — Save, **Ctrl+R** — Reset
- **Ctrl+T / Ctrl+Q** — tool/context actions
- **Ctrl+` (backquote)** — open Script Editor
- **F11** — fullscreen
- **Escape** — deselect / cancel tool
- **Delete** — delete selection
- **Arrow keys** — nudge selection
- **Space / Play** — start/pause simulation (context-dependent)

## 6. Preferences (toggles, `toolbar.preferences.*` / `preferences.*`)
Display: antialiasing, grid, grid size, body AABBs, body centers, body color,
body labels, rotation disc, body stenciling, body velocity vectors, fixture
labels, fixture normals, contact points/pairs/friction impulses, contact
impulses.
Physics/render: coefficient mixer (geometric mean), min display force/velocity,
impulse scale, velocity scale, FBD force names/colors/states, theme colors,
precision, electric-field line rendering, custom axis, gravity rendered,
simulation locked, window size, fonts (body & GUI: name/size/style).

## 7. Object lifecycle tools
- **Image → Body:** import an image, generate a body from its contour
  (`AddImageBody`, image-body dialog).
- **Add Animation:** sprite-sheet animation (texture regions + frame rate).
- **Add Sound:** attach audio (playback on events / via `Audio` API).
- **Merge / Split / Weld / Slice / Intersection** — boolean-style body editing.

## Cross-refs
FILE_FORMAT, PHYSICS_2D §4–5, CIRCUITS, OPTICS, GEOMETRY, GRAPHING_DATA,
FEATURES.md (SIMPHY-EDITOR-###, SIMPHY-UI-###).
