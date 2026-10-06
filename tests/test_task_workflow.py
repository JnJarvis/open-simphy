"""Test coordination failures and dependency unlocking without changing real tasks."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
SPEC=importlib.util.spec_from_file_location('tasks_tool',ROOT/'tools/tasks.py')
TASKS=importlib.util.module_from_spec(SPEC); SPEC.loader.exec_module(TASKS)

class WorkflowTests(unittest.TestCase):
    def setUp(self):
        self.data=json.loads((ROOT/'tasks/registry.json').read_text(encoding='utf-8'))
        # Exercise workflow independently of the repository's current task progress.
        for task in self.data['tasks']:
            task.update(status='BLOCKED',owner=None,block_reason=None,
                        completion_report=None,reviewer=None,merged_commit=None)
        for gate in self.data['gates'].values():
            gate.update(satisfied=False,evidence=None)
        TASKS.refresh(self.data)
        self.data['coordinator']='coordinator'
        self.temp=tempfile.TemporaryDirectory(); self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name); (self.root/'tasks/reports').mkdir(parents=True)

    def test_baseline_and_cycle_detection(self):
        self.assertEqual(TASKS.validate(self.data,ROOT),[])
        self.data['tasks'][0]['dependencies']=['BUILD-002']
        self.assertTrue(any('cycle' in e for e in TASKS.validate(self.data,ROOT)))

    def test_duplicate_claim_and_blocked_claim_fail(self):
        TASKS.transition(self.data,'claim',['CORE-001','alice'],self.root)
        with self.assertRaises(ValueError): TASKS.transition(self.data,'claim',['CORE-001','bob'],self.root)
        with self.assertRaises(ValueError): TASKS.transition(self.data,'claim',['CORE-002','bob'],self.root)

    def test_review_merge_unlocks_only_satisfied_dependencies(self):
        TASKS.transition(self.data,'claim',['BUILD-001','alice'],self.root)
        with self.assertRaises(ValueError): TASKS.transition(self.data,'review',['BUILD-001','alice'],self.root)
        (self.root/'tasks/reports/BUILD-001.md').write_text('Test evidence',encoding='utf-8')
        TASKS.transition(self.data,'review',['BUILD-001','alice'],self.root)
        with self.assertRaises(ValueError): TASKS.transition(self.data,'done',['BUILD-001','alice','1234567'],self.root)
        TASKS.transition(self.data,'done',['BUILD-001','bob','1234567'],self.root)
        statuses={t['id']:t['status'] for t in self.data['tasks']}
        self.assertEqual(statuses['BUILD-002'],'READY')
        self.assertEqual(statuses['CORE-002'],'BLOCKED')
        self.assertEqual(statuses['COMPAT-002'],'BLOCKED')

    def test_lock_excludes_second_writer_and_releases_after_error(self):
        with self.assertRaises(RuntimeError):
            with TASKS.locked(self.root):
                with self.assertRaises(ValueError):
                    with TASKS.locked(self.root): pass
                raise RuntimeError('simulated failure')
        with TASKS.locked(self.root): pass

    def test_overlapping_scope_rejected(self):
        TASKS.transition(self.data,'claim',['CORE-001','alice'],self.root)
        math=next(t for t in self.data['tasks'] if t['id']=='MATH-001')
        math['allowed_paths']=['spec/contracts/']
        with self.assertRaises(ValueError): TASKS.transition(self.data,'claim',['MATH-001','bob'],self.root)

    def test_explicit_blocker_survives_refresh_and_resumes_owner(self):
        TASKS.transition(self.data,'claim',['CORE-001','alice'],self.root)
        TASKS.transition(self.data,'block',['CORE-001','Await clarification'],self.root)
        TASKS.refresh(self.data)
        task=next(t for t in self.data['tasks'] if t['id']=='CORE-001')
        self.assertEqual(task['status'],'BLOCKED')
        TASKS.transition(self.data,'unblock',['CORE-001'],self.root)
        self.assertEqual(task['status'],'IN_PROGRESS')

if __name__ == '__main__': unittest.main()
