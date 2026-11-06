#!/usr/bin/env python3
"""Name hygiene across lib/: a definition or constructor declared in two library files is reported.
A later module's name silently rebinds the references of every module parsed after it (the
checker's own constructors tword/tdata were shadowed by lib/fo.cub's FT once; specialize.cub's
normalize by linproof.cub's), so the library keeps every top-level name unique. Exit 1 on a clash.
Known shared names are listed in ALLOWED with the reason."""
import re, sys, os, glob, collections
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ALLOWED = {
  'yes': 'check.cub Res and std.cub Dec: check.cub is parsed first and never after std.cub',
  'no': 'check.cub std.cub: same',
  'Equiv': 'std.cub restates prelude.cub with levels', 'not': 'prelude.cub std.cub: same', 'refl': 'prelude.cub std.cub: same', 'ua': 'prelude.cub std.cub: same',
  'andb': 'identical definitions (check.cub, match.cub)', 'orb': 'check.cub match.cub: same', 'notb': 'check.cub match.cub: same', 'omap': 'check.cub match.cub: same',
  'obind': 'identical (check.cub, lin.cub)', 'lapp': 'identical (check.cub, emitc.cub)',
  'lst-len': 'identical (fo.cub, emitc.cub, lin.cub)', 'lst-rev': 'identical (fo.cub, lin.cub)',
}
# an allowed name is allowed in the files its reason names, and nowhere else (derive.cub's typed
# lapp over List shadowed emitc.cub's over Lst for every module after it and crashed the machine)
ALLOWED_IN = { n: set(re.findall(r'[a-z0-9-]+\.cub', why)) for n, why in ALLOWED.items() }
names = collections.defaultdict(set)
for f in sorted(glob.glob(os.path.join(ROOT, 'lib', '*.cub'))):
    base = os.path.basename(f)
    for line in open(f, encoding='utf-8'):
        if line.startswith('--') or not line.strip(): continue
        m = re.match(r'^data\s+(\S+)(.*)$', line)
        if m:
            names[m.group(1)].add(base)
            for c in re.findall(r'(?:=|\|)\s*([^\s|()]+)', m.group(2)): names[c].add(base)
            continue
        m = re.match(r'^\s*[|=]\s*([^\s|()]+)', line)   # a constructor on its own line
        if m and not line.startswith(' ' * 4) or (m and re.match(r'^\s*\|', line)):
            names[m.group(1)].add(base); continue
        if line.startswith('represent '): continue   # declares T·R·name lines
        m = re.match(r'^(?:primitive|postulate)\s+(\S+)', line)   # keyword declarations
        if m: names[m.group(1)].add(base); continue
        m = re.match(r'^([^\s#:(][^\s]*)\s', line)
        if m: names[m.group(1)].add(base)
bad = [(n, sorted(fs)) for n, fs in names.items() if len(fs) > 1 and (n not in ALLOWED or not fs <= ALLOWED_IN[n])]
for n, fs in sorted(bad): print(f'{n}: {" ".join(fs)}')
print(f'{len(names)} names, {len(bad)} clashes')
sys.exit(1 if bad else 0)
