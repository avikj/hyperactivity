#!/usr/bin/env python3
"""history — the history of the work, read from git into a store (2026-11-15, Avik: "hyperactivity
history should enter history explorer in the product ... inserting zero llm judgement at run-time";
"deeply integrated perfectly fit to the intelligent design of hyperactivity").

Nothing here judges. Every sentence a view shows was written when the work was done (a change's
message, a node's statement, Avik's words); the views only read, group and link it.

The store's form is the machine's (src/store.c): an object is its bytes under the SHA-256 of their
content (`<key>/bytes`), and `names` is a log of `name key time change`, appended when a name is
bound to a new key (`-` unbinds). The machine's own names log has the same first three fields
(src/store.c names_bind; it reads only those), so `#history NAME` reads this store too
(CUBICAL_STORE=build/history-store). Git is the stand-in the store replaces (“State, change and
collaboration” (docs/to-be-incorporated-into-paper.md)), and this reader is its one reader: it reads
every commit reachable from any ref, oldest first, and for each
  - writes the change itself as an object: its git id, its parents' change keys, its times, its
    author and its message (so changes form a graph keyed by content, as commits do);
  - binds every file it adds, alters or removes, the file's path being its name;
  - binds every top-level declaration of a .cub file it alters, under `path#name`, the
    declaration's text (with its statement, the comment above it) being the object: its key moves
    when its code or its statement does; a second declaration of one name in a file is `#name#2`;
  - binds every ref to the change it points to (`ref:` names).
A file at a new path with the same key is a move, a key a name held before is a return to it, a
`-` is a removal: each is read off the log, not recorded as a kind. Merges are read against their
first parent. There is no canonical state: the log interleaves every line of work, and a state is
always the state at a change, accumulated along its first parents.

Run through `hyperactive history`; every view reads what is new in git first.
"""
import datetime, hashlib, json, os, re, subprocess, sys, importlib.util
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
STORE = os.path.join(ROOT, 'build', 'history-store')
GRAPH_FILE = 'task-graph/hyperactivity.cub'
DOCUMENT = ('.md', '.pdf', '.txt')          # writing; every other file is code
_spec = importlib.util.spec_from_file_location('tg', os.path.join(ROOT, 'tools', 'task-graph.py'))
tg = importlib.util.module_from_spec(_spec); _spec.loader.exec_module(tg)


def git(*a, text=True):
    return subprocess.run(['git', '-C', ROOT, *a], capture_output=True, text=text, check=True).stdout


def sha(b):
    return hashlib.sha256(b).hexdigest()


def put(key, data):
    d = os.path.join(STORE, key)
    if not os.path.exists(os.path.join(d, 'bytes')):
        os.makedirs(d, exist_ok=True)
        with open(os.path.join(d, 'bytes.part'), 'wb') as f: f.write(data)
        os.replace(os.path.join(d, 'bytes.part'), os.path.join(d, 'bytes'))


def get(key):
    with open(os.path.join(STORE, key, 'bytes'), 'rb') as f: return f.read()


class Blobs:
    """git's objects by id, read through one `git cat-file --batch`"""
    def __init__(self):
        self.p = subprocess.Popen(['git', '-C', ROOT, 'cat-file', '--batch'], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    def __call__(self, oid):
        self.p.stdin.write(oid.encode() + b'\n'); self.p.stdin.flush()
        size = int(self.p.stdout.readline().split()[2])
        data = self.p.stdout.read(size); self.p.stdout.read(1)
        return data


def decls(data):
    """a .cub file's declarations: name -> text (bytes), a repeated name numbered #2, #3, …"""
    out = {}
    for name, _, text in tg.declarations(data.decode('utf-8', 'surrogateescape')):
        k, n = name, 1
        while k in out: n += 1; k = f'{name}#{n}'
        out[k] = text.encode('utf-8', 'surrogateescape')
    return out


# ---------------------------------------------------------------- the reader
def read():
    """read every commit not yet read into the store; return nothing"""
    os.makedirs(STORE, exist_ok=True)
    done = dict(l.split() for l in open(os.path.join(STORE, 'read')) if l.strip()) if os.path.exists(os.path.join(STORE, 'read')) else {}
    ids = git('rev-list', '--all', '--date-order', '--reverse').split()
    new = [c for c in ids if c not in done]
    refs = [l.split() for l in git('for-each-ref', '--format=%(objectname) %(refname)').split('\n') if l.strip()]
    if not new and all(done.get(o) for o, _ in refs) and os.path.exists(os.path.join(STORE, 'refs')) \
            and open(os.path.join(STORE, 'refs')).read() == json.dumps(refs): return
    blob, parsed = Blobs(), {}
    def decls_of(oid):
        if oid not in parsed:
            if len(parsed) > 64: parsed.clear()
            parsed[oid] = decls(blob(oid))
        return parsed[oid]
    names = open(os.path.join(STORE, 'names'), 'a', encoding='utf-8')
    readlog = open(os.path.join(STORE, 'read'), 'a', encoding='utf-8')
    for c in new:
        parents, by, written, at, msg = git('show', '-s', '--format=%P%x00%an%x00%at%x00%ct%x00%B', c).split('\0', 4)
        parents, msg = parents.split(), msg.rstrip('\n')
        record = (f'change\ngit {c}\n' + ''.join(f'parent {done[p]}\n' for p in parents)
                  + f'time {at}\nwritten {written}\nby {by}\n\n{msg}\n').encode('utf-8', 'surrogateescape')
        key = sha(record); put(key, record)
        diff = git('diff-tree', '-r', '-z', '--no-renames', '--no-commit-id', '--root', *( [parents[0]] if parents else []), c).split('\0')
        lines = []
        def bind(name, k): lines.append(f'{name.replace(" ", "%20")} {k} {at} {key}\n')
        i = 0
        while i < len(diff) - 1:
            meta, path = diff[i], diff[i + 1]; i += 2
            if not meta.startswith(':'): continue
            m0, m1, o0, o1, st = meta[1:].split()
            if m1 == '160000' or (m0 == '160000' and st == 'D'): continue
            old = decls_of(o0) if path.endswith('.cub') and st != 'A' and m0 != '160000' else {}
            if st == 'D':
                bind(path, '-')
                for n in old: bind(f'{path}#{n}', '-')
                continue
            data = blob(o1); k = sha(data); put(k, data); bind(path, k)
            if path.endswith('.cub'):
                now = decls_of(o1)
                for n, t in now.items():
                    if old.get(n) != t: tk = sha(t); put(tk, t); bind(f'{path}#{n}', tk)
                for n in old:
                    if n not in now: bind(f'{path}#{n}', '-')
        names.writelines(lines); names.flush()
        readlog.write(f'{c} {key}\n'); readlog.flush()
        done[c] = key
    for o, r in refs:
        if r.endswith('/HEAD'): continue
        names.write(f'ref:{r.replace("refs/remotes/origin/", "").replace("refs/heads/", "")} {done[o]} - -\n')
    names.close(); readlog.close()
    open(os.path.join(STORE, 'refs'), 'w').write(json.dumps(refs))


# ---------------------------------------------------------------- what the store holds
class History:
    """the store as the views read it: changes, bindings by change and by name, refs"""
    def __init__(self):
        read()
        self.by_git = dict(l.split() for l in open(os.path.join(STORE, 'read')) if l.strip())
        self.changes = {}
        for g, k in self.by_git.items():
            hd, _, msg = get(k).decode('utf-8', 'surrogateescape').partition('\n\n')
            f = defaultdict(list)
            for l in hd.split('\n')[1:]:
                a, _, b = l.partition(' '); f[a].append(b)
            s = re.search(r'^Claude-Session: (\S+)', msg, re.M)
            self.changes[k] = dict(key=k, git=g, parents=f['parent'], time=int(f['time'][0]), written=int(f['written'][0]),
                                   by=f['by'][0], message=msg.rstrip('\n'), subject=msg.split('\n', 1)[0],
                                   session=s.group(1).rsplit('_', 1)[-1] if s else None, bindings=[])
        self.order = sorted(self.changes.values(), key=lambda c: (c['time'], c['git']))
        self.names, self.refs = defaultdict(list), {}
        for l in open(os.path.join(STORE, 'names'), encoding='utf-8'):
            n, k, t, c = l.split()
            n = n.replace('%20', ' ')
            if n.startswith('ref:'): self.refs[n[4:]] = k; continue
            if c not in self.changes: continue          # a change whose reading did not complete
            b = (n, k, c); self.names[n].append(b); self.changes[c]['bindings'].append(b)
        self.first_seen = {}
        for c in self.order:
            for n, k, _ in c['bindings']:
                if k != '-': self.first_seen.setdefault(k, (n, c['key']))
        self.ancestors = {}
        for k in self.by_git.values():                  # the read log is in an order parents come first
            a = set()
            for p in self.changes[k]['parents']: a |= self.ancestors[p] | {p}
            self.ancestors[k] = frozenset(a)
        self._states, self._lanes, self._decls, self._graphs, self._nodes, self._isnode = {}, None, {}, {}, {}, {}

    def decls(self, fk):
        """the declarations of a file's version, by its key"""
        if fk not in self._decls:
            if len(self._decls) > 512: self._decls.clear()
            self._decls[fk] = decls(get(fk))
        return self._decls[fk]

    def on_line(self, a, b):
        """a is b's first parent or one of its ancestors: a came before b on b's own line of work"""
        p = self.changes[b]['parents']
        return bool(p) and (a == p[0] or a in self.ancestors[p[0]])

    def find(self, x):
        """a change by git id, change key or a prefix of either"""
        hits = [c for c in self.changes.values() if c['git'].startswith(x) or c['key'].startswith(x)]
        if len(hits) != 1: raise LookupError(f'{x} names {len(hits)} changes')
        return hits[0]

    def state(self, key):
        """the files at a change: path -> key, accumulated along first parents"""
        chain = []
        while key and key not in self._states:
            chain.append(key); p = self.changes[key]['parents']; key = p[0] if p else None
        s = dict(self._states[key]) if key else {}
        for k in reversed(chain):
            for n, v, _ in self.changes[k]['bindings']:
                if '#' in n: continue
                if v == '-': s.pop(n, None)
                else: s[n] = v
            self._states[k] = dict(s)
        return self._states[chain[0]] if chain else self._states[key]

    def lanes(self):
        """change key -> the refs it is reachable from (main first)"""
        if self._lanes is None:
            self._lanes = defaultdict(list)
            for r in sorted(self.refs, key=lambda r: (r != 'main', r)):
                stack, seen = [self.refs[r]], set()
                while stack:
                    k = stack.pop()
                    if k in seen: continue
                    seen.add(k); self._lanes[k].append(r); stack.extend(self.changes[k]['parents'])
        return self._lanes

    def previous(self, name, change):
        """the key a name held in the first-parent state before a change"""
        p = self.changes[change]['parents']
        if not p: return None
        if '#' not in name: return self.state(p[0]).get(name)
        path, _, d = name.partition('#')
        fk = self.state(p[0]).get(path)
        if not fk: return None
        t = self.decls(fk).get(d)
        return sha(t) if t is not None else None


def code_lines(k):
    return '\n'.join(l for l in text_of(k).split('\n') if not l.startswith('--'))


def decl_change(h, n, k, change):
    """how a binding of a declaration changed it: added, removed, code (its code's text moved) or
    statement (only the comment above it did)"""
    if k == '-': return 'removed'
    p = h.previous(n, change)
    if p is None: return 'added'
    return 'statement' if code_lines(p) == code_lines(k) else 'code'


def utc(t, fmt='%Y-%m-%d %H:%M'):
    return datetime.datetime.fromtimestamp(t, datetime.timezone.utc).strftime(fmt)


def text_of(key):
    return get(key).decode('utf-8', 'replace')


# ---------------------------------------------------------------- the task graph through time
def graph_at(h, change):
    """the task graph as it stood at a change: (nodes, in-progress, resolved, path -> nodes it is
    essential context for), or None before the graph existed"""
    k = h.state(change).get(GRAPH_FILE)
    if not k: return None
    if k in h._graphs: return h._graphs[k]
    src = text_of(k)
    g, _, _ = tg.graph(src)
    lst = lambda n: (tg.names_in(m.group(1)) if (m := re.search(r'^' + n + r' = (.*)$', src, re.M)) else [])
    docs = {n: os.path.normpath(os.path.join('task-graph', q)) for n, q in re.findall(r'^([\w-]+) = document "([^"]*)"', src, re.M)}
    m = re.search(r'^essential-context = (.*)$', src, re.M)
    serves = defaultdict(list)
    for node, ds in (tg.pair_list(m.group(1)) if m else {}).items():
        for d in ds:
            if d in docs: serves[docs[d]].append(node)
    h._graphs[k] = (g, lst('in-progress'), lst('resolved'), serves)
    return h._graphs[k]


NODE_SIG = re.compile(r'^[\w-]+ : (Node|Ticket)\b', re.M)
NODE_DECL = re.compile(r'^[\w-]+ = node "([^"]*)"', re.M)


def is_node(h, k):
    """a declaration of the work's own type for tasks: Node (lib/task-graph.cub, and lib/work.cub
    before it, 2026-11-12) or Ticket (lib/work.cub's tickets)"""
    if k not in h._isnode: h._isnode[k] = bool(NODE_SIG.search(text_of(k)))
    return h._isnode[k]


def nodes_at(h, key):
    """the work's nodes at a change: path#name -> key, along first parents, wherever they are declared"""
    chain = []
    while key and key not in h._nodes:
        chain.append(key); p = h.changes[key]['parents']; key = p[0] if p else None
    s = dict(h._nodes[key]) if key else {}
    for k in reversed(chain):
        for n, v, _ in h.changes[k]['bindings']:
            if '#' not in n: continue
            if v != '-' and is_node(h, v): s[n] = v
            else: s.pop(n, None)
        h._nodes[k] = dict(s)
    return h._nodes[chain[0]] if chain else h._nodes[key]


def sentence(k):
    """a node's sentence (its name, in the task graph), or the first line of its statement"""
    t = text_of(k)
    m = NODE_DECL.search(t)
    if m: return m.group(1)
    first = next((l[2:].strip() for l in t.split('\n') if l.startswith('--')), '')
    return re.sub(r'^[\w-]+ \([^)]*\):\s*', '', first)


def node_events(h, c):
    """what a change did to the work's nodes: added, removed, renamed (a new sentence), restated (its
    statement changed); in the task graph, leaves set in progress or resolved"""
    out = []
    if not any('#' in b[0] and (b[1] != '-' and is_node(h, b[1]) or b[0] in nodes_at(h, c['parents'][0]) if c['parents'] else True)
               for b in c['bindings']): return out
    ident = lambda m: {n.split('#')[1]: k for n, k in m.items()}
    a1 = ident(nodes_at(h, c['key'])); a0 = ident(nodes_at(h, c['parents'][0])) if c['parents'] else {}
    for n, k in a1.items():
        if n not in a0: out.append(('node added', n, sentence(k)))
        elif k != a0[n]:
            renamed = NODE_DECL.search(text_of(k)) and NODE_DECL.search(text_of(a0[n])) and sentence(k) != sentence(a0[n])
            out.append(('node renamed' if renamed else 'statement', n, sentence(k)))
    out += [('node removed', n, sentence(a0[n])) for n in a0 if n not in a1]
    if any(b[0].startswith(GRAPH_FILE) for b in c['bindings']):
        g1 = graph_at(h, c['key']); g0 = graph_at(h, c['parents'][0]) if c['parents'] else None
        ip1, rs1 = (g1[1], g1[2]) if g1 else ([], []); ip0, rs0 = (g0[1], g0[2]) if g0 else ([], [])
        out += [('in progress', n, sentence(a1[n]) if n in a1 else '') for n in ip1 if n not in ip0]
        out += [('resolved', n, sentence(a1[n]) if n in a1 else '') for n in rs1 if n not in rs0]
    return out


def events(h, c):
    """a change's organization events, read off its bindings: namespaces (a path's first part)
    appearing and disappearing, documents added, moved, removed, the task graph; and counts of code"""
    ev, files = [], [b for b in c['bindings'] if '#' not in b[0]]
    before = h.state(c['parents'][0]) if c['parents'] else {}
    after = h.state(c['key'])
    top = lambda s: {p.split('/')[0] for p in s if '/' in p}
    ev += [('namespace added', t, '') for t in sorted(top(after) - top(before))]
    ev += [('namespace removed', t, '') for t in sorted(top(before) - top(after))]
    gone = {before[n]: n for n, k, _ in files if k == '-' and n in before}
    added = {k for n, k, _ in files if k != '-'}
    for n, k, _ in files:
        if not n.endswith(DOCUMENT): continue
        if k == '-':
            if before.get(n) not in added: ev.append(('document removed', n, ''))
        elif n not in before:
            ev.append(('document moved', n, gone[k]) if k in gone else ('document added', n, ''))
        else: ev.append(('document changed', n, ''))
    moves = [e for e in ev if e[0] == 'document moved']
    groups = defaultdict(list)
    for _, new, old in moves:
        a, b = old.split('/'), new.split('/')
        k = 0
        while k < min(len(a), len(b)) - 1 and a[-1 - k] == b[-1 - k]: k += 1
        groups[('/'.join(a[:len(a) - k]), '/'.join(b[:len(b) - k]))].append(new)
    ev = [e for e in ev if e[0] != 'document moved'] + [
        ('document moved', ps[0], o) if len(ps) == 1 else ('documents moved', f'{n}/', f'{o}/ ({len(ps)})')
        for (o, n), ps in groups.items()]
    for kind, verb in (('namespace added', 'document added'), ('namespace removed', 'document removed')):
        for _, t, _ in [e for e in ev if e[0] == kind]:
            inside = [e for e in ev if e[0] == verb and e[1].startswith(t + '/')]
            if inside:
                ev = [e for e in ev if e not in inside]
                ev[ev.index((kind, t, ''))] = (kind, t, f'{len(inside)} document' + ('s' if len(inside) > 1 else ''))
    ev += node_events(h, c)
    code = defaultdict(lambda: [0, 0, 0, 0])
    for n, k, _ in c['bindings']:
        if '#' not in n or n.startswith(GRAPH_FILE + '#'): continue
        code[n.split('#')[0]][('added', 'code', 'statement', 'removed').index(decl_change(h, n, k, c['key']))] += 1
    return ev, dict(code)


def returns(h, c):
    """the bindings of a change that give a name back a key it held before (not the one it had)"""
    out = 0
    for n, k, ck in c['bindings']:
        if k == '-': continue
        earlier = [b for b in h.names[n] if h.on_line(b[2], ck) and b[1] == k]
        if earlier and h.previous(n, ck) != k: out += 1
    return out


# ---------------------------------------------------------------- views
def lane_label(h, c):
    ls = h.lanes().get(c['key'], [])
    return '' if 'main' in ls else ('  [' + ', '.join(ls) + ']' if ls else '  [no ref]')


def who(c):
    """the author as the work knows them: Avik (the one human, under each of his git names), Claude,
    or a merge"""
    if len(c['parents']) > 1: return 'merge'
    return 'Avik' if c['by'] in ('Avik Jain', 'avikj') else c['by']


def view_days(h):
    """the whole history, a day at a time (UTC): how many changes, by whom, on which lines of work;
    then every change that moved the organization (namespaces, documents, the task graph), with its
    message's first line; then code declarations added, changed and removed per namespace"""
    days = defaultdict(list)
    for c in h.order: days[utc(c['time'], '%Y-%m-%d')].append(c)
    out = []
    for d, cs in days.items():
        by = defaultdict(int)
        for c in cs: by[who(c)] += 1
        sessions = sorted({c['session'] for c in cs if c['session']})
        out.append(f'{d}  {len(cs)} changes (' + ', '.join(f'{v} {k}' for k, v in sorted(by.items(), key=lambda x: -x[1]))
                   + ')' + (f', {len(sessions)} session' + ('s' if len(sessions) > 1 else '') if sessions else ''))
        code = defaultdict(lambda: [0, 0, 0, 0])
        for c in cs:
            ev, cd = events(h, c)
            for p, v in cd.items():
                t = p.split('/')[0] if '/' in p else '(top)'
                for i in range(4): code[t][i] += v[i]
            ev = [e for e in ev if e[0] not in ('statement', 'document changed')]
            if ev:
                out.append(f'  {utc(c["time"], "%H:%M")} {c["git"][:7]} {who(c)}: {c["subject"]}{lane_label(h, c)}')
                out += ['        ' + l for l in shown(ev)]
        if code:
            out.append('  declarations (added, code changed, statement changed, removed): '
                       + '; '.join(f'{t} +{a} ~{m} ·{s} -{r}' for t, (a, m, s, r) in sorted(code.items(), key=lambda x: -sum(x[1]))))
        out.append('')
    return '\n'.join(out)


def shown(ev):
    """a change's events as lines: more than six of one kind of node event in one change are one line
    of their identifiers"""
    out, kinds, done = [], defaultdict(list), set()
    for e in ev: kinds[e[0]].append(e)
    for e in ev:
        if e[0].startswith('node') and len(kinds[e[0]]) > 6:
            if e[0] not in done:
                done.add(e[0]); out.append(f'{e[0].replace("node", "nodes")} ({len(kinds[e[0]])}): ' + ', '.join(x[1] for x in kinds[e[0]]))
        else: out.append(show_event(e))
    return out


def show_event(e):
    kind, a, b = e
    if kind in ('namespace added', 'namespace removed'): return f'{kind}: {a}/' + (f' ({b})' if b else '')
    if kind in ('document moved', 'documents moved'): return f'{kind}: {b.split(" ")[0]} → {a}' + (' ' + b.split(' ', 1)[1] if ' ' in b else '')
    if kind.startswith('document'): return f'{kind}: {a}'
    return f'{kind}: {a}' + (f' — "{b}"' if b else '')


def view_day(h, day):
    """every change of one day (UTC), with its events and its code"""
    out = []
    for c in h.order:
        if utc(c['time'], '%Y-%m-%d') != day: continue
        ev, cd = events(h, c)
        out.append(f'{utc(c["time"], "%H:%M")} {c["git"][:7]} {who(c)}: {c["subject"]}{lane_label(h, c)}')
        out += ['      ' + l for l in shown(ev)]
        if cd: out.append('      declarations: ' + '; '.join(f'{p} +{a} ~{m} ·{s} -{r}' for p, (a, m, s, r) in sorted(cd.items())))
    return '\n'.join(out) or f'no changes on {day}'


def view_change(h, x):
    """one change: who, when, where it sits among the lines of work, its whole message, and what it
    bound: the task graph, documents, then code by file, each file with the nodes it is essential
    context for in the graph as it then stood"""
    c = h.find(x)
    out = [f'change {c["key"]}', f'git {c["git"]}', f'{utc(c["time"])} UTC by {c["by"]}'
           + (f' (written {utc(c["written"])} UTC)' if c['written'] != c['time'] else ''),
           'parents: ' + (', '.join(h.changes[p]['git'][:7] + ' ' + h.changes[p]['subject'] for p in c['parents']) or 'none'),
           'on: ' + (', '.join(h.lanes().get(c['key'], [])) or 'no ref'), '', c['message'], '']
    ev, cd = events(h, c)
    out += shown(ev)
    g = graph_at(h, c['key'])
    serves = g[3] if g else {}
    files = [b for b in c['bindings'] if '#' not in b[0] and not b[0].endswith(DOCUMENT)]
    for n, k, _ in files:
        d = [(m.split('#', 1)[1], v) for m, v, _ in c['bindings'] if m.startswith(n + '#')]
        line = f'{n}: ' + ('removed' if k == '-' else ('added' if h.previous(n, c['key']) is None else 'changed'))
        if serves.get(n): line += '  (essential context of ' + ', '.join(serves[n]) + ')'
        out.append(line)
        if d and n != GRAPH_FILE:
            by = defaultdict(list)
            for m, v in d: by[decl_change(h, f'{n}#{m}', v, c['key'])].append(m)
            out += [f'    {kind}: ' + ' '.join(ms) for kind, ms in by.items()]
    r = returns(h, c)
    if r: out.append(f'{r} binding' + ('s' if r > 1 else '') + ' give a name back a key it held before')
    return '\n'.join(out)


def matching(h, x):
    if x in h.names: return [x]
    return sorted(n for n in h.names if n.endswith('#' + x) or n.startswith(x.rstrip('/') + '/') and '#' not in n) \
        or sorted(n for n in h.names if x in n)


def view_of(h, x):
    """what changed here: every version of each name matching x (a path, path#declaration, a
    declaration's name, a namespace), when, by which change; a key first seen under another name is
    a move, a key the name held before a return; and, for a declaration, the declarations that use
    it where it now stands (the other sides of its squares)"""
    ns = matching(h, x)
    if not ns: return f'no name matches {x}'
    out = []
    for n in ns[:200]:
        out.append(n)
        held = []
        for _, k, ck in h.names[n]:
            c = h.changes[ck]
            note, before = '', [x for x, kk in held if kk == k]
            if k == '-': note = 'removed'
            elif before: note = 'returns to an earlier version' if any(h.on_line(x, ck) for x in before) else 'brought from another line of work'
            elif h.first_seen.get(k, (n,))[0] != n: note = 'moved from ' + h.first_seen[k][0]
            elif '#' in n and decl_change(h, n, k, ck) == 'statement': note = 'statement changed, code unchanged'
            if k != '-': held.append((ck, k))
            out.append(f'  {utc(c["time"])} {c["git"][:7]} {k[:12] if k != "-" else "-":12} {who(c)}: {c["subject"]}'
                       + (f'  ({note})' if note else '') + lane_label(h, c))
        if '#' in n and h.names[n][-1][1] != '-':
            word = re.compile(r"(?<![\w'-])" + re.escape(n.split('#')[1]) + r"(?![\w'-])")
            last = h.names[n][-1][2]
            users = []
            for p, fk in sorted(h.state(last).items()):
                if not p.endswith('.cub'): continue
                for m, t in h.decls(fk).items():
                    if f'{p}#{m}' != n and word.search('\n'.join(l for l in t.decode('utf-8', 'replace').split('\n') if not l.startswith('--'))):
                        users.append(f'{p}#{m}')
            out.append(f'  used by {len(users)} declaration' + ('s' if len(users) != 1 else '') + ' where it now stands' + (':' if users else ''))
            out += ['    ' + u for u in users[:40]] + ([f'    … and {len(users) - 40} more'] if len(users) > 40 else [])
    if len(ns) > 200: out.append(f'… and {len(ns) - 200} more names')
    return '\n'.join(out)


def view_node(h, x):
    """a node of the task graph through time: its sentence and statement as they changed, when it was
    set in progress and resolved, and every change to the documents essential to it"""
    name = f'{GRAPH_FILE}#{x}'
    if name not in h.names:
        said = lambda n: ' '.join(sentence(k) for _, k, _ in h.names[n] if k != '-')
        hits = [n for n in h.names if n.startswith(GRAPH_FILE + '#') and NODE_SIG.search(text_of(h.names[n][-1][1]) if h.names[n][-1][1] != '-' else '') and x.lower() in said(n).lower()]
        if len(hits) != 1: return f'{x}: ' + ('names no node' if not hits else 'names ' + ', '.join(n.split('#')[1] for n in sorted(hits)))
        name = hits[0]
    node = name.split('#')[1]
    out, said = [node], None
    for _, k, ck in h.names[name]:
        c = h.changes[ck]
        if k == '-': out.append(f'  {utc(c["time"])} {c["git"][:7]} removed: {c["subject"]}'); said = None; continue
        s = sentence(k)
        out.append(f'  {utc(c["time"])} {c["git"][:7]} ' + (f'"{s}"' if s != said else 'statement changed') + f': {c["subject"]}')
        said = s
    for c in h.order:
        for kind, n, _ in node_events(h, c):
            if n == node and kind in ('in progress', 'resolved'):
                out.append(f'  {utc(c["time"])} {c["git"][:7]} {kind}: {c["subject"]}')
    out.append('  changes to its essential context:')
    for c in h.order:
        g = graph_at(h, c['key'])
        if not g: continue
        hit = sorted({b[0] for b in c['bindings'] if '#' not in b[0] and node in g[3].get(b[0], [])})
        if hit: out.append(f'    {utc(c["time"])} {c["git"][:7]} {c["subject"]}  ({", ".join(hit)})')
    return '\n'.join(out)


WORDS = re.compile(r'^#### (\d{4}-\d{2}-\d{2}[^\n]*)', re.M)


def view_words(h):
    """Avik's words, verbatim, as they entered the work: each dated message (a `#### date` heading of a
    document), the change that first recorded it, and its first line"""
    seen, out = set(), []
    for c in h.order:
        for n, k, _ in c['bindings']:
            if k == '-' or not n.endswith('.md'): continue
            t = text_of(k)
            for m in WORDS.finditer(t):
                body = t[m.end():].lstrip('\n')
                first = next((l[2:].strip() for l in body.split('\n') if l.startswith('> ') and l[2:].strip()), '')
                if (m.group(1), first[:80]) in seen: continue
                seen.add((m.group(1), first[:80]))
                out.append(f'{m.group(1)}  recorded {utc(c["time"])} {c["git"][:7]} in {n}\n    {first[:160]}')
    return '\n'.join(out)


# ---------------------------------------------------------------- for the web page
def api(h, q):
    try: return api_view(h, q)
    except LookupError as e: return {'text': str(e)}


def api_view(h, q):
    if 'day' in q: return {'text': view_day(h, q['day'])}
    if 'change' in q: return {'text': view_change(h, q['change'])}
    if 'of' in q: return {'text': view_of(h, q['of'])}
    if 'node' in q: return {'text': view_node(h, q['node'])}
    if 'words' in q: return {'text': view_words(h)}
    days = defaultdict(list)
    for c in h.order:
        ev, cd = events(h, c)
        days[utc(c['time'], '%Y-%m-%d')].append(dict(
            git=c['git'][:7], time=utc(c['time'], '%H:%M'), by=who(c), subject=c['subject'], session=c['session'],
            lanes=h.lanes().get(c['key'], []), events=shown([e for e in ev if e[0] not in ('statement', 'document changed')]),
            code=sum(sum(v) for v in cd.values())))
    return {'days': [{'day': d, 'changes': cs} for d, cs in days.items()]}


USAGE = """hyperactive history                 the whole history, a day at a time (UTC)
hyperactive history DAY             every change of a day (2026-11-13)
hyperactive history change ID       one change (git id or change key, or a prefix)
hyperactive history of NAME         what changed here: every version of a name (path, path#declaration,
                                    a declaration, a namespace), moves, returns, and what uses it
hyperactive history node NODE       a task-graph node through time (its identifier, or words of it)
hyperactive history words           Avik's words as they entered the work"""


def main():
    a = sys.argv[1:]
    if a and a[0] in ('help', '-h', '--help'): print(USAGE); return
    h = History()
    if not a: print(view_days(h))
    elif re.match(r'\d{4}-\d{2}-\d{2}$', a[0]): print(view_day(h, a[0]))
    elif a[0] == 'change' and len(a) == 2:
        try: print(view_change(h, a[1]))
        except LookupError as e: sys.exit(f'history: {e}')
    elif a[0] == 'of' and len(a) >= 2: print(view_of(h, ' '.join(a[1:])))
    elif a[0] == 'node' and len(a) >= 2: print(view_node(h, ' '.join(a[1:])))
    elif a == ['words']: print(view_words(h))
    else: sys.exit(USAGE)


if __name__ == '__main__':
    import signal; signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    main()
