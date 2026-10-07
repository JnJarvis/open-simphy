# Task dependency chart

Updated 2026-10-07 from [registry.json](registry.json). Arrows run from prerequisite
to dependent task. Status labels are a snapshot; the registry remains authoritative.

DONE: 18 | REVIEW: 1 | IN_PROGRESS: 0 | READY: 1 | BLOCKED: 2

- The particle editor demo (INT-002) is merged; this is not a finished product.
- IO-001 is in REVIEW: native-file contract and transactional persistence proposal.
- COMPAT-001 remains READY and unclaimed for the later small-task worker.
- INT-001 code is merged; only mixed-scale multi-monitor hardware evidence blocks completion.
- COMPAT-002 still requires COMPAT-001 and a reviewed research evidence gate.

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
  IO_001["IO-001: Specify bounded native particle persistence<br/>REVIEW"]
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
  gate_research_available{"research_available: not satisfied"}
  gate_research_available -.-> COMPAT_002
  classDef DONE fill:#dcfce7,stroke:#475569,color:#111827
  class BUILD_001,CORE_001,MATH_001,REN_001,BUILD_002,TEST_001,CORE_002,MATH_002,SCENE_001,PHY_001,REN_002,SCENE_002,PHY_002,REN_003,BUILD_003,BUILD_004,EDT_001,INT_002 DONE
  classDef REVIEW fill:#dbeafe,stroke:#475569,color:#111827
  class IO_001 REVIEW
  classDef READY fill:#fef9c3,stroke:#475569,color:#111827
  class COMPAT_001 READY
  classDef BLOCKED fill:#fee2e2,stroke:#475569,color:#111827
  class INT_001,COMPAT_002 BLOCKED
  classDef IN_PROGRESS fill:#ede9fe,stroke:#475569,color:#111827
```

## Persistence follow-ups after contract approval

These are sequencing proposals, not claimed or READY implementation tasks.
The coordinator assigns IDs and final scopes after IO-001 acceptance.

```mermaid
flowchart LR
  contract["IO-001: approve native-file contract"] --> codec["Implement headless codec and tests"]
  contract --> file_contract["Specify portable file adapter"]
  file_contract --> files["Implement and test native file adapters"]
  codec --> integration["Integrate Save/Open and native workflow tests"]
  files --> integration
```

No future step is unblocked by a proposed contract alone. Platform adapters and
app integration remain separate from the pure codec; existing architecture edges
are unchanged. See [architecture](../spec/architecture.md) for the module DAG.
