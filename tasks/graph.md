# Initial backlog and dependency graph

READY now: BUILD-001, CORE-001, MATH-001, COMPAT-001.

These four tasks have disjoint writable paths and can run concurrently after claims.
They create policy/contracts/templates, not simulator code. REN-001 waits for
the platform policy from BUILD-001 before selecting a compatible backend.

Build implementation waits for BUILD-001; renderer work waits for REN-001;
all component work waits for its contract/build prerequisites. COMPAT-002 waits
for supplied research, regardless of foundation progress. License/release and
distant-stage questions are listed in spec/open-questions.md.

```mermaid
flowchart TD
  BUILD_001 --> REN_001
  BUILD_001["BUILD-001: Select foundation toolchain and coding policy"]
  CORE_001["CORE-001: Specify IDs and diagnostics contracts"]
  MATH_001["MATH-001: Specify 2D numeric contracts"]
  REN_001["REN-001: Select minimal render strategy"]
  COMPAT_001["COMPAT-001: Prepare research intake templates"]
  BUILD_002["BUILD-002: Create CMake and CI test scaffold"]
  BUILD_001 --> BUILD_002
  TEST_001["TEST-001: Add test registration and fixture conventions"]
  BUILD_002 --> TEST_001
  CORE_002["CORE-002: Implement stable ID and diagnostic values"]
  CORE_001 --> CORE_002
  BUILD_002 --> CORE_002
  TEST_001 --> CORE_002
  MATH_002["MATH-002: Implement minimal vector and transform operations"]
  MATH_001 --> MATH_002
  BUILD_002 --> MATH_002
  TEST_001 --> MATH_002
  SCENE_001["SCENE-001: Specify minimal document and snapshot contracts"]
  CORE_001 --> SCENE_001
  MATH_001 --> SCENE_001
  PHY_001["PHY-001: Specify fixed-step particle simulation port"]
  SCENE_001 --> PHY_001
  REN_002["REN-002: Specify render port and test doubles"]
  SCENE_001 --> REN_002
  REN_001 --> REN_002
  SCENE_002["SCENE-002: Implement particle document validation and snapshots"]
  SCENE_001 --> SCENE_002
  CORE_002 --> SCENE_002
  MATH_002 --> SCENE_002
  PHY_002["PHY-002: Implement uniform-gravity particle stepper"]
  PHY_001 --> PHY_002
  SCENE_002 --> PHY_002
  REN_003["REN-003: Implement minimal particle renderer"]
  REN_002 --> REN_003
  SCENE_002 --> REN_003
  INT_001["INT-001: Integrate the first runnable simulation"]
  PHY_002 --> INT_001
  REN_003 --> INT_001
  COMPAT_002["COMPAT-002: Normalize supplied SimPHY capability research"]
  COMPAT_001 --> COMPAT_002
  research["Supplied research + evidence gate"] --> COMPAT_002
```

Later parallel waves: CORE-002 with MATH-002; PHY-001 with REN-002; PHY-002 with REN-003.
Actual READY status always comes from the registry, not this initial snapshot.
