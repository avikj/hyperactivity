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

## Continuation (items 4–7, assigned by the coordinator after items 1–3 were accepted)

4. **`TsoSound` proved in general**, in `lib/isa/tso.cub` (or `lib/isa/tso-sound.cub` loaded
   after it): by induction on the schedule with an invariant relating buffers and memory to
   the witness built so far; lemmas about lists/orders written where used. If the rf/co form
   makes the invariant awkward, restate in the memory-order form (Owens–Sarkar–Sewell) and
   prove the two equivalent or replace one. Also: the acyclicity decision procedure proved
   correct against path-freeness; the converse (completeness) stated and proved, or the exact
   obstacle recorded.
   *Acceptance*: a term `tso-sound : TsoSound` checks; a term relating `tso-acyclic` to
   "no path from a node to itself" checks; `TsoComplete` is a type with a term, or its
   obstacle is under Limits with the precise missing step; no "stated, not proved" entry for
   soundness remains under Limits.
5. **The language-level model**, `lib/mem/rc11.cub`: the C++ (RC11-style) consistency
   predicate for the five orderings over `TsoEv`/`TsoX`-shaped candidates; its outcome sets on
   the litmus programs per ordering choice; as checked paths on those programs through both
   handlers, "outcomes on TSO ⊆ outcomes the language model allows"; that inclusion stated in
   general and proved from `TsoSound` plus "TSO-consistent executions of handled programs are
   RC11-consistent".
   *Acceptance*: the per-ordering outcome sets recorded with steps; the inclusion paths check
   for SB/MP/LB/IRIW × orderings × both handlers; the general inclusion has a term, or exactly
   which lemma is missing is recorded.
6. **Events from real instructions**: in `lib/isa/tso.cub`, the memory-event signature track
   B's model must meet (read/write of n bytes at an address, locked prefix, mfence, thread id)
   and the handler from it onto `TsoS`; a "Needs from track B" section with the exact request;
   checked against whatever B has landed.
   *Acceptance*: the signature and handler check; one program written against the signature
   runs over `tso-world` with its outcome set recorded; the request to B is written.
7. **`tso-ax-outcomes` orders only writes to one address** (n! → product of per-address
   factorials), and the spin-lock's axiomatic set is computed.
   *Acceptance*: the six `x-ax` paths still check (candidate counts restated); the spin-lock's
   axiomatic set recorded with steps and stated equal to the operational one.

## Needs from track A

- OPEN: append `lib/mem/rc11.cub` to `LIB` after `lib/mem/atomics.cub` (+31 checks).
- DONE by track A (commit `a30e0b2b`): both files are in `LIB`. The request was:
  `Makefile`: append `lib/isa/tso.cub lib/mem/atomics.cub` to `LIB` (after `lib/world.cub`'s
  dependents; they need only `lib/proc.cub`, `lib/world.cub`, and `tso.cub` before
  `atomics.cub`), so `make library` checks them: +134 checks, +568,491 steps. Not blocking:
  everything here is checked by naming the files after `$LIBS`.

## Needs from track B

For the imported x86 model's threads to run on the TSO machine (item 6), its memory accesses
must be **commands of `MemEvS`** (`lib/isa/tso.cub`), not operations on a memory held in its
state. Exactly:

1. The generated functions that touch memory are parameterized by the memory interface: where
   the emitter now threads `X86` with a `ByteMap` inside and calls `mem-load`/`mem-store`, the
   two (or however many) lowest memory accessors of the model (read n bytes at an address;
   write n bytes) issue `mev-read n a` (response: the value, little-endian in a `Word`) and
   `mev-write n a v`. Concretely the effectful class becomes
   `args → Regs → Step MemEvS (Outc exception A × Regs)` (registers threaded, memory as
   events; `Step` is `lib/world.cub`'s tree, `scmd c k`), or the same in `Proc` form.
   An instruction's n-byte access must be ONE event of n bytes, not n one-byte events: the
   machine keeps an n-byte store whole in the buffer and reads n bytes in one transition.
   Instruction fetch reads are `mev-read` too.
2. A LOCK-prefixed instruction (and `xchg` with a memory operand, which locks implicitly)
   issues ONE event `mev-rmw n a f` for its memory operand, where `f : Word → Word` is the
   value written as a function of the value read (response: the value read); the flags and
   register results are computed from the response after it. In the Sail source this is a
   read of the destination, pure code, and a write to the same address; the importer must
   emit the middle as `f`. (If that cannot be done for some instruction, say which: the
   fallback is `lock`/`unlock` bracket commands, which need a lock holder in `TsoSt`; not
   built.) `mfence` issues `mev-mfence`. `sfence`/`lfence` issue nothing.
3. The single-processor meaning B has today is the handler of `MemEvS` into `ByteMap` state
   (`mev-read` = `mem-load` of n bytes, `mev-write` = `mem-store`, `mev-rmw` = load, store of
   `f`), which B writes once; nothing else in the model changes.
4. Then one thread is `at-handle tso-of-mev (x86-run … regs)` (a `TsoThread` once its result is
   a `List Word`), and `tso-sys`/`tso-world` over a list of them is the multiprocessor
   semantics.
Checked against what B has landed (`lib/bits.cub` only; no generated model, no `X86` state
yet): nothing to adapt there; `ByteMap` (`mem-load`/`mem-store`) is the state the handler of
point 3 targets.

## Done

How to check (cached library, one file after another; the last line must read `N passed, 0 failed`):
`LIBS=$(make -s -n library | sed -n 's/^.*cubical \(lib\/prelude.*\)$/\1/p'); (ulimit -v 2500000; timeout 600 ./build/cubical $LIBS lib/isa/tso.cub)`

- `lib/isa/tso.cub`, operational part:
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
  7,642,250 steps, 3.4 s for `tso.cub` and the four litmus files after the cached library.
  The four files load together: `./build/cubical $LIBS lib/isa/tso.cub experiments/tso/{sb,mp,lb,iriw}.cub`
  ends `830 passed, 0 failed`.

- `lib/isa/tso.cub`, axiomatic part and the relation:
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
  (both directions, by computation).
  All eight files after the cached library: `967 passed, 0 failed`.

- **Final measurement** (at main `08d47a97` + rebase, 2 shared cores under load ~2.7):
  `make library`: `845 passed, 0 failed; 21,745,292 steps`, 26 s.
  Library and all eight files of this track, uncached, one run
  (`CUBICAL_NOCACHE=1 ./build/cubical $LIBS lib/isa/tso.cub experiments/tso/{sb,mp,lb,iriw}.cub
  lib/mem/atomics.cub experiments/tso/{atomics-sb,spinlock}.cub`, under `ulimit -v 2500000`):
  `1042 passed, 0 failed; 155,954,917 steps`, 90 s wall; so 197 checks and 134.2M steps are
  this track's (of which `iriw-sound` 69.5M and `spin-mutex-8` 27.5M). Counted from that
  run's log: `lib/isa/tso.cub` is 113 checks, 504,758 steps; `lib/mem/atomics.cub` 21 checks,
  63,733 steps; the six experiment files 63 checks.
  Acceptance: line 1 met (this entry); line 2 met (SB, MP, LB, IRIW entries above, sets with
  steps); line 3 met (the spin-lock entry, `SpinMutex`).

- **Item 7 done.** `tso-ax-outcomes` enumerates, through `tso-ax-cos`, every order of the
  writers of each address (groups concatenated) instead of every permutation of all writers;
  `tso-peels` stops at a round that removes nothing. Candidate/consistent counts now: SB 4/4
  (38,381 steps), MP 4/3 (35,454), LB 4/3 (35,433), IRIW 16/15 (345,629), SB seq_cst xchg
  4/3 (50,437), SB seq_cst mfence 4/3 (69,388); the six `x-ax` paths check with these counts.
  **Spin-lock, axiomatic** (`experiments/tso/spinlock.cub`, one attempt each, read values in
  {0,1,2}): 1,322 candidates, 4 consistent, set `{(0,1),(1,0),(1,1)}`, 17,634,373 steps;
  `spin-ax` states it and `spin-op` states the operational set is the same `spin-set`.
  The six experiment files after the library: `1123 passed, 0 failed`.

- **Item 6 done** (acceptance: signature and handler check; a program against the signature
  runs with its outcome sets recorded; request to B written above).
  Machine changes in `lib/isa/tso.cub`: three instructions `t-loadn a n`, `t-storen a n v`,
  `t-rmwn a n f` (n cells from a as bytes, little-endian, one transition each); a buffer entry
  is now the list of cell writes of one store (`buf : Word → List (List TsoB)`), flushed whole
  by `tso-apply`; trace labels and events stay per cell (per byte). `MemEv`
  (`mev-read n a`, `mev-write n a v`, `mev-rmw n a f`, `mev-mfence`), `MemEvS`,
  `tso-of-mev : Handler MemEvS TsoIS`, `tso-of-mev-at t : Handler MemEvS TsoS`; the axiomatic
  enumeration covers the three instructions.
  `experiments/tso/bytes.cub` (threads against `MemEvS`, through `tso-of-mev`):
  | program | outcome set | schedules | steps |
  |---|---|---|---|
  | 16-bit store of 513 ∥ 16-bit load | {0, 513} | 3 | 4,889 |
  | two 8-bit stores (1, 2) ∥ 16-bit load | {0, 1, 513} | 10 | 19,247 |
  | 16-bit store then own load of the high byte | {2} | 2 | 2,113 |
  | `lock inc` ∥ `lock inc` (values seen) | {(0,1),(1,0)} | 2 | 5,769 |
  | `inc` ∥ `inc` without lock | {(0,0),(0,1),(1,0)} | 20 | 55,360 |
  Soundness instances: `by-sound-w2` 35,388 steps, `by-sound-inc` 328,855, `by-sound-inc-nolock`
  11,345,303. The axiomatic set of the first program is {0, 1, 512, 513} (4 candidates, 4
  consistent): see Limits.
  After these machine changes every earlier statement rechecks: the seven experiment files
  after the library, `1156 passed, 0 failed; 179,416,793 steps` (uncached proofs), 145 s under
  load.

- **Item 5, computable part done.** `lib/mem/rc11.cub` (checks after the library; not yet in
  `LIB`, see Needs from track A): relations as lists of id pairs (`rc-union`, `rc-comp`,
  `rc-plus`, `rc-irreflexive`, `rc-dom`/`rc-cod`/`rc-id`, `rc-rel`); a language-level candidate
  `RcX = rc-x (x : TsoX) (modes : List (Word × MOrd))`; `rc11-consistent m0 c : Bool` =
  `tso-wf` ∧ coherence (hb, hb;eco irreflexive, hb = (sb ∪ sw)+ with sw through rs and
  fences) ∧ atomicity (rb;mo irreflexive; an update is one event) ∧ SC (psc_base ∪ psc_F
  acyclic) ∧ no-thin-air (sb ∪ rf acyclic); `Rc11Consistent` the type; `rc11-outcomes m0 dom
  ths` over `AtomThread`s; `rc-subset`; `rc-x86 hd ths`; and the general statement
  `AtomsX86Correct hd : Type` (no term: see Limits).
  RC11 sets (`experiments/tso/rc11.cub`; candidates / consistent / steps):
  | program | relaxed | release/acquire | seq_cst |
  |---|---|---|---|
  | SB | 4/4 all four, 55,273 | 4/4 all four, 60,735 | 4/3 no (0,0), 124,915 |
  | MP | 4/4 all four, 60,920 | 4/3 no (1,0), 67,865 | 4/3 no (1,0), 99,280 |
  | LB | 4/3 no (1,1), 84,173 | 4/3 no (1,1), 94,516 | 4/3 no (1,1), 127,628 |
  | IRIW | 16/16, 512,627 | 16/16, 633,869 | 16/15 no (1,0,1,0), 1,319,871 |
  **Compilation of the atomics to x86 on these programs**: the 24 definitions
  `rl-<prog>-<rlx|ra|sc>-<xchg|mfence> : isTrue (rl-incl hd n ths)` (no schedule cut by the
  bound ∧ TSO outcome set of the handled program ⊆ RC11 set) all check with `= unit`. Steps:
  SB 144k–388k each, MP 119k–224k, LB 147k–373k, IRIW 14.7M (rlx), 14.9M (ra), 2.2M
  (sc, xchg), 160.5M (sc, store+mfence). The two files: `1145 passed, 0 failed; 228,887,935
  steps`.

- **Item 4(a), first half.** `lib/isa/tso-acyclic.cub` (after `lib/isa/tso.cub`; 11 checks so
  far): `TsoIn` (membership), `TsoChain rel x zs y` (a chain x → z₁ → … → y),
  `tso-acyclic-closed` (a nonempty set of the list's nodes each with a predecessor in the set
  makes the decision false) and
  `tso-acyclic-sound : … → ((c : A) → TsoIn c (cons x zs) → TsoIn c ns) → TsoChain rel x zs x →
  isTrue (tso-acyclic rel ns) → Empty`: **when the decision answers true there is no cycle
  through nodes of the list.** Checks: `tso-acyclic-sound [22,288 steps]`.

## Found

- **Elaborator refuses a structural recursion when an argument before the matched one changes.**
  Input (after the library):
  `g1-a : Word → List Word → Word` / `g1-a j nil = j` / `g1-a j (cons x r) = g1-a (%wsucc j) r`
  gives `error: g1-a: a recursive call is not on a constructor field or a field's field from one
  of the definition's own matches (structural recursion)`. With `j` unchanged in the call, or
  with the list as the first parameter, it checks. Every such definition in my files puts the
  matched argument first (`tso-go`, `tso-quiet`, `tso-choices`). The error is fatal for the
  load (later files are not read), unlike a `FAIL:` line.
- **Elaborator refuses a recursion through a continuation rebound by a dependent case** (now
  track S's, `tracks/S-surface.md`). Smallest input (after the library, which has `TsoIS`):
  ```
  g5-f : Step TsoIS Word → Word
  g5-f (sdone a) = a
  g5-f (scmd i kk) =
    (case i return (λ x → (tsoi-resp x → Step TsoIS Word) → Word) of
       t-load a → λ k2 → g5-f (k2 #0)
       t-store a v → λ k2 → g5-f (k2 unit)
       t-xchg a v → λ k2 → g5-f (k2 #0)
       t-cmpxchg a o n → λ k2 → g5-f (k2 #0)
       t-mfence → λ k2 → g5-f (k2 unit)) kk
  ```
  gives `error: g5-f: a recursive call is not on a constructor field or a field's field from
  one of the definition's own matches (structural recursion)` (fatal for the load). `k2` is
  `kk`, the matched constructor's field, passed through the case's motive so its domain
  `tsoi-resp i` computes in each branch; the call `g5-f (k2 v)` is on that field applied.
  The non-dependent form `g (scmd c k) = h c (λ x → g (k x))` is accepted (`tso-step-fold`,
  `at-handle`), which is the way round used here.
- An unchanged file is `cached:` and its `#show` readings are not printed again.
- **"implicit argument is not determined" when a lemma about `isTrue (case l of …)` is applied
  at a non-variable list** (for track S). With
  `g7-nn : (A : Type) → (l : List A) → (x : A) → TsoIn {A} x l → isTrue (case l return (λ w → Bool) of nil → true; cons y r → false) → Empty`
  (checks), the use
  `g7-d : {A : Type} → (rel : A → A → Bool) → (ns : List A) → (x : A) → TsoIn x (tso-peels (tso-len ns) rel ns) → isTrue (tso-acyclic rel ns) → Empty`,
  `g7-d rel ns x m h = g7-nn A (tso-peels (tso-len ns) rel ns) x m h`
  is `FAIL … at: (case (case (case ns … reason: an implicit argument is not determined by the
  other arguments' types or the type expected; its type: Type`, with every implicit written
  out as well. The same proof with the case inlined
  (`(case tso-peels … return (λ l → TsoIn x l → isTrue (case l return (λ w → Bool) of …) → Empty) of nil → λ m h → m; cons y r → λ m h → h) m h`)
  checks (`tso-acyclic-closed`).
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

- `tso-ax-outcomes`'s `dom` must contain every value a read can return; that it does is the
  caller's claim, not checked.
- The axiomatic form's events are per cell (byte) and it has no axiom that the bytes of one
  n-byte store reach memory together (nor that an n-byte load is one point in the order), so
  on programs with multi-byte accesses it allows torn reads the machine does not produce
  (`by-w2`: axiomatic {0,1,512,513}, operational {0,513}). Soundness (machine ⊆ axiomatic) is
  unaffected; the two outcome sets are equal only for single-cell programs. Closing it: an
  "atomic group" relation on events (same store / same load) and the axiom that no other
  thread's write to the group's cells is ordered between the group's members.
- `AtomsX86Correct hd` (TSO outcomes of every handled program ⊆ RC11 outcomes) is stated and
  has no term. Proved: its 24 instances above, by computation. Missing for the general proof,
  in order: (1) `TsoSound` in general (item 4); (2) a map from a TSO execution of a handled
  program to a language-level candidate (events of one atomic command merged: a seq_cst
  store's `W;F` or `L` becomes one `W`, a failed `lock cmpxchg`'s `L` becomes an `R`, the
  ordering attached), preserving results; (3) the lemma that this map takes TSO-consistent to
  RC11-consistent: hb ⊆ the TSO order (po ∪ rfe-forward), eco forward in the same order
  except plain W→R pairs, and for psc that between seq_cst events of one thread a locked
  event or an mfence intervenes. None of (2), (3) is written.
- The converse of `TsoSound` (every consistent candidate of a program is some schedule's
  execution) is not stated as a type; what exists is the equality of the two outcome sets on
  the six programs above.

## Next step

Items 1–3, 6, 7 complete; item 5's computable part complete, its general proof depends on
item 4. Now: item 4, in this order.
(a) In `lib/isa/tso-acyclic.cub`: `tso-acyclic-sound` is done. Next, appended to that file,
    `tso-acyclic-stamp`: a stamp `A → Nat` increasing along `rel` on members ⇒
    `tso-acyclic rel ns ≡ true` (a member of least stamp has no predecessor, so a round on a
    nonempty list shortens it).
(b) `TsoSound`. Design decided (not started): give every transition, flushes included, a
    fresh id used as its time; the witness's order on writes becomes `List (Word × Word)`
    (writer id, commit time) and `tso-annot` reads `cp`/`scp` as commit times with
    earliest-match lookups defined by recursion on the latest-first log (so a lookup on
    `cons l lg` unfolds one step and a value once defined never changes). Stamp for
    `tso-ghb`: a write's commit time, any other event's id. Stamp for `tso-loc`: the pair
    (commit time of the source if it was pending at the read, else the read's id; the read's
    id). Invariants on `TsoSt`, each preserved by `tso-read1`, the store's enqueue,
    `tso-apply`, `tso-lockn`, the fence: (I1) a committed write's commit time exceeds its id;
    (I2) one thread's writes commit in id order; (I3) at a locked event or fence of thread t
    every earlier write of t is committed; (I4) a read from memory has its source committed
    before it; (I5) a read from the buffer has its own thread's newest pending write to the
    address as source; (I6) ids below `next` and distinct. Ids/times should become `Nat` (or
    the order lemmas on `%wlt` be proved first: transitivity, irreflexivity, n < n+1).
(c) The converse: state `TsoComplete`; record the obstacle if not proved.
`$LIBS` already contains `lib/isa/tso.cub lib/mem/atomics.cub`: name `lib/mem/rc11.cub` and the
experiment files after it.
