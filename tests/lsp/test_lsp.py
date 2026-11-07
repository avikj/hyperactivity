#!/usr/bin/env python3
"""A scripted session against tools/lsp.py: a file with an error, the error fixed, a dependent
re-checked against a changed definition, a data type changed (the machine restarted), a parse
error kept to its line, hover, shutdown. Prints ok/FAIL per step; exit 1 on a failure.

  python3 tests/lsp/test_lsp.py   (from scoped-machine/, after `make build/cubical`)
"""
import json, os, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
URI = "file:///tmp/lsp-test.cub"
fails = 0


class Client:
    def __init__(self):
        env = dict(os.environ, CUBICAL_WORKERS="0")
        self.p = subprocess.Popen([sys.executable, os.path.join(ROOT, "tools", "lsp.py"), "--root", ROOT],
                                  stdin=subprocess.PIPE, stdout=subprocess.PIPE, env=env)
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
check("data changed (restart)", d == [], json.dumps(d))
d = change(7, V7)
n = V7.split("\n").index("broken =")
check("parse error at its line, machine alive", len(d) == 1 and d[0]["range"]["start"]["line"] in (n, n + 1), json.dumps(d))
d = change(8, V6)
check("parse error removed", d == [], json.dumps(d))

h = c.request("textDocument/hover", {"textDocument": {"uri": URI}, "position": {"line": V6.split("\n").index("useN = plus twoN twoN"), "character": 13}})
check("hover shows the signature", (h.get("result") or {}).get("contents", {}).get("value") == "twoN : Nat", json.dumps(h))

c.send("textDocument/didClose", {"textDocument": {"uri": URI}})
check("close clears", c.diags() == [])
r = c.request("shutdown", None)
check("shutdown", "result" in r, json.dumps(r))
c.send("exit", None)
check("exit", c.p.wait(timeout=30) == 0)
print("lsp:", "all ok" if not fails else f"{fails} failed")
sys.exit(1 if fails else 0)
