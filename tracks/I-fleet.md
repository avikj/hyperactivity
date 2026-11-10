# Track I — systems: the network world, fleets, property types, deployment (Phases 7–8, D5)

Read `README.md` here (rules), `../PLAN.md` §6 Phases 7–8, and `lib/proc.cub` (worlds,
supervision, replay, `GProc`), `lib/world.cub`, `lib/choreo.cub`, `lib/session.cub`,
`lib/elf.cub` and `experiments/native/feed.cub` (how a process becomes a binary).

Files you own: `lib/net.cub`, `lib/fleet.cub`, `lib/deploy.cub`, `experiments/fleet/**`, this file.

## What is built

1. **`lib/net.cub`**: the network as a world. Node ids; messages as `Bytes`; the signature
   `NetS` a node sees (`send to m`, `recv` → a message or a timeout, `now`); the network state
   (in-flight multiset as a list); the **environment signature** whose responses are the
   adversary's choices at each step (deliver which, drop, duplicate, delay, partition), and
   the network as a `World` over node commands given those choices. Fairness as a predicate
   on infinite choice sequences (every message sent to a live, connected node is eventually
   delivered) in guarded/coinductive form, and bounded forms by fuel.
2. **`lib/fleet.cub`**: a fleet is `nodes : Fin n → Proc NetS A` (and guarded nodes) composed
   under the network: `fleet-run : choices → state → Outcome`, small-step and by fuel; the
   global state type; traces. **Property types**, each a type whose inhabitants are proofs:
   - `Safe inv fleet`: `inv` holds in every state reachable under every choice sequence;
     with the induction principle (holds initially, preserved by every step) as a lemma;
   - `Live fair goal fleet`: under every fair choice sequence the goal is eventually reached
     (a well-founded measure that decreases on fair steps, as the proof rule);
   - `Refines spec fleet`: the fleet's observable behaviour (client requests/responses) is
     that of a sequential specification process — stated with track H's `≈` when it lands
     (`lib/obs.cub`); until then with trace inclusion defined here, to be replaced;
   - `Converges`: replica state is a function of the **set** of updates received (state as a
     fold over a set, commutative/idempotent merge), so two replicas with equal sets are equal;
   - replay: `fleet-run` is a function of the recorded choices; equal choices, equal run.
3. **`experiments/fleet/kv/`**: a replicated key-value service of three nodes and a client:
   writes are broadcast, each node logs then applies; the five statements of PLAN's fleet
   acceptance stated and proved (safety by the invariant rule; liveness under fairness for a
   bounded number of in-flight messages; refinement to a single map for the sequential-client
   case; convergence by the set-fold lemma; replay by `refl`).
4. **`lib/deploy.cub`**: `Config` (per-node role, address, peers, platform parameters from
   `lib/os/platform.cub` once it exists), `Corpus` (the node programs by role), and
   `deploy : Corpus → Config → Fleet` as a term; the per-node process handled into `W`/Linux
   commands (track D's `lib/os/linux.cub`; until it lands, into `lib/world.cub`'s `W` with the
   network over files/pipes, replaced later) so that `#build` writes each node's binary;
   **change**: an upgrade as two corpora with a compatibility relation on the node-to-node
   message types (an equivalence or a retraction), and the statement that a mixed fleet
   (some nodes old, some new) keeps `Safe`.

## Acceptance

- `make library` passes; your files load after it with `N passed, 0 failed`, no postulates.
- The kv fleet's five statements checked; a `#show` of a 3-node run under a recorded choice
  sequence with a drop and a duplicate, its final replica states equal.
- `#build` of one node of the kv fleet (through `W`) produces a binary; three of them run as
  processes connected by pipes or UDP on localhost and serve a client's writes and a read.

## How to check (every command from `scoped-machine/`)

```
L=$(make -s -n library | sed -n 's/^.*cubical \(lib\/prelude.*\)$/\1/p')
(ulimit -v 2500000; timeout 600 ./build/cubical $L lib/net.cub lib/fleet.cub experiments/fleet/ping.cub \
  experiments/fleet/kv/kv.cub experiments/fleet/kv/run.cub experiments/fleet/kv/liveness.cub | grep -v '^ok: \|^cached: ')
```
The library is read from the cache (about 1 s); the last line is `N passed, 0 failed`.

## Design (fixed; everything below builds on it)

- `lib/net.cub`. Node ids are `Nat`. `NetSig M` = `n-send to m` → `Unit`, `n-recv` → `Rcv M`
  (`r-timeout | r-msg from m`), `n-now` → `Word`; `NetS = NetSig Msg`, `Msg = (n : Nat) × Bytes n`.
  `Net M` = packets in flight (list of `Pkt M` = from, to, msg), clock, partition side. The
  adversary's choice `Adv` = `a-run i | a-deliver k | a-drop k | a-dup k | a-tick d | a-part s`;
  `EnvS = PushS Adv`. `resolve : Adv → Net M → Resolved M` = `res-node i e` (node `i` scheduled
  with `e = ev-run | ev-deliver p rest`) or `res-net n`; `resolve-spec` says a delivered packet
  was in flight and nothing is added. `net-world : NodeId → Ev M → World (NetSig M) (Net M)`.
  Sets of packets are lists with `InL` (membership, a type) and `SubL` (inclusion).
- `lib/fleet.cub`. A node is a state with a view `NView M St` (`v-halt | v-send to m s | v-recv k
  | v-now k | v-tau s`); `Beh M St = NodeId → St → NView M St`; `FState M St = (NodeId → St) × Net M`.
  `fleet-step beh a g = fl-go beh g (resolve a (snd g))`, `node-step beh i e g = ns-go i e g (beh i (fst g i))`
  (state a lemma over an arbitrary `Resolved`/`NView` and apply it: that is how every proof
  about a step is written). `fleet-run` (list of choices), `fleet-at` (infinite sequence, by
  fuel), `fleet-trace`, `fleet-outcome`, `fleet-proc` (over `EnvS`). Processes are nodes by
  `proc-view` (state = `Step (NetSig M) A`).

## Needs from track A

1. **`#build-loop` for a machine whose state holds lists.** A deployed node is
   `init : St` and `step : St → Step W St` (one command of the machine handled into `W`) with
   `St` = `KvSt` of `experiments/fleet/kv/kv.cub` (records of `List (Word × Word)`, a data type
   `KvPend`, a `Hist` of dependent pairs) plus the socket and buffer words. Today
   `#build-loop` wants a hand-written `copy` with `copy-id` and refuses byte strings; A's own
   next step 1 (derive `copy`/`copy-id` from the data table) is what is needed here, for
   lists and nested records. Until then nodes are built by `#build` with a run-time bound of
   100000 machine steps (Found 6). Smallest input: `experiments/fleet/kv/build0.cub`.
2. **`#build` cost.** `kv-node0` is 145 M steps (70–100 s) for a replica of ~40 lines; four
   nodes are four such builds. Same cause as track D's item 7 (the handler stays in the code
   behind a stuck case). Nothing is blocked on it.

## Done

- `lib/net.cub` (49 definitions): signature, state, `Adv`, `EnvS`, `resolve` with `resolve-spec`,
  `net-world`, list-as-set lemmas, time (`nt-later`, `Eventually`, `WithinB`), fairness
  predicates (`SchedFair`, `DeliveryFair`, bounded `SchedFairB`/`DeliveryFairB`, `seq-of-str`).
- `lib/fleet.cub` (the generic part, complete):
  - the step, the runs, `fleet-replay`;
  - `Safe` with `safe-ind`, `safe-at`, and the per-event rule `safe-by-events` / `step-by-events`
    (`StepRun`, `StepDeliver`, `StepNet`);
  - `Fair`/`FairB` (`Took`, `Gone`), `Live`, the measure rule `live-measure`, `stable-later`,
    `step-other` (a node not receiving keeps its state under every choice but its own run),
    `wait-run` (the first run of a node from a time on finds it in the state it had),
    `step-run-eq`;
  - `Converges` with `converges-fold` (a fold of a commutative, associative, idempotent merge
    with unit over a list sees only the set), `maxw-semilat`, `wmax-semilat`;
  - `Refines` as trace inclusion against a `World` (`HistTo`, `HistOk`);
  - processes as nodes (`proc-view`, `procs-init`, `fin-procs`), machines as processes
    (`mach-proc`, `mach-proc-w`, the one-step lemma `mach-step`), guarded processes as nodes
    (`GNode`, `gview`, `gproc-beh`), `mach-gproc`.
- `experiments/fleet/ping.cub`: two processes exchanging one message (`#42`, 582 steps; trace
  `run 1, run 0, deliver (0,1,#42)`); the same with guarded nodes and with a machine under a
  duplicate (`#22`, 974 steps).
  Measured: library + the three files, 929 passed, 0 failed, 1.7 s from the cache.

- `lib/fleet.cub` also: the local-invariant rule `safe-local` (an invariant of each node's own
  state that its view preserves is `Safe`: `VOk`, `LocalInv`, `ns-self`, `ns-local`), and
  `fleet-proc-run` (the fleet process over `EnvS` run by `runp` against a recorded list through
  `feed-world` ends `done (fleet-run …)` with the record used up).
- `experiments/fleet/kv/kv.cub`: writes, messages, `kv-store`, the replica (`kv-rep-view`), the
  client state, `kv-beh cb` (node 3's behaviour a parameter), the broadcasting client
  (`kv-bc-view`), the sequential client (`kv-seq-view`).
  - **Safety** `kv-safe`: for every client behaviour `cb` and client state, in every reachable
    state each replica has `applied ⊆ log` and an owed acknowledgement's write logged
    (`safe-local`); `kv-applied-logged` is the PLAN wording for replica 0.
  - **Convergence** `kv-converges : Converges kv-store` (stores are equal as functions when the
    applied lists are the same set), `kv-replicas-agree`.
- `experiments/fleet/kv/run.cub`: a recorded run of 43 choices (partition and heal, tick,
  duplicate of the first write to replica 1, drop of replica 1's first ack). `#show`: the
  client's history `get 1 = 11` after three puts (12,043 steps), replicas' applied lists
  (replica 1 holds `(1,10)` twice), stores at key 1 all `#11`; **final replica states equal**
  as functions (`kvr-stores-01` by `kv-replicas-agree`, `kvr-stores-02` by `refl`).
  - **Replay** `kvr-replay` (the fleet process against the record by `feed-world` ends
    `done kvr-final , nil`, by `fleet-proc-run`), `kvr-replay-eq` (equal choices, equal run).
  Measured: library + net, fleet, ping, kv, run: 996 passed, 0 failed.

- `experiments/fleet/kv/liveness.cub`: **Liveness** `kv-live : (c0 : KvCli) → KvAllRep (c-out c0) →
  Live 4 (KvAllApp (c-out c0)) kv-bc-beh (kv-init c0)`: for the broadcasting client with any
  finite list of (replica, write) still to send, under every `Fair 4` sequence there is a time
  at which every write is applied by the replica it was addressed to; `kv-live-bcast ws` for
  `kv-bcast ws` (every write to every replica). No bound on the number of writes or of packets
  in flight is needed. Proof: `kvl-client` (scheduling fairness, `wait-run`, `step-run-eq`),
  `kvl-deliver` (delivery fairness, `took-elim`, a replica never halts), `kvl-replica`,
  `kv-app-later` (applied stays applied, `ns-local` + `stable-later`), `kvl-claim` (induction on
  the list, times merged by `nt-later-comm`). `lib/fleet.cub` gained `took-elim`.
  `kv-client-finishes` uses the measure rule `live-measure` on this fleet (measure: the number
  of writes still to send; premise from `kvl-client`).
  Measured: library + net, fleet, kv, liveness: 1033 passed, 0 failed, 3,457,330 steps, 27 s
  uncached (`kvl-owe` 446,906 steps is the largest).

- `lib/deploy.cub`: `handle-l` (handle at any level), `Wire` (four words), `Codec`, `NodeCfg`,
  `ConfigData`, `Config` (the data with the platform's little-endian fact), `Corpus` (view and
  initial state by role, codec, exit status), `net-on-linux` (the node signature as Linux
  commands on a bound UDP socket: `dp-send`, `dp-recv`, `dp-now`), `dp-node` (socket, bind,
  buffer, the role's machine by `mach-proc-w` through `handle-l net-on-linux`, `exit_group` with
  the corpus's status), `Fleet`, `deploy : Corpus M St → Config → Fleet M St` (behaviour, initial
  state, and each node's `Step W` through `linux-run`).
- `experiments/fleet/kv/deploy.cub`: `kv-enc`/`kv-dec` with `kv-dec-enc`, `kv-corpus prog` (role 0
  replica, role 1 the sequential client), `kv-config` (four nodes on 127.0.0.1:47600–47603,
  `x86-64-linux #2`, fuel 100000), `kv-fleet = deploy (kv-corpus kv-prog) kv-config`.
- **`#build` of node 0** (`experiments/fleet/kv/build0.cub`): `ok: build kv-node0 [build/kv-node0,
  107132 bytes, the checker's conversion, 144854613 steps]` (69–100 s). Run alone and driven
  from python bound to 127.0.0.1:47603 (node 3's address): three puts acknowledged
  (`(1,1,10,0)`, `(1,2,20,0)`, `(1,1,11,0)`), `get 1 → (3,7,11,0)`, `get 2 → (3,8,20,0)`,
  `get 5 → (3,9,0,0)`; under `strace` its calls are `mmap` ×3, `socket`, `bind 127.0.0.1:47600`,
  then one `recvfrom`/`sendto` pair per request.

## Found

1. **Conversion misses a refined scrutinee (checker incompleteness).** With `f` defined by
   clauses on a `Nat` (`kv-beh cb zero st = …st…`), the type `P (f j (h j))` written under a
   binder of `j` (a `subst` motive `λ j → …`, or the last clause `(suc (suc (suc j)))` of a
   definition whose type mentions `kv-bc-beh r (fst g r)`) reads back with `j` replaced by the
   constructor inside the branch (`case j of zero → … (fst g zero)`), while the same type
   reached by unfolding a definition applied to `j` keeps `fst g j` in the branch; the two are
   reported as a type mismatch (`expected … fst g i …  actual … fst g zero …`). Seen in
   `kvl-took` and `kvl-owe` (first versions, commit history of `liveness.cub`). Worked around by
   stating the transport over a variable behaviour (`took-elim` in `lib/fleet.cub`) and by
   splitting the absurd clause as the function's own clauses split.
2. **Out of memory on a `let` of a large Σ used several times.** `kvl-client` written as
   `let ev = fst fair t 3 unit in let wr = wait-run … (fst ev) t (snd ev) … in (suc (fst wr) , subst … (step-run-eq … (fst (snd wr))) (kvl-sent … (concat (λ i → c-out (snd (snd (snd wr) i))) e)))`
   exhausted `ulimit -v 2500000` after 40 s; the same proof with the Σ passed as an argument
   to a helper (`kvl-client-k`) checks in 199,177 + ~30,000 steps. Every lemma of
   `liveness.cub` is therefore written with intermediate results as arguments, never `let`.
3. A signature must be followed at once by its clauses (`kvl-claim :` followed by another
   signature is a parse error `expected =, found :`), and a multi-binding `let` inside
   parentheses is not laid out (`expected in`): one `let … in` per binding.
5. **Out of memory on `<` over a run's state.** `subst {B = λ o → above (kv-todo (fleet-at … t)) (nt-len o)} …`
   (the measure premise written directly over `fleet-at`) exhausted 2.5 GB after 60 s; the same
   two `subst`s as a lemma over three list variables (`kvl-len-lt`) applied to the run's lists
   check in 33,745 steps.
6. **`#build` unfolds a loop bound it knows; a large bound at run time exits 3.** With
   `cf-fuel` a literal reaching `mach-proc-w` (`%welimP` on `#1000000000000`), `#build` of
   node 0 ran out of 2.5 GB in 45 s; with the literal `#2` it did not finish in 600 s. With the
   bound written to memory and read back (`lx-poke64`/`lx-peek64`, so a run-time value) the
   build takes 145 M steps; the binary with bound 10¹² then leaves with status 3 right after
   its start (strace: `socket`, `bind`, `mmap`, `exit_group(3)`), with bound 100000 it serves.
   So a `%welimP` loop costs memory in its bound before its first round. A node that runs
   for ever needs `#build-loop` (see Needs).
7. `k-put` (mine, a constructor of `KvCmd`) clashed with `lib/os/contract.cub`'s `k-put` once
   `lib/os` joined `LIB`: mine are now `kc-put`, `kc-get`.
4. A type alias that hides a Π (`Beh M St`, `LocalInv P beh`) cannot be the signature of a
   definition by clauses (`a dimension is matched only by a variable`): the Π is written out.

## Next step

1. `lib/deploy.cub`, the node binary, the localhost run (see "What is built" 4 and Acceptance):
   done before refinement because it may need track A.
2. `experiments/fleet/kv/refine.cub`: `Refines kv-map-world (λ k → #0) (client history) kv-seq-beh
   (kv-init (kv-seq-cli prog))` by `safe-by-events` with the invariant: applied₀ ⊆ issued,
   acknowledged ⊆ applied₀, every `m-put` in flight issued, every `m-ack` from 0 applied at 0,
   every `m-get`/`m-val`(from 0)/owed value with request id ≤ `c-idx` and, when equal, the
   client waiting on that key and the value `kv-store conf k`; history is the map's. Write
   every lemma over explicit fields (records have η) and pass intermediate results as
   arguments (Found 2).
