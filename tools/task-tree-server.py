#!/usr/bin/env python3
"""task-tree-server — the task graph drawn as a graph at http://localhost:8080 (2026-11-13), and the
history of the work at http://localhost:8080/history (tools/history.py, 2026-11-15).

The graph is read from task-graph/hyperactivity.cub on every request (/api/graph); the page
(tools/task-graph-view.html) lays it out in layers from the root, the order of each layer chosen to
reduce crossings, and draws "serves" edges and "waits on" edges. Started by `hyperactive server`."""
import html, importlib.util, json, os, re, sys
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location('tg', os.path.join(ROOT, 'tools', 'task-graph.py'))
tg = importlib.util.module_from_spec(spec); spec.loader.exec_module(tg)
PAGE = os.path.join(ROOT, 'tools', 'task-graph-view.html')
HISTORY_PAGE = os.path.join(ROOT, 'tools', 'history-view.html')
spec = importlib.util.spec_from_file_location('history', os.path.join(ROOT, 'tools', 'history.py'))
history = importlib.util.module_from_spec(spec); spec.loader.exec_module(history)


def graph_json():
    g, roots, after = tg.graph(); kids = tg.kids_of(g)
    docs, ec = tg.essential_context()
    ip = tg.in_progress()
    rs = set(tg.resolved())
    leaves = [n for n in g if not kids.get(n) and n not in rs]
    lu = lambda n: {x for x in tg.under(g, kids, n) | {n} if not kids.get(x) and x not in rs}
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
        known = re.split(r' (?=Known \(\d{4}-\d{2}-\d{2}[,)])', st)
        statement, findings, parts = known[0].strip(), [k.strip() for k in known[1:]], []
        if n in rs:
            state = 'resolved'
        elif n in layer:
            state = 'first' if layer[n] == 1 and gain[n] else ('last' if not gain[n] else 'ready')
        elif n in waits and waits[n]:
            state = 'waiting'
        else:
            state = 'goal'
        ctx = []
        for m in sorted(tg.ancestors(g, n), key=lambda m: len(tg.ancestors(g, m))):
            for dn in ec.get(m, []):
                ctx.append(dict(path=docs.get(dn, dn), via=None if m == n else m))
        questions = [q.strip() for q in re.findall(r'Question for Avik \(\d{4}-\d{2}-\d{2}\):\s*(.*?\?)', st, re.S)]
        nodes.append(dict(id=n, name=d['id'], questions=questions, parents=d['parts'], statement=statement, findings=findings, parts=parts,
                          state=state, progress=('in progress' if n in ip else ('resolved' if n in rs else ('pending' if not kids.get(n) else None))), layer=layer.get(n), unblocks=sorted(gain.get(n, [])),
                          waits=sorted(waits.get(n, [])), after=after.get(n, []), context=ctx,
                          open=sum(1 for x in tg.under(g, kids, n) if not kids.get(x) and x not in rs)))
    return dict(nodes=nodes, roots=roots)


def doc_page(path, text):
    """a document rendered for reading: Markdown's headings, quotes, lists, code and paragraphs; any
    other file as text"""
    e = html.escape
    if not path.endswith('.md'):
        body = f'<pre>{e(text)}</pre>'
    else:
        out, para, quote, lst, code = [], [], [], [], None
        def inline(t):
            t = e(t)
            t = re.sub(r'`([^`]+)`', r'<code>\1</code>', t)
            t = re.sub(r'\*\*([^*]+)\*\*', r'<b>\1</b>', t)
            return re.sub(r'(?<![\w*])\*([^*\n]+)\*(?![\w*])', r'<i>\1</i>', t)
        def flush():
            if para: out.append('<p>' + inline(' '.join(para)) + '</p>'); para.clear()
            if quote:
                paras, cur = [], []
                for q in quote + ['']:
                    if q: cur.append(q)
                    elif cur: paras.append(' '.join(cur)); cur = []
                out.append('<blockquote>' + ''.join(f'<p>{inline(x)}</p>' for x in paras) + '</blockquote>'); quote.clear()
            if lst: out.append('<ul>' + ''.join(f'<li>{inline(x)}</li>' for x in lst) + '</ul>'); lst.clear()
        for line in text.split('\n'):
            if line.startswith('```'):
                flush()
                if code is None: code = []
                else: out.append('<pre>' + e('\n'.join(code)) + '</pre>'); code = None
                continue
            if code is not None: code.append(line); continue
            m = re.match(r'^(#{1,4})\s+(.*)', line)
            if m: flush(); out.append(f'<h{len(m.group(1))}>{inline(m.group(2))}</h{len(m.group(1))}>'); continue
            if line.startswith('>'):
                if para or lst: flush()
                quote.append(line[1:].strip()); continue
            m = re.match(r'^\s*(?:[-*]|\d+\.)\s+(.*)', line)
            if m:
                if para or quote: flush()
                lst.append(m.group(1)); continue
            if line.strip() == '---': flush(); out.append('<hr>'); continue
            if not line.strip(): flush(); continue
            if lst and line.startswith('  '): lst[-1] += ' ' + line.strip(); continue
            if quote: flush()
            para.append(line.strip())
        flush()
        body = ''.join(out)
    return f"""<!doctype html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>{e(path)}</title><style>
body{{margin:0;background:#f6f5f1;color:#1b1a17;font:16px/1.65 ui-sans-serif,-apple-system,"Segoe UI",system-ui,sans-serif}}
main{{max-width:760px;margin:0 auto;padding:40px 22px 90px}} .path{{color:#6f6b62;font:13px ui-monospace,Menlo,monospace}}
h1{{font-size:28px;line-height:1.25}} h2{{font-size:20px;margin-top:36px}} h3{{font-size:16px}}
hr{{border:0;border-top:1px solid #d9d4c8;margin:32px 0}} blockquote{{margin:14px 0;padding:4px 16px;border-left:3px solid #c9c3b6;background:#fff;border-radius:0 8px 8px 0}}
code{{background:#ece9e1;padding:1px 5px;border-radius:4px;font-size:14px}} pre{{background:#fff;border:1px solid #e3dfd5;border-radius:8px;padding:14px;overflow:auto;font-size:13px}}
@media (prefers-color-scheme:dark){{body{{background:#121211;color:#eceae4}} blockquote,pre{{background:#1b1b19;border-color:#2c2b28}} code{{background:#2a2926}}}}
</style></head><body><main><div class="path">{e(path)}</div>{body}</main></body></html>"""


class H(BaseHTTPRequestHandler):
    def log_message(self, *a): pass
    def send(self, body, ctype):
        b = body.encode()
        self.send_response(200); self.send_header('Content-Type', ctype); self.send_header('Content-Length', str(len(b)))
        self.end_headers(); self.wfile.write(b)
    def do_GET(self):
        try:
            if self.path.startswith('/api/graph'): return self.send(json.dumps(graph_json()), 'application/json')
            if self.path.startswith('/api/history'):
                import urllib.parse
                q = {k: v[0] for k, v in urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query).items()}
                return self.send(json.dumps(history.api(q)), 'application/json')
            if self.path.split('?')[0] == '/history': return self.send(open(HISTORY_PAGE, encoding='utf-8').read(), 'text/html; charset=utf-8')
            if self.path.startswith('/doc?'):
                import urllib.parse
                f = os.path.normpath(urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query).get('path', [''])[0])
                full = os.path.join(ROOT, f)
                if f.startswith('..') or os.path.isabs(f) or not os.path.isfile(full): return self.send('no such document', 'text/plain')
                if f.endswith('.pdf'):
                    b = open(full, 'rb').read()
                    self.send_response(200); self.send_header('Content-Type', 'application/pdf'); self.send_header('Content-Length', str(len(b)))
                    self.end_headers(); self.wfile.write(b); return
                return self.send(doc_page(f, open(full, encoding='utf-8', errors='replace').read()), 'text/html; charset=utf-8')
            return self.send(open(PAGE, encoding='utf-8').read(), 'text/html; charset=utf-8')
        except Exception as e:
            return self.send(f'<pre>{html.escape(repr(e))}</pre>', 'text/html; charset=utf-8')


if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    print(f'http://localhost:{port}/', flush=True)
    ThreadingHTTPServer(('127.0.0.1', port), H).serve_forever()
