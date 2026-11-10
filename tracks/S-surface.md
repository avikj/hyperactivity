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

8. (from track H, `tracks/H-obs.md` Found) **`#type D` for a data type `D`** ends the run with
   `not a bound variable or checked definition`: print the type former's type; a command that
   cannot be answered is that command's failure, not a fatal stop.
9. (from track H) **Guarded recursion**: a self-call inside a type under a tick binder, and in a
   proof of a later relation, is refused; the functional written out with `fix` by hand is
   accepted. Accept the self-call where the hand-written `fix` is (the same functional).
10. (from track H) **A guarded definition's functional is a checked definition**: `gbind`, `gmapp`,
   `ghandle`, `glift`, `grun` (lib/proc.cub) elaborate to `fix` of an unnamed functional, so
   `gmapp f (gtau t) ≡ gtau (λ α → gmapp f (t α))` fails by conversion. Bind the functional of a
   guarded definition `f` as `f·F` with `f = fix f·F`, so unfolding lemmas by `pfix` can be
   stated; then write a "From track S" line in `tracks/H-obs.md` (that one edit is allowed).

## Acceptance

- `experiments/surface/*.cub`: one small file per item, each loading with `N passed, 0 failed`
  (for item 5: a file with one wrong definition among right ones ends `k passed, 1 failed`).
- `make library` unchanged in count and 0 failures; `make brunerie` −2; the commands under
  "How to verify" in `A-compiler.md` unchanged.
- The workarounds in `lib/os/contract.cub`, `lib/isa/tso.cub` named in D's and G's Found
  sections replaced by the direct form, or listed here with why not.

## Needs from track A

Nothing.

## Done

- **Item 1** (`experiments/surface/rec-pos.cub`: 166 passed, 0 failed after the eight core files).
  `clauses_typed` (src/syntax.c): when no recursive call is on a field with the parameters in the
  order written (and course-of-values fails too), the clauses are compiled again with one later
  parameter `p` moved before every earlier parameter its type does not mention (`perm_next`,
  `perm_dest`, `tsig.ord`; each candidate `p` that some clause matches by a constructor is tried
  in turn, plain then course-of-values). The split of `p` is then outermost and the parameters
  moved behind it are Πs of its motive, exactly as later parameters already were, so the
  hypothesis `rec r` is applied to their new values:
  `g1-a j (cons x r) = g1-a (%wsucc j) r` becomes
  `(elim (λ l → Π j Word Word) l (nil → λ j → j) (cons x r → λ j → (rec r) (%wsucc j))) j`.
  The definition's own λs stay in the order written. Why it terminates: the term is one `elim`
  of the data type with `rec` on a recursive field, which the checker checks as it does every
  other clause definition; nothing is trusted. `rp-bad` (a call on the whole argument) is still
  refused.
- **Item 5, the mechanism** (`p_decl`, `p_decl_in`, `x_sem`, `decl_reset` in src/syntax.c): every
  declaration is handled under a `die_jb` of its own. An error raised while it is handled is
  printed as `FAIL: checks NAME` / `reason:` / `in: file:line` and counted in `failed` (`ok:
  rejected NAME` under `#fail`), and the load goes on at the next declaration of the block.
  Errors of text that cannot be read (the lexer's, the layout's, the parser's `x_err`) end the
  load as before. So every `die` in src/syntax.c and src/lang.c reached from a declaration (the
  clause compiler's "not on a constructor field", "a dimension is matched only by a variable",
  lang.c's "code: not a bound variable or checked definition", …) is now that declaration's
  failure. Not yet done: the `x_err` sites that are not syntax (see Next step).

Measured at this commit: `make library` 1127 passed, 0 failed, 24715925 steps (the same step
count as before the change); `make brunerie` −2 with 0 and 4 workers; A's commands: 33 jit, three
`ok: build` lines, `A` then 65, 5 then 2.

## Found

- `make brunerie` under `ulimit -v 3000000` hangs in its 4-worker run: the process sleeps in a
  futex at 1.8 MB resident with 3 threads and prints nothing (0 workers under the same limit:
  −2 in 17 s; 4 workers without the limit: −2 in 22 s). Track N's (`src/net.c`): a failed
  allocation at worker start leaves the main thread waiting. Reproduce:
  `(ulimit -v 3000000; CUBICAL_NOCACHE=1 CUBICAL_WORKERS=4 ./build/cubical <the 8 core files> experiments/brunerie/brunerie.cub experiments/brunerie/run-brunerie.cub)`.
- `clauses_typed` saved `row_used` after its `setjmp`, so a retry (course-of-values) restored the
  retry's own table, not the enclosing clause's: fixed with item 1 (saved before the `setjmp`).

## Next step

Item 5, rest: in src/syntax.c change the `x_err` calls whose message is not about unreadable
text to `x_sem` (the `#build`/`#jit`/`deriving`/`foreign:`/`open:`/`world:` ones, "not a module",
"a module inside a module", "a module's definitions have signatures", "a signature, before the
checker is loaded", "decreasing by needs a signature", "a primitive's type gives every argument",
"a clause of another definition after a signature", "not a field of the record", "a field given
twice"); write `experiments/surface/fail-local.cub` (one wrong definition among right ones, ends
`k passed, 1 failed`); then items 4, 3, 2, 6, 7, 8, 9, 10 in that order.
