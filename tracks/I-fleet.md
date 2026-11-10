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
(ulimit -v 2500000; timeout 600 ./build/cubical $L lib/net.cub lib/fleet.cub experiments/fleet/ping.cub | grep -v '^ok: \|^cached: ')
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
## Done

- `lib/net.cub` (44 definitions): signature, state, `Adv`, `EnvS`, `resolve` with `resolve-spec`,
  `net-world`, list-as-set lemmas, fairness predicates (`SchedFair`, `DeliveryFair`, bounded
  `SchedFairB`/`DeliveryFairB`, `seq-of-str` for streams).
- `lib/fleet.cub` so far: the step, the runs, `fleet-replay`, `Safe` with `safe-ind`, `safe-at`
  and the per-event rule `safe-by-events` (`StepRun`, `StepDeliver`, `StepNet`), `proc-view`,
  `procs-init`, `fin-procs`.
- `experiments/fleet/ping.cub`: two processes exchanging one message; `#show` gives `#42`
  (582 steps), the trace `run 1, run 0, deliver (0,1,#42)`, flight empty after.
  Measured: library + the three files, 864 passed, 0 failed.

## Found

(nothing yet)

## Next step

In `lib/fleet.cub`, in this order: (1) `Live` with `Fair` (`Took`, `Gone` from `dl-go` over a
`Resolved`), the measure rule `live-measure`, and the waiting lemma (a node whose view is a
send keeps its state until it is run); (2) `Converges` with the set-fold lemma for a
commutative idempotent merge, and `wmax`'s laws through `maxw`; (3) `Refines` as trace
inclusion against a `World` (`HistOk`); (4) the guarded instance `gview`, machines as
processes (`mach-proc`). Then `experiments/fleet/kv/kv.cub`.
