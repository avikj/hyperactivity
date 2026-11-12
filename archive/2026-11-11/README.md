# Tracks: how the work is split, and how to pick any of it up cold

Read `../PLAN.md` first (what the system is, the one rule, the phases). This directory is the
live state of the work: one file per track, each with its specification, its acceptance, its
log, and its **next step**. A fresh agent (or a person) continues a track by reading its file
top to bottom and doing what "Next step" says. Nothing about the state of the work lives
anywhere else: not in a chat, not in an agent's memory.

**Whole-work state: `STATE.md` (sourced; reading file by file); index: `TREE.md`.**

## Rules for every track

1. **Own clone.** Work in your own clone so builds and the git index never collide:
   `git clone /home/claude/avikj/math /home/claude/work-<track> && cd /home/claude/work-<track> && make build/cubical`
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
7. Language notes for writing `.cub`: read `../LANGUAGE.md` §1–§3 and two library files close to
   what you write (`lib/prim.cub`, `lib/decode.cub`, `lib/world.cub`). Names are bound once
   (a second definition of a name is an error); `code`, `one`, `feed`, `step`, `count`,
   `total`, `yes`, `no` and every constructor name in the library are taken. Check a file with
   `CUBICAL_NOCACHE=1 ./build/cubical $(make -s -n library | sed -n 's/^.*cubical \(lib\/prelude.*\)$/\1/p') your.cub`
   and read the last lines; a line `N passed, 0 failed` ends a clean load. A segfault or a
   kill is a finding: record the input that caused it in your track file under "Found".

## What is evidence here, and the forms the earlier failure took (audit of 2026-11-09 21:10)

The project lost a week to work that looked like progress and contradicted its one rule. Every
track checks itself against these before each commit. If your spec (written by track A) asks
for one of them, the spec is wrong: say so under "Found" and do the right thing.

1. **A second description tied to the first by agreement of outputs.** Two definitions of one
   thing are related by a checked path or theorem, or one of them is deleted. Runs that agree
   (our reading vs. Clang's binary, rustc's vs. clang's output, the net vs. a kernel model vs.
   `strace`) are **bring-up diagnostics**: record them as such; they are never an acceptance
   line and never the ground for calling two things equal.
2. **A meaning that is not the meaning of what runs.** If properties are proved of a term and a
   different term is compiled, there must be a checked path between them, or the property is
   about nothing. This is why memory is a signature (below).
3. **Assumptions passed off as facts.** A model of something outside the system (the kernel,
   the hardware's memory ordering, Clang's Sema, an imported ISA model) is an *assumption*:
   it is named in PLAN §7 and every theorem that uses it has it as a hypothesis or parameter.
4. **Code written without its reason, with "proof" as a later activity.** There is no proof
   work here, as there are no tests. A definition is written at the type that says what it is
   for, so that it cannot be written wrong: an encoder template is a value of
   `Σ bytes . ⟦x86⟧ bytes ≡ meaning`, not bytes with a lemma owed; a pass over codes returns
   its path with its result; a handler into a representation is written over a type indexed
   by what it represents; a machine whose runs must satisfy a predicate has transitions typed
   over states that satisfy it. The index is ghost and costs nothing at run time (the compiler
   erases what no result depends on). If writing the definition at its real type is hard, the
   design is wrong and that is found on the first line, not after the code exists. "State the
   theorem, prove it later", "one lemma per …, afterwards", a type with no inhabitant recorded
   as a result: all forbidden. The only reasoning ever supplied separately from code is about
   foreign code that did not carry it (a property of imported C++), and that is done by
   computation (a checker decides it) wherever it can be.
5. **Deferral.** "Not modelled", "v1", "later" are allowed only under "Limits" with what is
   missing; a Limit on something the track's samples use keeps the track open.

### Memory is a signature (decision; binds tracks M, F, R and every later language)

A language reader produces a process over a **signature of memory commands** (`alloc`, `free`,
`load k p`, `store k p v`, pointer arithmetic and comparison, …) and never touches a memory
value. There are then two handlers of that one signature, and one theorem:

- `mem-std`: into the standard's abstract memory (`lib/mem/memory.cub`'s `AMem`: allocations,
  bounds, liveness, initialization; undefined behaviour as an outcome). The reader's term
  under this handler **is** the language's meaning; properties are stated of it.
- `mem-flat` is **not a second handler with a theorem owed**. It is written over the type of
  flat memories *indexed by the abstract memory they represent*: `Flat s` for `s : AMem`
  (bytes at addresses, with the placement of `s`'s live allocations). Each command is
  implemented at the type that says it is the abstract one:
  `flat-load : (k p) → Flat s → (mem-std-load k p s ≡ ok v) → Σ v′ . v′ ≡ v`, likewise store
  (returning `Flat s′` for the abstract successor `s′`), alloc, free. The index `s` and the
  definedness argument are ghosts the compiler erases, so the compiled command is one
  instruction; and a reader's term run through these is, by its type, the meaning's value on
  every defined run. Nothing is proved afterwards because nothing can be written otherwise.

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
| E | `E-lemmas.md` (open; handoff 2026-11-11) | the compiler at its type (paper §9.3) | B |
| F | `F-cxx.md` | `lib/cxx/machine.cub`, `read-expr`, `read-stmt`, `read`: C++ read into terms, run, compiled, properties | C, M |

## Deliverables and where they stand

| | Deliverable | Status |
|---|---|---|
| D1 | `#build`: a process as a standalone native binary | **done** (`experiments/native/feed.cub`, commit `856ab5a8`) |
| D2 | compiler checked against the x86 model | tracks B then E |
| D3 | C++ through our compiler | tracks C then F |
| D4 | properties of that C++ | after D3 |
| D5 | a small fleet as a term, `deploy` | track D, then A |
