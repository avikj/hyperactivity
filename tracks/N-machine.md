# Track N — the machine: no crash on any input, and the cost of programs about programs

Read `README.md` here (rules), `../LANGUAGE.md` (the machine), `../PLAN.md` §2, §3 "Measured",
§4.4 item 8, and skim `src/net.c` (the rules), `src/lang.c` (build, readback, store).

Files you own: `src/net.c`, `src/big.c`, `src/drivers.c` is NOT yours; `experiments/machine/**`,
this file. `src/lang.c` and `src/syntax.c` are shared with track A: change them only for a
crash fix, in small commits, pulling first.

## What is built

1. **No input crashes the machine.** Known: normalizing or checking a unary `Nat` numeral of
   10⁶ (`n1m : Nat; n1m = 1000000` used in a definition's body, or a `List` literal that
   long) segfaults: C recursion depth in building, reading back, quoting or erasing deep
   right-nested data. Find every recursive C traversal over term/net depth (`build`, `era`,
   readback/printing, `to_code`, key hashing, cache load/store, `jit_in`/`jit_read`) and make
   each iterative or explicitly stack-allocated on the heap, so depth is bounded by memory.
   A process that exhausts memory must end with a message naming the definition or reading
   being evaluated, not a kill by signal where that can be arranged (`setrlimit`-aware
   allocation failure paths).
2. **`make asan` and `make tsan` clean** on `make library`'s files and on
   `experiments/jit/workloads.cub` and `experiments/native/*.cub`: run them, fix what they
   report (record each report under Found with its fix).
3. **Compile cost.** `#jit zig` (experiments/jit/workloads.cub) takes 1.3M steps and
   `#build feed-step` 1.2M; the specializer, the passes of `lib/x86.cub` and the encoder are
   ordinary terms, so this is the cost of the net running list-and-tree programs. Profile
   (`CUBICAL_PROF`, or add a per-global step counter printed at exit under an environment
   variable) to attribute steps to definitions; report the top 20 for both. Fix what is the
   machine's (sharing lost, re-evaluation of a let, memo keys not hit); report what is the
   library's (quadratic `lapp`, repeated `xlen`) to track A under "Needs from track A" with
   the measurement — do not edit `lib/x86.cub`.
4. **Open-dimension normalization** (PLAN §3 "Measured"): reproduce the 100–160 steps per
   result cell on Brunerie's g8 against ~24 for the generic 6-cube; attribute it; write the
   finding. This is measurement and diagnosis first; change the rules only with the
   measurement that justifies it, and `make library` + `make brunerie` (−2 at 0 and 4
   workers) + the workloads unchanged after.

## Acceptance

- A `Nat` numeral of 10⁷ and a 10⁶-element list literal check, print and compile to the point
  of an honest refusal or result; no signal.
- asan/tsan runs recorded clean.
- The two profiles recorded; each machine-side fix with steps before and after.

## Needs from track A

(none yet)

## Baseline (before any change of this track; machine shared, load average 4–7, so seconds are noisy)

At commit `main` of 2026-11-09 before the LIB additions: `make library` 842 passed, 0 failed,
21,740,361 steps, 19.4 s user. `experiments/jit/workloads.cub`: 33 `ok: jit`, 921 passed,
23,806,358 steps. `experiments/native/feed.cub`: `ok: build feed-step` 8210 bytes, 1,221,139
steps; `build/feed` prints 5 then 2. Brunerie at 0 workers: −2, the reading 233,628 steps,
3,851,045 steps in all, 5.3 s user; at 4 workers −2 (264,211 steps for the reading that run).
Track A reports the library is now 1398 passed, ~96M steps (lib/os, tso, atomics, obs, the C++
and Rust AST types added to LIB): **re-baseline after pulling before the first C change.**

## Done

Committed in `src/`: nothing yet. Written in the working copy `/home/claude/work-N`, built
and verified next (step 2 of "Next step" describes it exactly, so it can be rewritten from this
file if that copy is lost):

- `src/core.h` (end of file): prototypes `net_malloc`, `net_calloc`, `net_realloc`,
  `net_aligned_alloc`, `net_strdup`, `net_exhausted(what)`, `extern const char *net_doing[2]`,
  and macros mapping `malloc`/`calloc`/`realloc`/`aligned_alloc`/`strdup` onto them.
- `src/net.c` before `net_start`: the wrappers (end the run by `net_exhausted("memory")`),
  `net_exhausted` (writes `error: out of <what> in <form> <name> (file:line)` to stderr and,
  after flushing, to stdout when it is another file; `_exit(2)`), `net_fault` (SIGSEGV/SIGBUS
  on an alternate stack: a fault within 16 MB of the context's stack pointer is
  `net_exhausted("stack")`; any other prints `internal: fault … in <form>` and returns with
  the default action restored), `net_guard` (per thread: `sigaltstack` + `sigaction`),
  `net_start` creating workers with a 256 MB stack attribute and checking `pthread_create`
  (fewer workers and a note on stderr when one cannot be made). `jit_call`: the stack bottom
  compiled code may reach is clamped to 1 GB below the stack's top (as it was when the
  stack was 1 GB).
- `src/lang.c` `main`: `RLIMIT_STACK` raised to the hard limit (unlimited where allowed)
  instead of 1 GB before the existing re-exec; `toplevel`: sets `net_doing` to the form's
  head and name, restores it after.

Baseline after the LIB additions (pulled 21:05): `make library` 1398 passed, 0 failed,
95,683,095 steps.

## Found

1. **A numeral of 10⁶ segfaults in `ds` (src/lang.c:716), not in the net.** `experiments/machine/big-nat.cub`
   (`big : Nat; big = 1000000; #show big`): SIGSEGV with the 1 GB stack full
   (`$sp = 0x7fffbfffe9a0`, stack mapping `0x7fffbffff000-0x7ffffffff000`). `ds` has a
   2368-byte frame and recurses once per `suc`.
2. **The recursion is a class of 150 C functions in 95 mutually recursive groups** (from
   `gcc -fcallgraph-info=su`, strongly connected components of the call graph): 9 in
   `src/net.c` (`build_m`/`op1`, `synkey`, `defer_build`, `box_fd`/`fd_pending`/`fd_cell`/`fd_port`,
   `jit_in`, `jit_read`, `ih_maker`, `key_print_d`), the rest in `src/lang.c`, `src/syntax.c`,
   `src/store.c`. Frame sizes on the numeral's route: `toplevel_in` 68,640 bytes, `el_app`
   17,760, `syn_solve` 9,024, `by_lin` 7,904, `p_block_lets` 5,792, `ann` 2,384, `ds` 2,368,
   `pr` 2,336, `compile` 304, `to_code` 192, `build_m` 96, `synkey` 144.
3. **With the stack unlimited (`ulimit -s unlimited`) the numeral works but is not linear.**
   n = 10⁴: checks, `#show` reads 10000 in 1,130,045 steps (113 steps per `suc`). n = 2·10⁴:
   check 2,400,137 steps in ~8 s, `#show` 2,260,045 steps in ~71 s. n = 4·10⁴: check
   4,800,137 steps in ~28 s, `#show` not finished after 260 s. Steps are linear (120 per `suc`
   to check, 113 to show); the seconds are quadratic and they are C, not rewrites: every
   sample of the `#show` phase is inside `synkey` (src/net.c:4004) called from `build_m`.
   `build_m` calls `synkey(t, f)` at every node it builds, and `synkey` walks the whole
   subterm: building a term of depth d costs d² key computations. This is the machine's
   (item 3): the key of a built term must be computed once, bottom-up.
4. **4 workers under `ulimit -v 3000000` hang forever with no output.** `net_start`
   (src/net.c:3868) does not check `pthread_create`; each worker's stack is the 1 GB
   `RLIMIT_STACK` that `main` sets (4 × 1024 MB mappings, `VmSize` 4.28 GB), so two of four
   creations fail and `net_start` waits on `parked < nworkers` for threads that do not exist
   (gdb: main in `pthread_cond_wait` at net.c:3870, two workers parked). `make brunerie`'s
   4-worker half therefore cannot run under the 3 GB cap every run here is told to use.
5. Reported by track C (through A): checking a `data` declaration re-walks every type it
   mentions once per mention: in `lib/cxx/ast.cub`, `CExpr` 227k steps, `CDecl` (9
   constructors) 13.0M, `CTU` (one constructor) 11.7M; the two are 67% of the file's 37M
   steps. Not yet attributed (machine sharing/memo, or `lib/check.cub`'s data checking).
6. Reported by track D (through A): `#build` of a process written through a handler is
   superlinear: 7 commands 42M steps, 22 commands 1.21·10⁹ (tracks/D-os.md Found; D is
   making a per-stage table). Not yet attributed.

## Decisions (so a successor does not redo them)

- The 141 recursive functions of `lang.c`/`syntax.c`/`store.c` are not rewritten one by one:
  they run on the main thread only, and its stack is made to grow until memory ends
  (`RLIMIT_STACK` raised to unlimited before the re-exec `main` already does, where the hard
  limit allows; then growth is bounded by `RLIMIT_AS` and memory, not by 1 GB). Running out
  is reported by a SIGSEGV handler on an alternate stack that recognises a fault next to the
  stack pointer and prints `error: out of stack in <form> <name> (file:line)` and exits 2; any
  other fault prints where it was and is re-raised (a bug stays a signal).
- The 9 of `src/net.c` run on workers too, inside a rewrite: they are made iterative, and
  workers get a fixed modest stack (so 4 workers fit under a 3 GB cap).
- Allocation: `malloc`/`calloc`/`realloc`/`strdup`/`aligned_alloc` go through checked
  wrappers (macros in `src/core.h`, bodies in `src/net.c`) that end with
  `error: out of memory in <form> <name> (file:line)`, exit 2.
- On the numeral's route the fat frames (`ds`, `ann`, `el_app`, `pr`, `compile`, `to_code`)
  are made to loop along the last argument, so a right-nested value costs heap, not stack.

## Next step

1. `git pull`, `make build/cubical`, run the commands of `tracks/A-compiler.md` "How to
   verify" and `make brunerie`; record the remaining baseline numbers (workloads, feed) above.
2. In `src/net.c` after `net_start`: add `net_doing[2]`, the SIGSEGV handler
   (`sigaltstack`, `SA_ONSTACK|SA_SIGINFO`, compare `si_addr` with the context's `RSP`), the
   checked allocators, `pthread_attr_setstacksize(64 MB)` and a checked `pthread_create`
   (on failure: fewer workers, said on stderr). In `src/core.h` the macros. In `src/lang.c`
   `main`: `want = RLIM_INFINITY` when `rl.rlim_max` allows; in `toplevel`: set `net_doing`.
   Verify (library, workloads, feed, brunerie 0 and 4 under `ulimit -v 3000000`), commit.
3. `build_m`/`synkey` (src/net.c:3970–4200): return each built child's key to its parent
   instead of recomputing (a `let` passes none up: its body's keys were made under the bound
   value's key, its own under the bound term's). The ground is the argument in the comment
   (a child's free slots are bound before it is built and never rebound); the library's and
   Brunerie's step and memo-hit counts staying identical is the bring-up diagnostic. Record seconds
   for n = 2·10⁴, 4·10⁴, 10⁵ before and after.
4. Then the numeral at 10⁵, 10⁶, 10⁷ and a 10⁶ list literal, fixing each stop as found; then
   item 2 (asan, tsan), item 3 (profiles; findings 5 and 6), item 4 (g8).
