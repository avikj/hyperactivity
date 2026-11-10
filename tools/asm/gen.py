#!/usr/bin/env python3
"""The fixed machine-code routines of lib/x86.cub and lib/elf.cub, from their sources here.

  python3 tools/asm/gen.py            prints, for each routine, its length and its bytes as the
                                      .cub literal (a balanced tree of lapp over xb4/xb3/xb2/xb1)
  python3 tools/asm/gen.py --patch    rewrites the literals in place:
                                      i-driver-loop, i-driver-round and i-start in lib/elf.cub

driver.s  the driver loop of a #build binary (lib/elf.cub i-driver-loop): one case per command
          of WCmd (lib/world.cub), by constructor index
start.s   the ELF entry (lib/elf.cub i-start)
apply.s   the closure apply routine (lib/x86.cub i-apply; its literal there ends with the
          overflow exit appended by the encoder, so it is not patched automatically)
Needs binutils (as, objcopy). The routines are trusted with the encoder until the template
lemmas of PLAN Phase 2.
"""
import subprocess, sys, os, re, tempfile
here = os.path.dirname(os.path.abspath(__file__))
def assemble(name, defs=()):
    with tempfile.TemporaryDirectory() as d:
        o, b = os.path.join(d, 'x.o'), os.path.join(d, 'x.bin')
        subprocess.check_call(['as', '--64'] + ['--defsym=%s=1' % x for x in defs] + ['-o', o, os.path.join(here, name + '.s')])
        subprocess.check_call(['objcopy', '-O', 'binary', '-j', '.text', o, b])
        return open(b, 'rb').read()
def lit(b):
    ch = [b[i:i+4] for i in range(0, len(b), 4)]
    one = lambda c: ('xb%d ' % len(c)) + ' '.join('#%d' % x for x in c)
    def tree(l):
        if len(l) == 1: return one(l[0])
        m = len(l) // 2
        return 'lapp (%s) (%s)' % (tree(l[:m]), tree(l[m:]))
    return tree(ch)
if __name__ == '__main__':
    R = {n: assemble(n) for n in ('driver', 'start', 'apply')}
    R['round'] = assemble('driver', ('LOOP',))
    if '--patch' in sys.argv:
        p = os.path.join(here, '..', '..', 'lib', 'elf.cub'); s = open(p).read()
        for name, key in (('driver', 'i-driver-loop'), ('round', 'i-driver-round'), ('start', 'i-start')):
            s, n = re.subn(r'^%s = .*$' % re.escape(key), lambda m: '%s = %s' % (key, lit(R[name])), s, flags=re.M)
            assert n == 1, key
        open(p, 'w').write(s)
        print('patched lib/elf.cub:', {k: len(v) for k, v in R.items()})
    else:
        for k, v in R.items(): print(k, len(v)); print(lit(v))
