# Future SimPHY research intake

Do not reconstruct SimPHY now. Keep supplied findings in research/incoming/ with
source, observed version/platform, method, confidence, fixture provenance and
unresolved contradictions. Separate observation from interpretation.

Pipeline: finding → versioned requirement → scoped tasks → tests → reviewed PASS.

1. COMPAT-001 establishes metadata templates without inventing any findings.
2. COMPAT-002 starts only after a supplied map passes the external evidence gate.
   Normalize findings to REQ-COMPAT-###, retaining source links and uncertainty.
3. For each requirement record capability/version, source finding IDs, expected
   behavior, fixture IDs, tolerances, affected modules, task IDs and test IDs.
4. Resolve ambiguities with the research provider. UNKNOWN is not unsupported or PASS.
5. Plan contract work first, then separate parser/translator/model/physics/UI/IO
   tasks and a final integration task. Create only evidence-backed work.
6. Tests validate syntax separately from canonical translation and behavior.
   Review mismatch reports rather than loosening expectations to match code.
7. Mark PASS only with passing test evidence on the declared version/scope and a
   merged implementation. Reopen when evidence or behavior changes.

Matrix states: UNKNOWN (insufficient evidence), SPECIFIED, IMPLEMENTING, FAIL,
PASS, and OUT_OF_SCOPE (reason required). These are requirement states, distinct
from task statuses. Publish coverage with exclusions and tested versions.

Architecture: bounded external bytes → format-specific parser and neutral external
tree → translator → canonical scene + structured diagnostic/loss report → app.
Unknown required data fails safely; optional unsupported features are explicitly
reported. No eval, script execution, asset network fetches, or arbitrary filesystem
access from imported projects. Preserve opaque data only under a documented bounded
policy. Native serialization has a separate versioned schema and independent tests.

Export needs its own reverse mapping and fidelity policy. An importer does not
establish export or round-trip compatibility.
