#!/usr/bin/env python3
"""work — the work's graph (2026-11-12).

The graph is code: lib/work.cub (what a node is: its parts, its value, its breakdown; after; speeds),
work/graph.cub (this work's nodes, each defined after the nodes it is part of; a goal's statement is
the comment above its definition). A node opened for a specific piece of work is named tN, and its
text is the one comment in the repository that begins `tN (YYYY-MM-DD):`, at the place it concerns.
Every node in the graph is open; closing one removes it.

    python3 tools/work.py tree               the graph from its roots, open tN under each node
    python3 tools/work.py show NODE ...      a node's statement or text, place, parts, after, before
    python3 tools/work.py list [--under N]   the tN nodes, with the first words of their text
    python3 tools/work.py priority [K]       the tN nodes in the order the graph's relations imply
    python3 tools/work.py check              graph and comments correspond; the graph checks
    python3 tools/work.py next               the next tN number never used (the history included)

Run from the repository root or anywhere inside it.
"""
import os, re, signal, subprocess, sys
signal.signal(signal.SIGPIPE, signal.SIG_DFL)

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


GRAPH = os.path.join(ROOT, 'work', 'graph.cub')
NODE = re.compile(r'^([\w-]+) = (node|leaf) #(\d+) (\(.*\)|nil)(?: (all-of|each-of) (sufficient|not-known))?\s*$', re.M)


def names_in(s):
    return [x for x in re.findall(r'[\w-]+', s) if x not in ('cons', 'nil')]


def graph():
    """the nodes: name -> dict(id, parts, value, breakdown, statement), in definition order"""
    src = open(GRAPH, encoding='utf-8').read()
    lines = src.split('\n')
    out = {}
    for m in NODE.finditer(src):
        name = m.group(1)
        ln = src[:m.start()].count('\n')
        k, stmt = ln - 2, []
        while k >= 0 and lines[k].startswith('--'):
            stmt.insert(0, lines[k][2:].strip())
            if re.match(r'^-- ' + re.escape(name) + r' \(', lines[k]):
                break
            k -= 1
        if not (stmt and re.match(re.escape(name) + r' \(', stmt[0])):
            stmt = []
        out[name] = dict(id=int(m.group(3)), parts=names_in(m.group(4)), value=m.group(5) or 'all-of',
                         breakdown=m.group(6) or 'not-known', statement=' '.join(stmt))
    def pairs(key):
        m = re.search(r'^' + key + r' = (.*)$', src, re.M)
        res = {}
        if m:
            for a, b in re.findall(r'\(([\w-]+) , ((?:[^()]|\([^()]*\))*)\)', m.group(1)):
                res[a] = names_in(b)
        return res
    m = re.search(r'^roots = (.*)$', src, re.M)
    roots = names_in(m.group(1)) if m else []
    return out, roots, pairs('after'), pairs('speeds')


def kids_of(g):
    k = {n: [] for n in g}
    for n, d in g.items():
        for p in d['parts']:
            k.setdefault(p, []).append(n)
    return k


def is_t(n):
    return re.fullmatch(r't\d+', n) is not None


def first_sentence(t, width=110):
    t = re.sub(r'^\s*', '', t)
    return t if len(t) <= width else t[:width - 1] + '…'


def rel_word(g, parent):
    return 'prerequisite to' if g.get(parent, {}).get('value') == 'all-of' else 'part of'


def cmd_show(args):
    g, roots, after, speeds = graph()
    txt = texts()
    for a in args:
        if a not in g and re.fullmatch(r'\d+', a): a = 't' + a
        d = g.get(a)
        if not d:
            print(f'{a}: no such node\n'); continue
        rels = ', '.join(f'{rel_word(g, p)} {p}' for p in d['parts']) or ('a root' if a in roots else '-')
        print(f'{a}  {rels}' + (f'  [{d["value"]}, breakdown {d["breakdown"]}]' if not is_t(a) else ''))
        if after.get(a): print('  after: ' + ' '.join(after[a]))
        waits = [m for m, l in after.items() if a in l]
        if waits: print('  before: ' + ' '.join(waits))
        if speeds.get(a): print('  speeds: ' + ' '.join(speeds[a]))
        bodies = [(f, l, dt, t) for f, l, dt, t in txt.get(int(a[1:]), [])] if is_t(a) else []
        if not is_t(a) and d['statement']: bodies = [('work/graph.cub', 0, '', d['statement'])]
        for f, l, dt, t in bodies:
            print(f'  {f}' + (f':{l}  ({dt})' if l else ''))
            words, line = t.split(), ''
            for w in words:
                if len(line) + len(w) + 1 > 96:
                    print('    ' + line); line = w
                else:
                    line = (line + ' ' + w).strip()
            if line: print('    ' + line)
        if not bodies: print('  (no text)')
        print()


def under(g, kids, n):
    out, st = set(), [n]
    while st:
        x = st.pop()
        for k in kids.get(x, []):
            if k not in out: out.add(k); st.append(k)
    return out


def cmd_list(args):
    g, _, _, _ = graph()
    sel = set(g)
    if '--under' in args:
        n = args[args.index('--under') + 1]
        sel = under(g, kids_of(g), n)
    txt = texts()
    for n in [x for x in g if is_t(x) and x in sel]:
        t = txt.get(int(n[1:]), [('', 0, '', '')])[0]
        print(f'{n:<5} {",".join(g[n]["parts"]):<28} {first_sentence(t[3], 80)}')


def cmd_tree(args):
    g, roots, _, _ = graph()
    kids = kids_of(g)
    seen = set()
    def walk(n, depth, via):
        d = g[n]
        k = kids.get(n, [])
        nt = sum(1 for x in under(g, kids, n) if is_t(x))
        if is_t(n): return
        also = [p for p in d['parts'] if p != via]
        print('  ' * depth + f'{n}  [{d["value"]}, breakdown {d["breakdown"]}; {nt} open under it]'
              + (f'  (also {", ".join(also)})' if also and depth else ''))
        if n in seen: return
        seen.add(n)
        direct = [x for x in k if is_t(x)]
        if direct: print('  ' * (depth + 1) + ' '.join(sorted(direct, key=lambda x: int(x[1:]))))
        for c in k:
            if not is_t(c) and g[c]['parts'] and g[c]['parts'][0] == n:
                walk(c, depth + 1, n)
    for r in roots: walk(r, 0, None)


def cmd_check(args):
    g, roots, after, speeds = graph()
    txt = texts()
    bad = 0
    ts = {int(n[1:]) for n in g if is_t(n)}
    for n in sorted(ts | set(txt)):
        if n not in ts:
            print(f't{n}: a text comment with no node ({txt[n][0][0]}:{txt[n][0][1]})'); bad += 1
        elif n not in txt:
            print(f't{n}: a node with no text comment'); bad += 1
        elif len(txt[n]) > 1:
            print(f't{n}: {len(txt[n])} text comments: ' + ', '.join(f'{f}:{l}' for f, l, _, _ in txt[n])); bad += 1
    for n, d in g.items():
        if is_t(n) and d['id'] != int(n[1:]):
            print(f'{n}: id #{d["id"]} is not its number'); bad += 1
        if not d['parts'] and n not in roots:
            print(f'{n}: part of nothing and not declared a root'); bad += 1
        if d['parts'] and n in roots:
            print(f'{n}: declared a root but part of {", ".join(d["parts"])}'); bad += 1
        if not is_t(n) and not d['statement']:
            print(f'{n}: no statement'); bad += 1
    ids = [d['id'] for d in g.values()]
    if len(ids) != len(set(ids)):
        print('two nodes share an id'); bad += 1
    for n, l in list(after.items()) + list(speeds.items()):
        for m in [n] + l:
            if m not in g: print(f'{n}: names {m}, which is not a node'); bad += 1
    mk = open(os.path.join(ROOT, 'Makefile')).read()
    core = re.search(r'^CORE := (.*)$', mk, re.M).group(1).split()
    cmd = [os.path.join(ROOT, 'build', 'cubical')] + core + ['lib/work.cub', 'work/graph.cub']
    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, env=dict(os.environ, CUBICAL_NOCACHE='1'))
    last = [l for l in r.stdout.split('\n') if l.strip()][-1:] or ['']
    fails = [l for l in r.stdout.split('\n') if l.startswith('FAIL')]
    print('machine: ' + last[0])
    for f in fails: print('  ' + f)
    if r.returncode != 0 or fails:
        print(r.stderr.strip()[:2000]); bad += 1
    print(f'{sum(1 for n in g if is_t(n))} open tN nodes, {sum(1 for n in g if not is_t(n))} other nodes; '
          + ('consistent' if not bad else f'{bad} problem(s)'))
    sys.exit(1 if bad else 0)


def cmd_priority(args):
    """the tN nodes in the order the graph implies: a node's cost is the number of open nodes that must
    complete before any value lands through it, at a root, along its cheapest path (an all-of node on
    the path adds everything under it; a node with nothing under it counts as one piece of unspecified
    work). Not ready (an after still open) last; then by cost; then by how much its nodes speed. The
    count is a proxy for effort, which the graph does not hold."""
    g, roots, after, speeds = graph()
    kids = kids_of(g)
    memo = {}
    def needed(n):
        if n not in memo:
            u = under(g, kids, n)
            memo[n] = frozenset(x for x in u if is_t(x) or not kids.get(x)) or frozenset({n})
        return memo[n]
    def cost(n, carried, seen=()):
        if n in seen: return None
        need = carried | (needed(n) if g[n]['value'] == 'all-of' else frozenset())
        if n in roots: return need
        best = None
        for p in g[n]['parts']:
            c = cost(p, need, seen + (n,))
            if c is not None and (best is None or len(c) < len(best)): best = c
        return best
    rows = []
    for n in g:
        if not is_t(n): continue
        best = None
        for p in g[n]['parts']:
            c = cost(p, frozenset({n}))
            if c is not None and (best is None or len(c) < len(best)): best = c
        ready = all(m not in g for m in after.get(n, []))
        sp = sum(len(speeds.get(p, [])) for p in g[n]['parts'])
        rows.append((not ready, len(best) if best else 10**6, -sp, int(n[1:]), n))
    rows.sort()
    txt = texts()
    for blocked, c, sp, _, n in rows[: int(args[0]) if args else len(rows)]:
        first = txt.get(int(n[1:]), [('', 0, '', '')])[0][3]
        print(f'{n:<4} cost {c:<4}' + (' speeds' if sp else '       ') + (' after-open' if blocked else '           ')
              + f' {",".join(g[n]["parts"]):<28} {first_sentence(first, 70)}')


def cmd_next(args):
    used = {int(n[1:]) for n in graph()[0] if is_t(n)} | set(texts())
    try:
        log = subprocess.run(['git', 'log', '-p', '--all', '--', 'work', 'lib/work.cub'], cwd=ROOT,
                             capture_output=True, text=True).stdout
        used |= {int(x) for x in re.findall(r'\bt(\d+)\b', log)}
    except OSError:
        pass
    print(f't{max(used, default=0) + 1}')


def main():
    if len(sys.argv) < 2 or sys.argv[1] in ('-h', '--help'):
        print(__doc__); return
    c, a = sys.argv[1], sys.argv[2:]
    {'list': cmd_list, 'show': cmd_show, 'tree': cmd_tree, 'check': cmd_check, 'next': cmd_next,
     'priority': cmd_priority}.get(c, lambda _: sys.exit('unknown command ' + c))(a)


if __name__ == '__main__':
    main()
