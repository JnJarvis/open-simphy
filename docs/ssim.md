# Open a SimPHY project

Click **Open...**, press **Ctrl+O**, or drop a `.ssim` file onto the window.
The application reads the actual ZIP archive and `simulation.xml`. It displays the
project title/version, fixture outlines and a scrollable inventory of shapes,
joints, scripts, XML elements and archive members. Escape returns to your authored
scene. Failed or canceled opens retain the previous scene/source project.

This first slice is a **source preview**. Direct circle, rectangle and polygon
fixture definitions are outlined. Other geometry, clones, textures, joints,
scripts, circuits, optics and 3D content are retained in source bytes and counted;
they are not yet executed or fully drawn. Play is unavailable in source preview.
It does not establish SimPHY simulation parity. Editable bodies, collisions and
joints are the next engine milestones, prioritized over additional UI polish.

Command-line opening and verification:

```text
opensim_demo --open path/to/project.ssim
opensim_demo --inspect path/to/project.ssim
opensim_demo --inspect-directory path/to/local/examples
opensim_demo --smoke-source path/to/project.ssim
```

The reader supports observed XML root versions 4.0, 4.1 and 4.2 with bounded stored
or deflated ZIP members and UTF-8 XML. No extraction, external asset fetching or
script execution. `.sim` and other formats need their own supported loaders.
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
