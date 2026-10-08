# Open a SimPHY project

Click **Open...**, press **Ctrl+O**, or drop a `.ssim` file onto the window.
The application reads the actual ZIP archive and `simulation.xml`. It displays the
project title/version, fixture outlines and a scrollable inventory of shapes,
joints, scripts, XML elements and archive members. Escape returns to your authored
scene. Failed or canceled opens retain the previous scene/source project.

The app has two explicit modes:

- **Experimental 2D rigid mechanics:** circles, rectangles, convex polygons and
  compound fixtures, finite static planes, rigid distance joints, hinges and winding
  joints can play, step, reset and be dragged. A bounded JavaScript bridge
  reconstructs supported startup-created copies and button actions. The supplied
  Newton Cradle shows the original ball, five colored textured copies, suspensions,
  its description and working Reset Cradle button. Pull a ball outward/upward,
  release it and press Play. Motion transfers through real circle contacts.
- **Source preview:** unsupported profiles retain the original archive, outlines
  and feature report. They cannot play. This currently includes the supplied
  charge/prismatic, resonance/spring, optics and circuit examples. The supplied
  Static and Kinetic Friction is supported, including its three sliders and RESET
  callback.

The mechanics profile is experimental. It does not reproduce every source solver
setting or all JavaScript APIs. Collision callbacks/sounds, dynamic scripting after
startup, unsupported force/controller types, concave/large polygons without
decomposition and additional joints/domains remain unsupported. Force modes 0/1/2
and property index 4 (friction) are supported; coefficient mixing preserves the
source geometric-mean/minimum/maximum preferences independently. Unknown APIs and resource failures retain source
preview rather than inventing replacement particles. See the
[profile contract](../spec/contracts/mechanism.md). Source widget buttons honor
bounded canvas positions; read-only descriptions dock at the top right to fit the
current window. Body colors/images are preserved; the workspace remains neutral.

Command-line opening and verification:

```text
opensim_demo --open path/to/project.ssim
opensim_demo --inspect path/to/project.ssim
opensim_demo --inspect-directory path/to/local/examples
opensim_demo --smoke-source path/to/project.ssim
opensim_demo --smoke-mechanism "path/to/Newton Cradle.ssim"
opensim_demo --smoke-rigid "path/to/Static and Kinetic Friction.ssim"
opensim_demo --check-mechanics path/to/project.ssim
opensim_demo --audit-mechanics-directory path/to/local/examples
```

The reader supports observed XML root versions 4.0, 4.1 and 4.2 with bounded stored
or deflated ZIP members and UTF-8 XML. Inspection never executes scripts; the
separate app mechanics preparation runs supported initialization in a bounded VM.
No extraction, external asset fetching, filesystem/network/script module access. `.sim` and other formats need their own supported loaders.
Limits and diagnostic behavior: [reader contract](../spec/contracts/ssim-reader.md).
Original third-party examples are not included in the repository.

## Dependencies and portability

The reader uses private [miniz 3.1.2](https://github.com/richgel999/miniz/releases/tag/3.1.2)
and [pugixml 1.16](https://pugixml.org/docs/manual.html), both MIT licensed. Official
archives are SHA256 pinned in cmake/OpenSimSSIM.cmake. MIT notices accompany the
executable alongside SDL's notice. No third-party types appear in public APIs.
C is enabled only when the reader/desktop dependencies are built.

Desktop builds enable OPENSIM_BUILD_COMPAT by default. Plain headless engine builds
keep it OFF and acquire neither parser. Enable it explicitly to run compatibility
tests headlessly: `-DOPENSIM_BUILD_COMPAT=ON -DOPENSIM_BUILD_APP=OFF`.
File paths/dialogs remain in the host; ZIP/XML inspection contains no OS branches,
filesystem operations or window code. Linux/macOS use the same reader.


The optional mechanics profile privately uses Box2D 3.1.1 (MIT), QuickJS-NG 0.17.0
(MIT) and stb_image at commit2c980bb59875b0d32144a71867fbdebb2f77cd20 (MIT).
Hashes and notices are pinned in cmake/OpenSimMechanics.cmake. Keep BOX2D-LICENSE.txt,
QUICKJS-LICENSE.txt and STB-LICENSE.txt beside the executable. The ordinary particle
headless configuration acquires none of these. OPENSIM_BUILD_COMPAT=ON builds the
portable mechanics/script profile with tests, including without a desktop window.
The solver uses float internally; canonical scene values remain binary64. Exact
cross-simulator or arbitrary-input numerical equivalence is not established.

The circle backend refreshes contacts and restitution through eight complete world
microsteps per canonical fixed step. This resolves close-chain transfer that was
incorrect when the previous backend refreshed contacts once per eight-substep frame.
Materials and authored damping remain intact; no source filename triggers special
physics. See demo.md for measured rendering and cradle regression commands.

## Rigid friction profile (INT-008)

The real friction archive contains a wedge, block, ground plane, hinged pulley and
winding string. Slider values feed the actual forces/materials before each step;
changing them keeps runtime positions and clock. The source RESET script restores
and repositions bodies. Picking tests fixture geometry, including rotated polygons.
Imported XML body origin and local COM are distinct; snapshots/script positions are
world COM and radians. Density does not overwrite explicit imported mass/inertia.

A 7555x4087 source JPEG exposed the old 4096-side image limit. Referenced images now
accept up to 8192 per side/32Mi source pixels, decoded sequentially and bilinearly
reduced to <=2048 per side; <=16Mi aggregate retained pixels and <=16MiB encoded
member limit remain. Original source bytes are unchanged. Built-in procedural brush
patterns currently use their source fill color; exact source visual/solver parity
is not claimed. Source plane is a finite 700-wide/50-deep box when no size is given;
its surface must lie inside the supported world envelope.

The 67-file bundled audit completes ten seconds plus reset for Newton Cradle and
Static and Kinetic Friction. The other 65 still report missing domains/features.
This is general support for these primitives, not a claim of all-file compatibility.
The coordinator capability matrix records each file's current blocking feature.

## Elastic mechanics and exact polygon pieces (INT-009)

Simple polygons with up to64 boundary vertices are split into convex pieces that
preserve their occupied area, including concave cutouts. Crossing/touching boundaries
and holes require a separately described shape. Compound bodies allow up to256
fixtures, with the4096 aggregate limit retained. Internal edges are hidden and body
textures remain continuous across pieces.

Authored SpringJoint, frequency-based DistanceJoint, bounded RopeJoint and rigid/
angular-soft WeldJoint now have runtime implementations and reset behavior. Springs
use actual stiffness/axial damping and angular lever arms; zero stiffness remains
free. These do not yet supply line/prismatic constraints, fields, particle systems,
tracers, all body-controller properties, graphs/events, optics, circuits or3D.
Required missing features still prevent playback: opening and displaying a file
are not evidence that its entire simulation executes. The bundled coverage remains
2/67 until those additional behaviors are implemented and tested.

For the independently authored geometry/elastic native check:
```text
python tests/compatibility/ssim/INT-009-native.py build/elastic-smoke.ssim
opensim_demo --smoke-elastic build/elastic-smoke.ssim
```
This checks spring extension and rope bounds through actual mouse events, then
Play/Reset/callback behavior. It is synthetic evidence, separate from bundled coverage.
