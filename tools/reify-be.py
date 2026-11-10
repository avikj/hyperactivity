# tools/reify-be.py — a word expression printed by #show as lib/word.cub's BE term.
# Reads the normal form (an expression in %wand %wor %wxor %wshl %wshr %wadd, numerals #n and the
# λ's variables x0, x1, ...) on stdin and prints the BE term, each numeral with its bound named
# fit<n> (a definition WFits #n <width>, proved by refl). The term is untrusted: the statement it
# is used in is checked (lib/word.cub be-sound and conversion).
# usage: python3 tools/reify-be.py [width] < normal-form
import sys, re
# reads a normal form printed by #show (a word expression in %wand/%wor/%wxor/%wshl/%wshr/%wadd,
# numerals #n and variables x0, x1, ...) and prints the BE term for it
def toks(s):
    return re.findall(r'\(|\)|%w[a-z]+|#[0-9]+|x[0-9]+', s)
def parse(ts, i):
    t = ts[i]
    if t == '(':
        op = ts[i+1]; a, i = parse(ts, i+2)
        b, i = parse(ts, i)
        assert ts[i] == ')', ts[i:i+3]
        return (op, a, b), i+1
    if t.startswith('#'): return ('lit', int(t[1:])), i+1
    if t.startswith('x'): return ('var', int(t[1:])), i+1
    raise Exception(t)
names = {'%wand':'be-and','%wor':'be-or','%wxor':'be-xor','%wadd':'be-add'}
def emit(e, out, bound):
    k = e[0]
    if k == 'lit':
        out.append('(be-lit #%d %d fit%d)' % (e[1], bound, e[1]))
    elif k == 'var':
        out.append('(be-var %d)' % e[1])
    elif k in ('%wshl', '%wshr'):
        assert e[2][0] == 'lit', e
        out.append('(%s ' % ('be-shl' if k == '%wshl' else 'be-shr')); emit(e[1], out, bound); out.append(' %d)' % e[2][1])
    else:
        out.append('(%s ' % names[k]); emit(e[1], out, bound); out.append(' '); emit(e[2], out, bound); out.append(')')
src = sys.stdin.read()
body = '(' + (src[src.index('→')+1:] if '→' in src else src) + ')'
ts = toks(body)
e, i = parse(ts, 0)
out = []; emit(e, out, int(sys.argv[1]) if len(sys.argv) > 1 else 130)
print(''.join(out))
lits = sorted(set(re.findall(r'be-lit #([0-9]+) ', ''.join(out))))
if '--fits' in sys.argv:
    for v in lits: print('fit%s : WFits #%s %d\nfit%s = refl' % (v, v, bound, v), file=sys.stderr)
