# Track E — the encoder at its type (Phase 2 item 5; second half of D2)

Read `README.md` here first (rules, the forms the earlier failure took), then `../PLAN.md` §0,
§4, §6 Phase 2 item 5, then `lib/x86.cub`'s header (the value forms and the registers).

Files: `lib/compile/` (the template types, the combinators, the laws they use), `lib/x86.cub`
(rewritten at its type, with track A), `experiments/isa/`, this file.

## Specification

**The statement** (PLAN Phase 2 item 5, forced by the encoder's conventions in `lib/x86.cub`):

    Tmpl m = Σ bytes . for every state in the frame with rip at the bytes' first byte,
             the model's run reaches the byte after the last with m in rax and the frame kept
             (up to the flags and the dead stack), or takes the overflow exit (PLAN §4.2)

- *The state in the frame*, from the registers' roles in `lib/x86.cub`: every variable of the
  environment at its place (argument i of n at `[rbp + 16 + 8(n−1−i)]`, slot s at
  `[rbp − 8(s+1)]`) holding its value's form; `r15`/`r12` the arena's next and end; `r13` the
  context (word 0 the flag); `r14` the blob's base with the blob's bytes there; `rbx` the
  stack at entry; the stack below `rsp` free down to the context's limit. Every address is a
  variable: the code runs where `mmap` put it, so a statement about the code that runs holds at
  every placement. The model's address checks are the frame's hypotheses (canonical,
  disjoint), stated as the model's own checks.
- *m in rax*: rax holds m's value form by its type (the forms of `lib/x86.cub`'s header: a word
  by its bits, a fieldless constructor by its index, a constructor with fields the address of
  an arena cell whose fields hold the fields' forms, a pair, bytes, a Nat by its count). A
  cell's contents are in the arena between the old and new `r15`.
- *The run reaches*: a finite run of the model (the step count is the witness, its length
  read off the template), stopping when rip is the end.
- *The overflow exit*: the flag set and the call's return taken, as §4.2 states; then the net
  computes the call (`run p f ≡ f`).

**The pieces, each written at its type, never bytes first:**

1. `Frame`, `Tmpl`, and the run-composition law (a run to the end of `a` then from there to the
   end of `b` is a run over `a ++ b`), from `lib/compile/x86ok.cub`'s split lemma.
2. The straight-line templates: each construct's bytes in `lib/x86.cub` with its `Tmpl`
   inhabitant, by the model running on the literal bytes (conversion), the frame's hypotheses
   selecting the model's address checks, the laws of `lib/compile/wordlaw.cub`,
   `bitlaw.cub`, `flags.cub` where the model's byte memory and flags need them.
3. The combinators: sequence (`let` and argument evaluation: the value to a slot, the next
   template in the extended frame), case (the tag test and `jcc`, each branch a template), the
   counted loop of the word eliminator (induction on the count), call (arguments pushed, the
   callee's template in its own frame, the frame restored), arena allocation (a cell).
4. `cx : (c : Code) → Tmpl (eval-g c)`, by recursion on the code; `x-push`, `x-lift`, erasure,
   η-contraction returning their paths with their results; the blob and the entry stub at
   theirs. The untyped `lib/x86.cub` is replaced, not annotated.

**Acceptance** (PLAN Phase 2): every workload's bytes, run under the model from the loader's
state, give the machine's answer as a checked equation; `#trusted` lists the ISA model and no
templates.

**Deleted by this track** (they contradict the specification): `lib/compile/meaning.cub`'s
step-bounded `⟦bs⟧` and `Correct*` at one fixed layout, `lib/compile/runs.cub`'s whole-run
combinators, `experiments/isa/addw-all.cub` (a lemma about one output of the untyped encoder),
`lib/compile/process.cub`'s kernel (`kn-step`, a second kernel beside `lib/os/contract.cub`'s
`linux-model`; processes run over that world).

## Done

- The laws the templates use: the word ring and order laws, the byte digits of a word and its
  read-back through the model's 8-byte load (`wordlaw.cub`), bit laws (`bitlaw.cub`), the carry
  flag of `write_user_rflags` as the model's `fits` (`flags.cub`).

## Next step

Piece 1 and the first template of piece 2, at a variable frame: `i-ld-rbp d` (load a slot) and
the `add` template, with `rbp`, `r14`, the code's address and the memory variables. Measure
steps; the form of the frame's memory is the one under which the model's reads at `rbp + d`
compute. Record what was measured here.

## Found

- `lib/compile/tmpl.cub` has the frame-state approach with the code at a literal address; it is
  the start of piece 2, moved to a variable address.
