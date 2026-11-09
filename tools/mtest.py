#!/usr/bin/env python3
# make test, one recipe line at a time, resumable and robust to memory pressure.
#
# `make test` stops at the first failing line, and on a shared machine a line killed by the memory
# limit (exit 137, or a shell reporting "Killed") is not a failure of the code. This runner takes the
# recipe from `make -n test`, runs each line in its own shell with the recipe's environment
# (CUBICAL_NOCACHE=1), and:
#   - retries a line killed by the memory limit, up to RETRIES times, after a pause;
#   - records each passed line in build/mtest.state, keyed by the line's text, the sources of the
#     binary (src/), and the contents of every file the line loads (with the .cub files' imports,
#     followed); a line that names no file, or runs a script, is keyed by every file under lib/,
#     tests/, experiments/ and tools/. A rerun after an interruption, or after a commit that changed
#     only some files, reruns only the lines whose inputs changed;
#   - records each line's peak memory (the largest resident set of any process it ran) in its log
#     and in build/mtest.rss;
#   - stops at the first real failure, printing the line and the tail of its output.
# Usage: python3 tools/mtest.py [--fresh]   (--fresh forgets the recorded passes)
# Exit 0 when every line passed; the log of each line is in build/mtest/<n>.log.
import hashlib, os, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RETRIES, PAUSE = 6, 60

# `make -n` runs a recipe line that calls make itself (build/native-main's), and that line's
# output is mixed into the listing; a block is kept only when it starts like a command
import re
CMD = re.compile(r'^(\./|#|[A-Z_][A-Z0-9_]*=|(for|python3|grep|cmp|rm|env|tail|sh|bash|set|make|echo|mkdir|cc|ar|gcc|g\+\+|if|cd|test|diff|cat|true|\[)(\s|$))')
def recipe():
    out = subprocess.run(['make', '-n', 'test'], cwd=ROOT, capture_output=True, text=True, check=True).stdout
    lines, cur = [], ''
    for l in out.split('\n'):
        if cur: cur += '\n' + l
        else: cur = l
        if cur.endswith('\\'): continue
        if cur.strip() and CMD.match(cur): lines.append(cur)
        cur = ''
    # the build lines (cc ...) run once, first
    return lines

# the machine-wide memory gate (two slots; run.py lines run two programs at once, so they take both)
GATE = os.environ.get('HEAVY_GATE', '/home/claude/avikj/heavy.sh')
# Only the heavy lines go through it: a small line queued behind a long run would wait for nothing.
# Heavy: the proof files (foproof*), Brunerie, run.py, and any line recorded above 500 MB before.
HEAVY = re.compile(r'foproof|brunerie|run\.py|typed\.cub')
def recorded_heavy(cmd):
    p = os.path.join(ROOT, 'build', 'mtest.rss'); head = cmd.split(chr(10))[0][:200]
    if not os.path.exists(p): return False
    for l in open(p):
        mb, _, c = l.rstrip('\n').partition('\t')
        if c == head and mb.lstrip('-').isdigit() and int(mb) > 500: return True
    return False
def gate(cmd):
    if not os.path.exists(GATE) or not (HEAVY.search(cmd) or recorded_heavy(cmd)): return []
    return [GATE, '--big'] if 'run.py' in cmd else [GATE]

_fh = {}
def fhash(p):
    if p not in _fh:
        h = hashlib.sha256()
        try: h.update(open(p, 'rb').read())
        except OSError: h.update(b'missing')
        _fh[p] = h.hexdigest()
    return _fh[p]

def stamp():
    # the binaries are built from src/ (and the Makefile's flags)
    h = hashlib.sha256()
    for d, _, fs in sorted(os.walk(os.path.join(ROOT, 'src'))):
        for f in sorted(fs): h.update(f.encode()); h.update(fhash(os.path.join(d, f)).encode())
    h.update(fhash(os.path.join(ROOT, 'Makefile')).encode())
    return h.hexdigest()[:16]

IMPORT = re.compile(r'^import\s+"([^"]+)"', re.M)
def cub_closure(p, seen):
    if p in seen or not os.path.isfile(p): return
    seen.add(p)
    if p.endswith('.cub'):
        try: text = open(p, errors='replace').read()
        except OSError: return
        for m in IMPORT.finditer(text):
            q = m.group(1)
            cub_closure(q if os.path.isabs(q) else os.path.normpath(os.path.join(os.path.dirname(p), q)), seen)

ALL = None
def all_files():
    global ALL
    if ALL is None:
        ALL = []
        for top in ('lib', 'tests', 'experiments', 'tools'):
            for d, _, fs in os.walk(os.path.join(ROOT, top)):
                ALL += [os.path.join(d, f) for f in fs if not f.endswith(('.pyc', '.o'))]
        ALL.sort()
    return ALL

def line_key(cmd, s):
    h = hashlib.sha256((s + cmd).encode())
    files = set()
    for tok in re.findall(r'[A-Za-z0-9_./-]+', cmd):
        p = os.path.join(ROOT, tok)
        if '/' in tok and os.path.isfile(p): cub_closure(os.path.normpath(p), files)
    broad = not files or re.search(r'\b(python3|for|sh|bash)\b', cmd) or 'run.py' in cmd
    for p in (all_files() if broad else sorted(files)): h.update(p.encode()); h.update(fhash(p).encode())
    return h.hexdigest()[:24]

# runs one line and writes the largest resident set of anything it ran (RUSAGE_CHILDREN covers the
# whole tree, as bash waits for its children) to the file named by its second argument
WRAP = "import resource,subprocess,sys; r=subprocess.run(['bash','-c',sys.argv[1]]).returncode; open(sys.argv[2],'w').write(str(resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss)); sys.exit(r if r>=0 else 128-r)"

def main():
    os.makedirs(os.path.join(ROOT, 'build', 'mtest'), exist_ok=True)
    subprocess.run(['make', '-s', 'build/cubical', 'build/cubical-asan'], cwd=ROOT, check=True)
    st = os.path.join(ROOT, 'build', 'mtest.state')
    if '--fresh' in sys.argv and os.path.exists(st): os.remove(st)
    s = stamp()
    done = set()
    if os.path.exists(st):
        for l in open(st):
            done.add(l.split(' ', 1)[1].strip())
    env = dict(os.environ, CUBICAL_NOCACHE='1')
    lines = recipe()
    t0 = time.time()
    for i, cmd in enumerate(lines):
        key = line_key(cmd, s)
        if key in done: continue
        log = os.path.join(ROOT, 'build', 'mtest', f'{i}.log')
        for attempt in range(RETRIES + 1):
            t = time.time()
            with open(log, 'w') as f:
                rssf = log + '.rss'
                r = subprocess.run(gate(cmd) + [sys.executable, '-c', WRAP, cmd, rssf], cwd=ROOT, env=env, stdout=f, stderr=subprocess.STDOUT)
            text = open(log, errors='replace').read()
            killed = r.returncode in (137, -9) or 'Killed' in text[-2000:] and r.returncode != 0
            if r.returncode == 0: break
            if killed and attempt < RETRIES:
                print(f'[{i+1}/{len(lines)}] killed by the memory limit, retry {attempt+1} in {PAUSE}s', flush=True)
                time.sleep(PAUSE); continue
            print(f'[{i+1}/{len(lines)}] FAILED (exit {r.returncode}):\n{cmd[:600]}\n--- tail of {log}:\n{text[-3000:]}', flush=True)
            sys.exit(1)
        with open(st, 'a') as f: f.write(f'{s} {key}\n')
        try: mb = int(open(rssf).read()) // 1024
        except (OSError, ValueError): mb = -1
        with open(os.path.join(ROOT, 'build', 'mtest.rss'), 'a') as f: f.write(f'{mb}\t{cmd.split(chr(10))[0][:200]}\n')
        print(f'[{i+1}/{len(lines)}] ok {time.time()-t:.0f}s {mb} MB  {cmd.split(chr(10))[0][:100]}', flush=True)
    print(f'mtest: all {len(lines)} lines passed in {(time.time()-t0)/60:.0f} min', flush=True)

if __name__ == '__main__':
    main()
