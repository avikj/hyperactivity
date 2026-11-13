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
    python3 tools/work.py open FILE:LINE NODE [NODE ...] TEXT
                                             open the next tN, part of the NODEs, its text the
                                             comment `tN (date): TEXT` written above FILE:LINE;
                                             a part of the root or of a root's part asks y/N
    python3 tools/work.py view TERM          a view: a term over the work (lib/work.cub's views over
                                             `declarations Node`) evaluated by the machine, shown by
                                             its type (a List Word as text)
    python3 tools/work.py every              every file checked: each file no file imports, alone with its
                                             imports (--read); per file, the failures in its own text
    python3 tools/work.py why NAME ...       why a declaration of the code exists: the chain to a root
    python3 tools/work.py unexplained [FILE] the declarations with no reason, by file

Every declaration of the code is a node of the same graph. Its reason is computed: something with
a reason mentions it (the machine's reading `#uses`, over every file), or the
machine names it (src/), or it states one, in the comment above it: `(part of NODE)` or
`(prerequisite to NODE)`, NODE a node of work/graph.cub. A node of work/graph.cub has a reason when
it is a root or part of a node with one. The first reading of a changed file runs the machine on
each file no file imports (minutes when the x86 model is among them); the results are kept.

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
NODE = re.compile(r'^([\w-]+) = (node|leaf) "([^"]*)" (\(.*\)|nil)(?: (all-of|each-of) (sufficient|not-known))?\s*$', re.M)


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
        out[name] = dict(id=m.group(3), parts=names_in(m.group(4)), value=m.group(5) or 'all-of',
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
        if d['id'] != n:
            print(f'{n}: its name is "{d["id"]}", not its declaration\'s'); bad += 1
        if not d['parts'] and n not in roots:
            print(f'{n}: part of nothing and not declared a root'); bad += 1
        if d['parts'] and n in roots:
            print(f'{n}: declared a root but part of {", ".join(d["parts"])}'); bad += 1
        if d['breakdown'] == 'sufficient' and not any(n in e['parts'] for e in g.values()):
            print(f'{n}: claims its parts suffice and has none'); bad += 1
        if not is_t(n) and not d['statement']:
            print(f'{n}: no statement'); bad += 1
    for n, l in list(after.items()) + list(speeds.items()):
        for m in [n] + l:
            if m not in g: print(f'{n}: names {m}, which is not a node'); bad += 1
    cmd = [os.path.join(ROOT, 'build', 'cubical'), 'work/graph.cub']
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



STATED = re.compile(r'\((part of|prerequisite to) ([\w-]+)\)')


IMPORT = re.compile(r'^import "([^"]+)"', re.M)
READ_DIRS = ('lib', 'experiments', 'work')


def cub_files():
    out = []
    for d in READ_DIRS:
        for dp, _, fs in os.walk(os.path.join(ROOT, d)):
            out += [os.path.relpath(os.path.join(dp, f), ROOT) for f in fs if f.endswith('.cub')]
    return sorted(out)


def imports_of(f):
    try: s = open(os.path.join(ROOT, f), encoding='utf-8').read()
    except OSError: return []
    return [os.path.normpath(os.path.join(os.path.dirname(f), x)) for x in IMPORT.findall(s)]


def closure(f, seen=None):
    seen = [] if seen is None else seen
    if f in seen: return seen
    for g in imports_of(f): closure(g, seen)
    seen.append(f)
    return seen


def reading():
    """the machine's #uses over every file: [(file, line, names, [(file, line)])]. A file another
    file imports is read in that file's run; every file no file imports is loaded alone (its imports
    with it) and read there. A run's result is kept under a key of the machine and of the texts it
    loads (.cubical-store/uses/), so only what changed is read again."""
    import hashlib
    files = cub_files()
    imported = {g for f in files for g in imports_of(f)}
    leaves = [f for f in files if f not in imported]
    binary = os.path.join(ROOT, 'build', 'cubical')
    bh = hashlib.sha256(open(binary, 'rb').read()).hexdigest()
    cache = os.path.join(ROOT, '.cubical-store', 'uses'); os.makedirs(cache, exist_ok=True)
    env = dict(os.environ)
    env.setdefault('CUBICAL_STORE', os.path.join(ROOT, '.cubical-store'))
    def pos(x):
        fl, l = x.rsplit(':', 1)
        return os.path.relpath(fl, ROOT) if os.path.isabs(fl) else fl, int(l)
    decls = {}
    for f in leaves:
        h = hashlib.sha256(bh.encode())
        for g in closure(f):
            h.update(g.encode()); h.update(open(os.path.join(ROOT, g), 'rb').read())
        kp = os.path.join(cache, h.hexdigest())
        if os.path.exists(kp): text = open(kp, encoding='utf-8').read()
        else:
            print(f'reading {f}', file=sys.stderr, flush=True)
            r = subprocess.run([binary, '--read', f, '--serve'], cwd=ROOT, input='#uses\n.\n', capture_output=True,
                               text=True, env=env)
            text = '\n'.join(l for l in r.stdout.split('\n') if l.startswith('uses '))
            open(kp, 'w', encoding='utf-8').write(text)
        for ln in text.split('\n'):
            if not ln.startswith('uses '): continue
            w = ln.split(); i = w.index(':')
            if w[1] == '?': continue
            k = pos(w[1])
            names, uses = decls.setdefault(k, ([], set()))
            for n in w[2:i]:
                if n not in names: names.append(n)
            uses |= {pos(x) for x in w[i + 1:] if x != '?'}
    return [(f, l, names, sorted(uses)) for (f, l), (names, uses) in decls.items()]


def stated_reason(lines, l):
    """the stated reason in the comment block directly above line l (1-based), or None"""
    k = l - 2
    while 0 <= k < len(lines) and lines[k].lstrip().startswith('--'):
        m = STATED.search(lines[k])
        if m: return m.group(1), m.group(2)
        k -= 1
    return None


def reasons():
    """(decls, why): decls by (file, line); why[(file, line)] = (how, what) for those with a reason"""
    rd = reading()
    g, roots, _, _ = graph()
    src = ''.join(open(os.path.join(ROOT, 'src', f), encoding='utf-8', errors='replace').read()
                  for f in os.listdir(os.path.join(ROOT, 'src')) if f.endswith(('.c', '.h')))
    machine_names = set(re.findall(r'"([^"\s]+)"', src))
    machine_names |= {x[2:] for x in machine_names if x.startswith('%%')}   # "%%f": the library's f
    decls = {(f, l): (names, uses) for f, l, names, uses in rd}
    node_at = {}
    for f, l, names, _ in rd:
        if f == 'work/graph.cub':
            for n in names:
                if n in g: node_at[n] = (f, l)
    files = {}
    def lines_of(f):
        if f not in files:
            try: files[f] = open(os.path.join(ROOT, f), encoding='utf-8').read().split('\n')
            except OSError: files[f] = []
        return files[f]
    why = {}
    for n in roots:
        if n in node_at: why[node_at[n]] = ('a root', None)
    stated = {}
    for k, (names, _) in decls.items():
        if k[0] == 'work/graph.cub' and any(n in g for n in names): continue
        st = stated_reason(lines_of(k[0]), k[1])
        if st: stated[k] = st
        elif any(n in machine_names for n in names if not n.startswith('#')): why[k] = ('named by the machine (src/)', None)   # a command's kind (#show) is not a name
    changed = True
    while changed:
        changed = False
        for n, d in g.items():
            k = node_at.get(n)
            if k and k not in why:
                for p in d['parts']:
                    if node_at.get(p) in why:
                        why[k] = (rel_word(g, p), node_at[p]); changed = True; break
        for k, (how, node) in stated.items():
            if k not in why and node_at.get(node) in why:
                why[k] = (how, node_at[node]); changed = True
        for k, (names, uses) in decls.items():
            if k in why:
                for u in uses:
                    if u in decls and u not in why:
                        why[u] = ('used by', k); changed = True
    return decls, why, stated, g, node_at


def label(decls, k):
    names = [n for n in decls.get(k, ([], []))[0] if '%' not in n]
    return f'{" ".join(names) or "(generated)"}  {k[0]}:{k[1]}'


def cmd_why(args):
    decls, why, _, _, _ = reasons()
    for a in args:
        ks = [k for k, (names, _) in decls.items() if a in names or f'{k[0]}:{k[1]}' == a]
        if not ks: print(f'{a}: no such declaration in the reading\n'); continue
        for k in ks:
            seen = set()
            print(label(decls, k))
            while k in why and k not in seen:
                seen.add(k)
                how, nxt = why[k]
                print(f'  {how}' + (f' {label(decls, nxt)}' if nxt else ''))
                if not nxt: break
                k = nxt
            if k not in why: print('  no reason')
            print()


def cmd_unexplained(args):
    decls, why, stated, g, node_at = reasons()
    bad = [k for k, (n, _) in stated.items() if n not in g]
    for k in bad:
        print(f'{k[0]}:{k[1]}: states a reason, {stated[k][1]}, which is not a node of work/graph.cub')
    by = {}
    for k in decls:
        if k not in why and (not args or k[0] in args or any(k[0].startswith(a) for a in args)):
            by.setdefault(k[0], []).append(k)
    total = sum(len(v) for v in by.values())
    for f in sorted(by):
        print(f'{f}  ({len(by[f])})')
        for k in sorted(by[f], key=lambda k: k[1]):
            print(f'  {k[1]:<5} ' + ' '.join(n for n in decls[k][0] if '%' not in n))
    print(f'{total} of {len(decls)} declarations read have no reason')




def cmd_view(args):
    term = ' '.join(args)
    env = dict(os.environ); env.setdefault('CUBICAL_STORE', os.path.join(ROOT, '.cubical-store'))
    r = subprocess.run([os.path.join(ROOT, 'build', 'cubical'), 'work/graph.cub', '--serve'], cwd=ROOT,
                       input=f'#type ({term})\n.\n#show {term}\n.\n', capture_output=True, text=True, env=env)
    ty = next((l.rsplit(' : ', 1)[1] for l in r.stdout.split('\n') if l.startswith('type: ') and ' : ' in l), None)
    val = next((l.split('  = ', 1)[1].rsplit('   [', 1)[0] for l in r.stdout.split('\n') if l.startswith('  = ')), None)
    if ty is None or val is None:
        sys.exit('\n'.join(l for l in r.stdout.split('\n') if l.startswith(('FAIL', '   ', 'error'))) or r.stdout[-2000:])
    toks = re.findall(r'\(|\)|[^\s()]+', val)
    def parse(i):
        if toks[i] == '(':
            items, i = [], i + 1
            while toks[i] != ')':
                x, i = parse(i); items.append(x)
            return items, i + 1
        return toks[i], i + 1
    def app(i):   # an application: the head and its arguments, to the end of the enclosing group
        items = []
        while i < len(toks) and toks[i] != ')':
            x, i = parse(i); items.append(x)
        return items
    def norm(x):
        return x[0] if isinstance(x, list) and len(x) == 1 else x
    def as_list(x):
        out = []
        x = norm(x)
        while isinstance(x, list) and x and x[0] == 'cons':
            out.append(x[1]); x = norm(x[2])
        if x != 'nil': raise ValueError
        return out
    def show(x, t):
        t = t.strip()
        if t.startswith('(') and t.endswith(')'): t = t[1:-1]
        try:
            if t == 'List Word':
                bs = bytes(int(w[1:]) for w in as_list(x))
                return '"' + bs.decode('utf-8') + '"'
            if t.startswith('List '):
                return [show(e, t[5:]) for e in as_list(x)]
        except (ValueError, UnicodeDecodeError, TypeError, IndexError):
            pass
        def flat(y): return y if isinstance(y, str) else '(' + ' '.join(flat(z) for z in y) + ')'
        return flat(norm(x))
    v = show(app(0), ty)
    if isinstance(v, list):
        for e in v: print(e if isinstance(e, str) else e)
        print(f'({len(v)}, {ty})')
    else: print(v, ':', ty)



def cmd_every(args):
    """t30: every file is checked by checking each file no file imports, loaded alone with its imports
    (a file another imports is checked in that file's run); a failure is the file's own when its
    position is in the file. Commands that act on the world are not performed (--read)."""
    files = cub_files()
    imported = {g for f in files for g in imports_of(f)}
    leaves = [f for f in files if f not in imported]
    env = dict(os.environ); env.setdefault('CUBICAL_STORE', os.path.join(ROOT, '.cubical-store'))
    own = {}
    for f in leaves:
        try:
            r = subprocess.run([os.path.join(ROOT, 'build', 'cubical'), '--read', f], cwd=ROOT, capture_output=True,
                               text=True, env=env, timeout=1800)
            out = r.stdout + r.stderr
        except subprocess.TimeoutExpired:
            print(f'{f}: did not finish in 30 minutes', flush=True); continue
        ls = out.split('\n')
        for i, l in enumerate(ls):
            if l.startswith('FAIL'):
                blk = ' '.join(ls[i:i + 4])
                m = re.search(r' in: (\S+?):\d+', blk) or re.search(r'(/\S+\.cub):\d+', blk)
                if m:
                    g = os.path.relpath(m.group(1), ROOT)
                    own.setdefault(g, set()).add(blk.split(' in: ')[-1].split()[0] if ' in: ' in blk else blk[:120])
        fatal = [l for l in ls if l.startswith(os.path.join(ROOT, f)) or l.startswith('error')]
        last = [l for l in ls if l.strip()][-1:] 
        print(f'{f}: {last[0][:60] if last else ""}' + (f'  ({fatal[0][:100]})' if fatal else ''), flush=True)
    print()
    for g in sorted(own):
        print(f'{g}: {len(own[g])} failing')
    print(f'{len(leaves)} files no file imports, checked; {len(own)} files with failures of their own')


def depth(g, roots, n, seen=()):
    """the node's least distance below a root"""
    if n in roots: return 0
    ds = [depth(g, roots, p, seen + (n,)) for p in g.get(n, {}).get('parts', []) if p not in seen]
    ds = [d for d in ds if d is not None]
    return 1 + min(ds) if ds else None


def cmd_open(args):
    import datetime, textwrap
    if len(args) < 3: sys.exit('open FILE:LINE NODE [NODE ...] TEXT')
    site, nodes, text = args[0], args[1:-1], args[-1]
    f, line = site.rsplit(':', 1); line = int(line)
    g, roots, _, _ = graph()
    for n in nodes:
        if n not in g: sys.exit(f'{n}: not a node of work/graph.cub')
        d = depth(g, roots, n)
        if d is not None and d <= 1:
            print(f'{n} is {"a root" if d == 0 else "a part of the root"}: the most specific node a piece of work is part of')
            print('is usually far below it (python3 tools/work.py tree).')
            if not sys.stdin.isatty() or input(f'attach it directly to {n}? [y/N] ').strip().lower() != 'y':
                sys.exit('not opened')
    refuted = [x for x in nodes if g[x]['breakdown'] == 'sufficient']
    for x in refuted:
        print(f'{x} claims its parts suffice; a new part refutes that claim, and the missed requirement is a')
        print(f'class: {x} becomes not-known, and "determine everything {x} requires and fill every gap" is')
        print(f'opened as a part of it beside this one.')
        if not sys.stdin.isatty() or input(f'open a new part of {x}? [y/N] ').strip().lower() != 'y':
            sys.exit('not opened')
    used = {int(x[1:]) for x in g if is_t(x)} | set(texts())
    try:
        log = subprocess.run(['git', 'log', '-p', '--all', '--', 'work', 'lib/work.cub'], cwd=ROOT,
                             capture_output=True, text=True).stdout
        used |= {int(x) for x in re.findall(r'\bt(\d+)\b', log)}
    except OSError: pass
    n = max(used, default=0) + 1
    today = datetime.date.today().isoformat()
    path = os.path.join(ROOT, f)
    ls = open(path, encoding='utf-8').read().split('\n')
    c = f.endswith(('.c', '.h'))
    ind = re.match(r'\s*', ls[line - 1]).group(0) if line - 1 < len(ls) else ''
    body = textwrap.wrap(f't{n} ({today}): {text}', 100 - len(ind) - 3)
    if c: com = [ind + '/* ' + body[0]] + [ind + ' * ' + b for b in body[1:]]; com[-1] += ' */'
    else: com = [ind + '-- ' + b for b in body]
    ls[line - 1:line - 1] = com
    open(path, 'w', encoding='utf-8').write('\n'.join(ls))
    gp = GRAPH; gs = open(gp, encoding='utf-8').read()
    last = list(re.finditer(r'^t\d+ = leaf "t\d+" .*\n', gs, re.M))[-1]
    parts = ''.join(f'(cons {x} ' for x in nodes) + 'nil' + ')' * len(nodes)
    gs = gs[:last.end()] + f't{n} : Node\nt{n} = leaf "t{n}" {parts}\n' + gs[last.end():]
    for x in refuted:
        n2 = n + 1 + refuted.index(x)
        gs = re.sub(r'^(' + re.escape(x) + r' = node "[^"]*" .*) sufficient$', r'\1 not-known', gs, flags=re.M)
        last = list(re.finditer(r'^t\d+ = leaf "t\d+" .*\n', gs, re.M))[-1]
        gs = gs[:last.end()] + f't{n2} : Node\nt{n2} = leaf "t{n2}" (cons {x} nil)\n' + gs[last.end():]
        gl = gs.split('\n'); at = next(k for k, l in enumerate(gl) if l.startswith(x + ' : Node'))
        gl[at:at] = ['-- ' + b for b in textwrap.wrap(f't{n2} ({today}): determine everything {x} requires and fill '
                     f'every gap: t{n} was a missed requirement of {x}, whose breakdown was claimed sufficient, so that '
                     f'claim is refuted ({x} is now not-known) and the missed requirement is a class.', 97)]
        gs = '\n'.join(gl)
    open(gp, 'w', encoding='utf-8').write(gs)
    print(f't{n} opened, part of {", ".join(nodes)}, at {f}:{line}')
    for x in refuted: print(f't{n + 1 + refuted.index(x)} opened, part of {x}: determine everything {x} requires')


def main():
    if len(sys.argv) < 2 or sys.argv[1] in ('-h', '--help'):
        print(__doc__); return
    c, a = sys.argv[1], sys.argv[2:]
    {'list': cmd_list, 'show': cmd_show, 'tree': cmd_tree, 'check': cmd_check, 'next': cmd_next,
     'priority': cmd_priority, 'why': cmd_why, 'unexplained': cmd_unexplained, 'open': cmd_open, 'view': cmd_view, 'every': cmd_every}.get(c, lambda _: sys.exit('unknown command ' + c))(a)


if __name__ == '__main__':
    main()
