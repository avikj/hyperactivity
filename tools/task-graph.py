#!/usr/bin/env python3
"""work — the work's graph (2026-11-12; one kind of node and the order of work, 2026-11-13).

What the graph means and how it is used is docs/task-graph-design.md (Avik's design): read it first. The graph is
code: lib/task-graph.cub (what a node is: a task, its name and its parents; the relation after),
task-graph/hyperactivity.cub (this work's nodes, each defined after its parents; a goal's statement is the comment
above its definition). A node's children always cover it; where they are not certainly complete one
child is its residual (its text begins `residual of NODE`). A node opened for a specific piece of work
is named tN, and its text is the one comment in the repository that begins `tN (YYYY-MM-DD):`, at
the place it concerns. Every node in the graph is open; resolving one removes it.

    python3 tools/task-graph.py tree               the graph from its roots, open tN under each node
    python3 tools/task-graph.py show NODE ...      a node's statement or text, place, parts, after, before
    python3 tools/task-graph.py list [--under N]   the tN nodes, with the first words of their text
    python3 tools/task-graph.py priority           the order of work: what can start now, those that
                                             supersede others first; then what waits, on what
    python3 tools/task-graph.py check              graph and comments correspond; the graph checks
    python3 tools/task-graph.py next               the next tN number never used (the history included)
    python3 tools/task-graph.py open FILE:LINE NODE [NODE ...] NAME TEXT
                                             open the next tN, part of the NODEs, its text the
                                             comment `tN (date): TEXT` written above FILE:LINE;
                                             a part of the root or of a root's part asks y/N;
                                             a part of a node with no residual refutes its cover
    python3 tools/task-graph.py close tN           tN resolved: its node and text removed; a parent left
                                             with no children is resolved too, once confirmed
    python3 tools/task-graph.py view TERM          a view: a term over the work (lib/task-graph.cub's views over
                                             `declarations Node`) evaluated by the machine, shown by
                                             its type (a List Word as text)
    python3 tools/task-graph.py every              every file checked: each file no file imports, alone with its
                                             imports (--read); per file, the failures in its own text
    python3 tools/task-graph.py why NAME ...       why a declaration of the code exists: the chain to a root
    python3 tools/task-graph.py unexplained [FILE] the declarations with no reason, by file

Every declaration of the code is a node of the same graph. Its reason is computed: something with
a reason mentions it (the machine's reading `#uses`, over every file), or the
machine names it (src/), or it states one, in the comment above it: `(part of NODE)` or
`(prerequisite to NODE)`, NODE a node of task-graph/hyperactivity.cub. A node of task-graph/hyperactivity.cub has a reason when
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


GRAPH = os.path.join(ROOT, 'task-graph', 'hyperactivity.cub')
NODE = re.compile(r'^([\w-]+) = node "([^"]*)" (\(.*\)|nil)\s*$', re.M)


def names_in(s):
    return [x for x in re.findall(r'[\w-]+', s) if x not in ('cons', 'nil')]


def pair_list(text):
    """the pairs (a , list) of a relation written `cons (a , cons b (cons c nil)) (cons … nil)`: each pair is
    a parenthesized group whose first word is followed by ' , '; groups are read by balancing
    parentheses, so lists nest to any depth"""
    out, i = {}, 0
    while True:
        i = text.find('(', i)
        if i < 0: return out
        depth, j = 0, i
        while j < len(text):
            if text[j] == '(': depth += 1
            elif text[j] == ')':
                depth -= 1
                if depth == 0: break
            j += 1
        group = text[i + 1:j]
        m = re.match(r'([\w-]+) , (.*)$', group, re.S)
        if m:
            out[m.group(1)] = names_in(m.group(2)); i = j + 1
        else:
            i += 1


def graph():
    """the nodes: name -> dict(id, parts, statement), in definition order; the roots; after"""
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
        out[name] = dict(id=m.group(2), parts=names_in(m.group(3)), statement=' '.join(stmt))
    def pairs(key):
        m = re.search(r'^' + key + r' = (.*)$', src, re.M)
        res = {}
        if m: res = pair_list(m.group(1))
        return res
    m = re.search(r'^roots = (.*)$', src, re.M)
    roots = names_in(m.group(1)) if m else []
    return out, roots, pairs('after')


def essential_context():
    """documents (name -> path relative to the repository) and essential-context (node -> names)"""
    src = open(GRAPH, encoding='utf-8').read()
    base = os.path.dirname(GRAPH)
    docs = {n: os.path.relpath(os.path.normpath(os.path.join(base, q)), ROOT)
            for n, q in re.findall(r'^([\w-]+) = document "([^"]*)"', src, re.M)}
    ec = {}
    m = re.search(r'^essential-context = (.*)$', src, re.M)
    if m: ec = pair_list(m.group(1))
    return docs, ec


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
    return 'part of'


def cmd_show(args):
    g, roots, after = graph()
    txt = texts()
    for a in args:
        if a not in g and re.fullmatch(r'\d+', a): a = 't' + a
        d = g.get(a)
        if not d:
            print(f'{a}: no such node\n'); continue
        rels = ', '.join(f'{rel_word(g, p)} {p}' for p in d['parts']) or ('a root' if a in roots else '-')
        print(f'{a}  {rels}')
        print('  ' + d['id'])
        ks = kids_of(g).get(a, [])
        if ks: print('  served by: ' + ' '.join(ks))
        docs, ec = essential_context()
        ctx = [(dn, m) for m in sorted(ancestors(g, a), key=lambda m: depth(g, roots, m) or 0) for dn in ec.get(m, [])]
        for dn, m in ctx:
            print(f'  essential context: {docs.get(dn, dn)}' + ('' if m == a else f'  (from {m})'))
        if after.get(a): print('  after: ' + ' '.join(after[a]))
        waits = [m for m, l in after.items() if a in l]
        if waits: print('  before: ' + ' '.join(waits))
        bodies = [(f, l, dt, t) for f, l, dt, t in txt.get(int(a[1:]), [])] if is_t(a) else []
        if not is_t(a) and d['statement']: bodies = [('task-graph/hyperactivity.cub', 0, '', d['statement'])]
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
    g, _, _ = graph()
    sel = set(g)
    if '--under' in args:
        n = args[args.index('--under') + 1]
        sel = under(g, kids_of(g), n)
    txt = texts()
    for n in [x for x in g if is_t(x) and x in sel]:
        t = txt.get(int(n[1:]), [('', 0, '', '')])[0]
        print(f'{n:<5} {",".join(g[n]["parts"]):<28} {first_sentence(t[3], 80)}')


def cmd_tree(args):
    g, roots, _ = graph()
    kids = kids_of(g)
    seen = set()
    def walk(n, depth, via):
        d = g[n]
        k = kids.get(n, [])
        nt = sum(1 for x in under(g, kids, n) if is_t(x))
        if is_t(n): return
        also = [p for p in d['parts'] if p != via]
        print('  ' * depth + f'{d["id"]}  [{n}; {nt} open under it]'
              + (f'  (also {", ".join(also)})' if also and depth else ''))
        if n in seen: return
        seen.add(n)
        direct = [x for x in k if is_t(x)]
        for x in sorted(direct, key=lambda x: int(x[1:])): print('  ' * (depth + 1) + f'{x}  {g[x]["id"]}')
        for c in k:
            if not is_t(c) and g[c]['parts'] and g[c]['parts'][0] == n:
                walk(c, depth + 1, n)
    for r in roots: walk(r, 0, None)


def cmd_check(args):
    g, roots, after = graph()
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
        if ' ' not in d['id'] or d['id'] == n:
            print(f'{n}: its name must be an English sentence saying what it is, not an identifier'); bad += 1
        if not d['parts'] and n not in roots:
            print(f'{n}: part of nothing and not declared a root'); bad += 1
        if d['parts'] and n in roots:
            print(f'{n}: declared a root but part of {", ".join(d["parts"])}'); bad += 1
        if not is_t(n) and not d['statement']:
            print(f'{n}: no statement'); bad += 1
    for n, l in after.items():
        for m in [n] + l:
            if m not in g: print(f'{n}: names {m}, which is not a node'); bad += 1
    cmd = [os.path.join(ROOT, 'build', 'cubical'), 'task-graph/hyperactivity.cub']
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


def ancestors(g, n):
    """n and every node above it"""
    out, st = {n}, [n]
    while st:
        x = st.pop()
        for p in g.get(x, {}).get('parts', []):
            if p not in out: out.add(p); st.append(p)
    return out


def cmd_priority(args):
    """The order of work (docs/task-graph-design.md §5). There is no score. A leaf can start when every node its
    `after` names (on it or on any node above it: waiting on a node is waiting on every leaf under it)
    is resolved, i.e. gone from the graph. Among the leaves that can start now, X supersedes Y when the
    waiting work X is a prerequisite of strictly contains the waiting work Y is a prerequisite of (Avik:
    X and Y both dependencies of Z, X additionally unblocking other goals: X first). Printed in layers:
    no leaf of a layer is superseded by one of a later layer; within a layer, no order is expressed."""
    g, roots, after = graph()
    kids = kids_of(g)
    leaves = [n for n in g if not kids.get(n)]
    def leaves_under(n):
        return {x for x in under(g, kids, n) | {n} if not kids.get(x)}
    waits = {x: set().union(*([leaves_under(d) for m in ancestors(g, x) for d in after.get(m, []) if d in g] or [set()]))
             for x in leaves}
    ready = [x for x in leaves if not waits[x]]
    gain = {x: {y for y in leaves if x in waits[y]} for x in ready}   # the waiting work x is a prerequisite of (docs/task-graph-design.md §5)
    txt = texts()
    def line(n):
        t = txt.get(int(n[1:]), [('', 0, '', '')])[0][3] if is_t(n) else g[n]['statement']
        return f'  {n:<5} {",".join(g[n]["parts"]):<26} {first_sentence(t, 64)}'
    remaining, k = list(ready), 0
    print('can start now, in layers (a leaf supersedes the layers below it):')
    while remaining:
        top = [x for x in remaining if not any(gain[x] < gain[y] for y in remaining)]
        k += 1
        unblocking = [x for x in top if gain[x]]
        print(f' layer {k}' + ('' if unblocking else ' (a prerequisite of nothing that waits)'))
        for x in top:
            print(line(x) + (f'\n        a prerequisite of: {" ".join(sorted(gain[x]))}' if gain[x] else ''))
        remaining = [x for x in remaining if x not in top]
    blocked = [x for x in leaves if waits[x]]
    if blocked:
        print('waiting:')
        for x in blocked:
            print(line(x) + f'\n        waits on: {" ".join(sorted(waits[x]))}')


def cmd_next(args):
    print(f't{max(used_numbers(graph()[0]), default=0) + 1}')



STATED = re.compile(r'\((part of|prerequisite to) ([\w-]+)\)')


IMPORT = re.compile(r'^import "([^"]+)"', re.M)
READ_DIRS = ('lib', 'experiments', 'task-graph')


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
    # the files not yet read are read in parallel, one machine per core; each reader has its own store
    # (.cubical-store/readers/N), since the store writes its entries in place and a machine reading an
    # entry another is writing could read it half written; a reader's store warms after its first file
    import concurrent.futures, queue
    keys = {}
    for f in leaves:
        h = hashlib.sha256(bh.encode())
        for g in closure(f):
            h.update(g.encode()); h.update(open(os.path.join(ROOT, g), 'rb').read())
        keys[f] = os.path.join(cache, h.hexdigest())
    todo = [f for f in leaves if not os.path.exists(keys[f])]
    slots = queue.Queue()
    # as many readers as available memory allows: one machine reading the largest files grows past 5 GB
    # (measured 2026-11-13: experiments/isa/addw-all.cub was killed for lack of memory beside one other
    # reader on a 15 GB machine), so 8 GB per reader; HYPERACTIVE_READERS overrides
    try: mem_gb = int(next(l for l in open('/proc/meminfo') if l.startswith('MemAvailable')).split()[1]) / 2**20
    except (OSError, StopIteration, ValueError): mem_gb = 8
    n = int(os.environ.get('HYPERACTIVE_READERS') or max(1, min(os.cpu_count() or 1, int(mem_gb // 8))))
    for i in range(max(1, min(len(todo), n))): slots.put(i)
    def read_one(f):
        i = slots.get()
        try:
            print(f'reading {f}', file=sys.stderr, flush=True)
            e = dict(env, CUBICAL_STORE=os.path.join(ROOT, '.cubical-store', 'readers', str(i)))
            r = subprocess.run([binary, '--read', f, '--serve'], cwd=ROOT, input='#uses\n.\n', capture_output=True,
                               text=True, env=e)
            text = '\n'.join(l for l in r.stdout.split('\n') if l.startswith('uses '))
            if r.returncode < 0:   # killed (e.g. out of memory): not kept; a file whose definitions are all
                                   # refused reads as no uses, and that is kept
                print(f'reading {f} did not complete (exit {r.returncode}); it is read again next time', file=sys.stderr, flush=True)
                return
            open(keys[f], 'w', encoding='utf-8').write(text)
        finally:
            slots.put(i)
    with concurrent.futures.ThreadPoolExecutor(max_workers=slots.qsize() or 1) as ex:
        list(ex.map(read_one, todo))
    for f in leaves:
        if not os.path.exists(keys[f]): continue
        text = open(keys[f], encoding='utf-8').read()
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
    g, roots, _ = graph()
    src = ''.join(open(os.path.join(ROOT, 'src', f), encoding='utf-8', errors='replace').read()
                  for f in os.listdir(os.path.join(ROOT, 'src')) if f.endswith(('.c', '.h')))
    machine_names = set(re.findall(r'"([^"\s]+)"', src))
    machine_names |= {x[2:] for x in machine_names if x.startswith('%%')}   # "%%f": the library's f
    decls = {(f, l): (names, uses) for f, l, names, uses in rd}
    node_at = {}
    for f, l, names, _ in rd:
        if f == 'task-graph/hyperactivity.cub':
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
        if k[0] == 'task-graph/hyperactivity.cub' and any(n in g for n in names): continue
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
        print(f'{k[0]}:{k[1]}: states a reason, {stated[k][1]}, which is not a node of task-graph/hyperactivity.cub')
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
    r = subprocess.run([os.path.join(ROOT, 'build', 'cubical'), 'task-graph/hyperactivity.cub', '--serve'], cwd=ROOT,
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


def residual_of(g, txt, x):
    """the child of x whose text says it is x's residual, if any"""
    for k in kids_of(g).get(x, []):
        if is_t(k) and any(t.startswith(f'residual of {x}:') for _, _, _, t in txt.get(int(k[1:]), [])):
            return k
    return None


def render_pairs(pairs):
    out = 'nil'
    for a, bs in reversed(list(pairs.items())):
        lst = 'nil'
        for b in reversed(bs): lst = f'cons {b} ({lst})' if lst != 'nil' else f'cons {b} nil'
        out = f'cons ({a} , {lst}) ({out})' if out != 'nil' else f'cons ({a} , {lst}) nil'
    return out


def write_after(gs, after):
    return re.sub(r'^after = .*$', lambda m: 'after = ' + render_pairs(after), gs, count=1, flags=re.M)


def header_text(gs, n, body):
    """a tN text with no single site, written in task-graph/hyperactivity.cub's header before its import"""
    import textwrap
    lines = ['-- ' + b for b in textwrap.wrap(body, 97)] + ['--']
    at = gs.index('import "../lib/task-graph.cub"')
    return gs[:at] + '\n'.join(lines) + '\n' + gs[at:]


def add_node(gs, n, parents, name):
    last = list(re.finditer(r'^t\d+ = node ".*" .*\n', gs, re.M))[-1]
    parts = ''.join(f'(cons {x} ' for x in parents) + 'nil' + ')' * len(parents)
    return gs[:last.end()] + f't{n} : Node\nt{n} = node "{name}" {parts}\n' + gs[last.end():]


def used_numbers(g):
    used = {int(x[1:]) for x in g if is_t(x)} | set(texts())
    try:
        log = subprocess.run(['git', 'log', '-p', '--all', '--', 'work', 'lib/work.cub', 'task-graph', 'lib/task-graph.cub'], cwd=ROOT,
                             capture_output=True, text=True).stdout
        used |= {int(x) for x in re.findall(r'\bt(\d+)\b', log)}
    except OSError: pass
    return used


def cmd_open(args):
    import datetime, textwrap
    if len(args) < 4: sys.exit('open FILE:LINE NODE [NODE ...] NAME TEXT   (NAME: an English sentence saying what the ticket is)')
    site, nodes, name, text = args[0], args[1:-2], args[-2], args[-1]
    if ' ' not in name or '"' in name: sys.exit('NAME must be an English sentence saying what the ticket is, without double quotes')
    f, line = site.rsplit(':', 1); line = int(line)
    g, roots, after = graph()
    txt = texts()
    for x in nodes:
        if x not in g: sys.exit(f'{x}: not a node of task-graph/hyperactivity.cub')
        d = depth(g, roots, x)
        if d is not None and d <= 1:
            print(f'{x} is {"the root" if d == 0 else "a child of the root"}: attaching here puts the new node in front of')
            print('everyone; the most specific node is essential (docs/task-graph-design.md §6, points 6, 8, 10; python3 tools/task-graph.py tree).')
            if not sys.stdin.isatty() or input(f'attach it directly to {x}? [y/N] ').strip().lower() != 'y':
                sys.exit('not opened')
    # a child of a node with a residual is carved out of the residual's scope; a child of a node whose
    # children were held to cover it refutes that cover, and the missed requirement is a class (§3)
    refuted = [x for x in nodes if kids_of(g).get(x) and not residual_of(g, txt, x)]
    for x in refuted:
        print(f'{x}\'s children were held to cover it completely; a new child says they did not. The missed')
        print(f'requirement is a class (docs/task-graph-design.md §3, point 9): {x} also gets a residual, "determine and do')
        print(f'everything else {x} needs".')
        if not sys.stdin.isatty() or input(f'open a new child of {x}, and its residual? [y/N] ').strip().lower() != 'y':
            sys.exit('not opened')
    n = max(used_numbers(g), default=0) + 1
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
    gs = open(GRAPH, encoding='utf-8').read()
    gs = add_node(gs, n, nodes, name)
    for i, x in enumerate(refuted):
        n2 = n + 1 + i
        gs = add_node(gs, n2, [x], f'Everything else {g[x]["id"].rstrip(".")} requires.')
        gs = header_text(gs, n2, f't{n2} ({today}): residual of {x}: determine and do everything else {x} needs; '
                         f't{n} was a requirement its children did not cover, so their cover was refuted and the '
                         f'missed requirement is a class.')
    open(GRAPH, 'w', encoding='utf-8').write(gs)
    print(f't{n} opened, part of {", ".join(nodes)}, at {f}:{line}')
    for i, x in enumerate(refuted): print(f't{n + 1 + i} opened: the residual of {x}')


def remove_text(n):
    """delete tN's text comment at its site (and the blank comment line after it in the graph header)"""
    for f, l, _, _ in texts().get(n, []):
        path = os.path.join(ROOT, f)
        ls = open(path, encoding='utf-8').read().split('\n')
        i = l - 1
        j = i + 1
        block = '/*' in ls[i] and '*/' not in ls[i]
        while j < len(ls):
            if block:
                end = '*/' in ls[j]; j += 1
                if end: break
                continue
            cb = comment_body(ls[j])
            if MARK.search(ls[j]) or RULE.match(ls[j]) or cb is None or cb[0] == '' or cb[1] == '/*': break
            j += 1
        if f == 'task-graph/hyperactivity.cub' and j < len(ls) and ls[j].strip() == '--': j += 1
        del ls[i:j]
        open(path, 'w', encoding='utf-8').write('\n'.join(ls))


def remove_node(gs, x):
    """delete x's definition and, for a goal, its statement above it"""
    ls = gs.split('\n')
    at = next(k for k, l in enumerate(ls) if l == f'{x} : Node')
    top = at
    if not is_t(x):
        k = at - 1
        while k >= 0 and ls[k].startswith('--'):
            if re.match(r'^-- ' + re.escape(x) + r' \(', ls[k]): top = k; break
            k -= 1
    del ls[top:at + 2]
    return '\n'.join(ls)


def cmd_close(args):
    """tN is resolved: its node and its text are removed. A parent left with no children is resolved
    exactly when its children were (docs/task-graph-design.md §3), so it is removed too once confirmed; if it is
    not achieved, its children did not cover it, and its residual must be opened instead."""
    if len(args) != 1: sys.exit('close tN')
    x = args[0] if args[0].startswith('t') else 't' + args[0]
    g, roots, after = graph()
    if x not in g or not is_t(x): sys.exit(f'{x}: not an open tN node')
    if kids_of(g).get(x): sys.exit(f'{x} has children: it is resolved when they are')
    remove_text(int(x[1:]))
    gone, todo = [x], list(g[x]['parts'])
    gs = remove_node(open(GRAPH, encoding='utf-8').read(), x)
    while todo:
        p = todo.pop()
        left = [k for k, d in g.items() if p in d['parts'] and k not in gone]
        if left or p in roots: continue
        print(f'{p} has no children left: every part of it is resolved.')
        if sys.stdin.isatty() and input(f'is {p} achieved? [y/N] ').strip().lower() == 'y':
            gs = remove_node(gs, p); gone.append(p); todo += g[p]['parts']
        else:
            print(f'then its children did not cover it: open its residual (python3 tools/task-graph.py open ... {p} TEXT)')
    after = {a: [b for b in bs if b not in gone] for a, bs in after.items() if a not in gone}
    after = {a: bs for a, bs in after.items() if bs}
    gs = write_after(gs, after)
    open(GRAPH, 'w', encoding='utf-8').write(gs)
    print('resolved: ' + ' '.join(gone) + '  (commit with the fix; fold what matters into where it belongs)')


def main():
    if len(sys.argv) < 2 or sys.argv[1] in ('-h', '--help'):
        print(__doc__); return
    c, a = sys.argv[1], sys.argv[2:]
    {'list': cmd_list, 'show': cmd_show, 'tree': cmd_tree, 'check': cmd_check, 'next': cmd_next,
     'priority': cmd_priority, 'why': cmd_why, 'unexplained': cmd_unexplained, 'open': cmd_open, 'close': cmd_close, 'view': cmd_view, 'every': cmd_every}.get(c, lambda _: sys.exit('unknown command ' + c))(a)


if __name__ == '__main__':
    main()
