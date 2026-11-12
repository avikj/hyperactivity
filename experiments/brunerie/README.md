# Brunerie number experiment

`brunerie.cub` is an untyped port of Ljungström's `brunerie'` from agda/cubical
(`Cubical/Homotopy/Group/Pi4S3/BrunerieExperiments.agda`). The file's comment says it computes in about 1 second in Agda.
The pipeline is the same: `g10 (g9 (g8 λ i j → f7' (η₃ (push (loop i) (loop j)))))`.

The port includes the h-level proofs that the pipeline evaluates:
- `TL2` and `TL3`: isOfHLevelTypeOfHLevel 2 and 3, via the Σ≡Prop retract and the univalence retract (uaη, transpEquiv).
- `P4`: isOfHLevelΠ 4.
- `help`: the S²ToSetElim equivalence proof.

The two proofs that were placeholders are ported:
- isSetℤ (`g10`'s squash₂ case) is `isSetInt` in `lib/std.cub`: Hedberg's theorem (`Discrete→isSet`)
  over the decidable equality of ℤ, checked.
- surfc on `squash4` is `fill4OverSets` (isOfHLevel→isOfHLevelDep 4 for a family of sets), checked in
  `brunerie.cub`. The 3-cubes over the 4-cube's sections form a line of propositions, so the 3-cube
  over `t` and the one over `u` are joined; it uses `isPropPathP` and `isProp→isSet` from the library.

The data types `T2`, `T3` and `K2` check: a constructor field that is a path in the type itself
(of any dimension) is strictly positive.

Run from `scoped-machine/`:

    CUBICAL_NOCACHE=1 ./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub \
      experiments/brunerie/brunerie.cub experiments/brunerie/run-<name>.cub

| driver | what it shows | measured |
|---|---|---|
| `run-circle.cub` | g9 on `loop` and on `loop·loop` | `t2inc (pos 1)` in 1.7k steps, `t2inc (pos 2)` in 7.5k |
| `run-square.cub` | F, the η₃-on-push square through f7' | 45k steps; contains one `squash4` |
| `run-cube4.cub` | encodeTruncS2 along a genuine `squash4` 4-cube, which exercises TL3 | 155M steps, 365 s (an open 3-cube, read back in full) |
| `run-brunerie.cub` | the number | `negsuc (suc zero)`, -2, in 1.26M steps: 0.4 s, plus 1.1 s to load the cached library |
| `run-squash.cub` | g10 on a squash₂ square; surfc on a squash₄ cube at generic dimensions, read at a corner of Ω² | `pos 1` everywhere; the cube's point (201k steps) |

surfc on a squash₄ cube read in the interior of Ω² at generic dimensions (a 6-dimensional open term
over K₂) did not finish within 6.5 GB.

Measured after the Kan redesign (04f761c:
compositions of types as a type former, generalized composition) and with the glue base held
unbuilt; before these, on a 7 GB machine, the square and the number ran out of memory and the cube
did not finish.

(2026-11-12: not reproduced. A literal `0` is accepted as a path constructor's dimension argument and in path application, `lp 0` and `p @ 0` both checking and computing; a function over the interval is not a Π type here, so the case below cannot arise as written. Kept for the record.) The parser reads a literal `0` or `1` given as an interval argument (a constructor's, or an ordinary function's) as Nat `zero`. Bind the interval with `λ (a : I)` and apply it with `@` instead. `pE P = tE P 0` had this error: the face `i = 1` in `tE` met the numeral, and the number read as `?`.

`empty-hcomp-experiment.patch`, made against c5aab2d, makes `hcompD` with an empty system reduce to its base in every data type.
It is a stand-in for measuring only, and it is not coherent for higher inductive types: eliminating into the universe gives `Glue A []`, not `A`.

## The two causes

1. **Transport is `comp A [] a0`.** In a data type with path constructors, this leaves a formal `hcompD []` that never goes away.
   F carries 20 such layers around values that are really constant.
   `codeTruncS2` turns each layer into a composition in the universe of groupoids.
   Transport through n layers took 779, 2036, 6755 and 30385 steps for n = 1, 2, 3, 4.
   The fix is a transport primitive that is the identity on closed data and acts on constructors for data with parameters (`%transD`), with comp = hcomp ∘ transp.
2. **`%comp-u` encodes a composition of types as Glue with `lineequiv` equivalences.**
   Transporting through such a Glue (`%comp-glue`) needs `snd (snd S1) a1`, the fibre contraction of each equivalence on a true face.
   For a `lineequiv`, that contraction is itself a composition in the fibre types (Σ of a path) along the face line, one Glue level down.
   TL3 on the 4-cube nests about four such levels (retract hcomp, univalence retract, uaη, Hopf Glue), with up to 8 faces each.
   Agda instead treats a composition of types as its own type former, and transports through it using only transport along the faces.
3. **The glue base was built eagerly.** Composition through a Glue type or a composition of types ends
   with `glue tp base`, where `base` composes the unglued value with the fibre paths as its tube. On
   the end's faces the system is total and `base` is never read, but it was built, and the machine
   computes whatever is built: its tube's fibre paths composed generically one level down, whose
   glue bases did the same. Each nesting level paid for every level below it.

Causes 1 and 2 are fixed by commit 04f761c (generalized composition, which never leaves a formal
`hcompD` on a closed point; compositions of types as their own type former, transported through by
`%tproof`). Cause 3 is fixed by holding the glue base unbuilt.

## g8, measured against its result (A8)

`#show g8` (the loop in T3 at a generic dimension): 1,135,531 steps for a result of 2,346 cells,
484 steps per cell of result. A `#show` now prints the result's size beside the steps.

Steps by rule: substitution 31%, erasure 22%, application 13%, line abstraction (`plam`) 7%,
`hcomp` 5%, path application 7%, `%transD` 5%, duplication 3.5%.

Definitions entered: `%trans-hcomp-T3` 48,490 times and `%trans-t3inc` 45,708 times; everything
else under 6,000. Erasures by value kind: references 116K, thunks 30K, constructors 25K, faces 23K,
lines 20K, systems 13K, formal hcomps 10K.

The mechanism: the generated clause `%trans-hcomp-D` is `transp ψ (hcomp [u] u0) = hcomp [sysmap
(λ w. λ j. transD ψ (w @ j)) u] (transD ψ u0)`. An application reduces as soon as it exists (only
a subterm held unbuilt is lazy), so the transport is pushed into every side of the tube at once;
the result at an endpoint then selects one side of each layer and erases the rest, whose
transports were computed. Through 20 nested layers that multiplies, and the erased references
are those transports. Holding each side's transport unbuilt (`delay` around `%transD` in the
clause) is not the fix: the square driver is unchanged (45,428 steps) and g8 runs out of 3 GB,
the held sides accumulating substitutions through the layers and losing the sharing the eager
transports had. The structural fix is a change to the composition clauses (a proposal, not a
commit).

### The fix (plan 18.16): the transport of a formal composition is itself formal

A transport reaching a formal composition in a type with path constructors now makes a value,
`%transH` (`V_TRANSH`: the face ψ, the parameter lines, the tube and the cap, unexpanded), instead
of applying the clause. A substitution applies to its parts and re-decides the tube: while the tube
is still a system the value stays formal; a side the substitution selects is transported alone, at
its end (`transD ψσ (wσ @ 1)`); ψ made true makes it the composition again. A reading that demands
the structure (`%view`, a further transport, an elimination, the printer) distributes one layer:
the clause's right-hand side, whose inner transports are formal again. So at a generic dimension
a nest of twenty layers is twenty cells, and the endpoint substitution walks one side per layer.

`#show g8`: 1,135,531 → 475,558 steps, the same normal form (2,346 cells printed). The number:
1,263,260 → 619,852 steps. The square driver is unchanged (45,428 steps).

