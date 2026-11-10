# Track H — equality of behaviour by observation (Phase 6)

Read `README.md` here (rules), `../PLAN.md` §6 Phase 6, and `lib/proc.cub`, `lib/world.cub`,
`lib/std.cub` (set quotients, truncations), `lib/session.cub`, `lib/choreo.cub`.

Files you own: `lib/obs.cub`, `experiments/obs/**`, this file.

## What is built

1. Signature morphisms and `obs`: for `i : Obs ↪ S` (an inclusion of signatures) and a handler
   of the rest of `S`, the projection `obs : Proc S A → Proc Obs A`; functoriality as paths.
2. **Trace semantics**: the type of traces of a process over a signature (finite: lists of
   command/response pairs ending in a result; for guarded processes: the coinductive form),
   `traces : Proc S A → Trace S A → Type`, and trace equality.
3. **Bisimulation** for `Proc` (Church form and the guarded form `GProc`): the relation, its
   reflexivity/symmetry/transitivity, and **bisimilar ⇒ equal traces**. For the Church form,
   show bisimilarity coincides with path equality where it does (free monad on a signature is
   an inductive type: its equality is structural) and state precisely where the guarded form
   differs.
4. **The quotient**: `Beh Obs A = Proc Obs A / ~` as a set quotient (lib/std.cub's `_/_`), the
   map `beh : Proc S A → Beh Obs A`, and `p ≈ q := beh p ≡ beh q`; congruence of `≈` under
   `bind` and under handlers; transport of a predicate on behaviours along `≈` (a property
   stated over `Obs` proved of `p` holds of `q`).
5. **Cross-language shape**: two processes over *different* signatures `S₁`, `S₂` (stand-ins
   for two languages' abstract machines) with handlers into a common `Obs`, and the statement
   and proof that they are `≈` for a concrete pair in `experiments/obs/`: a counter service
   implemented once with a mutable cell signature and once with a state-passing signature,
   observed through request/response commands.
6. **Up-to techniques**: bisimulation up to bind and up to context, as lemmas that reduce a
   proof of `≈` to a relation on states (the form proofs about real programs will use).

## Acceptance

- `make library` passes; `lib/obs.cub` and `experiments/obs/*.cub` load with `N passed, 0 failed`,
  no postulates (`#trusted` after loading lists nothing of yours).
- The counter pair of item 5 proved `≈`, and a property (the reply to the n-th `get` is the
  number of `inc`s before it) proved of one and obtained for the other by transport.

## Needs from track A
## Done
## Found

## Next step

Read `lib/proc.cub` fully (Church `Proc`, `GProc`, `handle`, `World`, `runp`) and `lib/std.cub`'s
quotient section; write items 1–2 and commit when they check.
