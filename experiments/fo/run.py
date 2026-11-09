#!/usr/bin/env python3
"""Compile-and-run check for lib/emitc.cub: a first-order program is printed by the machine as a
list of bytes, decoded here, compiled with gcc, run, and its output compared with eval-fo's."""
import re, subprocess, sys, tempfile, os, resource, time
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(os.path.dirname(HERE))
CUB = os.path.join(ROOT, 'build', 'cubical')
# The libraries and the proof files every program imports are checked once per build and then read from
# the store's cache: its key mixes each file's text with the binary's build time, so a rebuilt binary or
# an edited file is checked afresh. The programs' own files are new each run, so they are always
# checked. RUNPY_NOCACHE=1 checks everything every time (the old behaviour).
_cenv = {k: v for k, v in os.environ.items() if k != 'CUBICAL_NOCACHE'}
_cenv['CUBICAL_CACHE'] = os.path.join(ROOT, 'build', 'runpy-cache')
CUB_ENV = dict(os.environ, CUBICAL_NOCACHE='1') if os.environ.get('RUNPY_NOCACHE') == '1' else _cenv
os.makedirs(_cenv['CUBICAL_CACHE'], exist_ok=True)
def FRESH():   # a first line unique to this run, so a program's own file is never read from the cache
    return f"-- run {os.getpid()} {time.time_ns()}\n"
LIBS = [os.path.join(ROOT, 'lib', f) for f in ('prelude.cub', 'code.cub', 'fo.cub', 'match.cub', 'nav.cub', 'emitc.cub', 'check.cub', 'std.cub', 'stage.cub', 'lower.cub', 'specialize.cub', 'linproof.cub', 'prim.cub', 'decode.cub', 'rules.cub', 'derive.cub', 'proc.cub', 'emitt.cub', 'emitu.cub', 'repr.cub', 'image.cub', 'reimport.cub', 'emitx86.cub', 'region.cub')]
PROGS = {
 # word subtraction is the source's truncating one: 3 - 5 is 0, not 2^64 - 2 (the C printers emitted a
 # wrapping subtraction until 2026-11-08), boxed and unboxed; X86_PROGS has its machine-code twin
 # max on words, lowered from a specialized source program: at 3, max 3 (3 + 1) = 4 (the lowering
 # compiled %wmax as an addition until 2026-11-07)
 'maxw': ("""mx-defs = lift-program #100000 (program-rep-at (Word → Word) #99999 lnil #20 (code (λ (a : Word) → %wmax a (%wadd a #1))))
defs = lcons (#0 , (lnil , fcall #99999 (lcons (flit #3) lnil))) mx-defs""", "4"),
 'tsub': ("""defs = lcons (#0 , (lnil , fcall #1 (lcons (flit #5) lnil))) (lcons (#1 , (lcons ftword lnil , fprim P-add (lcons (fprim P-sub (lcons (flit #3) (lcons (fvar #0) lnil))) (lcons (flit #7) lnil)))) lnil)""", "7"),
 # name: (definitions as .cub text building a Lst of (g , (arity , body)), expected output)
 'fact': ("""defs = lcons (#0 , (lnil , fcall #1 (lcons (flit #5) lnil)))
       (lcons (#1 , (lcons ftword lnil , fo-fact)) lnil)""", "120"),
 'switch': ("""data Sw = sw0 | sw1 (a : Word) (b : Word)
defs = lcons (#0 , (lnil , fswitch (fcon (didx (code Sw)) #1 (lcons (flit #9) (lcons (flit #30) lnil)))
                                 (lcons (#0 , (#0 , flit #0)) (lcons (#1 , (#2 , fprim P-add (lcons (fvar #1) (lcons (fvar #0) lnil)))) lnil)))) lnil""", "39"),
 # a code specialized by the machine (lib/specialize.cub), then printed and compiled: 2^n as a loop
 'specialized': ("""defs = lcons (#0 , (lnil , fcall #1 (lcons (flit #6) lnil)))
       (lcons (fo-def #1 (specialize (Word → Word) lnil c5)) lnil)""", "64"),
 # a closure capturing a let-bound value, lifted to its own definition and applied through the dispatcher
 'closure': ("""defs = case lift #1 t-clo of lifted t2 ds k → lcons (#0 , (lnil , t2)) ds""", "15"),
 # a thunk (fthunk/fforce): lifted to a closure of one ignored argument, forced by force() (its application to 0)
 'thunk': ("""defs = case lift #1 t-thunk of lifted t2 ds k → lcons (#0 , (lnil , t2)) ds""", "22"),
 # a law from the store (plusw-zero-l, a checked theorem) consumed by the specializer: the body
 # lowered and compiled is the identity, not the recursion plusw unfolds to
 'law': ("""zl = fromopt (rule-of (gidx (code plusw-zero-l))) (rule #0 cword cword)
spec-id = specialize (Word → Word) (lcons zl lnil) (clam (capp (capp (code plusw) (cwlit #0)) (cvar #0)))
#same spec-id = flam ftword (fvar #0)
defs = lcons (#0 , (lnil , fcall #1 (lcons (flit #7) lnil))) (lcons (fo-def #1 spec-id) lnil)""", "7"),
 # an assumed fact (a validity fact a certificate established, lib/lin.cub) printed as ASSUME:
 # the C compiler is told what the checker knows
 'assume': ("""defs = lcons (#0 , (lnil , flet ftword (flit #3) (fassume (cwop #1 (cvar #0) (cwlit #10)) (fprim P-mul (lcons (fvar #0) (lcons (fvar #0) lnil)))))) lnil""", "9", "ASSUME((v0"),
 # C type declarations from B12's layouts (lib/emitt.cub): a tree's typedef compiles, its recursive
 # fields typed pointers to its own struct; its size is the tag and the larger alternative, two pointers
 'types': ("""data Tr = lf (w : Word) | nd (l : Tr) (r : Tr)
prog = sizeof-prog (data-idx (code Tr))""", "24", "struct { struct T"),
 # the byte layer: a 32-byte buffer, 9 stored at 16 (64 bits) and 258 at 4 (32 bits), read back
 'bytes': ("""defs = lcons (#0 , (lnil , flet (ftbytes #32) (fprim P-store32 (lcons (fprim P-store64 (lcons (fprim P-bytes (lcons (flit #32) lnil)) (lcons (flit #16) (lcons (flit #9) lnil)))) (lcons (flit #4) (lcons (flit #258) lnil))))
  (fprim P-add (lcons (fprim P-load64 (lcons (fvar #0) (lcons (flit #16) lnil))) (lcons (fprim P-load32 (lcons (fvar #0) (lcons (flit #4) lnil))) lnil))))) lnil""", "267"),
 # in place on the byte layer (C21, item 22): a loop filling an 8-word buffer that is its own
 # state stores into it without copying (st64i); 7 + 3 read back
 'inplace-bytes': ("defs = lcons (#0 , (lnil , inplace loop-buf)) lnil", "10", "st64i("),
 # the Region threading idiom (C20, lib/region.cub): a program over a 16-byte region, two writes
 # and a sum of reads, specialized to the first-order form; the region is a fresh buffer, so the
 # stores on it are in place (st64i); 20 + 21
 'region': ('''p0 : Ptr 16
p0 = (0 , (8 , λ i → 16))
p8 : Ptr 16
p8 = (8 , (0 , λ i → 16))
two : Word → RM 16 Int64
two w = rthen (wr p0 (i64 w)) (rthen (wr p8 (i64 (%wsucc w))) (rbind (rd p0) (λ a → rmap (λ b → i64add a b) (rd p8))))
prog-two : Word → Word
prog-two w = i64toWord (eval-fresh 16 (two w))
sp = specialize-r (Word → Word) lnil (code (λ (w : Word) → prog-two w))
defs = lcons (#0 , (lnil , fcall #1 (lcons (flit #20) lnil))) (lift-program #2 (lcons (fo-def #1 (inplace sp)) lnil))''', "41", "st64i("),
 # Fin n → A beyond Int64 (item 22): Float64 elements stored by their bits (updateF is store64 at
 # f64bits, indexF f64ofbits after load64), specialized from the typed program; the buffer is fresh,
 # so both stores are in place (st64i); 7.0 * 3.0 read back as a word
 'farray': ('''fa : Word → Word
fa w = i64toWord (i64off64 (f64mul (indexF 4 (updateF 4 (updateF 4 (anew 4) 0 (3 , λ i → 4) (f64ofi64 (i64 w))) 1 (2 , λ i → 4) (f64 #3)) 0 (3 , λ i → 4))
                                   (indexF 4 (updateF 4 (updateF 4 (anew 4) 0 (3 , λ i → 4) (f64ofi64 (i64 w))) 1 (2 , λ i → 4) (f64 #3)) 1 (2 , λ i → 4))))
sp = specialize-r (Word → Word) lnil (code (λ (w : Word) → fa w))
defs = lcons (#0 , (lnil , fcall #1 (lcons (flit #7) lnil))) (lift-program #2 (lcons (fo-def #1 (inplace sp)) lnil))''', "21", "st64i("),
 # UInt32 elements on the 4-byte layer: a loop-free fill of two of four elements, stored in place
 # (st32i), summed: 40 + 2
 'u32array': ('''u32a : Word → Word
u32a w = u32toWord (u32add (index32 4 (update32 4 (update32 4 (anew32 4) 3 (0 , λ i → 4) (u32 w)) 1 (2 , λ i → 4) (u32 #2)) 3 (0 , λ i → 4))
                          (index32 4 (update32 4 (update32 4 (anew32 4) 3 (0 , λ i → 4) (u32 w)) 1 (2 , λ i → 4) (u32 #2)) 1 (2 , λ i → 4)))
sp = specialize-r (Word → Word) lnil (code (λ (w : Word) → u32a w))
defs = lcons (#0 , (lnil , fcall #1 (lcons (flit #40) lnil))) (lift-program #2 (lcons (fo-def #1 (inplace sp)) lnil))''', "42", "st32i("),
 # Float32 elements by their bits on the 4-byte layer (updateF32 / indexF32: f32bits and f32ofbits
 # the identity on the word of bits in the first-order form): w's bits at 2 and 1.0's at 0, stored
 # in place (st32i); read back as their patterns and summed: w + 1065353216
 'f32array': ('''f32a : Word → Word
f32a w = u32toWord (u32add (f32bits (indexF32 4 (updateF32 4 (updateF32 4 (anew32 4) 2 (1 , λ i → 4) (f32ofbits (u32 w))) 0 (3 , λ i → 4) (f32ofbits (u32 #1065353216))) 2 (1 , λ i → 4)))
                           (f32bits (indexF32 4 (updateF32 4 (updateF32 4 (anew32 4) 2 (1 , λ i → 4) (f32ofbits (u32 w))) 0 (3 , λ i → 4) (f32ofbits (u32 #1065353216))) 0 (3 , λ i → 4))))
sp = specialize-r (Word → Word) lnil (code (λ (w : Word) → f32a w))
defs = lcons (#0 , (lnil , fcall #1 (lcons (flit #5) lnil))) (lift-program #2 (lcons (fo-def #1 (inplace sp)) lnil))''', "1065353221", "st32i("),
 # UInt32 arithmetic wraps at 2^32 (the machine's u32 operations; until 2026-11-09 the lowering used the
 # word ones, so (2^32 - 1) + 3 carried to 2^32 + 2, 3 - 5 truncated to 0, 2^16 * 2^16 was 2^32 and
 # u32 (w + 2^32) kept the high bit): 2 + (2^32 - 2) + 0 + 3 at w = 3
 'u32wrap': ("""u32o : Word → Word
u32o w = wadd (wadd (u32toWord (u32add (u32 #4294967295) (u32 w))) (u32toWord (u32sub (u32 w) (u32 #5))))
              (wadd (u32toWord (u32mul (u32 (wadd w #65533)) (u32 #65536))) (u32toWord (u32 (wadd w #4294967296))))
sp = specialize-r (Word → Word) lnil (code (λ (w : Word) → u32o w))
defs = lcons (#0 , (lnil , fcall #1 (lcons (flit #3) lnil))) (lift-program #2 (lcons (fo-def #1 sp) lnil))""", "4294967299"),
 # u32 comparisons are on the 32 bits: 2^32 + 1 as a UInt32 is 1, not above 5 (10 + 0 at w = 2^32 + 1)
 'u32cmp': ("""u32c : Word → Word
u32c w = wadd (if u32eq (u32 w) (u32 #1) then #10 else #0) (if u32lt (u32 #5) (u32 w) then #100 else #0)
sp = specialize-r (Word → Word) lnil (code (λ (w : Word) → u32c w))
defs = lcons (#0 , (lnil , fcall #1 (lcons (flit #4294967297) lnil))) (lift-program #2 (lcons (fo-def #1 sp) lnil))""", "10"),
 # signatures from the declared types (item 14, lib/emitu.cub's emit-prog-d): selw's second
 # parameter is only returned and keep5's only passed through; from their checked types they are
 # uint64_t and the flat record by value. sig-use prints the same program with emit-prog-u, which
 # reads the binders' types (the type-directed lowering's): the same signatures. Until 2026-11-09
 # emit-prog-u typed a parameter by its first use, so these two were boxed (V *, the record boxed
 # and unboxed around the call). 3 + 1 + (5 + 10) = 19
 'sig-decl': ('''selw : Word → Word → Word
selw a b = if %wlt a #10 then b else a
useselw : Word → Word
useselw w = %wadd (selw w #3) #1
data Pt5 = pt5 (x : Word) (y : Word)
keep5 : Pt5 → Pt5
keep5 p = p
sum5 : Pt5 → Word
sum5 p = case keep5 p of pt5 x y → %wadd x y
sd-defs = lift-program #100000 (program-rep-at (Word → Word) #99999 lnil #20 (code (λ (n : Word) → %wadd (useselw n) (sum5 (pt5 n #10)))))
defs = lcons (#0 , (lnil , fcall #99999 (lcons (flit #5) lnil))) sd-defs
prog = emit-prog-d #99999 defs''', "19", r"re:(?s)(?=.*uint64_t f_\d+\(uint64_t v0, uint64_t v1\) \{)(?=.*T\d+ f_\d+\(T\d+ v0\) \{ return v0; \})"),
 'sig-use': ('''selw2 : Word → Word → Word
selw2 a b = if %wlt a #10 then b else a
useselw2 : Word → Word
useselw2 w = %wadd (selw2 w #3) #1
data Pt6 = pt6 (x : Word) (y : Word)
keep6 : Pt6 → Pt6
keep6 p = p
sum6 : Pt6 → Word
sum6 p = case keep6 p of pt6 x y → %wadd x y
su-defs = lift-program #100000 (program-rep-at (Word → Word) #99999 lnil #20 (code (λ (n : Word) → %wadd (useselw2 n) (sum6 (pt6 n #10)))))
defs = lcons (#0 , (lnil , fcall #99999 (lcons (flit #5) lnil))) su-defs
prog = emit-prog-u defs''', "19", r"re:(?s)(?=.*uint64_t f_\d+\(uint64_t v0, uint64_t v1\) \{)(?=.*T\d+ f_\d+\(T\d+ v0\) \{ return v0; \})"),
 # a code over the byte layer (two loads from an unknown buffer, with their bound proofs and Nat
 # lengths) lowered with the representations consumed (lib/repr.cub), compiled, run on a buffer
 'loads': ("""defs = lcons (#0 , (lnil , fcall #1 (lcons buf2 lnil))) (lcons (fo-def #1 sum2) lnil)""", "14", "ASSUME(((16 + 8) < (32 + 1)));"),
 # structural recursion: the length of a list, written as a case with rec, lowered to a recursion
 # form, lifted to a C function that calls itself on the field, run on a three-element list; the
 # hypothesis is read as the direct recursive call (through force(), the identity on its value);
 'rec': ("""len-main t2 = fapply t2 (lcons (fcon (didx (code LW)) #1 (lcons (flit #7) (lcons lw3 lnil))) lnil)
len-base = lcons (gidx (code zw) , (lnil , flit #0)) (lcons (gidx (code sw) , (lcons ftword lnil , fprim P-add (lcons (fvar #0) (lcons (flit #1) lnil)))) lnil)
len-prog = case lift #10 (lower (LW → Word) c-len) of lifted t2 ds k → lcons (#0 , (lnil , len-main t2)) (lapp2 len-base ds)
prog = emit-prog len-prog""", "3", r"re:f_\d+\(force\(f_\d+\(v2\)\)\)"),
 # a derived program compiled: the equality on Nat read off the data table (lib/derive.cub),
 # lowered, lifted (the recursion a C function, the inner λ a closure through the dispatcher),
 # applied to 2 and 2 then to 2 and 1; Bool is a word in the form: 10*eq(2,2) + eq(2,1)
 'deq': ("""deq-main t2 = fprim P-add (lcons (fprim P-mul (lcons (flit #10) (lcons (fapply t2 (lcons n2 (lcons n2 lnil))) lnil))) (lcons (fapply t2 (lcons n2 (lcons n1 lnil))) lnil))
deq-prog = case lift #10 eq-fo of lifted t2 ds k → lcons (#0 , (lnil , deq-main t2)) ds
prog = emit-prog deq-prog""", "10"),
 # a whole program from the store: the code calls lenw, a recursive definition of lib/lower.cub;
 # program-of lowers the code and then lenw from the quote of its value; lifted; compiled; the
 # main applies definition 0 to a three-element list
 'program': ("""prog-defs = lift-program #100000 (program-at (LW → Word) #99999 #10 (code (λ (l : LW) → lenw l)))
prog = emit-prog (lcons (#0 , (lnil , fcall #99999 (lcons (fcon (didx (code LW)) #1 (lcons (flit #7) (lcons lw3 lnil))) lnil))) prog-defs)""", "3"),
 # representations consumed in general (B12 §6, lib/repr.cub's program-rep): a program over Nat
 # (plus, a case on Nat, a list of Nats) with Nat laid out as Word — plus is the native addition by
 # its op line, a Nat field of a list a word, a case on a Nat through from, a constructor through to;
 # main applies it to 21: sumL [21, 5, 22] + predn (dbl 21) = 48 + 41
 'rep': ('''dblr : Nat → Nat
dblr n = plus n n
prednr : Nat → Nat
prednr n = case n of zero → zero; suc k → k
sumLr : List Nat → Nat
sumLr l = case l of nil → 0; cons x xs → plus x (rec xs)
rep-defs = lift-program #100000 (program-rep-at (Nat → Nat) #99999 lnil #20 (code (λ (n : Nat) → plus (sumLr (cons n (cons 5 (cons (suc n) nil)))) (prednr (dblr n)))))
#same eval-fo (table-of rep-defs) lnil (fcall #99999 (lcons (flit #21) lnil)) = vw #89
defs = lcons (#0 , (lnil , fcall #99999 (lcons (flit #21) lnil))) rep-defs''', "89"),
 # a record with a represented field (B12 layouts): PtN's layout is a struct of two words (Nat as
 # Word), so the unboxed printer passes it by value; a case on it reads the fields as words
 'rep-rec': ('''data PtN = ptn (a : Nat) (b : Word)
dblr2 : Nat → Nat
dblr2 n = plus n n
ptn-sum : PtN → Nat
ptn-sum p = case p of ptn a b → plus a (suc (fromWord b))
  where fromWord : Word → Nat
        fromWord w = %welimP (λ v → Nat) w zero (λ k r → suc r)
rr-defs = lift-program #100000 (program-rep-at (Nat → Nat) #99999 lnil #20 (code (λ (n : Nat) → ptn-sum (ptn (dblr2 n) #3))))
#same eval-fo (table-of rr-defs) lnil (fcall #99999 (lcons (flit #20) lnil)) = vw #44
defs = lcons (#0 , (lnil , fcall #99999 (lcons (flit #20) lnil))) rr-defs''', "44", (None, "uint64_t f0; uint64_t f1; } c0;")),
 # primitive fields at their C widths (item 18): a flat record of UInt32, Float32, Float64 and Int64 is
 # struct { uint32_t; float; double; int64_t } by value, a list of UInt32 a typed pointer whose struct
 # holds a uint32_t; each field read into the program's word (a float by its bits) and written back from
 # it. At 5: (5 + 1 + (10 + 2 + 2)) + (100 + 5) = 125
 'prim-rec': ('''data PxR = pxr (a : UInt32) (f : Float32) (d : Float64) (i : Int64)
data L32 = l32n | l32c (x : UInt32) (r : L32)
pxr-bump : PxR → PxR
pxr-bump p = case p of pxr a f d i → pxr a f (f64add d d) i
pxr-sum : PxR → Word
pxr-sum p = case p of pxr a f d i → plusw (u32toWord a) (plusw (u32toWord (f32bits f)) (i64toWord (i64add i (i64off64 d))))
l32-sum : L32 → Word
l32-sum l32n = #0
l32-sum (l32c x r) = plusw (u32toWord x) (l32-sum r)
pr-defs = lift-program #100000 (program-rep-at (Word → Word) #99999 lnil #20 (code (λ (n : Word) → plusw (pxr-sum (pxr-bump (pxr (u32 n) (f32ofbits (u32 #1)) (f64 #2) (i64 #10)))) (l32-sum (l32c (u32 #100) (l32c (u32 n) l32n))))))
#same eval-fo (table-of pr-defs) lnil (fcall #99999 (lcons (flit #5) lnil)) = vw #125
defs = lcons (#0 , (lnil , fcall #99999 (lcons (flit #5) lnil))) pr-defs''', "125", (None, "uint32_t f0; float f1; double f2; int64_t f3; } c0;")),
 # a type represented by a record (lib/repr.cub, represent RBox by RPt): bump through the
 # representation, a record of two words; at 30: (30 + 9) + 2 = 41 (before the fix, 30 + 9 + 0)
 'rep-box': ("""rb-defs = lift-program #100000 (program-rep-at (Nat → Nat) #99999 lnil #20 (code (λ (n : Nat) → rbox-sum (rbox-bump (rbox n 2)))))
#same eval-fo (table-of rb-defs) lnil (fcall #99999 (lcons (flit #30) lnil)) = vw #41
defs = lcons (#0 , (lnil , fcall #99999 (lcons (flit #30) lnil))) rb-defs""", "41"),
 # a process run against its world (lib/proc.cub), specialized under an unknown store: the run
 # becomes a function of the store (st 3 + 1); compiled with a store that answers 40; prints 41
 'process': ("""proc-defs = case lift #100 spec4 of lifted t2 ds k → lcons (#0 , (lnil , fapply t2 (lcons (flam ftword (flit #40)) lnil))) ds
prog = case lift #200 (snd (snd (fromopt (lnth proc-defs #0) (#0 , (lnil , flit #0))))) of lifted m ds k → emit-prog (lcons (#0 , (lnil , m)) (lapp2 ds (ltail proc-defs)))
ltail l = case l of lnil → lnil; lcons x xs → xs""", "41"),
 # fib by word elimination over a pair, specialized under an unknown argument to a loop; fib 90
 'fib': ("""fibp : Word → Word × Word
fibp n = %welimP (λ v → Word × Word) n (#0 , #1) (λ k p → (snd p , %wadd (fst p) (snd p)))
fibw : Word → Word
fibw n = fst (fibp n)
defs = lcons (#0 , (lnil , fcall #1 (lcons (flit #90) lnil))) (lcons (fo-def #1 (specialize (Word → Word) lnil (code (λ w → fibw w)))) lnil)""", "2880067194370816120"),
 # the tagged message decoder (lib/decode.cub) lowered from its code with the representations
 # consumed: type 1 reads the price at 8 (500), type 2 the quantity at 16 (7); 1000*first + second
 'itch': ("""itch-main = fprim P-add (lcons (fprim P-mul (lcons (flit #1000) (lcons (fapply dm-fo (lcons mb1 lnil)) lnil))) (lcons (fapply dm-fo (lcons mb2 lnil)) lnil))
defs = case lift #100 dm-fo of lifted t2 ds k → lcons (#0 , (lnil , fprim P-add (lcons (fprim P-mul (lcons (flit #1000) (lcons (fapply t2 (lcons mb1 lnil)) lnil))) (lcons (fapply t2 (lcons mb2 lnil)) lnil)))) ds""", "500007", "ASSUME(((8 + 8) < (32 + 1)))"),
 # a flat record (two word fields) by value in the unboxed printer: a compound literal, a switch
 # on its tag binding the fields as uint64_t; the boxed printer allocates it. Sum of 3 and 4
 'record': ("""data Pt2 = pt2 (x : Word) (y : Word)
pt-d = didx (code Pt2)
defs = lcons (#0 , (lnil , fswitch (fcon pt-d #0 (lcons (flit #3) (lcons (flit #4) lnil))) (lcons (#0 , (#2 , fprim P-add (lcons (fvar #1) (lcons (fvar #0) lnil)))) lnil))) lnil""", "7"),
 # the fixed-width and floating-point forms (B14): 7 as Int64 to Float64, times 3.0 (a Float64
 # literal is its bits), back, as a word (21), plus ten times the signed comparison -1 < 1 (the word 2^64-1 read as Int64): 31
 'numeric': ("""defs = lcons (#0 , (lnil , fprim P-add (lcons (fprim P-wofi64 (lcons (fprim P-i64off64 (lcons (fprim P-f64mul (lcons (fprim P-f64ofi64 (lcons (fi64 #7) lnil)) (lcons (ff64 #4613937818241073152) lnil))) lnil)) lnil))
  (lcons (fprim P-mul (lcons (flit #10) (lcons (fprim P-i64lt (lcons (fi64 #18446744073709551615) (lcons (fi64 #1) lnil))) lnil))) lnil)))) lnil""", "31"),
 # the same from a typed code: lib/prim.cub's Int64 and Float64 primitives lowered to the numeric
 # primitives by lib/repr.cub, the argument an Int64 literal
 'num-spec': ("""spec-num = specialize-r (Int64 → Word) lnil (code (λ x → wadd (i64toWord (i64off64 (f64mul (f64ofi64 x) (f64 #3)))) (if i64lt x (i64 #1) then #10 else #0)))
defs = lcons (#0 , (lnil , fcall #1 (lcons (fi64 #7) lnil))) (lcons (fo-def #1 spec-num) lnil)""", "21", "n27("),
 # in-place update (C21): a loop over a fresh record (two word fields), its second field increased
 # by the counter each iteration; the record is the loop's own, so the rebuild (a recursion on it
 # returning its constructor with one field changed) is an update stored in place: no allocation in
 # the loop (boxed: a store into the state cell; unboxed: the flat struct by value, a field assigned).
 # 10 + 3 + 2 + 1 = 16
 'inplace': ("""data Rc2 = rc2 (x : Word) (y : Word)
rc-d = didx (code Rc2)
loop-rc = floop (lcons ftword (lcons (ftdata rc-d lnil #0) lnil)) (lcons (flit #3) (lcons (fcon rc-d #0 (lcons (flit #0) (lcons (flit #10) lnil))) lnil))
  (fif (fprim P-lt (lcons (flit #0) (lcons (fvar #1) lnil)))
     (fcon #0 #0 (lcons (fprim P-sub (lcons (fvar #1) (lcons (flit #1) lnil))) (lcons (frec rc-d (fvar #0) (lcons (#0 , (#2 , (lnil , fcon rc-d #0 (lcons (fvar #1) (lcons (fprim P-add (lcons (fvar #0) (lcons (fvar #3) lnil))) lnil))))) lnil)) lnil)))
     (fcon #0 #1 (lcons (fvar #0) lnil)))
defs = case lift #1 (inplace loop-rc) of lifted t2 ds k → lcons (#0 , (lnil , fswitch t2 (lcons (#0 , (#2 , fvar #0)) lnil))) ds""", "16", ("V *up = v0; up->f[1] = ", "up.u.c0.f1 = ")),
 # copy-on-write: the record is a parameter (the caller may hold it), so the update copies the
 # cell first (a parameter whose only use is a switch is boxed in both printers). 5 + (6 + 1) = 12
 'cow': ("""data Pt4 = pt4 (x : Word) (y : Word)
upd-y : Pt4 → Pt4
upd-y p = case p of pt4 x y → pt4 x (%wadd y #1)
cow-main = fswitch (fcall #1 (lcons (fcon (didx (code Pt4)) #0 (lcons (flit #5) (lcons (flit #6) lnil))) lnil)) (lcons (#0 , (#2 , fprim P-add (lcons (fvar #1) (lcons (fvar #0) lnil)))) lnil)
defs = lcons (#0 , (lnil , cow-main)) (lift-program #2 (lcons (fo-def #1 (specialize (Pt4 → Pt4) lnil (code (λ (p : Pt4) → upd-y p)))) lnil))""", "12", "cpc("),
 # a sort through the whole pipeline: insertion sort on a list of words (structural recursion,
 # a nested case, if on a word comparison) from the store, lowered with program-of, lifted, printed
 # with typed pointers (unboxed) or boxed; the sorted list's head and the sum of its elements
 'sort': ("""data WL = wnil | wcons (x : Word) (xs : WL)
wins : Word → WL → WL
wins v l = case l of wnil → wcons v wnil; wcons x xs → if %wlt v x then wcons v (wcons x xs) else wcons x (rec xs)
wsort : WL → WL
wsort l = case l of wnil → wnil; wcons x xs → wins x (rec xs)
wsum : WL → Word
wsum l = case l of wnil → #0; wcons x xs → %wadd x (rec xs)
whead : WL → Word
whead l = case l of wnil → #0; wcons x xs → x
sort-main : WL → Word
sort-main l = %wadd (%wadd (whead (wsort l)) #1000) (wsum (wsort l))
wl-d = didx (code WL)
wl x r = fcon wl-d #1 (lcons (flit x) (lcons r lnil))
wl4 = wl #5 (wl #3 (wl #9 (wl #1 (fcon wl-d #0 lnil))))
prog-defs = lift-program #100000 (program-at (WL → Word) #99999 #20 (code (λ (l : WL) → sort-main l)))
defs = lcons (#0 , (lnil , fcall #99999 (lcons wl4 lnil))) prog-defs""", "1019", ("f_", "T")),
 'let-if': ("""defs = lcons (#0 , (lnil , flet ftword (flit #3) (fif (fprim P-lt (lcons (fvar #0) (lcons (flit #5) lnil))) (fprim P-mul (lcons (fvar #0) (lcons (fvar #0) lnil))) (flit #0)))) lnil""", "9"),
}
UNBOXED = os.environ.get('UNBOXED') == '1'
# ---------------------------------------------------------------- the C++ image (lib/image.cub, plan 10.1)
# name: (definitions as .cub text, the names exported as {cub name: C++ name}, a C++ main using the
# header as "image.hpp", expected output, and optionally the names whose re-import must equal the
# source code (lib/reimport.cub; the default is every name; a definition with an erased parameter
# re-imports only up to erasure and is listed as []). The source is what the image translates
# (def-src: a copattern definition's record as written, any other definition's value). The header is compiled with g++ -std=c++20.
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
 # a parameterized data type: the type parameter a template parameter of the structs, a function
 # over it a template, a case on it a visit by template lambdas; the sum and the length of a list
 # the consumer builds through the image, each definition read back equal to its source
 'params': ("""data TLq (A : Type) = tnilq | tconsq (x : A) (xs : TLq A)
sumq : TLq Word → Word
sumq l = case l of tnilq → #0; tconsq x xs → %wadd x (rec xs)
lenq : (A : Type) → TLq A → Word
lenq A l = case l of tnilq → #0; tconsq x xs → %wadd #1 (rec xs)
mk3q : Word → TLq Word
mk3q a = tconsq a (tconsq (%wadd a #1) (tconsq (%wadd a #2) tnilq))""", {'sumq': 'sumq', 'lenq': 'lenq', 'mk3q': 'mk3q'},
  """#include "image.hpp"
#include <cstdio>
int main() { auto l = mk3q(5); std::printf("%llu %llu\\n", (unsigned long long) sumq(l), (unsigned long long) lenq<uint64_t>(l)); }""", "18 3"),
 # delayed terms in a checked definition (delay/force, cthunk in its code): the machine builds a
 # delay where it is met, so the normal form the image translates has none; exported, run, and
 # read back equal to the source's
 'delay': ("""thki : Word → Word → Word
thki a b = %wadd (force (delay (%wadd a b))) (delay (%wadd b #1))
thsel : Word → Word → Word
thsel a b = if %wlt a b then force (delay (%wsub b a)) else delay (%wsub a b)""", {'thki': 'thk', 'thsel': 'thsel'},
  """#include "image.hpp"
#include <cstdio>
int main() { std::printf("%llu %llu %llu\\n", (unsigned long long) thk(2, 3), (unsigned long long) thsel(3, 10), (unsigned long long) thsel(10, 4)); }""", "9 7 6"),
 # word elimination over a pair: cub::welim with a lambda step; fib 90
 'fib': ("""fibpi : Word → Word × Word
fibpi n = %welimP (λ v → Word × Word) n (#0 , #1) (λ k p → (snd p , %wadd (fst p) (snd p)))
fibi : Word → Word
fibi n = fst (fibpi n)""", {'fibi': 'fib'},
  """#include "image.hpp"
#include <cstdio>
int main() { std::printf("%llu\\n", (unsigned long long) fib(90)); }""", "2880067194370816120"),
}
# records of operations as concepts (18.9): a Σ with an operation among its components taken as a
# parameter is a template parameter constrained by a generated concept; a checked definition of such
# a record (an instance) is a struct satisfying it and an object of it. The consumer folds with the
# image's instance and with a struct of its own that satisfies the same concept
IMAGES['concept'] = ("""fold3c : ((Word → Word → Word) × Word) → Word → Word → Word → Word
fold3c m a b c = fst m a (fst m b (fst m c (snd m)))
addmc : (Word → Word → Word) × Word
addmc = (λ x y → %wadd x y , #0)""", {'fold3c': 'fold3', 'addmc': 'addm'},
  """#include "image.hpp"
#include <cstdio>
struct mulm { cub::word f0(cub::word a, cub::word b) const { return a * b; } cub::word f1_ = 1; };
static_assert(fold3_k0<mulm>);
static_assert(!fold3_k0<int>);
int main() { std::printf("%llu %llu\\n", (unsigned long long) fold3(addm, 1, 2, 3), (unsigned long long) fold3(mulm{}, 2, 3, 4)); }""", "6 24")
# codata as classes (18.9): a codata type is a class with a lazily computed cell per field and its
# observations as member functions; a copattern definition's record delays each field (computed at
# the first observation, then kept). The stream from n observed three tails in: n + 3
IMAGES['codata'] = ("""codata Swc where
  shc : Word
  snc : Swc
fromc : Word → Swc
(fromc n).shc = n
(fromc n).snc = fromc (%wadd n #1)
nth3c : Swc → Word
nth3c s = shc (snc (snc (snc s)))
tl1c : Swc → Swc
tl1c s = snc s""", {'fromc': 'fromw', 'nth3c': 'nth3', 'tl1c': 'tl1'},
  """#include "image.hpp"
#include <cstdio>
int main() { auto s = fromw(5); std::printf("%llu %llu\\n", (unsigned long long) nth3(s), (unsigned long long) nth3(tl1(s))); }""", "8 9")
# representations' classes (18.9, B12): a type with a declared representation is a class wrapping
# its representation (from, to, and of/data through its data form D<d>_v); its constructors and cases
# go through the representation's own conversions, exported with it. The consumer builds and reads
# values both through the image's definitions and through the class. bumpi's Int64 addition prints
# as + and the re-import reads + as word addition, so it is checked by running, not read back
IMAGES['repcls'] = ("""data CentsI = centsi (c : Int64)
centsiTo : CentsI → Int64
centsiTo x = case x of centsi c → c
centsiFrom : Int64 → CentsI
centsiFrom w = centsi w
centsi-tf : (w : Int64) → centsiTo (centsiFrom w) ≡ w
centsi-tf w = λ i → w
centsi-ft : (x : CentsI) → centsiFrom (centsiTo x) ≡ x
centsi-ft x = case x return (λ y → centsiFrom (centsiTo y) ≡ y) of centsi c → λ i → centsi c
represent CentsI by Int64 where
  to : CentsI → Int64 = centsiTo
  from : Int64 → CentsI = centsiFrom
  to-from : (w : Int64) → to (from w) ≡ w = centsi-tf
  from-to : (x : CentsI) → from (to x) ≡ x = centsi-ft
bumpi : CentsI → CentsI
bumpi x = case x of centsi c → centsi (i64add c c)
mki : Word → CentsI
mki w = centsi (i64 w)
geti : CentsI → Int64
geti x = case x of centsi c → c""", {'bumpi': 'bumpi', 'mki': 'mki', 'geti': 'geti'},
  """#include "image.hpp"
#include <cstdio>
using C = decltype(mki(0));
static_assert(sizeof(C) == sizeof(std::int64_t));
int main() { std::printf("%lld %lld %lld\\n", (long long) geti(bumpi(mki(21))), (long long) bumpi(mki(5)).to(), (long long) geti(C::from(-3))); }""", "42 10 -3", ['mki', 'geti'])
# the module file (18.9): the same image as a C++20 module interface unit — the includes in the global
# module fragment, the helpers and the image exported — compiled first (g++ -fmodules-ts); the
# consumer imports it, and each definition is read back from the unit's text
IMAGES['module'] = ("""data TLm = tnilm | tconsm (x : Word) (xs : TLm)
tsumm : TLm → Word
tsumm l = case l of tnilm → #0; tconsm x xs → %wadd x (rec xs)
mk3m : Word → TLm
mk3m w = tconsm w (tconsm #20 (tconsm #300 tnilm))
twicem : (A : Type) → (A → A) → A → A
twicem A f a = f (f a)""", {'tsumm': 'tsum', 'mk3m': 'mk3', 'twicem': 'twice'},
  """import cubimg;
#include <cstdio>
int main() { std::printf("%llu %llu\\n", (unsigned long long) tsum(mk3(1)), (unsigned long long) twice<cub::word>([](cub::word x) { return x * 10; }, 7)); }""", "321 700",
  None, {'img': 'cxx-module ' + enc('cubimg') + ' {tbl} lnil {gs}', 'module': 'cubimg'})
# the image specialized against a configuration (18.15): its platform decides cub::word, the
# alignment of every defining struct and the byte order of the byte layer. Here a 32-bit, big-endian
# target with 16-byte alignment: the word is 4 bytes, the structs are aligned to 16, and a load reads
# the most significant byte first (the byte set is the last of the eight, not the first as in 'bytes')
IMAGES['platform'] = ("""ld8p : (b : Bytes 32) → le 16 32 → Word
ld8p b h = i64toWord (load64 32 b 8 h)
data TLpl = tnilpl | tconspl (x : Word) (xs : TLpl)
tsumpl : TLpl → Word
tsumpl l = case l of tnilpl → #0; tconspl x xs → %wadd x (rec xs)
mk2pl : Word → TLpl
mk2pl w = tconspl w (tconspl #20 tnilpl)""", {'ld8p': 'ld8', 'tsumpl': 'tsum', 'mk2pl': 'mk2'},
  """#include "image.hpp"
#include <cstdio>
using L = decltype(mk2(0));
static_assert(sizeof(cub::word) == 4 && alignof(L) == 16 && cub::big);
int main() { cub::bytes<32> b{}; b[15] = 42; std::printf("%u %u\\n", (unsigned) ld8(b), (unsigned) tsum(mk2(1))); }""", "42 21",
  ['tsumpl', 'mk2pl'], {'img': 'cxx-image-cfg (cons (config nil nil #0 (platform #32 big-e #16)) nil) {tbl} lnil {gs}',
                        'check': lambda h: 'using word = std::uint32_t;' in h and 'align = 16;' in h and 'big = true;' in h})
# consumers' instantiations (18.15): a template of the image registers each instantiation statically
# (cub::note); at exit the run writes them as a configuration ($CUB_CONFIG_OUT), which is read back
# as the next specialization's: lenq at the word becomes an explicit specialization in the second
# image, compiled with the same consumer
IMAGES['config'] = ("""data TLr (A : Type) = tnilr | tconsr (x : A) (xs : TLr A)
lenr : (A : Type) → TLr A → Word
lenr A l = case l of tnilr → #0; tconsr x xs → %wadd #1 (rec xs)
mk3r : Word → TLr Word
mk3r a = tconsr a (tconsr (%wadd a #1) (tconsr (%wadd a #2) tnilr))""", {'lenr': 'lenr', 'mk3r': 'mk3r'},
  """#include "image.hpp"
#include <cstdio>
int main() { std::printf("%llu\\n", (unsigned long long) lenr<cub::word>(mk3r(5))); }""", "3",
  None, {'roundtrip': ("""#include "image.hpp"
#include <cstdio>
int main() { std::printf("%llu\\n", (unsigned long long) lenr<cub::word>(mk3r(7))); }""", "3"),
         'img2': 'cxx-image-cfg {cfg} {tbl} lnil {gs}',
         'check2': lambda h: 'template<> cub::word lenr<cub::word>(' in h})
def run_image(name, defs, names, main, expect, validate=None, opts=None):
    # opts: 'img' the header's expression ({tbl}, {gs} filled; cxx-image by default); 'module' a
    # module name: the image is that module interface unit, compiled first, main imports it;
    # 'check' a function of the header text that must hold (a layout the platform decides);
    # 'roundtrip' a second main whose run writes the instantiations ($CUB_CONFIG_OUT), read back
    # as the configuration of a second image ('img2', {cfg} filled) that must compile with main
    opts = opts or {}
    with tempfile.TemporaryDirectory() as d:
        cub = os.path.join(d, name + '.cub')
        tbl = 'lnil'
        for k, v in reversed(list(names.items())): tbl = f'(lcons (gidx (code {k}) , {enc(v)}) {tbl})'
        gs = 'lnil'
        for k in reversed(list(names.keys())): gs = f'(lcons (gidx (code {k})) {gs})'
        checks = ''.join(f"#same omap (λ p → snd p) (lnth (reimport-header {tbl} (fst hdr)) #{i}) = omap erase-code (def-src (gidx (code {k})))\n"
                         for i, k in enumerate(names) if validate is None or k in validate)
        img = opts.get('img', 'cxx-image {tbl} lnil {gs}').replace('{tbl}', tbl).replace('{gs}', gs).replace('{cfg}', opts.get('cfg', 'nil'))
        open(cub, 'w').write(FRESH() + defs + f"\nhdr = {img}\n#show fst hdr\n#show report-text {tbl} (snd hdr)\n" + checks)
        out = subprocess.run([CUB] + LIBS + [cub], capture_output=True, text=True, env=CUB_ENV,
                             preexec_fn=lambda: resource.setrlimit(resource.RLIMIT_STACK, (resource.RLIM_INFINITY, resource.RLIM_INFINITY))).stdout
        m = re.search(r'^fst hdr\s*=\s*(.*?)\s*\[\d+ steps', out, re.S | re.M)
        if not m: print(name, 'FAIL: no reading'); print(out[-500:]); return False
        bs = bytes(int(x) for x in re.findall(r'#(\d+)', m.group(1)))
        r = re.search(r'^report-text.*?=\s*(.*?)\s*\[\d+ steps', out, re.S | re.M)
        rep = bytes(int(x) for x in re.findall(r'#(\d+)', r.group(1))).decode() if r else ''
        cpp = os.path.join(d, 'main.cpp'); open(cpp, 'w').write(main)
        exe = os.path.join(d, name)
        if 'check' in opts and not opts['check'](bs.decode()):
            print(name, 'FAIL: the header does not hold the expected layout'); print(bs.decode()); return False
        if 'module' in opts:
            mi = os.path.join(d, opts['module'] + '.cppm'); open(mi, 'wb').write(bs)
            cc = subprocess.run(['g++', '-std=c++20', '-fmodules-ts', '-O2', '-w', '-x', 'c++', '-c', mi, '-o', os.path.join(d, 'mod.o')], capture_output=True, text=True, cwd=d)
            if not cc.returncode:
                cc = subprocess.run(['g++', '-std=c++20', '-fmodules-ts', '-O2', '-w', '-o', exe, cpp, os.path.join(d, 'mod.o')], capture_output=True, text=True, cwd=d)
        else:
            open(os.path.join(d, 'image.hpp'), 'wb').write(bs)
            cc = subprocess.run(['g++', '-std=c++20', '-O2', '-w', '-I', d, '-o', exe, cpp], capture_output=True, text=True)
        if cc.returncode: print(name, 'FAIL: g++'); print(cc.stderr[:1200]); print(bs.decode()); print(rep); return False
        cfgf = os.path.join(d, 'config.txt')
        got = subprocess.run([exe], capture_output=True, text=True, env=dict(os.environ, CUB_CONFIG_OUT=cfgf)).stdout.strip()
        if 'roundtrip' in opts and got == expect:
            # the instantiations this run recorded, read back as the next specialization's configuration
            cfg = open(cfgf).read().strip()
            o2 = dict(opts); del o2['roundtrip']; o2['img'] = opts['img2']; o2['cfg'] = '(cons (' + cfg + ') nil)'
            if 'check2' in opts: o2['check'] = opts['check2']
            print(name, 'ok; recorded:', cfg)
            return run_image(name + '-cfg', defs, names, opts['roundtrip'][0], opts['roundtrip'][1], validate, o2)
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
        open(cub, 'w').write(FRESH() + defs + "\n#show prog\n")
        # the printer of src/syntax.c (pr → pr_flat → pr_str → pr) spends a stack frame of a few KB per
        # nesting level, and a program is a byte list nested once per byte: a 4000-byte program
        # overflows the default 8 MB stack. Raised here until the printer iterates on the last argument.
        out = subprocess.run([CUB] + LIBS + [cub], capture_output=True, text=True, env=CUB_ENV,
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
        # a 're:' expectation is a pattern (global indices shift as the libraries grow)
        has = contains is None or (re.search(contains[3:].encode(), bs) is not None if contains.startswith('re:') else contains.encode() in bs)
        ok = got == expect and has
        print(name, 'ok' if ok else f'FAIL: got {got}, expected {expect}' + ('' if has else f'; {contains!r} not in the source'))
        if not ok: print(bs.decode())
        return ok
# ---------------------------------------------------------------- X86=1: the JIT path (plan 18.7)
# lib/emitx86.cub emits one position-independent blob per program (an entry stub and one function per
# definition); experiments/fo/x86main.c copies it into executable memory and calls it with the words
# on its command line. Each entry: the definitions (defs), the arguments, the expected word.
X86 = os.environ.get('X86') == '1'
X86_PROGS = {
 # plus on words: the two arguments added
 # word subtraction is the source's truncating one: 3 - x at x = 5 is 0, not 2^64 - 2 (every printer
 # emitted a wrapping subtraction until 2026-11-08)
 'tsub': ("""defs = lcons (#0 , (lcons ftword lnil , fprim P-add (lcons (fprim P-sub (lcons (flit #3) (lcons (fvar #0) lnil))) (lcons (flit #7) lnil)))) lnil""", [5], "7"),
 # max on words (P-max: cmp then cmovb), specialized from the source: max 9 (9 + 1) = 10
 'maxw': ("""defs = lcons (fo-def #0 (specialize (Word → Word) lnil (code (λ (a : Word) → %wmax a (%wadd a #1))))) lnil""", [9], "10"),
 'plusw': ("""defs = lcons (#0 , (lnil , fcall #1 (lcons (flit #40) (lcons (flit #2) lnil)))) (lcons (#1 , (lcons ftword (lcons ftword lnil) , fo-plus)) lnil)""", [], "42"),
 # the arguments come from the command line through the stub (rdi the arena, then the words)
 'plusw-args': ("""defs = lcons (#0 , (lcons ftword (lcons ftword lnil) , fo-plus)) lnil""", [1000, 234], "1234"),
 # factorial as a loop over (n, acc): continue while n > 0
 'fact': ("""defs = lcons (#0 , (lcons ftword lnil , fo-fact)) lnil""", [10], "3628800"),
 # plusw from the store: the specialized loop (plusw unfolds to a recursion on its first argument)
 'plusw-spec': ("""defs = lcons (fo-def #0 (specialize (Word → Word → Word) lnil (code (λ (a : Word) (b : Word) → plusw a b)))) lnil""", [30, 12], "42"),
 # fib 90 through a pair in the loop's state: constructor cells in the arena, projections as loads
 'fib': ("""fibp : Word → Word × Word
fibp n = %welimP (λ v → Word × Word) n (#0 , #1) (λ k p → (snd p , %wadd (fst p) (snd p)))
fibw : Word → Word
fibw n = fst (fibp n)
defs = lcons (fo-def #0 (specialize (Word → Word) lnil (code (λ w → fibw w)))) lnil""", [90], "2880067194370816120"),
 # a call inside a loop body: fib by the specialized definition, summed over a counter
 'call-in-loop': ("""fibp2 : Word → Word × Word
fibp2 n = %welimP (λ v → Word × Word) n (#0 , #1) (λ k p → (snd p , %wadd (fst p) (snd p)))
fibw2 : Word → Word
fibw2 n = fst (fibp2 n)
sum-fib = floop (lcons ftword (lcons ftword lnil)) (lcons (flit #0) (lcons (flit #0) lnil)) (fif (fprim P-lt (lcons (fvar #1) (lcons (flit #10) lnil))) (fcon #0 #0 (lcons (fprim P-add (lcons (fvar #1) (lcons (flit #1) lnil))) (lcons (fprim P-add (lcons (fvar #0) (lcons (fcall #1 (lcons (fvar #1) lnil)) lnil))) lnil))) (fcon #0 #1 (lcons (fvar #0) lnil)))
defs = lcons (#0 , (lnil , sum-fib)) (lcons (fo-def #1 (specialize (Word → Word) lnil (code (λ w → fibw2 w)))) lnil)""", [], "88"),
}
def run_x86(name, defs, args, expect):
    with tempfile.TemporaryDirectory() as d:
        cub = os.path.join(d, name + '.cub')
        open(cub, 'w').write(FRESH() + defs + "\n#show x86-ok defs\nprog = emit-x86 defs\n#show prog\n")
        out = subprocess.run([CUB] + LIBS + [cub], capture_output=True, text=True, env=CUB_ENV,
                             preexec_fn=lambda: resource.setrlimit(resource.RLIMIT_STACK, (resource.RLIM_INFINITY, resource.RLIM_INFINITY))).stdout
        okm = re.search(r'^x86-ok defs\s*=\s*(\w+)', out, re.M)
        m = re.search(r'^prog\s*=\s*(.*?)\s*\[\d+ steps', out, re.S | re.M)
        if not m or not okm: print(name, 'FAIL: no reading'); print(out[-500:]); return False
        if okm.group(1) != 'true': print(name, 'FAIL: outside the x86 fragment'); return False
        bs = bytes(int(x) for x in re.findall(r'#(\d+)', m.group(1)))
        blob = os.path.join(d, name + '.bin'); open(blob, 'wb').write(bs)
        exe = os.path.join(d, 'x86main')
        cc = subprocess.run(['gcc', '-O2', '-w', '-o', exe, os.path.join(ROOT, 'experiments', 'fo', 'x86main.c')], capture_output=True, text=True)
        if cc.returncode: print(name, 'FAIL: gcc'); print(cc.stderr[:800]); return False
        r = subprocess.run([exe, blob] + [str(a) for a in args], capture_output=True, text=True)
        got = r.stdout.strip()
        ok = got == expect and r.returncode == 0
        print(name, 'ok' if ok else f'FAIL: got {got!r} (exit {r.returncode}), expected {expect}; {len(bs)} bytes')
        return ok
def one(n):
    import io, contextlib
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf): ok = run(n, *PROGS[n])
    return ok, buf.getvalue()
if __name__ == '__main__':
    if X86: sys.exit(0 if all(run_x86(n, *X86_PROGS[n]) for n in X86_PROGS) else 1)
    if IMAGE: sys.exit(0 if all(run_image(n, *IMAGES[n]) for n in IMAGES if not sys.argv[1:] or n in sys.argv[1:]) else 1)
    names = [n for n in PROGS if not sys.argv[1:] or n in sys.argv[1:]]
    # RUNPY_JOBS programs at a time (default 2, this machine's cores); each one's output is printed
    # whole, in the table's order. The first program warms the library cache alone.
    jobs = int(os.environ.get('RUNPY_JOBS', '2'))
    if jobs <= 1 or len(names) <= 1:
        sys.exit(0 if all(run(n, *PROGS[n]) for n in names) else 1)
    import multiprocessing
    ok0, out0 = one(names[0]); sys.stdout.write(out0); sys.stdout.flush()
    with multiprocessing.Pool(jobs) as pool:
        res = pool.map(one, names[1:], chunksize=1)
    for ok, out in res: sys.stdout.write(out)
    sys.exit(0 if ok0 and all(ok for ok, _ in res) else 1)
