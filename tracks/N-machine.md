# Track N — the machine: no crash on any input, and the cost of programs about programs

Read `README.md` here (rules), `../README.md` (the machine), `../PLAN.md` §2, §3 "Measured",
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
## Done
## Found

## Next step

Reproduce the crash: a file with `big : Nat` / `big = 1000000` and `#show big`, under
`stdbuf -oL` and gdb if present (else ASan build) to get the recursing function; fix; commit.
