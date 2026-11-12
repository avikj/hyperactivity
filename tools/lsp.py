#!/usr/bin/env python3
"""A language server for .cub files (item 30): JSON-RPC over stdio, wrapping one long-lived
`build/cubical LIBS --serve` per open document.

The document is split into top-level units (a column-0 line and the indented or blank lines after
it; a signature and its clauses are one unit; a `#fail` line belongs to the unit after it). Each
unit goes to the machine as its own block, so a position `<serve>:N` in a verdict is line N of the
unit. On a change, the units up to the first that differs keep their verdicts; the definitions of
every unit after it are retracted (`#retract name`) and those units are sent again in order, so
what depends on a changed definition is checked against the new one. A unit that cannot be
retracted (data, codata, record, mutual, postulate, module, variable, infix, import, ...) among
those after the first difference restarts the machine and sends the whole document.

Data, codata and record declarations are retracted like definitions (`#retract D` renames the type,
its constructors and fields out of reach; what was checked against it keeps it). Hover shows the
type the machine infers for the name under the cursor (`#type`), falling back to the signature
written in the document; completion offers the names in scope (`#scope`, refreshed after each
check, and the document's own); go-to-definition finds a name's unit in the document, or its
library declaration (`#where`). Text changes are applied incrementally (ranges in UTF-16 units).

  python3 tools/lsp.py [--cubical build/cubical] [--root DIR] [LIB.cub ...]

LIBs default to lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub, relative to --root (the
repository root, by default the parent of this file's directory).
"""
import json, os, re, subprocess, sys, threading

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_LIBS = ["lib/prelude.cub", "lib/code.cub", "lib/check.cub", "lib/std.cub"]
# heads whose effect #retract cannot undo: a change among them restarts the machine
STRUCTURAL = {"mutual", "postulate", "module", "open", "variable",
              "infix", "infixl", "infixr", "import", "instance", "primitive", "represent",
              "foreign", "#native", "#input", "#supply", "#layout", "#unbind", "#retract", "world"}
# heads that declare a type by the name after them, retracted by that name
TYPE_HEADS = {"data", "codata", "record"}


def log(*a):
    if os.environ.get("CUB_LSP_LOG"):
        print("[lsp]", *a, file=sys.stderr, flush=True)


# ---- the document as units ------------------------------------------------------------------

class Unit:
    def __init__(self, start, lines):
        self.start = start            # 0-based line of the unit's first line in the document
        self.lines = lines            # its lines, without newlines
        self.text = "\n".join(lines) + "\n"
        head = self.first_code_line()
        toks = head.split() if head else []
        self.head = toks[0] if toks else ""
        self.structural = self.head in STRUCTURAL
        self.name = None              # the definition it declares, for #retract
        if self.head in TYPE_HEADS and len(toks) >= 2:
            self.name = toks[1]
        elif toks and not self.head.startswith("#") and not self.structural and len(toks) >= 2:
            self.name = self.head

    def first_code_line(self):
        for l in self.lines:
            s = l.strip()
            if s and not s.startswith("--") and s != "#fail":
                return l
        return ""

    def defined_names(self):
        return [self.name] if self.name else []

    def name_line(self):
        """the document line of the unit's first code line (its signature or declaration)"""
        for i, l in enumerate(self.lines):
            s = l.strip()
            if s and not s.startswith("--") and s != "#fail":
                return self.start + i
        return self.start


def split_top(s, sep):
    """s split at sep where no bracket is open"""
    out, depth, cur, i = [], 0, "", 0
    while i < len(s):
        c = s[i]
        if c in "({[":
            depth += 1
        elif c in ")}]":
            depth -= 1
        if depth == 0 and s.startswith(sep, i):
            out.append(cur); cur = ""; i += len(sep); continue
        cur += c; i += 1
    out.append(cur)
    return out


def telescope(sig):
    """a signature's binders in order: (name or None, implicit, type)"""
    parts = split_top(sig, "→")
    res = []
    for seg in parts[:-1]:
        seg = seg.strip()
        # several binder groups in one segment: (a : A) {b : B} (c d : C)
        groups = re.findall(r"(\{\{?|\()([^(){}]*?)(\}\}?|\))", seg) if seg[:1] in "({" else []
        if groups and seg.replace(" ", "") == "".join(o + g + c for o, g, c in groups).replace(" ", ""):
            for o, g, c in groups:
                if ":" in g:
                    names, ty = g.split(":", 1)
                    for n in names.split():
                        res.append((n, o.startswith("{"), ty.strip()))
                else:
                    res.append((None, o.startswith("{"), g.strip()))
        else:
            res.append((None, False, seg))
    return res


def patterns(lhs):
    """a clause's argument patterns: atoms, (…) and {…} groups"""
    out, depth, cur = [], 0, ""
    for c in lhs.strip() + " ":
        if c in "({":
            depth += 1
        elif c in ")}":
            depth -= 1
        if c == " " and depth == 0:
            if cur:
                out.append(cur)
            cur = ""
        else:
            cur += c
    return out


def split_units(text):
    lines = text.split("\n")
    if lines and lines[-1] == "":
        lines.pop()
    # column-0 chunks: a line at column 0 and the indented or blank lines after it
    chunks = []
    for i, l in enumerate(lines):
        if l[:1] not in ("", " ", "\t") or not chunks:
            chunks.append((i, [l]))
        else:
            chunks[-1][1].append(l)
    units, pend, pstart = [], [], None
    def head(ls):
        t = ls[0].split()
        return t[0] if t else ""
    i = 0
    while i < len(chunks):
        start, ls = chunks[i]
        h = head(ls)
        # comments and a lone #fail go with what follows them
        if not h or h.startswith("--") or (h == "#fail" and len(ls[0].split()) == 1):
            if pstart is None:
                pstart = start
            pend = pend + ls
            i += 1
            continue
        ls = pend + ls
        if pstart is not None:
            start = pstart
        pend, pstart = [], None
        j = i + 1
        if not h.startswith("#") and h not in STRUCTURAL and h not in TYPE_HEADS:
            # a signature and its clauses (and clauses written without a signature)
            while j < len(chunks):
                t = chunks[j][1][0].split()
                if t and t[0] == h and not (len(t) >= 2 and t[1] == ":"):
                    ls = ls + chunks[j][1]; j += 1
                else:
                    break
        units.append(Unit(start, ls))
        i = j
    return units


# ---- the machine ------------------------------------------------------------------------------

class Machine:
    def __init__(self, cubical, root, libs):
        env = dict(os.environ)
        env.setdefault("CUBICAL_WORKERS", "0")
        self.p = subprocess.Popen([cubical] + libs + ["--serve"], cwd=root, env=env,
                                  stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT, text=True, bufsize=1)
        self.lib_output = self.request("")    # the libraries' verdicts, up to the first `.`

    def request(self, block):
        """send a block (a line `.` would end it early: sent blank), the output up to
        the machine's `.`; None if the machine is gone"""
        try:
            ls = block.split("\n")
            if ls and ls[-1] == "":
                ls.pop()
            for l in ls:
                # (kept as a blank line so the positions in the verdict stay the unit's)
                self.p.stdin.write(("" if l.strip() == "." else l) + "\n")
            self.p.stdin.write(".\n")
            self.p.stdin.flush()
        except (BrokenPipeError, OSError):
            return None
        out = []
        while True:
            l = self.p.stdout.readline()
            if not l:
                return None if not out else out
            l = l.rstrip("\n")
            if l == ".":
                return out
            out.append(l)

    def close(self):
        try:
            self.p.stdin.close()
        except OSError:
            pass
        try:
            self.p.wait(timeout=5)
        except subprocess.TimeoutExpired:
            self.p.kill()


POS = re.compile(r"<serve>:(\d+)(?::(\d+))?:?")


def verdicts(unit, out):
    """the diagnostics of a unit from the machine's output for it"""
    diags = []
    if out is None:
        return [diag(unit.start, 0, "the machine stopped while checking this declaration")]
    i = 0
    while i < len(out):
        l = out[i]
        if l.startswith("FAIL:"):
            msg = [l[5:].strip()]
            line = None
            j = i + 1
            while j < len(out) and out[j].startswith("   "):
                d = out[j].strip()
                m = POS.search(d) if d.startswith("in:") else None
                if m:
                    line = int(m.group(1)) - 1
                else:
                    msg.append(d)
                j += 1
            diags.append(diag(unit.start + (line if line is not None else 0), 0, "\n".join(msg), unit))
            i = j
            continue
        if l.startswith("error:"):
            m = POS.search(l)
            if m:
                ln, col = int(m.group(1)) - 1, int(m.group(2) or 1) - 1
                text = l[m.end():].strip()
            else:
                ln, col, text = 0, 0, l[6:].strip()
            diags.append(diag(unit.start + max(ln, 0), max(col, 0), text, unit))
        i += 1
    return diags


def diag(line, col, msg, unit=None):
    width = 1
    if unit is not None and 0 <= line - unit.start < len(unit.lines):
        width = max(1, len(unit.lines[line - unit.start]) - col)
    return {"range": {"start": {"line": line, "character": col},
                      "end": {"line": line, "character": col + width}},
            "severity": 1, "source": "cubical", "message": msg}


# ---- a document's state -----------------------------------------------------------------------

class Doc:
    def __init__(self, server, uri, text):
        self.server, self.uri, self.text = server, uri, text
        self.machine = None
        self.units = []               # units the machine has seen, in order
        self.diags = []               # per unit
        self.scope = []               # the machine's names in scope after the last check

    def ensure_machine(self):
        if self.machine is None:
            self.machine = Machine(self.server.cubical, self.server.root, self.server.libs)
            self.units, self.diags = [], []

    def update(self, text):
        self.text = text
        new = split_units(text)
        self.ensure_machine()
        k = 0
        while k < len(self.units) and k < len(new) and self.units[k].text == new[k].text:
            k += 1
        # the kept units may have moved (an edit above them cannot leave them equal, so only the
        # positions of units at or after k change; kept ones keep theirs)
        stale = self.units[k:]
        if any(u.structural for u in stale):
            log("restart: a structural unit changed")
            self.machine.close()
            self.machine = None
            self.ensure_machine()
            k = 0
            stale = []
        for u in stale:
            for n in u.defined_names():
                self.machine.request("#retract " + n)
        self.units = self.units[:k]
        self.diags = self.diags[:k]
        for u in new[k:]:
            out = self.machine.request(u.text)
            self.units.append(u)
            self.diags.append(verdicts(u, out))
            if out is None:
                # the machine stopped on this unit: a fresh one next time, the rest unchecked
                log("machine gone")
                self.machine = None
                self.units = []
                break
        self.refresh_scope()
        return self.flat()

    def refresh_scope(self):
        if self.machine is None:
            return
        out = self.machine.request("#scope") or []
        for l in out:
            if l.startswith("scope:"):
                self.scope = l[6:].split()

    def ask(self, line, prefix):
        """one command to the machine, the rest of its line beginning with prefix"""
        self.ensure_machine()
        out = self.machine.request(line)
        if out is None:
            self.machine = None
            self.units = []
            return None
        for l in out:
            if l.startswith(prefix):
                return l[len(prefix):].strip()
        return None

    def type_of(self, name):
        r = self.ask("#type " + name, "type:")
        if not r:
            return None
        # `name : T`; `?` when the machine infers none
        t = r.split(" : ", 1)
        return None if len(t) < 2 or t[1].strip() == "?" else t[0] + " : " + t[1]

    def where(self, name):
        r = self.ask("#where " + name, "where:")
        if not r:
            return None
        t = r.split()
        if len(t) < 2 or t[1] == "?" or t[1].startswith("<serve>"):
            return None
        f, _, ln = t[1].rpartition(":")
        try:
            return f, int(ln) - 1
        except ValueError:
            return None

    def apply_change(self, ch):
        """an incremental change (a range in UTF-16 units) or the whole text"""
        if "range" not in ch:
            return ch["text"]
        lines = self.text.split("\n")
        def off(pos):
            ln, c = pos["line"], pos["character"]
            if ln >= len(lines):
                return len(self.text)
            base = sum(len(l) + 1 for l in lines[:ln])
            u, k = 0, 0
            line = lines[ln]
            while k < len(line) and u < c:
                u += 2 if ord(line[k]) > 0xFFFF else 1
                k += 1
            return base + k
        a, b = off(ch["range"]["start"]), off(ch["range"]["end"])
        return self.text[:a] + ch["text"] + self.text[b:]

    def unit_of(self, name):
        for u in split_units(self.text):
            if u.name == name:
                return u
        return None

    def flat(self):
        return [d for ds in self.diags for d in ds]

    def signature_of(self, name):
        for l in self.text.split("\n"):
            t = l.split()
            if len(t) >= 2 and t[0] == name and t[1] == ":":
                return l.strip()
        return None

    def local_type(self, line, word):
        """a local of the definition the line is in: a parameter's type from the signature, the
        clause's patterns matched to its binders by position (implicit binders by name, or by a
        {x} pattern); None when the word is not such a local"""
        unit = None
        for u in split_units(self.text):
            if u.start <= line < u.start + len(u.lines):
                unit = u
        if unit is None or not unit.name:
            return None
        sig = None
        body = unit.text.split("\n")
        for i, l in enumerate(body):
            t = l.split()
            if len(t) >= 2 and t[0] == unit.name and t[1] == ":":
                # a signature may continue on indented lines
                sig = l.split(":", 1)[1]
                for l2 in body[i + 1:]:
                    if l2[:1] in (" ", "\t") and l2.strip():
                        sig += " " + l2.strip()
                    else:
                        break
                break
        if sig is None:
            return None
        binders = telescope(sig)
        for b in binders:
            if b[0] == word:
                return word + " : " + b[2]
        # the clause holding the line: its patterns by position
        clause = self.text.split("\n")[line]
        k = line
        while k > unit.start and not clause.startswith(unit.name + " "):
            k -= 1
            clause = self.text.split("\n")[k]
        if not clause.startswith(unit.name + " ") or "=" not in clause:
            return None
        lhs = clause[len(unit.name):].split("=", 1)[0]
        pats = patterns(lhs)
        explicit = [b for b in binders if not b[1]]
        implicit = [b for b in binders if b[1]]
        ei = ii = 0
        for p in pats:
            if p.startswith("{"):
                inner = p[1:-1].strip()
                if ii < len(implicit) and inner == word:
                    return word + " : " + implicit[ii][2]
                ii += 1
                continue
            if ei < len(explicit) and p == word:
                return word + " : " + explicit[ei][2]
            ei += 1
        return None

    def close(self):
        if self.machine:
            self.machine.close()
            self.machine = None


# ---- JSON-RPC -----------------------------------------------------------------------------------

class Server:
    def __init__(self, cubical, root, libs, rin, rout):
        self.cubical, self.root, self.libs = cubical, root, libs
        self.rin, self.rout = rin, rout
        self.docs = {}
        self.shutdown = False
        self.lock = threading.Lock()

    def send(self, msg):
        b = json.dumps(msg).encode()
        with self.lock:
            self.rout.write(b"Content-Length: %d\r\n\r\n" % len(b) + b)
            self.rout.flush()

    def read(self):
        n = None
        while True:
            l = self.rin.readline()
            if not l:
                return None
            l = l.strip()
            if not l:
                break
            if l.lower().startswith(b"content-length:"):
                n = int(l.split(b":")[1])
        if n is None:
            return None
        return json.loads(self.rin.read(n))

    def publish(self, uri, diags):
        self.send({"jsonrpc": "2.0", "method": "textDocument/publishDiagnostics",
                   "params": {"uri": uri, "diagnostics": diags}})

    def run(self):
        while True:
            m = self.read()
            if m is None:
                break
            meth, mid, pr = m.get("method"), m.get("id"), m.get("params") or {}
            try:
                r = self.handle(meth, pr)
            except Exception as e:  # a request fails, the server stays
                log("error", meth, repr(e))
                if mid is not None:
                    self.send({"jsonrpc": "2.0", "id": mid, "error": {"code": -32603, "message": repr(e)}})
                continue
            if meth == "exit":
                break
            if mid is not None and meth is not None:
                self.send({"jsonrpc": "2.0", "id": mid, "result": r})
        for d in self.docs.values():
            d.close()
        return 0 if self.shutdown else 1

    def handle(self, meth, pr):
        if meth == "initialize":
            return {"capabilities": {"textDocumentSync": {"openClose": True, "change": 2},
                                     "hoverProvider": True, "definitionProvider": True,
                                     "completionProvider": {"triggerCharacters": []}},
                    "serverInfo": {"name": "cubical-lsp"}}
        if meth == "shutdown":
            self.shutdown = True
            return None
        if meth == "textDocument/didOpen":
            td = pr["textDocument"]
            d = self.docs[td["uri"]] = Doc(self, td["uri"], td["text"])
            self.publish(td["uri"], d.update(td["text"]))
            return None
        if meth == "textDocument/didChange":
            uri = pr["textDocument"]["uri"]
            d = self.docs.get(uri)
            if d is None:
                d = self.docs[uri] = Doc(self, uri, "")
            text = d.text
            for ch in pr["contentChanges"]:
                d.text = text = d.apply_change(ch)
            self.publish(uri, d.update(text))
            return None
        if meth == "textDocument/didClose":
            uri = pr["textDocument"]["uri"]
            d = self.docs.pop(uri, None)
            if d:
                d.close()
            self.publish(uri, [])
            return None
        if meth in ("textDocument/hover", "textDocument/definition", "textDocument/completion"):
            uri = pr["textDocument"]["uri"]
            d = self.docs.get(uri)
            if not d:
                return None
            ln, ch = pr["position"]["line"], pr["position"]["character"]
            lines = d.text.split("\n")
            if ln >= len(lines):
                return None
            l = lines[ln]
            if meth == "textDocument/completion":
                m = re.search(r"[^\s(){}\[\],;.]*$", l[:ch])
                pre = m.group(0) if m else ""
                names = sorted(set(d.scope) | {u.name for u in split_units(d.text) if u.name})
                items = [{"label": n, "kind": 3} for n in names if n.startswith(pre)][:200]
                return {"isIncomplete": len(items) == 200, "items": items}
            word = None
            for m in re.finditer(r"[^\s(){}\[\],;]+", l):
                if m.start() <= ch <= m.end():
                    word = m.group(0)
            if not word or not re.match(r"^[^#\d\\λ→:=|]", word):
                return None
            if meth == "textDocument/hover":
                t = d.local_type(ln, word) or d.type_of(word) or d.signature_of(word)
                return {"contents": {"kind": "plaintext", "value": t}} if t else None
            u = d.unit_of(word)
            if u is not None:
                p = {"line": u.name_line(), "character": 0}
                return {"uri": uri, "range": {"start": p, "end": p}}
            w = d.where(word)
            if w is None:
                return None
            f, wl = w
            p = {"line": max(wl, 0), "character": 0}
            return {"uri": "file://" + os.path.abspath(os.path.join(self.root, f)), "range": {"start": p, "end": p}}
        return None


def main(argv):
    cubical, root, libs = None, os.path.dirname(HERE), []
    i = 0
    while i < len(argv):
        a = argv[i]
        if a == "--cubical":
            cubical = argv[i + 1]; i += 2
        elif a == "--root":
            root = argv[i + 1]; i += 2
        else:
            libs.append(a); i += 1
    if cubical is None:
        cubical = os.path.join(root, "build", "cubical")
    srv = Server(os.path.abspath(cubical), root, libs or DEFAULT_LIBS, sys.stdin.buffer, sys.stdout.buffer)
    return srv.run()


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
