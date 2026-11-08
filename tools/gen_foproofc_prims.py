#!/usr/bin/env python3
# lib/foproofc.cub's primitive chain: cpq-k / cpc-k / cpr-k (k = 0 .. 29) and cr-prim. q-prim
# (lib/foproofe.cub) and c-prim (lib/foproofc.cub) are if-chains over the primitive's number; cpq-k is
# q-prim's chain from its k-th test on, cpc-k c-prim's, and cpr-k proves the k-th tail refined, so the
# proof follows the chain one test at a time (one cr-bif per entry, nothing to normalize under an open
# test).
# To rerun after adding a primitive to q-prim and c-prim: add its row to PRIMS below, in the chains'
# order, run `python3 tools/gen_foproofc_prims.py > /tmp/prims.cub`, and replace lib/foproofc.cub's block
# from the comment "-- q-prim's if-chain, entry by entry" through cr-prim's clause with the output.
# Each row: (primitive number, kind, q-side, c-side, refinement lemma), where kind is
#   'w2' : a binary operation on words of one kind: q-side the q-family (q-ww, q-ii, q-iw, q-ff, q-fw),
#          c-side the word function f (shared by q and c), lemma the matching cr- family;
#   '1'  : a unary operation: q-side the KQ → KQ function, c-side the CV → CV function, lemma the
#          one-argument lemma (cu-...), applied to A df;
#   'ls' : a load or store: q-side the KQ function, c-side the CV function (with its width), lemma the
#          cr- lemma over the whole list.
PRIMS = [
    (0,  'w2', 'q-ww', 'wadd', 'cr-ww'),
    (1,  'w2', 'q-ww', 'wsub', 'cr-ww'),
    (2,  'w2', 'q-ww', 'dmulw', 'cr-ww'),
    (3,  'w2', 'q-ww', '(λ a b → dbw (weq a b))', 'cr-ww'),
    (4,  'w2', 'q-ww', '(λ a b → dbw (wlt a b))', 'cr-ww'),
    (8,  'ls', 'q-ld8', 'c-ld #8', 'cr-ld8'),
    (9,  'ls', 'q-st8', 'c-st #8', 'cr-st8'),
    (10, 'ls', 'q-ld4', 'c-ld #4', 'cr-ld4'),
    (11, 'ls', 'q-st4', 'c-st #4', 'cr-st4'),
    (12, '1',  'qk-bytes', 'c-bytes', 'cu-bytes'),
    (13, '1',  'q-blen', 'c-blen', 'cu-blen'),
    (14, 'w2', 'q-ii', '(λ a b → dib (i64add (dbi a) (dbi b)))', 'cr-ii'),
    (15, 'w2', 'q-ii', '(λ a b → dib (i64sub (dbi a) (dbi b)))', 'cr-ii'),
    (16, 'w2', 'q-ii', '(λ a b → dib (i64mul (dbi a) (dbi b)))', 'cr-ii'),
    (17, 'w2', 'q-iw', '(λ a b → dbw (i64eq (dbi a) (dbi b)))', 'cr-iw'),
    (18, 'w2', 'q-iw', '(λ a b → dbw (i64lt (dbi a) (dbi b)))', 'cr-iw'),
    (19, 'w2', 'q-ff', '(λ a b → dfb (f64add (dbf a) (dbf b)))', 'cr-ff'),
    (20, 'w2', 'q-ff', '(λ a b → dfb (f64sub (dbf a) (dbf b)))', 'cr-ff'),
    (21, 'w2', 'q-ff', '(λ a b → dfb (f64mul (dbf a) (dbf b)))', 'cr-ff'),
    (22, 'w2', 'q-ff', '(λ a b → dfb (f64div (dbf a) (dbf b)))', 'cr-ff'),
    (23, 'w2', 'q-fw', '(λ a b → dbw (f64eq (dbf a) (dbf b)))', 'cr-fw'),
    (24, 'w2', 'q-fw', '(λ a b → dbw (f64lt (dbf a) (dbf b)))', 'cr-fw'),
    (25, '1',  'q-i-of-w', '(c-wm (λ w → w))', 'cu-iw'),
    (26, '1',  'q-w-of-i', '(c-wm (λ w → w))', 'cu-wi'),
    (27, '1',  'q-f-of-i', '(c-wm (λ b → dfb (f64ofi64 (dbi b))))', 'cu-fi'),
    (28, '1',  'q-i-of-f', '(c-wm (λ b → dib (i64off64 (dbf b))))', 'cu-if'),
    (29, '1',  'q-f-of-w', '(c-wm (λ w → dfb (f64 w)))', 'cu-fw'),
    (32, '1',  'q-bits', '(c-wm (λ w → w))', 'cu-bits'),
    (33, '1',  'q-ofbits', '(c-wm (λ w → w))', 'cu-ofbits'),
]
SIG_R = '(A : Word → Nat) (df : Word → Bool) (p : Word) (as : KQ) (cs : CV) → CR A df as cs → CR A df (cpq-{k} p as) (cpc-{k} p cs)'
def sides(kind, q, c, lem):
    if kind == 'w2':
        return f'({q} {c} as)', f'(c-ww {c} cs)', f'({lem} A df {c} as cs h)'
    if kind == '1':
        return f'(q-1 {q} as)', f'(c-1 {c} cs)', f'(cr-1 A df {q} {c} ({lem} A df) as cs h)'
    return f'({q} as)', f'({c} cs)', f'({lem} A df as cs h)'
def gen():
    out = []
    out.append("-- q-prim's if-chain, entry by entry (cpq-k p as is the chain from its k-th test on, as q-prim has it; cpc-k the")
    out.append("-- same on cells), so the proof follows it one test at a time")
    n = len(PRIMS)
    out.append(f'cpq-{n} : Word → KQ → KQ')
    out.append(f'cpq-{n} p as = kqbot')
    out.append(f'cpc-{n} : Word → CV → CV')
    out.append(f'cpc-{n} p cs = cbot')
    out.append(f'cpr-{n} : ' + SIG_R.format(k=n))
    out.append(f'cpr-{n} A df p as cs h = unit')
    for k in range(n - 1, -1, -1):
        p, kind, q, c, lem = PRIMS[k]
        qs, cs_, pr = sides(kind, q, c, lem)
        out.append(f'cpq-{k} : Word → KQ → KQ')
        out.append(f'cpq-{k} p as = if weq p #{p} then {qs[1:-1]} else cpq-{k+1} p as')
        out.append(f'cpc-{k} : Word → CV → CV')
        out.append(f'cpc-{k} p cs = if weq p #{p} then {cs_[1:-1]} else cpc-{k+1} p cs')
        out.append(f'cpr-{k} : ' + SIG_R.format(k=k))
        out.append(f'cpr-{k} A df p as cs h = cr-bif A df (weq p #{p}) {qs} {cs_} (cpq-{k+1} p as) (cpc-{k+1} p cs) {pr} (cpr-{k+1} A df p as cs h)')
    out.append('-- q-prim is the chain from its first test, as c-prim is')
    out.append('cr-prim : (A : Word → Nat) (df : Word → Bool) (p : Word) (as : KQ) (cs : CV) → CR A df as cs → CR A df (q-prim p as) (c-prim p cs)')
    out.append('cr-prim A df p as cs h = cpr-0 A df p as cs h')
    return '\n'.join(out) + '\n'
if __name__ == '__main__':
    sys_out = gen()
    import sys
    sys.stdout.write(sys_out)
