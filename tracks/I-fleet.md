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

## Needs from track A
## Done
## Found

## Next step

Read `lib/proc.cub` and `lib/choreo.cub`; write `lib/net.cub` items (signature, state, the
world under choices) and a `#show` of two nodes exchanging one message; commit.
