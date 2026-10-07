# Particle scene values

Implements accepted scene v1. Link opensim::scene; dependencies are core and math
only. Document::create validates explicit revision/gravity/particles/high-water
and sorts IDs. It never repairs a supplied low high-water mark. Validated values
have no mutable field access; spans borrow from their owning value.

Commands return a new Document and optional affected ID in Edit. They never change
the original, including failure and ID exhaustion. Snapshot::initial copies the
starting document at time zero. Snapshot::create validates runtime samples against
that document; IDs, mass and display radius remain fixed, gravity is copied from it.
Snapshot copies own their samples and outlive their producer. Retain the starting
document for reset; no stepping, playback policy or physics caches are implemented.

DrawPacket validates and owns ordered Circle/Line overlays in world units. Color
channels are straight-alpha sRGB in [0,1]. No renderer or native resource is needed.
Allocation failures propagate as standard exceptions. Text diagnostics carry field
paths and entity IDs when applicable; callers must inspect Result before dereference.
