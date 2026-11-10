# Track S — the elaborator accepts what the tracks had to work around

Read `README.md` here (rules), `../LANGUAGE.md` §1–§3 (surface language, clauses, structural
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

15. (from track G, `tracks/G-memmodel.md` Found) **A lemma whose type contains `isTrue (case l of …)`
   applied at a non-variable list** fails with "an implicit argument is not determined", with
   every implicit written out; the same proof with the case inlined checks (input: `g7-nn`,
   `g7-d` there).

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
- **Item 5** (`experiments/surface/fail-one.cub`: `164 passed, 1 failed`;
  `experiments/surface/fail-local.cub`: five FAIL lines, `166 passed, 5 failed`, the definitions
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

- **Item 3** (`experiments/surface/scrut.cub`: 171 passed, 0 failed). The cause: lang.c's `ds`
  rebuilds the scrutinee variable in a branch as `let b = (C x…)`, the checker infers a let's
  value, and a constructor of a data type with parameters infers nothing. `el_alts`
  (src/syntax.c) now writes the rebuilt value itself, at the variable's type as the scope has
  it: `(let b (the (Sum Nat Unit) (%rebuilt (inr u))) body)`, for a variable scrutinee of a
  parameterized data type whose branch mentions it; `ds` (src/lang.c, 4 lines) leaves a branch
  that starts with that let alone. The elaboration of `let` annotates a value that infers no
  type when its parts' signatures give one: a pair of two typed parts at their Σ, a constructor
  of a parameterized type at `syn_type`'s answer. Every such form failed before (`no inferable
  type here`), so nothing that checked is elaborated differently. Not covered: a `let` whose
  value is a call with an implicit type argument nothing determines (D's third form) — the
  hole is the checker's to solve and it has no type to solve it from.
- **Item 2** (`experiments/surface/rec-case.cub`: 173 passed, 0 failed). Two changes, both used
  only after the recursion was refused as written (and after course-of-values and item 1's
  reordering were tried), so no definition that compiled before compiles differently:
  - `lift_clauses` (src/syntax.c): a `case` (or `case … return`) in a clause's right side whose
    scrutinee is one of the clause's own pattern variables, not rebound on the way, every
    constructor of its type covered by an alternative or the default, no path constructor, no
    index: the clause becomes one clause per constructor with the variable's pattern refined
    (`f (cons e r) = C[case r of nil → a; cons d r2 → b]` is `f (cons e nil) = C[a]`,
    `f (cons e (cons d r2)) = C[b]`), the variable replaced by the constructor form elsewhere
    in `C`; a selected alternative that is a λ applied to a variable is its body at the
    variable. The clauses that result go through the clause compiler as written ones do: the
    call is accepted only as a field's or a field's field's (course-of-values, `Below`).
  - sibling revert in `ccm_t` (`srv`, `ih_type`, `rec_of_field`): a split of a field `v` now
    reverts the fields in scope whose types mention `v`, and with each recursive one its
    hypothesis, as Πs of the split's motive: `rc-run (rc-cmd i kk) = (case i return … of
    rc-ask → λ k2 → rc-run (k2 5) …) kk` becomes
    `(elim (λ i → Π kk (rc-resp i → RcStep) → Π ih ((a : rc-resp i) → Nat) → Nat) i (rc-ask → λ kk ih → ih 5) …) kk (rec kk)`.
    The hypothesis's type is built only through Π codomains to the data type (the frame's
    motive at the field applied); another shape (a path, a Σ) leaves the split as it was.
  Forms accepted, exactly: a call on a pattern's field; on a field's field at any depth reached
  by nested patterns or by such cases; on a function-typed field applied (`f (k x)`), the field
  possibly rebound through a dependent case on an earlier field. Each ends as `elim` with `rec`
  on a field (or `Below`), checked by the checker. `rc-bad` (a call on a rebuilt larger list)
  is refused. G's `g5-f` is `rc-run` with G's types.
- **Item 6** (`experiments/surface/prelude-names.cub`: 165 passed, 2 failed, the two that use
  `append` and `length`/`map`). Decision: the prelude cannot state their types — it is loaded
  before the checker (`a signature, before the checker is loaded`), and the checker's own files
  use them untyped. So (a) the error (`die_unchecked`, src/lang.c) now reads `code: append is a
  helper defined before the checker was loaded (lib/prelude.cub and the checker's files): it
  has no type, so a checked definition cannot use it. Checked ones in lib/std.cub: list-map,
  list-length, list-append (for map, length, append), plus, times (for add, mul)`, and is that
  definition's failure (item 5); (b) `lib/std.cub` gained `list-map`, `list-length`,
  `list-append` (12 lines at its end; a shared file, the edit the spec's "use std's Y" needs:
  std had no list functions). `make library` counts 3 more for them.
- **Item 7** (`experiments/surface/show-cache.cub`: loaded twice with one `CUBICAL_CACHE`, the
  second load prints `cached:` and the same three readings). `x_top` (src/syntax.c) holds what
  a `#show`, `#observe`, `#type` or `#usage` prints while its module is being recorded and
  writes it after the form as `(shown x<hex>…)`; a cached module prints it from there.
  The `passed` count of a cached load is still smaller than an uncached one's (a cached module's
  `#same` and checks are not run: by design of the cache, not changed).
- **Item 8** (`experiments/surface/type-data.cub`: `type: Td1 : Type → Type`, `Td2 : Type`,
  `Td3 Nat : Type → Type`, `List : Type → Type`; `#type no-such-name` is one FAIL and the load
  goes on: 165 passed, 1 failed). `#type` of a data type's name applied to fewer arguments than
  its parameters asks the checker about `the ((ps…) → Type l) (λ ps → D ps)` for l = 0, 1, 2, 3
  and prints the first it accepts (`(type term shown-as)`, src/lang.c 2 lines). A type former
  whose universe depends on a level parameter is not found this way (the command fails).

Measured at this commit (main at `98692012`+): `make library` 1482 passed, 0 failed (3 of them
`list-map`, `list-length`, `list-append`); brunerie −2 with 0 and 4 workers; the surface files as
stated above (uncached, after the eight core files). A's `#jit`/`#build` commands: 33, three ok
lines, `A` 65, `5 2` as of item 1's commit; being re-run for this one (they take 10 minutes on
the loaded machine) — the next commit states the result.

## Found

- `make brunerie` under `ulimit -v 3000000` hangs in its 4-worker run: the process sleeps in a
  futex at 1.8 MB resident with 3 threads and prints nothing (0 workers under the same limit:
  −2 in 17 s; 4 workers without the limit: −2 in 22 s). Track N's (`src/net.c`): a failed
  allocation at worker start leaves the main thread waiting. Reproduce:
  `(ulimit -v 3000000; CUBICAL_NOCACHE=1 CUBICAL_WORKERS=4 ./build/cubical <the 8 core files> experiments/brunerie/brunerie.cub experiments/brunerie/run-brunerie.cub)`.
- A definition of a name already bound prints `ok: checks f` and then `FAIL: checks f` /
  `declared again (a name is bound once)` (lang.c checks before it refuses the binding): counted
  once as passed and once as failed. The earlier binding stays.
- The module cache's reading capture must give stdout back when the reading fails: `x_cap_end`
  is called from `decl_reset` (found when `#type no-such-name` made the rest of the output vanish).
- `clauses_typed` saved `row_used` after its `setjmp`, so a retry (course-of-values) restored the
  retry's own table, not the enclosing clause's: fixed with item 1 (saved before the `setjmp`).

## Next step

**Halted by the coordinator (2026-11-09): no implementation until the mathematical specification
of what the elaborator accepts (recursion forms, positivity) is written and reviewed.** State:

- Pushed and measured (`make library` 1482 passed, 0 failed; brunerie −2; the files in
  `experiments/surface/`): items 1, 2, 3, 4, 5, 6, 7, 8. Nothing of items 9–15 is in `src/`.
- **Not measured for the last commit (`48b48ce8`): A's `#jit`/`#build` commands** ("How to
  verify" in `tracks/A-compiler.md`). They were 33 / three ok lines / `A` 65 / `5 2` at item 1's
  commit (`0aa36444`); the re-run was stopped by the halt before its first line. First thing to
  do on resuming: run them.
- What needs the specification before it is relied on (each is in Done with the term it
  produces): item 1's reordering (`perm_next`), item 2's clause split of a case
  (`lift_clauses`) and sibling revert (`ih_type`), item 4's unfolding (`sig_unfold`). Each only
  changes which `elim` term is handed to the checker; none is trusted.
- Started and reverted (not in the repository): item 13, allowing a reserved word as a data
  field's name in `p_binders` under a flag set by `p_data_ctor`; `(then : S)` and `(else : S)`
  parsed, `(case of : Nat)` did not (`unexpected of`: the check for two names in one binder
  looks one token ahead only).
- Not started: the workarounds in `lib/os/contract.cub`, `lib/os/replay.cub`, `lib/isa/tso.cub`
  (D's and G's files; the direct forms are the inputs in `experiments/surface/` and check),
  items 9, 10, 11, 12, 14, 15, and the "From track S" lines in H's and C's track files.
