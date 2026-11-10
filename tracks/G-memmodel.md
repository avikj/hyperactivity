# Track G — memory models: x86-TSO and the language-level orderings

Read `README.md` here (rules), `../PLAN.md` §1, §6 Phase 2 item 4 and Phase 4 item 1, and
`lib/proc.cub` (`MemS`, `sc-world`), `lib/world.cub`.

Files you own: `lib/isa/tso.cub`, `lib/mem/atomics.cub`, `experiments/tso/**`, this file.

## What is built

1. **`lib/isa/tso.cub`**: x86-TSO as terms. Both forms, and their relation:
   - operational: per-thread FIFO store buffers over shared memory, the transitions (write to
     buffer, read from own buffer else memory, flush oldest, locked instruction/`mfence`
     requiring an empty buffer), as a `World` (lib/proc.cub) over thread-tagged memory
     commands with the scheduler's choices as the responses of an environment signature;
   - axiomatic: events (thread, kind R/W/F/locked, address, value), program order, the
     execution witness (rf, total order on writes), and the TSO consistency predicate as a
     type over a candidate execution (Owens–Sarkar–Sewell);
   - the statement, proved as far as it goes by computation on finite executions and stated in
     general: every trace of the operational machine yields a consistent execution.
2. **`lib/mem/atomics.cub`**: the language-level interface: load/store/compare-exchange/fence
   with the orderings (relaxed, acquire, release, acq_rel, seq_cst) as commands of a
   signature, and the handler onto TSO commands (on x86 every load is a plain load, a seq_cst
   store is a store followed by `mfence` or a locked `xchg`).
3. **Litmus programs as terms** in `experiments/tso/`: SB (store buffering), MP (message
   passing), LB, IRIW, each as two/four processes; for each, the set of final outcomes of
   the operational machine over **all** schedules, computed by the checker (enumerate the
   environment's responses), is exactly the known x86-TSO set (SB allows `0,0`; MP, LB, IRIW
   forbid their relaxed outcomes). These are `#show`s whose values you record.

## Acceptance

- `make library` passes; your files load after it with `N passed, 0 failed`.
- The four litmus outcome sets, with steps, recorded under Done.
- A two-thread spin-lock (xchg-based) as a term with mutual exclusion stated as a type over
  every schedule up to a bound and checked.

## Needs from track A

Nothing yet.

## Done

How to check (cached library, one file after another; the last line must read `N passed, 0 failed`):
`LIBS=$(make -s -n library | sed -n 's/^.*cubical \(lib\/prelude.*\)$/\1/p'); (ulimit -v 2500000; timeout 600 ./build/cubical $LIBS lib/isa/tso.cub)`

- `lib/isa/tso.cub`, operational part (58 definitions check; the file alone is 264k steps):
  instructions `TsoI` (load, store, xchg, lock cmpxchg, mfence) with signature `TsoIS`; machine
  commands `TsoC` (`tso-do t i`, `tso-flush t`) with signature `TsoS`; state `TsoSt` (memory,
  last flushed write id per address, per-thread buffers newest first, next event id, trace of
  labels `TsoL`); `tso-world : World TsoS TsoSt` (a refusal is a transition that is not
  enabled); `tso-tag`, `tso-of-mem` (proc.cub's `MemS` onto the instructions).
  Systems: `TsoThread = Step TsoIS (List Word)` with the writers `ti-load/ti-store/ti-xchg/
  ti-cmpxchg/ti-mfence/ti-ret`; the scheduler's signature `TsoSchedS` (one command, response
  `TsoCh = ch-go t | ch-flush t`); `tso-step` (the one transition function); `tso-sys n ths s :
  Step TsoSchedS TsoEnd` (the system as a tree of the scheduler's responses, at most n
  transitions; ends `tso-finished out s`, `tso-blocked`, `tso-nofuel s`); `tso-fold` (every
  response at every command); `tso-outcomes n ths : TsoSum` = (sorted duplicate-free set of
  results, number of finished schedules, number cut by the bound).
- **SB** (`experiments/tso/sb.cub`), `#show tso-outcomes 6 sb`:
  set `{(0,0),(0,1),(1,0),(1,1)}`, 80 finished schedules, 0 cut by the bound, 108,147 steps.
  (80 = C(6,3)·2·2: each thread's load and flush in either order after its store.)

- **MP** (`experiments/tso/mp.cub`), `#show tso-outcomes 6 mp`: set `{(0,0),(0,1),(1,1)}`
  ((1,0) absent), 30 finished schedules, 0 cut, 47,793 steps.
- **LB** (`experiments/tso/lb.cub`), `#show tso-outcomes 6 lb`: set `{(0,0),(0,1),(1,0)}`
  ((1,1) absent), 20 finished schedules, 0 cut, 30,140 steps.
- **IRIW** (`experiments/tso/iriw.cub`), `#show tso-outcomes 8 iriw`: 15 of the 16 tuples
  (r0,r1,r2,r3), exactly (1,0,1,0) absent, 2,520 finished schedules (= 8!/(2!)^4), 0 cut,
  7,642,250 steps, 3.4 s for the five files after the cached library.
  The four files load together: `./build/cubical $LIBS lib/isa/tso.cub experiments/tso/{sb,mp,lb,iriw}.cub`
  ends `830 passed, 0 failed`.

## Found

- **Elaborator refuses a structural recursion when an argument before the matched one changes.**
  Input (after the library):
  `g1-a : Word → List Word → Word` / `g1-a j nil = j` / `g1-a j (cons x r) = g1-a (%wsucc j) r`
  gives `error: g1-a: a recursive call is not on a constructor field or a field's field from one
  of the definition's own matches (structural recursion)`. With `j` unchanged in the call, or
  with the list as the first parameter, it checks. Every such definition in my files puts the
  matched argument first (`tso-go`, `tso-quiet`, `tso-choices`). The error is fatal for the
  load (later files are not read), unlike a `FAIL:` line.
- A data constructor takes no implicit arguments: `scmd {S} {A} c k` is `FAIL … an implicit
  argument {..} applied to something without implicit arguments`; write `scmd c k`.
- `append`, `map`, `length` of `lib/prelude.cub` are not checked definitions (`error: code: not a
  bound variable or checked definition: append`, fatal); `app-w` (proc.cub) is.

## Next step

In `lib/isa/tso.cub`, after `tso-outcomes`: the axiomatic part. Events `TsoEv` (id, thread, kind
R/W/L(locked)/F(mfence), address, value read, value written), witness `TsoX` (events, rf as
(read id, write id or #0), co as the list of write ids in order), `tso-exec-of` a trace (the
reversed `ts-log`), `tso-consistent : TsoX → Bool` (well-formedness; acyclic(po∖(W×R) ∪ rfe ∪ co
∪ fr) over R/W/L/F nodes; acyclic(po-loc ∪ rf ∪ co ∪ fr)), `TsoSound : Type` (general statement,
not inhabited), and a fold `tso-all-consistent n ths : Bool` over every schedule. Then
`experiments/tso/{mp,lb,iriw}.cub`, `lib/mem/atomics.cub`, `experiments/tso/spinlock.cub`.
