#!/usr/bin/env python3
"""task-graph — the task graph's tool (2026-11-12; one kind of node and the order of work, 2026-11-13).

What the graph means and how it is used is “The Hyperactive Workflow” (docs/to-be-incorporated-into-paper.md) (Avik's design): read it first. The
graph is code: lib/task-graph.cub (what a node is: a task, its name and its parents; the relations
after and essential-context), task-graph/hyperactivity.cub (this work's nodes, each defined after
its parents; a node's statement is the comment above its definition). Run through `hyperactive`.

    python3 tools/task-graph.py tree               the graph from its roots
    python3 tools/task-graph.py show NODE ...      a node's statement, parents, children, after, essential context
    python3 tools/task-graph.py priority           the order of work: what can start now, those that
                                             supersede others first; then what waits, on what
    python3 tools/task-graph.py check              the graph checks (names are sentences, statements exist, roots declared)
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
import json, hashlib, os, re, signal, subprocess, sys
signal.signal(signal.SIGPIPE, signal.SIG_DFL)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKIP_DIRS = {'.git', 'build', 'archive', '__pycache__'}
EXTS = ('.cub', '.c', '.h', '.py', '.md', '.s', '.sh', '.txt')


GRAPH = os.path.join(ROOT, 'task-graph', 'hyperactivity.cub')
NODE = re.compile(r'^([\w-]+) = node "([^"]*)" (\(.*\)|nil)\s*$', re.M)
STATED = re.compile(r'\((part of|prerequisite to) ([\w-]+)\)')
IMPORT = re.compile(r'^import "([^"]+)"', re.M)
READ_DIRS = ('lib', 'experiments', 'task-graph')


def rel(p):
    return os.path.relpath(p, ROOT)


def files():
    for d, ds, fs in os.walk(ROOT):
        ds[:] = sorted(x for x in ds if x not in SKIP_DIRS)
        for f in sorted(fs):
            if f.endswith(EXTS) or f == 'Makefile':
                yield os.path.join(d, f)


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


TOP = re.compile(r"^([^\s-][^\s:=]*)")


def declarations(text):
    """the top-level declarations of a .cub text, each with the comment directly above it (its
    statement): [(name, line, text)], line 1-based; read by `hyperactive def` and by the history"""
    lines, out, i = text.split('\n'), [], 0
    while i < len(lines):
        m = TOP.match(lines[i])
        if not m or m.group(1) in ('import', '#same', '#show', '#type', '#where'):
            i += 1; continue
        name = m.group(1) if m.group(1) not in ('data', 'codata', 'record') else (lines[i].split() + ['', ''])[1]
        start = i
        while start > 0 and lines[start - 1].startswith('--'): start -= 1
        j = i + 1
        while j < len(lines) and lines[j].strip() and (lines[j][0].isspace() or (TOP.match(lines[j]) and TOP.match(lines[j]).group(1) == name)):
            j += 1
        out.append((name, i + 1, '\n'.join(lines[start:j])))
        i = j
    return out


def graph(src=None):
    """the nodes: name -> dict(id, parts, statement), in definition order; the roots; after. src: the
    graph's text, by default task-graph/hyperactivity.cub as it is now (the history passes a past one)"""
    if src is None: src = open(GRAPH, encoding='utf-8').read()
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


def documents(file, name):
    """the documents a value of type List Document holds, in order, as the machine evaluates it:
    ('file', path from the repository root, sha256 hex) for a document-at, and ('text', name, text)
    for a projection, the text a value's projection computes (lib/document.cub). The machine reads
    `file` with its imports and shows the value's normal form."""
    machine = os.path.join(ROOT, 'build', 'cubical')
    os.makedirs(os.path.join(ROOT, 'build'), exist_ok=True)
    # the result is kept under the content it depends on: the machine, every file the evaluation
    # reads (file, its imports, the files their `text` forms load), and the bytes of every file the
    # result names, checked against its digests; any change there evaluates it again
    key = hashlib.sha256(name.encode())
    for f in [machine] + reads(file):
        with open(f, 'rb') as fh: key.update(f.encode() + b'\0' + hashlib.sha256(fh.read()).digest())
    kept = os.path.join(ROOT, 'build', 'documents-' + key.hexdigest() + '.json')
    if os.path.isfile(kept):
        with open(kept, encoding='utf-8') as fh: docs = [tuple(d) for d in json.load(fh)]
        if all(k != 'file' or (os.path.isfile(os.path.join(ROOT, a)) and file_digest(os.path.join(ROOT, a)) == b)
               for k, a, b in docs):
            return docs
    probe = os.path.join(ROOT, 'build', 'documents-' + re.sub(r'[^\w-]', '_', name) + '.cub')
    with open(probe, 'w', encoding='utf-8') as f:
        f.write(f'import "{os.path.abspath(file)}"\n#show {name}\n')
    r = subprocess.run([machine, probe], cwd=ROOT, capture_output=True, text=True)
    if r.returncode != 0 or re.search(r'^FAIL', r.stdout, re.M):
        sys.exit(f'the machine could not evaluate {name} ({os.path.relpath(file, ROOT)}):\n' + r.stdout[-3000:] + r.stderr[-2000:])
    docs = decode_documents(r.stdout, name)
    with open(kept + '.part', 'w', encoding='utf-8') as fh: json.dump(docs, fh)
    os.replace(kept + '.part', kept)
    return docs


def file_digest(path):
    with open(path, 'rb') as fh: return hashlib.sha256(fh.read()).hexdigest()


def reads(file):
    """the files the machine reads for `file`: it, its imports transitively, and what their
    `text "path"` forms load (paths relative to the file naming them)"""
    seen, todo = [], [os.path.abspath(file)]
    while todo:
        f = todo.pop()
        if f in seen or not os.path.isfile(f): continue
        seen.append(f)
        with open(f, encoding='utf-8', errors='replace') as fh: src = fh.read()
        for q in re.findall(r'^import "([^"]+)"', src, re.M) + re.findall(r'\btext "([^"]+)"', src):
            todo.append(os.path.normpath(os.path.join(os.path.dirname(f), q)))
    return sorted(seen)


def decode_documents(shown, name):
    """the entries of a shown normal form `name = cons (document-at P D) (cons (projection N T) … nil)`:
    ('file', P, D) and ('text', N, T), each field a List Word of bytes"""
    m = re.search(r'^' + re.escape(name) + r'\n\s*= (.*?)(?=^\S|\Z)', shown, re.M | re.S)
    if not m:
        sys.exit(f'the machine showed no value for {name}')
    out = []
    for kind, part in re.findall(r'(document-at|projection)((?:(?!document-at|projection).)*)', m.group(1), re.S):
        lists, cur = [], []
        for byte, nil in re.findall(r'#(\d+)|(nil)', part):
            if nil:
                lists.append(bytes(cur).decode('utf-8')); cur = []
                if len(lists) == 2: break
            else: cur.append(int(byte))
        if len(lists) != 2:
            sys.exit(f'{name}: a {kind} whose two fields are not both shown')
        out.append(('file' if kind == 'document-at' else 'text', lists[0], lists[1]))
    return out


def essential_context():
    """documents (name -> path relative to the repository) and essential-context (node -> names);
    an entry naming a value of type List Document (the root's: onboarding, docs/onboarding.cub) is
    that value's documents, as the machine evaluates it"""
    src = open(GRAPH, encoding='utf-8').read()
    base = os.path.dirname(GRAPH)
    docs = {n: os.path.relpath(os.path.normpath(os.path.join(base, q)), ROOT)
            for n, q in re.findall(r'^([\w-]+) = document "([^"]*)"', src, re.M)}
    ec = {}
    m = re.search(r'^essential-context = (.*)$', src, re.M)
    if m: ec = pair_list(m.group(1))
    for node, names in ec.items():
        expanded = []
        for n in names:
            if n in docs: expanded.append(n)
            else:
                for kind, label, _ in documents(GRAPH, n):
                    docs[label] = label; expanded.append(label)
        ec[node] = expanded
    return docs, ec


def in_progress():
    """the leaves being worked on now (`in-progress`); every other leaf is pending (“The Hyperactive Workflow” (docs/to-be-incorporated-into-paper.md))"""
    src = open(GRAPH, encoding='utf-8').read()
    m = re.search(r'^in-progress = (.*)$', src, re.M)
    return names_in(m.group(1)) if m else []


def resolved():
    """the leaves whose work is done (`resolved`); they stay in the graph, as Avik's rule has it: every
    internal node has a child in progress or complete (“The Hyperactive Workflow” (docs/to-be-incorporated-into-paper.md))"""
    src = open(GRAPH, encoding='utf-8').read()
    m = re.search(r'^resolved = (.*)$', src, re.M)
    return names_in(m.group(1)) if m else []


def kids_of(g):
    k = {n: [] for n in g}
    for n, d in g.items():
        for p in d['parts']:
            k.setdefault(p, []).append(n)
    return k




def first_sentence(t, width=110):
    t = re.sub(r'^\s*', '', t)
    return t if len(t) <= width else t[:width - 1] + '…'


def rel_word(g, parent):
    return 'part of'


def cmd_show(args):
    g, roots, after = graph()
    for a in args:
        d = g.get(a)
        if not d:
            print(f'{a}: no such node\n'); continue
        rels = ', '.join(f'{rel_word(g, p)} {p}' for p in d['parts']) or ('a root' if a in roots else '-')
        print(f'{a}  {rels}')
        print('  ' + d['id'])
        ks = kids_of(g).get(a, [])
        if ks: print('  served by: ' + ' '.join(ks))
        if not ks: print('  state: ' + ('in progress' if a in in_progress() else ('resolved' if a in resolved() else 'pending')))
        docs, ec = essential_context()
        ctx = [(dn, m) for m in sorted(ancestors(g, a), key=lambda m: depth(g, roots, m) or 0) for dn in ec.get(m, [])]
        for dn, m in ctx:
            print(f'  essential context: {docs.get(dn, dn)}' + ('' if m == a else f'  (from {m})'))
        if after.get(a): print('  after: ' + ' '.join(after[a]))
        waits = [m for m, l in after.items() if a in l]
        if waits: print('  before: ' + ' '.join(waits))
        for t in [d['statement']] if d['statement'] else []:
            words, line = t.split(), ''
            for w in words:
                if len(line) + len(w) + 1 > 96:
                    print('    ' + line); line = w
                else:
                    line = (line + ' ' + w).strip()
            if line: print('    ' + line)
        if not d['statement']: print('  (no statement)')
        print()


def under(g, kids, n):
    out, st = set(), [n]
    while st:
        x = st.pop()
        for k in kids.get(x, []):
            if k not in out: out.add(k); st.append(k)
    return out


def cmd_tree(args):
    g, roots, _ = graph()
    kids = kids_of(g)
    ip, rs = in_progress(), resolved()
    seen = set()
    def walk(n, depth, via):
        d = g[n]
        k = kids.get(n, [])
        nt = sum(1 for x in under(g, kids, n) if not kids.get(x) and x not in rs)
        also = [p for p in d['parts'] if p != via]
        print('  ' * depth + f'{d["id"]}  [{n}' + (f'; {nt} open tasks under it]' if k else (', in progress]' if n in ip else (', resolved]' if n in rs else ']')))
              + (f'  (also {", ".join(also)})' if also and depth else ''))
        if n in seen: return
        seen.add(n)
        for c in k:
            if g[c]['parts'] and g[c]['parts'][0] == n:
                walk(c, depth + 1, n)
    for r in roots: walk(r, 0, None)


def one_source_violations(commit=None):
    """Information stored twice (Avik, 2026-11-15: "ONE SOURCE OF INFORMATION"; "Nothing is ever
    allowed to be stale"; "by construction we must ensure no pushes violate this"). Read from the git
    index (what a commit will record) or from a commit's tree (what a push carries). Two rules:
      - no rendered presentation is stored (.pdf, .docx, .epub): a presentation is computed from the
        value it presents (the paper: the value hyperactivity, docs/hyperactivity.cub);
      - no line of prose of 100 characters or more (quotation marks stripped; for .cub files outside
        the code, the text of its string literals: the paper's lines) is held by two files.
    Code (src/, lib/, experiments/, tests/, playground/) is not prose and is not compared."""
    cmd = ['git', 'ls-tree', '-r', '-z', commit] if commit else ['git', 'ls-files', '-s', '-z']
    ls = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True).stdout
    entries = [e.split('\t', 1) for e in ls.split('\0') if e]
    bad, seen = [], {}
    code = ('src/', 'lib/', 'experiments/', 'tests/', 'playground/')
    for meta, path in entries:
        if path.lower().endswith(('.pdf', '.docx', '.epub')):
            bad.append(f'{path}: a stored presentation; it is computed from its value'); continue
        if path.startswith(code) or not path.endswith(('.md', '.txt', '.cub')): continue
        blob = meta.split()[2] if commit else meta.split()[1]
        text = subprocess.run(['git', 'cat-file', 'blob', blob], cwd=ROOT, capture_output=True).stdout.decode('utf-8', 'replace')
        lines = re.findall(r'"([^"\n]*)"', text) if path.endswith('.cub') else text.split('\n')
        for l in lines:
            k = l.strip().lstrip('>').strip()
            if len(k) < 100: continue
            if k in seen and seen[k] != path:
                bad.append(f'{path} and {seen[k]} both hold: {k[:80]}…')
            seen.setdefault(k, path)
    return bad


def paper_code_unresolved():
    """the names the paper's realized constructions give as their code (docs/hyperactivity.cub) that
    are not exactly one definition of lib/: a name that stops naming its code is caught here"""
    defs = {}
    for dp, dn, fs in os.walk(os.path.join(ROOT, 'lib')):
        for f in fs:
            if f.endswith('.cub'):
                src = open(os.path.join(dp, f), encoding='utf-8', errors='replace').read()
                for n in re.findall(r'^(\S+) :', src, re.M) + re.findall(r'^(?:data|codata|record) (\S+)', src, re.M):
                    defs[n] = defs.get(n, 0) + 1
    src = open(os.path.join(ROOT, 'docs', 'hyperactivity.cub'), encoding='utf-8').read()
    names = []
    for line in re.findall(r'^\S+ = realized .*$', src, re.M):
        m = re.search(r'\) ((?:\(cons "[^"]*" )+nil\)+)$', line)
        names += re.findall(r'"([^"]*)"', m.group(1)) if m else ['(unreadable names)']
    return [f'the paper names {n} as code; lib/ has {defs.get(n, 0)} definitions of it' for n in names if defs.get(n, 0) != 1]


def cmd_one_source(args):
    """one-source [COMMIT ...]: the index, or each commit's tree"""
    bad = [v for c in (args or [None]) for v in one_source_violations(c)]
    for b in bad: print(b)
    print('one source: ' + ('every piece of information is stored once' if not bad else f'{len(bad)} violation(s)'))
    sys.exit(1 if bad else 0)


def cmd_check(args):
    g, roots, after = graph()
    bad = 0
    for n, d in g.items():
        if ' ' not in d['id'] or d['id'] == n:
            print(f'{n}: its name must be an English sentence saying what it is, not an identifier'); bad += 1
        if not d['parts'] and n not in roots:
            print(f'{n}: part of nothing and not declared a root'); bad += 1
        if d['parts'] and n in roots:
            print(f'{n}: declared a root but part of {", ".join(d["parts"])}'); bad += 1
        if not d['statement']:
            print(f'{n}: no statement'); bad += 1
    for n, l in after.items():
        for m in [n] + l:
            if m not in g: print(f'{n}: names {m}, which is not a node'); bad += 1
    kids = kids_of(g)
    ip, rs = in_progress(), resolved()
    for n in ip + rs:
        if n not in g: print(f'{n}: named in-progress or resolved, but not a node'); bad += 1
        elif kids.get(n): print(f'{n}: in progress or resolved but has children (a leaf is; a node with children is resolved when they are)'); bad += 1
    for n in set(ip) & set(rs): print(f'{n}: both in progress and resolved'); bad += 1
    for n in g:   # Avik: every internal node has a child in progress or complete below it
        if kids.get(n) and n not in roots and not any(x in ip or x in rs for x in under(g, kids, n)):
            print(f'{n}: expanded, but nothing under it is in progress or resolved (a node is expanded exactly when a child begins)'); bad += 1
    # definitions already checked by the current checker are not checked again: the store keeps them
    # under the content key of their type and code (src/store.c), so a change to any is a new key
    cmd = [os.path.join(ROOT, 'build', 'cubical'), 'task-graph/hyperactivity.cub']
    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    last = [l for l in r.stdout.split('\n') if l.strip()][-1:] or ['']
    fails = [l for l in r.stdout.split('\n') if l.startswith('FAIL')]
    print('machine: ' + last[0])
    for f in fails: print('  ' + f)
    if r.returncode != 0 or fails:
        print(r.stderr.strip()[:2000]); bad += 1
    for v in one_source_violations() + paper_code_unresolved():
        print(v); bad += 1
    print(f'{len(g)} nodes; ' + ('consistent' if not bad else f'{bad} problem(s)'))
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
    """The order of work (“The Hyperactive Workflow” (docs/to-be-incorporated-into-paper.md)). There is no score. A leaf can start when every node its
    `after` names (on it or on any node above it: waiting on a node is waiting on every leaf under it)
    is resolved (listed in `resolved`). Among the leaves that can start now, X supersedes Y when the
    waiting work X is a prerequisite of strictly contains the waiting work Y is a prerequisite of (Avik:
    X and Y both dependencies of Z, X additionally unblocking other goals: X first). Printed in layers:
    no leaf of a layer is superseded by one of a later layer; within a layer, no order is expressed."""
    g, roots, after = graph()
    kids = kids_of(g)
    rs = set(resolved())
    leaves = [n for n in g if not kids.get(n) and n not in rs]
    def leaves_under(n):
        return {x for x in under(g, kids, n) | {n} if not kids.get(x) and x not in rs}
    waits = {x: set().union(*([leaves_under(d) for m in ancestors(g, x) for d in after.get(m, []) if d in g] or [set()]))
             for x in leaves}
    ready = [x for x in leaves if not waits[x]]
    gain = {x: {y for y in leaves if x in waits[y]} for x in ready}   # the waiting work x is a prerequisite of (“The Hyperactive Workflow” (docs/to-be-incorporated-into-paper.md))
    def line(n):
        return f'  {g[n]["id"]}  [{n}]'
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


def main():
    if len(sys.argv) < 2 or sys.argv[1] in ('-h', '--help'):
        print(__doc__); return
    c, a = sys.argv[1], sys.argv[2:]
    {'show': cmd_show, 'tree': cmd_tree, 'check': cmd_check, 'one-source': cmd_one_source, 'priority': cmd_priority, 'why': cmd_why,
     'unexplained': cmd_unexplained, 'view': cmd_view, 'every': cmd_every}.get(c, lambda _: sys.exit('unknown command ' + c))(a)


if __name__ == '__main__':
    main()
