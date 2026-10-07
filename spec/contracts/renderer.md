# CPU rendering port and raster contract v1

Status: ACCEPTED under REN-002 following explicit user approval, 2026-10-06.
Owner: renderer. Direct dependencies: core, math, scene; standard library backend.
Uses accepted REN-001 CPU-frame/SDL-host policy. No window, SDL, native handle,
physics object, file I/O, clocks, callbacks to app or platform-module dependency.

## Interface and lifecycle

The implementation target is a stateless `render(snapshot, packet, camera, extent)`
operation returning core::Result<Frame>. Camera contains math::Vec2 center and
double pixels_per_meter. Extent contains uint32 width,height. Snapshot and DrawPacket
are accepted scene values passed by const reference, never retained or mutated.
Frame owns width,height,stride and byte storage with const access. No public invalid
default Frame. Returned bytes remain valid until their owning frame is destroyed
or replaced; a copied frame owns independent bytes (immutable shared storage is
also acceptable). No public mutable pixel view into a published frame.

No explicit renderer init/shutdown is needed for the CPU backend. Each call renders
a full new frame; success has no dependence on a previous call. Resize means a new
Extent on the next call, same camera and world data. Never resample a prior image
or move particles to fit. Destroying a renderer helper cannot invalidate Frames.

App owns the native host surface and presents successful Frames as described by
REN-001; no native surface crosses this CPU port. A test presenter records frames
instead of creating a window. App must not replace its last good frame on render
failure. Native host failures are distinct from CPU errors and do not change the
snapshot, physics time or last good CPU frame. INT-001 owns the actual SDL adapter.

## Limits, validation and diagnostics

These are explicit initial demo limits, not physical scene validity constraints:

| Input/work | Limit / failure |
|---|---|
| Width and height | Each 1..4096; otherwise invalid_argument, path extent |
| Total pixels | width*height <= 4,194,304; otherwise invalid_argument/extent |
| Camera center and scale | finite center, finite scale>0; otherwise invalid_argument/camera |
| Particle count + overlay count | <=4096; otherwise invalid_argument/primitives |
| Projected centers/endpoints | absolute x/y <=1,048,576 and finite; otherwise invalid_data/geometry |
| Projected radius or line width | finite, >0, <=1,048,576; otherwise invalid_data/geometry |
| Total candidate pixel visits | <=67,108,864 clipped bounding-box pixels over all shapes; otherwise invalid_argument/work_budget |

Use uint64/checked arithmetic for pixel products, count sums and candidate-area sums
before narrowing/allocation. Maximum output bytes are 16,777,216; stride=width*4.
No allocator request happens before frame limits pass. Count all particles and
overlays, including transparent/offscreen ones. All geometry must be validated even
if outside the viewport or alpha=0. No silent omission of an invalid primitive.
The candidate-area budget includes transparent shapes but excludes fully clipped
empty boxes. Frame clear is bounded by total pixels, separate from that budget.

Validate/project/preflight the whole request before raster work and publication.
Any failure returns one error-severity Diagnostic, never a partial Frame. Geometry
failure identifies the particle EntityId when applicable; overlay errors have no
entity and may include packet index in message. Code/path are stable; message is
explanatory. Allocation failure may throw bad_alloc and still cannot mutate inputs
or any published frame. Do not catch bad_alloc and allocate another diagnostic.

Huge finite world values can exceed the render projection budget while remaining
valid scene/physics values. Report that rendering limitation without clamping world
state. Minimized/zero-size host output is handled by app skipping render, not by
passing zero dimensions and expecting a success Frame.

## Projection and safe clipping

For W,H, camera center c and scale s, use double intermediates in this order:

```text
dx=x-c.x; dy=y-c.y             # reject nonfinite differences
sx=dx*s; sy=dy*s              # reject nonfinite products
px=double(W)/2 + sx
py=double(H)/2 - sy
radius=world_radius*s; width=world_width*s
```

Then enforce the projected limits above. Do not algebraically cancel overflowing
terms or convert nonfinite/out-of-range floats to integers. World +y goes upward;
row numbers go downward. Pixel (i,j) is sampled at (i+0.5,j+0.5). No anti-aliasing,
subpixel jitter, unit conversion, camera rotation or hidden DPI scaling.

Circle floating bounds are center +/- radius. Segment bounds are componentwise
min/max endpoints +/- width/2. For each axis compute a conservative half-open
candidate interval [floor(lower),ceil(upper)), then clamp both values in floating
point to [0,dimension]. Only then convert to an integer index. Empty intervals do
no work. Candidate area is the product of the clipped interval lengths and counts
toward the budget even if no center is ultimately covered. Limits on projected
geometry keep all these operations finite and representable. Never iterate to a
world-derived unbounded coordinate or cast before clipping.

## Coverage, ordering and appearance

Clear background RGBA=(16,20,28,255). Every particle is a filled circle at its
snapshot position with its display_radius, color RGBA=(64,192,255,255). Traverse
particles in snapshot's ascending ID order, then overlay primitives in packet order.
No shape is outlined. Later covered samples composite over earlier values.

Circle coverage: with pixel-center offset (dx,dy), include exactly when
dx*dx+dy*dy <= radius*radius. Equality is included; no epsilon. Range bounds ensure
no overflow in these squares. Floating-point boundary behavior uses binary64;
golden examples use exactly representable coordinates away from ambiguous rounding.

Line coverage is a round-capped capsule: let a,b be projected endpoints, d=b-a,
q=pixel_center-a and length_squared=dot(d,d). If length_squared==0 (including
underflow), use the circle test centered at a with radius width/2. Otherwise set
u=clamp(dot(q,d)/length_squared,0,1), nearest=a+u*d, and use squared distance from
pixel center to nearest <= (width/2)^2. For a nonzero subnormal squared length,
avoid overflow in the division by checking numerator<=0 or >=length_squared first;
only divide when 0<numerator<length_squared. Products are bounded by the projected
coordinate limits. Do not expand width to a minimum visible pixel or use Bresenham
instead: that would have different coverage and cap semantics.

This is a display preview: compositing intentionally occurs in encoded sRGB values,
not linear light. The choice is explicit and may not be replaced by a host default.
Quantize each scene color component once: q(c)=floor(255*c+0.5), in [0,255].
For source byte Cs, source alpha As and opaque destination Cd, each RGB output is:

```text
out = (Cs*As + Cd*(255-As) + 127) / 255  # integer division
alpha_out = 255
```

Use unsigned integer arithmetic wide enough for intermediates (uint32 suffices).
As=0 preserves the destination; As=255 replaces it. All Frames are opaque after
compositing over the opaque background. Scene values remain straight-alpha; output
is already composited. App uses no second alpha blend when copying the texture.
Memory is literal RGBA bytes, top-to-bottom rows, stride width*4, no row padding.
Do not reinterpret as a native-endian packed uint32 pixel.

## Shared port tests and doubles

Consumer-owned RecordingRenderer has the same render call shape. It owns copies of
the submitted snapshot/packet/camera/extent and returns an independently specified
small valid Frame fixture. It can inject a chosen core Diagnostic on the next call.
It never calls World::step, reads a timer, opens a window or executes CPU raster code.
A production renderer has no persistent recording requirement; a test wrapper can
record calls around it. Do not create a new runtime plugin framework solely for tests.

Run a common consumer harness against a recorder and a wrapper around the real
backend: submit once, keep returned frame after input destruction, resize, submit
empty input, confirm input/time unchanged, retain last good frame on failed request.
Recorder tests prove call/ownership/failure plumbing; only real-backend tests prove
pixels. Host presentation recorder records byte order/stride and can inject failure;
actual SDL smoke is required separately at INT-001. Do not count either fake as a
successful native-window test. REN-003 owns CPU tests; INT-001 owns app/presenter tests.

## REN-003 conformance vectors

Use synthetic tiny scenes/packets with explicit provenance. Unless stated, camera
center=(0,0), scale=1, no particles/overlays. Compare bytes exactly, metadata exactly.
Projection helper checks may use absolute tolerance1e-12 for non-dyadic examples;
no pixel tolerance or percentage mismatch is permitted for golden cases below.

| ID | Input/action | Expected |
|---|---|---|
| R01 | 2x2 empty frame | 16 bytes: four copies of (16,20,28,255), stride8 |
| R02 | W800,H600,s100: (1,2), (0,-1), radius .5, width .1 | (500,100), (400,400), 50,10; no y double-flip |
| R03 | 4x4 white circle center world(0,0), radius1 | only indices (1,1),(2,1),(1,2),(2,2) white |
| R04 | 4x4 white circle center world(-1.5,1.5), radius .5 | only pixel(0,0) white; pixel-center convention |
| R05 | Same center, radius1 | pixels(0,0),(1,0),(0,1) white; equality included and negative candidates clipped |
| R06 | 4x4 white line world(-1.5,1.5)->(.5,1.5), width1 | row0 columns0,1,2 white; no row1 coverage |
| R07 | zero-length line at world(-1.5,1.5), width1 | identical to R04 |
| R08 | cover pixel with red alpha.5 over background | source As128; result (136,10,14,255) |
| R09 | then cover same pixel with blue alpha.5 | result (68,5,135,255); reverse order gives (132,5,71,255) |
| R10 | transparent valid overlay; opaque overlay | first preserves background; second exactly replaces RGB |
| R11 | particle at R04 center/radius, overlay white same shape | particle alone (64,192,255,255); overlay wins white |
| R12 | render same snapshot at 4x4 then 8x4 | origin moves from(2,2) to(4,2); same scale/radius, unchanged snapshot time/data |
| R13 | retain/copy Frame, destroy inputs and render again | old/copy bytes and dimensions unchanged |
| R14 | width0,height0,dimension4097,4096x4096 | invalid_argument/extent before allocation |
| R15 | 2048x2048 empty; one dimension larger causing pixel cap | first allowed, second rejected when cap exceeded |
| R16 | camera NaN/inf, s0/negative/inf | invalid_argument/camera |
| R17 | valid scene particle at max-double, camera center=-max-double | invalid_data/geometry with particle ID; no unsafe cast |
| R18 | valid huge radius or far coordinate projecting >2^20; scale causes radius underflow to0 | invalid_data/geometry even if offscreen/transparent |
| R19 | 4097 valid primitives | invalid_argument/primitives; no raster work |
| R20 | 17 circles each covering full2048x2048 bounding box | invalid_argument/work_budget; 16 fit exactly (67,108,864) |
| R21 | fully offscreen valid bounded circle; empty clipped box | background unchanged; zero candidate visits |
| R22 | recorder-injected render/presenter failure after a good frame | caller keeps good frame, returns diagnostic, no physics advancement |

For R20 use screen center(1024,1024), radius2048; snapshot empty and opaque overlay
circles, camera scale1. Bounds cover full frame; count limit is not the failure.
R15 can use 2049x2048 for the exceeding case. R18 uses valid positive world radius
denorm_min and scale0.5 for the underflow case. R08/R09 golden color values come
from the integer formula, not generated screenshots. Add malformed camera/limits
tests and a public-header-isolation translation unit. Maximum-size/budget arithmetic
can use private pure preflight helpers instead of repeatedly allocating/painting
large images. Keep at least one real tiny clipped raster test for each shape.

## Implementation sequence and stop conditions

First implement checked request/preflight, projection and frame ownership. Then
tiny circle tests, capsule tests, integer blending, ordering and independent pixel
goldens. Only after these pass connect the render call. BUILD-003 owns headless
registration; never turn APP=ON to hide missing renderer tests. Do not introduce SDL
or app code under REN-003. Stop for coordinator review if a new limit, shape or
pixel rule is needed; don't invent one or weaken a fixture to match implementation.
