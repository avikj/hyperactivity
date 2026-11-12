# Track E — the compiler at its type (PLAN Phase 2 item 5; paper §9.3)

Handoff written 2026-11-11 by the session that did this week's compile work. Every claim below
cites what it rests on; anything not verified is marked **unverified**. Read in this order:
`docs/Hyperactive_Final_Paper.pdf` (§9 fully, then §5–§7), `../PLAN.md` §0, §4.2, §6 Phase 2,
`README.md` here, `lib/x86.cub`'s header.

## What governs this work

- **The paper** (`docs/Hyperactive_Final_Paper.pdf`, dated 2026-11-11) states the construction
  exactly. The relevant parts:
  - `Block(a, P, Q) = Σ b:Bytes. Π s:S. P(s) → ExecTo(b, a, s, a+|b|, Q)` (9.12). `a` is the
    block's address, a parameter; `P` carries what the steps need about it. `ExecTo` is a finite
    trace of the machine's transition, an exit address, a final state meeting `Q`. A separate
    coinductive block type is for loops that continue and processes.
  - `seq : Block(a,P,Q) → Block(a+|b₁|,Q,R) → StablePrefix → RelocOK → Block(a,P,R)` (9.13), by
    concatenating bytes and traces. Branches need matching conditions; loops an invariant (or a
    coinductive simulation); calls the frame and world relation.
  - The compiler is the fold of a constructor algebra: `compile(cᵢ(t…)) = Cᵢ(compile t…)`,
    `correct = cong(Cᵢ, correct t…)` with one congruence per constructor (9.15–9.16).
  - A proof of a block is one row per instruction: "the target decoding equation and
    state-transition equation establish the corresponding row" (the 46-byte x86 decoder in §9.3,
    with its assertion table at every PC and its premises: disjoint regions, the return-address
    slot `[sp, sp+8)` disjoint from the stores, the ABI).
  - Artifact correctness composes: `ArtifactCorrect = SourceCorrect ∘ CompilerRefines` (9.17).
  - §9.4 describes this repo's `mn-run`/`Correct1` and states that `Correct1` does not assert
    termination or ordinary return, and that encoder-template proofs are separate obligations.
- **PLAN.md** Phase 2 item 5 states the same with `Tmpl m = Σ bytes . (for every state in the
  frame, ⟦x86⟧ bytes computes m into rax and keeps the frame)`, `cx : (c : Code) → Tmpl (eval-g c)`,
  passes returning their paths, "rewritten at these types when the model lands, not annotated
  afterwards"; acceptance: each workload's bytes run under the model from the loader's state give
  the machine's answer as a checked equation; `#trusted` lists the ISA model and no templates.
  `Tmpl` is the paper's `Block`; follow the paper's form.
- **PLAN §4.2**: the overflow exit is designed and sound — compiled code is pure, its arena is
  discarded, an overflow restores the stack and returns, the net computes the call (`run p f ≡ f`);
  in a standalone binary it is an abort outcome in `main`'s type.
- **`tracks/README.md`** item 4: a template is `Σ bytes . ⟦x86⟧ bytes ≡ meaning`, never bytes with
  a lemma owed afterwards.

## The proof method (from the paper, not from what the evaluator happens to compute)

Per instruction the encoder emits: its decoding equation and its transition equation under the
frame precondition `P`, as rows of an assertion table. Facts about the imported model these use,
each a lemma proved once from its definitions (read: `lib/isa/x86/application_level_memory.cub`
`rb_`/`wb`; `lib/isa/x86/step.cub` `x86_fetch_decode_execute`, near line 7295):
- the model reaches memory a byte at a time: `rb_`/`wb` loop over bytes, each with the canonical
  check of `signed(addr) + offset`, the address through `sail_mask_signed` 64→48, the offset at
  48 bits, sign-extension to 52, `memi`, then `x86-read-mem #1` (`logging.cub`);
- fetch reads the instruction bytes through `read_iptr`, `get_prefixes`, `add_to_iptr`.

Under `P` (each region's base a user address with the region's length as margin; regions
separated) these give: fetch returns the block's bytes; a read/write in a region is the plain byte
load/store; rip advances by the instruction length. Same-region address equality is the torsor law
(ring laws in `lib/compile/wordlaw.cub`).

Do **not** reason from what normalization or the specializer will or will not compute by itself.
This session wasted the day doing that (proposing core changes, a new bitvector representation,
routing proofs through the specializer). Proofs are the step-by-step reasoning.

## State of this week's work

**Wrong form — do not build on; replace (reasons cited):**
- `lib/compile/meaning.cub`: `⟦bs⟧`, `run-to` with fuel, `mn-run`, `Correct1/2(w)` at one fixed
  layout (`mn-code`, `mn-args`, `mn-ctx`, `mn-stack` literal addresses), judged by rax and the flag
  only. Paper §9.4 names the holes; it is not `Block`, covers one placement, has no frame.
- `lib/compile/runs.cub` (`QAll`, `q-run`, `q-step`, `q-stop`, `q-rax`, `RQ`) and
  `experiments/isa/addw-all.cub`: a whole-run proof of one output of the untyped encoder
  (`Correct2w addw-bs addw`), step counts and branch addresses found by running the model — the
  forbidden "bytes with a lemma owed" form. Its carry branch proves only the overflow fallback.
- `lib/compile/process.cub`: the ELF loading (`elf-segs`, `le-at`, segments at their addresses,
  rip at entry) is usable; the kernel `kn-step` is a second kernel beside `lib/os/contract.cub`'s
  `linux-model` (its own header says so). Replace `kn-step` by the world.
- `lib/compile/tmpl.cub`: frame-state lemmas (`bind-ok`, `wb-ok8`, `push-*`) with the code at a
  literal address; the frame-state idea is right, the literal code address is not.

**Usable as lemmas** (checked this week):
- `lib/compile/wordlaw.cub`: ring and order laws, byte digits, the 8-byte read-back (`rd8-*`,
  `rd8-push-reg`), bounded sums (`lt-add`), `mask64`.
- `lib/compile/bitlaw.cub`; `lib/compile/flags.cub` (the carry bit of `write_user_rflags` is the
  model's `fits`); `lib/compile/x86ok.cub` (run splitting, commuting stores).
- Load order (paths under `lib/`): `prelude code match nav cost lin check std linproof prim
  compile/wordlaw codes specialize rules machine x86 bits compile/bitlaw` then the files listed in
  `isa/x86/ORDER`, then `isa/x86/run compile/x86ok compile/meaning compile/runs compile/flags`.

**C changes this week** (`git log --since=2026-11-10 -- src/`): an evaluator series was reverted
(`c78304d9`). Remaining, to audit against the rule that nothing outside the language changes to
get a result unless the owner sanctioned it as a language defect:
- `d9c71c2e` — word operations with a variable operand give their result where the other operand
  alone determines it (`src/net.c`; diff read).
- `ace6fec0` — a recursive call given fewer arguments is the function of the rest; a primitive
  with no computation evaluates as the constant neutral of its type (`src/core.h`, `lib/prim.cub`,
  `LANGUAGE.md`; message read, diff not). Its message records **library 1,907 passed, 52 failed** —
  the library was not green at that commit; the current count is **unverified**.

**Paper constructions with no match in the repo by exact name** (grep of `lib/`, `experiments/`;
another name may exist — absence **unverified**): `record Language`, `Block`, `realize`,
`Adequate`, `StdDeriv`, `Der`, `reify`, `Maintained`, `xorCancel`/`cancelSound`. Present by name:
`o-handle`, `o-natural-path` (`lib/obs.cub`), `fleet-proc`, `safe-ind` (`lib/fleet.cub`),
`mem-std`, `AMem` (`lib/mem/memory.cub`; `AMem` also in `lib/cxx/machine.cub` — a C++ machine
exists in some form, **unread**), `linux-model` (`lib/os/contract.cub`).

## Library state (measured 2026-11-11, `make library`)

1,907 passed, 52 failed (same count as recorded at `ace6fec0`). Every failure is the mechanical
refusal of untyped code (in place since 2026-11-09) or follows from one: definitions without a
signature in `lib/derive.cub` (24), `lib/ring.cub` (9), `lib/intlin.cub` (6), `lib/eqproof.cub`
(3), `lib/autodiff.cub` (3); in `lib/cxx/ast.cub` (7) two primitives outside the bootstrap files
and four uses of the refused `cstr-fill`/`cstr-from`/`cstr-get`/`cstr-put`. These files need
their types; no failure is a wrong result.

## Next step

1. Read before writing anything: `lib/cxx/`, `lib/mem/`, `lib/obs.cub`, `lib/fleet.cub`,
   `lib/os/`, `lib/compile/process.cub` in full, the other `tracks/` files, the two C commits'
   diffs.
2. Then, in `lib/compile/`: `ExecTo` over `x86-step`, the frame `P` from `lib/x86.cub`'s
   conventions, `Block`, the model lemmas above, the instruction lemmas of the forms the encoder
   emits (start: load/store of a frame slot, `add`), `seq`; then branch, loop, call, allocation;
   then `cx` as the fold. Delete the wrong-form files when their replacements check.
