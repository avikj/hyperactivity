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

## How to check

From `scoped-machine/` (library files are cached; do not set CUBICAL_NOCACHE):
```
L="lib/prelude.cub lib/code.cub lib/match.cub lib/nav.cub lib/cost.cub lib/lin.cub lib/check.cub lib/std.cub lib/sort.cub lib/linproof.cub lib/eqproof.cub lib/ring.cub lib/autodiff.cub lib/intproof.cub lib/intlin.cub lib/prim.cub lib/derive.cub lib/decode.cub lib/fiber.cub lib/proc.cub lib/world.cub lib/session.cub lib/choreo.cub"
(ulimit -v 2500000; timeout 600 ./build/cubical $L lib/obs.cub experiments/obs/*.cub | grep -v '^ok' | tail)
```

## Design (decided; read before continuing)

- The Church `Proc` has no induction principle, so everything that needs induction is stated of
  the tree `Step S A` (lib/world.cub; it is what `#run` walks). Church processes enter through
  `to-step`. A Church term is *natural* (`o-Natural`, `o-NaturalAt` across levels) when each of its
  interpretations is the fold of its tree; `ret`, `cmd`, `bind`, `o-reify` preserve it. Theorems
  about Church `bind`/`handle` carry naturality hypotheses; nothing else can be proved
  internally (no parametricity).
- All names in `lib/obs.cub` are prefixed `o-` (trees) / `og-` (guarded form).
- `_∙_` chains often fail to infer their implicit type; use `calc a ≡ b by p ≡ c by q` with the
  folds' implicits written out (`o-fold {l} {S} {A} {R} …`).

## Needs from track A

- Add `lib/obs.cub` to the `library` target's file list in `Makefile`, after `lib/choreo.cub`
  (it needs `lib/proc.cub` and `lib/world.cub` only). Until then it is checked by the command above.

## Done

- Item 1, stateless half (`lib/obs.cub`, 780 passed 0 failed with the library, 5.09M steps total):
  `o-fold`, `o-reify` (+ `o-reify-step`), `o-bind`/`o-map` with the monad laws (`o-bind-ret`,
  `o-bind-assoc`, `o-fold-bind`, `o-bind-fold`); naturality (`o-NaturalAt`, `o-Natural`,
  `o-natural-ret/cmd/reify/bind`, `o-step-bind`, `o-natural-path`: natural processes with equal
  trees are equal Church terms); signature morphisms as tree handlers `o-THandler`, `o-handle`,
  functoriality `o-handle-id`, `o-handle-comp`, `o-handle-bind`; the Church `handle` is `o-handle`
  on trees for natural arguments (`o-handle-step`); inclusions `o-incl`, `o-lift`, `o-lift-id`,
  `o-lift-comp`.

- Item 1, stateful half and item 2 (803 passed 0 failed, 15.7M steps): `o-ObsOf`, `o-run`, `o-obs`,
  `o-run-bind`, `o-run-handle` (observation after a morphism is observation at the composite),
  `o-ido`/`o-obs-id`, `o-compo`/`o-run-comp` (two observations compose, states paired),
  `o-Section`/`o-obs-lift` (obs after lift along `i : O ↪ S` is the identity), the sum signature
  `o-SumS`, `o-RWorld`, `o-split`, `o-inl`, `o-inr`, `o-split-section`; traces `o-Tr`, `o-traces`,
  `o-TrEq`, `o-ptraces`.

- Item 3, tree half (821 passed 0 failed, 15.6M steps): `o-J`; `o-BisimR Q` and `o-Bisim`;
  `o-bisim-refl/sym/trans`; bisimilarity is path equality of trees (`o-bisim-path`,
  `o-path-bisim`); `o-bisim-traces` (bisimilar ⇒ equal traces); the relational form's
  `o-bisimR-sym/mono/trans`; `o-isProp-bisimR`, `o-isProp-bisim` (commands and results sets) and
  `o-isSet-step` (trees over a set of commands with a set of results are a set; nothing is asked
  of the response types).

- Item 4, first half (836 passed 0 failed, 18.1M steps): the set quotient `o-Quot` (HIT with
  `o-qin`, `o-qeq`, `o-qsquash`), `o-qrec`, `o-qset`; `o-PBisim`; `o-Beh O A`; `o-class`, `o-tbeh`,
  `o-beh H p s`; `o-ObsEq H1 p s1 H2 q s2` (the `≈` across two signatures: a path in the
  quotient); `o-tbeh-eq` (bisimilar observed trees ⇒ equal behaviours); the normal form `o-nf`
  with `o-nf-tbeh`; effectiveness `o-obseq-tree`; transport of predicates `o-beh-transport`,
  `o-tree-transport`.

- Item 4, rest, and item 6 (851 passed 0 failed, 19.7M steps): `o-qelimP` (eliminator into
  propositions, any level); `o-nf-sec`, `o-nf-ret` (for sets, `o-Beh O A` and `Step O A` are
  inverse to each other: the quotient encoded in path-free data); congruences `o-bisim-bind`,
  `o-bisim-handle`, `o-bisim-obs` (trees), `o-pbisim-bind`, `o-pbisim-handle` (Church, natural);
  up to bind `o-bisimR-bind`; `o-StRel`, `o-RunSim Rel H1 t s1 H2 u s2`, `o-runsim-obs`,
  `o-runsim-obseq` (similar runs ⇒ `o-ObsEq`), `o-runsim-bind`, up to context `o-runsim-context`.

- Item 5, first half (`experiments/obs/counter.cub`; with `lib/obs.cub` 884 passed 0 failed,
  19.6M steps): observation signature `oc-Obs` (`oc-recv : oc-Req`, `oc-reply n : Unit`); the
  cell machine `oc-S1 = o-SumS oc-Obs oc-CellS` with `oc-H1`, the state-passing machine
  `oc-S2 = o-SumS oc-Obs oc-StS` with `oc-H2`; the service twice (`oc-svc1`, `oc-svc2`, Church
  processes, bounded by a number of requests); `oc-sim` (`o-RunSim` at every pair of equal
  states, by induction on the bound) and **`oc-equal : (n : Nat) → o-ObsEq oc-H1 (oc-svc1 n) zero oc-H2 (oc-svc2 n) zero`**.
  Also `o-traces-cons` (trace inversion) in `lib/obs.cub`.

- Item 5, second half = both acceptance lines (`lib/obs.cub` + `experiments/obs/counter.cub`:
  901 passed 0 failed; `#trusted` after loading lists only the library's primitives, nothing
  `o-`/`oc-`): the property `oc-ok`/`oc-Good` on traces over `oc-Obs` (each reply is the number
  of incs before the oldest unanswered get; an unowed reply is not allowed);
  `oc-good-svc2 : (n) → oc-Good (o-obs oc-H2 (to-step (oc-svc2 n)) zero)` by induction;
  **`oc-good-svc1`** for the cell implementation by `o-tree-transport` along `sym (oc-equal n)`
  (201k steps), with `oc-isSet-cmd` (Hedberg over `oc-disc`) and `oc-isSet-unit`; `oc-witness`
  (a concrete trace inc, get, reply 1, stop of `oc-svc1 3`, so the property is not vacuous).

## Found

- `lib/std.cub` has no set quotient `_/_` and no truncations (the track text says it has); the
  only truncations in the repo are `T2`/`T3`/`K2` in `experiments/brunerie/typed.cub`. The set
  quotient is therefore defined in `lib/obs.cub` (`o-Quot`).

- `#type D` for a data type `D` ends the run: input `data o-Q2 (A : Type) = o-q2in (a : A)` then
  `#type o-Q2` prints `error: code: not a bound variable or checked definition: o-Q2` and exits 1
  (the data type itself is fine; `#type` of a definition works).

- Structural recursion on a later argument is refused when an earlier argument of a data type
  changes in the call. Input:
  ```
  zr-f : Nat → List Nat → Nat
  zr-f n (cons e r) = zr-f (suc n) r
  zr-f n nil = n
  ```
  ends the run with `error: zr-f: a recursive call is not on a constructor field or a field's
  field from one of the definition's own matches (structural recursion), in zr-f`. With the
  arguments in the other order (`List Nat → Nat → Nat`) it checks. Worked around in
  `counter.cub` (`oc-okt` takes the trace first; `oc-ok` reorders).
- Not a defect, a boundary: for the Church `Proc`, "bisimilar ⇒ equal" is provable only for
  natural processes (`o-natural-path`); for an arbitrary Church term it would need
  parametricity, which the theory does not have internally. Likewise `to-step (bind p f)` is
  the bind of the trees only for natural `p` (`o-step-bind`).
- Complete traces do not determine the tree when a response type is empty (a command with no
  possible response has no complete trace, whatever its continuation), so only
  bisimilar ⇒ equal traces is stated, not the converse.

## Next step

In `lib/obs.cub`, the guarded form (item 3 second half, item 2's coinductive traces), in order:
1. `og-BisimR`/`og-Bisim` on `GProc κ S A` by guarded recursion (a relation into `Type` defined
   with `fix`; the later relation applied under a tick), reflexivity, symmetry, transitivity.
2. guarded traces `og-Tr κ S A` (data with `▸ κ` tail, including silent steps) and
   `og-traces`; bisimilar ⇒ equal traces.
3. where the guarded form differs: `gtau (next p)` is not path-equal to `p` (constructor
   disjointness) though it has the same visible behaviour; the embedding `to-guarded` of a
   Church process and that tree bisimilarity implies guarded bisimilarity of the embeddings.
If the checker does not accept a type defined by guarded recursion, record the smallest input
under Found and state the relation by its finite unfoldings instead (`og-Bisim-n : Nat → …`
on `(κ : Clock) → GProc κ S A`, forcing with `forceκ` as `lib/choreo.cub`'s `cut` does).
