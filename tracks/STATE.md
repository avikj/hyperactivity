# State of the whole work — sourced (started 2026-11-11)

Every line cites its source. "Track file says" means the claim is the track file's own, not yet
re-verified; "measured" means run on 2026-11-11 at the commit this file is in.

## Governing documents, in precedence

1. The owner's rules (as stated to agents).
2. `docs/Hyperactive_Final_Paper.pdf` (2026-11-11): the mathematical object. Where it is more
   exact than PLAN.md (e.g. §9.3 `Block`/`seq`/compiler algebra for PLAN's `Tmpl`), it governs.
3. `PLAN.md` (rev 9, 2026-11-09): phases, trusted base (§7), working rules.
4. `tracks/README.md` (rules, the earlier failure's forms) and one file per track (live state).

## What `make library` checks (measured)

Files, in order (`make -s -n library`): prelude code match nav cost lin check std sort linproof
eqproof ring autodiff intproof intlin prim derive decode fiber proc world session choreo obs region
codes specialize rules machine x86 elf os/platform os/linux os/contract os/replay isa/tso
isa/tso-acyclic mem/atomics mem/rc11 cxx/ast lang/rust/ast.

Result: **1,907 passed, 52 failed**. Every failure is the refusal of a definition without a
signature (or a primitive outside the bootstrap files), or a use of one: `derive.cub` 24,
`ring.cub` 9, `cxx/ast.cub` 7 (two primitives; four uses of `cstr-fill/from/get/put`),
`intlin.cub` 6, `eqproof.cub` 3, `autodiff.cub` 3. No failure is a wrong result.

**Not checked by `make library`** (measured: present in `lib/`, absent from its list):
`bits.cub`, `deploy.cub`, `fleet.cub`, `net.cub`; `lib/isa/x86/*` (the imported model);
`lib/mem/{flat,layout,memory,monad,scalar,sig,target}.cub`; `lib/cxx/{machine,read-expr}.cub`;
`lib/lang/rust/{machine,read,read-place,read-rvalue}.cub`; `lib/compile/*`. Their track files
record them loading "after the library"; whether each still loads at this commit is **unverified**.

## The files `make library` does not check, loaded after it (measured)

Counts include the library's own 52 (above); the difference is the file's.
- `net.cub fleet.cub`: 1,954/52 — 0 new failures. (`fleet.cub` alone does not load: it needs
  `net.cub` first.)
- `net.cub fleet.cub deploy.cub`: 1,992/52 — 0 new. (`deploy.cub` alone: `n-send` not a
  constructor, i.e. needs `net.cub`.)
- `mem/{target,scalar,sig,layout,flat,monad,memory}` in that order: 2,179/57 — 5 new, 3 in
  `layout.cub`: `w-roundup` is defined in `flat.cub:133`, which loads after it (an order error).
- the memory files then `cxx/machine.cub cxx/read-expr.cub`: 2,340/157 — `machine.cub` 21,
  `read-expr.cub` 70. Roots: `cx-unsupported` (machine.cub:56) and `cx-run` (:70) type
  mismatches; a primitive at machine.cub:79 refused (outside the bootstrap files); the rest
  follow (`cx-sk`, `cx-size` undefined because refused; 11 "an implicit argument {..} applied to
  something without implicit arguments").
- the memory files then `lang/rust/{machine,read-place,read-rvalue,read}.cub`: 2,319/132 —
  machine 6, read-place 21, read-rvalue 26, read 22. Root: `machine.cub:135–136` "deriving needs
  lib/derive.cub loaded" — `derive.cub`'s own definitions are refused (untyped), so deriving
  fails, and the rest follow (`rm-ik-eq`, `rm-opd-rv`, `rm-lay`, `rm-place-al` undefined).
- not yet run: `bits.cub`, the imported model `lib/isa/x86/*` and `lib/compile/*` (they loaded
  this week in the order in `E-lemmas.md`).

So the track files' "loads with 0 failed" for C++ (F), memory (M) and Rust (R) no longer hold at
this commit: the refusal of untyped code (2026-11-09) and an ordering error break them. Repairs
needed, roots first: types for `derive.cub` (then Rust's deriving works), `ring.cub`,
`intlin.cub`, `eqproof.cub`, `autodiff.cub`; the two primitives in `cxx/ast.cub` and the one at
`cxx/machine.cub:79`; `cx-unsupported`/`cx-run`'s mismatches; `layout.cub`'s order.

## Per track: the track file's own status (unverified unless marked)

| track | file, last change | its "Done" says |
|---|---|---|
| A compiler | `A-compiler.md`, 11-09 | `#jit` (33 workloads), closures, codata, measures, erasure, η, overflow exit; `#build` (feed demo 1.07 s vs 0.91 s C); `#build-loop` (feedloop, bounded memory) |
| B ISA | `B-isa.md`, 11-09 | Sail importer `tools/sail2cub.py`; model from commit `0c6c023d` (not vendored, at `/home/claude/ext/sail-x86-from-acl2`); 63 files, 528 functions; `lib/bits.cub` |
| C clang | `C-clang.md`, 11-09 | `tools/clang2cub.py`; `lib/cxx/ast.cub`; three experiments; acceptance met |
| D OS | `D-os.md`, 11-09 | raw command (`w-sys`, `w-alloc`, `w-load8`, `w-store8`) in `world.cub`/`src/drivers.c`; `lib/os/platform.cub` (23 calls) |
| E compiler at its type | `E-lemmas.md`, 11-11 | handoff written this session (sourced) |
| F C++ | `F-cxx.md`, 11-09 | `lib/cxx/machine.cub` (~560 lines) |
| G memory model | `G-memmodel.md`, 11-09 | `lib/isa/tso.cub` (TSO operational) |
| H observation | `H-obs.md`, 11-09 | `lib/obs.cub` items 1–2 (handlers, naturality, observations) |
| I fleet | `I-fleet.md`, 11-09 | `lib/net.cub`, `lib/fleet.cub` generic part |
| M memory | `M-mem.md`, 11-09 | `lib/mem/target.cub`, `scalar.cub` |
| N machine (C runtime) | `N-machine.md`, 11-10 | "Committed in src/: nothing yet"; work in `/home/claude/work-N` |
| R Rust | `R-rust.md`, 11-09 | `lib/lang/rust/*` reader, 1,704 lines |
| S surface | `S-surface.md`, 11-10 | item 1: recursion position permutation in `src/syntax.c` |

Verified against this session's reading: track E (fully); `lib/obs.cub` contains `o-handle`,
`o-natural-path` (grep); `lib/fleet.cub` contains `fleet-proc`, `safe-ind` (grep);
`lib/os/contract.cub` contains `linux-model` (grep); `lib/cxx/machine.cub` exists and uses
`AMem` (grep).

## Next steps (in order)

1. Repair the roots above, roots first, recounting after each.
2. Per track, check each "Done" claim against its files; mark verified or wrong here.
3. Then the work per track E's next step.
