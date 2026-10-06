# SimPHY 4.2 — Master Feature Map (stable IDs)

This index records 209 findings from the supplied SimPHY 4.2.2412.330 survey.
Each has a namespaced research ID, evidence classification, description and
subsystem reference. Preserve the category/number suffix when citing older maps;
see [the ID migration](README.md#stable-id-namespaces). These are findings, not tasks.

Evidence key: **R** = provider-reported finding, not independently verified;
**E** = directly inspected supplied XML example; **M** = supplied manifest or
inventory evidence; **I** = inference; **U** = unknown. The former O entries default
to R because they conflated code, examples and UI observations. Only explicitly
checked claims have been promoted to E or M. No entry asserts runtime verification
or compatibility PASS. [EVIDENCE_REVIEW.md](EVIDENCE_REVIEW.md) records checked claims,
their sources and remaining limits. Subsystem references alone are not test evidence.

## 1. File & persistence (FILE)
| Research ID | Evidence | Feature | Ref |
|---|---|---|---|
| SIMPHY-FILE-001 | M | `.ssim` is a ZIP with `simulation.xml` + embedded assets (images, sounds, fonts, files, screenshot) | FILE_FORMAT §1 |
| SIMPHY-FILE-002 | R | Root `<Simulation version="4.2" title author access price categories speed>` | FILE_FORMAT §2 |
| SIMPHY-FILE-003 | R | `<System>` machine metadata (Java/OS/CPU/locale) | FILE_FORMAT §2 |
| SIMPHY-FILE-004 | R | `<Description>` CDATA notes | FILE_FORMAT §2 |
| SIMPHY-FILE-005 | R | `<Camera>` scale/translation/cameraBody/axis | FILE_FORMAT §2 |
| SIMPHY-FILE-006 | R | `<Textures>`/`<Animations>` (sprite sheets) /`<Sounds>`/`<Fonts>`/`<Files>` | FILE_FORMAT §2 |
| SIMPHY-FILE-007 | R | `<Fields>` expression force fields (Electric/Magnetic/Gravitational/Buoyancy) | FILE_FORMAT §3 |
| SIMPHY-FILE-008 | R | `<Rays>` persisted raycasts | FILE_FORMAT §4 |
| SIMPHY-FILE-009 | R | `<World>` full subtree (bodies, joints, shapes, circuit, tracers, controllers, particles, scripts, GUI) | FILE_FORMAT §5 |
| SIMPHY-FILE-010 | R | Body/Fixture/Shape serialization, `xsi:type` polymorphism | FILE_FORMAT §5.1-5.3 |
| SIMPHY-FILE-011 | R | Joint serialization, all types | FILE_FORMAT §5.4 |
| SIMPHY-FILE-012 | R | Tracer serialization | FILE_FORMAT §5.5 |
| SIMPHY-FILE-013 | R | BodyController serialization (12 types) | FILE_FORMAT §5.6 |
| SIMPHY-FILE-014 | R | ParticleSystem/Emitter serialization | FILE_FORMAT §5.7 |
| SIMPHY-FILE-015 | R | KeyBoardController serialization | FILE_FORMAT §5.8 |
| SIMPHY-FILE-016 | E | `<ScriptManager><Script>` embedded JS | FILE_FORMAT §5.9 |
| SIMPHY-FILE-017 | R | `<GuiManager><GuiXML>` widget tree | FILE_FORMAT §5.9 |
| SIMPHY-FILE-018 | E | `<Settings>` has 17 child fields in the supplied inventory and six inspected XML fixtures | FILE_FORMAT §6 |
| SIMPHY-FILE-019 | R | `<Preferences>` display/axis/FBD params | FILE_FORMAT §7 |
| SIMPHY-FILE-020 | R | `<Shapes>` constructive geometry (`Class`, `Parents`) | FILE_FORMAT §8 |
| SIMPHY-FILE-021 | E | `<Circuit>` opaque token stream | FILE_FORMAT §9 |
| SIMPHY-FILE-022 | R | `<LabTable>`/`<LabColumn>` + `<AssessmentQuestions>` | FILE_FORMAT §10 |
| SIMPHY-FILE-023 | R | Collision `CategoryFilter` (32-bit groups) | FILE_FORMAT §5.2 |
| SIMPHY-FILE-024 | R | Version upgrade path (`access`/`price`, E013 upgrade dialog) | FILE_FORMAT §12 |
| SIMPHY-FILE-025 | R | Export to standalone Java (dyn4j) code | EDITOR_TOOLS §3 |
| SIMPHY-FILE-026 | R | Import / Insert / Save-As / Recent | EDITOR_TOOLS §3 |

## 2. 2D physics (PHY)
| Research ID | Evidence | Feature | Ref |
|---|---|---|---|
| SIMPHY-PHY-001 | R | dyn4j (Box2D v2 port) engine, heavily patched | PHYSICS_2D §1 |
| SIMPHY-PHY-002 | R | Fixed-step solver + user time scale | PHYSICS_2D §10 |
| SIMPHY-PHY-003 | R | Per-body optional RK4 integration (`useRK4`) | PHYSICS_2D §3 |
| SIMPHY-PHY-004 | R | Tunable solver settings (iterations, Baumgarte, tolerances, CCD) | PHYSICS_2D §2 |
| SIMPHY-PHY-005 | R | Selectable broadphase / narrowphase / manifold / CCD algorithms | PHYSICS_2D §2 |
| SIMPHY-PHY-006 | R | Mass types: NORMAL, FIXED_LINEAR, FIXED_ANGULAR, INFINITE (+Y_MOTION_ONLY) | PHYSICS_2D §3 |
| SIMPHY-PHY-007 | R | Fixtures: density, friction, restitution, sensor, sticky | PHYSICS_2D §3 |
| SIMPHY-PHY-008 | R | World air friction (linear+quadratic drag, wind) | FILE_FORMAT §5 |
| SIMPHY-PHY-009 | R | Gravity (world + per-field + per-body scale) | PHYSICS_2D §6 |
| SIMPHY-PHY-010 | R | Pairwise body attraction (`attractionG`), body `charge` | PHYSICS_2D §3 |
| SIMPHY-PHY-011 | R | COMBody (rigid composite group) | PHYSICS_2D §3 |
| SIMPHY-PHY-012 | R | Collision events (sensed/begin/persist/end/pre/post) via listener | PHYSICS_2D §1 |
| SIMPHY-PHY-013 | R | Collision filtering by category/mask (32 groups) | PHYSICS_2D §3 |
| SIMPHY-PHY-014 | R | Coefficient mixer (friction/restitution combine modes) | PHYSICS_2D §1 |
| SIMPHY-PHY-015 | R | Body merge/split/weld/slice/intersection editing | EDITOR_TOOLS §7 |
| SIMPHY-PHY-016 | R | Convex world bounds (AABB) | PHYSICS_2D §10 |
| SIMPHY-PHY-017 | R | Image → body (contour) | EDITOR_TOOLS §7 |
| SIMPHY-PHY-018 | I | Body "killer/immortal" semantics for lifetimes/particles | PHYSICS_2D §3 |

### 2D shapes (OBJ)
| Research ID | Evidence | Feature | Ref |
|---|---|---|---|
| SIMPHY-OBJ-201 | R | Circle | PHYSICS_2D |
| SIMPHY-OBJ-202 | R | Ring | PHYSICS_2D |
| SIMPHY-OBJ-203 | R | Rectangle | PHYSICS_2D |
| SIMPHY-OBJ-204 | R | Ellipse | PHYSICS_2D |
| SIMPHY-OBJ-205 | R | Triangle | PHYSICS_2D |
| SIMPHY-OBJ-206 | R | Polygon (convex) | PHYSICS_2D |
| SIMPHY-OBJ-207 | R | Capsule | PHYSICS_2D |
| SIMPHY-OBJ-208 | R | Slice (pie) | PHYSICS_2D |
| SIMPHY-OBJ-209 | R | Parabola (arc) | PHYSICS_2D |
| SIMPHY-OBJ-210 | R | Plane (infinite/static) | PHYSICS_2D |
| SIMPHY-OBJ-211 | R | Link / Segment (rod) | PHYSICS_2D |
| SIMPHY-OBJ-212 | R | Convex (hull) | PHYSICS_2D |
| SIMPHY-OBJ-213 | R | Path (polyline/freehand, convex-hull or segment) | PHYSICS_2D |

### 2D joints (OBJ)
| Research ID | Evidence | Feature | Ref |
|---|---|---|---|
| SIMPHY-OBJ-301 | R | Revolute (hinge) | PHYSICS_2D §4 |
| SIMPHY-OBJ-302 | R | Prismatic (slider) | PHYSICS_2D §4 |
| SIMPHY-OBJ-303 | R | Distance (rod) | PHYSICS_2D §4 |
| SIMPHY-OBJ-304 | R | Rope | PHYSICS_2D §4 |
| SIMPHY-OBJ-305 | R | Spring | PHYSICS_2D §4 |
| SIMPHY-OBJ-306 | R | Torsional spring | PHYSICS_2D §4 |
| SIMPHY-OBJ-307 | R | Spring+Rope combined | PHYSICS_2D §4 |
| SIMPHY-OBJ-308 | R | Wheel | PHYSICS_2D §4 |
| SIMPHY-OBJ-309 | R | Pulley | PHYSICS_2D §4 |
| SIMPHY-OBJ-310 | R | Spindle (winding) | PHYSICS_2D §4 |
| SIMPHY-OBJ-311 | R | Motor | PHYSICS_2D §4 |
| SIMPHY-OBJ-312 | R | Angle | PHYSICS_2D §4 |
| SIMPHY-OBJ-313 | R | Friction | PHYSICS_2D §4 |
| SIMPHY-OBJ-314 | R | Pin | PHYSICS_2D §4 |
| SIMPHY-OBJ-315 | R | Weld | PHYSICS_2D §4 |
| SIMPHY-OBJ-316 | R | Path joint (follow x(t),y(t)) | PHYSICS_2D §4 |
| SIMPHY-OBJ-317 | R | Line joint (motion constraint) | PHYSICS_2D §4 |

### Fields, controllers, particles, tracers
| Research ID | Evidence | Feature | Ref |
|---|---|---|---|
| SIMPHY-PHY-101 | R | Electric field (force on charge, field lines) | PHYSICS_2D §6 |
| SIMPHY-PHY-102 | R | Magnetic field (Lorentz) | PHYSICS_2D §6 |
| SIMPHY-PHY-103 | R | Gravitational field | PHYSICS_2D §6 |
| SIMPHY-PHY-104 | R | Buoyancy field | PHYSICS_2D §6 |
| SIMPHY-PHY-105 | R | Force controller (expr-driven) | PHYSICS_2D §5 |
| SIMPHY-PHY-106 | R | Torque controller | PHYSICS_2D §5 |
| SIMPHY-PHY-107 | R | Velocity controller | PHYSICS_2D §5 |
| SIMPHY-PHY-108 | R | Acceleration controller | PHYSICS_2D §5 |
| SIMPHY-PHY-109 | R | Position controller | PHYSICS_2D §5 |
| SIMPHY-PHY-110 | R | Angle controller | PHYSICS_2D §5 |
| SIMPHY-PHY-111 | R | Angular-velocity controller | PHYSICS_2D §5 |
| SIMPHY-PHY-112 | R | Property / Toggle-properties controller | PHYSICS_2D §5 |
| SIMPHY-PHY-113 | R | Spring controller (to point) | PHYSICS_2D §5 |
| SIMPHY-PHY-114 | R | Path controller | PHYSICS_2D §5 |
| SIMPHY-PHY-115 | R | Keyboard controller (arcade) | PHYSICS_2D §5 |
| SIMPHY-PHY-116 | R | Tracers (trajectory + velocity/accel display) | PHYSICS_2D §8 |
| SIMPHY-PHY-117 | R | SPH particle system (fluids: water/powder/etc) | PHYSICS_2D §7 |
| SIMPHY-PHY-118 | R | Particle emitters + groups, lifetimes, destroy-by-age | PHYSICS_2D §7 |
| SIMPHY-PHY-119 | R | Raycast queries (SimphyRay) | PHYSICS_2D §9 |

## 3. 3D physics & scene (SIMPHY-PHY-3xx / SIMPHY-OBJ-4xx)
| Research ID | Evidence | Feature | Ref |
|---|---|---|---|
| SIMPHY-PHY-301 | R | Oimo.js-based 3D engine (`org.simphy.phys3d`) | PHYSICS_3D §2 |
| SIMPHY-PHY-302 | R | Rigid body config (mass type, damping, sleep) | PHYSICS_3D §2 |
| SIMPHY-PHY-303 | R | 3D geometries: box, sphere, plane, cylinder, cone, capsule, convex, convex-hull, heightfield, particle, triangle | PHYSICS_3D §2 |
| SIMPHY-PHY-304 | R | GJK+EPA narrowphase; BVH/bruteforce broadphase | PHYSICS_3D §2 |
| SIMPHY-PHY-305 | R | 3D joints: distance, point-to-point, prismatic, revolute, cylindrical, spherical, universal, wheel, weld, line, spring, generic, ragdoll | PHYSICS_3D §2 |
| SIMPHY-PHY-306 | R | SpringDamper, RotationalLimitMotor, TranslationalLimitMotor | PHYSICS_3D §2 |
| SIMPHY-PHY-307 | R | Raycast vehicle (wheels, suspension) | PHYSICS_3D §2 |
| SIMPHY-PHY-308 | R | 3D world gravity, air friction, solver iterations | PHYSICS_3D §3 |
| SIMPHY-PHY-309 | R | World3D add* APIs (sphere/box/cylinder/cone/capsule/convex/heightfield/plane/gear/arrow/ragdoll/particles/merge) | PHYSICS_3D §3 |
| SIMPHY-PHY-310 | R | World3D rayCast/convexCast/aabbTest queries | PHYSICS_3D §3 |
| SIMPHY-PHY-311 | R | World3D debug draw, velocity/force/gravity visualization | PHYSICS_3D §3 |
| SIMPHY-PHY-312 | R | 3D scene built by embedded JS (no XML) | PHYSICS_3D §5 |
| SIMPHY-PHY-313 | R | THREE.js-style scene graph (`org.simphy.threed`): meshes, materials, lights, cameras, controls, loaders (SVG/Collada/GLTF/PLY), animation, shadows | PHYSICS_3D §4 |
| SIMPHY-PHY-314 | R | GLB/GLTF model import for 3D demos | PHYSICS_3D §4 |

## 4. Circuits (OBJ-C###)
| Research ID | Evidence | Feature | Ref |
|---|---|---|---|
| SIMPHY-PHY-401 | R | Time-domain MNA circuit solver (Gaussian elimination) | CIRCUITS §1 |
| SIMPHY-PHY-402 | R | Trapezoidal integration for L/C (per-element toggle) | CIRCUITS §1 |
| SIMPHY-PHY-403 | R | Elements: wire, resistor, capacitor, inductor | CIRCUITS §2 |
| SIMPHY-PHY-404 | R | Sources: DC battery, AC source, current source | CIRCUITS §2 |
| SIMPHY-PHY-405 | R | Switches: SPST (momentary/group), SPDT (n-throw), push | CIRCUITS §2 |
| SIMPHY-PHY-406 | R | Meters: ammeter, voltmeter, galvanometer (range, waveform, mode) | CIRCUITS §2 |
| SIMPHY-PHY-407 | R | Transformer, tapped transformer (coupling, ratio, polarity) | CIRCUITS §2 |
| SIMPHY-PHY-408 | R | Potentiometer (position 0..1) | CIRCUITS §2 |
| SIMPHY-PHY-409 | R | Ground / rail / output / square rail / AC rail | CIRCUITS §2 |
| SIMPHY-PHY-410 | R | Bulb (nominal P/V, light color) | CIRCUITS §2 |
| SIMPHY-PHY-411 | R | Diodes: diode, LED, zener | CIRCUITS §2 |
| SIMPHY-PHY-412 | R | Transistors: NPN, PNP (beta, swap E/C) | CIRCUITS §2 |
| SIMPHY-PHY-413 | R | Logic: NOT, AND, OR, NOR, NAND, XOR + logic in/out | CIRCUITS §2 |
| SIMPHY-PHY-414 | R | Antenna (RF) | CIRCUITS §2 |
| SIMPHY-PHY-415 | R | Per-element value can be expression (time-dependent) | CIRCUITS §1 |
| SIMPHY-PHY-416 | R | Per-element info (V/I/P) display, color-coding, current animation | CIRCUITS §1 |
| SIMPHY-PHY-417 | R | Attach graph to any element | CIRCUITS §5 |

## 5. Optics (VIS-O###)
| Research ID | Evidence | Feature | Ref |
|---|---|---|---|
| SIMPHY-PHY-501 | R | 2D ray tracing (reflection/refraction) | OPTICS §2 |
| SIMPHY-PHY-502 | R | Point source / ray / parallel beam / white light (nm) | OPTICS §1 |
| SIMPHY-PHY-503 | R | Real lens (R1,R2,thickness,index), ideal lens (focal length) | OPTICS §1 |
| SIMPHY-PHY-504 | R | Plane mirror, ideal mirror, arc, parabolic arc | OPTICS §1 |
| SIMPHY-PHY-505 | R | Custom reflector / refractor (curve expressions) | OPTICS §1 |
| SIMPHY-PHY-506 | R | Refractive index, reflectivity, dispersive power (prism spectrum) | OPTICS §1 |
| SIMPHY-PHY-507 | R | Image construction (real/virtual, foci, axis), observer | OPTICS §1-2 |
| SIMPHY-PHY-508 | R | Blocker (absorber) | OPTICS §1 |

## 6. 2D constructive geometry (OBJ-G###)
| Research ID | Evidence | Feature | Ref |
|---|---|---|---|
| SIMPHY-PHY-601 | R | Dependent geometry graph (parents, live recompute) | GEOMETRY §3 |
| SIMPHY-PHY-602 | R | Points: free, expression, on-curve, on-body, intersection, projection, ratio, reflection, relative | GEOMETRY §2 |
| SIMPHY-PHY-603 | R | Lines/rays/segments (parallel, perpendicular, bisector, tangent, normal) | GEOMETRY §2 |
| SIMPHY-PHY-604 | R | Circles & arcs (2/3 points, center-radius, diameter, arc) | GEOMETRY §2 |
| SIMPHY-PHY-605 | R | Conics: general equation, 5-point, ellipse, hyperbola, parabola | GEOMETRY §2 |
| SIMPHY-PHY-606 | R | Conic features: foci, center, pole, polar, tangents, normals, intersections | GEOMETRY §2 |
| SIMPHY-PHY-607 | R | Curves: function y=f(x), parametric, polar, dynamic, spline, bezier, path, expression plotter | GEOMETRY §2 |
| SIMPHY-PHY-608 | R | Vectors: 2-point, sum, difference, unit, parallel, perpendicular, equation | GEOMETRY §2 |
| SIMPHY-PHY-609 | R | Measurements: angle, length, expression, ruler, protractor, point | GEOMETRY §2 |
| SIMPHY-PHY-610 | R | Equation display (LaTeX-capable) | GEOMETRY §3 |
| SIMPHY-PHY-611 | R | Images (2/3/4 points), labels, text, shape info | GEOMETRY §2 |
| SIMPHY-PHY-612 | R | Critical points, osculating circle | GEOMETRY §2 |

## 7. Scripting (SIMPHY-SCRIPT-###)
| Research ID | Evidence | Feature | Ref |
|---|---|---|---|
| SIMPHY-SCRIPT-001 | R | Self-contained JS engine (ECMA-3/5 builtins) | SCRIPTING §1 |
| SIMPHY-SCRIPT-002 | R | Global objects: App, World, Console, Actions, Widgets, Geometry, Circuit, Resources, Preferences, Threed, TwoD, THREE | SCRIPTING §1 |
| SIMPHY-SCRIPT-003 | R | Constructor types: Vector2, Color, Path2D, Image, Transform | SCRIPTING §1 |
| SIMPHY-SCRIPT-004 | R | Embedded `<Script>` per simulation | SCRIPTING §1 |
| SIMPHY-SCRIPT-005 | R | In-app Script Editor (Ctrl+`), external script library (scripts-src.zip) | SCRIPTING §1 |
| SIMPHY-SCRIPT-006 | R | `World` 2D API (~150 methods: bodies, joints, fields, controllers, tracers, graphs, timers, camera, queries, JSON) | SCRIPTING §2 |
| SIMPHY-SCRIPT-007 | R | `physics2d.*` wrappers (Body, COMBody, Joint + 13 types, Field, Controller, Grapher, Timer, Tracer, Camera, actions) | SCRIPTING §3 |
| SIMPHY-SCRIPT-008 | R | `geom.*` wrappers (Shape2D/Point/Line/Circle/Conic/Curve/Function/Parametric/Polar) | SCRIPTING §4 |
| SIMPHY-SCRIPT-009 | R | `widgets.*` GUI builder (24 widget classes) | SCRIPTING §5 |
| SIMPHY-SCRIPT-010 | R | `canvas.*` HTML5-canvas-like 2D drawing | SCRIPTING §6 |
| SIMPHY-SCRIPT-011 | R | 3D `Threed`/`THREE` + full `threed` scene graph | SCRIPTING §7 |
| SIMPHY-SCRIPT-012 | R | `circuit.*` programmatic circuit editing | SCRIPTING §8 |
| SIMPHY-SCRIPT-013 | R | `App` host services (input, dialogs, timers, network, expression eval) | SCRIPTING §9 |
| SIMPHY-SCRIPT-014 | R | `Resources` (images/sounds/fonts/files) | SCRIPTING §10 |
| SIMPHY-SCRIPT-015 | R | `Console` (ANSI, prompt) | SCRIPTING §11 |
| SIMPHY-SCRIPT-016 | R | `Actions` timeline animation API | SCRIPTING §12 |
| SIMPHY-SCRIPT-017 | R | `ParticleSystem`/`ParticleEmitter`/`ParticleGroup` | SCRIPTING §13 |
| SIMPHY-SCRIPT-018 | R | `World.saveToJson()`/`loadFromJson()` state round-trip | SCRIPTING §2 |
| SIMPHY-SCRIPT-019 | R | Export simulation → standalone dyn4j Java | SCRIPTING §14 |

## 8. GUI / UI (SIMPHY-UI-###)
| Research ID | Evidence | Feature | Ref |
|---|---|---|---|
| SIMPHY-UI-001 | R | Swing app, FlatLaf look & feel (multiple L&Fs), i18n (multiple languages) | ARCHITECTURE §1 |
| SIMPHY-UI-002 | R | 47-tool mechanics toolbar + 6 tabs (Mechanics/Circuits/Optics/Geometry/Tools/Settings) | EDITOR_TOOLS §1-2 |
| SIMPHY-UI-003 | R | Paint toolbar (pencil/pen/eraser/clear/thickness) | EDITOR_TOOLS §1 |
| SIMPHY-UI-004 | R | Full menu bar (File/Snapshot/Simulations/View/Help) | EDITOR_TOOLS §3 |
| SIMPHY-UI-005 | R | Context menus (body/joint/field/world/animation/assessment) | EDITOR_TOOLS §4 |
| SIMPHY-UI-006 | R | Keyboard shortcuts (undo/redo/new/open/save/reset/script/fullscreen) | EDITOR_TOOLS §5 |
| SIMPHY-UI-007 | R | Preferences (display, physics, FBD, axis, fonts, coeff mixer) | EDITOR_TOOLS §6 |
| SIMPHY-UI-008 | R | Scripted widget system (`GuiXML`, 24 widgets, action/perform callbacks) | GUI_WIDGETS |
| SIMPHY-UI-009 | R | Objects table, status bar, console, calculator, script editor panels | EDITOR_TOOLS |
| SIMPHY-UI-010 | R | Free-body diagram (configurable force names/colors) | GRAPHING_DATA §3 |
| SIMPHY-UI-011 | R | Frame-of-reference attachment | EDITOR_TOOLS §1 |
| SIMPHY-UI-012 | R | Custom axis (origin/angle/flipped/rotation) | FILE_FORMAT §7 |
| SIMPHY-UI-013 | R | Camera follow (camera body), zoom, pan, fullscreen | EDITOR_TOOLS §1 |
| SIMPHY-UI-014 | R | Multi-language (Language Settings) | EDITOR_TOOLS §3 |
| SIMPHY-UI-015 | R | Licensing (activate/deactivate key) | EDITOR_TOOLS §3 |
| SIMPHY-UI-016 | R | Bundled / downloaded / recent simulations management | EDITOR_TOOLS §3 |

## 9. Visualization (SIMPHY-VIS-###)
| Research ID | Evidence | Feature | Ref |
|---|---|---|---|
| SIMPHY-VIS-001 | R | 2D canvas renderer (JOGL/OpenGL), antialiasing | ARCHITECTURE §2 |
| SIMPHY-VIS-002 | R | Body rendering: fill/stroke, opacity, z-order, dashed/dotted patterns | PHYSICS_2D §3 |
| SIMPHY-VIS-003 | R | Sprite/animation (texture regions, frame rate) on bodies | FILE_FORMAT §2 |
| SIMPHY-VIS-004 | R | Velocity / position / acceleration / angular-velocity vectors (scaled, min-threshold) | GRAPHING_DATA §3 |
| SIMPHY-VIS-005 | R | Joint rendering (color, opacity, size), contact points/pairs/impulses | EDITOR_TOOLS §6 |
| SIMPHY-VIS-006 | R | Field line rendering (electric) | FILE_FORMAT §7 |
| SIMPHY-VIS-007 | R | Tracer trails (width, count, interval, mode, velocity display) | PHYSICS_2D §8 |
| SIMPHY-VIS-008 | R | 3D renderer (THREE-style, software/JOGL), materials, lights, shadows | PHYSICS_3D §4 |
| SIMPHY-VIS-009 | R | Particle rendering (render modes, scale, Voronoi) | PHYSICS_2D §7 |
| SIMPHY-VIS-010 | R | Grid, ruler, protractor, measurement overlays | EDITOR_TOOLS §1 |
| SIMPHY-VIS-011 | R | Screenshot (PNG), auto `screenshot.png` in `.ssim` | FILE_FORMAT §1 |

## 10. Analysis / data (SIMPHY-ANALYSIS-###)
| Research ID | Evidence | Feature | Ref |
|---|---|---|---|
| SIMPHY-ANALYSIS-001 | R | Live grapher (body/joint/circuit/expression, time-series) | GRAPHING_DATA §1 |
| SIMPHY-ANALYSIS-002 | R | Plottable: position/speed/velocity/accel/momentum/angle/ang-vel/ang-acc/ang-mom/linear-KE/rot-KE/KE/PE/force/torque/length/current | GRAPHING_DATA §1 |
| SIMPHY-ANALYSIS-003 | R | Grapher polling interval, time span, continuous, stacked, CSV export | GRAPHING_DATA §1 |
| SIMPHY-ANALYSIS-004 | R | Virtual lab table (columns, formulas via exp4j, formats) | LAB_ASSESSMENT §1 |
| SIMPHY-ANALYSIS-005 | R | Lab stats: mean, median, mean deviation, std dev, error | LAB_ASSESSMENT §1 |
| SIMPHY-ANALYSIS-006 | R | Lab regression (none/constant/linear), multi-range, render styles | LAB_ASSESSMENT §1 |
| SIMPHY-ANALYSIS-007 | R | CSV import/export (lab + grapher) | LAB_ASSESSMENT §1 |
| SIMPHY-ANALYSIS-008 | R | Assessment questions: BOOLEAN/NUMERIC/SELECT/DESCRIPTION | LAB_ASSESSMENT §2 |
| SIMPHY-ANALYSIS-009 | R | Snapshots (take/clear/last-run, named, load) | LAB_ASSESSMENT §3 |
| SIMPHY-ANALYSIS-010 | R | Ruler/protractor/measure tools | GRAPHING_DATA §3 |
| SIMPHY-ANALYSIS-011 | R | Info mode (object property readout) | GRAPHING_DATA §3 |

## 11. Other (SIMPHY-OTHER-###)
| Research ID | Evidence | Feature | Ref |
|---|---|---|---|
| SIMPHY-OTHER-001 | R | Sound manager (playback, per-sound, events) | ARCHITECTURE §2 |
| SIMPHY-OTHER-002 | R | Fonts (embedded + GUI/body fonts) | FILE_FORMAT §2 |
| SIMPHY-OTHER-003 | R | Calculator (exp4j, live plot) | GRAPHING_DATA |
| SIMPHY-OTHER-004 | R | Expression language (exp4j) for values/controllers/fields/formulas/calculator | throughout |
| SIMPHY-OTHER-005 | R | License activation, online updates check, simulation download/upload (server) | EDITOR_TOOLS §3 |
| SIMPHY-OTHER-006 | R | Multi-touch (MultiTouch Test example) | PHYSICS_2D |
| SIMPHY-OTHER-007 | U | "Simulation bundle" / market (access/price/categories) — server-side store | FILE_FORMAT §12 |
| SIMPHY-OTHER-008 | R | About / help topics / homepage | EDITOR_TOOLS §3 |

## Resolved during final verification pass
- `CoefficientMixer`: friction = **geometric mean** `√(μ1·μ2)`, restitution =
  **max** `max(e1,e2)` by default; each mixer independently selectable
  Geometric-Mean/Min/Max in prefs. (PHYSICS_2D §1)
- `OpticalSettings` defaults: MAX_RAY_INTERACTIONS=10, MIN_INTERSECTIONS_FOR_
  IMAGE=2, defaultRefractiveIndex=1.5, defaultFocalLength=3.0, 
  defaultNumRays_Beam=5. (OPTICS §1)
- `Audio`/`File` are accessed via `Resources`, not top-level script globals.
  (SCRIPTING §10)
- Full `Resources` API captured (files/fonts/images/animations/sounds). 
  (SCRIPTING §10)

## Remaining open items
4. Full list of selectable broadphase/narrowphase/manifold/CCD algorithm
   classes beyond the defaults (enums exist in dyn4j; defaults are
   DynamicAABBTree / Gjk / ClippingManifoldSolver / ConservativeAdvancement).
5. Exact `access`/`price` gating semantics for the simulation store.
6. Multi-touch API surface (a "MultiTouch Test" example exists).
