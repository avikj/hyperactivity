#!/usr/bin/env python3
"""A scripted session against tools/lsp.py: a file with an error, the error fixed, a dependent
re-checked against a changed definition, a data type changed (retracted, not restarted), a parse
error kept to its line, an incremental edit, hover (the machine's #type), completion (#scope),
go-to-definition (the document's unit, a library's #where), shutdown. Prints ok/FAIL per step; exit 1 on a failure.

  python3 tests/lsp/test_lsp.py   (from scoped-machine/, after `make build/cubical`)
"""
import json, os, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
URI = "file:///tmp/lsp-test.cub"
fails = 0


class Client:
    def __init__(self):
        env = dict(os.environ, CUBICAL_WORKERS="0", CUB_LSP_LOG="1")
        self.log = open("/tmp/lsp-test.log", "w")
        self.p = subprocess.Popen([sys.executable, os.path.join(ROOT, "tools", "lsp.py"), "--root", ROOT],
                                  stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=self.log, env=env)
        self.n = 0

    def send(self, method, params, req=False):
        m = {"jsonrpc": "2.0", "method": method, "params": params}
        if req:
            self.n += 1
            m["id"] = self.n
        b = json.dumps(m).encode()
        self.p.stdin.write(b"Content-Length: %d\r\n\r\n" % len(b) + b)
        self.p.stdin.flush()
        return self.n if req else None

    def recv(self):
        n = None
        while True:
            l = self.p.stdout.readline()
            if not l:
                raise EOFError("the server stopped")
            l = l.strip()
            if not l:
                break
            if l.lower().startswith(b"content-length:"):
                n = int(l.split(b":")[1])
        return json.loads(self.p.stdout.read(n))

    def until(self, pred):
        while True:
            m = self.recv()
            if pred(m):
                return m

    def request(self, method, params):
        i = self.send(method, params, True)
        return self.until(lambda m: m.get("id") == i)

    def diags(self):
        return self.until(lambda m: m.get("method") == "textDocument/publishDiagnostics")["params"]["diagnostics"]


def check(name, cond, detail=""):
    global fails
    print(name, "ok" if cond else "FAIL" + (": " + detail if detail else ""))
    if not cond:
        fails += 1


V1 = """twoN : Nat
twoN = suc (suc zero)

bad : Nat
bad = suc true

useN : Nat
useN = plus twoN twoN
#same useN = 4
"""
V2 = V1.replace("bad = suc true", "bad = suc zero")
V3 = V2.replace("twoN = suc (suc zero)", "twoN = suc zero")          # useN now 2: the #same fails
V4 = V3.replace("#same useN = 4", "#same useN = 2")
V5 = "data Col : Type where\n  red : Col\n  blue : Col\n\n" + V4 + "\ncol : Col\ncol = red\n"
V6 = V5.replace("  blue : Col\n", "  blue : Col\n  green : Col\n").replace("col = red", "col = green")
V7 = V6 + "\nbroken : Nat\nbroken =\n"

c = Client()
r = c.request("initialize", {"processId": None, "rootUri": None, "capabilities": {}})
check("initialize", r.get("result", {}).get("capabilities", {}).get("hoverProvider") is True, json.dumps(r))
c.send("initialized", {})

c.send("textDocument/didOpen", {"textDocument": {"uri": URI, "languageId": "cubical", "version": 1, "text": V1}})
d = c.diags()
check("error reported", len(d) == 1 and d[0]["range"]["start"]["line"] == 3 and "mismatch" in d[0]["message"], json.dumps(d))

def change(v, text):
    c.send("textDocument/didChange", {"textDocument": {"uri": URI, "version": v}, "contentChanges": [{"text": text}]})
    return c.diags()

d = change(2, V2)
check("error fixed, diagnostics cleared", d == [], json.dumps(d))
d = change(3, V3)
check("dependent re-checked against the new definition", len(d) == 1 and d[0]["range"]["start"]["line"] == 8, json.dumps(d))
d = change(4, V4)
check("dependent fixed", d == [], json.dumps(d))
d = change(5, V5)
check("data added", d == [], json.dumps(d))
d = change(6, V6)
check("data changed", d == [], json.dumps(d))
c.log.flush()
check("data changed without a restart", "restart" not in open("/tmp/lsp-test.log").read())
d = change(7, V7)
n = V7.split("\n").index("broken =")
check("parse error at its line, machine alive", len(d) == 1 and d[0]["range"]["start"]["line"] in (n, n + 1), json.dumps(d))
d = change(8, V6)
check("parse error removed", d == [], json.dumps(d))

# an incremental edit: col = green → col = blue, by its range
L = V6.split("\n").index("col = green")
c.send("textDocument/didChange", {"textDocument": {"uri": URI, "version": 9},
       "contentChanges": [{"range": {"start": {"line": L, "character": 6}, "end": {"line": L, "character": 11}}, "text": "blue"}]})
d = c.diags()
check("incremental edit", d == [], json.dumps(d))
c.send("textDocument/didChange", {"textDocument": {"uri": URI, "version": 10},
       "contentChanges": [{"range": {"start": {"line": L, "character": 6}, "end": {"line": L, "character": 10}}, "text": "true"}]})
d = c.diags()
check("incremental edit with an error", len(d) == 1 and d[0]["range"]["start"]["line"] in (L, L - 1), json.dumps(d))
c.send("textDocument/didChange", {"textDocument": {"uri": URI, "version": 11},
       "contentChanges": [{"range": {"start": {"line": L, "character": 6}, "end": {"line": L, "character": 10}}, "text": "green"}]})
check("incremental edit back", c.diags() == [])

def hover(line, ch):
    h = c.request("textDocument/hover", {"textDocument": {"uri": URI}, "position": {"line": line, "character": ch}})
    return (h.get("result") or {}).get("contents", {}).get("value")
U = V6.split("\n").index("useN = plus twoN twoN")
check("hover: the type the machine infers", hover(U, 13) == "twoN : Nat", str(hover(U, 13)))
check("hover: a library definition", hover(U, 8) == "plus : Nat → Nat → Nat", str(hover(U, 8)))
check("hover: a data type's value", hover(V6.split("\n").index("col = green"), 1) == "col : Col", str(hover(V6.split("\n").index("col = green"), 1)))
# locals: a clause's pattern variable and an implicit parameter, typed from the signature
V12 = V6.rstrip("\n") + "\ndbl : {z : Nat} → Nat → Bool → Nat\ndbl k b = plus k z\n"
d = change(12, V12)
check("a definition with locals", d == [], json.dumps(d))
K = V12.split("\n").index("dbl k b = plus k z")
check("hover: a pattern variable", hover(K, 15) == "k : Nat", str(hover(K, 15)))
check("hover: a later pattern variable", hover(K, 6) == "b : Bool", str(hover(K, 6)))
check("hover: an implicit parameter", hover(K, 17) == "z : Nat", str(hover(K, 17)))
d = change(13, V6)
check("locals removed", d == [], json.dumps(d))

def complete(line, ch):
    r = c.request("textDocument/completion", {"textDocument": {"uri": URI}, "position": {"line": line, "character": ch}})
    return [i["label"] for i in (r.get("result") or {}).get("items", [])]
cs = complete(U, 14)    # after "useN = plus tw"
check("completion: the document's names", "twoN" in cs and "plus" not in cs, str(cs[:10]))
cs = complete(U, 11)    # after "useN = plus"
check("completion: the library's names", "plus" in cs and "plus-comm" in cs, str(cs[:10]))

def definition(line, ch):
    r = c.request("textDocument/definition", {"textDocument": {"uri": URI}, "position": {"line": line, "character": ch}})
    return r.get("result")
g = definition(U, 13)
check("definition in the document", g and g["uri"] == URI and g["range"]["start"]["line"] == V6.split("\n").index("twoN : Nat"), json.dumps(g))
g = definition(U, 8)
check("definition in a library", g and g["uri"].endswith("/lib/std.cub") and g["range"]["start"]["line"] >= 0, json.dumps(g))

c.send("textDocument/didClose", {"textDocument": {"uri": URI}})
check("close clears", c.diags() == [])
r = c.request("shutdown", None)
check("shutdown", "result" in r, json.dumps(r))
c.send("exit", None)
check("exit", c.p.wait(timeout=30) == 0)
print("lsp:", "all ok" if not fails else f"{fails} failed")
sys.exit(1 if fails else 0)
