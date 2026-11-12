#!/usr/bin/env python3
"""work — the record of the work on this repository (2026-11-12).

The record is code: lib/work.cub (what a goal and a ticket are), work/goals.cub (the goals, each
with its statement as the comment above its `serves` clause), work/tickets.cub (one checked
definition `tN : Ticket Goal` per ticket: kind, goals, opened or closed #YYYYMMDD). A ticket's text
is the one comment in the repository that begins `tN (YYYY-MM-DD):`, at the place it concerns.

    python3 tools/work.py list [--all | --closed] [--goal G] [--kind K]
    python3 tools/work.py show tN ...         the text of each ticket, with its place
    python3 tools/work.py goals               the goals as a tree, open/closed tickets under each
    python3 tools/work.py check               records and comments correspond; the files check
    python3 tools/work.py next                the next free ticket number

Run from scoped-machine/ or anywhere inside it.
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKIP_DIRS = {'.git', 'build', 'archive', '__pycache__'}
EXTS = ('.cub', '.c', '.h', '.py', '.md', '.s', '.sh', '.txt')
MARK = re.compile(r'\bt(\d+) \((\d{4}-\d{2}-\d{2})\):')
RULE = re.compile(r'^\s*(--|#|//|\*)\s*-{4,}')


def rel(p):
    return os.path.relpath(p, ROOT)


def files():
    for d, ds, fs in os.walk(ROOT):
        ds[:] = sorted(x for x in ds if x not in SKIP_DIRS)
        for f in sorted(fs):
            if f.endswith(EXTS) or f == 'Makefile':
                yield os.path.join(d, f)


def comment_body(line):
    """the text of a comment line and its style, or None when the line is not a comment"""
    s = line.strip()
    for pre in ('--', '//', '#'):
        if s.startswith(pre):
            return s[len(pre):].strip(), pre
    if s.startswith('/*'):
        return s[2:].strip(), '/*'
    if s.startswith('*') and not s.startswith('*/'):
        return s[1:].strip(), '*'
    return None


def texts():
    """every ticket comment: number -> list of (file, line, date, text)"""
    out = {}
    for p in files():
        if os.path.abspath(p) == os.path.abspath(__file__):
            continue
        try:
            lines = open(p, encoding='utf-8', errors='replace').read().split('\n')
        except OSError:
            continue
        for i, line in enumerate(lines):
            m = MARK.search(line)
            if not m or comment_body(line) is None and '/*' not in line:
                continue
            n, date = int(m.group(1)), m.group(2)
            body = [line[m.end():].strip()]
            block = '/*' in line[:m.start()] and '*/' not in line[m.end():]
            if '*/' in body[0]:
                body[0] = body[0].split('*/')[0].strip()
                block = False
            j = i + 1
            while j < len(lines):
                nxt = lines[j]
                if MARK.search(nxt) or RULE.match(nxt):
                    break
                if block:
                    t = nxt.strip()
                    end = '*/' in t
                    t = t.split('*/')[0].strip().lstrip('*').strip()
                    if t:
                        body.append(t)
                    j += 1
                    if end:
                        break
                    continue
                cb = comment_body(nxt)
                if cb is None or cb[0] == '' or cb[1] in ('/*',):
                    break
                body.append(cb[0])
                j += 1
            out.setdefault(n, []).append((rel(p), i + 1, date, ' '.join(body).strip()))
    return out


def records():
    """the ticket records of work/tickets.cub: number -> (kind, goals, status)"""
    src = open(os.path.join(ROOT, 'work', 'tickets.cub'), encoding='utf-8').read()
    out = {}
    for m in re.finditer(r'^t(\d+) = ticket (\S+) (.*?) (opened|\(closed #(\d+)\))\s*$', src, re.M):
        goals = [g for g in re.findall(r'[\w-]+', m.group(3)) if g not in ('cons', 'nil')]
        out[int(m.group(1))] = (m.group(2), goals, 'open' if m.group(4) == 'opened' else 'closed ' + m.group(5))
    return out


def goals():
    """the goals: name -> (serves, statement)"""
    src = open(os.path.join(ROOT, 'work', 'goals.cub'), encoding='utf-8').read()
    names = re.search(r'^data Goal = (.*)$', src, re.M).group(1).split('|')
    names = [n.strip() for n in names]
    out = {}
    lines = src.split('\n')
    for i, line in enumerate(lines):
        m = re.match(r'^serves ([\w-]+) = (.*)$', line)
        if not m:
            continue
        serv = [g for g in re.findall(r'[\w-]+', m.group(2)) if g not in ('cons', 'nil')]
        k = i - 1
        stmt = []
        while k >= 0 and lines[k].startswith('--'):
            stmt.insert(0, lines[k][2:].strip())
            if re.match(r'^-- ' + re.escape(m.group(1)) + r' \(', lines[k]):
                break
            k -= 1
        out[m.group(1)] = (serv, ' '.join(stmt))
    for n in names:
        out.setdefault(n, ([], ''))
    return names, out


def first_sentence(t, width=110):
    t = re.sub(r'^\s*', '', t)
    return t if len(t) <= width else t[:width - 1] + '…'


def cmd_list(args):
    recs, txt = records(), texts()
    want = 'open'
    goal = kind = None
    it = iter(args)
    for a in it:
        if a == '--all': want = None
        elif a == '--closed': want = 'closed'
        elif a == '--goal': goal = next(it)
        elif a == '--kind': kind = next(it)
        else: sys.exit('unknown argument ' + a)
    for n in sorted(recs):
        k, gs, st = recs[n]
        if want == 'open' and st != 'open': continue
        if want == 'closed' and st == 'open': continue
        if goal and goal not in gs: continue
        if kind and kind != k: continue
        where, text = ('(no text)', '')
        if n in txt:
            f, l, d, text = txt[n][0]
            where = f'{f}:{l}'
        print(f't{n:<4} {st:<15} {k:<11} {",".join(gs) or "-":<32} {where}')
        print(f'      {first_sentence(text)}')


def cmd_show(args):
    recs, txt = records(), texts()
    for a in args:
        n = int(a.lstrip('t'))
        k, gs, st = recs.get(n, ('?', [], 'no record'))
        print(f't{n}  {k}  {st}  goals: {", ".join(gs) or "-"}')
        for f, l, d, text in txt.get(n, []):
            print(f'  {f}:{l}  ({d})')
            words, line = text.split(), ''
            for w in words:
                if len(line) + len(w) + 1 > 96:
                    print('    ' + line); line = w
                else:
                    line = (line + ' ' + w).strip()
            if line: print('    ' + line)
        if n not in txt: print('  (no text comment found)')
        print()


def cmd_goals(args):
    names, gl = goals()
    recs = records()
    kids = {n: [] for n in names}
    for n, (serv, _) in gl.items():
        for s in serv:
            kids.setdefault(s, []).append(n)
    def counts(g):
        o = sum(1 for r in recs.values() if g in r[1] and r[2] == 'open')
        c = sum(1 for r in recs.values() if g in r[1] and r[2] != 'open')
        return o, c
    seen = set()
    def walk(g, depth):
        o, c = counts(g)
        also = [s for s in gl[g][0]][1:] if depth else []
        print('  ' * depth + f'{g}  [{o} open, {c} closed]' + (f'  (also serves {", ".join(also)})' if also else ''))
        if g in seen: return
        seen.add(g)
        for k in kids.get(g, []):
            if gl[k][0] and gl[k][0][0] == g:
                walk(k, depth + 1)
    for g in names:
        if not gl[g][0]:
            walk(g, 0)
    un = [n for n, r in recs.items() if not r[1] and r[2] == 'open']
    if un: print('serving no goal yet: ' + ' '.join(f't{n}' for n in sorted(un)))


def cmd_check(args):
    recs, txt = records(), texts()
    names, gl = goals()
    bad = 0
    for n in sorted(set(recs) | set(txt)):
        if n not in recs:
            print(f't{n}: a text comment with no record ({txt[n][0][0]}:{txt[n][0][1]})'); bad += 1
        elif n not in txt and recs[n][2] == 'open':
            print(f't{n}: an open ticket with no text comment'); bad += 1
        elif n in txt and len(txt[n]) > 1:
            print(f't{n}: {len(txt[n])} text comments: ' + ', '.join(f'{f}:{l}' for f, l, _, _ in txt[n])); bad += 1
        elif n in txt and recs[n][2] != 'open':
            print(f't{n}: closed, but its comment is still at {txt[n][0][0]}:{txt[n][0][1]}'); bad += 1
        if n in recs:
            for g in recs[n][1]:
                if g not in names:
                    print(f't{n}: unknown goal {g}'); bad += 1
    for g in names:
        if not gl[g][1]:
            print(f'goal {g}: no statement'); bad += 1
    mk = open(os.path.join(ROOT, 'Makefile')).read()
    core = re.search(r'^CORE := (.*)$', mk, re.M).group(1).split()
    cmd = [os.path.join(ROOT, 'build', 'cubical')] + core + ['lib/work.cub', 'work/goals.cub', 'work/tickets.cub']
    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, env=dict(os.environ, CUBICAL_NOCACHE='1'))
    last = [l for l in r.stdout.split('\n') if l.strip()][-1:] or ['']
    fails = [l for l in r.stdout.split('\n') if l.startswith('FAIL')]
    print('machine: ' + last[0])
    for f in fails: print('  ' + f)
    if r.returncode != 0 or fails:
        print(r.stderr.strip()[:2000]); bad += 1
    print(f'{len(recs)} tickets ({sum(1 for r in recs.values() if r[2] == "open")} open), {len(names)} goals; '
          + ('consistent' if not bad else f'{bad} problem(s)'))
    sys.exit(1 if bad else 0)


def cmd_next(args):
    used = set(records()) | set(texts())
    print(f't{max(used, default=0) + 1}')


def main():
    if len(sys.argv) < 2 or sys.argv[1] in ('-h', '--help'):
        print(__doc__); return
    c, a = sys.argv[1], sys.argv[2:]
    {'list': cmd_list, 'show': cmd_show, 'goals': cmd_goals, 'check': cmd_check, 'next': cmd_next}.get(
        c, lambda _: sys.exit('unknown command ' + c))(a)


if __name__ == '__main__':
    main()
