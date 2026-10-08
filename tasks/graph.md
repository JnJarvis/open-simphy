# Task dependency chart

Updated 2026-10-08 from [registry.json](registry.json). Registry is authoritative.

INT-006 is approved and merged. INT-007 continuous contacts, viewport navigation and responsive Open remain in review. INT-008 adds general rigid fixtures, contacts, hinge/winding constraints and live friction controls; all 92 local tests and 14 platform jobs pass. The real Newton Cradle and Static and Kinetic Friction run; the bundled audit still identifies missing features in the other 65 of 67 files. Both implementation workers are stopped; their branches await independent review and merge. Full controllers, remaining joints, optics, circuits, 3D and scripting compatibility remain further work.

DONE: 27 | REVIEW: 3 | IN_PROGRESS: 0 | READY: 1 | BLOCKED: 2

```mermaid
flowchart TD
  BUILD_001["BUILD-001: Select foundation toolchain and coding policy<br/>DONE"]
  CORE_001["CORE-001: Specify IDs and diagnostics contracts<br/>DONE"]
  MATH_001["MATH-001: Specify 2D numeric contracts<br/>DONE"]
  REN_001["REN-001: Select minimal render strategy<br/>DONE"]
  COMPAT_001["COMPAT-001: Prepare research intake templates<br/>READY"]
  BUILD_002["BUILD-002: Create CMake and CI test scaffold<br/>DONE"]
  TEST_001["TEST-001: Add test registration and fixture conventions<br/>DONE"]
  CORE_002["CORE-002: Implement stable ID and diagnostic values<br/>DONE"]
  MATH_002["MATH-002: Implement minimal vector and transform operations<br/>DONE"]
  SCENE_001["SCENE-001: Specify minimal document and snapshot contracts<br/>DONE"]
  PHY_001["PHY-001: Specify fixed-step particle simulation port<br/>DONE"]
  REN_002["REN-002: Specify render port and test doubles<br/>DONE"]
  SCENE_002["SCENE-002: Implement particle document validation and snapshots<br/>DONE"]
  PHY_002["PHY-002: Implement uniform-gravity particle stepper<br/>DONE"]
  REN_003["REN-003: Implement minimal particle renderer<br/>DONE"]
  INT_001["INT-001: Integrate the first runnable simulation<br/>BLOCKED"]
  COMPAT_002["COMPAT-002: Normalize supplied SimPHY capability research<br/>BLOCKED"]
  BUILD_003["BUILD-003: Prepare portable renderer builds and optional SDL host dependency<br/>DONE"]
  BUILD_004["BUILD-004: Validate desktop application builds across native platforms<br/>DONE"]
  EDT_001["EDT-001: Specify transactional particle editing, picking and history<br/>DONE"]
  INT_002["INT-002: Build and integrate particle editor<br/>DONE"]
  IO_001["IO-001: Specify bounded native particle persistence<br/>DONE"]
  IO_002["IO-002: Implement bounded native document codec<br/>DONE"]
  PLAT_001["PLAT-001: Specify bounded reads and safe file replacement<br/>DONE"]
  COMPAT_003["COMPAT-003: Inspect real SSIM archives and bounded container validation<br/>DONE"]
  INT_003["INT-003: Build application workspace for interface feedback<br/>DONE"]
  COMPAT_004["COMPAT-004: Define SSIM opening and rigid-body migration<br/>DONE"]
  INT_004["INT-004: Open real SSIM archives and inspect project contents<br/>DONE"]
  INT_005["INT-005: Scalable workspace and functional imported circle mechanisms<br/>DONE"]
  SCENE_003["SCENE-003: Specify compound rigid bodies and immutable body snapshots<br/>REVIEW"]
  INT_006["INT-006: Optimize imported rendering and correct cradle collision transfer<br/>DONE"]
  INT_007["INT-007: Continuous circle contacts, viewport navigation and responsive Open<br/>REVIEW"]
  INT_008["INT-008: General imported rigid fixtures and common 2D constraints<br/>REVIEW"]
  BUILD_001 --> REN_001
  BUILD_001 --> BUILD_002
  BUILD_002 --> TEST_001
  CORE_001 --> CORE_002
  BUILD_002 --> CORE_002
  TEST_001 --> CORE_002
  MATH_001 --> MATH_002
  BUILD_002 --> MATH_002
  TEST_001 --> MATH_002
  CORE_001 --> SCENE_001
  MATH_001 --> SCENE_001
  SCENE_001 --> PHY_001
  SCENE_001 --> REN_002
  REN_001 --> REN_002
  SCENE_001 --> SCENE_002
  CORE_002 --> SCENE_002
  MATH_002 --> SCENE_002
  PHY_001 --> PHY_002
  SCENE_002 --> PHY_002
  REN_002 --> REN_003
  SCENE_002 --> REN_003
  BUILD_003 --> REN_003
  PHY_002 --> INT_001
  REN_003 --> INT_001
  BUILD_003 --> INT_001
  COMPAT_001 --> COMPAT_002
  BUILD_002 --> BUILD_003
  REN_001 --> BUILD_003
  BUILD_003 --> BUILD_004
  REN_003 --> BUILD_004
  SCENE_002 --> EDT_001
  REN_003 --> EDT_001
  PHY_002 --> EDT_001
  EDT_001 --> INT_002
  BUILD_004 --> INT_002
  SCENE_002 --> IO_001
  INT_002 --> IO_001
  IO_001 --> IO_002
  SCENE_002 --> IO_002
  IO_001 --> PLAT_001
  INT_002 --> INT_003
  COMPAT_003 --> COMPAT_004
  INT_003 --> COMPAT_004
  COMPAT_004 --> INT_004
  INT_003 --> INT_004
  INT_004 --> INT_005
  COMPAT_004 --> SCENE_003
  SCENE_002 --> SCENE_003
  PHY_002 --> SCENE_003
  INT_005 --> INT_006
  INT_006 --> INT_007
  INT_006 --> INT_008
  COMPAT_004 --> INT_008
```
