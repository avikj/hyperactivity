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

- `lib/isa/tso.cub`, axiomatic part and the relation (file now 112 definitions; all check):
  events `TsoEv` (id, thread, kind `k-R/k-W/k-L/k-F`, address, value read, value written),
  candidate `TsoX` (events, rf as (reader id, write id or #0 = initial memory), co as the list
  of writer ids in order); `tso-consistent m0 x : Bool` = `tso-wf` (ids distinct, exactly the
  readers have one rf entry to a same-address same-value writer or to m0, co lists exactly the
  writers once) ∧ `tso-acyclic tso-ghb` (po minus plain-W→plain-R, rf across threads, co, fr;
  mfence and locked events are nodes, so W→mfence→R and W→locked stay ordered; atomicity of a
  locked event is the 2-cycle fr;co) ∧ `tso-acyclic tso-loc` (po on one location, rf, co, fr);
  `TsoConsistent` the type. `tso-exec s` is the candidate a state's trace yields.
  `TsoSound : Type` is the general statement (every schedule from `tso-init`, buffers empty
  afterwards ⇒ consistent); **no term of it exists** (see Limits). `tso-sound-upto n ths : Bool`
  checks `tso-state-ok` at every state the scheduler is asked at and at every end, over every
  schedule of ≤ n transitions. The scheduler's command is now `sched-pick ths s` (it shows the
  state), so one fold (`tso-fold`) serves outcome sets and soundness.
- Soundness instances, each a definition `x-sound : isTrue (tso-sound-upto n x)` `= unit` that
  checks: SB 961,848 steps (`#show … = true`; the definition 963,306); MP 369,590; LB 228,444;
  IRIW 69,471,296 steps (13 s). With 0 cut by the bound in each, these are TsoSound at these
  four programs for schedules of every length.
  All five files, uncached: `881 passed, 0 failed; 77,191,500 steps`, 20.5 s wall.
  (A file that has not changed is `cached:` and its `#show`s are not printed again; to see the
  readings again run with `CUBICAL_NOCACHE=1`, which also rechecks the library, ~20 s more.)

- Each litmus set is also a checked statement `x-exact : tso-outcomes n x ≡ tso-sum x-set #count #0`
  (the set, the number of finished schedules, zero cut), by `λ i → …`: `sb-exact` 119,424
  steps, `mp-exact` 56,853, `lb-exact` 38,982, `iriw-exact` 7,755,380.
- `lib/mem/atomics.cub` (21 definitions check): `MOrd` (`mo-relaxed/mo-acquire/mo-release/
  mo-acq-rel/mo-seq-cst`), `Atom` (`at-load o a`, `at-store o a v`, `at-xchg o a v`, `at-cas so
  fo a old new` answering `Bool × Word`, `at-fence o`), `AtomS`, `atom-valid` (the orderings
  the languages allow per operation), handlers `atom-x86-xchg` (seq_cst store = locked xchg)
  and `atom-x86-mfence` (seq_cst store = store; mfence) `: Handler AtomS TsoIS` (every load a
  plain load, weaker stores plain stores, only the seq_cst fence emits mfence), `at-handle`
  (a handler applied to a `Step`), `AtomThread`, `at-thread`, writers `ai-*`.
- `experiments/tso/atomics-sb.cub`: SB through the atomics. Relaxed: all four outcomes, 80
  schedules, 111,663 steps; release/acquire: all four, 80, 111,641; seq_cst as xchg:
  `{(0,1),(1,0),(1,1)}`, 6 schedules, 8,964 steps; seq_cst as store+mfence: the same set, 70
  schedules, 136,133 steps. `asb-sc-xchg`, `asb-sc-mfence` state the seq_cst sets as paths;
  `asb-sound-xchg` (103,303 steps) and `asb-sound-mfence` (2,317,931) are TsoSound at these
  (executions with locked and mfence events).
  All seven files after the cached library: `921 passed, 0 failed; 91,412,925 steps`, 17 s:
  `./build/cubical $LIBS lib/isa/tso.cub experiments/tso/{sb,mp,lb,iriw}.cub lib/mem/atomics.cub experiments/tso/atomics-sb.cub`

- `lib/isa/tso.cub` gained `tso-every n ths p` (p at every finished end, false if any schedule
  is cut by the bound), `tso-trace`, `tso-sum-outs`, `tso-sum-cut`.
- **Spin-lock** (`experiments/tso/spinlock.cub`), written against the atomics
  (`lock.exchange(1, acquire)` up to k times, then `c = counter.load; counter.store(c+1);
  lock.store(0, release)`), run under `atom-x86-xchg`. `spin-ok` at a schedule's end:
  `spin-mutex` over the trace (no acquisition while another thread holds, between its
  successful locked event and its releasing store entering the buffer; no release without
  holding; no access to the counter without holding) ∧ lock free ∧ counter = number of
  threads that entered. **Mutual exclusion as a type over every schedule**:
  `SpinMutex k n = isTrue (tso-every n (spin-sys k) spin-ok)`, inhabited by `unit` and checked:
  | k (attempts) | bound n = 2k+10 | finished schedules | steps of the check |
  |---|---|---|---|
  | 1 | 12 | 28 | 72,808 |
  | 2 | 14 | 108 | 292,424 |
  | 4 | 18 | (not shown) | 2,155,961 |
  | 8 | 26 | 8,316 | 27,503,546 |
  Outcome sets (who entered): k=1 and k=2 `{(0,1),(1,0),(1,1)}`, 0 cut. Three threads, one
  attempt each (`spin-mutex3-1`, bound 18): 468 schedules, all 7 outcomes except (0,0,0),
  1,766,430 steps. `spin-sound-2`: TsoSound at the k=2 lock, 5,063,870 steps.
  Contrast recorded in the file: acquisition by load-then-store instead of exchange gives
  `tso-every 14 bad-sys spin-ok = false` (50,642 steps) and a lost increment (`= false`,
  179,301 steps).
  `tso.cub`, `atomics.cub`, `spinlock.cub` after the cached library: `905 passed, 0 failed;
  39,023,549 steps`, 16 s.

- **The axiomatic form's own outcome sets** (`lib/isa/tso.cub`, `tso-ax-outcomes m0 dom ths :
  TsoAx` = set of results, candidates, consistent candidates): each thread alone with read
  values from `dom` (`tso-step-fold`, `tso-ax-instr`, `tso-ax-thread(s)`), every rf
  (`tso-ax-rfs`: a reader to m0 or to a same-address same-value writer), every order of the
  writers (`tso-perms`), kept when `tso-consistent`. No machine state, buffer or schedule is
  involved. Readings, each also a checked path `x-ax : tso-ax-outcomes … ≡ tso-ax x-set #c #k`
  against the same `x-set` the operational `x-exact` uses:
  | program | candidates | consistent | set | steps (#show) |
  |---|---|---|---|---|
  | SB | 8 | 8 | all four | 70,161 |
  | MP | 8 | 6 | no (1,0) | 68,945 |
  | LB | 8 | 6 | no (1,1) | 68,615 |
  | IRIW | 32 | 30 | no (1,0,1,0) | 672,234 |
  | SB seq_cst as xchg (locked events) | 8 | 6 | no (0,0) | 87,587 |
  | SB seq_cst as store+mfence (mfence events) | 8 | 6 | no (0,0) | 145,351 |
  So on these six programs the operational and the axiomatic outcome sets are the same list
  (both directions, by computation). `lib/isa/tso.cub` is 138 definitions.
  All eight files after the cached library: `967 passed, 0 failed`.

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

## Limits

- `TsoSound` in general (all programs, all schedules) is stated and not proved. Proved: its
  instances at SB, MP, LB, IRIW over every schedule, by computation. A general proof is an
  invariant on `TsoSt` (every edge of `tso-ghb` and `tso-loc` goes forward in the trace when a
  write is placed at its flush) carried through `tso-step`; it has not been written.
- The consistency predicate is the rf/co ("global happens-before") formulation of x86-TSO
  (Owens–Sarkar–Sewell's model as rendered by Alglave–Maranget–Tautschnig's `x86tso.cat`), not
  OSS's own memory-order-with-reads formulation; the two are equivalent in the literature, and
  that equivalence is not a term here.
- Acyclicity is the decision procedure (peel sources n times); no proof relates it to the
  absence of a path from a node to itself.

- `tso-ax-outcomes` enumerates every permutation of all writers (n!), so it is for litmus-sized
  programs (the spin-lock's axiomatic set is not computed: up to 6 writers × 25 thread-run
  combinations). Only the per-location order enters `tso-consistent`; enumerating per location
  would remove the factor and is not done. Its `dom` must contain every value a read can
  return; that it does is the caller's claim, not checked.
- The converse of `TsoSound` (every consistent candidate of a program is some schedule's
  execution) is not stated as a type; what exists is the equality of the two outcome sets on
  the six programs above.

## Next step

Every item of "What is built" and every Acceptance line has an entry under Done. Remaining:
1. Final measurement: `make library`, then every file of mine uncached in one run
   (`CUBICAL_NOCACHE=1 ./build/cubical $LIBS lib/isa/tso.cub experiments/tso/{sb,mp,lb,iriw}.cub
   lib/mem/atomics.cub experiments/tso/{atomics-sb,spinlock}.cub`); record both under Done.
2. Then the open items under Limits, in this order: the general proof of `TsoSound` (the
   invariant on `TsoSt`), per-location enumeration in `tso-perms`.
