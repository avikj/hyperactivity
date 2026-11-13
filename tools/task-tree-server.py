#!/usr/bin/env python3
"""task-tree-server — the task graph drawn as a graph at http://localhost:8080 (2026-11-13).

The graph is read from task-graph/hyperactivity.cub on every request (/api/graph); the page
(tools/task-graph-view.html) lays it out in layers from the root, the order of each layer chosen to
reduce crossings, and draws "serves" edges and "waits on" edges. Started by `hyperactive server`."""
import html, importlib.util, json, os, re, sys
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location('tg', os.path.join(ROOT, 'tools', 'task-graph.py'))
tg = importlib.util.module_from_spec(spec); spec.loader.exec_module(tg)
PAGE = os.path.join(ROOT, 'tools', 'task-graph-view.html')


def graph_json():
    g, roots, after = tg.graph(); kids = tg.kids_of(g)
    docs, ec = tg.essential_context()
    leaves = [n for n in g if not kids.get(n)]
    lu = lambda n: {x for x in tg.under(g, kids, n) | {n} if not kids.get(x)}
    waits = {x: set().union(*([lu(d) for m in tg.ancestors(g, x) for d in after.get(m, []) if d in g] or [set()])) for x in leaves}
    ready = [x for x in leaves if not waits[x]]
    gain = {x: {y for y in leaves if x in waits[y]} for x in ready}
    layer, rem, k = {}, list(ready), 0
    while rem:
        k += 1
        top = [x for x in rem if not any(gain[x] < gain[y] for y in rem)]
        for x in top: layer[x] = k
        rem = [x for x in rem if x not in top]
    last = max(layer.values(), default=0)
    nodes = []
    for n, d in g.items():
        st = d['statement']
        st = re.sub(r'^[\w-]+ \([^)]*\):\s*', '', st)
        known = re.split(r'(?= Known \(\d{4}-\d{2}-\d{2}, at )', st)
        statement, findings = known[0].strip(), [k.strip() for k in known[1:]]
        if n in layer:
            state = 'first' if layer[n] == 1 and gain[n] else ('last' if not gain[n] else 'ready')
        elif n in waits and waits[n]:
            state = 'waiting'
        else:
            state = 'goal'
        ctx = []
        for m in sorted(tg.ancestors(g, n), key=lambda m: len(tg.ancestors(g, m))):
            for dn in ec.get(m, []):
                ctx.append(dict(path=docs.get(dn, dn), via=None if m == n else m))
        nodes.append(dict(id=n, name=d['id'], parents=d['parts'], statement=statement, findings=findings,
                          state=state, layer=layer.get(n), unblocks=sorted(gain.get(n, [])),
                          waits=sorted(waits.get(n, [])), after=after.get(n, []), context=ctx,
                          open=sum(1 for x in tg.under(g, kids, n) if not kids.get(x))))
    return dict(nodes=nodes, roots=roots)


class H(BaseHTTPRequestHandler):
    def log_message(self, *a): pass
    def send(self, body, ctype):
        b = body.encode()
        self.send_response(200); self.send_header('Content-Type', ctype); self.send_header('Content-Length', str(len(b)))
        self.end_headers(); self.wfile.write(b)
    def do_GET(self):
        try:
            if self.path.startswith('/api/graph'): return self.send(json.dumps(graph_json()), 'application/json')
            return self.send(open(PAGE, encoding='utf-8').read(), 'text/html; charset=utf-8')
        except Exception as e:
            return self.send(f'<pre>{html.escape(repr(e))}</pre>', 'text/html; charset=utf-8')


if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    print(f'http://localhost:{port}/', flush=True)
    ThreadingHTTPServer(('127.0.0.1', port), H).serve_forever()
