#!/usr/bin/env python3
# lib/foproofus.cub's word primitives on operands related only where they are words (WRL): for each
# family of q-prim's branches a word primitive takes, its value refined by the runtime's operation on
# the operands read back as cells (wr-..., the splits following lib/foproofc.cub's cr-... one constructor
# test at a time), and the chain wrp-k over lib/foproofc.cub's cpq-k / cpc-k: a word primitive's entry
# by its family's lemma (one cr-bif), a byte-layer entry (8 .. 13, not a word primitive: cu-wprim is
# false there) by the test's contradiction.
# To rerun after adding a primitive to q-prim (and lib/foproofc.cub's PRIMS): add its row below, in the
# chain's order, run `python3 tools/gen_foproofus_prims.py > /tmp/usprims.cub`, and replace
# lib/foproofus.cub's block from the comment "-- the word primitives' families" through wrp-0's clause
# with the output.
# Each row: (primitive number, family, q-side, c-side, the boxed link's unary lemma); families:
#   'w2' : a binary operation on words of one kind (q-side the q-family, c-side the word function);
#   '1'  : a unary operation (q-side the KQ → KQ function, c-side the CV → CV function);
#   'ls' : the byte layer, not a word primitive.
PRIMS = [
    (0,  'w2', 'q-ww', 'wadd', None),
    (1,  'w2', 'q-ww', 'wsub', None),
    (2,  'w2', 'q-ww', 'dmulw', None),
    (3,  'w2', 'q-ww', '(λ a b → dbw (weq a b))', None),
    (4,  'w2', 'q-ww', '(λ a b → dbw (wlt a b))', None),
    (8,  'ls', 'q-ld8', 'c-ld #8', None),
    (9,  'ls', 'q-st8', 'c-st #8', None),
    (10, 'ls', 'q-ld4', 'c-ld #4', None),
    (11, 'ls', 'q-st4', 'c-st #4', None),
    (12, 'ls', 'q-1 qk-bytes', 'c-1 c-bytes', None),
    (13, 'ls', 'q-1 q-blen', 'c-1 c-blen', None),
    (14, 'w2', 'q-ii', '(λ a b → dib (i64add (dbi a) (dbi b)))', None),
    (15, 'w2', 'q-ii', '(λ a b → dib (i64sub (dbi a) (dbi b)))', None),
    (16, 'w2', 'q-ii', '(λ a b → dib (i64mul (dbi a) (dbi b)))', None),
    (17, 'w2', 'q-iw', '(λ a b → dbw (i64eq (dbi a) (dbi b)))', None),
    (18, 'w2', 'q-iw', '(λ a b → dbw (i64lt (dbi a) (dbi b)))', None),
    (19, 'w2', 'q-ff', '(λ a b → dfb (f64add (dbf a) (dbf b)))', None),
    (20, 'w2', 'q-ff', '(λ a b → dfb (f64sub (dbf a) (dbf b)))', None),
    (21, 'w2', 'q-ff', '(λ a b → dfb (f64mul (dbf a) (dbf b)))', None),
    (22, 'w2', 'q-ff', '(λ a b → dfb (f64div (dbf a) (dbf b)))', None),
    (23, 'w2', 'q-fw', '(λ a b → dbw (f64eq (dbf a) (dbf b)))', None),
    (24, 'w2', 'q-fw', '(λ a b → dbw (f64lt (dbf a) (dbf b)))', None),
    (25, '1',  'q-i-of-w', '(c-wm (λ w → w))', 'cu-iw'),
    (26, '1',  'q-w-of-i', '(c-wm (λ w → w))', 'cu-wi'),
    (27, '1',  'q-f-of-i', '(c-wm (λ b → dfb (f64ofi64 (dbi b))))', 'cu-fi'),
    (28, '1',  'q-i-of-f', '(c-wm (λ b → dib (i64off64 (dbf b))))', 'cu-if'),
    (29, '1',  'q-f-of-w', '(c-wm (λ w → dfb (f64 w)))', 'cu-fw'),
    (32, '1',  'q-bits', '(c-wm (λ w → w))', 'cu-bits'),
    (33, '1',  'q-ofbits', '(c-wm (λ w → w))', 'cu-ofbits'),
]
H = '(A : Word → Nat) (df : Word → Bool) (T : Word → Word → Nat → Kd)'
W2 = {'q-ww': 'kqw', 'q-ii': 'kqi', 'q-iw': 'kqi', 'q-ff': 'kqf', 'q-fw': 'kqf'}
# each unary function's argument pattern
UN = {'q-i-of-w': 'kqw w', 'q-w-of-i': 'kqi b', 'q-f-of-i': 'kqi b', 'q-i-of-f': 'kqf b', 'q-f-of-w': 'kqw w', 'q-bits': 'kqf b', 'q-ofbits': 'kqi b'}
def w2(q):
    c = W2[q]; n = q[2:]; o = []
    o.append(f'wr-{n}d : {H} (f : Word → Word → Word) (p q : Word) (r : KQ) (cs : CV) → WRL A df T (kqcons ({c} p) (kqcons ({c} q) r)) cs → CR A df ({q} f (kqcons ({c} p) (kqcons ({c} q) r))) (c-ww f cs)')
    o.append(f'wr-{n}d A df T f p q kqnil (cvcons x (cvcons y z)) h = λ t → c-ww f (cvcons (fst h unit t) (cvcons (fst (snd h) unit t) (snd (snd h) t)))')
    o.append(f'wr-{n}d A df T f p q kqnil (cvcons x y) h = Empty-rec (snd h)')
    o.append(f'wr-{n}d A df T f p q kqnil cs h = Empty-rec h')
    o.append(f'wr-{n}d A df T f p q r cs h = unit')
    o.append(f'wr-{n}c : {H} (f : Word → Word → Word) (p : Word) (b r : KQ) (cs : CV) → WRL A df T (kqcons ({c} p) (kqcons b r)) cs → CR A df ({q} f (kqcons ({c} p) (kqcons b r))) (c-ww f cs)')
    o.append(f'wr-{n}c A df T f p ({c} q) r cs h = wr-{n}d A df T f p q r cs h')
    o.append(f'wr-{n}c A df T f p b r cs h = unit')
    o.append(f'wr-{n}b : {H} (f : Word → Word → Word) (p : Word) (r : KQ) (cs : CV) → WRL A df T (kqcons ({c} p) r) cs → CR A df ({q} f (kqcons ({c} p) r)) (c-ww f cs)')
    o.append(f'wr-{n}b A df T f p (kqcons b r) cs h = wr-{n}c A df T f p b r cs h')
    o.append(f'wr-{n}b A df T f p r cs h = unit')
    o.append(f'wr-{n}a : {H} (f : Word → Word → Word) (a r : KQ) (cs : CV) → WRL A df T (kqcons a r) cs → CR A df ({q} f (kqcons a r)) (c-ww f cs)')
    o.append(f'wr-{n}a A df T f ({c} p) r cs h = wr-{n}b A df T f p r cs h')
    o.append(f'wr-{n}a A df T f a r cs h = unit')
    o.append(f'wr-{n} : {H} (f : Word → Word → Word) (as : KQ) (cs : CV) → WRL A df T as cs → CR A df ({q} f as) (c-ww f cs)')
    o.append(f'wr-{n} A df T f (kqcons a r) cs h = wr-{n}a A df T f a r cs h')
    o.append(f'wr-{n} A df T f as cs h = unit')
    return o
def gen():
    o = ["-- the word primitives' families: each refined on operands related where they are words (WRL), the"
         "\n-- splits following the family's patterns; wrp-k the chain over cpq-k / cpc-k for a word primitive"]
    for q in ['q-ww', 'q-ii', 'q-iw', 'q-ff', 'q-fw']: o += w2(q)
    o.append(f'wr-1 : {H} (f : KQ → KQ) (g : CV → CV) → ((a : KQ) (x : CV) → WR A df T a x → CR A df (f a) (g x)) → (as : KQ) (cs : CV) → WRL A df T as cs → CR A df (q-1 f as) (c-1 g cs)')
    o.append('wr-1 A df T f g fg (kqcons a kqnil) (cvcons x z) h = subst {lzero} {lzero} {CV} {λ u → CR A df (f a) (c-1 g (cvcons x u))} {cvnil} {z} (sym (snd h)) (fg a x (fst h))')
    o.append('wr-1 A df T f g fg (kqcons a kqnil) cs h = Empty-rec h')
    o.append('wr-1 A df T f g fg as cs h = unit')
    for num, fam, q, c, lem in PRIMS:
        if fam != '1': continue
        nm = 'wu-' + lem[3:]
        o.append(f'{nm} : {H} (a : KQ) (x : CV) → WR A df T a x → CR A df ({q} a) ({c[1:-1]} x)')
        o.append(f'{nm} A df T ({UN[q]}) x h = {lem} A df ({UN[q]}) x (h unit)')
        o.append(f'{nm} A df T a x h = unit')
    n = len(PRIMS)
    sig = lambda k: f'wrp-{k} : {H} (p : Word) (as : KQ) (cs : CV) → cu-wprim p ≡ true → WRL A df T as cs → CR A df (cpq-{k} p as) (cpc-{k} p cs)'
    o.append(sig(n))
    o.append(f'wrp-{n} A df T p as cs hw h = unit')
    for k in reversed(range(n)):
        num, fam, q, c, lem = PRIMS[k]
        nxt = f'(wrp-{k + 1} A df T p as cs hw h)'
        o.append(sig(k))
        if fam == 'w2':
            o.append(f'wrp-{k} A df T p as cs hw h = cr-bif A df (weq p #{num}) ({q} {c} as) (c-ww {c} cs) (cpq-{k + 1} p as) (cpc-{k + 1} p cs) (wr-{q[2:]} A df T {c} as cs h) {nxt}')
        elif fam == '1':
            o.append(f'wrp-{k} A df T p as cs hw h = cr-bif A df (weq p #{num}) (q-1 {q} as) (c-1 {c} cs) (cpq-{k + 1} p as) (cpc-{k + 1} p cs) (wr-1 A df T {q} {c} (wu-{lem[3:]} A df T) as cs h) {nxt}')
        else:
            o.append(f'wrp-{k} A df T p as cs hw h =')
            o.append(f'  bool-split (weq p #{num}) (CR A df (cpq-{k} p as) (cpc-{k} p cs))')
            o.append(f'    (λ e → Empty-rec {{lzero}} {{CR A df (cpq-{k} p as) (cpc-{k} p cs)}} (false≢true (subst {{lzero}} {{lzero}} {{Word}} {{λ u → cu-wprim u ≡ true}} {{p}} {{#{num}}} (weq-sound p #{num} e) hw)))')
            o.append(f'    (λ e → subst {{lzero}} {{lzero}} {{Bool}} {{λ b → CR A df (if b then {q} as else cpq-{k + 1} p as) (if b then {c} cs else cpc-{k + 1} p cs)}} {{false}} {{weq p #{num}}} (sym e) {nxt})')
    return '\n'.join(o)
if __name__ == '__main__':
    print(gen())
