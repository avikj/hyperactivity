#!/usr/bin/env python3
"""Compile-and-run check for lib/emitc.cub: a first-order program is printed by the machine as a
list of bytes, decoded here, compiled with gcc, run, and its output compared with eval-fo's."""
import re, subprocess, sys, tempfile, os, resource
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(os.path.dirname(HERE))
CUB = os.path.join(ROOT, 'build', 'cubical')
LIBS = [os.path.join(ROOT, 'lib', f) for f in ('prelude.cub', 'code.cub', 'fo.cub', 'match.cub', 'nav.cub', 'emitc.cub', 'check.cub', 'std.cub', 'lower.cub', 'specialize.cub', 'linproof.cub', 'prim.cub', 'decode.cub', 'rules.cub', 'derive.cub', 'proc.cub', 'emitt.cub', 'emitu.cub', 'repr.cub')]
PROGS = {
 # name: (definitions as .cub text building a Lst of (g , (arity , body)), expected output)
 'fact': ("""defs = lcons (#0 , (#0 , fcall #1 (lcons (flit #5) lnil)))
       (lcons (#1 , (#1 , fo-fact)) lnil)""", "120"),
 'switch': ("""defs = lcons (#0 , (#0 , fswitch (fcon #7 #1 (lcons (flit #9) (lcons (flit #30) lnil)))
                                 (lcons (#0 , (#0 , flit #0)) (lcons (#1 , (#2 , fprim P-add (lcons (fvar #1) (lcons (fvar #0) lnil)))) lnil)))) lnil""", "39"),
 # a code specialized by the machine (lib/specialize.cub), then printed and compiled: 2^n as a loop
 'specialized': ("""lam-body f = case f of flam b → b; _ → f
defs = lcons (#0 , (#0 , fcall #1 (lcons (flit #6) lnil)))
       (lcons (#1 , (#1 , lam-body (specialize (Word → Word) lnil c5))) lnil)""", "64"),
 # a closure capturing a let-bound value, lifted to its own definition and applied through the dispatcher
 'closure': ("""defs = case lift #1 t-clo of lifted t2 ds k → lcons (#0 , (#0 , t2)) ds""", "15"),
 # a law from the store (plusw-zero-l, a checked theorem) consumed by the specializer: the body
 # lowered and compiled is the identity, not the recursion plusw unfolds to
 'law': ("""lam-body f = case f of flam b → b; _ → f
zl = fromopt (rule-of (gidx (code plusw-zero-l))) (rule #0 cword cword)
spec-id = specialize (Word → Word) (lcons zl lnil) (clam (capp (capp (code plusw) (cwlit #0)) (cvar #0)))
#same spec-id = flam (fvar #0)
defs = lcons (#0 , (#0 , fcall #1 (lcons (flit #7) lnil))) (lcons (#1 , (#1 , lam-body spec-id)) lnil)""", "7"),
 # an assumed fact (a validity fact a certificate established, lib/lin.cub) printed as ASSUME:
 # the C compiler is told what the checker knows
 'assume': ("""defs = lcons (#0 , (#0 , flet (flit #3) (fassume (cwop #1 (cvar #0) (cwlit #10)) (fprim P-mul (lcons (fvar #0) (lcons (fvar #0) lnil)))))) lnil""", "9", "ASSUME((v0"),
 # C type declarations from B12's layouts (lib/emitt.cub): a tree's typedef compiles; its size is
 # the tag and the larger alternative, two pointers
 'types': ("""data Tr = lf (w : Word) | nd (l : Tr) (r : Tr)
prog = sizeof-prog (data-idx (code Tr))""", "24", "struct { void *f0; void *f1; } c1;"),
 # the byte layer: a 32-byte buffer, 9 stored at 16 (64 bits) and 258 at 4 (32 bits), read back
 'bytes': ("""defs = lcons (#0 , (#0 , flet (fprim P-store32 (lcons (fprim P-store64 (lcons (fprim P-bytes (lcons (flit #32) lnil)) (lcons (flit #16) (lcons (flit #9) lnil)))) (lcons (flit #4) (lcons (flit #258) lnil))))
  (fprim P-add (lcons (fprim P-load64 (lcons (fvar #0) (lcons (flit #16) lnil))) (lcons (fprim P-load32 (lcons (fvar #0) (lcons (flit #4) lnil))) lnil))))) lnil""", "267"),
 # a code over the byte layer (two loads from an unknown buffer, with their bound proofs and Nat
 # lengths) lowered with the representations consumed (lib/repr.cub), compiled, run on a buffer
 'loads': ("""lam-body f = case f of flam b → b; _ → f
defs = lcons (#0 , (#0 , fcall #1 (lcons buf2 lnil))) (lcons (#1 , (#1 , lam-body sum2)) lnil)""", "14", "ASSUME(((16 + 8) < (32 + 1)));"),
 # structural recursion: the length of a list, written as a case with rec, lowered to a recursion
 # form, lifted to a C function that calls itself on the field, run on a three-element list
 'rec': ("""len-main t2 = fapply t2 (lcons (fcon (didx (code LW)) #1 (lcons (flit #7) (lcons lw3 lnil))) lnil)
len-base = lcons (gidx (code zw) , (#0 , flit #0)) (lcons (gidx (code sw) , (#1 , fprim P-add (lcons (fvar #0) (lcons (flit #1) lnil)))) lnil)
len-prog = case lift #10 (lower c-len) of lifted t2 ds k → lcons (#0 , (#0 , len-main t2)) (lapp2 len-base ds)
prog = emit-prog len-prog""", "3", "V *v3 = f_10(v2); f_"),
 # a derived program compiled: the equality on Nat read off the data table (lib/derive.cub),
 # lowered, lifted (the recursion a C function, the inner λ a closure through the dispatcher),
 # applied to 2 and 2 then to 2 and 1; Bool is a word in the form: 10*eq(2,2) + eq(2,1)
 'deq': ("""deq-main t2 = fprim P-add (lcons (fprim P-mul (lcons (flit #10) (lcons (fapply t2 (lcons n2 (lcons n2 lnil))) lnil))) (lcons (fapply t2 (lcons n2 (lcons n1 lnil))) lnil))
deq-prog = case lift #10 eq-fo of lifted t2 ds k → lcons (#0 , (#0 , deq-main t2)) ds
prog = emit-prog deq-prog""", "10"),
 # a whole program from the store: the code calls lenw, a recursive definition of lib/lower.cub;
 # program-of lowers the code and then lenw from the quote of its value; lifted; compiled; the
 # main applies definition 0 to a three-element list
 'program': ("""prog-defs = lift-program #100000 (program-at #99999 #10 (code (λ (l : LW) → lenw l)))
prog = emit-prog (lcons (#0 , (#0 , fcall #99999 (lcons (fcon (didx (code LW)) #1 (lcons (flit #7) (lcons lw3 lnil))) lnil))) prog-defs)""", "3"),
 # a process run against its world (lib/proc.cub), specialized under an unknown store: the run
 # becomes a function of the store (st 3 + 1); compiled with a store that answers 40; prints 41
 'process': ("""proc-defs = case lift #100 spec4 of lifted t2 ds k → lcons (#0 , (#0 , fapply t2 (lcons (flam (flit #40)) lnil))) ds
prog = case lift #200 (snd (snd (fromopt (lnth proc-defs #0) (#0 , (#0 , flit #0))))) of lifted m ds k → emit-prog (lcons (#0 , (#0 , m)) (lapp2 ds (ltail proc-defs)))
ltail l = case l of lnil → lnil; lcons x xs → xs""", "41"),
 # fib by word elimination over a pair, specialized under an unknown argument to a loop; fib 90
 'fib': ("""fibp : Word → Word × Word
fibp n = %welimP (λ v → Word × Word) n (#0 , #1) (λ k p → (snd p , %wadd (fst p) (snd p)))
fibw : Word → Word
fibw n = fst (fibp n)
lam-body2 f = case f of flam b → b; _ → f
defs = lcons (#0 , (#0 , fcall #1 (lcons (flit #90) lnil))) (lcons (#1 , (#1 , lam-body2 (specialize (Word → Word) lnil (code (λ w → fibw w))))) lnil)""", "2880067194370816120"),
 # the tagged message decoder (lib/decode.cub) lowered from its code with the representations
 # consumed: type 1 reads the price at 8 (500), type 2 the quantity at 16 (7); 1000*first + second
 'itch': ("""itch-main = fprim P-add (lcons (fprim P-mul (lcons (flit #1000) (lcons (fapply dm-fo (lcons mb1 lnil)) lnil))) (lcons (fapply dm-fo (lcons mb2 lnil)) lnil))
defs = case lift #100 dm-fo of lifted t2 ds k → lcons (#0 , (#0 , fprim P-add (lcons (fprim P-mul (lcons (flit #1000) (lcons (fapply t2 (lcons mb1 lnil)) lnil))) (lcons (fapply t2 (lcons mb2 lnil)) lnil)))) ds""", "500007", "ASSUME(((8 + 8) < (32 + 1)))"),
 # a flat record (two word fields) by value in the unboxed printer: a compound literal, a switch
 # on its tag binding the fields as uint64_t; the boxed printer allocates it. Sum of 3 and 4
 'record': ("""data Pt2 = pt2 (x : Word) (y : Word)
pt-d = didx (code Pt2)
defs = lcons (#0 , (#0 , fswitch (fcon pt-d #0 (lcons (flit #3) (lcons (flit #4) lnil))) (lcons (#0 , (#2 , fprim P-add (lcons (fvar #1) (lcons (fvar #0) lnil)))) lnil))) lnil""", "7"),
 # the fixed-width and floating-point forms (B14): 7 as Int64 to Float64, times 3.0 (a Float64
 # literal is its bits), back, as a word (21), plus ten times the signed comparison -1 < 1 (the word 2^64-1 read as Int64): 31
 'numeric': ("""defs = lcons (#0 , (#0 , fprim P-add (lcons (fprim P-wofi64 (lcons (fprim P-i64off64 (lcons (fprim P-f64mul (lcons (fprim P-f64ofi64 (lcons (fi64 #7) lnil)) (lcons (ff64 #4613937818241073152) lnil))) lnil)) lnil))
  (lcons (fprim P-mul (lcons (flit #10) (lcons (fprim P-i64lt (lcons (fi64 #18446744073709551615) (lcons (fi64 #1) lnil))) lnil))) lnil)))) lnil""", "31"),
 # the same from a typed code: lib/prim.cub's Int64 and Float64 primitives lowered to the numeric
 # primitives by lib/repr.cub, the argument an Int64 literal
 'num-spec': ("""lam-body f = case f of flam b → b; _ → f
spec-num = specialize-r (Int64 → Word) lnil (code (λ x → wadd (i64toWord (i64off64 (f64mul (f64ofi64 x) (f64 #3)))) (if i64lt x (i64 #1) then #10 else #0)))
defs = lcons (#0 , (#0 , fcall #1 (lcons (fi64 #7) lnil))) (lcons (#1 , (#1 , lam-body spec-num)) lnil)""", "21", "n27("),
 'let-if': ("""defs = lcons (#0 , (#0 , flet (flit #3) (fif (fprim P-lt (lcons (fvar #0) (lcons (flit #5) lnil))) (fprim P-mul (lcons (fvar #0) (lcons (fvar #0) lnil))) (flit #0)))) lnil""", "9"),
}
UNBOXED = os.environ.get('UNBOXED') == '1'
def run(name, defs, expect, contains=None):
    with tempfile.TemporaryDirectory() as d:
        cub = os.path.join(d, name + '.cub')
        if 'prog =' not in defs: defs += "\nprog = " + ("emit-prog-u defs" if UNBOXED else "emit-prog defs")
        open(cub, 'w').write(defs + "\n#show prog\n")
        # the printer of src/syntax.c (pr → pr_flat → pr_str → pr) spends a stack frame of a few KB per
        # nesting level, and a program is a byte list nested once per byte: a 4000-byte program
        # overflows the default 8 MB stack. Raised here until the printer iterates on the last argument.
        out = subprocess.run([CUB] + LIBS + [cub], capture_output=True, text=True, env=dict(os.environ, CUBICAL_NOCACHE='1'),
                             preexec_fn=lambda: resource.setrlimit(resource.RLIMIT_STACK, (resource.RLIM_INFINITY, resource.RLIM_INFINITY))).stdout
        m = re.search(r'^prog\s*=\s*(.*?)\s*\[\d+ steps', out, re.S | re.M)
        if not m: print(name, 'FAIL: no reading'); print(out[-500:]); return False
        bs = bytes(int(x) for x in re.findall(r'#(\d+)', m.group(1)))
        c = os.path.join(d, name + '.c'); open(c, 'wb').write(bs)
        exe = os.path.join(d, name)
        cc = subprocess.run(['gcc', '-O2', '-w', '-o', exe, c], capture_output=True, text=True)
        if cc.returncode: print(name, 'FAIL: gcc'); print(cc.stderr[:800]); print(bs.decode()); return False
        got = subprocess.run([exe], capture_output=True, text=True).stdout.strip()
        ok = got == expect and (contains is None or contains.encode() in bs)
        print(name, 'ok' if ok else f'FAIL: got {got}, expected {expect}' + ('' if contains is None or contains.encode() in bs else f'; {contains!r} not in the source'))
        if not ok: print(bs.decode())
        return ok
if __name__ == '__main__':
    sys.exit(0 if all(run(n, *PROGS[n]) for n in PROGS) else 1)
