"""Reproduce initial planning artifacts. Refuses to overwrite an existing registry."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

# id, title, stage, dependencies, allowed paths, objective, acceptance, checks, deliverables
ROWS = [
('BUILD-001','Select foundation toolchain and coding policy',0,[],['spec/build-policy.md','adr/BUILD-001-*.md'],
 'Choose the smallest supported C++ build and test baseline with a reproducible acquisition policy.',
 'Record C++ standard, compiler/platform matrix, CMake minimum, test framework, dependency pinning, formatting, warnings and CI strategy; justify choices and list exact clean-checkout commands; resolve exception/error and naming rules consistently with core requirements.',
 'Review every choice against headless testing and dependency-license constraints; verify selected tool versions are available using authoritative documentation and record sources; no simulator build required.',
 'Accepted toolchain ADR and build policy'),
('CORE-001','Specify IDs and diagnostics contracts',0,[],['spec/contracts/core.md'],
 'Freeze a small language-neutral contract for identifiers and structured errors before consumer code exists.',
 'Define ID width and invalid value, uniqueness scope, allocation ownership, diagnostic codes/severity/context, result semantics and threading/lifetime rules; include success and failure examples without a logging framework.',
 'Review invalid/duplicate ID examples and error propagation examples; map every invariant to a conformance test vector.',
 'Reviewed core contract and embedded test vectors'),
('MATH-001','Specify 2D numeric contracts',0,[],['spec/contracts/math.md'],
 'Define the initial vector and transform operations without choosing a third-party math library.',
 'Specify scalar representation, SI/radian conventions, coordinate handedness, finite-value handling, zero normalization, transform order, equality/tolerance policy and required minimal operations.',
 'Work through zero vector, axis rotation, composition order and invalid-number examples; record exact expected values or justified tolerances.',
 'Reviewed math contract and test vectors'),
('REN-001','Select minimal render strategy',0,['BUILD-001'],['spec/render-policy.md','adr/REN-001-*.md'],
 'Choose a narrow rendering route for the Stage 1 particle demo, with a headless contract-testing route.',
 'Compare a small set of options by platform support, license, setup, CI and surface ownership; specify snapshot-to-pixel mapping, minimal primitives and resize behavior; avoid choosing the full editor toolkit; explicitly reconcile platform assumptions before acceptance.',
 'Review a concrete render scenario and headless testing strategy; verify selected backend requirements from primary docs; identify platform-specific smoke tests.',
 'Accepted minimal-render ADR and render policy'),
('COMPAT-001','Prepare research intake templates',0,[],['research/templates/','research/README.md'],
 'Prepare empty evidence and requirement templates so future research can be consumed without inventing observations.',
 'Include source/version/platform/confidence, permission/hash, contradictory findings, requirement-task-test links and matrix state; include one clearly synthetic structural example only.',
 'Walk a synthetic finding through requirement, task and test references; verify UNKNOWN cannot be confused with PASS and provenance fields are present.',
 'Finding, fixture and requirement templates plus intake instructions'),
('BUILD-002','Create CMake and CI test scaffold',0,['BUILD-001'],['CMakeLists.txt','CMakePresets.json','cmake/','.github/workflows/','.clang-format','.gitignore'],
 'Create a minimal build/test scaffold according to the accepted toolchain policy.',
 'Provide module registration convention and category test hooks, developer presets, selected CI jobs and clean-checkout instructions in cmake/README.md; empty modules must not require unfinished source.',
 'Configure and build from a clean directory; run the registered scaffold check through CTest locally and on required CI targets; verify intentionally failed check is surfaced by the harness.',
 'Reproducible build scaffold and CI configuration'),
('TEST-001','Add test registration and fixture conventions',0,['BUILD-002'],['tests/CMakeLists.txt','tests/support/','tests/fixtures/README.md','spec/test-evidence.md'],
 'Establish reusable test registration and evidence conventions without production helpers.',
 'Register the test categories from spec/testing.md via existing build hooks; document fixture provenance and numeric assertions; demonstrate unit and contract category discovery with test-only sample data.',
 'Run test discovery and category filtering; demonstrate failure reports include expected/actual values and tolerance; validate one sample fixture metadata record.',
 'Test harness helpers, category registration and fixture evidence guide'),
('CORE-002','Implement stable ID and diagnostic values',1,['CORE-001','BUILD-002','TEST-001'],['src/core/','tests/unit/core/','tests/contract/core/'],
 'Implement only the accepted core value primitives.',
 'Match core contract; keep allocation scope explicit and expose no mutable global service; supply module build registration under existing conventions.',
 'Run all core contract vectors; test valid/invalid and duplicate IDs according to allocation policy, structured error propagation and value ownership.',
 'Core public headers, implementation if needed, unit and conformance tests'),
('MATH-002','Implement minimal vector and transform operations',1,['MATH-001','BUILD-002','TEST-001'],['src/math/','tests/unit/math/','tests/contract/math/'],
 'Implement only the vector/transform operations needed by the initial slice.',
 'Conform to numeric policy; no scene/renderer includes; document invalid-number and zero-vector behavior.',
 'Run accepted math vectors including identity, rotation, transform composition, zero handling and nonfinite input; verify tolerances are explicit.',
 'Minimal math module and numeric tests'),
('SCENE-001','Specify minimal document and snapshot contracts',0,['CORE-001','MATH-001'],['spec/contracts/scene.md'],
 'Define minimal particle authoring, runtime snapshots and presentation data independently of file formats.',
 'Specify IDs, mass, position/velocity, gravity, time, validation and edit atomicity; define immutable snapshot and simple draw packets; no solver caches or backend types; document ID ordering and reset semantics.',
 'Review valid/invalid scenes, duplicate IDs, nonfinite fields, invalid mass, snapshot isolation and coordinate examples; supply conformance vectors for each rule.',
 'Reviewed scene schema/command/snapshot contract and test vectors'),
('PHY-001','Specify fixed-step particle simulation port',0,['SCENE-001'],['spec/contracts/physics.md'],
 'Freeze a minimal runtime port for free particles under uniform gravity.',
 'Define construct/step/snapshot/reset signatures conceptually, numeric update equations, dt limits, failure atomicity, determinism scope and analytic error bounds; exclude collisions and choose no general solver architecture.',
 'Derive one-step expected position/velocity and a multi-step analytic bound; review zero gravity, invalid dt and reset examples against scene contract.',
 'Reviewed physics port, integrator decision and reference vectors'),
('REN-002','Specify render port and test doubles',0,['SCENE-001','REN-001'],['spec/contracts/renderer.md'],
 'Freeze a renderer boundary consuming snapshot and primitive data.',
 'Define draw/resize/lifecycle semantics and app-provided host surface, error reporting, camera coordinate mapping, and headless recorder expectations; no physics or platform-module dependency.',
 'Review known world-to-view points, empty frame, resize and backend failure examples; specify shared tests for recorder and real backend.',
 'Reviewed rendering port and conformance examples'),
('SCENE-002','Implement particle document validation and snapshots',1,['SCENE-001','CORE-002','MATH-002'],['src/scene/','tests/unit/scene/','tests/contract/scene/'],
 'Implement the canonical Stage 1 data types, validation and snapshot value semantics.',
 'Implement accepted minimal schema and command atomicity; no persistence or physics runtime; validation must report object-specific diagnostics.',
 'Run scene contract vectors: valid document, duplicate IDs, nonfinite fields, invalid mass, failed edit leaves original intact, copied snapshot remains unchanged.',
 'Canonical scene module and validation/contract tests'),
('PHY-002','Implement uniform-gravity particle stepper',1,['PHY-001','SCENE-002'],['src/physics/','tests/unit/physics/','tests/reference/physics/','tests/contract/physics/'],
 'Implement the accepted minimal particle runtime independent of wall clock and graphics.',
 'Support construct, fixed step, immutable snapshot and reset; apply specified update equations; reject invalid dt according to contract; no collision/constraint implementation.',
 'Run one-step vectors, zero gravity, analytic multi-step error bounds, reset equivalence and same-build determinism checks promised by the contract.',
 'Particle stepper and reference/conformance tests'),
('REN-003','Implement minimal particle renderer',1,['REN-002','SCENE-002'],['src/renderer/','tests/unit/renderer/','tests/contract/renderer/'],
 'Implement the selected minimal backend and headless recording double.',
 'Draw immutable snapshots and supported primitive packets; handle resize and lifecycle errors; build against the accepted contract without physics implementation.',
 'Run shared recorder/backend conformance cases as applicable, world-to-view mapping and empty-frame tests; perform selected-platform render smoke check with recorded result.',
 'Renderer module, recording double and backend verification'),
('INT-001','Integrate the first runnable simulation',1,['PHY-002','REN-003'],['src/app/','tests/integration/app/','docs/demo.md'],
 'Compose the completed scene, stepper and renderer into the smallest runnable simulation.',
 'Create one documented particle scene, explicit fixed-step loop and minimal host surface/input adapter private to app; start/advance/pause/reset controls may be keyboard or CLI; do not build a full editor or reusable platform framework.',
 'Headless lifecycle test asserts initial/advanced/paused/reset states and analytic trajectory bound; verify renderer receives snapshots; run documented visual smoke demo on selected platform.',
 'Runnable particle demo, integration tests and usage instructions'),
('COMPAT-002','Normalize supplied SimPHY capability research',0,['COMPAT-001'],['research/normalized/','research/requirements.json','research/intake-report.md'],
 'Turn the externally supplied map into traceable requirements and follow-up proposals.',
 'Requires research_available gate; preserve provenance/version/uncertainty, separate syntax/model/behavior observations, flag contradictions, and propose small implementation tasks only for supported findings.',
 'Check unique requirement IDs and resolvable finding/fixture references; review uncertain and unsupported examples; ensure no PASS entries lack implementation and test evidence.',
 'Normalized research matrix and scoped task proposals; no compatibility code'),
]

def main():
    registry = ROOT / 'tasks/registry.json'
    if registry.exists():
        raise SystemExit('Registry exists; do not reseed active work.')
    modules = 'core math scene collision constraints physics renderer editor visualization analysis serialization compat platform app'.split()
    categories = 'unit contract integration regression reference serialization compatibility malformed fixtures'.split()
    for relative in [*(f'src/{m}' for m in modules), *(f'tests/{c}' for c in categories), 'research/incoming', 'tasks/reports', 'spec/contracts']:
        path = ROOT / relative
        path.mkdir(parents=True, exist_ok=True)
        (path / '.gitkeep').touch()
    entries = []
    for ident, title, stage, deps, paths, objective, acceptance, checks, deliverables in ROWS:
        blocks = [r[0] for r in ROWS if ident in r[3]]
        gates = ['research_available'] if ident == 'COMPAT-002' else []
        state = 'BLOCKED' if deps or gates else 'READY'
        refs = ['spec/architecture.md','spec/interfaces.md','spec/testing.md']
        if ident.startswith('COMPAT'): refs.append('spec/research-intake.md')
        if ident.startswith('BUILD') or ident == 'TEST-001': refs.append('CONTRIBUTING.md')
        for dependency in deps:
            refs.append(f'tasks/items/{dependency}.md')
        entry = dict(id=ident,title=title,stage=stage,status=state,priority=1 if stage == 0 else 2,
                     owner=None,dependencies=deps,blocks=blocks,gates=gates,allowed_paths=paths,
                     spec_refs=refs,description=f'tasks/items/{ident}.md',block_reason=None,
                     completion_report=None,reviewer=None,merged_commit=None)
        entries.append(entry)
        body = f'''# {ident} — {title}

- Stage: {stage}
- Status, priority, ownership: authoritative in [registry](../registry.json), initially {state}, P{entry['priority']}, unowned.
- Dependencies: {', '.join(deps) or 'None'}
- Blocks: {', '.join(blocks) or 'No initial tasks'}
- External gates: {', '.join(gates) or 'None'}

## Objective
{objective}

## Allowed files/modules
{chr(10).join('- `' + p + '`' for p in paths)}

Registry/report updates go through the coordinator. A new dependency or required edit
outside these paths needs a scoped follow-up or reviewed scope amendment first.

## Relevant specifications/interfaces
{chr(10).join('- [' + p + '](../../' + p + ')' for p in refs)}

Read the merged outputs of dependency tasks as well as these baseline references.

## Requirements and acceptance criteria
{acceptance}

Every clause above requires evidence; the reviewer checks each in the completion report.

## Required tests/checks
{checks}

Run existing affected checks and `python tools/tasks.py validate` as applicable.
For documentation tasks, record reviewed examples and checklist outcomes.

## Deliverables
{deliverables}.

## Notes/assumptions
Only the small Stage 1 scope is authorized. No speculative SimPHY behavior.
Contract tasks produce reviewed specifications; downstream implementation owns C++
headers and must conform to those specifications. Missing decisions are blockers,
not permission to assume another agent's unpublished work.

## Completion report
Initially incomplete. Coordinator records [report](../reports/{ident}.md) before REVIEW,
using [template](../templates/completion.md). No report exists until work is submitted.
'''
        target=ROOT/entry['description']; target.parent.mkdir(parents=True,exist_ok=True); target.write_text(body,encoding='utf-8')
    registry.write_text(json.dumps(dict(schema_version=1,coordinator=None,gates={'research_available':{'satisfied':False,'evidence':None}},tasks=entries),indent=2)+'\n',encoding='utf-8')
    graph=['# Initial backlog and dependency graph','', 'READY now: BUILD-001, CORE-001, MATH-001, COMPAT-001.', '',
      'These four tasks have disjoint writable paths and can run concurrently after claims.',
      'They create policy/contracts/templates, not simulator code. REN-001 waits for',
      'the platform policy from BUILD-001 before selecting a compatible backend.', '',
      'Build implementation waits for BUILD-001; renderer work waits for REN-001;',
      'all component work waits for its contract/build prerequisites. COMPAT-002 waits',
      'for supplied research, regardless of foundation progress. License/release and',
      'distant-stage questions are listed in spec/open-questions.md.', '', '```mermaid','flowchart TD']
    for e in entries:
        graph.append(f'  {e["id"].replace("-","_")}["{e["id"]}: {e["title"]}"]')
        for dep in e['dependencies']: graph.append(f'  {dep.replace("-","_")} --> {e["id"].replace("-","_")}')
    graph += ['  research["Supplied research + evidence gate"] --> COMPAT_002','```','','Later parallel waves: CORE-002 with MATH-002; PHY-001 with REN-002; PHY-002 with REN-003.', 'Actual READY status always comes from the registry, not this initial snapshot.','']
    (ROOT/'tasks/graph.md').write_text('\n'.join(graph),encoding='utf-8')

if __name__ == '__main__': main()
