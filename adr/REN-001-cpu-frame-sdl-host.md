# REN-001: CPU frames with an SDL3 presentation host

Status: ACCEPTED following explicit user approval, 2026-10-06.

The first demo needs particles and simple overlays, stable headless checks, and
Windows interaction without platform code leaking into physics or scene. Select
a renderer-owned CPU raster backend returning RGBA8 frames and an app-private
SDL 3.2.28 adapter for window/events/texture presentation. No new module DAG edge.

Alternatives were direct SDL drawing, GLFW/OpenGL and raylib. The selected route
trades some CPU cost and explicit raster rules for display-independent tests and
a small host boundary. This is a bounded demo backend, not a full editor/GPU plan.

REN-002 freezes resource limits, pixel coverage and blend semantics. A new scoped
BUILD task is needed for headless renderer inclusion and optional pinned SDL
acquisition; REN-003 cannot silently modify root build files. INT-001 owns native
surface lifecycle and visual smoke tests. Full Linux/macOS desktop support remains
unclaimed until those native smoke checks exist. Upstream licensing notices must
be retained; the repository's own release license remains an open decision.
