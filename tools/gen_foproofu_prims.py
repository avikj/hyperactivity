#!/usr/bin/env python3
# lib/foproofu.cub's primitive kinds: the kind of each primitive's result (kpk, its chain pkk-k) and
# the proof that q-prim's value has it (kv-prim, its chain kvp-k over lib/foproofc.cub's cpq-k), with
# the lemmas for each family of q-prim's branches (kq-...: every leaf is unit, the splits follow the
# family's patterns one constructor test at a time so each leaf normalizes).
# To rerun after adding a primitive to q-prim (and lib/foproofc.cub's PRIMS): add its row to PRIMS
# below, in the chain's order, run `python3 tools/gen_foproofu_prims.py > /tmp/uprims.cub`, and replace
# lib/foproofu.cub's block from the comment "-- the primitives' kinds" through kv-prim's clause with
# the output.
# Each row: (primitive number, kind of the result, family, q-side); families:
#   'w2' : a binary operation, q-side the q-family (q-ww, q-ii, q-iw, q-ff, q-fw) and the word function;
#   '1'  : a unary operation, q-side the KQ → KQ function (its lemma kg-<name>);
#   'ls' : a load or store, q-side the function on the whole list (its lemma kq-<name>).
PRIMS = [
    (0,  'kw', 'w2', ('q-ww', 'wadd')),
    (1,  'kw', 'w2', ('q-ww', 'wsub')),
    (2,  'kw', 'w2', ('q-ww', 'dmulw')),
    (3,  'kw', 'w2', ('q-ww', '(λ a b → dbw (weq a b))')),
    (4,  'kw', 'w2', ('q-ww', '(λ a b → dbw (wlt a b))')),
    (8,  'kw', 'ls', 'q-ld8'),
    (9,  'kb', 'ls', 'q-st8'),
    (10, 'kw', 'ls', 'q-ld4'),
    (11, 'kb', 'ls', 'q-st4'),
    (12, 'kb', '1',  'qk-bytes'),
    (13, 'kw', '1',  'q-blen'),
    (14, 'kw', 'w2', ('q-ii', '(λ a b → dib (i64add (dbi a) (dbi b)))')),
    (15, 'kw', 'w2', ('q-ii', '(λ a b → dib (i64sub (dbi a) (dbi b)))')),
    (16, 'kw', 'w2', ('q-ii', '(λ a b → dib (i64mul (dbi a) (dbi b)))')),
    (17, 'kw', 'w2', ('q-iw', '(λ a b → dbw (i64eq (dbi a) (dbi b)))')),
    (18, 'kw', 'w2', ('q-iw', '(λ a b → dbw (i64lt (dbi a) (dbi b)))')),
    (19, 'kw', 'w2', ('q-ff', '(λ a b → dfb (f64add (dbf a) (dbf b)))')),
    (20, 'kw', 'w2', ('q-ff', '(λ a b → dfb (f64sub (dbf a) (dbf b)))')),
    (21, 'kw', 'w2', ('q-ff', '(λ a b → dfb (f64mul (dbf a) (dbf b)))')),
    (22, 'kw', 'w2', ('q-ff', '(λ a b → dfb (f64div (dbf a) (dbf b)))')),
    (23, 'kw', 'w2', ('q-fw', '(λ a b → dbw (f64eq (dbf a) (dbf b)))')),
    (24, 'kw', 'w2', ('q-fw', '(λ a b → dbw (f64lt (dbf a) (dbf b)))')),
    (25, 'kw', '1',  'q-i-of-w'),
    (26, 'kw', '1',  'q-w-of-i'),
    (27, 'kw', '1',  'q-f-of-i'),
    (28, 'kw', '1',  'q-i-of-f'),
    (29, 'kw', '1',  'q-f-of-w'),
    (32, 'kw', '1',  'q-bits'),
    (33, 'kw', '1',  'q-ofbits'),
]
TT = '(T : Word → Word → Nat → Kd)'
# the input constructor of each binary family
W2 = {'q-ww': 'kqw', 'q-ii': 'kqi', 'q-iw': 'kqi', 'q-ff': 'kqf', 'q-fw': 'kqf'}
# the unary functions: (the constructor each takes, its argument's field name)
UN = {'q-i-of-w': 'kqw w', 'q-w-of-i': 'kqi b', 'q-f-of-i': 'kqi b', 'q-i-of-f': 'kqf b', 'q-f-of-w': 'kqw w',
      'q-bits': 'kqf b', 'q-ofbits': 'kqi b', 'qk-bytes': 'kqw n', 'q-blen': 'kqb l'}
UNK = {'qk-bytes': 'kb'}
# the loads and stores: the constructor of each list position, the result's kind
LS = {'q-ld8': (['kqb l', 'kqw o'], 'kw'), 'q-ld4': (['kqb l', 'kqw o'], 'kw'),
      'q-st8': (['kqb l', 'kqw o', 'kqi v'], 'kb'), 'q-st4': (['kqb l', 'kqw o', 'kqw v'], 'kb')}
def w2(q):
    c = W2[q]; o = []
    o.append(f'kq-{q}d : {TT} (f : Word → Word → Word) (p q : Word) (r : KQ) → Kv T kw ({q} f (kqcons ({c} p) (kqcons ({c} q) r)))')
    o.append(f'kq-{q}d T f p q kqnil = unit')
    o.append(f'kq-{q}d T f p q r = unit')
    o.append(f'kq-{q}c : {TT} (f : Word → Word → Word) (p : Word) (b r : KQ) → Kv T kw ({q} f (kqcons ({c} p) (kqcons b r)))')
    o.append(f'kq-{q}c T f p ({c} q) r = kq-{q}d T f p q r')
    o.append(f'kq-{q}c T f p b r = unit')
    o.append(f'kq-{q}b : {TT} (f : Word → Word → Word) (p : Word) (r : KQ) → Kv T kw ({q} f (kqcons ({c} p) r))')
    o.append(f'kq-{q}b T f p (kqcons b r) = kq-{q}c T f p b r')
    o.append(f'kq-{q}b T f p r = unit')
    o.append(f'kq-{q}a : {TT} (f : Word → Word → Word) (a r : KQ) → Kv T kw ({q} f (kqcons a r))')
    o.append(f'kq-{q}a T f ({c} p) r = kq-{q}b T f p r')
    o.append(f'kq-{q}a T f a r = unit')
    o.append(f'kq-{q} : {TT} (f : Word → Word → Word) (as : KQ) → Kv T kw ({q} f as)')
    o.append(f'kq-{q} T f (kqcons a r) = kq-{q}a T f a r')
    o.append(f'kq-{q} T f as = unit')
    return o
def un(g):
    k = UNK.get(g, 'kw')
    return [f'kg-{g} : {TT} (a : KQ) → Kv T {k} ({g} a)', f'kg-{g} T ({UN[g]}) = unit', f'kg-{g} T a = unit']
def ls(q):
    pos, k = LS[q]; o = []
    n = len(pos)
    # level i: the list so far is a known prefix of i positions, then a variable list r (or the
    # position's value b before its constructor is known)
    def pre(i, last):
        s = last
        for j in reversed(range(i)): s = f'(kqcons ({pos[j]}) {s})'
        return s
    names = [f'kq-{q}{chr(ord("a") + j)}' for j in range(2 * n + 1)]
    # names[0] splits the whole list; then alternately the head's constructor and the tail
    vars_of = lambda i: ' '.join(p.split()[1] for p in pos[:i])
    for lvl in reversed(range(2 * n + 1)):
        i = lvl // 2
        nm = names[lvl] if lvl else f'kq-{q}'
        if lvl % 2 == 0:
            # the list after i known positions is r
            v = vars_of(i)
            ty = ' '.join(f'({p.split()[1]} : {"BL" if p.split()[0] == "kqb" else "Word"})' for p in pos[:i])
            sig = f'{nm} : {TT} {ty} (r : KQ) → Kv T {k} ({q} {pre(i, "r")})'.replace('  ', ' ')
            o.append(sig)
            if i == n:
                o.append(f'{nm} T {v} kqnil = unit'.replace('  ', ' '))
                o.append(f'{nm} T {v} r = unit'.replace('  ', ' '))
            else:
                o.append(f'{nm} T {v} (kqcons b r) = {names[lvl + 1]} T {v} b r'.replace('  ', ' '))
                o.append(f'{nm} T {v} r = unit'.replace('  ', ' '))
        else:
            v = vars_of(i)
            ty = ' '.join(f'({p.split()[1]} : {"BL" if p.split()[0] == "kqb" else "Word"})' for p in pos[:i])
            sig = f'{nm} : {TT} {ty} (b r : KQ) → Kv T {k} ({q} {pre(i, "(kqcons b r)")})'.replace('  ', ' ')
            o.append(sig)
            c, x = pos[i].split()
            v2 = (v + ' ' + x).strip()
            o.append(f'{nm} T {v} ({c} {x}) r = {names[lvl + 1]} T {v2} r'.replace('  ', ' '))
            o.append(f'{nm} T {v} b r = unit'.replace('  ', ' '))
    return o
def gen():
    out = ["-- the primitives' kinds: each family of q-prim's branches has its result's kind, the splits following"
           "\n-- the family's patterns (every leaf unit); kpk p the kind of primitive p's result, kv-prim that q-prim's"
           "\n-- value has it, entry by entry along lib/foproofc.cub's cpq-k"]
    for q in ['q-ww', 'q-ii', 'q-iw', 'q-ff', 'q-fw']: out += w2(q)
    for g in UN: out += un(g)
    out.append(f'kq-1 : {TT} (k : Kd) (g : KQ → KQ) → ((a : KQ) → Kv T k (g a)) → (as : KQ) → Kv T k (q-1 g as)')
    out.append('kq-1 T k g hg (kqcons a kqnil) = hg a')
    out.append('kq-1 T k g hg as = kv-bot T k')
    for q in LS: out += ls(q)
    n = len(PRIMS)
    out.append(f'pkk-{n} : Word → Kd')
    out.append(f'pkk-{n} p = kw')
    out.append(f'kvp-{n} : {TT} (p : Word) (as : KQ) → Kv T (pkk-{n} p) (cpq-{n} p as)')
    out.append(f'kvp-{n} T p as = unit')
    for k in reversed(range(n)):
        num, kd, fam, q = PRIMS[k]
        if fam == 'w2':
            form, lem = f'({q[0]} {q[1]} as)', f'(kq-{q[0]} T {q[1]} as)'
        elif fam == '1':
            form, lem = f'(q-1 {q} as)', f'(kq-1 T {kd} {q} (kg-{q} T) as)'
        else:
            form, lem = f'({q} as)', f'(kq-{q} T as)'
        out.append(f'pkk-{k} : Word → Kd')
        out.append(f'pkk-{k} p = if weq p #{num} then {kd} else pkk-{k + 1} p')
        out.append(f'kvp-{k} : {TT} (p : Word) (as : KQ) → Kv T (pkk-{k} p) (cpq-{k} p as)')
        out.append(f'kvp-{k} T p as = kv-bif T (weq p #{num}) {kd} (pkk-{k + 1} p) {form} (cpq-{k + 1} p as) {lem} (kvp-{k + 1} T p as)')
    out.append('kpk : Word → Kd')
    out.append('kpk p = pkk-0 p')
    out.append(f'kv-prim : {TT} (p : Word) (as : KQ) → Kv T (kpk p) (q-prim p as)')
    out.append('kv-prim T p as = kvp-0 T p as')
    return '\n'.join(out)
if __name__ == '__main__':
    print(gen())
