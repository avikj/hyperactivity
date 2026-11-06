#!/usr/bin/env python3
"""Compile-and-run check for lib/emitc.cub: a first-order program is printed by the machine as a
list of bytes, decoded here, compiled with gcc, run, and its output compared with eval-fo's."""
import re, subprocess, sys, tempfile, os, resource
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(os.path.dirname(HERE))
CUB = os.path.join(ROOT, 'build', 'cubical')
LIBS = [os.path.join(ROOT, 'lib', f) for f in ('prelude.cub', 'code.cub', 'fo.cub', 'match.cub', 'nav.cub', 'emitc.cub', 'check.cub', 'std.cub', 'stage.cub', 'lower.cub', 'specialize.cub', 'linproof.cub', 'prim.cub', 'decode.cub', 'rules.cub', 'derive.cub', 'proc.cub', 'emitt.cub', 'emitu.cub', 'repr.cub', 'image.cub', 'reimport.cub')]
PROGS = {
 # name: (definitions as .cub text building a Lst of (g , (arity , body)), expected output)
 'fact': ("""defs = lcons (#0 , (#0 , fcall #1 (lcons (flit #5) lnil)))
       (lcons (#1 , (#1 , fo-fact)) lnil)""", "120"),
 'switch': ("""data Sw = sw0 | sw1 (a : Word) (b : Word)
defs = lcons (#0 , (#0 , fswitch (fcon (didx (code Sw)) #1 (lcons (flit #9) (lcons (flit #30) lnil)))
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
 # C type declarations from B12's layouts (lib/emitt.cub): a tree's typedef compiles, its recursive
 # fields typed pointers to its own struct; its size is the tag and the larger alternative, two pointers
 'types': ("""data Tr = lf (w : Word) | nd (l : Tr) (r : Tr)
prog = sizeof-prog (data-idx (code Tr))""", "24", "struct { struct T"),
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
 # in-place update (C21): a loop over a fresh record (two word fields), its second field increased
 # by the counter each iteration; the record is the loop's own, so the rebuild (a recursion on it
 # returning its constructor with one field changed) is an update stored in place: no allocation in
 # the loop (boxed: a store into the state cell; unboxed: the flat struct by value, a field assigned).
 # 10 + 3 + 2 + 1 = 16
 'inplace': ("""data Rc2 = rc2 (x : Word) (y : Word)
rc-d = didx (code Rc2)
loop-rc = floop (lcons (flit #3) (lcons (fcon rc-d #0 (lcons (flit #0) (lcons (flit #10) lnil))) lnil))
  (fif (fprim P-lt (lcons (flit #0) (lcons (fvar #1) lnil)))
     (fcon #0 #0 (lcons (fprim P-sub (lcons (fvar #1) (lcons (flit #1) lnil))) (lcons (frec rc-d (fvar #0) (lcons (#0 , (#2 , (lnil , fcon rc-d #0 (lcons (fvar #1) (lcons (fprim P-add (lcons (fvar #0) (lcons (fvar #3) lnil))) lnil))))) lnil)) lnil)))
     (fcon #0 #1 (lcons (fvar #0) lnil)))
defs = case lift #1 (inplace loop-rc) of lifted t2 ds k → lcons (#0 , (#0 , fswitch t2 (lcons (#0 , (#2 , fvar #0)) lnil))) ds""", "16", ("V *up = v0; up->f[1] = ", "up.u.c0.f1 = ")),
 # copy-on-write: the record is a parameter (the caller may hold it), so the update copies the
 # cell first (a parameter whose only use is a switch is boxed in both printers). 5 + (6 + 1) = 12
 'cow': ("""data Pt4 = pt4 (x : Word) (y : Word)
upd-y : Pt4 → Pt4
upd-y p = case p of pt4 x y → pt4 x (%wadd y #1)
lam-body f = case f of flam b → b; _ → f
cow-main = fswitch (fcall #1 (lcons (fcon (didx (code Pt4)) #0 (lcons (flit #5) (lcons (flit #6) lnil))) lnil)) (lcons (#0 , (#2 , fprim P-add (lcons (fvar #1) (lcons (fvar #0) lnil)))) lnil)
defs = case lift #2 (lam-body (specialize (Pt4 → Pt4) lnil (code (λ (p : Pt4) → upd-y p)))) of lifted t2 ds k → lcons (#0 , (#0 , cow-main)) (lcons (#1 , (#1 , t2)) ds)""", "12", "cpc("),
 'let-if': ("""defs = lcons (#0 , (#0 , flet (flit #3) (fif (fprim P-lt (lcons (fvar #0) (lcons (flit #5) lnil))) (fprim P-mul (lcons (fvar #0) (lcons (fvar #0) lnil))) (flit #0)))) lnil""", "9"),
}
UNBOXED = os.environ.get('UNBOXED') == '1'
# ---------------------------------------------------------------- the C++ image (lib/image.cub, plan 10.1)
# name: (definitions as .cub text, the names exported as {cub name: C++ name}, a C++ main using the
# header as "image.hpp", expected output, and optionally the names whose re-import must equal the
# source code (lib/reimport.cub; the default is every name; a definition with an erased parameter
# re-imports only up to erasure and is listed as []). The header is compiled with g++ -std=c++20.
def enc(s):
    r = 'lnil'
    for b in reversed(s.encode()): r = f'(lcons #{b} {r})'
    return r
IMAGES = {
 # word functions and a function uniform in a type: a template the consumer instantiates
 'words': ("""add3i : Word → Word → Word
add3i a b = %wadd (%wadd a b) #3
twicei : (A : Type) → (A → A) → A → A
twicei A f a = f (f a)
maxi : Word → Word → Word
maxi a b = if %wlt a b then b else a""", {'add3i': 'add3', 'twicei': 'twice', 'maxi': 'maxw'},
  """#include "image.hpp"
#include <cstdio>
int main() { std::printf("%llu %llu %llu\\n", (unsigned long long) add3(1, 2), (unsigned long long) twice<uint64_t>([](uint64_t x) { return x * 10; }, 7), (unsigned long long) maxw(3, 9)); }""", "6 700 9"),
 # a data type as structs and a variant; a structural recursion as fix over visit; a constructor
 # built on the C++ side and on ours
 'list': ("""data TLi = tnili | tconsi (x : Word) (xs : TLi)
tsumi : TLi → Word
tsumi l = case l of tnili → #0; tconsi x xs → %wadd x (rec xs)
mk3i : Word → TLi
mk3i w = tconsi w (tconsi #20 (tconsi #300 tnili))""", {'tsumi': 'tsum', 'mk3i': 'mk3'},
  """#include "image.hpp"
#include <cstdio>
int main() { std::printf("%llu\\n", (unsigned long long) tsum(mk3(1))); }""", "321"),
 # the byte layer: a load at a literal offset from a buffer the consumer owns; the proof parameter
 # is absent, the bound an assume in cub::load64
 'bytes': ("""ld8i : (b : Bytes 32) → le 16 32 → Word
ld8i b h = i64toWord (load64 32 b 8 h)""", {'ld8i': 'ld8'},
  """#include "image.hpp"
#include <cstdio>
int main() { cub::bytes<32> b{}; b[8] = 42; std::printf("%llu\\n", (unsigned long long) ld8(b)); }""", "42", []),
 # word elimination over a pair: cub::welim with a lambda step; fib 90
 'fib': ("""fibpi : Word → Word × Word
fibpi n = %welimP (λ v → Word × Word) n (#0 , #1) (λ k p → (snd p , %wadd (fst p) (snd p)))
fibi : Word → Word
fibi n = fst (fibpi n)""", {'fibi': 'fib'},
  """#include "image.hpp"
#include <cstdio>
int main() { std::printf("%llu\\n", (unsigned long long) fib(90)); }""", "2880067194370816120"),
}
def run_image(name, defs, names, main, expect, validate=None):
    with tempfile.TemporaryDirectory() as d:
        cub = os.path.join(d, name + '.cub')
        tbl = 'lnil'
        for k, v in reversed(list(names.items())): tbl = f'(lcons (gidx (code {k}) , {enc(v)}) {tbl})'
        gs = 'lnil'
        for k in reversed(list(names.keys())): gs = f'(lcons (gidx (code {k})) {gs})'
        checks = ''.join(f"#same omap (λ p → snd p) (lnth (reimport-header {tbl} (fst hdr)) #{i}) = omap erase-code (unfold (gidx (code {k})))\n"
                         for i, k in enumerate(names) if validate is None or k in validate)
        open(cub, 'w').write(defs + f"\nhdr = cxx-image {tbl} lnil {gs}\n#show fst hdr\n#show report-text {tbl} (snd hdr)\n" + checks)
        out = subprocess.run([CUB] + LIBS + [cub], capture_output=True, text=True, env=dict(os.environ, CUBICAL_NOCACHE='1'),
                             preexec_fn=lambda: resource.setrlimit(resource.RLIMIT_STACK, (resource.RLIM_INFINITY, resource.RLIM_INFINITY))).stdout
        m = re.search(r'^fst hdr\s*=\s*(.*?)\s*\[\d+ steps', out, re.S | re.M)
        if not m: print(name, 'FAIL: no reading'); print(out[-500:]); return False
        bs = bytes(int(x) for x in re.findall(r'#(\d+)', m.group(1)))
        r = re.search(r'^report-text.*?=\s*(.*?)\s*\[\d+ steps', out, re.S | re.M)
        rep = bytes(int(x) for x in re.findall(r'#(\d+)', r.group(1))).decode() if r else ''
        open(os.path.join(d, 'image.hpp'), 'wb').write(bs)
        cpp = os.path.join(d, 'main.cpp'); open(cpp, 'w').write(main)
        exe = os.path.join(d, name)
        cc = subprocess.run(['g++', '-std=c++20', '-O2', '-w', '-I', d, '-o', exe, cpp], capture_output=True, text=True)
        if cc.returncode: print(name, 'FAIL: g++'); print(cc.stderr[:1200]); print(bs.decode()); print(rep); return False
        got = subprocess.run([exe], capture_output=True, text=True).stdout.strip()
        reimported = '\nFAIL' not in out
        ok = got == expect and 'not exportable' not in rep and reimported
        print(name, 'ok' if ok else f'FAIL: got {got}, expected {expect}' + ('' if reimported else '; the re-import differs from the source'))
        if not ok: print(bs.decode()); print(rep); print(out[-1500:])
        return ok
IMAGE = os.environ.get('IMAGE') == '1'

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
        if isinstance(contains, tuple): contains = contains[1 if UNBOXED else 0]   # (boxed, unboxed)
        ok = got == expect and (contains is None or contains.encode() in bs)
        print(name, 'ok' if ok else f'FAIL: got {got}, expected {expect}' + ('' if contains is None or contains.encode() in bs else f'; {contains!r} not in the source'))
        if not ok: print(bs.decode())
        return ok
if __name__ == '__main__':
    if IMAGE: sys.exit(0 if all(run_image(n, *IMAGES[n]) for n in IMAGES) else 1)
    sys.exit(0 if all(run(n, *PROGS[n]) for n in PROGS) else 1)
