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

**The form of the proofs** (from the definitions; supersedes the specializer form recorded
earlier, which cannot work):

- Decode branches on the fetched bytes, so a fetched byte that is not a literal makes
  normalization quote every opcode's branch. At a code address that is a variable, every fetch
  passes points the constants do not decide: `add_to_iptr`'s check of the new rip,
  `read_iptr`, and in `rb_` per byte the canonical check of `signed(addr) + offset`, the
  sign-mask chain, and the lookup. The specializer rewrites only after normalizing, and a held
  memory function's result still feeds decode, so neither normalization nor the specializer
  can prove a template at a variable placement. Nothing in the language or the core decides
  a range test on a variable from a hypothesis.
- What works with neither a core change nor a fixed address: the checker's conversion is lazy
  (it unfolds only what a comparison needs), so a proof is the model's own sequence of binds,
  each step by `bind-ok` (`lib/compile/tmpl.cub`) with the equation of the step's action; the
  only actions not decided by conversion are the address checks, and each is one of three
  laws proved once: `iptr-ok` (`add_to_iptr`, `read_iptr` under rip's range hypothesis),
  `rb-ok` and `wb-ok` (the byte loops under the access's range hypothesis, by induction on the
  loop, giving `mem-load-n`/`mem-store-n` at the plain address). Between them conversion
  computes on the literal bytes. An instruction lemma is then one `bind-ok` per fetch and per
  data access, the continuations as the model writes them.
- The frame's hypotheses are range facts on each region's base (user address, with the
  region's length as margin) and separation of regions; same-region address equality is the
  torsor law in `wordlaw.cub`.

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

`iptr-ok`, `rb-ok`, `wb-ok` in `lib/compile/tmpl.cub` (statements from `add_to_iptr`,
`read_iptr`, `rb_`, `wb` in `lib/isa/x86/`), then the instruction lemma of `i-ld-rbp d` at a
variable rip, rbp and memory as the first use; then the rest per the pieces.

## Found

- `lib/compile/tmpl.cub` has the frame-state approach with the code at a literal address; it is
  the start of piece 2, moved to a variable address.

- **Measured 2026-11-11: the model's step at a variable placement does not normalize.** One
  instruction (`mov rax, [rbp+16]`, 7 bytes at `p`, `rbp` and memory variables) runs out of
  memory in `normalize`. Cause, measured on the primitives: comparisons between addresses of
  one region stay stuck though the constants determine them, and each stuck comparison in the
  store chain splits the residual in two:
  - `weq ((p+1) mod 2⁶⁴) p` stays `%weq (%wand (%wsucc p) M64) p` (false for every p);
  - `weq ((p+1) mod 2⁶⁴) ((p+2) mod 2⁶⁴)` stays stuck (false for every p);
  - `weq t t` for a compound `t` stays stuck (only a bare variable compared with itself folds);
  - `((p+1) mod 2⁶⁴ + 2) mod 2⁶⁴` is not `(p+3) mod 2⁶⁴`.
  Each is determined by the constants alone. What decides them: a normal form `(x + c) mod 2ⁿ`
  for the word primitives' sums and masks over one variable, equality of two such forms with
  the same `x` decided by `c₁ ≡ c₂ (mod 2ⁿ)`, and `weq t t` true for one term. The canonical
  checks and cross-region comparisons are not determined by the constants and remain the
  frame's hypotheses (rules with premises).

- **Read 2026-11-11: the model's memory is reached a byte at a time.** `rb_`/`wb`
  (`lib/isa/x86/application_level_memory.cub`) loop over the bytes; each byte: the canonical
  check of `signed(addr) + offset` as an integer, the address through `sail_mask_signed`
  64→48, the offset added at 48 bits, sign-extended to 52, `memi`, then `x86-read-mem #1`
  (`logging.cub`) at the 64-bit mask. So a word in memory is always 8 byte writes and 8 byte
  reads; no representation of `ByteMap` sees whole words. At a symbolic address the signed
  reading (`bv-neg?`), the range test and the mask chain are decided only by the hypothesis
  that the address is a user address; within a region, address equality by the torsor law.
  Consequence for the form: during specialization `rb_` and `wb` are held, and their meaning
  under the frame's hypotheses is two laws proved once by induction on the byte loop; the rest
  of an instruction normalizes on the literal bytes.
