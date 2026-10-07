# Renderer fixture provenance

All fixtures are synthetic C++ literals authored for REN-003 from the accepted
REN-002 contract, version 1. No third-party assets or external compatibility claim.
World coordinates/radii/widths are meters; camera scale is pixels per meter.
Expected RGBA bytes and covered pixel indices are specified independently from
raster output. Exact byte equality (zero tolerance) is required. Fixture source is
versioned with the implementation commit; there are no external fixture files.

pixels.cpp covers R01 and R03-R12/R21. limits.cpp in unit/renderer covers R02 and
R14-R20 using real preflight to avoid expensive maximum-budget painting.
lifecycle.cpp runs the same consumer harness against an independent recording
fixture and a recording wrapper of the CPU backend (R13/R22). FrameBuilder is a
private test seam, not a new public frame factory. No physics module is linked.
R22 native presenter errors and native-window smoke remain INT-001's responsibility.
