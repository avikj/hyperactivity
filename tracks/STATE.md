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

1. Load each file not in `make library` after it, record pass/fail here per file.
2. Per track, check each "Done" claim against its files; mark verified or wrong here.
3. Then the work per track E's next step.
