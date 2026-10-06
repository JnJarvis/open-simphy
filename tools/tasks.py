"""Small, single-checkout task coordinator. Python standard library only."""
import argparse
from contextlib import contextmanager
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
STATES = {'BACKLOG', 'BLOCKED', 'READY', 'IN_PROGRESS', 'REVIEW', 'DONE'}

def eligible(task, by_id, gates):
    return (not task['block_reason']
            and all(by_id[d]['status'] == 'DONE' for d in task['dependencies'])
            and all(gates[g]['satisfied'] for g in task['gates']))

def refresh(data):
    by_id = {t['id']: t for t in data['tasks']}
    for task in data['tasks']:
        if task['status'] in {'READY', 'BLOCKED'}:
            task['status'] = 'READY' if eligible(task, by_id, data['gates']) else 'BLOCKED'

def validate(data, root):
    errors = []
    tasks = data['tasks']; by_id = {t['id']: t for t in tasks}
    if data.get('schema_version') != 1: errors.append('Unsupported registry schema')
    if len(tasks) != len(by_id): errors.append('Duplicate task IDs')
    for task in tasks:
        ident = task['id']
        if not re.fullmatch(r'[A-Z]+-\d{3}', ident): errors.append(f'{ident}: invalid ID')
        if task['status'] not in STATES: errors.append(f'{ident}: invalid status')
        if task['priority'] not in range(4): errors.append(f'{ident}: invalid priority')
        if not task['allowed_paths']: errors.append(f'{ident}: missing scope')
        for path in [task['description'], *task['spec_refs']]:
            if not (root / path).is_file(): errors.append(f'{ident}: missing {path}')
        missing = set(task['dependencies']) - by_id.keys()
        if missing: errors.append(f'{ident}: unknown dependencies {sorted(missing)}')
        if set(task['gates']) - data['gates'].keys(): errors.append(f'{ident}: unknown gate')
        expected = {t['id'] for t in tasks if ident in t['dependencies']}
        if set(task['blocks']) != expected: errors.append(f'{ident}: blocks is inconsistent')
        if task['status'] in {'IN_PROGRESS','REVIEW','DONE'} and not task['owner']:
            errors.append(f'{ident}: missing owner')
        if task['status'] in {'REVIEW','DONE'}:
            if not task['completion_report'] or not (root/task['completion_report']).is_file():
                errors.append(f'{ident}: missing completion report')
        if task['status'] == 'DONE' and (not task['reviewer'] or task['reviewer'] == task['owner'] or not task['merged_commit']):
            errors.append(f'{ident}: missing independent review/merge evidence')
        if not missing and not (set(task['gates']) - data['gates'].keys()):
            if task['status'] in {'READY','IN_PROGRESS','REVIEW','DONE'} and not eligible(task,by_id,data['gates']):
                errors.append(f'{ident}: unmet prerequisites for {task["status"]}')
    visiting, visited = set(), set()
    def walk(ident):
        if ident in visiting:
            errors.append(f'Dependency cycle at {ident}'); return
        if ident in visited or ident not in by_id: return
        visiting.add(ident)
        for dep in by_id[ident]['dependencies']: walk(dep)
        visiting.remove(ident); visited.add(ident)
    for ident in by_id: walk(ident)
    return errors

@contextmanager
def locked(root):
    lock = root/'tasks/.registry-lock'
    try: lock.mkdir()
    except FileExistsError: raise ValueError('Registry locked; retry later. Never remove a live lock.')
    try:
        (lock/'holder.json').write_text(json.dumps({'pid':os.getpid(),'utc':datetime.now(timezone.utc).isoformat()}),encoding='utf-8')
        yield
    finally:
        (lock/'holder.json').unlink(missing_ok=True)
        lock.rmdir()

def overlaps(a, b):
    # Conservative prefix comparison also reserves wildcard namespaces.
    a = a.split('*')[0].rstrip('/').lower()
    b = b.split('*')[0].rstrip('/').lower()
    return a == b or a.startswith(b+'/') or b.startswith(a+'/')

def transition(data, command, values, root):
    by_id = {t['id']:t for t in data['tasks']}
    if command == 'coordinator':
        if data['coordinator'] and data['coordinator'] != values[0]:
            raise ValueError('Coordinator already appointed; use reviewed administrative handover.')
        data['coordinator'] = values[0]; return
    if not data['coordinator']: raise ValueError('Appoint the canonical coordinator first.')
    if command == 'refresh': refresh(data); return
    if command == 'gate':
        name, evidence = values
        if name not in data['gates']: raise ValueError('Unknown gate')
        if not (root/evidence).is_file(): raise ValueError('Evidence file does not exist')
        data['gates'][name] = {'satisfied':True,'evidence':evidence}
        refresh(data); return
    if values[0] not in by_id: raise ValueError('Unknown task')
    task = by_id[values[0]]
    status = task['status']
    if command == 'claim':
        if status != 'READY' or task['owner']: raise ValueError('Task is not unclaimed READY work')
        for other in data['tasks']:
            if other['id'] != task['id'] and other['status'] in {'IN_PROGRESS','REVIEW'}:
                if any(overlaps(a,b) for a in task['allowed_paths'] for b in other['allowed_paths']):
                    raise ValueError(f'Scope overlaps active task {other["id"]}')
        task['owner'] = values[1]; task['status'] = 'IN_PROGRESS'
    elif command == 'review':
        if status != 'IN_PROGRESS' or task['owner'] != values[1]: raise ValueError('Only current owner can submit work')
        report = f'tasks/reports/{task["id"]}.md'
        if not (root/report).is_file(): raise ValueError('Completion report is required')
        task['completion_report'] = report; task['status'] = 'REVIEW'
    elif command == 'done':
        if status != 'REVIEW': raise ValueError('Task must be in REVIEW')
        if values[1] == task['owner']: raise ValueError('Independent reviewer required')
        if not re.fullmatch(r'[0-9a-fA-F]{7,40}',values[2]): raise ValueError('Provide the verified merged commit hash')
        task['reviewer'] = values[1]; task['merged_commit'] = values[2]; task['status'] = 'DONE'
        refresh(data)
    elif command == 'resume':
        if status != 'REVIEW': raise ValueError('Only REVIEW may resume for corrections')
        task['status'] = 'IN_PROGRESS'
    elif command == 'block':
        if status == 'DONE': raise ValueError('DONE requires explicit administrative reopening')
        task['block_reason'] = values[1]; task['status'] = 'BLOCKED'
    elif command == 'unblock':
        if status != 'BLOCKED': raise ValueError('Task must be BLOCKED')
        task['block_reason'] = None; refresh(data)
        if task['status'] == 'READY' and task['owner']: task['status'] = 'IN_PROGRESS'
    else: raise ValueError('Unknown command')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command',choices=['ready','validate','coordinator','claim','review','done','block','unblock','resume','refresh','gate'])
    parser.add_argument('values',nargs='*')
    args=parser.parse_args()
    counts={'ready':0,'validate':0,'coordinator':1,'claim':2,'review':2,'done':3,'block':2,'unblock':1,'resume':1,'refresh':0,'gate':2}
    if len(args.values)!=counts[args.command] or any(not v.strip() for v in args.values): parser.error('Incorrect or empty arguments; see tasks/README.md')
    registry=ROOT/'tasks/registry.json'
    def read(): return json.loads(registry.read_text(encoding='utf-8'))
    def check(data):
        errors=validate(data,ROOT)
        if errors: raise ValueError('\n'.join(errors))
    try:
        if args.command in {'ready','validate'}:
            data=read(); check(data)
            if args.command == 'validate': print(f'Valid: {len(data["tasks"])} tasks')
            else:
                for t in sorted(data['tasks'],key=lambda t:(t['priority'],t['id'])):
                    if t['status']=='READY' and not t['owner']: print(f'{t["id"]} P{t["priority"]} stage={t["stage"]}: {t["title"]}')
            return
        with locked(ROOT):
            data=read(); check(data)
            if args.command == 'done':
                commit=args.values[2]
                if not re.fullmatch(r'[0-9a-fA-F]{7,40}',commit):
                    raise ValueError('Provide a merged commit hash')
                result=subprocess.run(['git','merge-base','--is-ancestor',commit,'HEAD'],cwd=ROOT,capture_output=True)
                if result.returncode != 0:
                    raise ValueError('Commit is not an ancestor of coordinator HEAD; merge and verify first.')
            transition(data,args.command,args.values,ROOT); check(data)
            temporary=registry.with_suffix('.json.tmp')
            temporary.write_text(json.dumps(data,indent=2)+'\n',encoding='utf-8')
            os.replace(temporary,registry)
        print('Registry updated; review and commit the coordination change.')
    except (ValueError,KeyError,OSError) as error:
        parser.exit(1,f'{error}\n')

if __name__ == '__main__': main()
