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

11. (from track C, `tracks/C-clang.md` Found) **Nested positivity**: `List D`, `Opt D`, `D × D`, or
   any data type at a strictly positive parameter, as a field type of `data D`, is rejected as
   not strictly positive. Compute per data type which parameters are strictly positive; an
   occurrence of `D` inside an argument at such a parameter is strictly positive. Recursion
   through such a field (the nested form: a call on an element obtained by matching the
   container): accept, and say exactly which forms are accepted. Positivity must stay sound.
12. (from track C) **Mutual `data`** (`mutual` over data types referring to each other), positivity
   checked across the group, and mutual structural recursion over them (`mutual` over functions
   is codata-only today).
13. (from track C) **Keywords as field names** (`then`, `else`, `record` as `data` field names are
   syntax errors): allow any identifier as a field name, or report the clash as that
   declaration's failure naming the keyword.
14. (from track C) **A byte-string literal** (`b"abc" : Bytes 3`, or a `bytes-of` form on a string
   token), so no file needs a primitive of its own to build text (C's ast.cub declares
   `primitive cstr-put`, a line in `#trusted`); then C's primitive replaced by a checked
   definition (through "Needs from track S" in C's file). After 11–12: a "From track S" line in
   `tracks/C-clang.md`.

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
- **Item 5** (`experiments/surface/fail-one.cub`: `159 passed, 1 failed`;
  `experiments/surface/fail-local.cub`: five FAIL lines, `160 passed, 5 failed`, the definitions
  after each still checked). `p_decl` (src/syntax.c) handles every declaration under a `die_jb`
  of its own (`p_decl_in` is the old body). An error raised while a declaration is handled is
  printed as `FAIL: checks NAME` / `reason: <the same message>` / `in: file:line`, counted in
  `failed` (`ok: rejected NAME` under `#fail`), and the load goes on at the next declaration of
  the block (the tokens skipped to the next item at the declaration's depth). What ends the
  load now: text that cannot be read, which is the lexer's and layout's `x_die` and every
  `x_err` (the parser's "expected …, found", "too many …", "unexpected"). What changed from
  fatal to that declaration's failure:
  - every `die(…)` of src/syntax.c and src/lang.c reached while a surface declaration is
    handled (59 sites in syntax.c, 123 in lang.c at this commit: all of them go through `die`,
    which now finds the declaration's `die_jb`): the clause compiler's (`not on a constructor
    field`, `a dimension is matched only by a variable`, `a clause with fewer patterns than
    parameters`, `the clauses do not cover every case`, `clauses with different numbers of
    patterns`, `not a pattern`, the guarded and `decreasing by` ones), the elaborator's outside
    `x_checked`'s own guard, and lang.c's on the forms a declaration becomes (`code: not a bound
    variable or checked definition`, `declared again (a name is bound once)`, `a checked
    definition is declared again`, `cannot open` of an import, …);
  - the 40 `x_err` calls that are not about unreadable text, now `x_sem`: `#build`/`#build-loop`/
    `#jit` (needs a library loaded, not a checked definition, the plan did not read back, the
    file cannot be written, not a checked theorem), `deriving` (5), `foreign:` (5) and "a
    foreign function's type has no implicit arguments", `open:` (2), `world:` (2), "not a
    module", "a module inside a module", "a module's definitions have signatures", "a
    copattern definition inside a module", "a signature, before the checker is loaded",
    "decreasing by needs a signature", "a primitive's type gives every argument", "a tick
    primitive needs its computation", "a clause of another definition after a signature", "a
    signature without clauses in a mutual block", "not a field of the record", "a field given
    twice".
  Not changed: a file of core forms (not `.cub`) read by lang.c's own loader, and the command
  line's `cannot open`: no surface declaration is being handled there.
- **Item 4** (`experiments/surface/clause-unfold.cub`: 169 passed, 0 failed uncached).
  `tele_more`/`sig_unfold` (src/syntax.c): when a clause matches by a constructor or a numeral
  past the signature's parameters, the result type's head is unfolded (the definition's surface
  body, kept by `x_checked` in `BODS`; its λs replaced by the arguments, an implicit one not
  given a hole) while it is a defined name, and the Πs it unfolds to become parameters (their
  binders given fresh names, so no clause variable or argument is captured). The definition is
  still checked at the signature as written; the unfolding only tells the clause compiler what
  to split. Patterns that are all variables are left as before (the result's own λs: `isPropNeg
  f g = …` in lib/std.cub is one).

Measured: `make library` 1127 passed, 0 failed, 24715925 steps before and after item 1; after
items 4 and 5 at main `a2f2c04d`+ (more library files): 1398 passed, 0 failed; `make brunerie` −2 with 0 and 4 workers; A's commands: 33 jit, three
`ok: build` lines, `A` then 65, 5 then 2.

## Found

- `make brunerie` under `ulimit -v 3000000` hangs in its 4-worker run: the process sleeps in a
  futex at 1.8 MB resident with 3 threads and prints nothing (0 workers under the same limit:
  −2 in 17 s; 4 workers without the limit: −2 in 22 s). Track N's (`src/net.c`): a failed
  allocation at worker start leaves the main thread waiting. Reproduce:
  `(ulimit -v 3000000; CUBICAL_NOCACHE=1 CUBICAL_WORKERS=4 ./build/cubical <the 8 core files> experiments/brunerie/brunerie.cub experiments/brunerie/run-brunerie.cub)`.
- A definition of a name already bound prints `ok: checks f` and then `FAIL: checks f` /
  `declared again (a name is bound once)` (lang.c checks before it refuses the binding): counted
  once as passed and once as failed. The earlier binding stays.
- `clauses_typed` saved `row_used` after its `setjmp`, so a retry (course-of-values) restored the
  retry's own table, not the enclosing clause's: fixed with item 1 (saved before the `setjmp`).

## Next step

Item 3 (the scrutinee variable in a branch: `no inferable type here`). Reproduce in
`experiments/surface/scrut.cub` with `case b of inl x → …; inr u → f b` at `b : Sum A Unit` and
`case evs of cons x xs → …; nil → (k , evs)`; find where a `case` branch replaces the scrutinee
variable by the constructor form (src/syntax.c, the elaboration of `case`, search `cfree` in
lib/check.cub for where the error is raised) and annotate the replacement with the variable's
type (`the T (c fields)`). Then items 2, 6, 7, 8, 13, 14, 11, 12, 9, 10 in that order. After each:
`make build/cubical && make library` (0 failed), `make brunerie` (run the 4-worker half without
`ulimit -v`, see Found), `/tmp/S-verify.sh`-style run of A's commands.
