# INT-008 synthetic fixture provenance

All added XML/ZIP scenarios are independently authored in mechanism.cpp, generated
in memory with miniz, not copies of source simulation files. Units: SI, +y up;
XML body rotations degrees, script/snapshot angles radians. Synthetic conformance
vectors do not establish external compatibility. Geometry/pose tolerance 1e-12;
sliders and declared scalar values exact. Accepted rectangle/nested slider/force/
friction/reset scenarios and rejected concavity/unknown controller/property cases
are explicit in each test. Canonical physics references are in unit/physics/
mechanism.cpp with timestep and numerical bounds in the assertions/comments.

Real archives remain local/untracked and are separately tested via native/headless
CLI; their hashes and evidence are recorded in the coordinator completion report.
