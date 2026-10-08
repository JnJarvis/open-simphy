# SSIM source reader v1 — INT-004

Narrow implementation contract under the user-authorized SSIM opening direction.
inspect_ssim(span<const uint8_t>) -> core::Result<Project>. compat owns parsing;
app owns bounded file reads, native dialogs and presentation. No scene mutation.
Project owns archive bytes, title/version, member names, all element/type counts,
preview outlines and diagnostics. It never denotes a runnable simulation.
Outline geometry is a source preview only, with diagnostic omissions. API contains
no parser types, paths, SDL handles or callbacks. Failure returns no partial project;
allocation failure may propagate. Identical input yields deterministic counts.

Limits: 64 MiB input, 4096 members, 128 MiB total expansion, 8 MiB XML,
1000:1 ratio, 128 depth and 200000 element starts. Only stored/deflate ZIP, UTF-8
XML and observed root versions 4.0/4.1/4.2; reject ZIP64/encryption/links, ambiguous
or unsafe member paths, CRC failures, DTD/entities, malformed XML and other versions.
All members are streamed and CRC checked without extraction; original bytes retained.
UTF-16 and NUL are rejected. XML lexical starts bounded before DOM creation, depth
and node count verified afterward. This is a bounded first reader, not a fuzzing
coverage claim. Unknown nodes/types are inventoried, never advertised as supported.

Geometry outlines cover directly defined Circle, Rectangle, Triangle/Polygon/Segment
fixtures using finite source translation and degree rotations. Other shapes and
cloned/group-generated geometry remain inventoried with omissions explicit.
Scripts are counted but never run. Physical settings are source data only.
Existing scene/editor/physics/native serialization contracts remain untouched.

Preview budgets: 4096 outlines, 262144 points, 4096 vertices per polygon;
world preview coordinates limited to magnitude 1e9. Geometry beyond these budgets
is omitted with a counted diagnostic; original source remains fully retained.
At most 128 detailed omission messages plus the total omitted count are emitted.
