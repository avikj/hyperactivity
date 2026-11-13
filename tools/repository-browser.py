#!/usr/bin/env python3
"""repository-browser — the repository as one graph of reasons, browsed (2026-11-13, read-only).

Every page is a node: a goal or ticket of the task graph, a declaration of the code, a document, or a
file. Every page answers the same questions: why it is here (its chain of reasons up to the root),
what it is (statement, text, code), what serves it (its children, in the order of work), and its
essential context. Files are storage places, not the organization: a file's page lists what is stored
there and each item's reason. What exists without a reason is listed at the root. Everything shown is
read from the repository when asked; nothing is stored here. Started by `hyperactive browse`.
"""
import importlib.util, json, os, re, subprocess, sys, threading, urllib.parse
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location('task_graph', os.path.join(ROOT, 'tools', 'task-graph.py'))
tg = importlib.util.module_from_spec(spec); spec.loader.exec_module(tg)
PAGE = os.path.join(ROOT, 'tools', 'repository-browser.html')
SKIP = {'.git', 'build', '__pycache__', '.cubical-store'}

state = {'reasons': None, 'reasons_error': None}


def compute_reasons():
    try:
        decls, why, stated, g, node_at = tg.reasons()
        state['reasons'] = (decls, why, node_at)
    except Exception as e:  # shown on the page, not hidden
        state['reasons_error'] = repr(e)


def graph_state():
    g, roots, after = tg.graph()
    kids = tg.kids_of(g)
    txt = tg.texts()
    docs, ec = tg.essential_context()
    leaves = [n for n in g if not kids.get(n)]
    def leaves_under(n): return {x for x in tg.under(g, kids, n) | {n} if not kids.get(x)}
    waits = {x: set().union(*([leaves_under(d) for m in tg.ancestors(g, x) for d in after.get(m, []) if d in g] or [set()]))
             for x in leaves}
    ready = [x for x in leaves if not waits[x]]
    gain = {x: {y for y in leaves if waits[y] and waits[y] <= {x}} for x in ready}
    layer, remaining, k = {}, list(ready), 0
    while remaining:
        k += 1
        top = [x for x in remaining if not any(gain[x] < gain[y] for y in remaining)]
        for x in top: layer[x] = k
        remaining = [x for x in remaining if x not in top]
    return dict(g=g, roots=roots, after=after, kids=kids, txt=txt, docs=docs, ec=ec, waits=waits, gain=gain, layer=layer)


def node_text(S, n):
    if tg.is_t(n):
        t = S['txt'].get(int(n[1:]), [])
        return (t[0][3], f'{t[0][0]}:{t[0][1]}', t[0][2]) if t else ('', None, None)
    return (S['g'][n]['statement'], tg.rel(tg.GRAPH), None)


def decl_key(f, l): return f'd:{f}:{l}'


def decl_names(decls, k): return [n for n in decls.get(k, ([], []))[0] if '%' not in n]


def item(S, nid):
    """a short description of a node for lists: id, kind, title, first words, state"""
    if nid.startswith('n:'):
        n = nid[2:]; t, place, date = node_text(S, n)
        st = None
        if n in S['layer']: st = 'can start now' + (' — unblocks ' + ' '.join(sorted(S['gain'][n])) if S['gain'][n] else '')
        elif n in S['waits']: st = 'waits on ' + ' '.join(sorted(S['waits'][n]))
        open_under = sum(1 for x in tg.under(S['g'], S['kids'], n) if tg.is_t(x))
        return dict(id=nid, kind='ticket' if tg.is_t(n) else 'goal', title=n, summary=tg.first_sentence(t, 160),
                    state=st, layer=S['layer'].get(n), open_under=open_under)
    if nid.startswith('d:'):
        f, l = nid[2:].rsplit(':', 1)
        decls = state['reasons'][0] if state['reasons'] else {}
        names = decl_names(decls, (f, int(l)))
        return dict(id=nid, kind='declaration', title=' '.join(names) or '(generated)', summary=f'{f}:{l}')
    if nid.startswith('doc:'):
        return dict(id=nid, kind='document', title=S['docs'].get(nid[4:], nid[4:]), summary=nid[4:])
    if nid.startswith('f:'):
        return dict(id=nid, kind='file', title=nid[2:], summary='')
    return dict(id=nid, kind='?', title=nid, summary='')


def node_parent_ids(S, nid):
    """what nid serves, as ids: graph parents; a declaration's reason (a declaration or a node)"""
    if nid.startswith('n:'):
        return ['n:' + p for p in S['g'][nid[2:]]['parts']]
    if nid.startswith('d:') and state['reasons']:
        decls, why, node_at = state['reasons']
        f, l = nid[2:].rsplit(':', 1); k = (f, int(l))
        if k in why and why[k][1]:
            nk = why[k][1]
            for name, at in node_at.items():
                if at == nk: return ['n:' + name]
            return [decl_key(*nk)]
    return []


def chain(S, nid):
    """the chain of reasons from the root down to nid (the first parent at each step)"""
    out, seen, cur = [], set(), nid
    while cur and cur not in seen:
        seen.add(cur); out.append(cur)
        ps = node_parent_ids(S, cur)
        cur = ps[0] if ps else None
    return [item(S, x) for x in reversed(out)]


def essential(S, nid):
    ctx = []
    for x in [c['id'] for c in chain(S, nid)]:
        if x.startswith('n:'):
            for dn in S['ec'].get(x[2:], []): ctx.append(dict(id='doc:' + dn, path=S['docs'].get(dn, dn), via=x[2:]))
    return ctx


def decl_children(nid):
    """declarations whose reason is nid"""
    if not state['reasons']: return []
    decls, why, node_at = state['reasons']
    if nid.startswith('n:'):
        target = node_at.get(nid[2:])
    else:
        f, l = nid[2:].rsplit(':', 1); target = (f, int(l))
    if not target: return []
    out = [k for k, (how, nxt) in why.items() if nxt == target and k[0] != tg.rel(tg.GRAPH)]
    return [decl_key(*k) for k in sorted(out)]


def source_of(f, l):
    """the declaration's text: from its line to the next column-0 line that starts another declaration"""
    try: ls = open(os.path.join(ROOT, f), encoding='utf-8', errors='replace').read().split('\n')
    except OSError: return ''
    i = l - 1; j = i + 1
    while j < len(ls) and (not ls[j] or ls[j][0] in ' \t' or ls[j].startswith(ls[i].split(' ')[0] + ' ')): j += 1
    while j > i + 1 and not ls[j - 1].strip(): j -= 1
    k = i
    while k > 0 and ls[k - 1].startswith('--'): k -= 1
    return '\n'.join(ls[k:j])


def tickets_at(S, f, l):
    """tickets whose text sits directly above this declaration"""
    out = []
    for n, rows in S['txt'].items():
        for tf, tl, _, _ in rows:
            if tf == f and tl < l and ('t%d' % n) in S['g']:
                try: ls = open(os.path.join(ROOT, f), encoding='utf-8', errors='replace').read().split('\n')
                except OSError: continue
                if all(x.startswith(('--', '/*', ' *', '#')) or not x.strip() for x in ls[tl - 1:l - 1]):
                    out.append('n:t%d' % n)
    return out


def all_files():
    for d, ds, fs in os.walk(ROOT):
        ds[:] = sorted(x for x in ds if x not in SKIP)
        for f in sorted(fs): yield os.path.relpath(os.path.join(d, f), ROOT)


def page(nid):
    S = graph_state()
    p = dict(item=item(S, nid), chain=chain(S, nid), context=essential(S, nid), reasons_ready=state['reasons'] is not None,
             reasons_error=state['reasons_error'])
    if nid.startswith('n:'):
        n = nid[2:]
        if n not in S['g']: return dict(error=f'no node {n}')
        t, place, date = node_text(S, n)
        p.update(text=t, place=place, date=date, after=S['after'].get(n, []))
        kids = S['kids'].get(n, [])
        index = {m: i for i, m in enumerate(S['g'])}
        def soonest(x):   # the earliest layer of the order of work at which anything under x can start
            ls = [S['layer'][y] for y in tg.under(S['g'], S['kids'], x) | {x} if y in S['layer']]
            return min(ls) if ls else 99
        order = lambda x: (soonest(x), index[x])
        p['children'] = [item(S, 'n:' + x) for x in sorted(kids, key=order)]
        p['declarations'] = [item(S, x) for x in decl_children(nid)]
        if n in S['roots']:
            p['unexplained'] = unexplained()
    elif nid.startswith('d:'):
        f, l = nid[2:].rsplit(':', 1); l = int(l)
        p.update(code=source_of(f, l), place=f'{f}:{l}')
        p['declarations'] = [item(S, x) for x in decl_children(nid)]
        p['tickets'] = [item(S, x) for x in tickets_at(S, f, l)]
        if state['reasons']:
            decls = state['reasons'][0]
            uses = decls.get((f, l), ([], []))[1]
            p['uses'] = [item(S, decl_key(*u)) for u in sorted(uses) if u in decls]
            p['used_by'] = [item(S, decl_key(*k)) for k, (_, us) in sorted(decls.items()) if (f, l) in us]
    elif nid.startswith('doc:'):
        dn = nid[4:]; path = S['docs'].get(dn)
        p.update(path=path, context_of=[item(S, 'n:' + n) for n, ds in S['ec'].items() if dn in ds])
        if path and path.endswith('.md'):
            p['markdown'] = open(os.path.join(ROOT, path), encoding='utf-8').read()
    elif nid.startswith('f:'):
        f = nid[2:]
        p['path'] = f
        if state['reasons']:
            decls, why, node_at = state['reasons']
            ks = sorted(k for k in decls if k[0] == f)
            p['stored'] = [dict(item(S, decl_key(*k)), reason=[item(S, x) for x in node_parent_ids(S, decl_key(*k))]) for k in ks]
        p['tickets'] = [item(S, 'n:t%d' % n) for n, rows in S['txt'].items() for tf, _, _, _ in rows if tf == f and ('t%d' % n) in S['g']]
        if not f.endswith('.cub') or not p.get('stored'):
            try: p['raw'] = open(os.path.join(ROOT, f), encoding='utf-8', errors='replace').read()[:400000]
            except OSError: pass
    return p


def unexplained():
    """what exists without a reason: declarations no reason reaches, and files outside the graph"""
    if not state['reasons']: return None
    decls, why, node_at = state['reasons']
    by = {}
    for k in decls:
        if k not in why: by.setdefault(k[0], []).append(k)
    in_graph = {k[0] for k in decls}
    outside = [f for f in all_files() if f not in in_graph and not f.startswith(('archive/', 'docs/'))]
    return dict(declarations=[dict(file=f, count=len(v), items=[dict(id=decl_key(*k), title=' '.join(decl_names(decls, k)) or '(generated)')
                                                                for k in sorted(v)]) for f, v in sorted(by.items())],
                total=sum(len(v) for v in by.values()), of=len(decls), files_outside=outside)


def search(q):
    S = graph_state(); q = q.lower(); out = []
    for n in S['g']:
        t, _, _ = node_text(S, n)
        if q in n.lower() or q in t.lower(): out.append(item(S, 'n:' + n))
    if state['reasons']:
        decls = state['reasons'][0]
        for k, (names, _) in decls.items():
            if any(q == x.lower() or q in x.lower() for x in names if '%' not in x): out.append(dict(id=decl_key(*k), kind='declaration', title=' '.join(decl_names(decls, k)), summary=f'{k[0]}:{k[1]}'))
    for f in all_files():
        if q in f.lower(): out.append(dict(id='f:' + f, kind='file', title=f, summary=''))
    return out[:200]


class H(BaseHTTPRequestHandler):
    def log_message(self, *a): pass
    def send(self, code, body, ctype):
        b = body if isinstance(body, bytes) else body.encode('utf-8')
        self.send_response(code); self.send_header('Content-Type', ctype); self.send_header('Content-Length', str(len(b)))
        self.end_headers(); self.wfile.write(b)
    def do_GET(self):
        u = urllib.parse.urlparse(self.path); q = urllib.parse.parse_qs(u.query)
        try:
            if u.path == '/': return self.send(200, open(PAGE, encoding='utf-8').read(), 'text/html; charset=utf-8')
            if u.path == '/api/page': return self.send(200, json.dumps(page(q.get('id', ['n:' + tg.graph()[1][0]])[0])), 'application/json')
            if u.path == '/api/search': return self.send(200, json.dumps(search(q.get('q', [''])[0])), 'application/json')
            if u.path == '/api/status': return self.send(200, json.dumps(dict(reasons_ready=state['reasons'] is not None, error=state['reasons_error'])), 'application/json')
            if u.path == '/raw':
                f = os.path.normpath(q.get('path', [''])[0])
                if f.startswith('..') or os.path.isabs(f): return self.send(403, 'no', 'text/plain')
                ct = 'application/pdf' if f.endswith('.pdf') else 'text/plain; charset=utf-8'
                return self.send(200, open(os.path.join(ROOT, f), 'rb').read(), ct)
            self.send(404, 'not found', 'text/plain')
        except Exception as e:
            self.send(500, json.dumps(dict(error=repr(e))), 'application/json')


def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8765
    threading.Thread(target=compute_reasons, daemon=True).start()
    srv = ThreadingHTTPServer(('127.0.0.1', port), H)
    print(f'http://127.0.0.1:{port}/', flush=True)
    srv.serve_forever()


if __name__ == '__main__':
    main()
