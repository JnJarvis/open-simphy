# Open Simphy workspace preview

This is the runnable application workspace for interface feedback, backed by the
particle editor and gravity simulation. Its layout is a starting point for the main
application, not finished UI styling or a complete simulator. Three cyan
particles move independently; the gray axes are reference lines, not collision
surfaces. Particles can leave the screen. Press R to bring them back.

## Controls

- Space: play / pause (starts paused).
- Right Arrow: pause and advance exactly one physics step.
- R: stop simulation and return to authoring the initial scene at time zero.
- Escape or window close: exit.

The title bar and viewport header show simulation time. Resize preserves world scale and
center; minimizing skips rendering. Errors pause playback and appear in the title
and console. Recoverable output-size errors clear when the window fits again.

## Editing controls

- Click a particle to select; drag to move. Escape cancels a drag.
- N or Add creates a particle at the camera center. Delete removes the selection.
- Ctrl+Z / Ctrl+Y undo and redo up to 64 committed edits; IDs are never reused.
- Click a property value to edit mass, position, velocity, radius or gravity.
  Type a finite number, Enter applies, Escape cancels. Invalid values are rejected.
- Tab selects the first property; while entering a value, Tab moves to the next.
  Scroll over the properties panel to reach lower fields on smaller windows.
- Scroll over the scene to zoom; WASD pans.

Editing requires Authoring mode. Pausing an advanced simulation does not enable
editing: use Reset first. Reset retains the authored document, history and selection.
The toolbar provides Play/Pause, Step, Reset, Add, Delete, Undo, Redo, Home and Help. Error/status text
appears at the bottom. No save/load, collisions or advanced object types yet.

The canvas excludes the header, toolbar, object list, inspector and status bar.
CPU pixels are copied one-to-one into its own rectangle; input subtracts the same
physical viewport origin before picking or dragging.
UI size follows display scaling independently of physical world pixels per meter.
High-DPI layout uses adaptive toolbar widths and scrolling properties; moving a
window between scales cancels an active drag before refreshing coordinate mapping.

## Workspace layout for feedback

The left Scene Objects panel selects particles by identity, above the Properties
table for selected objects and world gravity. Both areas scroll independently in
small windows. The viewport has adaptive major/minor grid lines and meter rulers;
they follow camera pan and zoom and do not change physics or enable snapping.
Home returns the camera to the initial center and scale. Help or F1 opens a guide;
Escape closes the guide without exiting. Disabled-looking edit/history controls
reflect simulation mode, selection and history availability.

The scene list and viewport share one real selection and undo history. No fake
Save/Open/import buttons are shown: their file integration is still pending.
UI text currently uses SDL's built-in debug font; typography, docking, themes and
richer object tools are future refinements. The application still starts with the
three-particle demo so feedback can focus on layout and actual interactions.

## Build and run

Use the pinned toolchain described in spec/build-policy.md. On Windows run these
commands in a Visual Studio 2022 Developer PowerShell, from the checkout root:

```text
cmake -S . -B build/demo -G Ninja -DCMAKE_BUILD_TYPE=Debug -DOPENSIM_BUILD_APP=ON -DOPENSIM_BUILD_TESTS=ON
cmake --build build/demo --parallel 4
ctest --test-dir build/demo --output-on-failure
./build/demo/src/app/opensim_demo.exe
```

SDL 3.2.28 is acquired with the accepted pinned hash. An explicit verified offline
source may be passed as OPENSIM_SDL_SOURCE_DIR; Catch2 has a corresponding
OPENSIM_CATCH2_SOURCE_DIR. See cmake/SDL.md. SDL remains app-private; headless
presets still build all integration controller tests without SDL or a window.
Tests-off app builds need neither Catch2 nor Python. SDL-LICENSE.txt is copied
beside the built executable. This is a development build, not a licensed release
package; project release licensing remains unresolved.

Linux/macOS use the same code and commands (executable has no .exe suffix), but
native desktop support is not claimed until their window smoke checks pass.
Linux may require SDL's documented window-system development libraries.

## Scene and timing

Meters, seconds, +y upward. Gravity is (0,-9.8125) m/s². Initial particles:

| ID | Mass kg | Position m | Velocity m/s | Radius m |
|---|---|---|---|---|
| 1 | 1 | (-3,1) | (2,5) | .18 |
| 2 | 2 | (0,2) | (.5,2) | .25 |
| 3 | .5 | (3,1) | (-1,4) | .14 |

The camera initially centers at (0,0), 80 physical pixels per meter. Physics always
uses 1/128 second; the host accumulates elapsed steady-clock time. At most 32
steps (.25 seconds) are caught up per event-loop iteration; excess wall time after
stalls is discarded. Simulation can therefore lag wall time under load. Pausing
clears the remainder, single-step advances once, and reset clears the remainder and stops playback.
There is no frame-dependent change to the physics timestep.

## Smoke and failure checks

From a writable build directory with an interactive desktop:

```text
./src/app/opensim_demo.exe --smoke
./src/app/opensim_demo.exe --fail-window
./src/app/opensim_demo.exe --fail-texture
```

--smoke creates a real window/renderer/streaming texture, sends control events
through the normal handler, checks time, resizes, minimizes/restores, moves across
available displays, and closes. It saves smoke-initial.bmp, smoke-advanced.bmp and
smoke-resized.bmp from the native renderer before presentation. It prints actual
display scale/output sizes. Success exits 0. Failure injections intentionally exit
1 with contextual diagnostics; they do not open blocking error dialogs.

Visual checklist: inspect circles and upward/downward motion, axes, no stretching
on resize, minimize/restore, controls and clean close. Check both normal and high
DPI and, where available, moving between displays with different scales. A machine
with only one scale cannot establish cross-DPI behavior. Record the actual coverage
in tasks/reports/INT-002.md; headless tests alone are not native-window evidence.

The current --smoke additionally drives real editor events for selection, dragging,
undo/redo, numeric mass and scrolled gravity entry, creation and deletion. It saves
smoke-editor.bmp and smoke-properties.bmp as well as the simulation captures.
Automated integration tests cover 100/125/150/200 percent and asymmetric coordinate
ratios; these do not substitute for actual monitor testing. The local editor was
also exercised on a real Windows display reporting scale1.5. Mixed-monitor and
Linux/macOS interactive smoke remain separate hardware checks.

Open... / Ctrl+O / file drop opens a real SSIM source preview. Escape returns to
the authored scene. See [SSIM controls and limits](ssim.md); imported projects
require future body/collision/joint support before they can play.
