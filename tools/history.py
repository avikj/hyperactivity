#!/usr/bin/env python3
"""history — the history of the work, read from git as it is asked for (2026-11-15, Avik: "hyperactivity
history should enter history explorer in the product ... inserting zero llm judgement at run-time";
"near zero machinery"). Git is the stand-in the store replaces (docs/no-filesystem.md); this reads it
and keeps nothing. Every sentence shown was written when the work was done; it reports only what the
work itself declares: its nodes (declarations of type Node, and Ticket before it), the leaves listed
in-progress and resolved, and the documents its code points to (`document "path"`).

    hyperactive history                 the history a day at a time (UTC), with every change to the work's nodes and documents
    hyperactive history DAY             every change of a day (2026-11-13)
    hyperactive history change ID       one change: its message, its files, what it did to the nodes and documents
    hyperactive history of NAME         every version of a file or a declaration, with the change that made it
    hyperactive history node NODE       a node's versions and states
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
            if m1:
                ev += [f'{key.replace("-", " ")}: {n}' for n in tg.names_in(m1.group(1)) if n not in (tg.names_in(m0.group(1)) if m0 else [])]
    say = lambda n, t: n + (f' — "{m.group(1)}"' if (m := SENTENCE.search(t)) else '')
    for n, t in after.items():
        if n not in before: ev.append('node added: ' + say(n, t))
        elif t != before[n]:
            s0, s1 = SENTENCE.search(before[n]), SENTENCE.search(t)
            ev.append(('node renamed: ' if s0 and s1 and s0.group(1) != s1.group(1) else 'node restated: ') + say(n, t))
    ev += ['node removed: ' + n for n in before if n not in after]
    return ev


def line(c):
    return f'{utc(c["time"], "%H:%M")} {c["id"][:7]} {c["by"]}: {c["subject"]}'


def view_days(cs):
    days, out = defaultdict(list), []
    for c in cs: days[utc(c['time'], '%Y-%m-%d')].append(c)
    for d, dcs in days.items():
        out.append(f'{d}  {len(dcs)} changes')
        for c in dcs:
            ev = [e for e in events(c) if not e.startswith('node restated')]
            if ev: out += ['  ' + line(c)] + ['        ' + e for e in ev]
        out.append('')
    return '\n'.join(out)


def view_day(cs, day):
    out = []
    for c in cs:
        if utc(c['time'], '%Y-%m-%d') == day: out += [line(c)] + ['      ' + e for e in events(c)]
    return '\n'.join(out) or f'no changes on {day}'


def view_change(x):
    c = next((c for c in changes() if c['id'].startswith(x)), None)
    if not c: return f'no change {x}'
    return '\n'.join([git('show', '-s', '--format=%H%nby %an, %ad%nparents %P%n%n%B', '--date=iso', c['id']).rstrip(),
                      git('show', '--stat=120', '--format=', c['id']).rstrip(), ''] + events(c))


def view_of(x):
    """a file: every change to it; a declaration: every change whose diff touches a line declaring it"""
    if '/' in x or '.' in x:
        r = git('log', '--all', '--date-order', '--format=%ct %h %an: %s', '--', x)
        return x + '\n' + '\n'.join('  ' + utc(l.split(' ', 1)[0]) + ' ' + l.split(' ', 1)[1] for l in r.split('\n') if l)
    out = [x]
    r = git('log', '--all', '--date-order', '--format=@%ct %h %an: %s', '--name-only', '-G', rf'^{re.escape(x)}( |$)', '--', '*.cub')
    for l in r.split('\n'):
        if l.startswith('@'): out.append('  ' + utc(l[1:].split(' ', 1)[0]) + ' ' + l[1:].split(' ', 1)[1])
        elif l: out.append('      ' + l)
    return '\n'.join(out + [f'(what uses it now: hyperactive def {x})'])


def view_node(cs, x):
    out = [view_of(x)]
    for c in cs:
        out += [f'  {utc(c["time"])} {c["id"][:7]} {e}' for e in events(c) if e in (f'in progress: {x}', f'resolved: {x}')]
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
