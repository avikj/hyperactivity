#!/usr/bin/env python3
# lib/foproofd.cub's generated parts: FC equality (fc-eq), its soundness (fcs-*, fc-eq-sound), DTok and the
# table decision (dtok-chk, dtok-auto), over the forms listed in DC below.
# To rerun after adding a form to the data mirror: add it to DC (its fields: W a word, N a Nat, C a
# sub-form), run `python3 tools/gen_foproofd.py > /tmp/gen.cub`, and replace each definition in
# lib/foproofd.cub by the one of the same name in the output (fc-eq, the fcs-* lemmas and fc-eq-sound,
# dtok-chk and dtok-auto are one block there; DTok stands earlier, before the tables are used).
import sys
# generator for lib/foproofd.cub's boilerplate: FC equality and its soundness, the table decision
DC = [("dcl",[("w","W")]),("dcv",[("n","N")]),("dcp",[("p","W"),("a","C"),("b","C")]),("dcp1",[("p","W"),("a","C")]),
      ("dclet",[("a","C"),("b","C")]),("dcu",[]),("dcpr",[("a","C"),("b","C")]),("dcfst",[("a","C")]),("dcsnd",[("a","C")]),
      ("dccon",[("d","W"),("k","W"),("a","C")]),("dccase",[("s","C"),("alts","C")]),("dcalt",[("k","W"),("n","N"),("b","C"),("r","C")]),
      ("dcnone",[]),("dcif",[("c","C"),("t","C"),("e","C")]),("dclam",[("i","W"),("b","C")]),("dcapp",[("f","C"),("a","C")]),
      ("dccall",[("g","W"),("m","W"),("a","C")]),("dcby",[("a","C")]),("dcld",[("w","W"),("b","C"),("o","C")]),
      ("dcst",[("w","W"),("b","C"),("o","C"),("v","C")]),("dcblen",[("b","C")])]
def fcname(n): return "fcclo" if n=="dclam" else "fc"+n[2:]
FC = [(fcname(n), [] if n=="dclam" else fs) for n,fs in DC]
FC = [(n, [("i","W")] if n=="fcclo" else fs) for n,fs in FC]
TY = {"W":"Word","N":"Nat","C":"FC"}
EQ = {"W":"weq","N":"neqb"}
SO = {"W":"weq-sound","N":"neqb-sound"}
out=[]
def conj(es):
    if not es: return "true"
    if len(es)==1: return es[0]
    return "andbt %s (%s)" % (es[0], conj(es[1:])) if len(es)>2 else "andbt %s %s" % (es[0], es[1])
def eqexpr(t,x,y):
    return "(fc-eq %s %s)"%(x,y) if t=="C" else "(%s %s %s)"%(EQ[t],x,y)
def pat(n,fs,v): return "("+" ".join([n]+["%s%d"%(v,i) for i in range(len(fs))])+")" if fs else n
# fc-eq
out.append("fc-eq : FC → FC → Bool")
for n,fs in FC:
    out.append("fc-eq %s y = case y return (λ z → Bool) of" % pat(n,fs,"x"))
    for m,gs in FC:
        if m==n:
            es=[eqexpr(t,"x%d"%i,"y%d"%i) for i,(f,t) in enumerate(fs)]
            out.append("    %s → %s" % (pat(m,gs,"y")[1:-1] if gs else m, conj(es)))
        else:
            out.append("    %s → false" % (pat(m,gs,"y")[1:-1] if gs else m))
# soundness lemmas
for n,fs in FC:
    args=" ".join("(x%d : %s)"%(i,TY[t]) for i,(f,t) in enumerate(fs))
    hyps=" → ".join("((y : FC) → fc-eq x%d y ≡ true → x%d ≡ y)"%(i,i) for i,(f,t) in enumerate(fs) if t=="C")
    lhs=pat(n,fs,"x")
    sig="fcs-%s : %s%s(y : FC) → fc-eq %s y ≡ true → %s ≡ y" % (n, (args+" → ") if args else "", (hyps+" → ") if hyps else "", lhs, lhs)
    out.append(sig)
    hn=" ".join("h%d"%i for i,(f,t) in enumerate(fs) if t=="C")
    xs=" ".join("x%d"%i for i in range(len(fs)))
    out.append("fcs-%s %s %s y = case y return (λ z → fc-eq %s z ≡ true → %s ≡ z) of" % (n, xs, hn, lhs, lhs))
    for m,gs in FC:
        p = pat(m,gs,"y")[1:-1] if gs else m
        if m!=n:
            out.append("    %s → λ e → Empty-rec (false≢true e)" % p); continue
        es=[eqexpr(t,"x%d"%i,"y%d"%i) for i,(f,t) in enumerate(fs)]
        if not fs:
            out.append("    %s → λ e i → %s" % (p, n)); continue
        parts=[]
        for i,(f,t) in enumerate(fs):
            # proof of es[i] ≡ true from e : conj(es) ≡ true
            pr="e"
            for j in range(i):
                pr="(andbt-r %s (%s) %s)"%(es[j], conj(es[j+1:]), pr) if len(es[j+1:])>1 else "(andbt-r %s %s %s)"%(es[j], conj(es[j+1:]), pr)
            if i < len(es)-1:
                rest=conj(es[i+1:])
                rest = "(%s)"%rest if len(es[i+1:])>1 else rest
                pr="(andbt-l %s %s %s)"%(es[i], rest, pr)
            if t=="C": parts.append("(h%d y%d %s i)"%(i,i,pr))
            else: parts.append("(%s x%d y%d %s i)"%(SO[t],i,i,pr))
        out.append("    %s → λ e i → %s %s" % (p, n, " ".join(parts)))
out.append("fc-eq-sound : (x y : FC) → fc-eq x y ≡ true → x ≡ y")
for n,fs in FC:
    rec=" ".join("(fc-eq-sound x%d)"%i for i,(f,t) in enumerate(fs) if t=="C")
    out.append("fc-eq-sound %s = fcs-%s %s %s" % (pat(n,fs,"x"), n, " ".join("x%d"%i for i in range(len(fs))), rec))
# DTok, dtok-chk, dtok-auto
def ttree(xs):
    if not xs: return "Unit"
    if len(xs)==1: return xs[0]
    return "%s × %s"%(xs[0], ("(%s)"%ttree(xs[1:])) if len(xs)>2 else ttree(xs[1:]))
out2=["DTok : (Word → FC) → DC → Type"]
out3=["dtok-chk : (Word → FC) → DC → Bool"]
out4=["dtok-auto : (T : Word → FC) (c : DC) → dtok-chk T c ≡ true → DTok T c"]
for n,fs in DC:
    p=pat(n,fs,"x")
    ch=["x%d"%i for i,(f,t) in enumerate(fs) if t=="C"]
    items=["DTok T %s"%c for c in ch]
    bools=["(dtok-chk T %s)"%c for c in ch]
    if n=="dclam":
        items=["(T x0 ≡ dconv x1)"]+items
        bools=["(fc-eq (T x0) (dconv x1))"]+bools
    out2.append("DTok T %s = %s"%(p, ttree(items)))
    out3.append("dtok-chk T %s = %s"%(p, conj(bools)))
    # auto: build tuple of proofs
    def prf(i):
        pr="e"
        for j in range(i):
            rest=conj(bools[j+1:]); rest="(%s)"%rest if len(bools[j+1:])>1 else rest
            pr="(andbt-r %s %s %s)"%(bools[j], rest, pr)
        if i < len(bools)-1:
            rest=conj(bools[i+1:]); rest="(%s)"%rest if len(bools[i+1:])>1 else rest
            pr="(andbt-l %s %s %s)"%(bools[i], rest, pr)
        return pr
    comps=[]
    for i,b in enumerate(bools):
        if n=="dclam" and i==0: comps.append("fc-eq-sound (T x0) (dconv x1) %s"%prf(i))
        else:
            c = ch[i-1] if n=="dclam" else ch[i]
            comps.append("dtok-auto T %s %s"%(c, prf(i)))
    def tup(xs):
        if not xs: return "unit"
        if len(xs)==1: return xs[0]
        return "(%s , %s)"%(xs[0], tup(xs[1:]))
    out4.append("dtok-auto T %s e = %s"%(p, tup(comps)))
sys.stdout.write("\n".join(out)+"\n")
sys.stdout.write("\n".join(out2+out3+out4)+"\n")

