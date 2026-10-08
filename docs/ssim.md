# Open a SimPHY project

Click **Open...**, press **Ctrl+O**, or drop a `.ssim` file onto the window.
The application reads the actual ZIP archive and `simulation.xml`. It displays the
project title/version, fixture outlines and a scrollable inventory of shapes,
joints, scripts, XML elements and archive members. Escape returns to your authored
scene. Failed or canceled opens retain the previous scene/source project.

The app has two explicit modes:

- **Experimental circle mechanics:** eligible single-circle bodies and rigid
  distance joints can play, step, reset and be dragged. A bounded JavaScript bridge
  reconstructs supported startup-created copies and button actions. The supplied
  Newton Cradle shows the original ball, five colored textured copies, suspensions,
  its description and working Reset Cradle button. Pull a ball outward/upward,
  release it and press Play. Motion transfers through real circle contacts.
- **Source preview:** unsupported profiles retain the original archive, outlines
  and feature report. They cannot play. This currently includes the supplied
  charge/prismatic, resonance/spring, plane/friction, optics and circuit examples.

The mechanics profile is experimental. It does not reproduce every source solver
setting or all JavaScript APIs. Collision callbacks/sounds, dynamic scripting after
startup, arbitrary forces/controllers, compound/non-circle geometry and other
joints/domains remain unsupported. Unknown APIs and resource failures retain source
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
