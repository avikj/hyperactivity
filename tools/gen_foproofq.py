#!/usr/bin/env python3
# tests/foproofq.cub's per-program part: for every program of experiments/fo/run.py's proved-*
# programs, the checked path from its source to the first-order form handed to the back ends.
# For a data-route program the reading (dc-prog of its normal form) is taken from the machine and
# written out as checked terms: its main (a DC), the definitions' table (G) and the lifted λs' table
# (E: body and depth), the definitions' arities (A); DGOK and QGOK are proved case by case (if-all,
# each case decided by computation), and lib/foproofq.cub's q-path-n is instantiated. Readings tie the
# instance to the machine: the reading itself, and every entry of the table the program hands to the
# back ends equal to the printed entry of the instance's table.
# To rerun (after a program or the mirror changes): `python3 tools/gen_foproofq.py > tests/foproofq-progs.cub`
# from scoped-machine/ with build/cubical built; it runs the machine on the readings.
import os, re, subprocess, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LIBS = ['prelude', 'code', 'fo', 'match', 'nav', 'emitc', 'check', 'std', 'stage', 'lower', 'specialize', 'linproof',
        'prim', 'rules', 'proc', 'repr', 'foproof', 'foproofd', 'foproofq', 'region']
# the programs' own definitions (run.py defines them in the program text)
PRELUDE = '''p0 : Ptr 16
p0 = (0 , (8 , λ i → 16))
p8 : Ptr 16
p8 = (8 , (0 , λ i → 16))
two : Word → RM 16 Int64
two w = rthen (wr p0 (i64 w)) (rthen (wr p8 (i64 (%wsucc w))) (rbind (rd p0) (λ a → rmap (λ b → i64add a b) (rd p8))))
prog-two : Word → Word
prog-two w = i64toWord (eval-fresh 16 (two w))
fibx : Word → Word
fibx n = fst (%welimP (λ v → Word × Word) n (#0 , #1) (λ k p → (snd p , %wadd (fst p) (snd p))))
dc-prog-dx = some (#0 , (dx-main , lcons (#1 , (#1 , dx-body1)) (lcons (#2 , (#1 , dx-body2)) lnil)))
'''
# (name, the reading's argument, the table handed to the back ends, the arguments (first-order), expected)
# the reading: a normal form's (specialize-proved's data route) or a code read as written
SP = lambda T, c: ('dc-prog (normalize (%s) %s)' % (T, c), 'snd (specialize-proved (%s) lnil %s)' % (T, c), 'fst (specialize-proved (%s) lnil %s)' % (T, c))
PROGS = [
    ('qp-spec', SP('PQ → Word', '(code (λ (p : PQ) → pq-app p))'), ['fcon (didx (code PQ)) #1 (lcons (flit #9) (lcons (flit #2) lnil))'], '3'),
    ('qp-region', SP('Word → Word', '(code (λ (w : Word) → prog-two w))'), ['flit #20'], '41'),
    ('qp-cmp', SP('Word → Word → Word', 'dq-c1'), ['flit #3', 'flit #9'], '10'),
    ('qp-tags', SP('DqTg → Word', 'dq-c2b'), ['dq-tg1'], '11'),
    ('qp-list', SP('DqL → Word', 'dq-c3'), ['dq-lst'], '7'),
    ('qp-loop', SP('Word → DqTg → Word', 'dq-c4'), ['flit #3', 'dq-tg1'], '36'),
    ('qp-num', SP('Word → Word', 'dq-c5'), ['flit #5'], '12'),
    ('qp-float', SP('Word → Word', 'dq-c6'), ['flit #3'], '4620693217682128896'),
    ('qp-calls', ('dc-prog dq-c8', 'dc-prog-defs (fromopt (dc-prog dq-c8) dc-dflt)', None), ['flit #4'], '9'),
    ('qp-bytes', SP('Word → Word', 'sp-b2'), ['flit #12884901890'], '3'),
    ('qp-bytes32', SP('Word → Word', 'sp-b3'), ['flit #4294967301'], '5'),
    ('qp-data', ('dc-prog-dx', 'dc-defs dx-prog', None), [], '15'),
    ('qp-pair', SP('Word × Word → Word', 'dq-c10'), ['fcon #0 #0 (lcons (flit #3) (lcons (flit #4) lnil))'], '7'),
]

# ---------------------------------------------------------------- reading the machine's printing
def tokens(s):
    return re.findall(r'\(|\)|,|[^\s(),]+', s)
def parse(ts, i=0):
    # a term: an atom, or ( items ) with items an application or a tuple
    items, cur = [], []
    while i < len(ts):
        t = ts[i]
        if t == '(':
            sub, i = parse(ts, i + 1); cur.append(sub)
        elif t == ')':
            break
        elif t == ',':
            items.append(cur); cur = []
        else:
            cur.append(t)
        i += 1
    items.append(cur)
    if len(items) == 1:
        return ('app', items[0]), i
    return ('tup', [('app', x) for x in items]), i
def text(x):
    x = unwrap(x)
    if isinstance(x, str): return x
    if x[0] == 'tup': return ' , '.join(text(y) for y in x[1])
    return ' '.join(atomtext(y) for y in x[1])
def atomtext(y):
    y = unwrap(y)
    return y if isinstance(y, str) else '(' + text(y) + ')'
def unwrap(x):
    while not isinstance(x, str) and x[0] == 'app' and len(x[1]) == 1: x = x[1][0]
    return x
def head(x):
    x = unwrap(x)
    return x if isinstance(x, str) else (x[1][0] if x[0] == 'app' else None)
def args(x):
    x = unwrap(x)
    return [] if isinstance(x, str) else x[1][1:]
def nat(x):
    x = unwrap(x)
    if isinstance(x, str): return 0 if x == 'zero' else int(x)
    assert x[1][0] == 'suc'; return 1 + nat(x[1][1])
DCPOS = {'dcp': [1, 2], 'dcp1': [1], 'dcpr': [0, 1], 'dcfst': [0], 'dcsnd': [0], 'dccon': [2], 'dccase': [0, 1], 'dcif': [0, 1, 2],
         'dcapp': [0, 1], 'dccall': [2], 'dcby': [0], 'dcld': [1, 2], 'dcst': [1, 2, 3], 'dcblen': [0]}
def lams(x, d, out):
    # lib/foproofd.cub's dc-lifts-q: each λ i b at depth d, its body one deeper
    h, a = head(x), args(x)
    if h == 'dclet': lams(a[0], d, out); lams(a[1], d + 1, out)
    elif h == 'dcalt': lams(a[2], d + nat(a[1]), out); lams(a[3], d, out)
    elif h == 'dclam': out.append((text(unwrap(a[0])), d, text(unwrap(a[1])))); lams(a[1], d + 1, out)
    elif h in DCPOS:
        for p in DCPOS[h]: lams(a[p], d, out)
def chain(key, cases, dflt):
    # if weq key #k1 then v1 else (... dflt)
    s = dflt
    for k, v in reversed(cases): s = 'if weq %s %s then %s else (%s)' % (key, k, v, s)
    return s
def proof_chain(X, P, key, cases, dflt, pf):
    s = pf
    acc = [dflt]
    for k, v in reversed(cases):
        rest = acc[-1]
        s = 'if-all (%s) (%s) (weq %s %s) (%s) (%s) %s (%s)' % (X, P, key, k, v, rest, pf, s)
        acc.append('if weq %s %s then %s else (%s)' % (key, k, v, rest))
    return s

def run_machine(probe):
    path = '/tmp/gen_foproofq_probe.cub'
    open(path, 'w').write(probe)
    libs = [os.path.join(ROOT, 'lib', l + '.cub') for l in LIBS]
    env = dict(os.environ, CUBICAL_WORKERS='0')
    out = subprocess.run([os.path.join(ROOT, 'build', 'cubical')] + libs + [path], capture_output=True, text=True, env=env).stdout
    res = []
    lines = out.split('\n')
    for j, l in enumerate(lines):
        if l.startswith('dc-prog') and j + 1 < len(lines) and lines[j + 1].lstrip().startswith('= '):
            res.append(re.sub(r'\s*\[[^\]]*\]\s*$', '', lines[j + 1].strip()[2:]))
    if len(res) != len(PROGS): sys.exit('gen_foproofq: %d readings for %d programs:\n%s' % (len(res), len(PROGS), out[-3000:]))
    return res

def gen():
    probe = PRELUDE + ''.join('#show %s\n' % p[1][0] for p in PROGS)
    readings = run_machine(probe)
    R = '(fst std-frule)'
    o = []
    o.append('-- generated by tools/gen_foproofq.py (do not edit: rerun it)')
    o.append(PRELUDE.rstrip())
    o.append('-- a first-order value as the checked semantics has it (a word, a constructor with its fields)')
    o.append('fv-kq v = case v of vw w → kqw w; vcon d c fs → kqcon d c (fv-kql fs); _ → kqbot')
    o.append('fv-kql l = case l of lnil → kqnil; lcons x r → kqcons (fv-kq x) (fv-kql r)')
    o.append('fo-kq t = fv-kq (eval-fo no-defs lnil t)')
    for (name, (rd, tab, route), fargs, expect), r in zip(PROGS, readings):
        t, _ = parse(tokens(r))
        t = unwrap(t)
        assert head(t) == 'some', r
        tup = unwrap(args(t)[0])
        assert tup[0] == 'tup', r
        K, main, gs = [unwrap(x) for x in tup[1]]
        k = int(text(K).lstrip('#'))
        # the definitions: lcons (g , a , body) (... lnil)
        defs = []
        g = gs
        while head(g) == 'lcons':
            e = unwrap(args(g)[0]); assert e[0] == 'tup'
            gk, ga, gb = [unwrap(x) for x in e[1]]
            defs.append((text(gk), text(ga), text(gb)))
            g = args(g)[1]
        assert text(g) == 'lnil', r
        L = []
        lams(main, k, L)
        for gk, ga, gb in defs:
            gt, _ = parse(tokens(gb)); lams(gt, 1, L)
        mt = text(main)
        P = name
        o.append('-- %s: %s' % (name, rd))
        o.append('#same %s = some (#%d , %s , %s)' % (rd, k, mt, text(gs) if not isinstance(gs, str) else gs))
        o.append('%s-m : DC' % P)
        o.append('%s-m = %s' % (P, mt))
        o.append('%s-G : Word → DC' % P)
        o.append('%s-G g = %s' % (P, chain('g', [(gk, '(%s)' % gb) for gk, ga, gb in defs], 'dcl #0')))
        o.append('%s-A : Word → Word' % P)
        o.append('%s-A g = %s' % (P, chain('g', [(gk, ga) for gk, ga, gb in defs], '#0')))
        o.append('%s-E : Word → DC × Nat' % P)
        o.append('%s-E i = %s' % (P, chain('i', [(li, '((%s) , %d)' % (lb, ld)) for li, ld, lb in L], '(dcl #0 , 0)')))
        o.append('%s-T : Word → FC' % P)
        o.append('%s-T i = dconv (fst (%s-E i))' % (P, P))
        o.append('%s-D : Word → Nat' % P)
        o.append('%s-D i = snd (%s-E i)' % (P, P))
        o.append('%s-GT : Word → FC' % P)
        o.append('%s-GT g = dconv (%s-G g)' % (P, P))
        o.append('%s-gok : DGOK %s-G %s-T %s-GT' % (P, P, P, P))
        o.append('%s-gok g = ((λ j → dconv (%s-G g)) , dtok-auto %s-T (%s-G g) (%s))' % (P, P, P, P,
                 proof_chain('DC', 'λ c → dtok-chk %s-T c ≡ true' % P, 'g', [(gk, '(%s)' % gb) for gk, ga, gb in defs], 'dcl #0', '(λ j → true)')))
        o.append('%s-gq : QGOK (fr-tab %s %s-T) (fr-tab %s %s-GT) %s-D %s-A' % (P, R, P, R, P, P, P))
        o.append('%s-gq = ((λ g → %s) , (λ i → %s))' % (P,
                 proof_chain('DC', 'λ c → qtok %s-D %s-A (fr-bu %s (dconv c)) (suc zero) ≡ true' % (P, P, R), 'g', [(gk, '(%s)' % gb) for gk, ga, gb in defs], 'dcl #0', '(λ j → true)'),
                 proof_chain('DC × Nat', 'λ e → qtok %s-D %s-A (fr-bu %s (dconv (fst e))) (suc (snd e)) ≡ true' % (P, P, R), 'i', [(li, '((%s) , %d)' % (lb, ld)) for li, ld, lb in L], '(dcl #0 , 0)', '(λ j → true)')))
        # the arguments: the last innermost
        xs = ['x%d' % j for j in range(1, k + 1)]; vs = ['v%d' % j for j in range(1, k + 1)]
        qa = 'qa0'
        for j in range(k): qa = '(qa1 %s %s %s)' % (xs[j], vs[j], qa)
        ev = 'kqnil'
        for j in range(k): ev = '(kqcons %s %s)' % (vs[j], ev)
        FT = 'q-ft (fr-tab %s %s-T) (fr-tab %s %s-GT) %s-D %s-A' % (R, P, R, P, P, P)
        binds = ''.join(' (%s : DV) (%s : KQ)' % (xs[j], vs[j]) for j in range(k))
        o.append('-- the checked path: at every fuel, whatever the source computes at the arguments, the printed form computes')
        o.append('%s-path : (n : Nat)%s (m : Nat) → qa-ok %s-D %s-T %s' % (P, binds, P, P, qa))
        o.append('  → qrel %s-D (derase (dsev %s-G n %s-m (qa-ρ %s))) (evq (%s) (fr-addn n m) (fc-to-fq %d (fr-bu %s (dconv %s-m))) %s)' % (P, P, P, qa, FT, k, R, P, ev))
        o.append('%s-path n%s m ha = q-path-n std-frule %s-G %s-T %s-GT %s-gok %s-D %s-A %s-gq n %s-m %s m (dtok-auto %s-T %s-m (λ j → true)) ha (λ j → true)' % (
            P, ''.join(' %s %s' % (xs[j], vs[j]) for j in range(k)), P, P, P, P, P, P, P, P, qa, P, P))
        # what the back ends get is the instance's table, printed
        if route: o.append('#same %s = #2' % route)
        o.append('#same table-of (%s) #0 = fq-to-fo (fc-to-fq %d (fr-bu %s (dconv %s-m)))' % (tab, k, R, P))
        for gk, ga, gb in defs:
            o.append('#same table-of (%s) %s = fq-to-fo (%s %s)' % (tab, gk, FT, gk))
        for li, ld, lb in L:
            o.append('#same table-of (%s) (%%wadd dc-base %s) = fq-to-fo (%s (wadd dcbase %s))' % (tab, li, FT, li))
        # the checked meaning computes run.py's answer at its arguments
        kargs = 'kqnil'
        for a in fargs: kargs = '(kqcons (fo-kq (%s)) %s)' % (a, kargs)
        o.append('#same evq (%s) (dnat #64) (fc-to-fq %d (fr-bu %s (dconv %s-m))) %s = kqw #%s' % (FT, k, R, P, kargs, expect))
    return '\n'.join(o) + '\n'

if __name__ == '__main__':
    sys.stdout.write(gen())
