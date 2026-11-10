# Tracks: how the work is split, and how to pick any of it up cold

Read `../PLAN.md` first (what the system is, the one rule, the phases). This directory is the
live state of the work: one file per track, each with its specification, its acceptance, its
log, and its **next step**. A fresh agent (or a person) continues a track by reading its file
top to bottom and doing what "Next step" says. Nothing about the state of the work lives
anywhere else: not in a chat, not in an agent's memory.

## Rules for every track

1. **Own clone.** Work in your own clone so builds and the git index never collide:
   `git clone /home/claude/avikj/math /home/claude/work-<track> && cd /home/claude/work-<track>/scoped-machine && make build/cubical`
   (if `/home/claude/avikj/math` is absent, clone `https://github.com/avikj/math`). Set the
   remote to GitHub: `git remote set-url origin $(git -C /home/claude/avikj/math remote get-url origin)`.
2. **Commit small, push every time.** After every piece that checks (a file loads, a function
   is done): `git add <your files> && git commit && git pull --rebase origin main && git push origin HEAD:main`.
   Never `git add -A`. Never leave more than ~20 minutes of work unpushed. End commit messages with
   the attribution lines the session gives you.
3. **Update your track file in the same commit**: move the item to Done with what was measured,
   and rewrite "Next step" so it is exactly what to do next (file, function, command).
4. **Touch only your files** (listed in the track). If you need a change in a shared file
   (`src/*.c`, `lib/x86.cub`, `lib/check.cub`, `lib/code.cub`, `lib/world.cub`, `Makefile`), write the
   exact change under "Needs from track A" in your track file, push, and continue with what
   does not depend on it. Track A applies it.
5. **The one rule** (PLAN §0): the checker can reject your output when it is wrong. No tests,
   no harnesses, no second model beside the thing. Generated `.cub` must check. Evidence is
   `make library` passing and the commands under your track's Acceptance.
6. **No network installs** (pip/npm/apt are off). `git clone` from GitHub works. Use what is
   installed: python3, gcc, clang, binutils.
7. Language notes for writing `.cub`: read `README.md` §1–§3 and two library files close to
   what you write (`lib/prim.cub`, `lib/decode.cub`, `lib/world.cub`). Names are bound once
   (a second definition of a name is an error); `code`, `one`, `feed`, `step`, `count`,
   `total`, `yes`, `no` and every constructor name in the library are taken. Check a file with
   `CUBICAL_NOCACHE=1 ./build/cubical $(make -s -n library | sed -n 's/^.*cubical \(lib\/prelude.*\)$/\1/p') your.cub`
   and read the last lines; a line `N passed, 0 failed` ends a clean load. A segfault or a
   kill is a finding: record the input that caused it in your track file under "Found".

## The tracks

| Track | File | Delivers | Depends on |
|---|---|---|---|
| A | `A-compiler.md` | the compiler: `#jit`, `#build`, registers, specialization on arrival; integration of every other track | — |
| B | `B-isa.md` | `lib/bits.cub`, `tools/sail2cub.py`, the x86 model in `lib/isa/x86/`, state and loader | — |
| C | `C-clang.md` | `tools/clang2cub`, `lib/cxx/ast.cub`, the sample C++ unit as checked data with layouts | — |
| D | `D-os.md` | `lib/os/linux.cub`: syscalls as a typed world with contracts over one raw syscall command | — |
| M | `M-mem.md` | `lib/mem/`: targets, scalars, memory, layouts, the evaluation monad every language reader targets | — |
| G | `G-memmodel.md` | x86-TSO (operational and axiomatic), language atomics, litmus outcome sets | — |
| H | `H-obs.md` | `lib/obs.cub`: observation, bisimulation, the quotient, cross-language equality | — |
| I | `I-fleet.md` | network world, fleets, property types, the kv fleet, `deploy` | D for the last step |
| N | `N-machine.md` | no crash on any input; asan/tsan; compile-cost profiles | — |
| R | `R-rust.md` | `tools/mir2cub.py`, `lib/lang/rust/ast.cub`, the Rust twin of the C++ sample | C's sample |
| S | `S-surface.md` | the elaborator: recursion forms, branch typing, clause typing, non-fatal failures | — |
| E | `E-lemmas.md` (opens when B lands) | the template lemmas, the encoder's path | B |
| F | `F-cxx.md` (opens when C lands and A fixes the object-model interface) | `lib/cxx/machine.cub`, `lib/cxx/read.cub` | C |

## Deliverables and where they stand

| | Deliverable | Status |
|---|---|---|
| D1 | `#build`: a process as a standalone native binary | **done** (`experiments/native/feed.cub`, commit `856ab5a8`) |
| D2 | compiler checked against the x86 model | tracks B then E |
| D3 | C++ through our compiler | tracks C then F |
| D4 | properties of that C++ | after D3 |
| D5 | a small fleet as a term, `deploy` | track D, then A |
