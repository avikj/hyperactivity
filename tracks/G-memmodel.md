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

- `lib/isa/tso.cub`, operational part: instructions `TsoI` (load, store, xchg, lock cmpxchg,
  mfence) with signature `TsoIS`; machine commands `TsoC` (`tso-do t i`, `tso-flush t`) with
  signature `TsoS`; state `TsoSt` (memory, last flushed write id per address, per-thread
  buffers newest first, next event id, trace of labels `TsoL`); `tso-world : World TsoS TsoSt`
  (a refusal is a transition that is not enabled); `tso-tag`, `tso-of-mem` (proc.cub's `MemS`
  onto the instructions). 22 definitions check, 184,874 steps, `789 passed, 0 failed`.

## Found

Nothing yet.

## Next step

In `lib/isa/tso.cub`, after `tso-of-mem`: the scheduler signature (`TsoCh = ch-go t | ch-flush t`,
`TsoSchedS`), threads as `Step TsoIS (List Word)`, `tso-step` (one choice applied to threads and
state), `tso-sys : Nat → threads → TsoSt → Step TsoSchedS TsoEnd` (the system as a tree of the
scheduler's choices), `tso-fold` over every response (all schedules), the outcome set as a
sorted duplicate-free `List (List Word)`. Then `experiments/tso/sb.cub` with `#show` of SB's set.
Then the axiomatic part (events `TsoEv`, witness `TsoX`, `tso-exec-of` a trace,
`tso-consistent`, `TsoSound`), MP/LB/IRIW, `lib/mem/atomics.cub`, the spin-lock.
