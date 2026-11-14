#!/usr/bin/env python3
r"""agda2cub — a translator from Cubical Agda source to this language (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §12 “The build list”, “E. The language people and models write in”, item 29, E29: the translator from agda/cubical).

Usage: tools/agda2cub.py FILE.agda [-o FILE.cub]

What it does, construct by construct (the standard interdefinability of Agda's primitives with
CCHM composition):
  transp A φ a         →  comp A (λ i → [ φ ↦ a ]) a      (φ = i0 gives the empty system)
  hcomp {A = T} u a    →  comp (λ j → T) u a               (T inferred from the signature when
                                                           the implicit is not written)
  λ { (i = i0) → t ; (j = i1) → u ; _ → v }  →  [ i = 0 ↦ t , j = 1 ↦ u , tt ↦ v ]
  i0 / i1 in faces → 0 / 1 ; ∧ ∨ → /\ \/ ; Type ℓ, Set, ℓ-max, ℓ-zero, ℓ-suc → Type l, Type, lmax,
  lzero, lsuc ; ∀ {ℓ} → … → {l : Level} → … ; data D : Type where c : … is this language's own
  form (constructors by type, a path type at the end is a dimension with that boundary), so data
  declarations pass through; module headers, imports, pragmas, private and variable blocks are
  dropped (the target auto-generalizes free names in signatures; the prelude and lib/std.cub stand
  for Cubical.Foundations.Prelude: sym, cong, _∙_, transport, subst, funExt, toPathP, ua, refl).
A line beginning `-- cub:` passes its rest through, which is how the readings (#same, #check) of
a round-trip example are written beside the Agda source.
A clause's named implicit {x = x} is dropped (a signature's binders are in scope in the body
here); {x = y} renames y to x in the clause. A signature's Π is never over I in this language: a
function of dimensions is stated as a path type (PathP), which is how the examples write fillers.
What it refuses (reported on stderr, the line kept as is): Partial/PartialP/Sub types,
inS/outS/primPOr, two-dimensional path constructors, ∀ over named term binders, mutual blocks,
abstract, instance blocks, with-abstraction.
"""
import re, sys

LEVEL_SUBST = [("ℓ-max", "lmax"), ("ℓ-zero", "lzero"), ("ℓ-suc", "lsuc"), ("Type₀", "Type"), ("Set₀", "Type")]
UNSUPPORTED = ["Partial", "PartialP", "Sub", "inS", "outS", "primPOr", "primComp", "primHComp", "primTransp"]

def warn(ln, msg):
    sys.stderr.write("agda2cub: line %d: %s\n" % (ln, msg))

# ---------------------------------------------------------------- tokens
TOK = re.compile(r"""--[^\n]*|\{-#.*?#-\}|\{-.*?-\}|\s+|[(){}\[\];,]|[^\s(){}\[\];,]+""", re.S)

def tokens(s):
    return [t for t in TOK.findall(s)]

# ---------------------------------------------------------------- partial elements
def face_of(toks):
    """(i = i0) (j = i1) → 'i = 0 /\\ j = 1'; '_' → 'tt'."""
    s = " ".join(toks).strip()
    if s == "_": return "tt"
    parts = re.findall(r"\(\s*([^\s=]+)\s*=\s*(i0|i1)\s*\)", s)
    if not parts: return None
    return " /\\ ".join("%s = %s" % (v, "0" if c == "i0" else "1") for v, c in parts)

def split_top(toks, sep):
    out, cur, d = [], [], 0
    for t in toks:
        if t in "([{": d += 1
        elif t in ")]}": d -= 1
        if d == 0 and t == sep: out.append(cur); cur = []
        else: cur.append(t)
    out.append(cur)
    return out

def partial_to_system(body, ln):
    clauses = [c for c in split_top(body, ";") if any(x.strip() for x in c)]
    entries = []
    for c in clauses:
        c = [x for x in c if x.strip()]
        try: k = c.index("→")
        except ValueError:
            warn(ln, "a partial element's clause without →"); return None
        f = face_of(c[:k])
        if f is None:
            warn(ln, "a face that is not a conjunction of (i = i0)/(i = i1): " + " ".join(c[:k])); return None
        entries.append(f + " ↦ " + " ".join(c[k + 1:]))
    return "[ " + " , ".join(entries) + " ]"

# ---------------------------------------------------------------- formulas to systems
def formula_system(toks, val):
    """a formula φ (i, ~ i, i ∧ j, i ∨ j, i0, i1) to the system [ φ ↦ val ]"""
    s = " ".join(toks).strip()
    if s in ("i0", "(i0)"): return "[]"
    if s in ("i1", "(i1)"): return "[ tt ↦ %s ]" % val
    disj = [d.strip() for d in re.split(r"\s∨\s", s)]
    ent = []
    for d in disj:
        conj = [c.strip().strip("()") for c in re.split(r"\s∧\s", d)]
        fs = []
        for c in conj:
            m = re.fullmatch(r"~\s*([^\s~]+)", c)
            if m: fs.append("%s = 0" % m.group(1))
            elif re.fullmatch(r"[^\s~()]+", c): fs.append("%s = 1" % c)
            else: return None
        ent.append(" /\\ ".join(fs) + " ↦ " + val)
    return "[ " + " , ".join(ent) + " ]"

# ---------------------------------------------------------------- expressions
def group(toks, i):
    """the argument starting at toks[i]: a bracketed group or one token; returns (tokens, next)"""
    while i < len(toks) and not toks[i].strip(): i += 1
    if i >= len(toks): return [], i
    if toks[i] in "([{":
        close = {"(": ")", "[": "]", "{": "}"}[toks[i]]; d = 0; j = i
        while j < len(toks):
            if toks[j] == toks[i]: d += 1
            elif toks[j] == close:
                d -= 1
                if d == 0: return toks[i:j + 1], j + 1
            j += 1
        return toks[i:], len(toks)
    return [toks[i]], i + 1

def strip_parens(g):
    return g[1:-1] if len(g) >= 2 and g[0] == "(" and g[-1] == ")" else g

class Ctx:
    def __init__(self): self.binders = {}; self.result = None

def infer_hcomp_type(base, ctx, ln):
    """the type of hcomp's base from the signature: a path applied to a dimension, or a variable"""
    b = [t for t in strip_parens(base) if t.strip()]
    head = b[0] if b else None
    ty = ctx.binders.get(head)
    if ty is None and ctx.result is not None:
        return ctx.result
    if ty is None: return None
    t = [x for x in ty if x.strip()]
    if len(b) >= 2 and len(t) >= 3 and t[1] == "≡":  # p : x ≡ y with p i as base: the type of x
        xt = ctx.binders.get(t[0])
        return " ".join(x for x in xt) if xt else None
    if len(b) >= 2 and t and t[0] == "PathP":
        line, _ = group(t, 1); line = strip_parens(line)
        if len(line) >= 3 and line[0] == "λ" and "→" in line:
            return " ".join(line[line.index("→") + 1:])
        return None
    if len(b) >= 2 and t and t[0] == "Path":
        A, _ = group(t, 1); return " ".join(A)
    if len(b) == 1: return " ".join(t)
    return None

def translate_expr(toks, ctx, ln):
    out = []; i = 0
    while i < len(toks):
        t = toks[i]
        if t == "λ":
            # a partial element λ { … }
            j = i + 1
            while j < len(toks) and not toks[j].strip(): j += 1
            if j < len(toks) and toks[j] == "{":
                g, nxt = group(toks, j)
                sysx = partial_to_system(translate_expr(g[1:-1], ctx, ln), ln)
                out.append(sysx if sysx else " ".join(toks[i:nxt])); i = nxt; continue
            out.append(t); i += 1; continue
        if t == "transp":
            A, j = group(toks, i + 1); phi, j = group(toks, j); a, j = group(toks, j)
            A = translate_expr(A, ctx, ln); a = translate_expr(a, ctx, ln)
            sysx = formula_system(phi, "".join(a).strip())
            if sysx is None:
                warn(ln, "transp with a formula that is not built from i, ~ i, ∧, ∨: " + " ".join(phi)); out.append(t); i += 1; continue
            dim = "i"
            if len(A) >= 2 and A[0] == "(" and A[1] == "λ":
                k = 2
                while k < len(A) and not A[k].strip(): k += 1
                dim = A[k] if k < len(A) else "i"
            out.append("comp %s (λ %s → %s) %s" % ("".join(A), dim, sysx, "".join(a))); i = j; continue
        if t == "hcomp":
            j = i + 1; T = None
            while True:
                g, k = group(toks, j)
                if g and g[0] == "{":
                    inner = [x for x in g[1:-1] if x.strip()]
                    if inner and inner[0] == "A" and len(inner) > 2 and inner[1] == "=": T = " ".join(translate_expr(inner[2:], ctx, ln))
                    elif inner and inner[0] in ("φ", "ℓ", "l"): pass
                    elif T is None and inner and inner[0] not in ("φ",) and "=" not in inner: T = " ".join(translate_expr(inner, ctx, ln))
                    j = k; continue
                break
            u, j = group(toks, j); a, j = group(toks, j)
            u = translate_expr(u, ctx, ln); a = translate_expr(a, ctx, ln)
            if T is None: T = infer_hcomp_type(a, ctx, ln)
            if T is None:
                warn(ln, "hcomp without a type: write hcomp {A = T} (the signature gave none)"); T = "?"
            dim = "j"
            uu = strip_parens(u)
            if len(uu) >= 2 and uu[0] == "λ":
                k = 1
                while k < len(uu) and not uu[k].strip(): k += 1
                dim = uu[k] if k < len(uu) else "j"
            out.append("comp (λ %s → %s) %s %s" % (dim, T, "".join(u), "".join(a))); i = j; continue
        if t == "∧": out.append("/\\"); i += 1; continue
        if t == "∨": out.append("\\/"); i += 1; continue
        if t in UNSUPPORTED: warn(ln, "unsupported primitive: " + t)
        out.append(t); i += 1
    return out

# ---------------------------------------------------------------- declarations
def rename_levels(s):
    for a, b in LEVEL_SUBST: s = s.replace(a, b)
    s = re.sub(r"\bSet\b", "Type", s)
    s = re.sub(r"ℓ(['′]*)", lambda m: "l" + ("2" * len(m.group(1)) if m.group(1) else ""), s)
    s = re.sub(r"\bl(\d)\b", r"l\1", s)
    return s

def translate_forall(s, ln):
    # ∀ {ℓ ℓ'} → …  (levels only); ∀ {A} → is left with a warning
    def rep(m):
        names = m.group(1).split()
        if all(n.startswith("l") for n in names): return "{" + " ".join(names) + " : Level} →"
        warn(ln, "∀ over non-level binders is not translated: " + m.group(0)); return m.group(0)
    s = re.sub(r"∀\s*\{([^}:]+)\}\s*→", rep, s)
    if "∀" in s: warn(ln, "∀ over named binders is not translated")
    return s

def signature_binders(ctx, line):
    """record (x : T), {x y : T} from a signature, and the result type after the last →"""
    ctx.binders = {}; ctx.result = None
    toks = [t for t in tokens(line) if t.strip() and not t.startswith("--")]
    for m in re.finditer(r"[({]\s*([^:(){}]+?)\s*:\s*([^(){}]+?)\s*[)}]", line):
        ty = tokens(m.group(2))
        for nm in m.group(1).split(): ctx.binders[nm] = [t for t in ty if t.strip()]
    arrows = split_top(toks, "→")
    res = arrows[-1]
    if len(res) >= 3 and res[1] == "≡":
        xt = ctx.binders.get(res[0]); ctx.result = " ".join(xt) if xt else None
    elif res and res[0] == "PathP":
        line_, _ = group(res, 1); line_ = strip_parens(line_)
        if "→" in line_: ctx.result = " ".join(line_[line_.index("→") + 1:])
    elif res and res[0] == "Path":
        A, _ = group(res, 1); ctx.result = " ".join(A)

def translate(src):
    out = []; ctx = Ctx(); skip_block = False; skip_indent = 0
    lines = src.split("\n")
    for n, raw in enumerate(lines, 1):
        line = raw.rstrip()
        stripped = line.strip()
        if skip_block:
            if stripped and (len(line) - len(line.lstrip())) <= skip_indent and not stripped.startswith("--"): skip_block = False
            else: continue
        if not stripped: out.append(""); continue
        if stripped.startswith("-- cub:"): out.append(stripped[len("-- cub:"):].strip()); continue
        if stripped.startswith("--"): out.append(line); continue
        if stripped.startswith("{-#"): continue
        if re.match(r"(open\s+import|import|open)\b", stripped): continue
        if re.match(r"module\b", stripped) and stripped.endswith("where"): continue
        if re.match(r"(private|variable|mutual|abstract|instance)\b", stripped):
            if stripped.split()[0] in ("mutual", "abstract", "instance"): warn(n, stripped.split()[0] + " block dropped")
            skip_block = True; skip_indent = len(line) - len(line.lstrip()); continue
        if "with" in stripped.split(): warn(n, "with-abstraction is not translated")
        s = rename_levels(line)
        s = translate_forall(s, n)
        if re.match(r"\S+\s*:", s) and not stripped.startswith("data") and "=" not in s.split(":", 1)[0]:
            signature_binders(ctx, s)
        # a clause's named implicit {x = x}: the signature's binder is in scope in the body here,
        # so the binding is dropped; {x = y} renames y to x in the clause
        if "=" in s and not re.match(r"\s*data\b", s) and not re.match(r"\S+\s*:", s):
            d = 0; k = -1
            for idx, ch in enumerate(s):
                if ch in "({[": d += 1
                elif ch in ")}]": d -= 1
                elif ch == "=" and d == 0: k = idx; break
            if k < 0: lhs, rhs = s, ""
            else: lhs, rhs = s[:k], s[k + 1:]
            while True:
                m = re.search(r"\{\s*(\S+)\s*=\s*(\S+)\s*\}", lhs)
                if not m: break
                nm, pat = m.group(1), m.group(2)
                lhs = lhs[:m.start()] + lhs[m.end():]
                if pat != nm and re.fullmatch(r"[^\s(){}]+", pat): rhs = re.sub(r"(?<![\w'])%s(?![\w'])" % re.escape(pat), nm, rhs)
            s = lhs.rstrip() + " =" + rhs
        toks = tokens(s)
        body = translate_expr(toks, ctx, n)
        s2 = "".join(body)  # tokens carry their own whitespace
        s2 = re.sub(r"\(\s*(\S+)\s*=\s*i0\s*\)", r"\1 = 0", s2)  # faces elsewhere (boundaries)
        s2 = re.sub(r"\(\s*(\S+)\s*=\s*i1\s*\)", r"\1 = 1", s2)
        if re.match(r"\s*data\s", s2) and re.search(r"\(\s*\w+\s+\w+\s*:\s*I\s*\)", s2):
            warn(n, "a two-dimensional path constructor is not translated")
        out.append(s2)
    return "\n".join(out) + "\n"

def main(argv):
    if len(argv) < 2 or argv[1] in ("-h", "--help"): print(__doc__); return 0
    src = open(argv[1], encoding="utf-8").read()
    res = translate(src)
    if "-o" in argv:
        open(argv[argv.index("-o") + 1], "w", encoding="utf-8").write(res)
    else:
        sys.stdout.write(res)
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv))
