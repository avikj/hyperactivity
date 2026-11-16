#!/usr/bin/env python3
"""history — the history of the work, read from git as it is asked for (2026-11-15, Avik: "hyperactivity
history should enter history explorer in the product ... inserting zero llm judgement at run-time";
"near zero machinery"). Git is the stand-in the store replaces (docs/no-filesystem.md); this reads it
and keeps nothing. Every sentence shown was written when the work was done; it reports only what the
work itself declares: its nodes (declarations of type Node, and Ticket before it), the leaves listed
in-progress and resolved, and the documents its code points to (`document "path"`). Every ref is read;
a change is compared with its first parent, so a merge shows what it brought to the line it joined, and
a change on a branch is marked with the branches it is on.

    hyperactive history                 the history a day at a time (UTC), with every change to the work's nodes and documents
    hyperactive history DAY             every change of a day (2026-11-13)
    hyperactive history change ID       one change: its message, its files, what it did to the nodes and documents
    hyperactive history of NAME         every change to a file, or to a declaration's text (code or statement) in every file that declared it
    hyperactive history node NODE       a node's every change and its states (in progress, resolved)
    hyperactive history words           Avik's words as they entered the work
"""
import datetime, importlib.util, os, re, subprocess, sys, threading
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
_spec = importlib.util.spec_from_file_location('tg', os.path.join(ROOT, 'tools', 'task-graph.py'))
tg = importlib.util.module_from_spec(_spec); _spec.loader.exec_module(tg)
NODE = re.compile(r'^[\w-]+ : (Node|Ticket)\b', re.M)
SENTENCE = re.compile(r'^[\w-]+ = node "([^"]*)"', re.M)
POINTER = re.compile(r'document "([^"]+)"')


def git(*a):
    return subprocess.run(['git', '-C', ROOT, *a], capture_output=True, text=True, errors='replace').stdout


def utc(t, f='%Y-%m-%d %H:%M'):
    return datetime.datetime.fromtimestamp(int(t), datetime.timezone.utc).strftime(f)


def changes():
    """every change of every ref, oldest first"""
    out = []
    for l in git('log', '--all', '--date-order', '--reverse', '--format=%H%x00%P%x00%an%x00%ct%x00%s').split('\n'):
        if l:
            h, p, a, t, s = l.split('\0', 4)
            out.append(dict(id=h, parents=p.split(), by='merge' if ' ' in p else 'Avik' if a in ('Avik Jain', 'avikj') else a, time=t, subject=s))
    return out


_cat, _lock = [], threading.Lock()


def lines_of_work():
    """change id -> the branches it is on, for every change not on main (every pull request's head is
    on a branch, so branches cover them)"""
    refs = [r for r in git('for-each-ref', '--format=%(refname:short)', 'refs/heads', 'refs/remotes').split()
            if not r.endswith('/HEAD') and '/pull/' not in r and r != 'origin']
    on = {r: set(git('rev-list', r).split()) for r in refs}
    main = on.get('main', set()) | on.get('origin/main', set())
    out = defaultdict(list)
    for r in refs:
        for c in on[r] - main: out[c].append(r.replace('origin/', ''))
    return {c: sorted(set(rs)) for c, rs in out.items()}


def blob(rev, path):
    """a file's text at a change, through one `git cat-file --batch`"""
    if not rev: return ''
    with _lock: return _blob(rev, path)


def _blob(rev, path):
    if not _cat: _cat.append(subprocess.Popen(['git', '-C', ROOT, 'cat-file', '--batch'], stdin=subprocess.PIPE, stdout=subprocess.PIPE))
    p = _cat[0]; p.stdin.write(f'{rev}:{path}\n'.encode()); p.stdin.flush()
    head = p.stdout.readline().split()
    if head[-1] == b'missing': return ''
    data = p.stdout.read(int(head[2])); p.stdout.read(1)
    return data.decode('utf-8', 'replace')


def events(c):
    """what a change did to the work's own declarations, against its first parent: nodes added,
    removed, renamed (a new sentence) or restated; leaves set in progress or resolved; document
    pointers added or removed"""
    p = c['parents'][0] if c['parents'] else None
    files = git('diff-tree', '-r', '--no-commit-id', '--name-only', '--root', *([p] if p else []), c['id'], '--', '*.cub').split()
    before, after, ev = {}, {}, []
    for f in files:
        old, new = blob(p, f), blob(c['id'], f)
        if not (NODE.search(old) or NODE.search(new) or POINTER.search(old) or POINTER.search(new)): continue
        for side, text in ((before, old), (after, new)):
            for name, _, t in tg.declarations(text):
                if NODE.search(t): side[name] = t
        ev += [f'document pointer added in {f}: {d}' for d in sorted(set(POINTER.findall(new)) - set(POINTER.findall(old)))]
        ev += [f'document pointer removed from {f}: {d}' for d in sorted(set(POINTER.findall(old)) - set(POINTER.findall(new)))]
        for key in ('in-progress', 'resolved'):
            m0, m1 = (re.search(r'^' + key + r' = (.*)$', t, re.M) for t in (old, new))
            l0, l1 = (tg.names_in(m.group(1)) if m else [] for m in (m0, m1))
            ev += [f'{key.replace("-", " ")}: {n}' for n in l1 if n not in l0]
            if key == 'in-progress':
                ev += [f'no longer in progress: {n}' for n in l0 if n not in l1]
    say = lambda n, t: n + (f' — "{m.group(1)}"' if (m := SENTENCE.search(t)) else '')
    for n, t in after.items():
        if n not in before: ev.append('node added: ' + say(n, t))
        elif t != before[n]:
            s0, s1 = SENTENCE.search(before[n]), SENTENCE.search(t)
            ev.append(('node renamed: ' if s0 and s1 and s0.group(1) != s1.group(1) else 'node restated: ') + say(n, t))
    ev += ['node removed: ' + n for n in before if n not in after]
    return ev


def line(c, work=None):
    on = (work or {}).get(c['id'])
    return f'{utc(c["time"])} {c["id"][:7]} {c["by"]}: {c["subject"]}' + (f'  [{", ".join(on)}]' if on else '')


def shown(ev):
    """a change's events, its restated nodes on one line"""
    r = [e[len('node restated: '):].split(' — ')[0] for e in ev if e.startswith('node restated')]
    return [e for e in ev if not e.startswith('node restated')] + (['nodes restated: ' + ', '.join(r)] if r else [])


def view_days(cs):
    days, out, work = defaultdict(list), [], lines_of_work()
    for c in cs: days[utc(c['time'], '%Y-%m-%d')].append(c)
    for d, dcs in days.items():
        out.append(f'{d}  {len(dcs)} changes')
        for c in dcs:
            ev = shown(events(c))
            if ev: out += ['  ' + line(c, work)] + ['        ' + e for e in ev]
        out.append('')
    return '\n'.join(out)


def view_day(cs, day):
    out, work = [], lines_of_work()
    for c in cs:
        if utc(c['time'], '%Y-%m-%d') == day: out += [line(c, work)] + ['      ' + e for e in shown(events(c))]
    return '\n'.join(out) or f'no changes on {day}'


def view_change(x):
    c = next((c for c in changes() if c['id'].startswith(x)), None)
    if not c: return f'no change {x}'
    on = lines_of_work().get(c['id'])
    return '\n'.join([git('show', '-s', '--format=%H%nby %an, %ad%nparents %P', '--date=iso', c['id']).rstrip(),
                      'on ' + (', '.join(on) if on else 'main'), '', git('show', '-s', '--format=%B', c['id']).rstrip(),
                      git('diff', '--stat=120', c['parents'][0], c['id']).rstrip() if c['parents'] else git('show', '--stat=120', '--format=', c['id']).rstrip(),
                      ''] + events(c))


def declaration(text, name):
    """a declaration's text (its statement and code) in a file's text, every occurrence joined; None
    when the file declares no such name"""
    ts = [t for n, _, t in tg.declarations(text) if n == name]
    return '\n\n'.join(ts) if ts else None


def code_of(t):
    return '\n'.join(l for l in t.split('\n') if not l.startswith('--'))


def changes_of(paths):
    """every change of every ref touching any of these paths, oldest first"""
    if not paths: return []
    ids = set(git('log', '--all', '--format=%H', '--', *paths).split())
    return [c for c in changes() if c['id'] in ids]


def view_of(x):
    """a file: every change to it. A declaration: every change to its text, in every file that ever
    declared it: added, its code changed, only its statement changed, or removed"""
    work = lines_of_work()
    if '/' in x or '.' in x:
        return '\n'.join([x] + ['  ' + line(c, work) for c in changes_of([x])])
    files = sorted(set(git('log', '--all', '--format=', '--name-only', '-G', rf'^((data|codata|record) )?{re.escape(x)}( |$)', '--', '*.cub').split()))
    out = [x]
    for c in changes_of(files):
        p = c['parents'][0] if c['parents'] else None
        what = []
        for f in files:
            a, b = declaration(blob(p, f), x), declaration(blob(c['id'], f), x)
            if a == b: continue
            what.append(f'{f}: ' + ('added' if a is None else 'removed' if b is None else 'code changed' if code_of(a) != code_of(b) else 'statement changed'))
        if what: out += ['  ' + line(c, work)] + ['      ' + w for w in what]
    return '\n'.join(out + [f'(what uses it now: hyperactive def {x})'])


def view_node(cs, x):
    """a node's every change, then the changes that set it in progress, took it out, or resolved it"""
    out, work = [view_of(x), '  states:'], lines_of_work()
    lists = set(git('log', '--all', '--format=', '--name-only', '-G', r'^(in-progress|resolved) = ', '--', '*.cub').split())
    for c in changes_of(sorted(lists)):
        ev = [e for e in events(c) if e in (f'in progress: {x}', f'resolved: {x}', f'no longer in progress: {x}')]
        if ev: out += ['    ' + line(c, work)] + ['        ' + e for e in ev]
    return '\n'.join(out)


def view_words():
    """each `#### date` heading of a document, from the change that first added it"""
    seen, out, c = set(), [], ''
    for l in git('log', '--all', '--date-order', '--reverse', '-p', '-U0', '--format=\x01%ct %h', '-G', r'^#### [0-9]{4}-', '--', '*.md').split('\n'):
        if l.startswith('\x01'): c = utc(l[1:].split()[0]) + ' ' + l.split()[1]
        elif l.startswith('+#### 20') and l[6:] not in seen:
            seen.add(l[6:]); out.append(f'{l[6:]}   recorded {c}')
    return '\n'.join(out)


def api(q):
    if 'day' in q: return {'text': view_day(changes(), q['day'])}
    if 'change' in q: return {'text': view_change(q['change'])}
    if 'of' in q: return {'text': view_of(q['of'])}
    if 'node' in q: return {'text': view_node(changes(), q['node'])}
    if 'words' in q: return {'text': view_words()}
    return {'text': view_days(changes())}


def main():
    a = sys.argv[1:]
    if not a: print(view_days(changes()))
    elif re.match(r'\d{4}-\d{2}-\d{2}$', a[0]): print(view_day(changes(), a[0]))
    elif a[0] == 'change' and len(a) == 2: print(view_change(a[1]))
    elif a[0] == 'of' and len(a) == 2: print(view_of(a[1]))
    elif a[0] == 'node' and len(a) == 2: print(view_node(changes(), a[1]))
    elif a == ['words']: print(view_words())
    else: print(__doc__)


if __name__ == '__main__':
    import signal; signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    main()
