# Track S — the elaborator accepts what the tracks had to work around

Read `README.md` here (rules), `../README.md` §1–§3 (surface language, clauses, structural
recursion, `decreasing by`), and in `src/syntax.c` the clause compiler (`cc_match`, `ccm_t`,
`clauses_typed`, the recursion check that prints "not on a constructor field") and
`x_checked`.

Files you own: `src/syntax.c` (shared with track A: pull before every edit, small commits),
`experiments/surface/**`, this file. Not `src/net.c` (track N), not `lib/*.cub` except to
remove a workaround after its cause is fixed (then say which, and keep `make library` passing).

## What is built (each found independently by tracks D and G; their track files have the inputs)

1. **Structural recursion when the matched argument is not the first and an earlier argument
   changes**: `g j (cons x r) = g (%wsucc j) r` is refused with "not on a constructor field";
   it checks when the list is first. Accept recursion on any one argument position (the
   function's other arguments may vary freely in the recursive call), and report the refusal
   as an ordinary failed check of that definition, not a fatal error ending the load.
2. **Structural recursion through a nested `case`** (rather than a nested pattern) on a field
   of the matched argument, and through a continuation rebound by a dependent
   `case … return`: accept when the recursive call's argument is a field (transitively) of
   the matched one.
3. **The scrutinee variable inside a branch**: using the matched variable in an `inr u` or
   `nil` branch fails with `no inferable type here`. In a branch the variable is the
   constructor applied to the branch's fields, at its type.
4. **Clauses at a type that only unfolds to a function type**: a definition by clauses whose
   declared type is a defined name (`World Linux St`) that unfolds to Π stops the load with
   `a dimension is matched only by a variable`. Unfold the declared type to weak head normal
   form to find the parameters.
5. **Every error in a definition is that definition's failure**: find every `x_err`/`die`
   reachable from checking one declaration that ends the whole load, and make it a `FAIL:`
   line for that declaration with the same message, the load continuing (a fatal stop remains
   only for unreadable syntax). List the ones changed.
6. **`lib/prelude.cub`'s unchecked names** (`append`, `map`, `length` …) used in a checked
   definition give `code: not a bound variable or checked definition`: either they are
   checked definitions (give them signatures in the prelude if they can be typed there) or
   the error says "prelude helper X has no type; use std's Y". Decide by what the prelude can
   state; record the decision.
7. A cached file's `#show` readings are not reprinted: print them from the cache (store the
   reading's text with the entry) so a load's output does not depend on the cache.

## Acceptance

- `experiments/surface/*.cub`: one small file per item, each loading with `N passed, 0 failed`
  (for item 5: a file with one wrong definition among right ones ends `k passed, 1 failed`).
- `make library` unchanged in count and 0 failures; `make brunerie` −2; the commands under
  "How to verify" in `A-compiler.md` unchanged.
- The workarounds in `lib/os/contract.cub`, `lib/isa/tso.cub` named in D's and G's Found
  sections replaced by the direct form, or listed here with why not.

## Needs from track A
## Done
## Found

## Next step

Reproduce item 1 from `tracks/G-memmodel.md`'s Found section in `experiments/surface/rec-pos.cub`;
find the check in `src/syntax.c`; fix; commit.
