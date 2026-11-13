#!/usr/bin/env python3
"""task-tree-server — the whole task graph rendered as one tree at http://localhost:8080 (2026-11-13).
Read from task-graph/hyperactivity.cub on every page load. Started by `hyperactive server`."""
import html, importlib.util, json, os, re, sys
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location('tg', os.path.join(ROOT, 'tools', 'task-graph.py'))
tg = importlib.util.module_from_spec(spec); spec.loader.exec_module(tg)


def page():
    g, roots, after = tg.graph(); kids = tg.kids_of(g); txt = tg.texts()
    leaves = [n for n in g if not kids.get(n)]
    lu = lambda n: {x for x in tg.under(g, kids, n) | {n} if not kids.get(x)}
    waits = {x: set().union(*([lu(d) for m in tg.ancestors(g, x) for d in after.get(m, []) if d in g] or [set()])) for x in leaves}
    ready = [x for x in leaves if not waits[x]]
    gain = {x: {y for y in leaves if x in waits[y]} for x in ready}   # the waiting work x is a prerequisite of (docs/task-graph-design.md §5)
    def text(n):
        if tg.is_t(n):
            r = txt.get(int(n[1:]), [])
            return (r[0][3], f'{r[0][0]}:{r[0][1]}') if r else ('', '')
        return (re.sub(r'^[\w-]+ \([^)]*\):\s*', '', g[n]['statement']), '')
    shown = set()
    def node(n, parent):
        t, site = text(n)
        others = [p for p in g[n]['parts'] if p != parent]
        badge = ''
        if n in gain: badge = '<span class="now">can start now' + (f' — a prerequisite of {len(gain[n])} waiting' if gain[n] else '') + '</span>'
        elif waits.get(n): badge = f'<span class="wait">waits on {" ".join(sorted(waits[n]))}</span>'
        also = f'<span class="also">also under: {html.escape("; ".join(g[o]["id"] for o in others))}</span>' if others and parent else ''
        head = (f'<span class="name g">{html.escape(g[n]["id"])}</span> <span class="also">{html.escape(n)}</span> {badge} '
                f'<span class="open">{sum(1 for x in tg.under(g, kids, n) if tg.is_t(x))} open below</span>' if not tg.is_t(n) else
                f'<span class="name t">{html.escape(n)}</span> {badge}')
        body = f'<div class="text">{html.escape(t)}</div>' + (f'<div class="site">{html.escape(site)}</div>' if site else '') + also
        ks = kids.get(n, [])
        if not ks: return f'<li class="leaf"><div class="row">{head}</div>{body}</li>'
        if n in shown: return f'<li><div class="row">{head} <span class="also">(expanded above)</span></div></li>'
        shown.add(n)
        order = [x for x in ks if not tg.is_t(x)] + sorted([x for x in ks if tg.is_t(x)], key=lambda x: int(x[1:]))
        return (f'<li><details open><summary class="row">{head}</summary>{body}'
                f'<ul>{"".join(node(c, n) for c in order)}</ul></details></li>')
    tree = ''.join(node(r, None) for r in roots)
    return f'''<!doctype html><html><head><meta charset="utf-8"><title>Task graph</title>
<meta name="viewport" content="width=device-width, initial-scale=1"><style>
body{{font:15px/1.5 -apple-system,"Segoe UI",sans-serif;margin:24px;background:#fbfaf7;color:#1d1c1a}}
ul{{list-style:none;padding-left:22px;border-left:1px solid #ddd8ce;margin:4px 0}} li{{margin:6px 0}}
summary{{cursor:pointer}} .name.g{{font-weight:700}} .name.t{{font-weight:600;color:#a0522d}}
.text{{color:#55524b;font-size:13.5px;max-width:980px;margin:2px 0 2px 2px}} .site,.also,.open{{color:#9a968d;font-size:12.5px}}
.now{{color:#1f7a3a;font-size:12.5px;font-weight:600}} .wait{{color:#9a6b00;font-size:12.5px}}
@media (prefers-color-scheme:dark){{body{{background:#161615;color:#e9e6df}} .text{{color:#bdb9b0}} ul{{border-color:#333}}}}
</style></head><body><h2>Task graph</h2>
<p class="site">Read from task-graph/hyperactivity.cub. Every node with its text; tickets show whether they can start now or what they wait on. A node with several parents is expanded once.</p>
<ul style="border:0;padding:0">{tree}</ul></body></html>'''


class H(BaseHTTPRequestHandler):
    def log_message(self, *a): pass
    def do_GET(self):
        try: b = page().encode()
        except Exception as e: b = f'<pre>{html.escape(repr(e))}</pre>'.encode()
        self.send_response(200); self.send_header('Content-Type', 'text/html; charset=utf-8')
        self.send_header('Content-Length', str(len(b))); self.end_headers(); self.wfile.write(b)


if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    print(f'http://localhost:{port}/', flush=True)
    ThreadingHTTPServer(('127.0.0.1', port), H).serve_forever()
