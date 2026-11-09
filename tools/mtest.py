#!/usr/bin/env python3
# make test, one recipe line at a time, resumable and robust to memory pressure.
#
# `make test` stops at the first failing line, and on a shared machine a line killed by the memory
# limit (exit 137, or a shell reporting "Killed") is not a failure of the code. This runner takes the
# recipe from `make -n test`, runs each line in its own shell with the recipe's environment
# (CUBICAL_NOCACHE=1), and:
#   - retries a line killed by the memory limit, up to RETRIES times, after a pause;
#   - records each passed line in build/mtest.state (keyed by the line's text and the build's
#     binaries' mtimes), so a rerun after an interruption resumes where it stopped;
#   - stops at the first real failure, printing the line and the tail of its output.
# Usage: python3 tools/mtest.py [--fresh]   (--fresh forgets the recorded passes)
# Exit 0 when every line passed; the log of each line is in build/mtest/<n>.log.
import hashlib, os, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RETRIES, PAUSE = 6, 60

def recipe():
    out = subprocess.run(['make', '-n', 'test'], cwd=ROOT, capture_output=True, text=True, check=True).stdout
    lines, cur = [], ''
    for l in out.split('\n'):
        if cur: cur += '\n' + l
        else: cur = l
        if cur.endswith('\\'): continue
        if cur.strip(): lines.append(cur)
        cur = ''
    # the build lines (cc ...) run once, first
    return lines

# the machine-wide memory gate (two slots; run.py lines run two programs at once, so they take both)
GATE = os.environ.get('HEAVY_GATE', '/home/claude/avikj/heavy.sh')
def gate(cmd):
    if not os.path.exists(GATE) or 'cubical' not in cmd and 'run.py' not in cmd: return []
    return [GATE, '--big'] if 'run.py' in cmd else [GATE]

def stamp():
    h = hashlib.sha256()
    for b in ('build/cubical', 'build/cubical-asan'):
        p = os.path.join(ROOT, b)
        if os.path.exists(p): h.update(f'{b}:{os.path.getmtime(p)}'.encode())
    return h.hexdigest()[:16]

def main():
    os.makedirs(os.path.join(ROOT, 'build', 'mtest'), exist_ok=True)
    subprocess.run(['make', '-s', 'build/cubical', 'build/cubical-asan'], cwd=ROOT, check=True)
    st = os.path.join(ROOT, 'build', 'mtest.state')
    if '--fresh' in sys.argv and os.path.exists(st): os.remove(st)
    s = stamp()
    done = set()
    if os.path.exists(st):
        for l in open(st):
            if l.startswith(s + ' '): done.add(l.split(' ', 1)[1].strip())
    env = dict(os.environ, CUBICAL_NOCACHE='1')
    lines = recipe()
    t0 = time.time()
    for i, cmd in enumerate(lines):
        key = hashlib.sha256(cmd.encode()).hexdigest()[:20]
        if key in done: continue
        log = os.path.join(ROOT, 'build', 'mtest', f'{i}.log')
        for attempt in range(RETRIES + 1):
            t = time.time()
            with open(log, 'w') as f:
                r = subprocess.run(gate(cmd) + ['bash', '-c', cmd], cwd=ROOT, env=env, stdout=f, stderr=subprocess.STDOUT)
            text = open(log, errors='replace').read()
            killed = r.returncode in (137, -9) or 'Killed' in text[-2000:] and r.returncode != 0
            if r.returncode == 0: break
            if killed and attempt < RETRIES:
                print(f'[{i+1}/{len(lines)}] killed by the memory limit, retry {attempt+1} in {PAUSE}s', flush=True)
                time.sleep(PAUSE); continue
            print(f'[{i+1}/{len(lines)}] FAILED (exit {r.returncode}):\n{cmd[:600]}\n--- tail of {log}:\n{text[-3000:]}', flush=True)
            sys.exit(1)
        with open(st, 'a') as f: f.write(f'{s} {key}\n')
        print(f'[{i+1}/{len(lines)}] ok {time.time()-t:.0f}s  {cmd.split(chr(10))[0][:110]}', flush=True)
    print(f'mtest: all {len(lines)} lines passed in {(time.time()-t0)/60:.0f} min', flush=True)

if __name__ == '__main__':
    main()
