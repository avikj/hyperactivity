# The system: direction, scope and build plan

Revision 7.9, 2026-11-06. Repo avikj/math, `scoped-machine` at `a774e2ee` (revision 7.8 was at `dd301c8`; the day's commits are listed in section 12's queue paragraph; the specs of section 18 were written against `9b94b43` and their status paragraphs say what was built). This document lives at `scoped-machine/PLAN.md` from this revision on; it is the single plan and status document — a fresh agent starts here.
Each revision replaces the last. Section 17 records changes and retractions.

---

## Start here: catching up before touching anything

This section exists because an agent that reads the queue first and the rest never will make
confident wrong decisions. The owner spent many sessions building the context below; the
queue in section 12 is the last thing to read, not the first. Budget: reading this document
end to end is ~60k tokens; the library files named in the reading order are ~40k more. Do
that before writing a line. There is no deadline that justifies skipping it; a wrong
decision made quickly costs more than the reading.

### What this is

Homotopy type theory, in its cubical form (CCHM: De Morgan connections, generalized
composition, Glue, univalence as a theorem that computes), with an interaction net as its
runtime, its checker a term on that same runtime, and a foundational claim: computation has
one correct form — substitution along identities, which here are paths — and every existing
language is an approximation to it made without equality as a primitive. Univalence is not
a feature of the theory used somewhere; it is the mechanism by which everything in this
document is derived. A representation of a type by a layout is an equivalence, hence by
`ua` a path in the universe, and *compiling* is transporting a program along that path;
converting between two layouts is transport along the composite; the proof that an
optimization is safe is a path between two programs, and rewriting along a law is
substitution along it; a replay is a path between two runs; a higher inductive type is data
with path constructors, and a quotient's canonical form is its decoding equivalence into
path-free data; the store names a term by its elaborated content, so two definitions that
are the same term are one key, and the identities the store holds are the view navigation
moves in. The net computes `comp`, `hcomp`, `transp` and Glue on open terms — that is what
"normalizing a term with open dimensions" in section 0 refers to, and the A8 measure is the
cost of exactly that. The project is not "a dependently typed language with a
compiler"; it is the position that compilation, memory layout, concurrency, build systems,
versioning and contracts are all *derived* here from the one theory (section 16's ledger of
~90 frontier-language features says, row by row, what each is a consequence of). Sections
0–11 are the content; read them as a mathematician reads definitions, not as marketing. The
owner regards all of it as research scope and all of it as current scope: nothing
implementable is deferred to "later".

### What makes it the language, concretely

Each of these is a property the system has now, with where it lives; together they are why
the owner holds this to be the form every other language approximates.

- **The machine runs itself.** The composition clauses — how transport and composition
  compute at each type former — are terms (`lib/prelude.cub`), not C. The checker is a term
  (`lib/check.cub`) running on the same net, reading values through `%view`, quoting them to
  codes and evaluating codes back; it checks the standard library, the libraries that
  extend it, and the programs that extend it in turn. There is no separate compiler, type
  checker or build system written in another language to trust: `src/net.c` holds the
  rewrite rules and nothing semantic beyond them. `#trusted` and `#safe f` list exactly what
  is assumed, and for most definitions the answer is nothing.
- **Programs about programs are ordinary programs.** The specializer, the first-order
  lowering, the C printers, the matcher and navigator, the derivation of decidable
  equality from the data table, the three verified decision procedures: every one is a
  definition in the language over codes, with readings in the same test suite as any other
  definition. A proof by reflection (`ring-eq`) is one evaluation of the normalizer; the
  proof term is a constant path. Nothing in the pipeline is a pass of the implementation
  that the language cannot see, quote, prove things about, or replace.
- **One runtime, one notion of time.** There is no compile time versus run time; there is
  the binding time of each value (now, a consumer's build, a known run-time moment, never),
  and one back end substitutes what is known when it is known. A schedule — what to inline,
  fuse, specialize, to what generality — is a value the specializer takes, not a pragma or
  a flag.
- **Equality is primitive and computes.** Paths, transport, composition, Glue and
  univalence reduce on open terms. A representation of a type by a layout is an equivalence
  and so a path; the layout is computed from it; compiling is transport along it. The laws of
  an operation are paths, consumed by rewriting; what cannot be a path in the target (C,
  C++) is erased or becomes an `[[assume]]`.
- **Locality, so parallelism is free.** A rewrite step touches two cells at their principal
  ports and consults nothing else; reading a held value locks nothing (wires are
  variables). Workers cannot change a result; the test suite compares the single-threaded
  run with 8 and 16 workers on every suite.
- **Sharing by content, retention by reachability.** A producer is named by its body over
  the keys of what it binds, so one term is one pointer and equal work reached by two routes
  is done once while the first is retained; a shared value lives exactly while something
  refers to it, by counted references, with no collector, no retention policy and no
  reclamation protocol.
- **The store is the codebase.** A checked definition's key is the hash of its elaborated
  form with names outside; renaming changes no key, a changed proof changes its key and its
  dependents'; a definition the current checker has accepted is never checked twice. Names,
  files, versions and branches are views over keys, not primitives.
- **Erasure is computed, not declared.** Multiplicities are read off a definition after it
  checks; a proof passed to an access that ignores it is erased because its usage is zero,
  not because someone wrote an annotation; the C that results carries the fact as an
  `ASSUME` for the C compiler and nothing else.
- **The boundary with the world is a type.** A process is a tree of commands over a
  signature; a world is a handler; what the world may refuse is in the outcome type; a run
  is reproduced from the responses alone (replay); supervision is a function on outcomes.
  Foreign code, raw addresses and syscalls are trusted primitives that announce themselves.
- **The hot path is the C an engineer writes.** Words, pairs and flat records are unboxed
  by inference; loops have no allocation; a two-load decoder prints as
  `uint64_t f(V *b) { return ld64(b, 16) + ld64(b, 0); }`. The same term also runs on the
  net with open variables, where it is a proof.
- **Nothing is a hole in meaning.** Every feature in section 16's ledger of frontier
  languages is either a consequence of the theory, a library with no new judgment form, or
  declared not worth having — and a row with no answer is a gap to close, not a limitation
  to document.

### The conversations this document came out of

The sections below are the outcomes; this is the reasoning that produced them, in the order
it was built up, so that a reader starts from the owner's premises rather than from the
usual folklore about languages and proof assistants. Two Claude sessions worked the repo:
one changing the machine (commits tagged `session_019CdfGcL8YbfbiCH37R9UoY`), one writing
library code and this document (`session_01WfrbRsdQ3rXCwxPvyw3a7e`); the owner relayed
between them. That is history, not a structure to preserve.

**The owner's premise.** Computation has a correct form — substitution along identities —
and the machine here is that form made literal. The question put repeatedly was never "is
this a good language" but "what follows": what the hot path becomes, what a C++ codebase
looks like from here, what every feature of every frontier language *is* once equality is
primitive. Answers were expected at full strength and from these premises; answers built
from what is common in languages ("here is how other systems do it") were called losing the
plot. The stated goal is to supersede the features of every frontier language — C++ first
(every feature added or proposed in recent years was gone through), then Rust, Haskell,
OCaml, Lean, Agda, Erlang, Zig, Mojo, Julia, and the rest — and the ledger in section 16 is
the measure of that goal, not a survey. All of it is research scope and all of it is
current scope.

**What compilation is here (section 5).** The owner's starting objection: "I can define a
native function f(x) in any language — what does *first-order* add?" The answer that held:
a compiled function is a precompiled reading of a *class* of terms, and the class is the one
whose answers are plain data — no dimensions in the term, the types fixed at the boundary,
data at the boundary path-free, no open Kan operation, no reflection on live values. The
owner's own phrasing of the key: the line between ordinary inductive types and higher
inductive types is the line between what has a native form and what is read by the net.
Binding time is then one axis with four points (now; C++ build; run-known, i.e. a JIT;
run) and a single back end: what is known at each stage is substituted at that stage. The
checker's own decision reflects on values, so it is never first-order; it runs on the net.
What falls outside the first-order fragment (open dimensions, higher inductive types,
Kan operations on open terms, reflection) is *not* a limitation and must not be written up
as one — the owner rejected a draft that listed four such "limits": they are the machine's
own domain, read by the net at work proportional to the result, and the fragment is just
the part that also has a native form. Codata is unfolded only on observation. Schedules — what to inline, fuse, specialize, and
to what generality — are values the specializer takes, not pragmas.

**Byte-level control (section 6).** The owner asked whether this gives C-level control of
memory, and then about reinterpret casts, the C++ stack, syscalls. The answer: control is
not obtained by the language dropping abstraction but by representations — equivalences
between a type and a layout, with their round-trip laws — and by validity facts at the
boundary (a read at an offset is allowed by a proof, a certificate, or an assumption, and
`safe` means no assumption, no oracle, no foreign call). A reinterpret cast is a typed read
through a layout; the stack is a region with a discipline; a syscall is a command of a
world or a `foreign` primitive, trusted and listed. `std::thread` and arbitrary syscalls
are the high bar for lifting C++ code and are met by handlers over a memory interface
(release-acquire or sequentially consistent; relaxed is not offered) and by `foreign`,
never by pretending they are pure. Floats are exact IEEE values with their own laws, not
approximations of reals.

**Representations that do not mirror constructors.** `∥helix∥₂ ≃ ℤ`-style facts were the
owner's example: a type's useful representation can be another type entirely, reached by
an equivalence in the theory, and the layout follows the representation, not the
constructors. This is why "representation choice is out of the key" and why `represent T
by R` carries laws.

**The C++ image (section 10).** The owner's background is low-latency market data at
Citadel Securities, where with Gasper Azman they built a language generating template-only
C++ parsers. The questions: can this emit C++ headers; what does "header-only" even mean
(packaging forced by templates needing visible bodies, nothing deeper); how much of a large
trading system could be lifted; and — the one that reshaped the section — "suppose we
erase the cubical machine and keep an ideal C++ image": the image must preserve
abstraction and factoring, with binding time as late as C++ can express, and the emitted
fragment has its own semantics, so re-import and translation validation are the check.
Executors and senders/receivers are relevant: codata and processes map to senders
(stdexec, becoming `std::execution`), handlers to schedulers, stopped outcomes to stop
tokens. C++26 is not yet a usable target in compilers; C++20 plus stdexec is, with
contracts under `__cpp_contracts`. The system to be lifted is situated in the world —
feeds, books, strategies, risk, deployment, the NIC — and the owner's demand was the
*entire* trading system in this language with the world boundary explicit (section 10.2),
not a kernel of it; "what about `std::thread`" and "what about arbitrary syscalls" were
tests of whether that demand holds, and the answers above are what makes it hold.

**What a law is, and what survives erasure.** A law is an identity about operations (a
path), consumed by the specializer: it rewrites along it before erasure. The owner asked
whether C++'s type system can express or check laws: concepts constrain syntax only; a
concept cannot require that an operation satisfies an equation, with or without a proof,
so laws cannot be carried into C++ as types. The first proposal — laws become comments —
was rejected outright. The resolution, in the owner's words, "compilation can always erase
things": laws are used, not exported; what survives is `[[assume]]` where an instance of a
proven law is a C++ expression, contracts at consumer boundaries, and optionally
certificates (`static_assert` by exhaustion). The library now does exactly this
(`run.py`'s `law` and `assume` cases). The owner also asked whether anything provable
remains provable in the lifted state: yes — the system lives in the theory with all its
proofs; the C++ image is a projection of it, and nothing is proved *in* the image.

**HoTT versus HOTT.** Whether the machine can run HoTT "as CCHM" was called a research
question by the builder; the owner objected that CCHM supports HoTT, and the resolution is
in section 4: CCHM with De Morgan connections is the prelude; univalence computes; HITs are
data with path constructors. Higher observational type theory (no interval, `Id` by type
recursion) is an alternative prelude, not the foundation here. The Brunerie number is a
measurement of A8, never the point.

**Measurement claims.** "Linear in nesting for every term" was claimed and retracted: a
term open in k dimensions can have 2^k faces. The claim that stands is work proportional to
the size of the result. Keys give sharing when the second route arrives while the first is
retained; the schedule decides retention; no policy is added.

**Critiques from the builder, and their disposition.** Accepted: A2 done, g8's transports
are pairwise distinct (keys cannot share them), dynamic uniqueness is structural, strictness
applies only to checked terms. Rejected: an LRU (no retention policy), "can't run Agda"
(false), fixation on the Brunerie benchmark. Errors made in writing this document, all
corrected in place: the linearity claim; laws as comments; floats as lossy; cutting the
plan back after a critique (the plan stays full); a title "the final system"; time
estimates an order of magnitude too high (the right method: lines × token rate plus
machine time, tests once at the end); and, on 2026-11-06, a reading of the specializer's
fallback as "definitions are not unfolded under neutral arguments" — wrong, retracted at
`7be3368`; the real limit is that a stuck primitive operation has no code.

**How the owner works.** From a phone, in long sessions; expects deliverables consolidated
in one document; will not read a draft that needs a critical read (do it yourself and
proceed); wants the complete list of remaining changes with reasons before work continues;
wants questions only when more work could not answer them; forbids subagents on this work;
never "the final"; everything implementable is in scope now. The working mode that was
asked for and worked: continuous autonomous work for hours ("continue, don't stop"),
pulling often, short messages with commit hashes as things land, no waiting on test runs;
reports relayed from the other session are checked against the code before being acted on
(one said the machine "can't run Agda"; it was wrong). Answer the question that was asked;
an answer to a neighbouring question reads as no answer. Commits carry the attribution lines
in the git log and are pushed at once; pull before every commit.

### The decisions, with their reasons (settled; reopen only with a new argument)

- **Wires as variables** (`9b94b43`): reading a held value locks nothing; retention is
  structural (counted references, last holder erases). *No retention policy* (LRU or any
  other) is ever added: an entry lives exactly while reachable. The builder once proposed an
  LRU; rejected.
- **Keys by content**: a producer is named by its body over the keys of what it binds; equal
  work reached by two routes is done once while the first is retained. The schedule decides
  retention; nothing else does.
- **Work proportional to result size** is the performance claim (A8), not "linear in
  nesting" — a term open in k dimensions has 2^k faces. "Linear" was claimed once and
  retracted. Benchmarks (Brunerie's number) are measurements of this, never the goal; the
  owner explicitly does not want fixation on them.
- **Binding times** now / C++-build / run-known / run with one back end; the **first-order
  criterion** (no dimensions, types fixed, path-free data at the boundary, no open Kan
  operations, no live reflection) decides what is native code. The checker's own decision is
  *not* first-order (it reflects). B13 built it (`lib/stage.cub`).
- **Multiplicities** 0 / 1-affine / 1-linear / ω: usage is *computed* after a definition
  checks; erasure is by computed usage, never by the binder's annotation; linear values are
  introduced only in `primitive` definitions (the memo invariant); dynamic uniqueness is
  structural (box holder counts), not a static analysis.
- **Representations are equivalences** only (`represent T by R where to/from/laws`); layouts
  are computed from them; floats are exact IEEE values with their own laws, *not*
  approximations of reals — the owner rejected the "lossy representation" framing outright.
- **Laws are consumed by the specializer before erasure** and never emitted as comments; the
  only things that survive into the C/C++ image are `[[assume]]`/`ASSUME` facts and
  consumer-boundary contracts. "Laws become comments" was proposed once; the owner called it
  insane; it is gone.
- **The C++ image** (10.1): the export preserves abstraction and factoring; binding time
  "as late as C++ can express"; the emitted fragment has its own semantics, so re-import and
  translation validation are the check, not trust.
- **The store is the codebase** (F34, built): keys are hashes of elaborated code with names
  outside; retention by reachability; no versions, no branches as primitives.
- **Verified certificate checking** (F32): search is unverified, the checker is a verified
  term, applied by evaluation. Built three times over (`linproof`, `eqproof`, `ring`).
- **Processes** are guarded trees over a signature; worlds are handlers; replay reproduces
  a run from the responses alone. The inductive part is built in Church form (`lib/proc.cub`);
  the guarded form waits on positivity under Π codomains (18.18).
- **Navigation** (F33): a view is a set of identities; applicability is first-order matching
  on normal forms; validity is never a step; choice is by cost and purpose.

### Vocabulary you must have before reading code

`%view`/quote/eval (reflection: a value read as a code and back), `celim`/`ccase` (eliminations
with hypotheses for recursive fields, the hypothesis a suspended recursion read with
`cforce`), `cwelim` (word elimination; what every word function normalizes to), `cglob` (a
checked definition by index; `(code name)` quotes to it), `%gty`/`%gval`/`%gusage` (a
global's type, value, usages), `%dent`/`%dtab`/`%rtab` (the data, case and representation
tables), telescopes `tx/ty/tz`, `Lst`/`lcons`/`lnil` (the untyped list used everywhere before
the checker), `some`/`none` (`Opt`), "typed" versus "untyped" definitions (a definition with
a signature is checked; one without is not, and a checked definition may not mention an
unchecked one), the first-order form `FO` (`lib/fo.cub`: `fvar/flit/fcon/fswitch/frec/fprim/
flet/fif/floop/fclos/flam/fapply/fassume`), `lower` (codes → FO), `lift` (closure conversion
and recursions to definitions), `specialize` (normalize, rewrite along rules, lower),
`emit-prog` (boxed C) and `emit-prog-u` (unboxed C), `Lew`/`le` (order as a witnessed sum,
over Word and over Nat), `fromNat`/`toNat` (the Word ≃ Nat used to carry proofs across).

### The elaborator's current behaviour (these cost hours each when unknown)

1. Definitions must be declared before use; a mutually recursive pair is written with the
   later function passed as an argument.
2. A name declared by a later module rebinds the references of every module parsed *after*
   it — silently. `tools/names.py` (first in `make test`) keeps every top-level name and
   constructor in `lib/` unique. Run it before every commit.
3. A checked definition may not mention an untyped one (stderr: `not a bound variable or
   checked definition`, while stdout continues — a test reading stdout sees a silent exit).
4. `data` after the checker needs typed fields; untyped data goes before `lib/check.cub`.
5. `(code …)` does not accept word literals (`#0`) or `Word` as a motive; put the constant in
   a checked definition and name it.
6. Composed paths (`catw`, `transN`) need their endpoints from a signature: write step lemmas
   with explicit arguments rather than nesting implicits; λ-paths in argument positions have
   their endpoints evaluated, so use congruence lemmas (`congw`, `cong2N`).
7. `Empty-rec` in a copattern or step needs its implicits explicit (`{lzero} {T}`).
8. No Π over `Level` inside a type's body; level polymorphism lives in signatures, so a
   level-polymorphic type takes the level as a parameter (`Proc {l} S A`).
9. Positivity: a recursive occurrence under a Π codomain (`psend (n : Word → Pr)`) is
   rejected until 18.18 lands.
10. The store (`CUBICAL_STORE`, default `.cubical-store`) skips definitions the current
    checker already accepted; `CUBICAL_NOCACHE=1` turns it off, and the tests set it.

### Known defects and cautions in the machine (from the builder's own review, 2026-11-06)

- The store's certificate trusted a checker named by the library texts alone, so a change to
  `net.c`'s rules with `check.cub` unchanged kept every certificate valid. Fixed at `dd301c8`:
  the certificate's checker name also hashes the running binary; a rebuilt machine rechecks
  the library once. The identity key is unchanged (build-independent, as F34 requires).
- `retire_boxes` in `src/net.c`: a box freed in one thread while another still releases it;
  reproduced on `19728ef`, predates the day's commits; makes the workers stage of `make test`
  fail spuriously at about 2% per 16-worker run (a segfault, more likely under memory
  pressure). Fix in `src/net.c` before reading a failing workers stage as a regression.
- Unmerged, on the builder's branch (not on `main`): `elab` store entries holding hole
  solutions as positional codes (`cglob 34`); a different load order would substitute a
  stale solution — a spurious rejection, never a false acceptance. Store them in canonical
  `G<key>` form, or fold the positional signature into the `elab` key, before merging.
- Instance resolution reports a genuine type error against the last candidate tried;
  module member substitution does not respect clause-pattern binders (a naming corner);
  `ptr-of` pins the string forever by design; `foreign` accepts the two x86-64 call families
  (all integer/pointer, all double) and refuses others at declaration; `V_NPRIM` adds only
  equalities the machine already computed and has no code yet.
- Never run two heavy checker processes at once in a small container: the 16-worker stage
  of `make test` plus another run exhausts memory and kills one of them (seen 2026-11-06).
- When working from a scratch copy of the repo, check that `lib` is not a symlink into the
  repo before writing (two repo files were overwritten that way and restored).

### The state in one picture

Machine (builder): the net, composition, the checker as a term, keys, structural retention,
multiplicities (B11), representations and the byte layer (B12), the first-order criterion
(B13), content keys and the store (F34), `#safe`, instance arguments, records, modules,
`deriving eq`, `foreign` C calls, stuck primitive operations as neutrals (no code for them
yet), C22 arrays. Library (side agent): verified checkers for linear arithmetic, equational
rewriting and the semiring of Nat, with autodiff's Taylor theorem by reflection; the store's
theorems as rewrite rules; the back end from codes through the first-order form (recursion,
closures, the byte layer, representations consumed) to C, boxed and unboxed (words, pairs
and flat records by value; `fib` and a two-load decoder print as the C an engineer writes);
decidable equality derived from the data table and compiled; a byte decoder with its bounds
discharged by certificates; processes with handlers, supervision and replay. Seventeen
programs compile with gcc and run under both printers; `make test` is green at the commit
in the header.

### Reading order

1. This section, then sections 0–3 (value, core, principle, machine state): the premises.
2. Sections 4–7 (semantics, compilation, representations, world) and 10–11 (the C++ image,
   the store): what is being built and why in that form.
3. Section 12 (the build list with statuses) and section 13 (the trusted surface).
4. Section 18's status paragraphs (18.1, 18.2, 18.3, 18.5, 18.6, 18.7, 18.13, 18.14) and
   18.12's elaborator findings; the full specs only for the item you take on. The specs of
   section 18 are drafts written before the items were built and critically reviewed by
   their author, not by the owner; where a status paragraph and the spec text differ, the
   status paragraph (what was built, with its commit) is the truth and the spec is the
   intention it was built against.
5. `README.md` §9 and these files, in this order, each with its readings at the end:
   `lib/code.cub` (the Code type), `lib/fo.cub`, `lib/lower.cub`, `lib/specialize.cub`,
   `lib/repr.cub`, `lib/emitu.cub`, `lib/linproof.cub` (how proofs are written in this
   elaborator), `lib/proc.cub`, `tests/prim.cub` and `lib/prim.cub` (B12), `lib/stage.cub`
   (B13).
6. Run `make test` once (it takes a while and is the arbiter). Then the queue.

### Before taking an item, you should be able to answer these without looking

Why is no retention policy added? Why is erasure by usage and not by binder? Why are floats
not approximations? Why do laws never reach the emitted image as comments, and what does?
Why does a stuck primitive operation without a code block specialization of anything over the
byte layer? Why is the checker's decision not first-order? What does "work proportional to
result size" promise and not promise? Why is the inductive process type in Church form, and
what exactly waits on 18.18? What does `tools/names.py` protect against, and why is it in
`make test`? Why are open dimensions and higher inductive types not limitations of the
compiler? What is section 16's ledger for, and what does an unanswered row become? Where exactly
does univalence compute in the path from a definition to the C that runs — name the
equivalence, the path, and the transport? If any answer is missing, read the section that
holds it.

---

## 0. The value

One language holds the specification, the implementation, the memory layout, the proof that
they agree, and the connection to the outside world, and all five compute on one runtime.
Every boundary a conventional stack keeps between those things becomes an equivalence inside
the theory, and transport along it does the work people now do by hand: compiling, converting
between layouts, checking that an optimization is safe, keeping a simulator in sync with
production, building, versioning, deploying. The hot path runs as the machine code a C++
engineer would write. Every assumption that code rests on is written where it is made, and
wherever it is proved, it stops being an assumption.

This is foundations, not tooling. The claim is that computation has a correct form, in which
what a machine does is substitute along identities, and that every existing language is a
partial approximation to that form made under the constraint of not having equality as a
primitive. The mechanisms those languages added (templates, handlers, borrow checkers, build
systems, hash-addressed codebases, contracts, RCU, schedules) are each a row in the ledger
with a construct here that is derived rather than added.

Success is measured by three things, none of them a benchmark:
1. The ledger (section 16) has no unanswered row: every feature of every frontier language is
   a consequence of the theory, a library written in it with no new judgment form, or a hole
   in meaning not worth having. A row with no answer is a gap and becomes a ★ item.
2. Normalizing a term with open dimensions does work proportional to the size of its result
   (times nesting depth for the traversal): nothing is built that the result does not
   contain (A8). Not "linear in nesting for every term": a term open in k dimensions can have
   a normal form with 2^k faces, so no evaluator is linear there. g8 at 1.1M steps for a 20 KB
   result is the defect in these terms, checkable per term as steps against normal-form
   size. Published numbers such as Brunerie's are measurements of this, not the goal.
3. A full-scale system, the trading stack (section 10), runs at hand-written speed with every
   assumption visible.

## 1. The core

Five things, unchanged across every revision of this plan:
1. **The rewrite rules** of the interaction net (`src/net.c`): each step rewrites two cells at
   their principal ports and consults nothing else.
2. **The composition clauses** as terms (`lib/prelude.cub`): how transport and composition
   compute at each type former; CCHM with De Morgan connections, generalized composition,
   composition in the universe as a type former.
3. **The checker as a term** (`lib/check.cub`) over `%view` reflection, codes, quote and eval.
4. **Keys**: every producer is named by its body over the keys of what it binds, so one term is
   one pointer, and equal work reached by different routes is done once *when the second route
   arrives while the first is still retained*. Measured: Brunerie gets 248 hits on one thread
   (breadth-first ring) and 9 with workers (depth-first deques), same answer, same steps within
   the memo-off count. The schedule decides retention, and no retention policy (LRU or other)
   is added: an entry lives exactly while reachable, nothing more.
5. **Retention by reachability**: a shared value lives exactly while something refers to it.
   Since `9b94b43` this is structural: a box is reached only through counted references and
   the last holder erases it; no reclamation protocol exists or is to be written.
Item 1 became literally true at `9b94b43`; before it, reading a held value locked its
neighbours.

Every property claimed below is inherited from these, in a forced order. Locality (1) is what
makes parallel evaluation unable to change a result and makes every later guarantee a property
of the rules rather than of a scheduler. On locality sit the composition clauses (2): because
transport is a term the net runs, univalence computes. On computing univalence sits
canonicity: generalized composition leaves nothing formal on closed points, so closed
path-free values are constructors. On canonicity sits native code: a constructor has a layout,
so a region of constructors transports to bytes with nothing lost, and the path is the
compiler's correctness. On the checker as a term (3) sit all programs about programs: the
specializer, the certificate checkers, the navigation layer; and since they run on the net,
the net's cost on symbolic terms is the cost of every program about programs, which is why
A8 and A9 come before the back end. On keys (4) sits the store. On coinduction sits the world.
Remove a layer and the ones above lose their justification, which is why a proposal that
changes one of the five is a core decision (★ in section 12) and everything else is a
consequence to state or a library to write.

The relation to the earlier documents: Respecting Equality fixed the claim (a computation is
correct when it respects the equalities its data satisfy); posing a declaration with its
identities and normalizing (5.27) is a reading here, and reflection with no outer layer
(5.26) is `%view` and the checker as a term. Witnessed Computation names the trace, which the
step counts and the memo's accounting report and the store keeps under keys. The Interactive
Symbolic Computer's fiberized computation `A ≃ Σ_b fib_f(b)` is the factorization of every
irreversible program into an equivalence and a projection (C23).

## 2. The principle, and the spectrum it covers

The language has values that are equalities, and using an equality means transporting along
it. Wherever a conventional system has two things that are "the same" only by convention, this
system has an equivalence, and the convention becomes a computation.

| Where a stack keeps two things apart | Here |
|---|---|
| specification and implementation | a clean definition and a representation, joined by an equivalence |
| compile time and run time | normalization with some inputs known, at any moment a value arrives |
| source and compiled code | a term and its transport to a native representation; the path is the correctness proof |
| one data layout and another | an equivalence; every program and proof transports across |
| an optimization rule and its justification | a theorem the specializer applies |
| pure code and the outside world | a program over an interface, with the world as a handler |
| production and simulation | one program, two worlds |
| the checker and the programs it checks | the checker is a term on the same machine |
| the codebase, the build cache, version history, deployment | the content-keyed store: a definition is its key |
| typed and untyped programs | one runtime; checking is a term run per definition, now or later |
| meaning and schedule | a term and a schedule value; every schedule gives the same answer |

The machine runs unchecked terms (Brunerie's pipeline is untyped and computes); checking a
definition is running a program on it, now, later or never: the span from scripting language
to proof assistant on one runtime. `#input`/`#supply` is the runtime of a reactive notebook: a
reading proceeds as far as its inputs allow and continues when they arrive. The paradigms
conventional languages split across (imperative, object, functional, reactive, query, array,
shader, hardware description, scripting, proof) are each one region of this spectrum; the
ledger in section 16 names the construct for each.

## 3. State of the machine (at `9b94b43`)

Built and tested:
- The net running CCHM: paths, univalence via Glue, higher inductive types including
  truncations, `%hcompU`, generalized composition (the tube gets the complement of its face),
  constructor-matching composition for path-free data. Canonicity as enforced: a closed value
  of a path-free data type is a constructor; the checker accepts a formal `hcompD` in such a
  type only when its tube covers every closed point.
- Composition clauses as terms: `%comp-pi`, `%comp-sigma`, `%comp-path`, `%comp-glue`,
  `%comp-u`, `%comp-gluel` with `%tproof`, `%case-hcomp`; data-type composition derived from
  declarations.
- Coinduction by copatterns; guarded recursion with clocks, ticks, timelessness, `dfix` at
  `⋄`, `forceκ`, `pfix` as a computing path; productivity checked.
- The checker: conversion with η at Π, Σ, paths; levels with `lomega`; positivity, boundaries,
  coverage, termination (structural, course-of-values, measured), clocks and ticks; quote/eval
  laws tested.
- The surface language and elaborator: holes, dependent matching including on path
  constructors, `decreasing by … using`, copatterns, tick insertion, `calc`, records.
- Sharing: full laziness, lazy one-level copying, substitutions on references, unbuilt
  branches and glue bases; memo with keys per producer, retention by reachability, names by
  origin.
- Wires as variables (`9b94b43`): a port is a principal port or a slot linked by atomic
  exchange; cells carry no back pointers; rules lock nothing; a popped pair is never stale;
  one set of rules for one thread and many; per-worker Chase-Lev deques; termination by an
  in-flight count. Results identical at every worker count with the memo off; TSan clean.
- Brunerie's number (simplified, Ljungström's `brunerie'`, untyped port): −2 in 1.26M steps;
  end to end 3.1 s with one worker, 2.7 s on one thread, on the builder's box. Cached modules
  replay the checked program.

Built on 2026-11-06, in parallel with the builder, as new library files (each with its readings
in `make test`):
- `lib/fo.cub` (`c4c1d79`): the first-order form `FO` (18.5) as data, its evaluator, and
  `lower` from the first-order fragment of `Code` (variables, words and their operations,
  constructors and case, pairs, λ and application, let, word elimination as a loop; everything
  else a call into the net).
- `lib/match.cub` (`80e92db`): first-order matching of patterns with metavariables against
  codes, capture-safe under binders, instantiation, rules, rewriting at the root and at the
  first position, the count of applicable positions: the operation behind 18.6 step 3 and
  18.14.
- `lib/nav.cub` (`48b58f8`): the first executable navigation (18.14): available steps from a
  view, choice by result size with a goal pattern first, navigation to a bound.
- `lib/specialize.cub` (`c99c25e`): the pipeline of 18.6 as it exists today: normalize a
  closed code by the machine (eval then quote), rewrite along rules to a bound, lower to `FO`;
  readings show a known computation becoming a literal, an open function becoming a λ over a
  primitive that runs natively, and word elimination becoming a loop. Steps 4, 5 and 7 wait
  on B11, B13 and the proof lemmas. Load order: the untyped libraries before `check.cub`.
- `lib/lin.cub` (`b8be7b1`): linear arithmetic over codes (F32's core): normal forms, equality,
  order over naturals, Farkas certificates verified by `check-cert`, a bounded unverified
  search whose answer is checked before use; the decoder's `off + 4 ≤ n` obligation as the
  reading. Soundness of `check-cert` against `plusw`'s laws: to state in `lib/std.cub`.
- `lib/emitc.cub` (`d93dfd5`, `21d3894`): the C printer for the first-order form (B16 milestone
  1): boxed values, one C function per definition, GNU statement expressions for let, switch and
  loops, closures through a generated dispatcher; `experiments/fo/run.py` decodes, compiles with
  gcc and runs: factorial as a loop, a switch, let/if, a closure capturing a value, and a code
  specialized by the machine (2^n as a loop) — the pipeline runs from a code to a native binary.
- `lib/fo.cub` gained closure conversion (`lift`): every λ becomes a definition over its free
  variables; the evaluator and the printer agree.
- `lib/linproof.cub` (`39b4c66`): the linear-arithmetic normalizer verified (42 checked
  definitions): `same-norm-eq`, with `plusw` commutativity, associativity, middle exchange,
  `mulw` distributivity and `weq-sound` proved by word induction; order and the certificate
  theorem (`c8cadc0`, `cert-sound`, 251 checks).
- `lib/eqproof.cub` (`2cf715f`, `7fcedec`): equality certificates over uninterpreted
  functions with schematic hypotheses, verified (`eq-cert-sound`), and a bounded trace search
  whose output the checker checks; 308 checks on the line.
- `lib/ring.cub` (`5564454`): the commutative semiring of `Nat` decided by reflection:
  `norm : Ex → Poly` (sorted sums of monomials), `ring-eq : norm a ≡ norm b → eval e a ≡
  eval e b` from the semiring laws of `lib/std.cub`; `(x+1)² = x²+2x+1` and the two-variable
  square are theorems at every valuation by one evaluation of the normalizer (12k steps),
  the proof term the constant path at the shared normal form. This is F31's method
  (reflection) and F32's third checker; 310 checks on the line.
- `lib/derive.cub` (`693067f`): `derive-eq d`, decidable equality as a code read off the data
  table (fields by `%weq`, recursive fields by the case's hypothesis, other data types by
  their own derived equality; HITs none); evaluated on `Nat`, `Bool`, a tree, a record; the
  derived function quotes back at its type. F31's "deriving as terms over `dent`".
- `lib/decode.cub` (`442bbbb`): B12's byte layer meeting F32: `load32`'s bound `le (plus i 4)
  n` from `i + 8 ≤ n` proved by `cert-sound` on a one-multiplier certificate, the check
  evaluated by the checker (17k steps for the bound), carried across `Word ≃ Nat`.
- `lib/emitt.cub` (`49049a6`): C typedefs from B12's layouts (a tag and a union of structs);
  `run.py` compiles a tree's and prints its size.
- The byte layer end to end (`70854ad`): `fo.cub` gains the load/store/bytes primitives
  (evaluated by the machine's), `emitc.cub` a bytes value and `ld64`/`st64`/… in the C
  runtime, `lib/repr.cub` consumes the representations (primitive definitions → native
  primitives with the lengths and bound proofs dropped, conversions the identity, std's word
  operations and represented `plus` native, closed Nat trees words by `%rtab`); `run.py`
  compiles two loads from an unknown buffer with their proofs and runs them (14). Found: a
  term over a stuck primitive has no quote, so nothing over the byte layer specializes under
  an unknown input — recorded under 18.6 as the machine work that B15 needs first.
- Structural recursion in the first-order form (`f502f0d`, `6a6cf58`): `frec d x alts` with
  the recursive fields' hypotheses (from the data table's shapes, `lib/lower.cub` after the
  checker), `vrec` suspensions in the evaluator, `lift` turning a recursion into a definition
  that calls itself on the field; `ccase`/`celim` lower to it. `run.py`: the length of a list
  written as a case with `rec` → a C function calling itself on the tail; the derived
  equality on `Nat` (`derive-eq`) lowered, lifted and compiled (`deq`, prints 10). 12 cases.
- Assumed facts generated (`4f59d7f`): the proof argument a primitive access drops
  becomes `fassume (off + w < n + 1)` around the access; the C image carries the `ASSUME`.
- The specializer under unknowns (`e96cc45`, corrected `7be3368`): a process run against
  its world with the store unknown normalizes to a function of the store and compiles with
  a store closure; the schedule as a value (`Sched`, the rewrite bound).
- `lib/emitu.cub` (`89aed69`): the C printer with words unboxed (typed locals, parameters
  and returns by inference; loops over typed state); all 16 harness programs compile and
  run with it (`UNBOXED=1`, in `make test`).
- Bool as a word in the first-order form, a tagged message decoder (`e2ee1dd`): the decoder's
  `case weq tag 1` lowers to an `if` over loads at literal offsets under their assumed
  bounds; compiled and run (`run.py`'s `itch`, 16 cases). `lower.cub`/`specialize.cub` now
  load after `std.cub`.
- Whole programs (`cd27cdc`): `program-of` lowers a code and then, transitively, every
  checked definition it calls, from the quote of the definition's value (the quote inlines
  the non-recursive definitions it meets; recursive ones become `celim` bodies; what does
  not quote — definitions over stuck primitives — is left as a call); `lift-program`
  converts every body. `run.py`: a code calling the library's `lenw` becomes a C program
  with `lenw` as a function. 13 cases.
- `lib/autodiff.cub` (`bd6130b`): F31's example done as a typed fragment: `D` the symbolic
  derivative over polynomial codes, `fwd` forward-mode AD by dual numbers, `fwd-der : snd
  (fwd x p) ≡ pat x (D p)`, and `taylor : p(x+h) ≡ p(x) + h·p'(x) + h²·q` with `q`
  computed, each inductive step a ring identity discharged by one evaluation of the
  normalizer. 352 checks on the line (409 after B11's usage walk).
- `lib/rules.cub` (`b497f96`): the store's identities as rewrite rules. `rule-of g` reads a
  checked theorem's type through `quote-ty` and makes `l → r` with its Π binders as
  metavariables; `view-of` builds a navigation view from named theorems. Because a type's
  value holds the endpoints in normal form (`plus a b` on a variable is its stuck
  elimination), the rules and the specializer's normalized terms meet in the same form:
  `plus-assoc` rewrites the normal form of `(a+b)+c` to that of `a+(b+c)`, navigation
  reaches it in one step, and `plus-zero` is consumed by the specializer so that what is
  lowered to `FO` is the identity — 10.1's "laws used by the specializer, never emitted",
  running. `match.cub`/`nav.cub` now traverse `celim`, `cforce`, `cdata`. Found and fixed:
  `fo.cub`'s `FT` constructors `tword`/`tdata` shadowed the checker's own (`Tm`), so loading
  `fo.cub` before `check.cub` made the checker loop without bound on `std.cub` — the
  specializer and the typed library had never been loaded together; renamed `ft…`. For E28:
  a constructor name declared by a later module silently rebinds an earlier module's
  references (a checker-side name-shadowing error is the fix, ~20 lines in `src/lang.c`).
- `4e33086`, `bbe9d77`: `experiments/fo/run.py` gains `law` — `plusw-zero-l` read from the
  store, consumed by the specializer, the lowered body `flam (fvar 0)`, compiled with gcc
  and run; `cwelim` added to the matcher (word functions normalize to word eliminations).
  `tools/names.py` keeps every top-level name and constructor in `lib/` unique and runs
  first in `make test`; the clashes it found were renamed (eqproof's `Tm`/`tv`/`tapp` were
  the checker's own; ring's `eval` was code.cub's evaluator; linproof's parallels of lin.cub;
  fo's `shift-list`, a different function from match's).
- `lib/proc.cub` (`92f5aa4` … `953e177`): processes over a signature in Church form,
  worlds as state handlers that may refuse, outcomes, handlers between signatures,
  supervision with restart strategies, replay by recording and replaying worlds (D24–D25,
  7.3); the guarded form waits on 18.18. A process run against its world with the store
  unknown specializes to a function of the store and compiles (`run.py`'s `process`).
- `experiments/fo/run.py`: seventeen programs compiled with gcc and run, with both printers.

Measured, open:
- g8 (transport through the groupoid truncation of the Hopf fibration) is 1.136M of 1.26M
  steps for a 20 KB result; its 6,706 transports of formal compositions carry no memo key
  (systems of faces have none); 291 hits in the run.
- Substitution and erasure are 53% of steps; many substitutions enter values with no dimension.
- `surfc` on a 4-truncation cube with every dimension open does not finish in 6.5 GB.
- Library load about 1.0 s, 0.85 s compiling `check.cub`.
- The Brunerie port is untyped except its data declarations, the 4-cube filler and the
  library lemmas it uses.
- Not in the library (as of 2026-11-06): indexed families, strings with a byte
  representation, arrays (C22), the guarded (run-forever) process form, session types,
  choreographies. (Floating point and fixed-width integers: B12; linear binders: B11;
  decision procedures: `linproof`, `eqproof`, `ring`; processes: `proc.cub`.)

## 4. Semantics

The theory is homotopy type theory with univalence computing, presented cubically after
Cohen–Coquand–Huber–Mörtberg: an interval with De Morgan connections, path types as
functions out of it, composition `comp` and its generalization, `Glue` giving univalence as
a term, higher inductive types as data with path constructors, and universes closed under
all of it. Nothing in the system is a postulate over this: the composition clauses at every
type former are terms of `lib/prelude.cub`, and the checker that decides the theory is a
term of `lib/check.cub` on the same net. Univalence is what makes the rest of this document
possible rather than aspirational: a representation (section 6) is an equivalence and so a
path, a specialization (section 5) is a transport, the identities the store holds (section
11) are the paths navigation moves along (section 15), and the C++ image (section 10) is a
projection of a system whose proofs are all paths in this theory.

The net normalizes open terms: terms with free variables and free interval dimensions, giving
a term with those unknowns still in it. This is what makes paths, higher inductive types, Glue
and the universe compute, and it is what specialization is. Rewrites are local, so parallel
evaluation cannot change results; producers are keyed by content, so equal work reached by
different routes is done once.

Canonicity is the bridge to native code. Values of higher inductive types, Glue and the
universe can be formal compositions; they reach native code through a decoding equivalence
into path-free data, which for a quotient or truncation is a canonical-form function (a set as
a sorted array without duplicates; ‖ℤ‖₂ as `Int`). That is how set quotients get native
layouts.

Alternatives at the theory level (section 15): cartesian cubical (cleaner metatheory, costlier
`fill`); higher observational type theory (no interval, no Kan filling; `Id` by type
structure, `ap` by term structure; HITs and metatheory not yet settled; a second prelude on the
same net if they settle).

Vocabulary: canonicity (Huber); normalization (Sterling–Angiuli, cartesian); ground and
positive types; strong versus weak reduction; two-level type theory (Kovács); quantitative
type theory (Atkey; Idris 2).

## 5. Compilation

### 5.1 Binding times: one back end
| when the value is known | what the back end does with it |
|---|---|
| now, in the net | normalize it away |
| at a C++ consumer's build | a template or `constexpr` parameter (section 10) |
| at run time, at a known moment | a JIT specialization point, cached by key |
| never fixed | an ordinary function argument |

C, C++ templates and the JIT consume one residual term; the only decision per free variable is
its binding time, read off the term.

### 5.2 The first-order criterion, strictness, schedules
A region compiles when it has no free variables or dimensions beyond those assigned to later
stages; every type in it is determined by first- and second-stage variables; its inputs and
outputs are path-free inductive types (closures are code plus captured values). Kan operations
over fixed lines reduce during specialization to first-order terms (identity along a constant
line; `e` along `ua e`; `%comp-pi`/`%comp-sigma` applied along lines of function or pair
types, computable, not free). Only Kan operations with free dimensions remain net work.

Strictness: closed *checked* terms of inductive type normalize, so strict and lazy evaluation
agree on them; the machine also runs unchecked terms, which strict evaluation can send into a
divergence lazy evaluation would skip, so strictness is a schedule choice for checked regions
only. A codata value's normal form is its stuck call; native code unfolds it only on
observation, to the depth observed; the delay type is run step by step until an observation is
met. A native stage that forced codata would loop.

Schedules: everything that fixes cost without fixing meaning (strictness, binding-time
assignment, representation choice, fusion depth, inlining depth, parallel split) is sound by
construction, so a schedule is a first-class value written beside the term; the back end's
defaults are a default schedule; tuning is search over schedules, every candidate correct.
Generality is on the same axis: a binding time may be set *later* than the value is known,
so that an export keeps an abstraction a consumer may instantiate differently; the C++ image
(10.1) is the assignment "as late as C++ can express".

### 5.3 Multiplicities: erasure, uniqueness, linearity ★
Proof terms are terms, and types disappearing does not remove them. Cubical makes "erase
h-level 1" wrong (transport along `ua e` runs `e`). One lattice, two mechanisms:
- **Static, by the checker's usage accounting (QTT):** a binder is **0** when its variable
  occurs only in types or in positions the specializer reduces away; **1-linear** when it
  occurs exactly once and erasure is rejected (an order in flight, a socket, a pool slot: the
  checker reports a resource reaching the end of its scope unconsumed). Native code is emitted
  once for all runs, so what it omits is decided here.
- **Dynamic, by the net (structural since `9b94b43`):** every value cell has exactly one
  handle; sharing exists only through counted boxes and duplicators; so a value is unique
  (**1-affine**) when it is not in a box or its box has one holder, readable at any moment with
  no analysis, and the substitution rules already mutate payloads in place on it; **ω**
  otherwise. Uniqueness may also be asserted statically and checked.
Closed paths in path-free data normalize to constructor trees in the dimension, transport
along them along constant families reduces away, and the path is then 0 by usage. Linearity
and the memo coexist by one invariant: a 1-linear value is never memo-identified, which holds
because resources are created by commands and a command's response enters every dependent
key; purely computed values are never linear.

### 5.4 Components
1. **The specializer, as a term.** Takes a region, known inputs and a schedule; unfolds,
   selects, fixes interval formulas; applies theorems marked as rewrite directions (fusion,
   deforestation) under a terminating discipline (oriented rules with a well-founded order, or
   bounded application; commutativity-shaped paths are never rules); emits first-order code
   with its justifying equivalence. Runs on the runtime; compiles itself.
2. **The first-order intermediate form.** `case` → switch on tag; constructor → struct; tail
   recursion → loop; call → direct call; word arithmetic → machine ops; arrays → native arrays;
   multiplicity-0 binders absent.
3. **The C back end.** Readable C. gcc/clang for speed; CompCert (verified; x86, ARM, RISC-V,
   PowerPC; 10–20% slower than gcc -O2) where the compiler is to leave the trusted base today.
4. **The JIT**, for third-stage variables, memo-cached.
5. **The call boundary.** Closed application of a compiled definition to constructor-built
   arguments → native call; results back as cells or opaque native cells; reading back is
   transport along the inverse equivalence.
6. **Compiled code in the store** under the specialized term's key (section 11).
7. **The C ABI in both directions**: foreign C as postulated representations; compiled
   definitions over declared representations as exported C functions.
8. **The C++ export** (section 10).
Optional targets: WebAssembly (linear memory is the byte layer; the component model gives
sandboxed modules); MLIR for accelerators; circuits (a first-order region over fixed-width data
is a circuit; Clash and Bluespec are the precedents).
Precedents that validate the design: Low\*/KaRaMeL (HACL\* in Firefox and Linux), Lean 4 and
Idris 2 to C.

### 5.5 Compilation and the JIT, worked through

The sections above are the compressed form; this is the same content rebuilt from the
beginning with the examples that exist in the repository, because the questions "what does
first-order mean", "what is a JIT here", and "how is this different from Julia, Zig, GHC
rules, Halide, C++ templates" were each asked and answered at length, and the answers are
part of the design.

**One term, four moments it can be known.** Take `fibw : Word → Word` (fib by word
elimination over a pair). Applied to `#30`, everything is known now: the net normalizes
`fibw #30` to `#832040` and there is nothing to compile — normalization *is* the compiler
at binding time "now". Applied to a variable `w`, the net still normalizes: it unfolds `fibw`
to its body, the elimination on `w` is stuck on the unknown, and the normal form is the
loop itself with `w` free (`cwelim w (0,1) (λ k p → (snd p, fst p + snd p))`). That normal
form is *first-order* — it mentions no dimension, every type in it is fixed, its input and
output are words — so it is handed to the back end, which lowers it to the first-order form
(`lib/lower.cub`: a `floop`), and prints it as C (`lib/emitu.cub`): a `for` over a
`uint64_t` and a struct of two words. The variable `w` has become a parameter: that is the
binding time "never fixed". If instead the consumer is a C++ translation unit that knows
`w` at *its* build, `w` is a template parameter and the consumer's compiler does the
substitution: binding time "C++ build". And if `w` is known at a moment during the run — a
message layout learned at startup, a table size read from a config — the specializer runs
*then*, on the term with that value substituted, and the result is compiled and cached:
binding time "run-known", which is the JIT. There is one term and one back end; the only
decision per free variable is when it is known.

**What "first-order" adds over "I can write a native f(x) in any language".** In any
language a function is compiled once and covers every argument. Here too — but the class of
terms a compiled definition covers is exactly the class whose answers are plain data: terms
with no interval dimension, whose types are fixed at the boundary, whose inputs and outputs
are path-free inductive data, with no composition or transport left over an unknown
dimension and no reflection on the value. The criterion (B13, `lib/stage.cub`) decides
membership and names the reason when a term is outside it (`dim`, `dep-type`, `higher`,
`kan`, `reflect`, `mult`). What is outside is not rejected: it is read by the net, at work
proportional to the result, where paths, higher inductive types and the universe compute.
The owner's phrasing of the line is exact: ordinary inductive types versus higher inductive
types. A proof is a term over paths; it normalizes on the net; what it *proves about* a
first-order term is what the back end uses (a bound becomes an `ASSUME`), and the proof
itself is erased by usage.

**A path argument does not force the net.** `decode` takes `h : le (plus i 8) n` and passes
`bound-i4 i n h` to `load32`. The bound is a certificate checked by evaluation
(`lib/decode.cub`): `cert-check` evaluates to `true` and `cert-sound` turns that into the
proof. At lowering, the proof's usage is zero (it reaches nothing that is read at run time),
so it is dropped, and the fact it carried is printed as `ASSUME((i + 4) < (n + 1))` for the
C compiler. A transport along a *known* equivalence likewise reduces during normalization
(transport along `ua e` runs `e`); only a transport along an unknown dimension stays on the
net. So "the term has proofs in it" is never the reason a term is not native.

**The JIT is a specialization point, and its cache is the store.** When `v` becomes known
at run time, the machine substitutes `v` into the term and normalizes; the result, if
first-order, is compiled and stored under the key of the specialized term — the hash of its
elaborated form, which includes `v`. The next time the same term is specialized on the same
value, the key is already in the store and nothing is compiled. There is no speculation, no
deoptimization and no invalidation: a key is the content, so it can never go stale, and a
term that is not first-order after substitution simply keeps running on the net, which is
always correct. This is the difference from a JVM or V8 JIT (profile, speculate, deoptimize)
and from Julia (specialize per method on argument *types*, cached per signature, with no
proofs between the specializations): here specialization is on known *values and types
alike*, every specialization is an identity (the justifying path), and the cache is
content-addressed.

**What the specializer does, in order** (`lib/specialize.cub`, running on the net, itself a
term): normalize the code at its type (evaluate, then quote the value back to a code —
definitions unfold, known arguments reduce, unknown ones stay as variables); rewrite along
the rules the schedule names — theorems of the store read as directions (`lib/rules.cub`:
`plus-assoc`, `plus-zero`, map fusion), under a bound; consume the representations
(`lib/repr.cub`: `plus` on a `Nat` represented by `Word` is the machine's add; a call of
`load64 n b off pf` is the native load with the length and the proof gone and an `ASSUME`
in their place; a closed `Nat` is the word it represents); lower to the first-order form;
lift closures and recursions to definitions; print. The schedule (`Sched`, B13's stage
types) is a value: how many rewrites, what to inline and fuse, how general to keep the
result. Every candidate schedule yields a correct program, so tuning is search over
schedules with nothing to verify afterwards. That is Halide's algorithm/schedule split with
the schedule's correctness by construction rather than by the schedule language's design.

**Rewriting along theorems, versus GHC RULES.** A GHC rule is a syntactic rewrite the
compiler trusts. Here a rule is a checked theorem whose two sides are quoted from its type
(`rule-of`); applying it is substitution along a path; the specializer can only ever produce
a term equal to the one it was given. `plus x 0 → x` is consumed before lowering, so the
loop that `plus` unfolds to never reaches C (`run.py`'s `law` case).

**Zig `comptime`, C++ `constexpr`, templates.** All three substitute values known at compile
time, and nothing else: they cannot specialize on a value known at run time, cannot act on
a term with an unknown inside (Zig's comptime evaluator stops at the first unknown), and
carry no laws. Here the same mechanism — normalization — runs at every binding time, with
unknowns, and the laws it applies are theorems. The C++ image (section 10) is the choice to
*stop* specializing at the point C++ can still express — so the exported header keeps a
template where C++ would, and the consumer's compiler does the rest.

**Strictness and codata.** A closed checked term of inductive type normalizes, so for it
strict and lazy evaluation agree and strictness is a schedule choice. Codata (a process, a
stream) has as its normal form its stuck call; native code unfolds it only when observed, to
the depth observed; a native stage that forced codata would loop. That is why a process
compiled against its world is a function of the store it reads (`run.py`'s `process`), and
why the run-forever form of processes waits on the guarded (▸) types.

**The call boundary and readback.** A closed application of a compiled definition to
constructor-built arguments is a native call; the result comes back as cells, or as an
opaque native cell read through the inverse of the representation's equivalence. Nothing
crosses the boundary without a type on both sides.

**Where this stands today.** The pipeline above runs end to end on seventeen programs
(`experiments/fo/run.py`), with the boxed and the unboxed printer. Still machine-side: a
code for stuck primitive operations (so a term over the byte layer can be quoted and
specialized under an unknown input; `lib/repr.cub` lowers the source code directly until
then), the specializer calling `first-order` before lowering and reading the schedule's
stages, the JIT's specialization points and the store holding compiled code under keys.
Each is in section 12's queue with its reason.

## 6. Representations

Each is an equivalence between a type and its encoding; there is no other kind.

### 6.1 Automatic layouts
Lists, records, sums: tag plus fields from constructors; identity on structure; no proof.

### 6.2 Declared representations ★
Declared once with a model in the language, the equivalence, and for each fast operation its
in-language definition and a path to the clean operation, checked once. Trusted: that hardware
or C matches each small definition. First set:
- `Nat` and `Int` as machine words with `big.c` beyond 64 bits (in the machine, not declared).
- Fixed-width integers: their own types, modular; the ring map to ℤ is a theorem; saturating
  operations are other operations on them.
- Floating point `Float32`/`Float64`: their own exact types, finite bit patterns with IEEE 754
  operations, rounding modes, comparisons, special values; layout equivalence to 32/64 bits;
  `simd` lanes. First step: operations postulated with their IEEE specifications (trusted
  surface); defining them in the language comes after.
- Strings: code-point sequences with UTF-8 bytes; validity is well-formedness.
- `Fin n → A` and `Fin n → Fin m → A` as arrays with layout policies (row-major, column-major,
  strided, custom); bounded vectors as `Σ (k ≤ n), Vec k A` in fixed storage; pools with
  stable addresses.
- Messages as wire buffers read in place; intrusive lists; struct-of-arrays books;
  shared-memory rings.
- Quotients and truncations by canonical forms (section 4).
Derived, not declared: trivial relocatability (position-independent encoding, so moving is
`memcpy`); punning validity (`A ≃ Σ bs, validA bs`; a pun `A → B` is valid when every valid
encoding of `A` is valid for `B`).

### 6.3 The byte layer and the definition of safe
Memory is a byte array the program owns; an address is a machine integer; loads and stores
read and write bytes at computed addresses. Pointer arithmetic, casts to and from integers,
tagged pointers, `container_of`, allocators as explicit values, alignment, unions, placement,
bit-fields, byte order, relative pointers: integer and byte operations compiling to the same
instructions. A typed read decodes bytes through a layout (`reinterpret_cast`, `bit_cast` cost
nothing; reading a buffer as a struct is defined where the bytes are valid). The stack is a
region scoped to a call. Memory the program does not own is reached through world commands, so
every access happens as written. SIMD, prefetch, popcount, inline assembly: primitives with
specifications.

Each access carries a validity fact: prove it (discharged by a procedure, section 12.F, or by
hand), check it, or assume it (marked, searchable). **Safe**: a region with no assumed
validity facts, no unchecked oracle uses and no foreign calls. A safe region calling an unsafe
one inherits its assumptions as visible hypotheses.

### 6.4 Uniqueness, sharing, mutation, sending
- A producer's key is computed at creation; cheaper keys never mean later keys.
- Current behaviour: a value is updated in place when it is not in a box or its box has one
  holder; when a reader's copy is erased and the count returns to one, uniqueness returns. What
  remains is letting the first-order form exploit this (C21).
- Through parts: projecting a field out of a unique constructor cell consumes the cell;
  rebuilding with one field changed reuses it, so projections and a rebuild compile to stores
  at offsets; when the value is shared the rebuild allocates: copy-on-write.
- Sending: a value sent to another process is consumed if unique or shared if immutable; a
  mutable shared value cannot be sent.
- A destructor that only frees is erasure; one with side effects is a command at a definite
  point. Aliasing pointers are integers into a region threaded as state; they are not wires.

## 7. The world

### 7.1 Interfaces, programs, handlers
An interface is a signature of commands with the type of each response. A program is a guarded
tree over it (the free guarded monad); a world is a handler. `#input`/`#supply` is this today.
The signature is the parameter of the process type (`Proc Cmd Resp`), which exists: no new
judgment form, no effect rows, no effect polymorphism in the theory. Inclusion between
signatures is an explicit map applied by transport. A region with no signature is pure, which
the first-order criterion reads; handlers compose (a mock exchange over a network model); a
region can only issue the commands in its type, which is capability security. The world can
push: a feed, an interrupt or a signal is a `recv` whose response is the next event. Logical
time is the ticks of guarded recursion; wall-clock time is a response.

### 7.2 Outcomes, supervision, failure
A process's outcome is a value: `done a | stopped | failed why`. A supervisor is a process
whose interface includes `spawn k` (instantiate key `k`) and `await h`; a restart strategy is
a function from outcomes to actions; restart is instantiating a known key with the
configuration value, so hot code loading is rebinding a name. Isolation is structural. A
process that fails in the language while holding a linear resource has an outcome carrying
it, so cancellation on that failure is a clause of the supervisor and provable by bisimulation.
A process killed from outside returns nothing: the `await` response then comes from the world,
the resource is gone from the language's view, and accounting for it is a recovery protocol
against the counterparty with its own specification. State migration across versions needs an
equivalence between state types; rebinding a name does not supply it.

### 7.3 Consequences
- One strategy term, three worlds; fused simulation; replay from the observation stream, which
  is also the debugger. The stream records the order of the world's responses, which is the
  only interleaving that can affect a result; the net's own rewrite order cannot, and is not
  reproduced. A replay reproduces the run, not its time: cost is schedule-dependent (memo
  hits 248 on one thread, 9 with workers) while results are not.
- Invariants as theorems for every input stream.
- Unspecified behaviour is a world choice; implementation-defined behaviour a platform value.
- Concurrency: processes over a memory interface with a named model (SC and release/acquire
  clean; relaxed unsettled); `atomically` as a command whose handler is the transaction
  manager, typed so a transaction cannot do I/O; linearizability as refinement (safety
  properties carry, one direction); Iris-style reasoning for lock-free structures.
- Protocols: session types (a codata type whose fields are the allowed next messages);
  choreographies (the global protocol once, each binary its endpoint projection; projection is
  specialization against the role for programs meeting the standard projectability
  conditions, which the checker enforces).
- Model checking: a reading over every choice a finite world can make, a term.
- Trusted: the drivers.

## 8. What is provable
Any true statement about a byte-level procedure: its meaning under a representation; what
memory it touches (region splitting is an equivalence, so framing is from the types);
invariants; safety properties for every world; bisimulations. Lifting makes proofs cheap by
transport. Assumptions and world specifications are hypotheses. Physical time is not meaning;
latency claims hold against a cost model; address-level claims outright.

## 9. The correspondence with C++
Export is constructive (section 10). Import splits: for arbitrary C++ it is a thesis (every
C++ program with defined behaviour has a counterpart; UB becomes a proof, check or marked
assumption; data races have no counterpart), a theorem only once a formal C++ semantics
exists in the language (none complete exists anywhere; Cerberus covers much of C). For the
fragment the image emits (10.1) it is constructive and finite: the fragment has a semantics we
define because we emit it, so code written in it imports losslessly up to proofs, and that
fragment is the migration dialect. The construct-by-construct mapping is in the ledger (16.1).

## 10. The C++ export and lifting a trading system

### 10.1 The C++ image
C++ holds, at compile time, a functional language over types and integral constants; at run
time, a simply typed language over structs and functions; and no types depending on run-time
values. A translation into C++ therefore assigns each dependency one of four binding times:
the net (normalized away), C++ compile time (a template or `constexpr` parameter), C++ run
time (a value, with the dependency erased from the type and kept as an invariant), or erased
(proofs). The specialized export sets each as early as possible. The **image** sets each as
late as C++ can express: dependencies on types and constants stay template parameters,
dependencies on run-time values become run-time values with contracts, proofs are erased, and
only what C++ cannot express at any stage (paths as objects, HITs without a declared
canonical form, dimensions, universe levels) is normalized away first. Every assignment
between the two is a valid image; the assignment is a schedule parameter (5.2).

The image is as factored as the source because the translator is **compositional**: one
source form to one C++ form, one definition to one C++ entity, nothing inlined or specialized
unless the assignment says so; names, modules and documentation come from the store's
metadata as identifiers, namespaces and comments. Form by form:
- function uniform in a type → template; function over codes matching on the code → partial
  specializations or `if constexpr` over traits; a type computed from a constant by recursion
  → a template metafunction on a non-type parameter;
- records of operations with laws → a concept plus the generic algorithm as a constrained
  template; instance resolution → overload resolution or a traits specialization; laws → the
  four carriers below, never a comment;
- inductive data → `std::variant` of structs or a tagged struct, matching as `std::visit`;
  where a representation is declared, its layout and operations instead;
- dependent pair with hidden type → `variant` over a finite code set, else a virtual
  interface or `function_ref`-style erased object;
- codata and processes → senders (stdexec) or coroutine awaitables; ticks and clocks erased;
- paths → erased at multiplicity 0; transport along a representation equivalence → the
  explicit conversion pair with its round-trip laws carried as below;
  `transport (ua e)` → `e`; a HIT exports only through its canonical-form equivalence;
- multiplicities → 1: move-only by value; ω: `const&`; 0: absent; 1-linear: move-only with
  `[[nodiscard]]` and a debug-asserting destructor (C++ cannot enforce linearity);
- representations → classes with the declared layout; `mdspan` for index layouts; `bit_cast`
  for punning; universe levels erased; modules → namespaces and headers; the codebase key as
  an attribute on the entity.

**Laws.** Laws are data the specializer consumes before erasure: it rewrites along them,
decides validity facts so no check is emitted, and fixes representations. Compilation then
erases them. What reaches C++ is only what still has a use after our erasure, and there are
two such uses: `[[assume(expr)]]` where an instance of a proven law is a C++ expression,
because the C++ optimizer runs after us and can use a fact we proved (a false assumption is
undefined behaviour, so this slot is for proven facts and nothing else); and a contract
`pre`/`post` at a consumer boundary, because a consumer-supplied parameter arrives after our
erasure and the hypothesis about it is enforced there. Optional output, not part of the
translation: `static_assert` by exhaustion on finite domains, and the law as a named
`constexpr` predicate with its certificate checked by `static_assert(check(cert, goal))`
through the image of F32, for readers who want the compiler to verify it.

What C++ cannot hold is the proofs themselves; the image carries each law in the strongest
form C++ has, so its correctness argument is reduced to C++'s trusted forms, which is what
"erase the machine" means; the codebase stands alone and compiles with gcc. Productivity,
linearity and dependency on run-time values become checked or trusted. The translator reports
per definition: exported fully, exported with contracts, or not exportable with the reason.

The image uses a deliberate **fragment** of C++ (concepts, `constexpr`, `if constexpr`, folds,
`variant`/`visit`, `span`, `array`, `unique_ptr`, move-only types, senders, `bit_cast`,
`mdspan`, contracts where available; no raw `new`, no macros, no inheritance except for type
erasure). The fragment has a semantics we define, so: translation validation of the image is
feasible without a semantics of all of C++ (re-read the emitted fragment as a term, compare
with the source up to erasure, per definition), and consumer code written in the fragment
imports losslessly up to proofs (section 9). The residue printer (B19's first milestone) is
the image at the fully specialized assignment.

### 10.2 Lifting a trading system
A trading stack is the first full-scale system because it exercises every layer and has an
exact standard: its hot path has to be the machine code a C++ engineer would write (transport
and the byte layer); its codecs and books are representations with laws; its strategies are
processes against exchanges (the world, replay, fused simulation); its orders are linear
resources with lifecycles (multiplicities, failure); its per-box binaries are specializations
of one corpus against a configuration including the box's role (the store, projection); its
existing C++ is the consumer of the export; and its success is tick-to-trade latency against
hand-written C++, a number.

The C++ compiler is a weak-reduction evaluator over types and constants (template = unreduced
λ, instantiation = β, `if constexpr` = case, packs = lists of codes, folds = recursion,
inlining = β for function parameters); a residual term open in second-stage variables, printed
as a template, is reduced by the consumer's compiler. Header-only is packaging, forced by
templates needing visible bodies. Target: C++20 baseline; senders through stdexec (becoming
`std::execution`); hypotheses as contracts under `__cpp_contracts` (ignore/observe/enforce =
assume/observe/check; "prove" is the mode C++ lacks); layout `static_assert`s now, reflection
later; `mdspan` layouts; move-only RAII for unique regions. The concept gap: concepts check
syntax; hypotheses are stated, checked by contracts where decidable, or removed by bringing the
parameter into the language.

| component | what varies, when | emitted as |
|---|---|---|
| codecs | handler, message subset: build | template over handler and message pack; decode fused into handler |
| normalizer | venue set: build | fold over a venue pack |
| order book | price/qty types, depth, layout: build | class template; representation per instantiation |
| OMS, session, sequencing, recovery | interface: build | process templates; orders as linear resources; supervisor's crash clause |
| risk checks | limits: build or run time | `constexpr` predicates or arguments; invariants proved |
| rings, shared memory, pools | element types, sizes: build | representation class templates |
| timers, clock, pinning, NUMA | the machine | inline wrappers over world commands |
| logging, telemetry | sink: build | async process template |
| mock exchange, replay | the strategy: build | process templates; the sim binary is one instantiation |
| configuration | per box and role: build | a `constexpr` value every template takes |
| strategies | all of the above | consumers of the headers, or terms |

A binary is `main`: instantiate against the configuration, connect to a scheduler and the
drivers; emittable from the configuration. Outside headers: vendor drivers, the kernel,
assembly, run-time variation until the JIT, unported C++. Costs: per-TU instantiation,
controlled by emitting closed C for configurations the net sees and templates only for what is
open. Order of lifting: codecs; books and normalizer; rings, pools, session; OMS, risk, mock
exchange; strategies.

## 11. The store is the codebase ★
A definition is its key. Two keys with a stored mapping: the **codebase key** is the hash of
the elaborated code (the `Code` type, type annotation included), stable across compiler
versions; the **runtime key** is today's memo key (the compiled body over bound keys), derived
per compiler version. Names are rows in a separate table. Under a codebase key the store holds
the term, its type, normal forms, compiled forms per configuration and compiler version, and
the proofs mentioning it. Consequences: no build step; versioning by content (an edit is a new
key; a proved equivalence between old and new transports every dependent's proofs; a merge is
a third definition someone writes); hot loading (rebinding a name; running instances keep
their key; state migration needs an equivalence between state types); distribution (a wire
across machines carries a key; the receiver fetches what it lacks). Retention is reachability
from the named corpus.

## 12. The build list

Dependency order. DONE is as of the revision's commit. ★ marks a core decision (changes the
checker, the key, or the net's rules); other items are consequences to state or library to
write.

**The queue as of revision 7.9 (2026-11-06, after the three-line build).** Of revision 7.8's
queue below, everything landed: the `retire_boxes` race (`69b1372`), the stuck-primitive
code `cprim` (`c8931ab`), primitive parameter usages (`df1c553`), 18.18 (`d5e47fb`), A8
measured with `%transH` and the next step named (18.16's status), the specializer consuming
usages, B13 and `Sched` (`36b9e30b`), recursive data in `emitu`, the acceptance programs and
`tests/fo.cub` (`8b099b02`), choreographies (`6d098a67`). Also landed the same day: B18
(both directions for words, `e9b9c983`, `680485ea`), B19 the C++ image (`ee5c04a2`), C21
in-place update (`e27eda2c`), D24's guarded form (`5b9de865`), D26 (`43e8fb10`,
`bf3ea4eb`), E28's redeclaration error, rebuilt scrutinees and fording (`e7632b1e`,
`38c1fa89`), F31's remaining pieces (`896713ea`, `e1f33110`, `5b1b57f9`), F32's directed
search and bridges (`89ec3e0e`, `d1e5f790`), item 29 (`fd188bf6`), `%gsteps` (`c20116d7`),
`--serve` (`50ffb680`), the rewriting discipline (`bd21c486`). What remains, in order of
value: (1) A8's next step — off the queue as of 2026-11-07 (18.16's A8d status: dimension-invariant keys built, no keyed composition repeats, keys alone +1–4% steps, withdrawn; A8c status: sharing by key never applies, 0 of 311 compositions with keyed inputs; the watermark bound cut nothing, the rest unbuilt behind wires with no back pointer; the generic 6-cube past a 2.5 GB cap); not formal compositions in path types (tried, withdrawn, 18.16):
sharing a composition across the dimensions the tube maps apply it at; (2) B18 DONE for words, byte strings and constructors both ways (`#native` over
`emit-lib-boxed-u`: a constructor whose fields are values crosses as the boxed `V`, a boxed
result is rebuilt by the entry's `box<d>`; `cubical_call_data(name, args, types)` from C;
libraries cached by the program's hash, item 17); `V_NATIVE` DONE 2026-11-06 (opaque handles
of type `Native` from hand-written C, `#native … from`; boxed results kept native, `#native … keep`;
18.8); (3) A4's
bitmask formulas (18.19, designed); (4) E28: DONE 2026-11-06 for unification-based implicits through reducing hypothesis types,
holes in composition lines, generalized variables, and the hypothesis at the field's own index
for recursion on a fording family (`vmap`; 18.12); the goal rewritten by the index equation and index
unification at splits with absurd cases (2026-11-06); (5) DONE: item 30's retraction (`#retract`) and positions in errors; (6) DONE: F32 over `Int` (`lib/std.cub`'s
`Z2` reading with its laws, `lib/intproof.cub`, `lib/intlin.cub`); (7) DONE: the memory interface in `W` (`w-load`/`w-store`/`w-cas`/`w-fence`, acquire /
release / acquire-release / sequentially consistent, on addresses from `ptr-of`;
`tests/world.cub`); (8) item 10's same-machine Agda comparison; (9) DONE (`7a2c96c4`, `62c669fc`, `13ae6b9f`, `54be3050`): Line B's leftovers — `lib/emitx86.cub` (18.7's
encoder for the word-only fragment, run from executable memory; `X86=1 run.py`), 18.5 step 7's
statements read by computation per family (the assembled path for an arbitrary term of the word fragment since built, `lib/foproof.cub`, item 15),
`fthunk` in the printers, parameterized data types in the C++ image (templates, read back). A trap for test writers: `#same f #1 = #8` parses `#1 = #8` as a
face (`src/syntax.c`'s system syntax); parenthesize the application. The list of revision 7.8
follows for the record.

Before anything: the `retire_boxes` race in
`src/net.c` (above). Then, machine side, in order of what they unblock: (1) a *code* for the new stuck-primitive neutrals (`V_NPRIM` is read by `%view`
as `wprim k args` but `qneu` gives none), so that terms over the byte layer quote and
specialize under unknowns — the one thing standing between `lib/repr.cub`'s direct lowering
and the specializer proper (18.6); (2) 18.1's primitive parameter usages (erasure of bound
proofs by usage instead of `repr.cub`'s by-name dropping); (3) 18.18 positivity under Π
codomains (guarded processes, session types); (4) 18.16 A8; (5) the back end consuming
B13's verdicts and `%rtab` (the specializer calling `first-order` before `lower`). Library
side (this agent's line), independent of those: recursive data as typed pointers to their
own structs in `lib/emitu.cub` (the rest of B16 milestone 2), the acceptance programs at
size (`fib 30` recursive, a sort over C22 arrays, the decoder from the boundary), the
specializer reading `Sched`'s stages, choreographies in Church form. Everything listed is
current scope; none of it is blocked on a decision.

**Priority, derived from the claims.** Three properties are claimed: determinism across
workers (A1, DONE); canonicity (DONE); and that normalizing a term with open dimensions does
work proportional to its result, building nothing the result does not contain (section 0,
measure 2). The third holds in one family (the nested-loop table: linear results, linear
work), fails in another (`surfc`), and concentrates in a third (g8: 1.1M steps, 20 KB). A8
is the item that makes it hold, and it comes first because every program about programs
pays the net's cost; Brunerie's original number is one measurement of it against a published
figure, a useful benchmark and not the center. Engineering with a known answer includes
B11's static side (usage accounting; built 2026-11-06, usages computed, erasure by them
still to be applied by the back end). A9 is separate: it closes the simplified comparison with
Agda by removing load time, which is engineering with a known answer. Neither gates the core
decisions B11–B13, which proceed in parallel and in that order (B11 and B12 landed; B13 next); the back end (B14–B18) is
engineering once they are made. D and E are independent of B. F follows B and E. The typed
port (A10b) is the checker at scale and gates E28.

### A. The rewriting runtime
1. Concurrency: wires as variables, no locks, per-worker deques, in-flight termination. DONE.
2. DONE at `9b94b43` (boxes are holder-counted and erased by the last holder; see core 5).
3. DONE in the form the runtime has (`1010151e`, `e61e081a`; `global_copy` in `src/lang.c`):
   a definition whose value is a closed λ, a constant constructor or a data type is built from
   its compiled syntax at every reference (one cell, no copy); any other is copied lazily one
   level at a time from the held value, named by the definition's key.
4. Intervals, faces, substitutions in the cell. DONE; the bitmask design (18.19) is CLOSED by
   measurement 2026-11-06 (`a698b0e0`): formulas average 1.3 clauses of 1.26 literals and all
   formula work is under 1% of a run, so per-scope bitmask indices (a change to how formulas
   name dimensions) would buy under 1%; the hotspots were `maxlev` and `lookup` in `lang.c`,
   fixed under A9.
5. DONE (`07053c5`): node arena marked and released per compile; cells and slots from
   per-thread slabs returned between readings.
6. DONE (`e0f27ef`): records and case tables interned by structure so equal bodies have one
   key. "Indices at key time" is not specified further in this document; records are keyed
   by their interned pointer (`K_LAM` over the record and its environment's keys, `synkey`).
7. CLOSED, not landed (`937fc2e`): the skip saved 1.8% and made counts schedule-dependent;
   the schedule-independent form needs a dimension-free bit on every wire, a design change
   not worth 1.8%. Two key bugs found under it were fixed (interval/face keys follow their
   payloads; a reference with a pending substitution names the copy by the box's key).
8. ★ Structural: work proportional to result size for terms with open dimensions. Measured
   (`e32c39b`): g8 is 1,135,531 steps for 2,346 cells, 484 per cell; the mechanism is
   `%trans-hcomp-D` distributing the transport into every tube side as soon as the
   composition exists, after which endpoint substitutions select one side per layer and
   erase the rest, multiplied through 20 layers; `delay` on the sides does not fix it (the
   held sides accumulate substitutions and lose sharing). **Proposal (section 18.16):** the
   transported composition becomes a formal value `%transH` that reduces only when a
   substitution makes a face true (one side transported) or a reader demands one layer.
   Essential; this is the property, and the original Brunerie number is one measurement.
   **Status (2026-11-07):** rounds one to four landed (18.16); the three remaining pieces were tried and
   withdrawn with their numbers (18.16, A8c); keys for path λs and systems were then built and
   withdrawn (A8d: the compositions whose inputs can all be named never repeat — 39 of 312 at the
   corner, 11 of 285 in g8, all distinct — and the keys alone cost 1–4% steps). What is left is back
   pointers from wires to producers (for bounds on unbuilt values), outside the net as it is.
9. DONE (`15625ce`, `91c128b`): interned symbols, free names kept per node, one desugaring,
   kept arena chunks; the compiled-form cache beside the recorded forms. Cached library
   load 0.105 s; Brunerie end to end 0.35 s, computation about 0.25 s. Further (2026-11-06,
   from A4's profile: `maxlev` 24%, `lookup` 9% of an uncached Brunerie run): `maxlev` reads a
   per-node set of the names a term may refer to (`lvs`, cached while the arena generation it
   was made in lasts) instead of walking the subterm at every node compiled; interned names
   keep their hash and their latest binder in their header (`sym_hash`, `lookup_i`, valid
   within a binder epoch — bumped at every pop); `compile`'s head dispatch compares interned
   pointers. Uncached Brunerie end to end 3.4 s → 2.7 s (callgrind: 15.8G → 15.3G
   instructions; what remains at the top is `compile`'s `strcmp` on `%%` constructor names and
   `checked_index`, and the net itself). **Elaboration memory (2026-11-07):** the elaborator's
   nodes were made outside any arena mark and never returned, so a large definition's rounds (its
   terms, the holes' guesses, the verdicts read back) stayed resident: `lib/foproof.cub`'s data
   mirror took 3.5 GB, 300–420 MB for each of `ds-let` and `ds-call` alone, the same with the store
   holding them (the elaboration runs before the key is known). `x_checked` now elaborates inside a
   mark, copies the settled type and term out (`el_copy`) and releases it, the node-keyed tables
   (hole tables, λ-head reductions, case motives, tick wraps, λ types) cut back to what they held
   before; a free-name set cached on a node (`fvs`) is valid only in the arena generation it was made
   in, as `lvs` already was. The same line peaks at 1.6 GB (`tests/foproofd.cub`'s), the first-order
   line at 0.5 GB (3.4 GB before); every output identical, step counts included.
   `CUBICAL_NOELMARK=1` turns it off; `CUBICAL_ARPOISON=1` frees every released chunk with a fixed
   mmap threshold, so a node kept past its release faults (the data line, the elaboration, codata,
   check, prim, guarded, cert, `--serve` and error suites and the typed port run identically so).
10. Measured 2026-11-06 on a 2-core container (`CUBICAL_NOCACHE=1`, end to end): 2.6 s at 0
    workers, 3.6 s at 1, 2 and 4 (no gain on two cores; the number's own computation is about
    0.3 s of it, the rest library checking); 619,852 steps, −2 at every count. Agda is not
    installed there (no package installs in that container), so the same-machine comparison
    is still open. Brunerie's original number at 1–16 workers, and Agda measured on the same machine (doable;
    it is a measurement, on the builder's queue). Demonstration. **10b. DONE (`546bdb8`):**
    `experiments/brunerie/typed.cub`, a signature on every definition, 229 checks; −2 in
    3.16M steps (untyped 1.26M; std's path algebra and the generic set-eliminators are the
    difference). With 18.17 landed (`c186b81`) no primitive is left: `rec2/3/4` and `surfc` are checked
    definitions, −2 in 1,349,493 steps. Four elaborator gaps closed on the way (λ binder
    types kept and used; λ arguments inferred against a known domain; `B x` against `T` solved
    as a pattern; errors show the term's own shape). Two notes from then, settled 2026-11-07: a
    definition named as a loaded constructor, which was read as the constructor, is refused,
    typed or untyped (`tests/errors/ctor-name.cub`, `ctor-name-u.cub`); a primitive without
    computation used as a value stands as a neutral, applied or not (`tests/elab.cub`'s `frob0`).

### B. Compilation by transport
Specifications for the ★ items are section 18 of this document, each at revision 2 after a
critical pass, with syntax, checker rules, acceptance tests and size. The
builder took B11 and B12 together (shared field-annotation syntax), then B13 and F34; none
touches `net.c` or the composition clauses. Corrections from the pass that bind the plan:
erasure is by computed usage, never by a binder's type; the checker's decision runs on the
net (only code-to-code programs compile); representation choice is a schedule, not part of
identity; linear values are introduced only in `primitive` definitions.
11. ★ DONE (`0729899`, 18.1): multiplicities, static side: usage computed after each
    definition checks, erasure by usage, linear introduction only in primitives, the errors.
    The dynamic side is structural (DONE).
12. ★ DONE (`01c839c`, 18.2): representation equivalences as a construct, the primitive
    types (floats trusted), `Bytes n` with proved accesses, layouts, `#trusted`. Quotients by
    canonical forms: a `represent` whose `to`/`from` are the canonical-form maps — DONE
    2026-11-06 (`tests/prim.cub`): `ZQ`, pairs of naturals modulo `a + d ≡ c + b` as a set
    quotient (a path constructor for the relation, a squash for setness), represented by `Int`:
    `to` the difference's normal form (`subN`, respecting the relation by cancellation),
    `from` the canonical pair `(n , 0)` / `(0 , n + 1)`, `from-to` by elimination into the
    family of propositions (every pair related to its canonical one, `zq-canon`); `#layout
    ZQ` is Int's. Found on the way: `#fo` crashed on any constructor applied to a field
    (`lib/stage.cub`'s `fo-list` read its tail as a code since it landed); fixed. As for
    `Tr2`, `#fo` of a function on the quotient stays `not-fo dim` after the representation
    (its body holds the relation's branch) until the specializer transports it along the
    representation.
13. ★ DONE (`1c5282a`, `lib/stage.cub`): `first-order T c` with the reasons dim, dep-type,
    higher, kan, reflect, mult; `fo-normal` on the normal form; the stage/verdict/strictness
    types as data; `#fo`. Schedules as values: `lib/specialize.cub`'s `Sched` (the rewrite
    bound) and `lib/code.cub`'s stage types; the specializer calls `first-order` (item 15).
14. First-order intermediate form. Essential. Built as a library: `lib/fo.cub` (the
    form, its evaluator, closure conversion, structural recursion, the byte layer),
    `lib/lower.cub` (from codes, with the data table's shapes), `lib/repr.cub` (the
    representations consumed). **Unboxing by representations and layouts (2026-11-07):**
    `program-rep reps fuel c` (`lib/repr.cub`) consumes every declared representation in a whole
    program (B12 §6): each definition lowered from its code as written (`%gcode n`, new: the
    checked definition's desugared term kept and quoted on demand, so calls stay calls — the
    normal form had unfolded `plus` into its recursion, and an op line had nothing to replace);
    a call of an operation with an op line becomes its native counterpart (aliases chased:
    `plus` → `plusw` → `P-add`); a constructor of a represented type its representation (a closed
    tree computed now through `to` on the net, an open one `to` at run time with its represented
    fields read back by `from`); a recursion on a represented value recurses on `from` of it, the
    branch's represented fields read through `to`; `to`, `from` and what they call stay as written.
    The table is `%rops d = some (to , (from , [(op , native)]))` (new, filled by `represent`'s op
    lines through `(repops …)`). In the printers (`lib/emitt.cub`, `lib/emitu.cub`), a field of a
    type represented by `Word` is a word (`cshape-r`, as its layout `lword` says): a record with a
    `Nat` field is a struct of two words passed by value. `run.py` programs `rep` (plus as `v0 + v0`,
    a list of Nats, a case through `from`: 89) and `rep-rec` (a record by value: 44) under every
    printer; boxed/UNBOXED/IMAGE/X86 all ok. What has no op line is correct and slow (a case on a
    `Nat` costs `from`, B12 §6's W-slow-op); **Declared signatures (2026-11-07, fork B2):** `emit-prog-d m defs` / `emit-lib-d m defs`
    (`lib/emitu.cub`) type every definition that comes from the store (global g is definition g + 1 in
    `program-at`/`program-rep-at m`'s numbering) from its checked type: `global-sig`'s FT, each
    parameter and the result a word (Word, Int64, Float64), a pair of such, or a data type (flat by
    value, pointer data a typed pointer, else boxed), and by use where FT says nothing (ftany, byte
    strings, functions) or its arity is not the definition's (an erased parameter). `run.py`'s
    `sig-decl` and `sig-use` print the same program both ways: a parameter only returned is
    `uint64_t` declared, `V *` by use, and a record only passed through is `T<d>` by value declared,
    boxed and unboxed around the call by use (both 19). Not changed: `emit-prog-u`/`emit-lib-u`
    (and so `#native`) still infer by use, since `ft-of` reads a represented type as its
    representation and is right only for programs with the representations consumed (`program-rep`);
    the entry and lifted closures have no declared type and are typed by use.
15. **Status (2026-11-06):** built — `lib/specialize.cub` normalizes, rewrites along the store's theorems under the terminating discipline (`bd21c486`: oriented rules to the fixpoint, unoriented under the bound), erases by `%gusage`, runs `first-order` and honours `Sched` (`36b9e30b`). 18.5 step 7: the per-family statements `eval-fo (lower (code f)) ≡ f` are read by computation in `tests/fo.cub` (`62c669fc`; `#same`s, since a checked path may only mention checked definitions and the first-order libraries are untyped); **the path assembled from the rewrites, for an arbitrary term of the word fragment, is built (2026-11-07, `lib/foproof.cub`):** the fragment mirrored as checked data (`WC`: literals, variables, addition, subtraction, successor, let, word recursion; `FW`: the forms `lower` makes of them, the counting loop of `lower-welim` included) with checked evaluators (`wc-ev` as `eval-g`, `fo-ev` as `eval-fo`) and lowering (`lower-w`); one lemma per form, proved once (`lower-sound`, the loop by `fw-go-rec`: counting up from `rec i` for n steps reaches `rec (i + n)`); verified rewrites as values (`VRule`: a function on codes with its soundness; `zero-vrule` and `fold-vrule` built from the store's `plusw-zero-l`/`plusw-zero-r`/`plusw≡wadd`), applied bottom up with soundness by congruence (`rw-bu-sound`); `spec-path r c ρ : fo-ev (spec-w r c) ρ ≡ wc-ev c ρ` for every code and environment, and for a term, `ex-path x : fo-ev (spec-w std-vrule ex-c) (envx x) ≡ ex-f x` (a checked definition; the rules fired: 0 + … gone, 2 + 3 folded). The bridges (`wc-of`, `fw-of`, `fw-to-fo`, untyped) read real codes and forms into the mirror and back: `fw-of (lower c) = some (lower-w (wc-of c))` and `eval-fo` = `fo-ev` on the readings, so the untyped `lower` is the checked one on them; `run.py` program `proved` compiles the checked pipeline's output under the boxed, unboxed and x86 printers (13). Data, closures and calls (2026-11-07, `6f57a2aa`): the mirror grows constructors with word fields, a two-way case, λ with a table index, application and calls to a global table; `dconv` (closure conversion) is checked and `dc-sound` proves the converted program means the source for every fuel, code and good environment given a right table (decided per program by `fc-eq-sound`, `dtok-auto`); `dx-path` for an example with all four, taken to the first-order form by `dc-defs`, compiled boxed and unboxed (`run.py` `proved-data`, 15). The specializer consuming the verified rewrites: `specialize-proved` (`lib/foproof.cub`) lowers a normal form whose body is in the word fragment by `spec-w std-vrule` (justified by `spec-path`) and falls back to the untyped `specialize` elsewhere, tagging which was used; on the word acceptance programs (and `plusw x 3` through the store) it is the verified one and its residual computes what the untyped one's does. **The bridge from `Code` into `DC` and the byte layer in the mirror (2026-11-07):** `dc-prog` reads a normal form into the data mirror — word literals and variables, word and `Int64` addition and subtraction (by their bits), successor, let, constructors and cases of one data type of one or two constructors each with two word fields, λ (numbered, threaded), application, a global applied to one argument (its quoted value read too, transitively: `dc-globals`), and the byte layer at 8 bytes (`dcby` a fresh string, `dcld`/`dcst` at a literal offset that is a multiple of 8, where the cell model `dvb` is exact; the `Int64`/word conversions the identity on bits); `DV`/`TV` grow `dvb`/`tvb` (a string as its cells by offset), `dsev`/`dtev`/`dconv`/`DTok`/`dgood` their clauses, `dc-sound` its cases (`ds-ld`, `ds-st` from `dv-ld-ok`, `dv-st-ok`), `fc-eq-sound` and `dtok-auto` theirs. `specialize-proved` takes the word route (tag 1, `spec-path`), else the data route (tag 2: the reading's tables decided by `dtok-chk`, so `dc-sound` holds of it through `dtok-auto`, converted by `dconv`, taken to first-order definitions by `dc-prog-defs`), else the untyped `specialize` (tag 0). `lib/foproof.cub`'s readings: `pq-sum`, `pq-app` (a closure chosen by a case and applied), `pq-mk` (a constructor built), calls read from a code as written (`dc-c4`: two globals), lib/region.cub's `two` as its normal form (`by-f`: two stores and two loads, 41 at 20), a load at offset 4 refused to the untyped route; `run.py` `proved-spec` and `proved-region` boxed and unboxed. (The data mirror, its rewrites and `specialize-proved` are `lib/foproofd.cub`, loaded after `lib/foproof.cub` and tested on their own line, `tests/foproofd.cub`.) **Verified rewrites over the data mirror (2026-11-07):** a rewrite on the converted form is an `FRule` — a function on `FC` with its soundness as refinement (`fr-le a b`: `a` undefined at this fuel, or `b` the same value; a closure's environment pointwise), since a rewrite may make a form defined at less fuel than the source: `fr-mono` (more fuel and a refined environment refine the value), `fr-trans`, `fr-then`. The rules: `std-vrule`'s two ported (`fold-frule`: a primitive on two literals folded; `zero-frule`: `0 + x`, `x + 0` gone, from `wadd-zero-l`/`wadd-zero-r`) and the byte layer's forwarding (`load-frule`: a load from a store at the same literal offset is the stored word, at another literal offset the load from the string below, through any chain of stores — lib/region.cub's `rd-wr` and `rd-wr-disj` in the cell model); `std-frule` their composite. Applied bottom up under every construct (`fr-kids`, the lifted λ table and the global table rewritten entry by entry, `fr-tab`), sound by congruence (`fr-cong`, `fr-bu-ok`: at every fuel, in every refined environment); `fr-dc-path r G T GT …` composes `dc-sound` with it: for a program read into the mirror with right tables, the rewritten conversion refines the source at every fuel for every good argument (`pq-rpath`, a checked instance). The data route prints the rewritten forms (`dc-defs`, `dc-lifts`), so `specialize-proved`'s tag 2 is justified by `fr-dc-path` at `std-frule`; readings: `0 + …` under a case removed (`pq-z`'s definitions equal `pq-id`'s), the byte program's two loads forwarded through the stores to `w + (w + 1)`. **What `lib/rules.cub`'s engine still does unverified:** every specialization whose normal form is outside both mirrors, i.e. tag 0 — the untyped pipeline (`normalize`, rewriting along the store's marked theorems by matching, `lower`); the theorems it applies are checked, their application (matching, the instantiation, the congruence) is not. On `tests/fo.cub`'s programs (`tests/foproofd.cub`, copies under their own names): `addk`, `factw` and `fibw2` take the word route, `bump` and the store-then-load on a parameter string the data route. **A pair through a word recursion (2026-11-07):** `WC` grows `wcrec2 w z1 z2 s1 s2 sel` — a word recursion whose accumulator is a pair of words (`P2W`, its own data type so a stuck evaluation shows an inferable constructor), the step's body seeing the components as variables 0 and 1 and the counter as 2, one component picked; `FW` grows `fwloop2` (`fw-go2`, the counting loop over the pair), with `fw-go2-rec`/`fw-go2-0`/`fw-go2-ext`, `lower-sound`'s case, `rw-bu-sound`'s through `rec2-cong`, and a clause in every rule; `wc-of` reads `fst`/`snd` of a `cwelim` whose start is a `cpair` and whose step is `λ k p. cpair b1 b2` with `p` used only through its projections (`wc-p`); the printer (`fw-pr`, a variable map, the identity on the old forms) gives `lower-welim` over a pair opened by two lets, the component projected. `fib-path : fo-ev (spec-w std-vrule fib-c) (envx x) ≡ fib-wp x` is checked (the mirror's recursion over `P2W` simulated against the term's over `Word × Word` by induction, `fib-sim`); fact at 6 is 720 and fib at 30 832040 through the verified pipeline. **The whole first-order fragment in the data mirror (2026-11-07, fork C6):** `DC` (and `FC`, `DV`, `TV`) are rebuilt so that every form `lower` gives a first-order meaning is a node: `dcp p` any binary primitive by lib/fo.cub's numbering (`dprim2`: word addition, subtraction, multiplication, `eq`/`lt` answering 1 or 0; Int64's and Float64's on the words of their bits, the machine's operation between the conversions as the evaluator clause), `dcp1 p` the conversions that change bits (`dprim1`; the others are the identity on bits and no node), `dcif` (Bool is 1 and 0, a case on it an if), `max` as a let and an if, `dcu`/`dcpr`/`dcfst`/`dcsnd` (pairs and unit), `dccon d k x` a constructor of any data type whose fields are a tuple and `dccase s alts` over a chain of alternatives (`dcalt k n b r`: tag k, its n fields pushed), calls `dccall g m a` of any arity (one argument, a tuple of them, or unit), and the byte layer at widths 4 and 8 and any literal offset — a string is its stores over a base (`BL`: its length and bytes, fresh zero or a parameter's), a load at the latest store's width and offset is its value (truncated to 32 bits at width 4), one disjoint from it reads below, one overlapping in part byte by byte, little-endian (`bl-bytes`, by a bit scan, the machine having no division). `dc-sound`, `fr-mono`, `fr-cong` and every rule have a clause for each (the case through `dv-alt-ok` / `fr-alt-le` by induction on the chain, the push of fields through `dv-push-ok` / `fr-push-le`); `fr-le` orders constructor values and pairs field by field. The rules: folding every primitive (`fr-fp`, `fr-fp1`), `0 + x` / `x + 0`, forwarding a load through stores at its width with disjointness decided on literal offsets (`fr-l1`; a same-width same-offset load is the stored value, truncated), and `proj-frule` (a projection of a pair written out, an if on a literal). `fc-eq`, its soundness and `dtok-auto` are generated over the 21 forms (`tools/gen_foproofd.py` prints them; each `fcs-` lemma is a case over the forms). Elaborating a composition of paths whose endpoints are inferred read the evaluators' stuck cases back at a cost that grew past memory with 21 forms (3 GB on one lemma); `dtrx` takes the endpoints given. The bridge (`dcr`, untyped) reads any data type (path constructors aside), Bool as an if, every primitive `lower-prim` lowers, globals applied to any number of arguments (erased ones unit, a partial application η-expanded by λs, extra ones applied), and lifts a case on a type with recursive fields — the hypotheses `rec` — and a word recursion to definitions of their own (keys from `dc-lbase`), each called with the scrutinee (or the loop's state) and every variable in scope as a tuple, its body unpacking them (`dc-unpack`), the hypotheses (the next iteration) calls of the same definition; the printer (`fc-to-fo`) gives a tuple's components as a call's arguments or a constructor's fields, a definition of m parameters rebuilding its tuple, Int64/Float64 operations between `P-i64ofw`/`P-wofi64` and `P-f64ofbits`/`P-f64bits`, a 32-bit truncation as a 4-byte store and load, and main's Int64/Float64 parameters and result converted at entry (`dc-wrap-main`, from the type). Checked paths per form (`dq-path-if`, `-max`, `-case`, `-pair`, `-num`, `-call`, `-bytes`, by `dc-sound` at every fuel and argument). Readings (in `lib/foproofd.cub`, route and value): comparisons with an if and max (`dq-c1`), two data types of 0-, 1- and 3-field constructors (`dq-c2b`), a list's sum by structural recursion (`dq-c3`, `dq-c3b`), a word recursion casing on data (`dq-c4`), Int64 and Float64 arithmetic and a Float64 comparison (`dq-c5`–`dq-c7`), calls of three arguments and of none and a partial application (`dq-c8`, `dq-c9`), a pair parameter and a pair result (`dq-c10`, `dq-c11`), a partially overlapping load and a 4-byte store and load (`sp-b2`, `sp-b3`), a case on Bool (`sp-f4`) — all on the verified route (tag 2, or tag 1 where normalization leaves a word program); `run.py`'s `proved-cmp`, `proved-tags`, `proved-list`, `proved-loop`, `proved-num`, `proved-float`, `proved-calls`, `proved-bytes`, `proved-bytes32`, `proved-pair` compiled boxed and unboxed. Found on the way, and fixed: the untyped route read a byte offset (a `Nat`) as a data value, which its loads took for 0 (`by-g` there answered the word stored at 0; the mirror answers the bytes at 4) — `lower-off` now gives a numeral as its word and any other offset as `fromNat` of it, and the bridge reads a variable offset the same way (`dcr-off`; `by-v`: a store at a `Nat` parameter, a load at 8, 77 at n = 8 and 0 at n = 0 on both routes); `lower`'s `wop-prim` lowered `max` as addition (max 3 9 ran as 12) — now `lower-max`, a let and an if, as the mirror reads it; `lib/emitu.cub`'s `box-pr` (a pair boxed into a `V`) declared its temporary as a `V *` and read its fields through `).f0`, so no unboxed program with a pair passed boxed compiled (`proved-list` was the first) — it holds the pair by value now. `%blen` (a byte string's length, from an untyped code; a checked `blen n b` is its index `n`) is read by the bridge as `dcblen` (`sp-bl3`). X86: `run.py`'s `proved-fib` runs fib 90 from `specialize-proved`'s word route as machine code. The line's peak was 2.37 GB resident (`CUBICAL_NOCACHE=1`; 1.45 GB since `and-br`, below). **What still takes tag 0:** only forms outside the first-order fragment, each of which `lower` itself turns into a call into the net (`fnet`) — dimensions, paths and path constructors, universes and types as values, codata and copatterns past `cthunk`/`cforce`, the 1- and 2-byte accesses, `ptr-of` and `region-at`, primitive operations `lower-prim` does not number — nothing else: every form `lower` gives a first-order meaning (literal or variable offsets, `%blen`, `max`, every numbered primitive, any data, calls of any arity, pairs) is read by a mirror. UInt32 values are words without wrapping, as `lower` has them. **The last link, from the mirrors to the first-order form (2026-11-07, fork C7, `lib/foproofq.cub`):** `FQ` is lib/fo.cub's `FO` as checked data (`lib/foproof.cub`), `fq-to-fo` prints it constructor for constructor, and both printers are checked functions into it (`fw-to-fq`, `fc-to-fq`; `fw-to-fo`/`fc-to-fo` are `fq-to-fo` of them, and the untyped printer they replaced, kept as `fc-to-fo-ref`, prints the same on every reading). `evq`, a checked evaluator by fuel over `KQ` (words, Int64 and Float64 by their bits, constructors, closures, λs, byte strings as the mirrors' `BL`), is FO's reference meaning: `tests/foproofq.cub` reads it against `eval-fo` construct by construct on every form the printers emit (literals, variables, let, every primitive `kn2` lists, the conversions, constructors, switch, projection, if, λ and closures applied, calls, the counting loop, the byte layer at widths 8 and 4 and its length). The printers proved right: `q-sound` (at every fuel, the converted form's meaning `dtev` is refined by its printed form's, every converted term meeting `qtok` — widths 4 or 8, known primitives, a constructor's fields a tuple, a closure made at its λ's depth and named by its key, a call at its definition's arity — in every refining environment, the printed form spending fuel only at calls, closures applied and loops), `fw-sound` (the word mirror's meaning is what the printed form computes whenever it ends). Composed with `fr-dc-path` and `spec-path`: `q-path-n` (the data route, a main of any arity: what the source computes at every fuel and argument the printed form computes) and `fw-path` (the word route). Every `run.py` proved program's path to the form handed to the back ends is a checked definition (`tests/foproofq-progs.cub`, generated by `tools/gen_foproofq.py` from the machine's readings: the reading written out as checked tables, `DGOK` and `QGOK` proved case by case by `if-all`, `q-path-n` instantiated; `#same`s tie each instance's table, printed, to the table `specialize-proved` hands over, entry by entry, and `evq` at run.py's arguments gives run.py's answer), and `ex-qpath`, `fib-qpath` for the two word programs. Making the line fit: the checker returned the solutions of one spine of implicit arguments per run, and every rerun compiled the definition anew, its records interned for good — a lemma of 50 such spines (the generated `fcs-` lemmas, one `Empty-rec` per constructor) took 50 runs and kept every compilation; the branches of a split are checked against goals of their own, so `lib/check.cub`'s `and-br` checks every branch past an elaboration and returns all of their solutions together (the data mirror's line went from 269M steps and 2.37 GB to 85M and 1.45 GB). The line's peak is 2.39 GB resident. **The C link (2026-11-08, fork C8, `lib/foproofc.cub`):** `CA` is the C subset `lib/emitc.cub` prints, as checked syntax — `mkw(w)`, locals `v<l>`, the runtime's operations, a block `({ V *v<l> = a; b; })`, `mkc` cells, a field `x->f[k]`, a switch over the cell's tag with its alternatives' fields declared, the word ternary, closures `mkc(999999 + g, …)`, `apply`, calls `f_g(…)`, the state loop — and `evc` its meaning by fuel on C's own values (`CV`: a word cell, a constructor cell, a closure cell, a byte string) with named locals under C's block scoping (`CS`). The subset is narrower than C and `evc` stricter than C at stated points (the file's header lists them; at each `evc` has no value, so nothing is claimed): the runtime's operations at their specification (`c-prim`, the mirrors' machine functions), allocation never failing, an operation on a cell of the wrong shape, a switch matching no case or with another field count, a loop body answering a cell of another shape, `apply` on a key with no dispatcher case or from 1000000 up or with another capture count, a local read out of scope; locals are never assigned but a loop's state (its rebinding at each turn), no address is taken, cells are immutable (the proved route emits no in-place store). `fq-to-ca` is the checked translation (a term, a list, alternatives; locals named by depth, `ca-lvl`), `ca-ok` its precondition (a λ as a value is outside the subset: no printer on the proved route emits one). `ca-sound`: at every fuel, every term meeting `ca-ok` in a scope refining its environment (`Corr`), the C's value refines `evq`'s (`CR`), given a table whose C functions are the translations of its definitions — by one induction over the term (`pstep`) inside an induction over the fuel for calls and loops (`coks`: `CallOK`, `LoopOK`), the primitives one lemma each (`cpq-`/`cpc-`/`cpr-`, `cr-ld*`/`cr-st*` for the byte layer at widths 8 and 4, Int64/Float64 by their bits). Composed: `ca-path-n` (the data route: what the source computes at every fuel and argument the first-order form computes, refined, and the C computes, refined) and `ca-fw-path` (the word route: whenever the first-order form has a value, the C's word is the source code's). `ca-to-c` (`ca-c`, `ca-prog`) prints `CA` construct for construct; it and gcc are what the path leaves outside it. The proved route emits its C through the link: `specialize-proved-q` gives the first-order definitions as checked terms (the word route's main, the data route's table), `ca-harness` adds a main calling the program's on its arguments, and `run.py` prints every proved program's boxed C as `ca-prog` of their translation (`QROUTE`; all 14 compile and answer as before). The primitive chain (`cpq-`/`cpc-`/`cpr-`, `cr-prim`) is generated by `tools/gen_foproofc_prims.py` from one table, byte for byte (`deb07bd1`). Per program (`tests/foproofc-progs.cub`, generated by `tools/gen_foproofc.py` over the instances of `tests/foproofq-progs.cub`): the table's keys as a list, its obligations decided over them (`keys-ok`, by `weq-sound`), `ca-path-n` instantiated (`P-cpath`), and readings that each C function of the table the route prints is the instance's and that `evc` computes run.py's answer; `ex-cpath`, `fib-cpath` for the two word programs (`tests/foproofc.cub`, with `evc` read against `evq` on every construct and primitive the printers emit, and a program printed through the link equal, character for character, to `emit-prog`'s). So on the proved route no untyped evaluator is relied on: `evq` is the first-order form's meaning, `evc` the C's, each step between them proved. **What remains unverified on the verified routes:** `ca-to-c`'s text (the printing of `CA`, construct for construct) and gcc on the boxed route; the unboxed printer (`lib/emitu.cub`, from the first-order form) and the x86 printer (`lib/emitx86.cub`) in full, which the proved programs also run through; the routes outside the first-order fragment (tag 0, above). `eval-fo` is read against `evq`, not proved equal to it (it is untyped); nothing on the proved boxed route uses it. The line (libraries, `lib/foproofc.cub`, both per-program files and both test files) peaks at 3.5 GB resident: the libraries it needs alone peak at 2.39 GB, and the growth over a file is retained, not transient (the node arena's chunks and the heap both climb definition by definition, `pstep` the largest step at +572 MB), so fitting it under 2.5 GB needs the checker to release what it keeps per definition. Previously open: open. Spec as written: Specializer as a term, applying marked theorems under a terminating discipline. **The unboxed and x86 links, planned (2026-11-08, fork C10; not built — the work is about 4,000 lines of checked proof, split here into six closed pieces, each with its check):** (U1) kinds for the first-order form: `KW` (a word, an Int64 or a Float64: a C `uint64_t`), `KB` (a byte string), `KC d` (a constructor of data type `d`, its fields' kinds from a per-program table `T d c i`, decided against every `fqcon`), `KA` (anything, boxed); a checked kind check `fq-kind T Σ Γ t` (signatures `Σ` for calls, closure targets all `KA`, loop states typed) and `kind-sound`: at every fuel, in an environment whose values have their kinds, `evq`'s value has the term's kind (or none), by the induction `pstep` uses — emitu's inference (`infer-u`, `use-ty`, first use decides) is not sound by construction, so the unboxed route must check its typing, not trust it. (U2) `CU`: `CA` plus what `lib/emitu.cub` prints — `uint64_t` locals, parameters and returns, `(a op b)` and `n<p>(a, b)` on words, the conversions `mkw(e)`, `e->n`, `box_d`/`unbox_d`, a pair `Pr<k>` by value with `.f0`/`.f1`, a flat `T_d` literal with `x.u.cK.fJ`, a pointer `T_d *` literal with `x->u.cK.fJ`, switches on `.tag`/`->tag`, the `for` loop with temporaries — its meaning `evu` on `CV` (a C value abstracted: `uint64_t w` ↦ `cw w`, a pair or a data value by value or through its pointer ↦ its constructor cell; a conversion the identity on a value of its shape, no value otherwise; a field written at a width narrower than 64 bits ↦ its truncation), `fq-to-cu` (emitu's printing as a checked function, the layouts passed as data), `cu-to-c` (construct for construct, the only trusted printing step). (U3) `cu-sound` (the `pstep` induction with U1's kinds discharging every conversion's shape), per-program paths generated as `tools/gen_foproofc.py` does, `run.py`'s `UNBOXED` route for the proved programs printing through `cu-to-c` (all 14 ok). (X1) `XA`: the instructions `lib/emitx86.cub` emits (its list in that file's header: pushes and pops of rax/rcx/the argument registers, `mov` of immediates, slot loads and stores at `[rbp − 8(s+1)]`, `add`/`sub`/`imul`/`cmp`/`sete`/`setb`/`movzx`/`test`, the arena stores and loads at `[rax + 8(k+1)]`, `add r15`, base-relative `call`, `add`/`sub rsp`, prologue and epilogue, `ud2`) and its two control shapes (the conditional `test; je; …; jmp; …` and the loop's back and end jumps) as structured blocks whose displacements are the encoded lengths; `evx`: registers and memory as finite maps (one memory: the stack below the entry's `rsp`, the arena from its pointer, disjoint and bounded so no address wraps; `evx` has no value past the arena's bound), fuel per call and loop turn. (X2) `fq-to-xa` on the fragment `x86-ok` accepts and `xa-sound`: a word refined by itself, a cell by an address below `r15` whose memory holds its tag and refined fields (preserved as the arena grows: stores only at or above `r15`), the stack discipline (an expression's code leaves `rsp` and every slot as it found them, its value in `rax`). (X3) `xa-to-bytes` (the encoder, `emit-x86`'s bytes construct for construct: the only trusted step with the CPU), `run.py`'s `X86` route for `proved` and `proved-fib` through it, their paths. Each piece's line must stay under 2.5 GB.
    Essential. (The discipline, usage and the criterion, once listed here as not yet applied,
    are applied: see the status above.)
16. **Status (2026-11-06):** milestones 1 and 2 built (`lib/emitc.cub`, `lib/emitu.cub` with recursive data as typed pointers `a8d9028c`, 22 harness programs under both printers); the JIT is `#native` through gcc (`e9b9c983`); `fthunk` in both printers as a thunk cell computed once by `force()` (`7a2c96c4`); 18.7's own encoder `lib/emitx86.cub` for the word-only fragment, run from executable memory by `experiments/fo/x86main.c` (`54be3050`, `X86=1 run.py`: six programs, fib 90 among them). Spec as written: C back end (gcc/clang, CompCert); JIT. Essential. Milestone 1 as a library:
    `lib/emitc.cub` prints the first-order form as C (boxed values, closures through a
    dispatcher, `ASSUME` for assumed facts), `experiments/fo/run.py` compiles and runs seven
    cases with gcc. WebAssembly, MLIR, circuits: optional.
17. **Status (2026-11-07):** the mechanism is `#native f n = prog` and `specialize-v` (`36b9e30b`); **cached by key (2026-11-07):** a `#native` specialization is named before it is computed — by its expression with each checked definition and data type replaced by its codebase key (F34, which covers what it depends on), the values written in it (the run-known values), the arity, `keep`, the checker's version (`chk_salt`, the binary included) and the libraries loaded before the file (an unchecked definition — the specializer, the printer — has no key; an unchecked definition of the file itself adds the file's text) — and a hit (`.cubical-cache/native/spec-<key>`, naming the library's hash) skips the specializer and the C compiler: `ok: native add5b [specialization cached by key, 0 steps, …]`; a second run of `tests/native.cub` hits on every compiled entry; another value (`add6`) is another key. **Re-entry while the program runs (2026-11-07, fork B2):** `#native f n at k = P` (core `(native-at f n k P)`) — f's first k arguments become known only at run time. f becomes a lookup entry: a call whose arguments are words calls the specialization kept under its first k values; when there is none (in this run or in the specialization cache by key, P's key text with the values), the call becomes a generated builder's, in which the net itself computes `P v1 … vk` (the C of the residual over the other n − k words, `emit-lib-u`'s) as a byte string (`bytes-of-list`, `lib/emitu.cub`), and the machine compiles it (once per text), enters it, keeps it under the values and calls it. The program is computed by the net as part of the run, not by a nested evaluation (a `readback` inside a reduction waits on the reduction it is in). `tests/native.cub`: `scaleat e b` (b added e times) at 1: `native at run: scaleat at #3 [specialized now]`, the residual `((0 + v0) + v0) + v0`; the later calls with 3, and those inside `use-scaleat`, use it; 4 is a second specialization; a call on a variable is the definition on the net; a second run reads both from the cache by key. **Workers and a bound (2026-11-07, fork B4):** the kept specializations of an entry live in at most `CUBICAL_NATIVE_AT_MAX` (1–256, default 16) foreign-table slots reserved when the entry is declared (the foreign table no longer grows while the program runs), read and changed under one lock; a call copies its slot under the lock and calls outside it, counted in the slot's in-flight count; a new specialization past the bound evicts the least recently called slot with no call in it (its library `dlclose`d; `[evicts f at #v]`), and when every slot is in a call it is called once without being kept; a builder's compile is under the lock. `tests/native-at.cub`: twelve values from one reading with a bound of 4, then again (each evicted value re-entered from the cache by key), with `CUBICAL_WORKERS=4` and under TSan. Not built: arguments other than words (represented or boxed values in the known or residual positions). Spec as written: Progressive specialization on run-time values, cached by key. Essential.
18. **Status (2026-11-06):** both directions for words, byte strings and constructors (`e9b9c983`, `680485ea`, `e5bbd429`, 18.8's status); `V_NATIVE` opaque handles from hand-written C (`#native … from`, type `Native`, finalized by `cubical_release`) and boxed results kept native (`#native … keep`); **represented values by their representation (2026-11-07):** a `#native` entry whose parameter or result has a type with a declared representation takes and returns the representation (the compiled program is `program-rep`'s: Nat as `uint64_t`); the call converts by `to` before and `from` after, and the definition it falls back to is the one over the representations (`to ∘ f ∘ from`, a generated `%natfb`), so both paths agree; from C, `cubical_call_data` with a type naming a represented data type takes the representation's value (a word for `Nat`) and reads a represented result out by `to` (`tests/native/main.c`: `plus 40 2` on Nat as words = 42). **Records by value (2026-11-07, fork B2):** a `#native` entry whose parameter or result is a record (one constructor) of word fields or fields of word-represented types is emitted by `emit-lib-u` as its flat C struct `T<di>` taken and returned by value; the machine appends a trampoline (`<entry>__tw(const uint64_t *a, uint64_t *r)`: the struct's fields as words in, the result's as words out) and opens the record into its fields, each converted by its field type's `to`, rebuilding the result's by `from`; the fallback (`%natsfb`) is the definition over the same words, so both paths agree (tests/native.cub: `swapPt`, `sumPt` on `Pt` of two Nat). Libraries are linked `-Bsymbolic` (every one exports `cubical_entry`, opened `RTLD_GLOBAL`). From C, `cubical_call_data` takes a type `"struct D"` as a pointer to the struct of D's fields (each a `uint64_t`, a represented field as its representation) and `cubical_call_data_r(…, "struct D")` returns the result as a malloc'd struct of its fields (`tests/native/main.c` over `tests/native/rec.cub`: `swapPt {3,40} = {41,3}`, `sumPt {41,3} 2 = 46`). Then not built: records of more than one constructor (the tagged union by value), nested records (a field itself a struct), fields of floating or 32-bit types (each field is a `uint64_t` word), and more than 8 argument words or 16 result words (all but the argument limit built by fork B5, below). A type represented by a record (`represent Box by Pt`) crosses as its own struct (the back end keeps it as itself: `program-rep` consumes representations by `Word`), but the program `program-rep` emits for it is wrong on its own: it routes the value through `to`/`from` and applies the field type's `from` (Nat's `toNat`, counting a tree) to fields that are already words (`f_104(mkw(v))` reads 0), so `bumpBox (box 1 2)` answered `box 10 0`. **Fixed (2026-11-07, fork B4):** a type represented by a data type has its `to` and `from` consumed like the rest of the program with the type itself unrepresented (`lib/repr.cub` `tb-del`, `rcv-convs`), so the record they make or take is in the program's form (a Nat field a word); a constructor is `to` of the record built from the program's fields and a case reads `from` of it, no field conversion either side (the typed C back end has one layout per data type, so tree and word fields cannot share it). `lib/repr.cub`'s `rbox` readings (12, 3; failing before), `run.py rep-box` boxed and unboxed (41). The first reading after a plain `#native` (B3's report: neither the compiled code nor the fallback) does not reproduce on this tree: every reading of `tests/native.cub` under `CUBICAL_SHOW_NATIVE=1` with unbuffered output has its call line (the missing lines were stdout/stderr buffering), fresh and cached libraries alike; `tests/native.cub`'s `probe` pins the first reading to the compiled answer. What does answer from neither is a definition compiled before the `#native` (`useprobe`): it keeps the definition it was compiled against, by design (the name is rebound, not the global); rebinding in place is not built. **The rest of the record crossing (2026-11-07, fork B5):** a record is described once from its data type (`crec_of`, src/lang.c): no parameters, path constructors or recursive fields; each field a word (`Word`, or a type represented by `Word`), a primitive at B12's widths (`UInt32`, `Int64`, `Float32`, `Float64`), or such a record (inline; a type represented by a record, its representation). Between the net and C the value is a run of slots: the tag when there are several constructors, then the constructor's fields (a nested record its own slots), padded to the widest constructor, each slot a word (a 32-bit field its bits). `#native` (emitted `T<di>` or a hand-written `struct D` in a `from` file, any member names) reads the entry's prototype, matches each struct's text against the data type (`cr_tw`: a tag word and a union of one anonymous struct per constructor, or one constructor's fields; `uint32_t`/`float`/`double`/`int64_t`/`uint64_t` for the scalars) and generates the trampoline from it (a switch on the tag both ways, `__builtin_memcpy` for the float bits); the definition opens each record by a case per constructor (the tag a literal, each scalar to its word: `%wofu32`, `%wofi64`, `%f32bits`, `%f64bits`, a represented field by `to`) and reads the native's raw result (built by `crec_result` in net.c from the slots, the tag choosing the constructor) back by a case per constructor (`%u32ofw`, `%f64ofbits`, …, `from`); the fallback takes the same slots and rebuilds each record by branching on the tag slot (`(if (%weq s #c) …)`), so both paths agree (under a binder the fallback's answer is `from (to n)` where the definition's is `n`: equal values, not equal readings). A parameter that is not a record keeps its kind (a `V *` boxed, a `void *` a handle), as a non-record result does (a `V *` result unboxed: `segsum`'s). From C, `"struct D"` is laid out as C lays it out (a field at its width and alignment, the union after the tag at 8; `crec_read`/`crec_write`), `%recin_D` rebuilds from the slots (branching on the tag; a tag naming no constructor answers NULL) and `%recout_D` reads the result raw. Results up to 32 slots. In the emitted program (`lib/emitu.cub`, `lib/emitt.cub`): a switch alternative's locals scoped to it (every multi-constructor case binding fields in two alternatives failed to compile: made here and, the same fix, in `bb6009f0`), a record whose fields are words or flat records with a field is flat (`flat-lay`; Seg holds two `T<Pt>` inline; Bool and other field-less types stay as before), its typedef after the ones it holds (`flat-by-depth`), boxed and unboxed through theirs; `t-fld`/`t-vptr` were not strings (a list applied to a numeral since `a8d9028c`: the typedef of any type with a data-typed or boxed field did not terminate, `emit-lib-u` on `Seg` ran out of memory). Tests: `tests/native.cub` `area`, `grow`, `segsum`, `segswap` emitted (failing before: the C compiler rejected `area`/`grow`, `segsum` ran out of memory) and `shape-area`, `shape-grow`, `seg-swap`, `px-mix`, `px-fbits` from `tests/native/rec.c` as probes (the definitions answer otherwise; seven readings failing before); `tests/native/main.c` over `tests/native/rec.cub`: `area`, `grow` twice, a bad tag refused, `segswap`, `pxmix {7,1.5,2.25,-3} = {8,1.5,4.5,-6}` (failing before). **Primitive fields in emitted programs (2026-11-07, fork C3):** `lib/emitt.cub`'s `cprims d c` gives each field of a primitive type other than `Bytes` its kind (wpty k + 1); `cshape-r` reads such a field as a word (it is one in the first-order form), the typedef holds it at its C width (`uint32_t`, `int64_t`, `float`, `double`, as `crec_of` lays it out), and a type is flat when its layout is (a primitive's `lbytes` now accepted) and every field is a word or a flat record (`fields-flat`: a `Bytes 8` or `Native` field, bytes to the layout but boxed to the printers, keeps the type boxed). `lib/emitu.cub` converts at every place a field is read or written — projection, switch, update, literal, `box_d`/`unbox_d` and the pointer forms — by `rd-prim`/`wr-prim` (integer casts; a float by its bits through `W`/`W4` and `F`/`F4`, `F4`/`W4` added to the runtime), so a list of `UInt32` is a typed pointer whose struct holds a `uint32_t`. `tests/native.cub`: `pxmix-e` against its fallback, `px-probe` (the identity over `pxmix-e`'s emitted program; the UInt32 at 2³² − 1 wraps to 0 in its field), `nflip`/`nflip-probe` (a sum of `Int64`, `Float64`, and `UInt32` with `Float32`), `pxk-probe` (Px inline in a record), `bbw` (a `Bytes 8` field stays boxed); `run.py` `prim-rec` boxed and unboxed (125; the unboxed source holds `struct { uint32_t f0; float f1; double f2; int64_t f3; }`). Not changed: inside the program a `UInt32` is a word that `u32add` can carry past 32 bits (the emitted `u32add` is a plain addition), truncated only when stored into a field. **An open term reads as the definition (2026-11-07, fork C3):** over represented values the converting call is its own definition (`%natcv<k>`, `from (native (to a…))`) and the entry is `(lam a… (%natrep cv g a…))`: the arguments are read whole (`O_DEEP`/`O_DEEPC`, a constructor's fields forced in turn) and, when every one is closed data (words, primitive values, byte strings, native values, constructors of such all the way down), the call is `cv`'s; an argument with a neutral part (under a binder) takes `g`, the definition itself, so `λ n → dbln n` reads as `λ n → dbl0 n` and not as `from (to …)` of it (`tests/native.cub`, three `#same`s under a binder, failing before). **More argument words and fields (2026-11-07, fork C3):** a `#native` entry takes up to `NATIVE_MAXW` = 32 parameters and argument words (`src/core.h`; the foreign table's kinds, the prototype reader and the struct code sized by it); above six parameters the call goes through the entry's trampoline, as a struct crossing does (a direct call covers at most six words). A record's constructors and fields have no fixed bound (`CRec`'s per-constructor arrays allocated to size); a record crosses by value while its slots fit 32 words, else boxed. `tests/native.cub`: `sum10p` (ten words, 77), `r6p` (a six-word record and four more words, 65), `r20p` (a record of twenty fields with a `UInt32`, 321), each failing before (refused: one to six arguments, more than eight words, more than sixteen fields). Found on the way: `CHKSRC` and `CHKTY` (a checked definition's desugared term and type, `%gcode` and the boundary's reading) grew without zeroing their new entries, so a definition that never set one read garbage (`gcode_build` dereferenced it: ASan over `tests/native.cub`, `to_code` at a 0xbe… address); the new entries are zeroed, as `CRECS`' were. ASan over `tests/native.cub` clean. Spec as written: Call boundary; C ABI both directions. Essential.
19. **Status (2026-11-06):** built, `ee5c04a2` (`lib/image.cub`, `lib/reimport.cub`; `IMAGE=1 run.py`: 4 headers through g++ -std=c++20, re-imported equal to the source up to erasure); parameterized data types exported as templates and read back (`13ae6b9f`: a type parameter a template parameter of the structs, a case a visit by template lambdas; a parameter at a value stays outside the fragment). **18.9's remaining constructs and 18.15's configuration (2026-11-07, fork C5):** records of operations as concepts, codata as classes, representations' classes, the module file, and the image specialized against a `Config` (its `Platform`: word width, byte order, alignment) with consumers' instantiations recorded statically and read back as the next specialization's `Config` (18.9's status); `IMAGE=1 run.py` runs twelve programs (`concept`, `codata`, `repcls`, `module`, `platform`, `config` new). Spec as written: The C++ image (10.1): a compositional translator over codes, one clause per source form
    (~30 forms), parameterized by the binding-time assignment; the fully specialized residue
    printer is its first milestone; the fragment's re-import for translation validation and
    for importing consumer code in the fragment. ~3,500 lines. Essential for adoption; the
    highest adoption value per line in the plan.

### C. Memory and state
20. DONE in parts: the byte layer (B12), validity facts as certificates (`lib/decode.cub`),
    `ptr-of`/`region-at` trusted and `#safe f` listing what a definition assumes
    (`1c5282a`). The Region threading idiom: DONE 2026-11-06, `lib/region.cub` — a region is
    `Bytes n` threaded as state, `RM n A = Region n → Region n × A` with `rret`/`rbind`/`rthen`/
    `rmap`, pointers as offsets with their proof (`Ptr n`, aliasing pointers are integers into
    the region, 6.4), `rd`/`wr`/`rmodify` the byte layer's proved accesses, `run-fresh`/`run-on`,
    `enter-at` (memory the program does not own, through `region-at`) and `raddr` (through
    `ptr-of`); the monad laws by computation, `rd-wr` and `rd-wr-disj` from the byte layer's
    laws; `tests/region.cub` (readings, laws at pointers, `#safe` listing `region-at` only for
    the program that enters foreign memory). Compiled: `run.py`'s `region` specializes a program
    over a fresh 16-byte region to the first-order form, its stores in place (`st64i`), under
    both printers. Found on the way: a byte string had no code, so the specializer's normal
    form of any term holding one fell back to the unnormalized code (`normalize`'s `fromopt`);
    `lib/check.cub`'s `q-bytes` quotes it as the fresh string and a store per nonzero word. Also
    found: `code t` in an untyped definition elaborates `t` without inserting implicit
    arguments (an explicit argument landed in the implicit's place, silently); since 2026-11-07
    it is refused with the remedy named, as any untyped application of a checked definition to
    its explicit arguments only is (`tests/errors/untyped-imp-code.cub`), so a program to
    specialize is written as a checked definition and quoted applied (`code (λ w → prog w)`).
21. **Status (2026-11-06):** built, `e27eda2c`, `02a4a615` (`fupd`, constructor reuse where the base is the node at hand, uniqueness by use, copy-on-write, in both printers). Spec as written: In-place update: whole values, through parts, copy-on-write, sending (6.4). Essential.
22. Started (`be7149d`): `Array n = Bytes (times n 8)` with `index`/`update` at a proof,
    `tabulate`, and `index-update` proved from the byte layer's laws (trusted primitives
    `load64-store64`), possible since stuck primitive operations became neutrals. DONE
    2026-11-06: `Fin n → Int64` as the interface — `Fin→Array` (`967e48e7`) with the operations
    carried across it, `apply-by` (application is `index`) and `fin-set-by` (the functional
    update `fin-set` is `update`) in `lib/prim.cub`, read in `tests/prim.cub` (`by-9`; `#fail`
    `by-bad`); `represent` takes a data type on the left, so a function type's representation is
    the library equivalence. Found on the way: byte strings never converted unless shared (the
    checker's `wbytes` case was `false`), so no path between two buffers built apart could
    check; now `%beq` compares contents (`O_BEQ`, false on a neutral). In place when unique:
    `lib/fo.cub`'s uniqueness pass covers the byte layer — a store whose base is fresh, or a
    let-bound / loop-state buffer whose other uses are reads before it (loads are reads, not
    holds), becomes `P-store64-ip`/`P-store32-ip`, printed `st64i`/`st32i` by both printers
    (no copy); a parameter or an unsequenced read keeps the copy (`st-par`, `st-beside`);
    `run.py`'s `inplace-bytes` fills an 8-word buffer in a loop with no copies. The element type
    is `Int64` (the byte layer's word). **Other element types (2026-11-07, fork B2):** `UInt32` on
    the 4-byte layer — `Array32 n = Bytes (times n 4)`, the Int64 development at width 4
    (`index32`, `update32`, `atab32`, `index-tabulate32`, `tabulate-index32`, `index-update-if32`,
    `Fin→Array32`, `fin-set-by32`) over two more trusted laws (`load32-store32-disj`,
    `array32-ext`); UInt32 is not equivalent to Int64 (8 bytes would hold values UInt32 cannot), so
    it has its own layer. Any `A` with an equivalence to `Int64` on the 8-byte layer, proved once:
    `indexV`/`updateV`/`atabV`/`aindexV` (from after index, update at to), `index-updateV`,
    `index-update-neV`, `index-tabulateV`, `tabulate-indexV`, `Fin→ArrayV A to from ft tf n`,
    `fin-set-byV`. `Float64` by its bits: new primitives `f64bits`/`f64ofbits` (the machine retags
    the word; `P-f64bits`/`P-f64ofbits` in the first-order form, the printers' `n32`/`n33` the
    identity) with their inverse laws trusted, so `Fin→ArrayF`, `index-updateF`, `fin-set-byF`; a
    type represented by `Int64`, by its layout: `tests/prim.cub`'s `Cents` (`represent Cents by
    Int64`), `Fin→ArrayV Cents centsTo centsFrom …`, read. In place: `updateF` is `store64` and
    `update32` is `store32`, so `run.py`'s `farray` (st64i, 7.0·3.0 = 21) and `u32array` (st32i,
    40 + 2) store without copies, boxed and unboxed. Not built: a represented type whose
    representation is not `Int64` (a record, or `Word`: Nat by Word is not equivalent to Int64).
    **Float32 (2026-11-07, fork B4):** new primitives `f32bits : Float32 → UInt32` /
    `f32ofbits : UInt32 → Float32` (the machine retags the low 32 bits; a Float32 value is its
    32 bits and prints as the single it is, `#f32:0.25`), their inverse laws trusted; the 8-byte
    development copied onto the 4-byte layer for any `A ≃ UInt32` (`indexV32` … `Fin→ArrayV32`,
    `fin-set-byV32`), so `Fin→ArrayF32`, `index-updateF32`, `fin-set-byF32` (`tests/prim.cub`).
    In the first-order form a Float32 is the word of its bits, as a UInt32 is, so both primitives
    lower to the identity and no printer changes; `run.py`'s `f32array` (st32i) boxed and unboxed.
    Not built: Float32 arithmetic (no `f32add` …; a computation on singles goes through its bits).
23. DONE (`967e48e7`, `lib/fiber.cub`; used by `6d098a67`'s state programs): a map factors
    through its fibers, `fiberEquiv f : A ≃ (b : B) × fiber f b`, the projection the first
    component. State programs as `A ≃ Σ (b : B), fiber f b` then projection. Valuable.

### D. The outside world
24. **Status (2026-11-06):** built as `lib/proc.cub` (Church form, handlers, worlds, replay), its guarded form `GProc` (`5b9de865`), `lib/session.cub`, `lib/choreo.cub` (`6d098a67`); projection as specialization against the role (2026-11-06,
`tests/choreo-spec.cub`): the global program `λ m → project-raw m g` over a three-party
choreography, specialized with the role a now value (`lib/specialize.cub`'s `residual`: 18.6 step
1, the schedule's st-now binders applied to the environment's codes — wherever they stand: a now
binder after run-time ones is specialized under their λs, `λ x → c x v`, 2026-11-07 — then
normalization and rewriting), leaves each role's local process as its residual, equal to the one written by
hand (roles 1, 2, 3; the residuals differ pairwise, run under a scripted world); left at run
time, the role stays a λ. Spec as written: The process library: interfaces, guarded trees, handlers, signature inclusion, push
    events, session types, choreographic projection (7.1, 7.3). Consequence plus library.
    Essential.
25. **Status (2026-11-06):** built, `7a865cac` (`Outcome`, supervisors by key). Spec as written: Outcomes as values (consequence); supervisors and restart by key (library). Essential.
26. DONE for the loop and the library STM (2026-11-06, `lib/world.cub`, `src/drivers.c`,
    `bf3ea4eb`); the hardware memory interface is DONE too (`lib/world.cub`'s `w-load` /
    `w-store` / `w-cas` / `w-fence` at acquire / release / acquire-release / sequentially
    consistent, on addresses from `ptr-of`, run by `src/drivers.c` with `__atomic_*`;
    `tests/world.cub`). Relaxed atomics' memory model stays a section 15 thread.
27. **Status (2026-11-06):** `foreign` built; `world "name" = handler` is `lib/proc.cub`'s named worlds and registry (`96971cee`) with its surface form (2026-11-06, `src/syntax.c`): `world "kv" = kv-world` checks `world-kv : Named S St = (id , kv-world)` and `world-kv-id : Word = id`, id the name's FNV-1a hash, the type from the handler's signature or an annotation (`world "n" : World S St = term`); `tests/proc.cub`. A handler is a checked term, so nothing joins `#trusted`; a world implemented by `foreign` functions is listed there through them. Spec as written: Started (`1c5282a`): `foreign name "symbol" : T` as a trusted primitive over primitive
    types (a C function of the program or a shared library), listed by `#trusted`/`#safe`.

### E. The language people and models write in
28. DONE (started `1c5282a`, 2026-11-06): instance arguments `{{e : C A}}` by instance search,
    records by field name and update, parameterized modules with `open`, indexed families by
    fording, the interval-literal fix, holes in composition lines, generalized variables,
    implicits through reducing hypothesis types, the hypothesis at the field's own index for
    recursion on a family, the goal rewritten by the index equation, index unification with
    absurd cases left out (18.12). DONE; findings (iv) and (vi) of 18.12 (mutual structural
    recursion; prenex levels) stand by design.
29. **Status (2026-11-06):** built, `fd188bf6` (`tools/agda2cub.py`, `tests/agda/` checked by `make test`). Spec as written: Translator from agda/cubical (Agda's `transp`/`hcomp` to CCHM `comp` by the standard
    interdefinability; Mathlib is not a source: its quotients, funext and propext are axioms).
    Essential for value.
30. DONE (2026-11-06): incremental checking is the store (a definition accepted by the
    current checker is not checked again, F34) and the module cache; the editor protocol's
    first form is `--serve` (`src/lang.c`): after the files, blocks of surface declarations
    from the standard input, each ended by a line `.`, processed as a file would be, a fatal
    error ending the request and not the machine, each reply ended by `.`; retraction is
    `#retract f` (2026-11-06): the checked definition is hidden, the name is declared again,
    dependents keep the definition they were checked against (`tests/serve/retract.in`). Errors
    print on stdout as well as stderr with the term at fault (`explain`). Step profiles:
    `CUBICAL_PROFILE=1` (whole run) and `=show` (per reading), `%gsteps` per definition.
    A debugger by replay: a process's run is a value replayed by `lib/proc.cub` (D25's
    recording); stepping the net itself is `CUBICAL_TRACE` (a trace file). Positions: a failed
    check prints its declaration's `file:line` (`decl_pos`) and the term at fault. An LSP
    server (2026-11-06, `tools/lsp.py`, Python stdlib): JSON-RPC over stdio, full-document sync,
    diagnostics from the verdicts, one `--serve` per open document; the document is split into
    top-level units sent one block each (a verdict's `<serve>:N` is line N of its unit), and on
    a change the units after the first that differs are retracted (`#retract`) and sent again
    in order, so dependents are checked against the new definition; a data type, `variable`,
    `mutual`, `module`, … among them restarts the machine. Hover shows the signature written in
    the document (the machine prints no types on request). Parse errors under `--serve` now end
    the request, not the machine (`x_die` through `die`), and a fatal error after a checked
    definition in the same block no longer exits (the checker's and the printer's handlers
    restore `--serve`'s). `tests/lsp/test_lsp.py`: an error reported and cleared, a dependent
    re-checked, data changed, a parse error at its line, hover, shutdown. Then (2026-11-06):
    `#retract D` of a data, codata or record type (the type, its constructors and fields — a
    record's projections — renamed out of reach and named in the reply; what was checked against
    it holds it by index and stays as checked, a declaration of the name again is a new type), so
    the LSP retracts data units like definitions instead of restarting; `#type e` (the type the
    checker infers, `type: e : T`, surface syntax; the reason when there is none), `#scope` (the
    names in scope on one line) and `#where name` (a checked definition's or data type's
    declaration, `file:line`, kept per definition at check time); the LSP's hover is `#type` (the
    written signature as fallback), completion from `#scope` and the document's names,
    go-to-definition to the document's unit or the library's `#where`, incremental (range) sync in
    UTF-16 units; verdicts under `--serve` print in surface syntax. `tests/lsp/test_lsp.py` (23
    steps) and `tests/serve/data.in`. A hover on a local variable (2026-11-06): a clause's pattern
    variable or an implicit parameter is typed from the definition's own signature, the patterns
    matched to its binders by position (`{x}` patterns and binder names for the implicit ones),
    in `tools/lsp.py` (`local_type`, `telescope`); a variable bound inside the right side (a λ, a
    `let`, a nested pattern's field) shows nothing, as the machine keeps no positions below a
    declaration. DONE.

### F. Programs about programs
31. DONE in parts: `deriving eq D` (`1c5282a`), `deriving show`/`derive-show` (`896713ea`),
    autodiff with the Taylor theorem and `D-code` over codes (`896713ea`), reflection by
    `ring-eq`; the stuck-primitive neutrals have a code (`cprim`, `c8931ab`); `%view` covers
    every value kind the net has (stuck calls as `wcofix`/`wproj`, primitives as `wprim`,
    words with the big case under `wwlit`); `wnative id k` views a `V_NATIVE` handle (B18,
    2026-11-06). `D-correct` over the fragment's codes (2026-11-06, `lib/autodiff.cub`): `PCode`
    mirrors the fragment's shapes of `Code` constructor for constructor (the λ's variable, a
    numeral's zero and successor, `plus`/`times` applied), `cev` is what `eval-g` computes on
    it, `dpc` the derivative code to code, and `D-correct : (c : PCode) (x h : Nat) → Σ q, cev
    (x + h) c ≡ cev x c + h·cev x (dpc c) + h²·q` is checked by induction on codes (with
    `D-correct-fwd`: forward mode agrees); the untyped copy `pcode-of-code`/`code-of-pcode`
    round-trips and `eval-g` agrees with `cev` on the readings (`Code` is untyped, declared
    before the checker, so the theorem quantifies over the mirror).
32. **Status (2026-11-06):** the two verified checkers (`lib/linproof.cub`, `lib/eqproof.cub`), the ring checker, the directed Farkas search (`89ec3e0e`) and the bridges from codes (`d1e5f790`) are built, and over `Int` too (`lib/intproof.cub`, `lib/intlin.cub`: the checker is the word checker over the coordinates of differences, its soundness transported); the elaborator calling the search on holes is `by-lin`
    (2026-11-06, `src/syntax.c` `by_lin`): at a hole of type `Lew a b`, the goal and the
    hypotheses `h : Lew l r` in scope are read as linear codes over their atoms (`plusw`,
    `%wsucc`, word literals; any other subterm an atom), `lib/lin.cub`'s `omega-cert` runs at
    elaboration time (untrusted, through `readback`), and the hole becomes `cert-sound env hs ms a
    b holds refl` at the atoms' environment, the verified checker's verdict decided by the
    checker's own normalization (`tests/cert.cub`: the decoder's obligation, a chain of three
    with doubling, a false goal refused). Over `Int` (`leZ a b`): the goal is definitionally the
    word claim a₁ + b₂ ≤ b₁ + a₂ on `toZ2`'s coordinates, so the word path is used with each
    `addZ`, `negZ` and literal met adding the equations that tie its coordinates to its parts'
    (`toZ2-fromZ2-rel`, `toZ2-neg-fst2/snd2`, computation), each as two inequalities by
    `eq-le`: `leZ-trans`, `leZ-add`, `leZ-neg` and `x ≤ y + (−3) → x + 3 ≤ y` are one-word proofs
    (`tests/cert.cub`). DONE. Spec as written: Certificate checkers as verified terms over codes; search unverified (in-language or SMT);
    unchecked solvers marked. Essential.
33. **Status (2026-11-06):** built as `lib/nav.cub` with `lib/cost.cub`'s models (size, recorded steps through `%gsteps`, `13f09662`). Spec as written: Navigation over established identities. Precisely: the store holds identities; a view is
    a subset a reasoner currently holds; from a view the available transports are computable
    (for each identity and term in view, whether it applies and what the term becomes);
    navigation computes that set and chooses by cost and purpose, never by validity, since
    every available transport is valid by construction; a model or tactic changes the view
    and has no other role; nothing is proposed and then checked. Safety follows: no
    participant can make an invalid step count, because establishment is normalization,
    local and needing no authority, and the store spreads by keys with no center. The query
    optimizer is the existing instance with fixed identities over relational algebra.
    Essential for the substrate. The cost model and the choice rule: DONE 2026-11-06
    (`lib/cost.cub`: `cmodel` of recorded steps, size and a schedule's latency; purposes
    `reach-p`/`minimize`/`reach-min`; best-first `navigate-best`, ties by key; 18.14).
34. ★ DONE (`1c5282a`, `src/store.c`): keys as the SHA-256 of the canonical elaborated form
    (names outside), the store under `CUBICAL_STORE` with one line per accepting checker, a
    definition accepted by the current checker not checked again, `#key`/`#names`/`#history`;
    `lib/std.cub` against a filled store: 145 of 145 checks skipped. Retention by reachability
    is `#gc` (`store_gc_cmd`); the certificate's checker name hashes the running binary
    (`dd301c8`), the runtime key per compiler version. `%gsteps n` (2026-11-06) reads the steps
    a checked definition took, from the run or from the store's ok line: 18.14's cost model
    table in-language (`lib/cost.cub` can consume it).

### G. Scale
35. Distribution by keys over wires; the net on GPUs. Optional now.

**After the build.** The system extends itself: new components are terms, judged by the same
normalization as user programs, accepted into the store under their keys; the theory contains
its own theory, so no layer is outside it, and for what counts as a valid term the rules are
the only thing trusted, a few thousand lines (what else is trusted today is section 13). That is the autopoiesis the premises ask
for, and the trajectory after the build is the system doing this across machines.

## 13. The trusted surface
The rewrite rules, including three things about them that are tested and not proved: that they
implement CCHM, that normalization terminates on checked terms, and that the canonicity
statement holds for them. The checker term's correctness relative to the rules (tested by the
quote/eval laws and the `#fail` suites). The small definition or postulated specification
behind each native primitive. The drivers. Stated hypotheses for consumer C++ parameters.
Unchecked solvers. The C compiler, removed today by CompCert for proof-critical builds and
later by translation validation against Sail instruction-set models.

## 14. Measured facts to keep
Brunerie simplified: 1.26M steps, −2; g8 1.136M; 6,706 unkeyed transports; 291 hits;
substitution+erasure 53%; library load 1.0 s cached, 1.4 s uncached (one thread); compute
about 0.3 s; end to end 3.1 s (one worker) / 2.7 s (one thread) on the builder's box; memo
hits 248 on one thread, 9 with workers; Agda's figure for the same construction is about 1 s on other
hardware and is not comparable until measured on the same machine. Nested-loop transport
linear: 4.2k/9.1k/19.0k/78.4k (left), 5.1k/12.0k/27.0k/146k (right) at 4/8/16/64 loops.
`fibw 90` by word elimination over a pair reads in 2,416 steps on the net (words are exact:
`fibw 1000` reads its 209 digits); specialized under an unknown argument it is a `for` loop in
C that gcc runs in the microseconds (`run.py`'s `fib`), the same number.
Reflection (`lib/ring.cub`): `(x+1)^n` against its right-nested product checks in 9.6k / 15.5k /
24.2k / 37.3k steps at n = 2/3/4/5, `(x+y)^n` in 18.7k / 30.3k at n = 3/4 — the normalizer's
cost, roughly the number of monomial insertions; the proof term is constant.

## 15. Open threads
- The memory model for relaxed atomics.
- The configuration value's form (venues, parameters, role) and feedback of consumers'
  instantiations: answered and built (18.15).
- The cost model (latency claims, schedules, navigation): navigation's form is decided
  (18.14's status: weighted recorded steps, size and a schedule's latency per global; purpose
  a predicate or an objective; best-first, ties by key); latency claims for whole programs
  (end-to-end bounds from a schedule) remain open.
- Navigation (F33): what a step is, the cost model and the choice rule are settled (18.14,
  `lib/cost.cub`'s `navigate-best`).
- Lévy-optimal sharing versus full laziness plus memo.
- Cartesian cubical and higher observational type theory as alternative preludes.
- Multiplicities over cubical paths: the usage rules through the composition clauses are
  written (2026-11-06, 18.1: the line read for what the composition reads, tube and base
  joined).

## 16. The ledger

Each row: a feature; its kind here, **C** (consequence of the core, stated), **L** (library,
no new judgment form), **N** (a hole in meaning, not wanted); the construct; what is stronger.
An empty last cell means parity, which satisfies the criterion without a stronger claim; a
cell naming a trade says where the other language is stronger. A row with no answer in the
construct column is a gap and becomes a ★ item.

### 16.1 C++
| feature | kind | construct | stronger because |
|---|---|---|---|
| templates uniform in a type | C | functions over types; the image keeps them as templates | the same language at both stages |
| templates specialized per type; partial specialization | C | functions over codes, matching on the code | checked; specialization at any time |
| `constexpr`, `consteval`, `if constexpr`, folds, pack indexing, expansion statements | C | normalization, also with unknowns | computes with unknowns; no separate interpreter |
| concepts, SFINAE | C | propositions on codes | laws included, not only syntax |
| reflection (`^^`, splices, `define_aggregate`) | C | codes, `%view`, quote, eval | equational theory over codes; `deriving` over codes |
| virtual interfaces, type erasure, `std::function` | C | dependent pair of hidden type and operations; call = projection | devirtualization is specialization |
| `variant`, `optional`, `expected` | C | sum types; a second return | sums are native, not a library; exhaustiveness checked |
| coroutines, `generator` | C | codata; the frame is the stuck call | one process model |
| senders/receivers, schedulers, `stop_token` | C | programs over interfaces; handlers; stopped outcome | laws are paths; specialization at any time |
| ranges and views | C | lazy pipelines fused by normalization | fusion is not an inliner heuristic |
| contracts (`pre`, `post`, `contract_assert`) | C | validity facts in prove/check/assume modes | "prove" exists and costs nothing |
| erroneous behaviour, hardened library | C | the "initialized here" fact; check mode | provable instead of checked |
| profiles (deferred) | C | safe = no assumed facts, oracles or foreign calls | a definition, which profiles lacked |
| `bit_cast`, `start_lifetime_as`, `is_within_lifetime`, `launder` | C | typed reads decode bytes through layouts | defined wherever bytes are valid |
| `mdspan` layouts, `inplace_vector`, `hive` | L | declared representations | equivalences, so programs transport across layouts |
| trivial relocation | C | position-independent encodings | derived, not declared |
| `simd`, `linalg`, saturation arithmetic | L | primitives on exact types; specs postulated at first | the trusted surface is listed by `#trusted` |
| hazard pointers, RCU | C | retention by reachability; counted boxes (structural at `9b94b43`) | no reclamation protocol to write |
| modules | C | the store (F34); cached modules replay the checked program | content-keyed; renaming is free |
| exceptions | C | a second return | typed |
| ownership, moves, RAII, `unique_ptr`, `shared_ptr<const T>` | C | multiplicities; erasure; memo boxes | uniqueness by wires and holder counts; linearity by the checker; neither by convention |
| raw pointers, pointer arithmetic, unions, placement, bit-fields | C | the byte layer | same instructions; validity facts visible |
| undefined behaviour | N | a proof, check or marked assumption | the optimizer's assumptions are theorems |
| data races | N | synchronized code over the memory interface | a race is a world choice under a named model, never undefined behaviour |
| pattern matching (deferred to C++29) | C | `case` on data | includes path constructors |
| `#embed` | C | a file as a term | |
| STM (not in C++; Haskell/Clojure) | L | `atomically` on the memory interface | typed: no I/O inside |

### 16.2 Systems languages: Rust, Zig, Swift, Hylo, Mojo, Carbon, Austral, ATS, Odin, Jai
| feature | kind | construct | stronger because |
|---|---|---|---|
| Rust borrowing, lifetimes, `&mut` | C | multiplicities; holder counts; projection reuse | derived from wires and boxes; Rust's is wholly static (no counts at run time), ours is dynamic unless asserted: a trade, not a win |
| Rust traits | L | records of operations with laws by implicit unification | laws |
| Rust `unsafe` | C | an assumed validity fact | searchable, hypotheses inherited |
| Rust async | C | codata processes | one model |
| Zig `comptime` | C | normalization | with unknowns; proofs |
| Zig explicit allocators | C | regions as values | |
| Swift `inout`, copy-on-write, noncopyable, `consuming`/`borrowing` | C | projection reuse; copy-on-write by holder count; multiplicities on parameters | |
| Hylo mutable value semantics, projections | C | same | |
| Mojo MLIR, SIMD-first, parametric comptime | C/L | normalization; `simd` primitives; MLIR as optional target | |
| Carbon interop-first | C | the C++ export | with proofs |
| Austral linear types, capabilities | C | 1-linear; signatures as capabilities | |
| ATS dependent + linear | C | the same, cubical | |
| Odin/Jai comptime, allocators | C | as Zig | |

### 16.3 Effects, functional, proof assistants: Koka, Effekt, OCaml 5, Haskell, Idris 2, Agda, Lean 4, Coq, F\*, Dafny, Verus
| feature | kind | construct | stronger because |
|---|---|---|---|
| Koka effect handlers, Perceus | C | processes over signatures; worlds as handlers; holder-count reuse | no new judgment form |
| Effekt, OCaml 5 handlers | C | same | |
| Haskell laziness, fusion | C | the net's laziness; normalization | |
| GHC RULES | C | theorems as rewrite directions | checked |
| Haskell linear types | C | multiplicities | |
| Haskell STM | L | `atomically` | typed |
| Idris 2 QTT | C | multiplicities | cubical |
| Agda/Cubical Agda | C | the machine runs the theory | one runtime for checking and running; native stage |
| Lean 4 elaborator in Lean, tactics, `simp` | C | the checker as a term; navigation over identities | |
| Lean 4 Perceus | C | holder-count reuse | |
| Coq reflective tactics (`lia`, `ring`) | L | certificate checkers as terms over codes | |
| Coq/Iris | L | the proof library for the memory interface | |
| F\*/Low\* extraction, ghost code | C | first-order criterion; multiplicity 0 | representation change is transport |
| Dafny, Verus SMT discharge | L | certificate checkers; marked oracles | proofs are terms; parity of automation is F32's job, not yet built |

### 16.4 Arrays, numerics, scheduling: Futhark, Halide, Dex, JAX, Julia, Wolfram
| feature | kind | construct | stronger because |
|---|---|---|---|
| Futhark size types, fusion | C | `Fin n → A`; normalization | |
| Halide algorithm/schedule split, autotuning | C | schedules as values; search over correct candidates | correctness is by construction |
| Dex typed indices, `for` | C | functions from index types | |
| Dex/JAX autodiff | L | a transformation over codes with a correctness path | the path |
| Julia run-time specialization, multiple dispatch | C | stage-3 specialization on codes; `case` on codes | cached by key |
| Wolfram symbolic evaluation, notebooks | C | normalization of open terms; `#input`/`#supply` | rules checked; types; native stage |

### 16.5 Processes, distribution, identity: Erlang, Pony, Go, Kotlin, Scala 3, Roc, Unison, Nix, Bend
| feature | kind | construct | stronger because |
|---|---|---|---|
| Erlang isolation, let-it-crash, supervision, hot loading | C/L | outcomes as values; supervisors; keys | provable restart; state migration explicit |
| Pony reference capabilities | C | 1 and ω on sends | race freedom by locality |
| Go/Kotlin/Swift structured concurrency | C | the process tree with the stopped outcome | |
| Scala 3 capture checking | C | signatures as capabilities | |
| Roc platforms | C | worlds as named handler implementations | |
| Unison content addressing, no builds, distribution by hash | C | the store as the codebase; keys over wires | proofs between versions |
| Nix/Dhall reproducibility, totality | C | content keys; totality | |
| Bend/HVM GPU interaction nets | — | see 16.6 | |

### 16.6 Data, queries, hardware, sandboxes: SQL, Datalog/Prolog, TLA+, Chisel/Clash, CUDA, WebAssembly, Python/JS
| feature | kind | construct | stronger because |
|---|---|---|---|
| SQL query optimization | C | navigation over proven equivalences by cost | the optimizer's identities are theorems |
| Datalog, Prolog, miniKanren search | N | not a language feature | derivation is navigation, not search |
| TLA+/P model checking | L | a reading over every choice of a finite world | a term |
| Chisel, Clash, Bluespec | C | the circuit target | optional |
| CUDA/Triton kernels | L | first-order regions over `simd` types; MLIR optional | |
| WebAssembly linear memory, component model | C | the byte layer; optional target | |
| Python/JS dynamic typing, REPL | C | the untyped end of the spectrum; readings | checkable later on the same runtime |
| Python/JS ecosystems | L | foreign code as postulated representations | trusted surface explicit |

No row is without an answer at this revision. The rows marked L are the library's table of
contents; none adds a judgment form.

### 16.6 The HVM/Bend lineage (HigherOrderCO): the nearest runtime, and the upper end to supersede

The only other line of work that runs programs on interaction nets at scale. Its stages, as
published: Formality and Kind (a proof language over an interaction-net runtime, equality by
rewriting, self types); HVM1 (2022, interaction nets, the Kind back end); HVM2 (2024, symmetric
interaction combinators, a CUDA evaluator, the runtime of Bend, near-linear scaling across
thousands of GPU threads for programs without sequential bottlenecks, an interpreter at the
core); HVM3 (2025, the "Interaction Calculus": affine λ with no scope boundaries, first-class
duplications and first-class superpositions, functions compiled to C, SupGen/NeoGen using
superpositions to run a program on an enumerated family of inputs at once for program
synthesis); HVM4 and Bend2 (2025–26, as announced: compilation of Interaction Calculus
functions including superpositions to machine code with no interpretive overhead, a native
type system running on the nets so HVM can serve as a parallel proof verifier, Bend2 targeting
GPUs, lazy HVM, JS/Python, and "a complete proof system like Lean and Kind"; a Bend→Metal
compiler written in 2026). What the lineage proves: an interaction-net runtime reaches
hardware parallelism without a scheduler, optimal β-sharing is implementable, and the same
runtime can carry a type system. Row by row:

| feature | kind | construct here | stronger because |
|---|---|---|---|
| interaction nets / combinators as the runtime; locality gives parallelism | C | the same (section 1): rules on two cells at principal ports, workers compared with the single thread on every suite | the rules are the theory's own (composition clauses as terms), not a generic combinator basis the language is encoded into |
| optimal β-reduction by duplication nodes with labels (Lamping); affine λ, global substitution, no scope | C | wires as variables; sharing by content keys; retention by counted boxes | sharing is of *equal work* (one term is one pointer), not of λ-bodies by label bookkeeping; no label/oracle problem, no "dup of a dup" cases; ordinary λ, not affine |
| superpositions as first-class values (a term in two states at once, collapsed by dups) | C | open-term normalization: the common work of a family of inputs is done with the variable free, then substituted (binding time); keys do the rest of the sharing | the "family" is a type, not a labelled pair; the shared result is a theorem about the family, not a coincidence of reduction order |
| SupGen/NeoGen: enumerate programs in parallel through superpositions | L | navigation over the store's identities (F33); specialization with open variables; the certificate checkers | candidates are reached along identities and are correct by construction; search is unverified, acceptance is a verified term |
| HVM2 on CUDA: thousands of threads | — | CPU workers today; GPUs are the symbolic stage's optional target (B16) | parity is the goal here, not a claim |
| HVM3/HVM4: functions compiled to C / machine code, superpositions included | C | the first-order stage (B13–B16): a criterion names what is native and why; the net reads the rest | the boundary carries proofs (assumed facts, certificates, representations as equivalences); native code is the *same term* specialized, with its justifying path |
| Kind's types: self types, equality by rewriting; HVM4's native type system on the net | C | the checker as a term on the net; cubical HoTT with univalence computing, HITs, Glue | paths compute: representations, specializations and replays are transports; no HVM-line system has univalence or higher inductive types computing |
| Bend2's "complete proof system like Lean" (announced) | C | the standard library, the verified decision procedures, certificates, the store of theorems — built and in the suite | exists now, on the one runtime |
| single-core speed of the interpreted combinator runtime (poor; parallelism compensates) | — | work proportional to the result (A8) plus the native stage for first-order regions | the hot path is C an engineer writes; the net is for what has no native form |
| content-addressed program identity (not in HVM; in Unison) | C | the store (F34) | proofs between versions |
| a Python-like surface (Bend) | L | a surface is a translation into the store's codes; none written yet (E29's translator from Agda is the first) | |

The sentence to keep: the HVM line is the proof that this runtime shape scales; what it does
not have — equality as a primitive that computes, a checker that is a term, proofs carried
across the native boundary, sharing by content rather than by duplication labels — is
exactly this project's content. Where it leads (GPUs, machine-code compilation of the whole
calculus), parity is the acceptance and is in the queue, not claimed.

## 17. Changes in revisions 4 to 6, and retracted claims
Revision 7.9 (2026-11-06, the three-line build: a machine line and two library lines in
parallel worktrees, 37 commits): 7.8's whole queue landed and the items listed in section
12's queue paragraph; statuses written into 18.8, 18.12, 18.16 (with the A8 measurements and
the boundary-skip withdrawn as nondeterministic), 18.19 added (A4 design), D26, items 3, 10,
26, 30, 31, 34 of section 12. Retracted: nothing; one correction — `make test`'s long line
was broken for an hour by a typed `lapp` shadowing the untyped one (fixed, and `names.py` now
scopes its allow-list; redeclaring a checked name is an error since `e7632b1e`).
Revision 7.8 (handoff): the builder's `1c5282a` and `be7149d` recorded — B13 (`lib/stage.cub`),
F34 (keys and the store), C20's `#safe`, E28's instances/records/modules, `deriving eq`,
D27's `foreign`, stuck primitive operations as neutrals, C22 arrays over the byte layer;
the queue rewritten for a fresh agent; the document moved into the repo at
`scoped-machine/PLAN.md`.
Revision 7.7: 18.17 landed by the builder (`c186b81`): hypotheses for path-typed recursive
fields; the typed port has no primitive, −2 in 1.35M steps. On my side: the C printer with
closures and the end-to-end compile (B16 milestone 1), closure conversion, and the verified
normalizer (`lib/linproof.cub`). Two elaborator findings recorded for E28: composed paths'
implicit endpoints need a signature (not a motive inside a step), and λ-paths in argument
positions have their endpoints evaluated (use congruences).
Revision 7.6: A10b done (typed port, 229 checks, 3.16M steps); `lib/lin.cub` pushed.
Revision 7.5: `lib/specialize.cub` pushed; the code→normal form→rewrite→first-order form→
native evaluation pipeline runs end to end on readings.
Revision 7.4: `lib/fo.cub`, `lib/match.cub`, `lib/nav.cub` built, tested and pushed; B14's
form and lowering, 18.6 step 3's matching, and 18.14's first executable form exist.
Revision 7.3: strict positivity under Π codomains and Σ components for data (18.18),
found by writing the process library against the machine; `lib/proc.cub` held in the
scratchpad until it lands; the builder's queue gains 18.18 after 18.17.
Revision 7.2: induction hypotheses for path-typed recursive fields specified (18.17) to
remove the trusted truncation recursors from the typed port; the builder's two early
elaborator fixes recorded under E28.
Revision 7.1: A5, A6, A9 marked DONE at `91c128b`; A7 closed as not landed with its
reason; A8's mechanism recorded from the builder's measurement and the fix given as a
proposal (18.16); measurements updated.
Revision 7: specifications for every remaining build item added as 18.5–18.13 (B14–B19,
C20–22, D24–27, E28, F31–F32), a concrete design for navigation (18.14) and current answers
to the open threads (18.15). Every item in section 12 now has a specification with
acceptance tests and size.
Revision 6.5: specs merged into section 18 (one document); ledger pass on the "stronger"
column: six rows corrected (variant, simd, modules, ownership, data races, Rust borrowing,
Verus), empty cells defined as parity, trades named.
Revision 6.4: the four ★ specifications written and critically revised; their binding
corrections recorded in section 12.B.
Revision 6.3: laws restated as data consumed by the specializer before erasure; only
`[[assume]]` and consumer-boundary contracts survive as part of the translation; certificates
and `static_assert`s are optional output.
Revision 6.1: laws in the image carried by `[[assume]]`, `static_assert`, contracts and a
named `constexpr` predicate with the proof's key; never a comment.
Revision 6: the C++ image (10.1) as a compositional translator with the binding-time
assignment "as late as C++ can express", preserving factoring; the fragment it emits, with
its own semantics, giving translation validation of the image and a finite, constructive
import for code written in the fragment (9); generality as a schedule parameter (5.2); B19
restated with the residue printer as its first milestone and the size revised from ~800 to
~3,500 lines.
Revision 5.1 (builder's review of the scope statement, all points accepted): the cost claim
corrected from "linear in nesting for every term" (false: 2^k faces) to "work proportional to
result size, nothing built the result does not contain" (0, 12, A8); A8's priority grounded
in the checker and every program about programs paying the net's cost (1, 12); the trust
sentence after the build list scoped to validity of terms with section 13 for the rest;
replay stated as reproducing the world's response order and the run, not the net's schedule
or the time (7.3); F33's open part narrowed to the cost model and purpose (15); B11's static
side listed as planned, not built (12).
Revision 5: section 0 restated as foundations with three success measures; the forced
dependency order of the core and the relation to Respecting Equality, Witnessed Computation
and the Interactive Symbolic Computer added to section 1; the trading stack's rationale as
first full-scale system added to section 10; F33 navigation stated precisely (view, available
transports, choice by cost and purpose, models change the view, safety); autopoiesis stated
after the build list. Machine state unchanged at `9b94b43`.
Revision 4.1 (builder's review, accepted where it matched the code at `9b94b43`): A2 done;
g8 removed from A6 (its transports are pairwise distinct); dynamic uniqueness structural, B11
reduced to the static side; core item 4 stated at its true strength with the measured
schedule dependence of hits, and no retention policy added (the builder's LRU is rejected:
an entry lives exactly while reachable); strict/lazy agreement restricted to checked terms;
bitmask formulas marked as a decision; arena release as A5; both load figures; the priority
reframed from Brunerie as gate to the linear-cost property as the claim, with Brunerie as one
measurement; "Agda cannot be measured here" rejected.
Revision 4: the core stated as five things (1); the criterion for superseding stated (0);
the ledger (16) as the check on it; priority derived from the claimed properties (12); the
typed Brunerie port added as A10b and as the gate on E28; the trading-system table inlined;
WebAssembly/MLIR/circuits consolidated as optional targets; sections renumbered.
Revision 3.1 (critical review): multiplicities split into static (checker) and dynamic (net);
certificate checking primary; two keys for the store; supervision's linear-resource claim
limited to in-language failures; effects via the existing signature parameter, no new
judgment; the linear-and-memo invariant; rewrite theorems under a terminating discipline; A8
as hypothesis; floats postulated first; "safe" excludes oracles and foreign calls; the C++
correspondence as thesis; choreographies under projectability; the machine's unproven
metatheory in the trusted surface.
Retracted (stand): a "lossy representation" category; keys-on-duplication; codes not types for
specialization; linearity for owned memory only; `A ≃ Σ bs, valid`; linearizability as
refinement; sequence locks not clean; header-only is packaging; path-free data not sets; term
growth not per-step cost; Agda on the same machine; "HoTT on the net is a research question"
(HoTT is what the machine runs; HOTT, higher observational, is the alternative theory).

## 18. Specifications of the ★ items

Each at revision 2 after a critical pass. Section numbers inside each spec are its own.

### 18.1 B11 — Multiplicities (static side)

**DONE** (`0729899`, 2026-11-06): `(x :0 A)`, `(x :1 A)`, ω unwritten, kept through the
passes into `V_PI.x` and `cpi m A B`, conversion ignoring it; usage computed after a
definition checks, in one walk (`usage` in `lib/check.cub`): variables 1, arguments by the
callee's binder or its computed usage (`%gusage`, so `lib/std.cub`'s unannotated levels and
types are 0 with no edit), types 0, Glue systems ω, transport lines read for what the
composition's computation reads (2026-11-06, below), case = scrutinee +
join of branches, let as the table; errors `E-erased-used`, `E-linear-dropped`,
`E-linear-dup`, `E-linear-intro` (a checked definition may not apply a constructor with a
`:1` field; a primitive may). Usages recorded with the module's forms; `#usage f` prints
them. As specified: erasure by computed usage, not by binder type; linear introduction only
in primitives. The library's check costs about a quarter more. Still open from the spec: the
back end reading the multiplicity from the Π (B14/B16 take it from `cpi`). Found 2026-11-06
(`#usage load64` prints nothing; `%gusage` answers ω): a `primitive`'s parameter usages are
not computed from its body, so `load64 n b off pf = %load64 b off` has `n` and `pf` at ω
rather than 0, and every proof passed to it (the decoder's `h` in `sum-msgs`) is ω too —
the erasure of bound proofs rests on this one walk over primitives' bodies (~20 lines in
the usage pass; the machine-primitive application `%load64 b off` reads `b` and `off`
once). Until then `lib/repr.cub` drops the primitives' length and proof arguments by name.
Erasure at the call boundary needs both sides at once (B14): a definition lowered from its
code drops its 0-parameters and every call of it drops the corresponding arguments.

Spec for the checker, elaborator and codes. Revision 2 (after a critical pass; changes at the end). Against scoped-machine `9b94b43`.
The dynamic side (uniqueness by box holders) is structural and needs no change.

#### 1. What is added

Every term binder carries a multiplicity `m ∈ {0, 1, ω}`:
- `0`: the variable is used only in positions that the back end erases;
- `1`: the variable is consumed exactly once and may not be dropped (linear);
- `ω` (default, unwritten): no constraint.

A fourth reading, "used at most once", is not an annotation: uniqueness for in-place update is
decided dynamically by box holders, and a static assertion of it is `1` with dropping allowed,
which is not needed as a separate mode.

Multiplicities are written on Π binders and on constructor fields only; λ binders take
theirs from the Π they are checked against and carry no syntax. An annotation is a
constraint the user asserts and the checker verifies. Erasure does not depend on
annotations: a binder is erased when its *computed* usage is `0`, annotated or not. So an
unannotated binder is `ω` as a constraint (no check) and still erased if unused. Dimension binders, tick binders and clock binders have no
multiplicity: dimensions are not resources, ticks and clocks are erased.

#### 2. Surface syntax (`src/syntax.c`)

```
(x :0 A) → B        (x :1 A) → B        (x : A) → B      -- ω
{x :0 A} → B        -- implicit binders likewise; an implicit's default stays ω
data Order (s : OrderState) = order (id :0 Word) (qty : Qty)     -- fields
record BVec (n : Nat) = { k : Nat ; v : Vec k A ; bound :0 le k n }
```

No defaults by type: a `{A : Type l}` binder is erased because its computed usage is `0`,
not because it is a type; a type passed to `ua` or into a Glue system has usage `ω` by the
table and is kept. This makes every `{l : Level} {A : Type l}` in `lib/std.cub` erased with
no edits and with no rule that could erase a computationally used type.

#### 3. Codes (`lib/code.cub`)

`cpi A B` becomes `cpi m A B` and `clam b` becomes `clam m b`, with `m` a word (0, 1, 2 for ω).
Constructor telescopes in `dent` carry a multiplicity per field. `code`, `eval`, `quote`
and readback (`src/lang.c`) carry `m` through; conversion ignores it (two Π types with
different multiplicities are convertible: multiplicity is a usage constraint, not part of the
type's identity). `%view` of a Π gives `wpi m A B`.

#### 4. The accounting (`lib/check.cub`, new pass `usage`)

Run after `check` succeeds on an elaborated term (the term carries its values, so heads have
inferred types). `usage t x` returns the usage of variable `x` in `t`, a value in `{0, 1, ω}`
with `1 + 1 = ω`, `ω + _ = ω`, join `0 ⊔ 1 = 1`, `_ ⊔ ω = ω`. Positions:

| position | contribution |
|---|---|
| `x` itself | 1 |
| a type annotation (`cann _ T`, the `A` of a binder, `cuniv`, the `A a b` of a path type used as a type) | 0 |
| argument `a` of `capp f a` where `f : (y :m A) → B` | `m · usage a x` (0·u = 0, 1·u = u, ω·u = ω if u ≠ 0) |
| function position `f` | `usage f x` |
| pair `(a , b)` | sum |
| `fst p`, `snd p` | `usage p x` |
| `case e of … branches` | `usage e x + ⊔ over branches` (each branch counted with its fields bound) |
| `comp L u a`, `transp L a`: the line `L` | `ω · usage L x` (a type line is consumed computationally: transport along `ua e` runs `e`) |
| `comp L u a`: the tube `u` and the cap `a` | sum |
| `λ i → e` (path), `e @ r` | `usage e x`; the interval expression `r` contributes 0 |
| `Glue A S`, `glue S a`, `unglue`, `%hcompU`: the systems `S` | `ω · usage` (used by transport) |
| `let y = t in b` | `usage b x + (usage b y) · (usage t x)` |
| codata copattern bodies | per field, as a term; a stuck call's arguments count by the definition's Π multiplicities |
| `dfix`, `fix`, `forceκ` applications | by their signatures' multiplicities (all ω) |

Checks, reported with new error numbers (the table in `src/lang.c`):
- a binder annotated `0` with usage `1` or `ω`: **E-erased-used** (the variable, the term);
- a binder annotated `1` with usage `0`: **E-linear-dropped**; with usage `ω`: **E-linear-dup**;
- a constructor field annotated `1`: the constructor's value is linear; a `case` on it binds
  the field at `1`;
- in a `case`, every branch of a `1`-bound scrutinee field is checked at `1` (each branch
  consumes it exactly once).

Data types whose constructors have a `1` field are **linear types**; a value of a linear type
is itself at multiplicity `1` wherever bound.

#### 5. Where linear values come from

A constructor of a linear type is applicable only inside a definition declared `primitive`
(world commands and drivers; `ack`, `cancel`, `close` are such). A plain definition applying
one is **E-linear-intro**. This keeps the memo invariant without a runtime change: a
command's response enters every dependent key, so two linear values never share a key.

#### 6. What the back end reads

The first-order form (B14) omits every `0` binder and every `0` field. A `0` field is absent
from the automatic layout (6.1 of the plan): `record BVec` lays out as `k` and `v` only. The
specializer re-runs `usage` on a residual, so occurrences that specialization reduced away
lower a binder to `0` at compile time.

#### 7. Cubical cases, fixed by the table

- A path `p : a ≡ b` passed to `subst P p` is `ω` (the line `λ i → P (p i)` is consumed).
- A path in a path-free data type that normalizes to a constructor tree is still `ω` by the
  static table; it becomes `0` only after the specializer reduces the transport, which is the
  intended division: static accounting is sound, specialization makes it precise.
- `isSet A` passed to a set-truncation eliminator is `ω` (used in `%case-hcomp`).
- Implicit type and level arguments are `0` everywhere.

#### 8. Acceptance

1. `lib/std.cub` and `lib/check.cub` check unchanged; every `{l : Level}` and `{A : Type l}`
   binder reports usage `0` (a `#usage` command prints a definition's binder usages).
2. `record BVec` above: `#layout BVec` (B12) shows two fields.
3. `tests/check.cub`: `#fail` on a definition that drops an `Order sent`; `#fail` on one that
   uses it twice; `#fail` on `λ (p :0 a ≡ b) → subst P p x`.
4. `experiments/brunerie/brunerie.cub` (typed, A10b): every implicit level and type binder `0`;
   no proof binder `0` before specialization, which is correct.

#### 9. Changes in revision 2 (critical pass)
- Erasure by computed usage, not by annotation or by the binder's type; the "default 0 for
  types and levels" rule removed (it would erase a type used by `ua` or Glue).
- λ annotations removed; multiplicities are written on Π and fields only.
- Linear introduction restricted to `primitive` definitions, one rule.
- The `let` rule written correctly.

#### 10. Size

`lib/code.cub` +20; `lib/check.cub` +~300 (`usage`, the four errors, the linear-intro rule);
`src/syntax.c` +~120 (`:0`, `:1`, field annotations, `#usage`); `src/lang.c` +~40 (readback,
error names, `%view` of `wpi m`). About 480 lines.


**Through the composition clauses (2026-11-06, §15's thread).** `lusage` in `lib/check.cub`
reads a composition's line for what its computation reads, in place of counting the whole line
ω: a path type's endpoints become the sides of the composition in it (ω); a Glue's or a
universe composition's system runs its equivalences (ω); a data type's parameter that is a type
is read as a line again, any other parameter counts ω (it can reach a field's path endpoints,
as a fording index does); a family applied counts ω (it unfolds); a variable in type position
counts nothing. The tube and the base are joined (`⊔`), not added: where both are present they
agree (`u 0 = a` on the tube's face), the value on the face is the tube's end and off it is
built from the base, so a resource in both is one resource at its two ends; used twice within
the base or within one side it is ω. `tests/check.cub`: `tr0 : (X :0 Type) → X → X` by a
transport along `X` checks with `X 0` (rejected before); `tr-end`, whose line's endpoint is a
binder at 0, is rejected (`E-erased-used`); `lin-tb`, a linear `n` in the tube and the base,
checks with `n 1` (rejected before as `E-linear-dup`); `add2 n n` in the base is still rejected.
The back end is unaffected where it matters: a region with a composition along an open line is
`not-fo kan` (B13), so nothing erased by these rules is read by compiled code.

### 18.2 B12 — Representations as a construct

**DONE** (`01c839c`, 2026-11-06): primitive types `Int64`, `UInt32`, `Float64`, `Float32`
(bits in the cell; the floats with no specification, trusted and listed) and `Bytes n` with
`%load8..64`/`%store8..64` (a store is a copy) and `%blen`; `lib/prim.cub` declares them as
`primitive`s with types, `load32`/`store32`/`load64`/`store64` taking the proof `le (plus off
w) n`; `represent T by R where to/from/to-from/from-to and op f : A = native by law = proof`,
each line a checked definition, the laws required, `%rtab d` the table the back end reads;
`Nat` represented by `Word` with `plus` by `plusw`; layouts computed by the checker (`#layout
T`, `#layout T as R`: unions of structs of the fields not at multiplicity 0, pointers for
recursive fields, widths for primitives, none for path constructors); `#trusted` lists every
postulate and primitive. `lib/decode.cub` (`442bbbb`) uses it: a decoder's `le (plus i 4) n`
discharged by `lib/linproof.cub`'s certificate checker, evaluated. Open from the spec: the
`Region` threading and `%ptr-of`/`%region-at` (C20), `safe` as a command, and the back end
consuming `%rtab` and layouts (B14/B16).

Spec. Revision 2 (after a critical pass; changes at the end). Against scoped-machine `9b94b43`. Depends on B11 (multiplicities on fields).

#### 1. What a representation is

A representation of a type `T` is a type `R` with a declared layout, an equivalence
`T ≃ R`, and, for each operation on `T` that is to run natively, an operation on `R` with a
path showing the two agree across the equivalence. Nothing else: no new judgment form. The
declaration is checked by the checker as the terms it contains; the construct exists so that
the back end can find, for a type, its layout and its native operations by name.

#### 2. Three kinds, one declaration form

```
represent List A by ListRep A where           -- 2.1 automatic: no body needed
represent Nat by Word where                    -- 2.2 declared; Word is how numerals already run
                                               -- (cwlit/cwsucc/cwop/cwelim); this formalizes it
  to   : Nat → Word   = …
  from : Word → Nat   = …
  to-from : (w : Word) → to (from w) ≡ w = …
  from-to : (n : Nat)  → from (to n) ≡ n = …
  op add  : Word → Word → Word   = %wadd            -- a native primitive
    by (a b : Nat) → from (add-w (to a) (to b)) ≡ plus a b = …
  op mul  : … by …
represent ∥ℤ∥₂ by Int where                    -- 2.3 canonical form of a quotient/truncation
  to = rec2 Int isSetInt (λ z → z) ; from = ∣_∣ ; to-from = … ; from-to = …
```

- **2.1 Automatic.** For a data type without path constructors, `represent T by auto` (or
  omitting the declaration) gives the tag-plus-fields layout from the constructors, with `0`
  fields omitted. The equivalence is the identity on structure and is not written.
- **2.2 Declared.** `R` is a type whose layout is known: a primitive (`Word`, `Int64`,
  `Float64`, `Bytes n`), an array type, or another represented type. `to`/`from` and the two
  round-trip laws are terms checked at the stated types. Each `op` names an operation on `T`
  by its definition's name, gives its native counterpart (a primitive, or a term over `R`),
  and a path `by` relating them through `to`/`from`.
- **2.3 Canonical form.** The same form as 2.2 where `T` has path constructors; the
  equivalence is the decoding into path-free data. This is how a quotient or truncation gets
  a layout. The checker accepts it like any 2.2 declaration; the content is the user's
  theorem.

A type may have several representations; each is named by `R`. A definition chooses one by
`using Nat as Word` in its schedule (plan 5.2), or the back end chooses the first declared.

#### 3. Primitives (`lib/prim.cub`, new)

```
primitive type Word                               -- exists (big.c behind it)
primitive type Int64 ; UInt32 ; …                -- modular, with %i64add etc. and specs
primitive type Float64 ; Float32                  -- IEEE 754
primitive %f64add : Float64 → Float64 → Float64   -- at first with no in-language spec: wholly
                                                  -- trusted and listed by #trusted; a spec is
                                                  -- added when a model of rounding exists
primitive type Bytes (n : Nat)                    -- the byte layer: load/store at offsets
primitive %load32 : (b : Bytes n) → (off : Nat) → le (plus off 4) n → UInt32
```

Postulated specifications are listed by `#trusted`, which prints every postulate and every
`primitive` in scope: the trusted surface as a command.

#### 4. Layouts

A layout is first-order data the checker computes and the back end reads:
```
data Layout = lword | lbytes n | lstruct (fields : List (name × Layout × align)) |
             lunion (tag : Layout) (alts : List Layout) | larray n Layout | lptr Layout
```
- automatic: `lunion` over constructors, each `lstruct` of its non-`0` fields, recursive
  fields `lptr` (or an index into a region when the enclosing representation says so);
- declared: the layout of `R`;
- `#layout T` prints it; `#layout T as R` for a named representation.
Offsets, sizes and alignments follow the platform value (plan 7.3): `#layout` takes it as an
optional argument, default the host.

#### 5. What the checker does

Nothing new in typing. A `represent` block elaborates to ordinary definitions
(`T.R.to`, `T.R.from`, `T.R.to-from`, `T.R.from-to`, `T.R.op.add`, `T.R.op.add.by`), each
checked at its signature, plus an entry in a representation table (`%rtab`, like `%dtab`):
`(T, R, layout, [(opname, native, path-key)])`. `#trusted` reads `%rtab` for primitives.
The equivalence's two laws are required; an `op` without a `by` is rejected
(**E-op-unproved**) unless its native counterpart is a primitive with a postulated spec, in
which case the spec is the path and the op is listed by `#trusted`.

#### 6. What the back end reads (B14/B15)

For a first-order region at type `T` with chosen representation `R`:
- values of `T` are laid out as `R`'s layout;
- a call of an operation `f` on `T` that has an `op` entry becomes the native counterpart;
  one without becomes `from ∘ f' ∘ to` where `f'` is `f` specialized, which is correct and
  slow, and the specializer reports it (**W-slow-op**) so the user can add the `op`;
- crossing between representations is `to`/`from` of the composite equivalence.
The round-trip laws are what justify reading native results back (plan 5.4.5); the `by`
paths are what justify the native call.

#### 7. Transport across representations

`T ≃ R` is an equivalence, so `ua` gives `T ≡ R`, and `transport` carries any program or
proof over `T` to `R`. The construct adds nothing to this; it exists so the back end can do it
by name at zero cost instead of by evaluating `%comp-glue`.

#### 8. Acceptance

1. `represent Nat by Word` with `add`, `mul`, `iszero`, `lt` and their `by` paths; `#same`
   readings of `plus` on numerals unchanged; `#layout Nat as Word` prints `lword`.
2. `record BVec` (B11): `#layout` prints `lstruct [k : lword, v : larray …]`.
3. `represent ∥ℤ∥₂ by Int` using `isSetInt` from `lib/std.cub`; checks.
4. `#trusted` lists exactly the primitives and postulates in scope and nothing else.
5. A `represent` with a wrong `to-from` is rejected at the law's type (ordinary checking).
6. `Bytes 64` with `%load32` at offset 12: the obligation `le 16 64` normalizes to `Unit`
   and the hole fills; at a variable offset the hole is reported (B11's F32 fills it later).

#### 9. Changes in revision 2 (critical pass)
- `Nat` by `Word` stated as the formalization of the existing numeral mechanism; acceptance 1
  requires no change in any reading.
- Floats: no fictitious `ieee-round`; the operations are primitives with no spec at first,
  listed by `#trusted`.

#### 10. Size

`src/syntax.c` +~250 (`represent`, `primitive type`, `#layout`, `#trusted`); `lib/check.cub`
+~150 (table, `E-op-unproved`, layout computation); `lib/code.cub` +10; `lib/prim.cub` ~200
(primitives and specs, floats postulated); `src/lang.c`/`net.c` +~150 (`Int64`, `Float64`
cells and ops; `Bytes`). About 760 lines.


### 18.3 B13 — The first-order criterion, strictness, schedules

Spec. Revision 2 (after a critical pass; changes at the end). Against scoped-machine `9b94b43`. Depends on B11, B12. Consumed by B14
(the first-order form) and B15 (the specializer).

#### 1. The question the criterion answers

Given a term and a binding-time assignment for its free variables, which subterms may be
printed as native code? The answer is a predicate on a region (a subterm together with the
assignment of its free variables), computed by a term over codes, `first-order : Code →
Assign → Verdict`, with `Verdict = fo | not-fo reason`. It is a function of the code and the
inferred types the elaborated term carries; it runs nothing.

#### 2. Binding times

```
data Stage = now | build | run-known | run     -- plan 5.1's four rows
Assign = List (var × Stage)
```
`now` variables do not reach the criterion: the specializer has substituted them. `build`
variables are the C++ image's template parameters (plan 10.1) and, for the C back end, are
treated as `now` after instantiation. `run-known` variables are JIT specialization points.
`run` variables are ordinary arguments.

#### 3. The criterion

A region is `fo` when all of the following hold, each a separate reason when it fails:
1. **No dimensions.** After specialization no interval variable or path abstraction remains
   in the region (**not-fo dim**). Paths are not data; a closed path in path-free data used at
   endpoints reduces away before the criterion runs.
2. **Types fixed.** Every type occurring in the region, after substitution of `now` and
   `build` variables, is closed and contains no `run` variable (**not-fo dep-type**). A
   dependency on a `run` value through a `0` field (a `le k n` proof) is allowed, because the
   field is erased (B11); a dependency through a non-`0` field is `dep-type`.
3. **Path-free data at the boundary.** The region's inputs and result are of types whose
   representation (B12) is automatic or declared, i.e., path-free inductive types, primitives,
   functions between such (closures), or codata (see 5). A HIT, a Glue type, a `%hcompU`
   type or a universe at the boundary is **not-fo higher** unless a canonical-form
   representation is declared for it, in which case the boundary is `R` and `to`/`from` are
   inserted.
4. **No Kan operation with an open line.** Every `comp`/`transp` in the region has a line
   closed after substitution; the specializer reduces it (identity on a constant line; `e` on
   `ua e`; `%comp-pi`/`%comp-sigma` applied on function and pair lines; a declared `to`/`from`
   on a representation equivalence). One that remains is **not-fo kan**.
5. **No reflection.** No `%view`, `quote`, `eval`, `%var`, `%dim` in the region
   (**not-fo reflect**). Codes are path-free data, so code-to-code programs (a hash, a
   transformation, the specializer's rewriting of codes, the certificate checker) compile.
   `eval` returns a live value of an arbitrary type and `%view` reads one, so the checker's
   decision procedure (`check : Code → Res`, which evaluates and views) runs on the net, not
   natively. This is consistent with the plan: the checker pays the net's cost.
6. **Multiplicities decided.** The usage pass (B11) has run on the region; `0` binders are
   omitted.

The verdict carries the first failing reason and the subterm, so a user asking `#fo t` sees
why a region stays in the net.

#### 4. Strictness

Native code evaluates strictly except at two kinds of position, which the first-order form
marks:
- **Codata.** A value of a codata type is a stuck call; native code represents it as a
  closure (the definition's code pointer with its arguments) and unfolds it only when a
  projection is applied, to the depth the projection asks. The delay type for partial
  computation is codata and is run the same way. A region that forced a codata value fully
  would loop; the form never emits such a force.
- **Unchecked terms.** Strict evaluation agrees with the net only on *checked* closed terms
  of inductive type (section 13 of the plan). An unchecked region is still compilable, and
  the form marks each unchecked definition so the schedule can keep it lazy (a thunk per
  argument) if the user asks; the default for unchecked regions is lazy.
Everything else is strict: arguments evaluated before calls, fields before construction.
Totality of checked terms is what makes this sound.

#### 5. Codata at the boundary

A codata value crosses the boundary as a closure, never as data. A process (plan 7.1) is a
codata value whose unfolding issues commands; the driver loop (D26) projects it step by
step. So a region may return a process, and the process's transition function, being a
first-order region itself, compiles; the stream structure stays a closure.

#### 6. Schedules

A schedule is a value of a data type the specializer reads:
```
data Sched = sched (strict : Strictness) (stages : Assign) (reps : List (type × name))
                   (fuse : Nat) (inline : Nat) (par : Par) (generality : Generality)
```
- `strict`: `strict | lazy | per-definition list`, constrained by section 4;
- `stages`: the binding-time assignment, free to set a variable later than it is known
  (generality, plan 5.2), never earlier than it is known;
- `reps`: the representation chosen per type (B12 §2); default the first declared;
- `fuse`, `inline`: depths; `par`: which `par` annotations the back end honours;
- `generality`: `specialized | image` as the two named assignments of plan 10.1.
Every field preserves meaning by construction (totality, equivalences, locality), so two
schedules of one term give convertible results; `#same` across schedules is a test. A
schedule is attached by `schedule f = sched …` beside the definition, or passed to `#compile`.

#### 7. What the specializer (B15) does with the verdict

For each definition and schedule: substitute the `now` variables, normalize to the fuse
and inline depths applying marked theorems under the terminating discipline, run `usage`,
run `first-order`; `fo` regions go to the first-order form (B14); `not-fo` regions stay in
the net and are called from native code through the call boundary (B18) as closures over the
net's value cells. A definition that is entirely `not-fo` is reported with its reason.

#### 8. Acceptance

1. `#fo plus` on `Nat` as `Word`: `fo`. `#fo (λ i → loop i)`: `not-fo dim`. `#fo (transport
   (ua not) true)` after specialization: `fo` (reduces to `not true`); before: `not-fo kan`.
2. `#fo rec2 Int isSetInt f x` with `x` a `run` variable of `∥ℤ∥₂`: `not-fo higher`; with
   `represent ∥ℤ∥₂ by Int` declared: `fo`, boundary `Int`.
3. `#fo check` with its code argument `run`: `not-fo reflect` at the first `%view`;
   `#fo quote-top` and `#fo eval` likewise (their boundary is a live value). `#fo kcode`
   (F34's hash over codes): `fo`.
4. A stream `nats : Str Nat` returned from a region: `fo` with a codata boundary; the form
   marks it lazy; `nth 10` on it compiles as ten projections.
5. `#same` of `fib 20` under `sched strict …` and `sched lazy …` agree.

#### 9. Changes in revision 2 (critical pass)
- Criterion 1 restated: no interval binders or variables remain, since paths are not data.
- Criterion 5 corrected: `eval` and the checker's decision run on the net; only code-to-code
  programs compile. Acceptance 3 corrected accordingly.

#### 10. Size

`lib/check.cub` or a new `lib/fo.cub`: `first-order` ~250 lines; `Sched` and its reader
~80; `#fo`, `#compile` in `src/syntax.c` ~80. About 410 lines. B14 and B15 are separate.


### 18.4 F34 — The store as the codebase: two keys

Spec. Revision 2 (after a critical pass; changes at the end). Against scoped-machine `9b94b43`.

#### 1. What exists

- **Runtime keys** (`src/core.h`, "keys: terms as names"): a producer's key is its compiled
  body with each free position replaced by the key of the value bound there; a value's key is
  its constructor over its parts' keys; a box is named by what it holds. These are the memo's
  names. They depend on the compiler (`lang.c`) because the body is the *compiled* form.
- **Module cache** (`lang.c` 2211, 3213–3306): a running key hashes the machine's build
  (`__DATE__ __TIME__`) and every file loaded so far, in order; a module whose key has a
  marker in `.cubical-cache` replays its recorded elaborated forms without rerunning checks.
  The key is per file, positional, and invalidated by any rebuild or any earlier file change.

Neither is a durable identity for a definition. The first changes with the compiler; the
second changes with file order and the build date.

#### 2. The codebase key

For each checked definition `d`, its **codebase key** `K(d)` is the hash of:
- its elaborated code (the `Code` value, after the elaborator's hole solutions, with its type
  annotation), in which every reference to another definition is replaced by that
  definition's codebase key (so `K` is recursive, like the runtime key, but over *code*, not
  compiled cells);
- its multiplicity annotations (B11), which are part of its type; not its representation
  choice, which is a schedule and lives under `obj/<sched>`;
- nothing else: not its name, not its file, not its position, not the machine's build.

`K` is computed by a term over codes (`lib/store.cub`, `kcode : Code → Hash`), so it is itself
a checked program, and the hash is a fixed function (BLAKE3 or SHA-256; one, chosen once and
part of the store's format). Names are not in `K`: renaming a definition, or a definition it
uses, does not change its key.

#### 3. The runtime key as derived

The runtime key of a definition's compiled body is a function of `K(d)` and the compiler's
version: `R(d) = runtime-key(K(d), compiler-version)`. The store records the mapping
`K(d) → R(d)` per compiler version, so a cached compiled body is found by `K`, and a compiler
change invalidates compiled forms without touching identities. Today's memo keys continue to
be computed exactly as now; nothing in `net.c` changes. The mapping is written when a
definition is compiled and read when a module is loaded from the store.

#### 4. The store

A directory (or a database later) keyed by `K`. Under `K(d)`:
```
code        the elaborated Code (the identity's content)
type        its type's Code
check       the checker's verdict and the step count (a checked definition's certificate)
usage       B11 binder usages
nf/<sched>  normal forms computed under a schedule (optional)
obj/<cv>/<sched>  compiled forms per compiler version and schedule (B14–B16 output)
cxx/<sched> the C++ image (B19) per generality assignment
proofs      keys of definitions whose statements mention K(d)
```
A separate table `names`: `path/name → K`, with history: each rebinding appends
`(name, K_old, K_new, time)`. A module file is then a list of names and, on load, resolves
each to its `K`; a definition whose `K` is in the store with a `check` entry is not rechecked
(this replaces the positional module cache, and the recorded elaborated forms move under `K`).

#### 5. Operations (`src/lang.c`, `lib/store.cub`)

- `load`: for each definition in a file, compute `K` after elaboration; if present with
  `check`, replay the stored code and skip the check; else check, then write `code`, `type`,
  `check`, `usage`.
- `#key d`: print `K(d)`. `#names K`: the names bound to it, now and historically.
- `#history name`: the rebinding log.
- Later, with F31: `#equiv d d'` records a path `d ≡ d'` under both keys in `proofs`;
  `#transport-to d'` rewrites a dependent's references along it. Not part of this item.
- Fetch by key over a wire (G35): out of scope here; the format is what it will fetch.

#### 6. Retention

Entries live while reachable from the `names` table or from a `proofs` link of a reachable
entry; `#gc` removes the rest. This is the memo's rule applied to the store: reachability
from the named corpus, no policy.

#### 7. Hot loading and state

Rebinding a name changes what new instantiations use; a running process keeps `K_old` until
its last holder goes. Migrating a process's state to `K_new` requires an equivalence between
the two state types; `#transport-to` applies it when given, and refuses otherwise. Nothing is
automatic here, by design.

#### 8. Acceptance

1. Renaming `plus` to `add` in `lib/std.cub` changes no key; `#key plus-comm` before and
   after agree.
2. Changing `plus-suc`'s proof changes `K(plus-suc)` and `K(plus-comm)`, and no other key in
   `lib/std.cub` that does not depend on them.
3. Loading `lib/std.cub` twice with a rebuilt machine between: the second load rechecks
   nothing (all `K` present with `check`), and the `obj` mapping is absent for the new
   compiler version until compiled.
4. Reordering files on the command line changes no key.
5. `#gc` after unbinding a name removes its entry and the entries only it reached.

#### 9. Changes in revision 2 (critical pass)
- Representation choice removed from the key (it is a schedule).
- `#equiv`/`#transport-to` moved out to F31; acceptance 5 dropped.

#### 10. Size

`lib/store.cub` ~200 (`kcode`, the hash over codes, the table formats); `src/lang.c` ~300
(store directory, `load` replacing the positional cache, the commands); `src/syntax.c` ~60
(commands). About 560 lines. The hash function: BLAKE3 reference C, ~150 lines vendored.


### 18.5 B14 — The first-order form

Revision 1. The target of the specializer and the input of every back end.

**Status (2026-11-06).** Built as `lib/fo.cub`, with these differences from the definition
below: `fswitch` alternatives carry their field count (`(c , (n , body))`); a recursion form
`frec d x alts` (alternatives `(c , (n , (hyps , body)))`, the hypotheses the suspended
recursions on the fields named, from the data table's shapes) stands beside `fswitch` and is
what `case`/`rec` codes lower to, `lift` turning it into a self-calling definition; the byte
layer is `fprim` with `P-load64 … P-blen` on `vbytes` values rather than `fbytes` literals;
`fi64`/`ff64` are not yet forms (FO holds words; the conversions are dropped at the
boundary); `fcall` is by global index (+1 in a program), not by codebase key (F34); `FT` is
declared and not yet attached to terms, so the printer boxes every value — the typed form
and B12's layouts are the next step here. The justifying path `eval-fo f ≡ t` as an
assembled proof term is produced for the word fragment (`lib/foproof.cub`, 2026-11-07: `spec-path`
for every code, `ex-path` for a term, from the verified rewrites and one lemma per form; §12 item
15), and for data, closures and calls (2026-11-07, `6f57a2aa`: `DC`/`FC` with fuel-indexed
evaluators `dsev`/`dtev`, closure conversion `dconv` checked, `dc-sound` for every fuel, code and
good environment given a right table — decided for a concrete program by `fc-eq-sound` and
`dtok-auto` — and `dx-path` for an example with a closure, a constructor, a case and a call;
`dc-defs` takes it to the first-order form, `run.py` program `proved-data` compiles it boxed and
unboxed: 15); the specializer takes the verified route where it can (`specialize-proved`: a
normal form whose body is in the word fragment is lowered by `spec-w std-vrule`, whose path is
`spec-path`, else the untyped `specialize`, the result tagged with which: the word acceptance
programs and `plusw x 3` from the store go the verified way, a case on `Bool` the untyped);
beyond those fragments its statements are read by computation, family by family
(`tests/fo.cub`'s justifying-path section, `62c669fc`: words, loops, pairs, data, closures,
thunks, the numeric and byte layers), as `#same`s — a `#check`ed path may mention checked
definitions only, and the first-order libraries are untyped by design. `fthunk` lifts to a
one-argument closure under `fthunk (fapply c [0])` (`7a2c96c4`). `lower` lives in
`lib/lower.cub` (after the checker, for the shapes). Delayed terms (2026-11-06): Code has
`cthunk t` (`code (delay e)` works, `eval-g` reads it as a delay); in a checked term `delay e`
is typed as `e` (`tthunk`) and is transparent at the machine — `e` held unbuilt (`V_THUNK`),
built by the operation that meets it, `force` of it is it — where before a checked `delay` was
refused (`ann` had no form) and a bare one met by `%wadd` did not compute. `lower` maps
`cthunk t` to `lower t` (the first-order form is strict and a checked term terminates; `fthunk`
stays for lift's suspensions); the image translates normal forms, which hold no delay, and
exports a definition written with them (`IMAGE=1 run.py` program `delay`, read back equal).

#### Definition
A data type in the language, `lib/fo.cub`:
```
data FO
  = fvar n                       -- de Bruijn, a run or run-known variable or a local
  | flit (w : Word) | fi64 n | ff64 bits | fbytes (bs : List Word)
  | fcon (d : Word) (c : Word) (fs : List FO)        -- constructor with its non-0 fields
  | fswitch (x : FO) (alts : List (c × FO))          -- case on a constructor tag
  | fproj (d c k : Word) (x : FO)                    -- field k of constructor c
  | fcall (g : Key) (args : List FO)                 -- a compiled definition, by codebase key
  | fprim (p : Word) (args : List FO)                -- a native primitive (%wadd, %load32 …)
  | flet (t : FO) (b : FO) | fif (c t e : FO)
  | floop (init : List FO) (body : FO)               -- tail recursion as a loop over state
  | fclos (g : Key) (env : List FO)                  -- a closure: code pointer and captured values
  | fapply (f : FO) (args : List FO)                 -- call through a closure
  | fthunk (t : FO) | fforce (x : FO)                -- lazy positions (codata, unchecked regions)
  | fnet (k : Key) (args : List FO)                  -- a call into the net (B18) for a not-fo region
  | fassume (fact : Code) (b : FO)                   -- a proven fact at this point, for the C++ image
```
Every `FO` is typed by a first-order type `FT`: `tword | ti64 | tf64 | tbytes n | tdata d
(ps) as R | tfun (args) ret | tlazy FT`, where `tdata` carries its chosen representation.
Multiplicity-0 binders and fields do not occur. There are no types, paths, dimensions or
universes in `FO`.

#### Properties
- `FO` is path-free inductive data, so programs over it compile, and `FO` itself has an
  automatic layout: the specializer, the printers and the certificate checker are first-order.
- Each `FO` program carries its justifying path: the specializer produces `(f : FO) × (p :
  eval-fo f ≡ t)` where `eval-fo : FO → Value` is the first-order evaluator written in the
  language and `t` the source term; `p` is 0 and erased, and it is what `#same` checks when
  asked to validate a compilation.

#### Acceptance
`eval-fo` on the `FO` of `plus`, `fib`, `sort` on numerals agrees with the net's readings
(`#same`); `#fo-print t` shows the form; `FO` round-trips through its own automatic layout.

#### Size
`lib/fo.cub` ~250 (the type, `eval-fo`, printer). Part of B15's 1,500 in the plan's count.

### 18.6 B15 — The specializer

Revision 1. A term over codes: `specialize : Code → Sched → Assign → (FO × Proof) + Report`.

**Status (2026-11-06).** `lib/specialize.cub`: `specialize T rules c` = normalize (eval,
then quote at `T`), rewrite along rules to a bound, lower; `specialize-s` takes the
schedule as a value; `program-of` lowers a code and the
definitions it calls; `fo-report` is the report of the criterion over codes; the rules come
from the store's identities (`lib/rules.cub`). Not yet: the schedule as a value with its
`fuse`/`inline` depths and generality, the assignment of binding times, usage (B11) and
the criterion (B13) applied, the ordered terminating discipline for rewriting (a count
today), the justifying proof.

**Finding (2026-11-06, `70854ad`, corrected at `7be3368`), binding on step 2 and on the
machine.** A value containing a stuck primitive application (`load64` on a neutral buffer)
has no quote (`quote-top` gives `none`: F31's missing `wnative`/stuck-call view), and
`normalize` then falls back to the source code, so a program over B12's byte layer cannot
be specialized under an unknown input — the whole of it, not only the primitive call.
(Normalization itself does unfold definitions applied to neutral arguments: `lenw l` and
`run4 st` normalize to their bodies; an earlier reading of the fallback as a kept call was
wrong and the inliner written for it was retracted.) The fix is a code form for stuck
primitive calls. Half landed at `be7149d`: a stuck operation is now a neutral `V_NPRIM`
that `%view` reads as `wprim k args` and the checker compares argument by argument (so two
loads of the same string at the same offset are equal, and C22's array law is proved from
the byte layer's); what remains is the code (`qneu` gives none for it): a constructor
`cprim k args` in `lib/code.cub`, `qneu` producing it, `eval-g` applying the operation, and
`lower` mapping it to `fprim` — ~60 lines, the first item of the queue. Until
then `lib/repr.cub` lowers the source code of such programs directly (calls of the
primitive definitions mapped to native primitives, lengths and proofs dropped). With no
primitives in the way, specialization under unknowns works today: a process of
`lib/proc.cub` run against its world with the store unknown normalizes to a function of the
store (`pr1`, which puts before it gets, to the constant 12; `pr4` to `st 3 + 1`), and the
latter compiles with a store closure (`run.py`'s `process`, 41).

#### Steps, in order
1. **Substitute** the `now` variables from the environment.
2. **Normalize** to the schedule's `fuse` and `inline` depths. This is the net's own
   normalization invoked on a code (`eval` then `quote`), so no second evaluator exists; the
   depth bounds are applied by the quoter stopping at calls below the depth.
3. **Rewrite** along marked theorems: a library of paths tagged `rewrite →`, applied left to
   right by matching on codes, under the terminating discipline: rules are oriented, a
   rule's right side is smaller under a fixed order (size, then a user-given rank), a rule
   whose sides are equal in size (commutativity shapes) is rejected at tagging time
   (**E-rule-unoriented**). Each application records the path.
4. **Usage** (B11) on the residual; drop 0 binders.
5. **Criterion** (B13) per region; `not-fo` regions become `fnet` calls.
6. **Lower** to `FO`: constructors by representation (B12), `case` to `fswitch`, tail
   recursion to `floop`, calls to compiled definitions to `fcall` by codebase key, calls to
   primitives with `op` entries to `fprim`, codata to `fclos`/`fthunk`.
7. **Prove**: assemble `p : eval-fo f ≡ t` from the recorded paths (steps 2–3) and the
   representation `by` paths (step 6); the lowering of steps 4–6 is justified by one lemma
   per `FO` constructor, proved once in `lib/fo.cub`.
8. **Store** `(f, p)` under `obj/<cv>/<sched>` of the definition's codebase key (F34), and
   `f`'s own key for the memo of compiled code.

The specializer specializes itself: its code is first-order over `Code` and `FO`, so step
6 applied to `specialize` yields its own `FO`, compiled by B16; `#fo specialize` is `fo`.

#### Progressive specialization (B17)
At run time, a `run-known` variable receiving a value re-enters step 1 with that value
`now`, producing a new `(f, p)` under a new key, which the JIT (B16) compiles and the call
boundary installs for later occurrences. The schedule says which variables are `run-known`.

#### Acceptance
`specialize` of `plus` on `Nat as Word`: `fprim %wadd`; of `map f (map g xs)` with the
fusion rule tagged: one loop; of `transport (ua not) b` with `b` run: `fswitch` on `b`;
of `g10` in the Brunerie port with `x` run: `fnet` for the truncation, `fcall` for
`rec2`'s target with `represent ∥ℤ∥₂ by Int`: `fo` throughout. `eval-fo` agrees with the
net on each.

#### Size
~1,200 lines of `.cub` (`lib/specialize.cub`) plus the rewrite tagging in `src/syntax.c`
(~60).

### 18.7 B16 — The C back end and the JIT

Revision 1.

**Status (2026-11-06).** Milestone 1 built as `lib/emitc.cub` (`emit-prog`): C source as a
list of bytes with a small runtime of boxed values (`V {tag, n, f[]}`: a word, a
constructor with its fields, a byte string with the bytes in `f[]`); one C function per
definition, GNU statement expressions for `let`, `switch` and loops, closures through a
generated dispatcher (`apply`), `ASSUME` for `fassume` (`__builtin_assume` on clang,
`__attribute__((assume))` on gcc ≥ 13, nothing otherwise), `ld64`/`st64`/`ld32`/`st32`/`mkb`
for the byte layer, hypotheses of recursions as eager lets. `lib/emitt.cub` prints a data
type's layout as a typedef. Milestone 2, first part (`89aed69`, `lib/emitu.cub`): words
unboxed — every local, parameter and return typed `uint64_t` or boxed by inference
(parameters by their uses, returns by their bodies, three rounds; conversions where the
types differ; closure targets boxed for the dispatcher); a loop of `lower-welim`'s shape is
a `for` over typed state with no allocation. Two loads from a buffer print as
`uint64_t f(V *b) { return ld64(b, 16) + ld64(b, 0); }` under their `ASSUME`s; factorial
is a `for` over two `uint64_t`; pairs are C structs by value (`63aa45e`), so `fib` by word
elimination over a pair is a `for` over a `uint64_t` and a `Pr0` with no allocation. Flat data
types (a union of structs of words, by B12's layouts) are `lib/emitt.cub`'s typedefs by
value (`1a843be`): compound literals, field projections, a switch on the tag binding
`uint64_t` fields, `box_d`/`unbox_d` generated where a `V` is wanted or given. Recursive
and pointer-carrying types (lists, trees) remain boxed `V`: typed pointers to their own
structs, with the same layouts, are what is left of milestone 2. `experiments/fo/run.py` compiles fourteen programs with gcc at
`-O2` and runs them (factorial, a switch, a specialized loop, a closure, a law consumed, an
assumed fact, the typedefs, the byte layer, loads with proofs, a recursion, a derived
equality, a whole program from the store, a process against its world). `fthunk` in both
printers (`7a2c96c4`): a thunk is a closure cell tagged `2000000 + g` through the dispatcher
(which switches on the tag modulo 1000000), `force()` applies it to 0 once, stores the value
in its first field and retags it `3000000`, and leaves any other value alone (lifted recursion
hypotheses are read through `fforce` too). The JIT's own encoder, `lib/emitx86.cub`
(`54be3050`): the word-only fragment (fvar, flit, add/sub/mul/eq/lt, let, if, loops with their
continue/break cells, calls up to six arguments, constructor cells of words in a bump arena with
projections as loads) to one position-independent x86-64 blob — a stack machine over rbp
slots, an entry stub setting r15 (the arena) and r14 (the blob's base, calls base-relative),
the stack 16-aligned at calls, offsets from a measuring pass; `x86-ok` says whether a program
is inside the fragment. `experiments/fo/x86main.c` copies a blob into executable memory and
calls it; `X86=1 run.py` runs plus (by call and by the stub's arguments), factorial, plusw's
specialized loop, fib 90 through pairs, a call inside a loop. Not yet: `fnet` natively, the
codebase keys in the file header, CompCert, SSE2 floats and `fswitch` in the encoder, the
acceptance programs at size (the sort, the decoder from the boundary).

#### C
`emit-c : FO × FT → CSource`, a term over `FO` producing a string (the fragment of C it
emits is: structs, unions with tags, `switch`, `for` loops, function calls, fixed-width
integer and float arithmetic, `memcpy` for relocation, no undefined behaviour by
construction given valid representations). One C function per `fcall` target; `fclos` as a
struct of a function pointer and its environment; `fthunk` as a struct with a computed flag;
`fnet` as a call into `libcubical`'s boundary (B18). `fassume` prints `__builtin_assume`
(gcc/clang) or `[[assume]]` (C++23) in the image; nothing in plain C99 for CompCert, where
it is dropped. Representations' layouts print as their structs. Every emitted file begins
with the codebase keys it was generated from.

Two compilers: gcc/clang at `-O2`; CompCert for proof-critical builds, with `fassume`
dropped and the fragment restricted to what CompCert accepts (C99 without VLAs).

#### JIT
`emit-x86 : FO × FT → Bytes`, the same lowering to machine code for the `run-known` path,
emitted into executable memory with a fixed calling convention matching the boundary's.
First target x86-64 System V; a second target is a second `emit-*` over the same `FO`.
The JIT emits straightforward code (no register allocation beyond a linear scan); the C
path is for peak performance, the JIT for latency of re-specialization.

#### Acceptance
`fib 30`, `sort` on 10⁶ words, the ITCH-shaped decoder from the codec example, compiled
through gcc and run from the boundary: results equal the net's; `#same`. The same `FO`
through CompCert compiles and agrees. The JIT on `plus` and the decoder agrees.

#### Size
`lib/emit-c.cub` ~600; `lib/emit-x86.cub` ~700 (encoder for the instruction subset used:
mov, add/sub/imul, cmp/jcc, call/ret, load/store with offsets, SSE2 for floats).

### 18.8 B18 — The call boundary and the C ABI

**Status (2026-11-06).** Both directions built for words. Net → native: `#native name n =
prog` (`src/lang.c`, `src/syntax.c`) takes the bytes of `emit-lib-u defs` (`lib/emitu.cub`,
`c7f3cfc9`: the unboxed program with definition 0 exported as `cubical_entry`), compiles them
with `cc -shared` into the cache directory, opens the library as a foreign entry and
redefines `name` as `λ a₁…aₙ → %native k g a₁…aₙ`; the `O_NATIVE` rule (`src/net.c`) reads the
arguments in turn and calls the compiled code when all are words, else applies the
definition `g` itself (so `λ x → plusw x #0` still reads `λ x → x` on the net).
`tests/native.cub`: `plusw` entered (23.7k steps to specialize, lower and print; 3.3 KB of C),
`plusw #3 #4`, `plusw #1000000 #1000000` native; a byte string crosses by the entry's prototype
(`V *` parameters and results are the emitted runtime's boxed bytes, copied in and out; `sum2-def`
over `Bytes 32`). Native → net: `src/libcubical.h`,
`build/libcubical.a` (the sources with `-DCUBICAL_LIB`, `main` renamed), `cubical_init(files)`
and `cubical_call_w(name, words, n, &out)` (the application read back by the net, a word or a
Bool); `tests/native/main.c` calls `plusw` and `eqw`, in `make test`. Not built: marshaling
of values of declared representations by their layouts (`tdata` walks, `Bytes` as pointer
and length), exported prototypes per compiled definition, the
`foreign` → `fprim` lowering; `#native` is per definition on demand, not a cache by key
(item 17). The foreign side of D27 (`O_FFI`) is what `O_NATIVE` reuses (`ffi_invoke`).
`V_NATIVE` (2026-11-06): a cell holding a refcounted `NObj`. (a) Opaque handles: `#native f n
from "lib.c" "sym"` enters a hand-written C file's function (compiled once by the text's hash);
a `void *` result is a handle of the primitive type `Native` (`%pty #5`, `lib/prim.cub`), a
`void *` parameter takes only handles the same library made (another library's, or any other
value, falls back to `f` on the net — for a postulate, stuck); the library's `cubical_release`,
when it defines one, runs exactly once when the last copy is erased (the count reaches 0 once;
arguments are erased after the call, so a handle passed as its last copy outlives the call).
Nothing else reads a handle: an eliminator meeting one is stuck; a reading prints `(%handle #k)`
(k the entry that made it, so readings are the same at any schedule) and drops its copy; `%view`
gives `wnative id k` (`lib/code.cub`), id a serial number unique for the run, and the checker's
conversion compares ids. The entries are trusted to treat a handle as read-only (the net shares
it). (b) Boxed results kept native: `#native f n keep = prog` keeps a `V *` result as the
emitted runtime's value (a copy the cell owns, freed by the data type's own walk), passed to the
next compiled call as a copy with no cells built; any other reader unboxes it into cells
(`native_unbox`). The schedule is the programmer's word `keep`, not an analysis. Tests:
`tests/native.cub` (handles made, shared, finalized: `buf-live` 0 after every reading; `consk`
kept and passed to `lenw`), TSan clean on the handle tests at 4 workers. Found on the way: a
memo race (an entry freed by a reader that found its box dying while the box's releaser was
about to read it, `entry_hold_box`/`memo_begin`), fixed.

Revision 1. In `src/net.c` and `src/lang.c`.

- **Net → native.** A cell `O_NATIVE` (`0..n-1` = arguments, `out`): when all arguments are
  constructor-built values (checked by the same readiness the `O_CALL` rule uses), marshal
  each by its representation's layout into native memory, call the compiled function (found
  by codebase key in the compiled-code memo), and unmarshal the result into cells by its
  layout, or wrap it as an opaque `V_NATIVE` cell when the schedule says the result is
  consumed by other compiled code. Marshaling of a `tdata` value is a walk over its
  constructors; of a `Word` it is the word; of a `Bytes` it is a pointer and a length.
- **Native → net.** `fnet k args` calls `cubical_call(k, args)`: build the `O_CALL` for the
  definition `k` with marshaled arguments, run the net to normalization of its result (a
  reading), unmarshal. This is how a `not-fo` region inside compiled code is reached. It is
  synchronous; a compiled process that needs the net blocks on the reading.
- **Exported C functions.** For every compiled definition over declared representations,
  `emit-c` also prints a C prototype with the representation's struct types, and
  `libcubical.h` declares `cubical_call`. A C program links `libcubical.a` and calls either.
- **Foreign C in.** A `primitive` definition with a declared type and a C symbol (`foreign
  "memcpy" : …`) becomes an `fprim`; its validity facts are the declared type's and are
  trusted (`#trusted` lists it).

#### Acceptance
A C `main` calls the compiled `sort` on an array and prints it; the net calls `sort` through
`O_NATIVE` and reads the same; `fnet` from compiled code into `check` returns the checker's
verdict on a code. `make test` unchanged.

#### Size
`src/net.c` ~250 (`O_NATIVE`, marshaling), `src/lang.c` ~150 (`cubical_call`, prototypes,
`foreign`), `src/syntax.c` ~40. About 440 lines.

### 18.9 B19 — The C++ image

**Status (2026-11-06).** Built (`ee5c04a2`; `lib/image.cub`, `lib/reimport.cub`), `IMAGE=1 run.py`
compiling five headers with g++ at `-std=c++20` and re-importing each definition equal to its
source up to erasure. Parameterized data types (`13ae6b9f`): a type parameter is a template
parameter of the structs (`template<class P0> struct D; … struct D_c1 { P0 f0;
std::shared_ptr<D<P0>> f1; }; … std::variant<D_c0<P0>, …>`), the field types the constructor
telescope's quoted codes over the parameters; `D<args>` as a type; a constructor at the expected
type's arguments (`D<T>{D_c<T>{…}}`, reported when the type is unknown); a case on such a type a
fix over `const auto &x` whose alternatives are template lambdas `[&]<class Q0>(const D_c<Q0> &k)`;
the re-import reads all of it back (template struct declarations skipped, the scrutinee's type
from the alternatives). A parameter at a value (an index) stays outside the fragment.
**The rest of 18.9 (2026-11-07, fork C5)**, each a program of `IMAGE=1 run.py` compiled by g++
-std=c++20, run, and read back equal to the source (`def-src`: a copattern definition's record as
written, any other definition's value):
- *Concepts from records* (`concept`): a parameter whose type is a non-dependent Σ with an
  operation among its components is a template parameter `C<i>` constrained by a concept
  generated from the components' signatures (`template<class C> concept g_k<i> = requires(const C
  &c) { { c.f0(std::declval<A>()…) } -> std::convertible_to<R>; { c.f1_ } -> …; }`); a projection
  is the member, an operation called with all its arguments at once. A checked definition of such a
  record (an instance) is `struct [[cub::instance]] g_t { R f0(A v0, …) const { return e; } T f1_
  = e; }; inline const g_t g{};`. The consumer folds with the image's instance and with its own
  struct satisfying the same concept (`static_assert`ed).
- *Codata as classes* (`codata`): a codata type without parameters is a class with one lazily
  computed cell per field (`std::shared_ptr<cub::lazy<T>>`: computed at the first observation,
  then kept) and the observations as member functions `D<d>_o<k>()`; a copattern definition's
  record is `D<d>{cub::delay<T>([=]() { return e; }), …}` (the checked group keeps its body as
  written, `%gcode`, since its value is its stuck head); the re-import reads the record back.
- *Representations' classes* (`repcls`): a type with a declared representation is `struct D<d> {
  R r; static D<d> from(R); R to() const; static D<d> of(const D<d>_v &); D<d>_v data() const; }`,
  its data form the structs `D<d>_v`/`D<d>_c<k>`; constructors are `D<d>::of(D<d>_v{…})`, cases
  visit `(x).data()`, through the representation's own conversions (`%rops`), exported with it.
  The class is the size of its representation (`static_assert`ed).
- *The module file* (`module`): `cxx-module name …` is the image as a C++20 module interface unit
  (`module;` and the includes, `export module name;`, the helpers and the image exported),
  compiled with `-fmodules-ts` (g++ 13.3) and imported by the consumer; read back from its text.
Not in the re-import fragment, checked by running instead: primitive arithmetic on Int64, UInt32
and Float64 (printed as the C++ operator, read back as word arithmetic).


Revision 1. `image : Code × Sched → CxxSource + Report`, a term over codes. The schedule's
`generality` is `specialized` (then `image` = `emit-c` over `specialize`'s `FO`, printed as
C++) or `image` (the compositional translation below). Mixed assignments are per binder.

#### The compositional translation, one clause per source form
| source form | C++ |
|---|---|
| definition `f : T = t` | one entity in namespace from the name table; `// key: K(f)` as an attribute comment |
| `(A : Type) → B` with `A` at `build` | `template<class A> …`; `(n : Nat) → B` with `n` at `build` | `template<std::size_t n>` |
| `(x : A) → B` with `x` at `run` and `B` not depending on `x` | a parameter |
| `(x : A) → B` with `B` depending on `x` at `run` | `B` erased to its run-time shape; the dependency becomes a `[[assume]]`/contract (B11's `0` field cases erase cleanly) |
| record of operations with laws, as a parameter | `template<Concept C>`; the concept generated from the record's operation signatures; laws per 10.1 |
| instance of such a record | a `struct` satisfying the concept; instance resolution → the argument at the call |
| function over codes matching on codes | partial specializations of a class template, or `if constexpr (std::is_same_v<…>)` chains, chosen by whether the code set is closed |
| `data` without a representation | `struct` per constructor; `std::variant<…>`; `case` → `std::visit` with an overloaded lambda; recursive fields `std::unique_ptr` |
| `data` with a representation | the representation's class (B12) |
| `Σ (A : Type), …` hidden type, finite code set | `std::variant`; open | an abstract base with virtual projections, or a `function_ref` when it has one operation |
| codata | a class with the copattern projections as members; a process → a sender type whose `connect` builds the operation state from the guarded tree |
| `let` | a local; `fif`/`fswitch` | `if`/`switch`; `floop` | `for` |
| `0` binders and fields | absent |
| `1`-linear binders | move-only parameter types with `[[nodiscard]]` |
| paths | erased; representation equivalences → `to`/`from` members; laws per 10.1 |
| `fassume` | `[[assume]]` |
| `fnet` | a call into `libcubical` (the image can also be built without the net: then `fnet` is a report line "not exportable: needs the net") |
| modules | headers, one per module, `#pragma once`; a `module` file when the consumer's compiler supports C++20 modules |

Names: the source identifier, converted to C++ rules (`plus-comm` → `plus_comm`); Unicode
operators to named functions.

#### Validation of the image
`reimport : CxxFragment → Code`, a parser for exactly the fragment the image emits (not
C++), followed by `#same` of the re-imported code against the source up to erasure. Run
on every exported definition when `#image --validate` is asked. This is the fragment's
semantics (plan 10.1), given as a parser into our own codes rather than as a separate
semantics, which is what makes it one document and one truth.

#### Acceptance
The image of `map`, `fold`, `sort` with a comparator record: a header-only library with a
`template<class A, Ord<A> C>` sort; instantiated by a hand-written `main` with `int` and
`std::less`; compiles with gcc 13 at `-std=c++20`; `reimport` of the header equals the
source up to erasure. The image of the ITCH-shaped decoder over a handler record: a
template over the handler; a hand-written handler inlines into the decode loop (checked by
inspecting the assembly for one message type). The image of a `Str Nat` process: a sender
usable with stdexec's `sync_wait`.

#### Size
`lib/image.cub` ~2,400 (the clauses, concept generation, name conversion, the laws' four
carriers); `lib/reimport.cub` ~900 (the fragment parser). About 3,300; the plan's 3,500
stands.

### 18.10 C20–C22 — Memory and state

Revision 1. Mostly consequences of B11–B12; what is left to write:

- **C20.** The `Bytes n` primitive with `%loadN`/`%storeN` at offsets and the validity
  obligation `off + N ≤ n` (B12 §3); `Region` as `Bytes n` threaded as state (a region is
  passed and returned; in-place when unique by the structural rule); addresses as `Word`;
  `%ptr-of : Region → Word` and `%region-at : Word → (n : Nat) → Region` as trusted
  primitives for memory the program does not own (device, shared), listed by `#trusted`.
  `safe` as a checker command: `#safe f` reports every assumed validity fact, oracle use and
  foreign call reachable from `f`. ~200 lines across `src/net.c`, `lib/prim.cub`, `check.cub`.
- **C21.** Nothing to build in the net (structural). In `FO`/B16: `fcon` into a `flet`-bound
  value whose `FT` says unique → the C printer emits an in-place field store instead of a
  new struct (constructor reuse); the uniqueness of an `FO` local is static by B11's
  annotation or by the specializer seeing a single use. ~80 lines in `emit-c`.
- **C22.** `represent (Fin n → A) by Array n A` in `lib/prim.cub` with `index`, `tabulate`,
  `update` and their laws (`index (tabulate f) ≡ f` by funext; `tabulate (index a) ≡ a`;
  `index (update a i v) j ≡ if i == j then v else index a j`); `mdspan`-shaped layouts as
  `represent (Fin n → Fin m → A) by Array2 n m A layout`. ~250 lines of `.cub`, with
  `%aindex`/`%aupdate` primitives in `net.c` (~80).

### 18.11 D24–D27 — The world

Revision 1. Library over existing codata, plus drivers in C.

- **D24, `lib/proc.cub`.** Started (`92f5aa4`): `Sig`, `Proc S A` in Church form (a process
  is what it does to any interpretation of return and command: `(R : Type) → (A → R) → ((c :
  Cmd S) → (Resp S c → R) → R) → R`, needing no positivity rule and covering every
  terminating process), `ret`/`cmd`/`bind`, `World S St` as a state handler that may refuse,
  `runp` into `Outcome A × St`; readings over a key-value world (puts and gets, a refused put
  stopping the process, the monad laws by computation); handlers (`343d15f`: `Handler S W`,
  `handle`, a logging handler into a larger signature run in a counting world); supervision
  (`383f8c5`: a refusal carries the state after it, `Strategy A = Outcome A → Nat → Action`,
  `supervise` restarting from the state the world was left in, a flaky world refusing twice
  then accepting); replay (`recording` and `replaying` worlds: a run reproduced from the
  responses alone, refusing where the record ends — 7.3's replay as a term). The guarded form below, for processes that run forever, waits on 18.18
  (confirmed: `data Pr = pend | psend (n : Word → Pr)` is rejected as not strictly
  positive). Spec as written:
  `Sig = (Cmd : Type) × (Resp : Cmd → Type)`; `Proc S A` as the
  guarded free monad: `codata Proc S A = step : (c : Cmd S) × ((r : Resp S c) → ▸ Proc S A)
  + A`. `bind`, `issue c`, `done a`. Signature inclusion `incl : Sig → Sig → Type` as a
  function on commands with a function back on responses; `lift : incl S T → Proc S A → Proc
  T A` by transport. Handler: `Handler S W A = Proc S A → Proc W A` (a world is a handler
  into the drivers' signature `W`); composition is function composition; the fused
  simulation is `normalize (handler model (strategy))`. Session types: a protocol as a
  codata type of allowed next messages; a process typed by it. Choreographies: a global
  process over a signature indexed by participant; `project : (role : R) → Global → Proc
  (S role) A`, defined for globals meeting the projectability check (a predicate over codes,
  rejects otherwise); the configuration value includes `role`. Push: `recv : Cmd` with
  `Resp recv = Event`. ~600 lines.
- **D25.** `Outcome A = done a | stopped | failed Reason`; `spawn : Key → Cmd`, `await :
  Handle → Cmd` with `Resp (await h) = Outcome A`; `supervise`/`watch` as in the plan's
  sketch; restart strategies as functions `Outcome A → Action`. External kills arrive as
  `failed external` from the driver. ~150 lines in `lib/proc.cub`.
- **D26, status (2026-11-06).** Built as `lib/world.cub` and `src/drivers.c`: the signature
  `W` (`w-now`, `w-write fd n b`, `w-read fd n`, `w-sleep ns`, `w-exit code`, responses by
  `w-resp`), the commands as processes (`time-now`, `write`, `read`, `sleep`, `exit`), a
  process as a tree of steps (`data Step S A = sdone a | scmd c (k : Resp S c → Step S A)`,
  positive by 18.18; `to-step p = p (Step S A) sdone scmd`), and the loop `#run e`: the head
  observed, the command's fields read (words, Nat numerals, byte strings), the system call
  made, the response built as a value and the continuation applied, until `sdone` (printed
  with the loop's own steps); `w-open n path flags mode` and `w-close fd` for files.
  `tests/world.cub` writes `hello` to the standard output and to `/tmp/cubw.txt`, in `make test`.
  The memory interface is `lib/proc.cub`'s `MemS`/`atomically` (library STM, `bf3ea4eb`);
  `mmap`, `poll` and hardware atomics on Regions are not offered yet (no memory model
  chosen). Spec as written:
  The driver signature `W`: `now`, `send fd bytes`,
  `recv fd` (blocking, or `poll` over a set), `mmap`, `load`/`store`/`cas`/`fence` on
  `Region`s with the model named `release-acquire` (SC at `fence seq_cst`; relaxed not
  offered until a model is chosen), `atomically` as a handler in `lib/proc.cub` over
  `load`/`store` with a log and retry (a library STM, not a hardware TM). The driver loop:
  project the process's `step`, dispatch the command to the OS, supply the response as an
  input (`#supply`), repeat. ~500 lines of C.
- **D27.** `foreign "symbol" : T` (B18) and `world "name" = handler` naming a handler
  implementation a binary links against; `#trusted` lists both. ~60 lines.

### 18.12 E28 — Elaboration

Revision 1. In `src/syntax.c` and `lib/check.cub`, gated by A10b (the typed port tells
which of these it actually needed; the list is what Agda-level convenience means).

- Implicit arguments by unification: holes solved by first-order unification against the
  expected type during `check`, not only syntactically; postponed constraints retried once
  per definition. ~400.
- Instance resolution: `instance` declarations for records of operations; an implicit of a
  record type is solved by searching instances in scope, depth-first, with a bound. ~250.
- Records with η: `record` elaborates to Σ with η in `conv` (exists for Σ); named-field
  construction and update syntax `r { f = v }`. ~150.
- Parameterized modules: `module M (A : Type) (ord : Ord A) where …` elaborates to
  definitions abstracted over the parameters; `open M Int ordInt` instantiates. ~250. With
  them, namespaces: today a name declared by a later module rebinds the references of every
  module parsed after it, silently (the two bugs of 2026-11-06, FT's `tword`/`tdata` and
  linproof's `normalize`); until modules land, redeclaring a name in scope is an error
  (`src/lang.c`, ~20 lines) and `tools/names.py` enforces uniqueness across `lib/`.
- Induction hypotheses for path-typed recursive fields (18.17): required by A10b; the
  truncation recursors are trusted primitives until it lands. ~120.
- Landed early by the builder under A10b: a written λ binder type informs the implicit
  arguments of the function it is passed to; an implicit function applied to a bound
  variable is solved as a pattern (`B x` against `T` gives `B := λ x → T`).
- Indexed families by fording: DONE (2026-11-06, `src/syntax.c` `Ford`, `lib/check.cub`
  `uniform-params`, `%dnidx`): `data Vec (A : Type) : (n : Nat) → Type where vnil : Vec A zero;
  vcons : (n : Nat) → A → Vec A n → Vec A (suc n)` declares the indices as the last parameters
  (`%ix0 …`), each constructor gets an equality field per index (`PathP (λ m → Idx) %ix0 e`, at
  multiplicity 0), the strict-positivity uniformity check skips the indices, an application
  without the equality fields gets `refl` at the declared index expressions, a pattern without
  them is padded; written out, the field is transported along or refuted (`vtail` by `snotz`).
  Not inserted: transports in matching (the branch sees `e : m ≡ e'`), and recursion on an
  indexed parameter with an earlier explicit index changing (`vlen (n : Nat) …` with `vlen m xs`)
  is outside the structural rule's earlier-parameters-fixed shape — write the index implicit.
  `tests/elab.cub`. Spec as written: `data Vec (A : Type) : Nat → Type where nil : Vec A zero ;
  cons : …` elaborates to a parameterized type with an equality field per index
  (`cons : (n : Nat) → A → Vec A n → (m ≡ suc n) → Vec A m`), with the equality at
  multiplicity 0 so it is erased; matching inserts the transports. ~300.
- The interval-literal fix: `0`/`1` as interval arguments when the expected type is `I`. ~20.
  DONE (already accepted on `main`, 2026-11-06: `p 0`, `p @ 1`, `p i0` all check).
- **Status (2026-11-06).** Redeclaring a checked name is an error (`sig_add`,
  `CUBICAL_REDECLARE=1` allows it); two test names renamed (`tlist`, `rt-arr`). Fatal errors
  print on stdout as well when it is not stderr (iii, and the side agent's silent-exit
  finding). A scrutinee rebuilt from a branch's fields (`case k of suc j → … k …`) infers:
  a constructor of a parameterless data type infers its type (`infer` in `check.cub`); for
  a parameterized one the rebuilt mention still needs an annotation. Not accepted, by
  design: a Π over `I` in a signature (`I` is not a type; fillers are stated as `PathP`); a
  pattern named like a constructor in scope (`ok` is `Res`'s) is that constructor.
- Found while writing `lib/linproof.cub` and `lib/eqproof.cub` (2026-11-06): (i) FIXED
  (2026-11-06, `lib/check.cub` `infer` of `twelim`, `src/syntax.c` `sol_fill`, `el_app`): the
  implicit endpoints of a composed path (`catw`, `congw`) are now determined through a
  hypothesis whose type only reduces to the needed form (`h : P n` inside a `%welimP` step, `h :
  motw b n` for a defined `motw`, a λ applied in a signature). The cause was not matching: the
  solution's value was a stuck word recursion (`plusw b n`), whose code has no motive, so its
  quotation carried a hole the next round could not fill; a hole motive of a `%welimP` is now
  solved as the constant motive at the zero case's type (the non-dependent case every quoted
  recursion is), a solution's own holes are filled by their solutions once found, and a λ at
  the head of an application is reduced in the surface syntax (the checker does not infer a
  λ's type). `tests/elab.cub` (`plusw-comm-e`, `step-e`, `step-l`); the explicit endpoints in
  `linproof.cub`/`eqproof.cub`/`intproof.cub` stay as written. Also 2026-11-06: holes in
  composition lines (`comp _ u a`: the constant line at the type expected, or at the base's type;
  `check`'s `tcomp` case and `comp-hole-line`; a composition at `Word` with an empty tube is its
  base, `O_COMPW`), generalized variables (`variable n : Nat`; `autobind` binds the variables a
  signature mentions free, with those their types mention, in declaration order), and the index
  unified at the split for recursion on a fording family (the clause compiler writes the motive
  of a split on `v : D … n` with variable indices as `(λ ix… v → R[n := ix]) n…`; `hyp-motive` in
  `check.cub` reads that shape and types a recursive field's hypothesis at the field's own index,
  `vmap f xs : Vec B m`; before, the hypothesis was at the outer index, which let `vbad f (vcons
  m x xs e) = vbad f xs` through at `Vec B n` — a soundness hole, now closed, `tests/elab.cub`).
  The goal rewritten by the index equation (2026-11-06): a clause that leaves the equality
  fields out (`vmap f (vcons m x xs) = vcons m (f x) (vmap f xs)`) has its branch checked at the
  constructor's own index with refl for the equations and transported along them to the motive
  at v, the line `λ i → R[ix := e (~ i), v := c fs (λ j → e (~ i ∨ j))]` (`ccm_t`, skipped when
  the goal does not depend on the index); naming the field keeps the old reading. Soundness
  (found in review, 2026-11-06, `f6739ad7`): a recursion on a fording family whose motive is not
  over the indices (an index argument that is not a variable, `Vec A (suc n)`, or a motive
  written by hand) still typed a recursive field's hypothesis at the outer index, so `sbad
  (vcons m x xs e) = sbad xs` checked at `Vec A (suc n)`; `hyp-motive` now gives such a
  hypothesis the motive at f only where f's indices are the scrutinee's, and otherwise a fresh
  abstract type (usable as nothing; `tests/elab.cub` `sbad`, `sbad2`). Index unification at a split (2026-11-06, as Agda's): when a
  split's scrutinee has indices that are not all variables, its index arguments are unified with
  each constructor's index expressions over the fields (`u_walk` in `ccm_t`): a constructor
  clash makes the branch absurd — the identity on the goal G transported along the equation
  through `case (e i) of <the scrutinee's constructor> → G → G; … → G` — so the clause may be
  left out (`vhead (vcons m x xs) = x`, `vzip`'s mixed cases); the same constructor recurses
  on the arguments; a field met by a term it does not occur in is solved, the branch built with
  it replaced and the fields whose types mention it transported along the equation's argument
  at its position (`vzip (vcons m x xs) (vcons m2 y ys) = vcons m (x , y) (vzip xs ys)`: `ys`
  becomes a `Vec B m`). The goal rewriting by a variable index now carries a substitution
  (`SGX`/`sg_apply`) to the reverted later parameters' types, the goal and the right sides, so
  the second vector of `vzip` is at the constructor's index below the first split. A missing
  clause is covered when a later variable's type admits no constructor under the substitution
  (`vlookup (vcons …)` only: `Fin zero` is empty below `vnil`). A clause that names an equality
  field keeps the old reading. `subst_multi` treats case alternatives as binders (it had
  flattened `((suc u) u)` into an application). `tests/elab.cub` `vzip`, `vhead-u`, `vtl-u`,
  `vlookup`. The fields transported for a solved field form a dependent telescope in field
  order (a field whose type mentions an earlier transported one is transported with it), and
  several solved fields are peeled one transport at a time, innermost first, each line holding
  the later solutions at their values (`tests/elab.cub` `getq`: `h : Tagged m i`; `both`: two
  solved fields). Refused, with the remedy named: a goal that depends on a solved field or on a
  field to transport. A transported value's equality fields are composite paths (CCHM transport
  is not regular), so such values are equal to the hand-written ones as paths, not by `#same`.
  Two elaborator gaps found by fork C (2026-11-06), fixed: (a) an untyped definition is not
  elaborated, so `idw #3` with `idw : {A : Type} → A → A` bound `A := #3` and returned `λ x → x`
  silently (under `code t` too); an untyped definition applying a checked one to exactly its
  explicit arguments, fewer than all its parameters, is now refused with the remedy named (every
  parameter in order, as `ua lzero Bool Bool e`, is fine; `tests/errors/untyped-imp.cub`); (b) a
  hole whose solution is a primitive type's code (`cpty`, e.g. `Bytes n × Int64`) was not read
  back by `cn`, so `rd q r ≡ rd q r` did not infer `≡`'s type; `cn` reads `cpty` as
  `lib/prim.cub`'s names (`tests/region.cub` `rd-same`, `rd-disj-stated`). (ii) FIXED
  (2026-11-06): the motive of a `%welimP` now travels with the elimination (`O_WELIM`'s last
  operand, `V_NWELIM`/`V_WSUSP` keep it), so a stuck word recursion of path type has a type at
  the machine and `p 0` is its type's left end, as for any neutral path (`O_PAPPP`'s endpoint
  rule, `O_NTYPE`); `catw (plusw-suc-l #0 b) (λ i → %wsucc (plusw-zero-l b i))` checks
  (`tests/elab.cub` `lam-path-arg`). (iii) FIXED (fatal errors on stdout too). (v) FIXED (the
  redeclaration error). (iv) and (vi) stand: mutual structural recursion is written with the
  later function as an argument (a `mutual` group is for copattern definitions), and `Level` is
  not a type, so level polymorphism is prenex, in signatures. Findings as recorded: (ii) a λ-path in an argument position has its endpoints evaluated,
  so congruences are stated as lemmas; (iii) a forward reference, or a reference from a
  checked definition to an untyped one, is reported on stderr (`not a bound variable or
  checked definition`) while stdout continues, so a test line that only reads stdout sees a
  silent exit 1; (iv) definition order matters everywhere, so a mutually recursive pair is
  written with the later function passed as an argument; (v) a user name that collides with
  a `lib/std.cub` name (`subst`, `le-refl`) is silently the library's; (vi) a Π over
  `Level` inside a type's body (`{l : Level} → (R : Type l) → …` as a definition's value) is
  not elaborated (`data type not checked: Level`); level polymorphism is written in
  signatures only, so a level-polymorphic type is indexed by a level parameter
  (`Proc : {l : Level} → Sig → Type → Type (lsuc l)`) and its inhabitants carry the level.
About 1,400 lines.

### 18.13 F31–F32 — Reflection and certificates

Revision 1.

- **F31.** `%view` over every value kind (the `View` type in `lib/code.cub` is extended to
  the forms it lacks: `wsusp` for stuck calls, `wnative` for `V_NATIVE`, `wword` with the
  big case); `quote`/`eval` total over them with the two laws re-tested; `deriving Eq`,
  `deriving Show` as terms over `dent` (the data table) producing definitions. Autodiff as
  the example: `D : Code → Code` for the first-order arithmetic fragment with `D-correct :
  eval (D f) ≡ derivative (eval f)` stated for polynomial `f` (a proof over the fragment's
  codes). ~500. DONE 2026-11-06 (item 31's status): over `PCode`, the typed mirror of the
  fragment's codes, the derivative in the Taylor form over `Nat`. Found on the way: `let t =
  taylor … in (fst t , … snd t …)` in a checked body exhausted memory where the same term with
  the projections written out checks in 9k steps (`D-correct` is written so). Rechecked after
  `d42df94a` (`subst_multi` no longer flattens case alternatives nor substitutes under their
  pattern variables): the `let` form checks in 7.7k steps; the blow-up does not reproduce.
- **F32, status (2026-11-06).** Both verified checkers exist as typed terms, each with its
  soundness theorem checked by the machine: `lib/linproof.cub` (`cert-sound`: a Farkas
  certificate that checks proves `a ≤ b` wherever the hypotheses hold; order as a witnessed
  sum, the check an equality test on `f + (g − f)`, multiples by repeated addition) and
  `lib/eqproof.cub` (`eq-cert-sound`: a trace of rewrite steps with positions, hypothesis
  indices, directions and substitutions proves `s = t` in every model where the schematic
  hypotheses hold at every valuation; terms are curried applications of constants). The
  unverified searches (`find-cert` by bounded enumeration, `find-trace` by bounded
  depth-first matching) produce what the checkers check. Left: the bridge from `Code` to
  the typed fragments (`linearize`, and a `Code → Tm` reading of applications of globals)
  with its soundness against the evaluator, and the elaborator calling the checkers on
  holes (needs E28's hole solving). Over `Int` (built): `lib/std.cub` reads an integer as a
  difference of words (`Z2 = Word × Word`, `toZ2`/`fromZ2` with `fromZ2-toZ2`, `addZ`, `subZ`,
  `negZ`, order `leZ` as a witnessed inequality of cross sums); `lib/intproof.cub` proves the
  commutative-monoid laws (`addZ-comm`, `addZ-zero-l/r`, `addZ-neg`, `addZ-assoc` through
  `toZ2-fromZ2-rel`, the normal form's coordinates standing for the same difference) and
  builds the checker: an `IntCode` (literals, variables, sums, negations) becomes the pair of
  word codes of its coordinates over doubled variables (`tc`, variable `v` to `2v`/`2v+1`,
  `repr` the faithfulness), a claim `a ≤ b` the word claim `a⁺ + b⁻ ≤ b⁺ + a⁻`, and
  `int-cert-sound` is `cert-sound` transported along the translation. The order laws
  (`leZ-refl`, `leZ-trans`, `leZ-neg`, `leZ-add`) are proved by the checkers themselves at
  environments of the integers in question. `lib/intlin.cub` is the search side (the same
  translation then `omega-cert`/`find-cert`); `tests/cert.cub` runs it end to end with the
  theorem applied at an environment.
- **F32.** `Cert` data type for linear arithmetic over `Nat`/`Int` (Farkas-style
  certificates: a nonnegative combination of hypotheses yielding the goal) and for
  equalities with uninterpreted functions (a sequence of rewrites); `check-cert : (hs : List
  Code) → (goal : Code) → Cert → Option Proof` proved sound once; an in-language search
  (`omega`-style elimination for small problems) and an external solver hook (`z3` via a
  `foreign` call) producing certificates; the elaborator calls `check-cert` on holes whose
  type is in the fragment. ~700.

### 18.14 F33 — Navigation: the concrete form

**Status (2026-11-06).** `lib/match.cub` (matching, instantiation, rewriting at the root,
at the first position, counting positions), `lib/nav.cub` (views, available steps, choice by
size with a goal pattern first, navigation to a bound), `lib/rules.cub` (the view from the
store's identities, read off checked theorems' types; forward and backward). The cost model
and the choice rule, decided 2026-11-06 (`lib/cost.cub`): a model is a value `cmodel ws wz wl
steps lat` — weighted recorded steps of the calls a term makes (`%gsteps`, `store-steps`, or a
table refined by `cost-record`), the term's size, and a schedule's latency estimate (each
global placed `pl-net` at its recorded steps, `pl-compiled` at one unit a call, `pl-foreign`
at a fixed boundary cost; `lat-of-sched`); a purpose is a value `reach-p goal | minimize |
reach-min goal` with the goal a predicate on codes (`goal-pat` makes one of a pattern); the
choice rule is best-first, the frontier ordered by the model's cost of each reachable term with
ties broken by the term's key (its canonical serialization `code-ser`, compared
lexicographically), a visited set, a budget of expansions (`navigate-best`): for a goal the
first goal term in cost order, for `minimize` the least-cost term reached. The readings show
one view chosen differently under two purposes (reach `h ?` stops at `h 9`, minimize goes to
the constant) and under two models (size ends at the constant, the latency model with the
constant across a foreign boundary ends at `h 9`), and a tie decided by key in either order
of the view. Open: purpose as a type to inhabit (goal types read by `rules.cub`), and costs
measured by the specializer's residual rather than the source term.

Revision 1. A design, since this is the open item; stated so it can be criticized.

- **The view.** A finite set of codebase keys (definitions and identities) the reasoner holds,
  with each identity's type as a code: `view : List (Key × Code)`.
- **Applicability.** For an identity `e : a ≡ b` in view and a term `t` in view, `e` applies
  to `t` at a position `π` when `a` matches the subterm of `t` at `π` under a substitution
  `σ` (matching over codes, first-order, as the rewrite step of B15 §3). The result is `t`
  with `b σ` at `π`, and the path `cong π (e σ)`. `available : View → List (Key × Pos ×
  Subst)` computes every such triple. This is finite and computable; it is the "next
  available transport".
- **Choice.** A `purpose` is a code `g` (a goal type) or a cost target; `score : Step → Cost`
  by the cost model; navigation picks the step minimizing `score` among those that keep `g`
  reachable (reachability of `g` is itself a view question: is there an identity whose
  right side matches `g`?). With no purpose, navigation is breadth-first over `available`,
  which is the saturation a `simp` set performs.
- **Models and tactics** are functions `View → View`: they add keys (from the store, by
  search in the store's index, or by proposing a code that is then *checked* and, if it
  checks, becomes a key). The proposal is never a step; only its checked result, as a key in
  view, enables steps. This is the exact sense in which nothing is proposed and then
  stepped on unchecked.
- **Safety.** Every step is `cong π (e σ)` for an `e` in the store; the store holds only
  checked keys; so every term reached is equal to the start by a path the store can
  reproduce. No participant can add a step the rules reject, because adding a key is
  checking.
- **Cost model (open thread).** Steps (the machine's own count) per key, recorded in the
  store from readings; the score of a step is the recorded cost of normalizing its result,
  estimated by the size of `b σ` when unknown. This is the first cost model; refining it is
  what A8's steps-against-size measurement feeds.

### 18.15 Open threads, current answers
- **Relaxed atomics.** Not offered. The memory interface names `release-acquire` and `SC`;
  relaxed is added only with a chosen model (the promising semantics is the candidate), as a
  separate signature so no program silently depends on it.
- **The configuration value.** `Config = (venues : List Venue) × (params : Params) × (role :
  R) × (platform : Platform)`; a binary is `main config`; `Platform` holds widths, byte order
  and alignment; the image is specialized against `Config` with `platform` at `build`.
  Consumers' instantiations are recorded by the image's `static` registration in each
  translation unit, fed back as a `Config` list for the next specialization.
  **Built (2026-11-07, fork C5, `lib/image.cub`):** `data Platform = platform (pl-wbits : Word)
  (pl-endian : Endian) (pl-align : Word)`, `data Config P R = config (cf-venues : List Venue)
  (cf-params : P) (cf-role : R) (cf-platform : Platform)` with the parameters a `List Inst`
  (`inst-at g args`, the type arguments as `TyArg`). `cxx-image-cfg cfgs …` reads the first
  configuration's platform at build: the preamble defines `cub::word` (`std::uint<w>_t`),
  `cub::align` (every defining struct is `alignas(cub::align)`) and `cub::big` (the byte layer's
  loads and stores assemble bytes in that order); `run.py`'s `platform` builds for a 32-bit,
  big-endian, 16-aligned target and checks the layout (`sizeof(cub::word) == 4`, `alignof == 16`,
  the most significant byte read first). A template whose template parameters are all types
  registers each instantiation statically (`template<class P0> inline const bool g_reg =
  cub::note(g, {cub::tyarg<P0>::text()});`, used in the body), and at exit the run writes them as a
  configuration to `$CUB_CONFIG_OUT`; every configuration's recorded instantiations become
  explicit specializations (`template<> R g<T>(…)`) in the next image. `run.py`'s `config` records
  `lenr<cub::word>`, reads the file back as the configuration, and compiles the specialized image
  with the consumer.
- **The cost model.** 18.14's first form: recorded steps per key, size as the estimate.

### 18.16 A8 — Proposal: formal transported compositions

**Status (2026-11-07, A8 round four: the `%comp-path` volume).** `%comp-path` is the most
entered definition (surfc's corner, `CUBICAL_PROFILE=show`: 3,420 entries, next `%bdy-squash4`
1,021, `%comp-pi` 370). **Kept:** its two boundary tubes were the η-redexes `λ i → l @ i` and
`λ i → r @ i`; they are now `l` and `r` (path η holds judgmentally in CCHM, so the composition
is the same term). Measured (steps of the read alone, memo on, `CUBICAL_WORKERS=0`): g8 319,371
→ 305,036 (−4.5%, 2,189 cells both), surfc's corner 127,513 → 121,271 (−4.9%), the number
457,431 → 443,066 (−3.1%), the typed port 1,056,716 → 999,755 (−5.4%). The 30 suite lines of
`make test` give identical answers; memo off, outputs identical across 0, 1, 4, 16 workers on the
determinism list plus squash and Brunerie. **Found, not caused:** TSan at 4 workers on the g8 and
surfc reads reports a race between `cell` (`net.c:81`, a write in split/bind) and `fd_cell`
(`net.c:938`, read from `box_fd`, `net.c:1001`); the old prelude reports the same race, so it is
round one's free-dimension walk running with workers. **What remains:** the entries themselves
(3,420 at the corner) — the composition's tube map re-applies `%@` per side and per entry; the
cross-dimension sharing below is still the mechanism that would cut them.

**Status (2026-11-07, what is left of A8 and why it is not queued).** Three pieces remained: sharing a
composition across the dimensions its tube maps apply it at, a free-dimension bound for values
still being built (A8b's "not done"), and the 6-cube. **The 6-cube, measured:** the read generic in
all six dimensions (`sq4 (squash4 … i j k l) @ a @ b`, Brunerie's library loaded,
`CUBICAL_WORKERS=0`, memo on) passes 2.5 GB resident 34 s into the run (library load included) and
is killed at the cap; a read at one Ω² endpoint stays 47 cells. So whether its normal form is small
is still not known, and A8's acceptance clause "finishes at generic dimensions" is not met.
**Cross-dimension sharing:** no design exists. A prototype (a memo at `O_COMPH` on `PathP`, keyed
by a fingerprint of the inputs with dimensions canonical by first appearance) was tried and
withdrawn: it never fired on g8 or surfc's corner (0 lookups), and the fingerprint census on g8
found 284 compositions in path types of which 211 could not be fingerprinted (the inputs are
still pending or under substitutions that move bound dimensions). Without an exact key there is no
measured ceiling for what sharing would save. **The bound for unbuilt values:** a further cut of
irrelevant substitutions with no measured gain behind it. All three are recorded here as
undesigned and are not on the queue: no other item depends on them, and the A8 rounds that landed
(boxes' free-dimension sets, `O_CONW`, A8b, η-reduced tubes) keep every answer identical.

**Status (2026-11-07, A8d: keys for path λs and systems, built and withdrawn).** The naming design
A8c pointed to was built: path λs keyed independently of the bound dimension's name (`synkey`'s
`S_PLAM` under a placeholder; a run-time `mkplam_of(v, i)` keyed `K_PLAM` over v's key with i
renamed to a canonical marker, `KMARK + depth`, so λ i → P @ i and λ j → P @ j get one key; a key
records the range of dimensions it mentions so a renaming skips subkeys without i), systems keyed
from their faces and branches (`S_SYS0`, `S_SYSCONS`, `S_SYSMAP`, `S_FEQ`, `S_NEG`, `S_BIN`, the
system rules passing the operation's key on), a substituted value keyed `K_SUB` over its
provider's and the substitution's keys. Found on the way: a marker's literal sent `dnf_lit`'s dense
table to 2^61 entries (the read never finished); big literals need their own table. **Census**
(compositions at `O_COMPH` on `PathP`, which inputs carry a key, memo on, `CUBICAL_WORKERS=0`):
corner 312, all five keyed 39, distinct 39; g8 285, all five 11, distinct 11; the number 285, 11,
11; the typed port 1,533, 24, 24 (the tube keyed 147/312, 26/285, 26/285, 55/1,533: tubes made by
`sysmap` over run-time values). No composition whose inputs are all keyed repeats, so sharing by
key has nothing to share. **The keys alone** (the memo's existing sharing with more named values;
memo hits rise, corner 2 → 725, g8 161 → 1,142) cost steps: g8 305,036 → 309,686 (+1.5%), corner
121,261 → 125,782 (+3.7%), the number 443,066 → 447,999 (+1.1%), the typed port 999,755 →
1,007,258 (+0.8%); answers identical (−2 both, corner 47 cells, g8 2,189). Withdrawn, the diff
kept in the worktree (`build/c2/c2k-tried.patch`). This settles A8's naming route: the
compositions met in these reads are distinct up to dimension renaming wherever they can be named.

**Status (2026-11-07, A8c: the three pieces tried to a decision).** Each was built, measured and
withdrawn; none landed, so every count is as before (g8 305,036 steps / 2,189 cells, surfc's corner
121,261 / 47, the number 443,066, the typed port 999,755; memo on, `CUBICAL_WORKERS=0`).
**(1) Sharing compositions in path types by key.** Rule: at `O_COMPH` on `V_PATHP`, when the type
line's three parts, the tube and the base all carry the machine's structural keys, name the
composition by them and the bound dimension and reuse the held result. Census first (the only
route: the profile's 3,420 `%comp-path` entries at the corner count entries into the clause's
inner λs, about eleven per composition; the compositions themselves arrive only through
`O_COMPH`, 311 at the corner, 284 in g8): **no composition had all five keyed — 0 of 311, 0 of
284.** The type line was keyed 0 times in either, the tube 0 times; the endpoints 4 of 311 at the
corner, the base 3. Why: the line and the tube are made at run time — `mkplam_of` (the line is the
type's family abstracted over the composition's own dimension) and systems (`O_SYSCONS`/`V_SYS`)
carry no key, `synkey` has no case for `S_PLAM` or the system forms, and everything the
composition clauses build from those is unkeyed in turn. So the rule never applies, and no step
can be saved by it. What would make it apply is a new design, not this rule: keys for path λs
and systems that are invariant under renaming the bound dimension (a key-level renaming of the
binder, with interval and face payloads substituted), propagated through `rule_mkplam`,
`O_SYSCONS2`, `O_SYSMAP` and substitution — a change to the memo's naming of every value,
with its own soundness argument.
**(2) A bound for values still being built.** The audit of every operation that produces a value
(the `O_*` list in `src/core.h`): a result's free dimensions are its inputs' (through a
reference: the box's set under the reference's substitution), plus its payload's (the
`owns_dnf` kinds, a substitution's values), minus a binder's own (`O_MKPLAM`, `O_FORALL`,
`O_COMPH`, `O_ABS`), plus dimensions made by `fresh_dim` (`O_MKDIM`, binder renaming in
substitution, path applications at fresh dimensions, compiled `S_PLAM`/`S_FORALL`). A global
(`O_GLOB`, `O_CALL`, `V_GLOBAL`) is closed. The only dimensions not among the inputs are therefore
fresh ones, all numbered from `dim_mark()` (the next `fresh_dim`) on, and no substitution made
earlier names them. So the A8b give-ups (`O_MKDIM`, `O_GLOB`, a record or table that may reach
`%dim`) became a watermark: the bound holds the inputs' set plus "every dimension from the mark
on", and a reference's substitution keeps its entries at or above the mark. Measured at the
corner (`CUBICAL_PROFILE=show`): the "may make a dimension" refusals went 204 → 0 and the tries
exhausted 2,303 → 289, but cuts stayed at 2, drops at 1, and every step count above was
unchanged. Why: the bounds that remain unknown stop at a part not there — a wire whose value has
not arrived (2,595 variable ports, 1,045 unknown sets) — and the net keeps no pointer from a wire
back to the operation that will produce its value (cells have no back pointers, by design), so a
reader cannot bound what it cannot reach.
**(3) The generic 6-cube, again.** Read in all six dimensions under a 2.5 GB resident cap (polled;
`ulimit -v` breaks the arena): killed at the cap with main's binary and with (2)'s, 70–90 s in on
the shared machine, the library's 118 checks done and the read not finished. Whether its normal
form is small stays unmeasured.

**Status (2026-11-07, the TSan race at `cell()`'s memset).** The intermittent report (`net.c:81`, about 1 run in 5 at 2–8 workers over the `make tsan` lines) was real, not a false positive, and came from round one's walk, not A8b: `box_fd` → `fd_cell` (from `rule_subst` on `V_REF` or `ref_read`, in a worker) read a held value's parts non-atomically while another reader claimed them (`claim`'s atomic store against the plain read, via `copy_top`/`lazy_copy`), consumed and freed them, and the main thread's `cell()` reused a freed cell (memset via `split`/`bind`, `con_build_bdy`). `fd_cell` also reads payloads that `payload_release` frees, so deferring cell reuse (a quiescent-state reclamation scheme was tried) does not make the walk safe. **Fix:** `box_fd` walks only when `net_solo`, as A8b already did; with workers a box's set is whatever a solo phase cached (immutable), else unknown, and the read is not cut. Round one's gains therefore hold only at `CUBICAL_WORKERS=0` (the default); with workers step counts are as before A8. TSan: baseline 3 hits in 16 runs; gated, 0 in 63 runs plus `make tsan` twice; memo-off output identical across 0, 1, 4 workers.

**Status (2026-11-07, A8b: bounds through pending parts).** Round one's box sets (`box_fd`)
were unknown while a box's parts were still being built (of g8's 20,664 substituted reads, 12,584
past eight tries, 3,018 a part not arrived, 2,787 an unbuilt part). **Built:** (1) a compile-time
taint, `record_mkdim`/`casetab_mkdim`/`global_mkdim` (`lang.c`): may running this record, case
table or global reach `%dim` (`O_MKDIM`) or the checker's tables (`S_GLOBTAB`), directly or
through calls; globals by Tarjan's components, a global without a body (other than an axiom or an
input) or a native counted as tainted, a component cached clean only when every member is
defined. (2) Bounds for pending values (`fd_pending`, `net.c`): what will arrive at an
operation's port is built from what reaches its other ports (an `O_DUP` copy from its input
only), plus the dimensions its payload names (`O_SUBST`'s values, a face's formula), minus the
binder of `O_MKPLAM`/`O_FORALL`; `O_MKDIM`, `O_GLOB`, `O_ERA`, a tainted case table or native,
fail. An unbuilt value (a thunk, a suspension) is bounded by its environment plus a thunk's
pending substitution's values (missing that, `isPropPathP` in std failed with a wrong endpoint),
unless its record is tainted. (3) Such a bound is kept on the box marked inexact and refined at
later tries (up to eight); `rule_subst` and `ref_read` cut substitutions to it as to an exact set.
The walk reads cells other workers rewrite, so it runs with one thread (`CUBICAL_WORKERS=0`, the
default) and the memo on; with workers, or `CUBICAL_NOA8B=1`, everything is as before. Measured
(steps of the read alone, memo on, `CUBICAL_WORKERS=0`): g8 329,076 → 319,371 (−2.9%), surfc's
corner 130,291 → 127,503 (−2.1%), the number 470,557 → 457,431 (−2.8%), the typed port 1,077,444
→ 1,056,716 (−1.9%); `tests/hit.cub` 4,804 → 4,687, `tests/fo.cub` 19,229,984 → 19,214,555.
With workers the counts are unchanged. Every suite line of `make test` gives the same answers as
without it; without the memo the outputs are identical across 0, 1, 4, 16 workers on the
determinism list plus the squash and Brunerie lines, with it the answers are; TSan clean on the
`make tsan` lines. **What remains:** g8 still meets 12,233 tries without a set; nearly all now
fail at a dangling wire (`fd-var`, 7,963: a part whose producer has not yet been linked), none at
an operation's principal port; the taint stops 204. The gain is small because the reads that
matter are mostly cut already or wait on wires not yet made; the cost is in `%comp-path`'s volume
(3,271 entries in the corner read), not in uncut substitutions.

**Status (2026-11-07, a path constructor without its boundary).** `con_build` applied a path
constructor's boundary clause (`%bdy-squash4` …) at every construction and every substitution,
then checked its face; the rebuilt boundary was erased whenever no face held, the common case at
generic dimensions (2,839 entries of `%bdy-squash4` in surfc's corner read). The boundary's faces
are conjunctions of the constructor's dimensions' ends, so a face holds only when some dimension
is 0 or 1. **Built:** `O_CONW` waits for the dimensions in turn (one step each, whatever the
arrival order, so counts stay schedule-independent) and, while none is at an end, makes the
constructor without building the boundary; a dimension at an end builds it as before
(`CUBICAL_NOCONW=1` turns it off). Measured (steps of the read alone, memo on, one worker): g8
458,888 → 329,076 (−28%; 150 steps per result cell, 2,189 cells); surfc's corner 170,648 →
130,291 (−24%, 47 cells); the number 600,349 → 470,557 (−22%); the typed port 1,349,493 →
1,077,444; `tests/hit.cub` 6,599 → 4,804. Normal forms identical; without the memo the outputs
are identical across 0, 1, 4, 16 workers on the determinism list plus the squash and Brunerie
lines, with it the answers are; every suite line passes with the same counts elsewhere.
**Measured, not reached:** the read generic in all six dimensions runs past 182M steps (library
included, ~4M) before a 2.5 GB cap stops it (`CUBICAL_PROGRESS=n`, added: steps and cells every n
steps), while every read with a cube dimension at 0 or 1 is 5–6 cells in under 250 steps and a
read at one Ω² endpoint is 47 cells; so the generic normal form's size is still unknown, and the
reads that fix any one of the six dimensions are small. Why box sets stay unknown
(`CUBICAL_PROFILE`'s `fd-unknown`, added), for g8's read: of 20,664 substituted reads, 12,584 hit
a box past its eight tries, 3,018 a part not yet arrived, 2,787 an unbuilt part (a thunk or a
hypothesis), 938 a root not yet arrived; the lazy reader reads boxes while their producers are
still running. Bounding a pending value's dimensions by its producer's inputs is sound except
where a fresh free dimension is made: the audit of `src/net.c` finds one rule, `O_MKDIM`
(`%dim`, reflection); every other `fresh_dim` is bound at once (`rule_subst`'s capture-avoiding
rename of a line, `S_PLAM`/`S_FORALL` builds, the hcomp side test `O_HCM2`/`O_HCM3` whose fresh
dimension is erased with the side, a path-typed field's hypothesis, the printer's binders in
`lang.c`). A bound through closures needs the dimension-making taint of every record and global
a value can reach (check.cub's `%dim` is loaded in every checked run). **Tried and withdrawn
(2026-11-07):** (a) bounding a keyed box's dimensions by those its key names (a keyed value is a
function of what its key names, %dim's dimensions kept apart by a bit per dimension): sound, but
g8 329,076 → 327,117 and surfc 130,291 → 130,085 — most boxes the reads meet are unkeyed parts
(`copy_top`'s), not memo results; (b) the `%comp-path` body held unbuilt (`delay`) so an
endpoint read never builds the generic fibre: g8 +1%, surfc +2.6% — endpoint applications are
not where the cost is (as with `V_COMPP`); (c) a tube side mapped under its own binder
(`%linemap w g`: `λ k → g B k` for `w = λ k → B`, no renaming of `B`): unsound — a line's
binder can be free elsewhere among the clause's operands (`O_COMPH` re-wraps a family's parts
under the family's own dimension, and `rule_subst` keeps a λ's binder when the substitution's
range does not mention it, so copies share binders): `equivEq` in std failed with a captured
dimension, surfc's normal form changed (50 cells), g8 failed. The six-dimension read stopped at
45 s (`CUBICAL_PROFILE` prints on SIGTERM): 1.79M `%comp-path` entries, 591K `%bdy-squash4` (the
boundary at an end), SUBST 31.7M, ERA 17.9M — the corner read's mix at the same density per step,
so no single runaway clause; its normal form's size stays unknown.

**Status (2026-11-06, the sharing step).** Where the cost is (`CUBICAL_PROFILE`, steps by
operator and by the value met, added): g8's 475,558 steps are a third `SUBST` (half of those a
substitution composed onto a reference) and a fifth `ERA`; path applications themselves are 5%.
A box is read under about 8 distinct substitutions in g8 (34,204 substituted reads of 4,321
boxes) and 13 in surfc's corner read (12,947 of 956) — the composition body applied at many
dimensions — and equal (box, substitution) pairs are only 8–12% of them, so the memo shared
little. Of the substituted reads whose box value was built through, two thirds substitute only
dimensions the value does not mention (g8 7,183 of 11,193; surfc 6,020 of 9,170), most of them
closed values (5,110; 3,502). **Built:** a box keeps the free dimensions of its value once that
value is built through (`box_fd`: a walk reading nested boxes' sets, a few tries, kept on the
box), and a reference's pending substitution is cut to them (`rule_subst` on `V_REF`,
`ref_read`): reads of a box at dimensions it does not mention become reads of the box itself,
named by its key, and the memo shares them. Measured (steps of the read alone): g8 475,558 →
458,888; surfc's corner 201,170 → 170,648 (−15%, 47 cells); the number 619,852 → 600,349; wall
time unchanged within noise; answers identical. Whether the set is known when a read happens
depends on what has been built, so it runs only with the memo (whose counts already depend on
that): without it every step count is as before, and the determinism check holds (identical
output across 0, 1, 4 workers without the memo, identical answers with it; TSan clean).
`CUBICAL_NOA8=1` turns it off. **Not done:** a set for a value still being built — a pending
operation's free dimensions are not always its inputs' (`O_MKDIM` makes a dimension), so a bound
for unbuilt parts needs every rule audited; this is where the rest of the irrelevant
substitutions are (the lazy reader reads a box before its parts are built). **The 6-cube:** a
read generic in both Ω² dimensions, even on the 4-cube's diagonal (`@ i @ i`), still exceeds a
2.5 GB cap (the machine is shared; 5.4 GB before), so whether its normal form is small stays
unmeasured; a read at one Ω² endpoint is 47 cells; a cube dimension at 0 collapses the read to
the boundary (5 cells).

**Status (2026-11-06, after `ab7a78d`'s `%transH` and the boundary rule below).** Measured on
`main`: `#show g8` 475,558 steps for 2,189 cells, 217 per cell (from 484 before `%transH`);
the number 619,529 steps; the square driver unchanged; every suite identical across workers
and memo on/off. `surfc` on the squash₄ cube: a read at any endpoint of either Ω² dimension
(`@ 0 @ 0`, `@ m @ 0`, `@ 0 @ m`) is 47–48 cells for about 196k steps — 4k steps per cell,
the profile per `#show` (`CUBICAL_PROFILE=show`, added) showing `%comp-path` entered 3,287
times and `%bdy-squash4` 3,095 times — and the read at both Ω² dimensions generic (the 6-cube)
exceeds 5.4 GB and is killed. Skipping the boundary rebuild when no substituted dimension
occurs in the constructor's dimensions removes a tenth of the entries (tried, withdrawn:
reading the dimension values at substitution time made the step count depend on arrival
order, and the suites must be identical across workers); the rest come from the
composition tubes restricting the four cube dimensions. Done deterministically (an
operation that waits for the dimension values) it would be worth that tenth. The `%comp-path` count (3,287 entries) is the dependent 4-cube over sets
(`fill4OverSets`) composing in path types at every level. **Tried and withdrawn (2026-11-06):**
a formal composition in a path type (`V_COMPP` holding the clause's five arguments; applied
to a dimension, the fibre's composition at that dimension alone through a `%comp-path-at`
clause, at an endpoint the side's line at 1 without the fibre; read otherwise, the clause's
λ; substitution on the parts) measured worse on every reading: g8 475,558 → 489,833 steps,
surfc's corner read 196k → 326k, the number 619,852 → 630,540. The profile says why: the
applications are at generic dimensions, from the tube maps `λ w i → w @ i @ j` (5,493
`%comp-path-at` entries against 3,287 λ entries before), and rebuilding the fibre's
composition there costs more than substituting into the one λ body the readers share
through its box; endpoint applications, where the formal form wins outright, are rare in
these reads. So the next step is not this one. What would help is sharing the composition
*across* the dimensions it is applied at (one body, the applications as substitutions of a
held value: the λ form already does that) and cutting the tube maps' re-applications — a
different mechanism, not designed. Whether the fully generic 6-cube's normal form is itself
small is not known (a read at one endpoint is), so the acceptance clause "finishes at generic
dimensions" needs its result size measured first.

Revision 1. A change to the generated composition clause and one value kind; the checker is
untouched.

**Today.** `%trans-hcomp-D`: `transp ψ (hcomp [u] u0) = hcomp [sysmap (λ w j. transD ψ (w @
j)) u] (transD ψ u0)`, generated in `lang.c` (the `HCOMP_OPEN` site near 3330), applied as
soon as the composition exists. Endpoint substitutions later select one side per layer and
erase the others, transports already done. Twenty layers multiply. `delay` on the sides keeps
the distribution and loses sharing.

**Proposed.** The clause produces a formal value `%transH D ψ lines u u0` (cell kind
`V_TRANSH`, beside `V_HCOMP`), holding its parts unexpanded. Two rules:
1. *Substitution.* Apply σ to the parts; re-decide `u`'s faces as `V_HCOMP`'s rule does by
   rebuilding `O_HCOMP`. If a face is true with side `w`: the cell becomes `transD (ψσ)
   ((wσ) @ 1)`. Otherwise it stays `%transH` with substituted parts.
2. *Reading.* When `%case-hcomp`, a further `transD`, conversion or `%view` demands the
   structure, distribute one layer: today's right-hand side, with inner occurrences again
   formal. `%view` presents the distributed form, so `check.cub` sees what it sees today.

**Why it is linear.** At a generic dimension nothing reads the nest, so it is one formal cell
per layer. The endpoint substitution cascades inward selecting one side per layer; the cap is
transported only on the path that needs it. One cell per layer, substitution applied once to
its parts, sides never separate producers until selected: this is what `delay` lacked.

**Acceptance.** g8's steps per result cell within a small constant of the nested-loop
table's ratio (the table is linear: 4.2k/9.1k/19.0k/78.4k steps at 4/8/16/64 loops); the
square driver unchanged; `surfc` on the 4-truncation cube at generic dimensions finishes;
every suite identical across workers and memo on/off; TSan clean.

**Size.** `lang.c` ~30 (emit the formal form), `net.c` ~110 (`V_TRANSH`, the two rules,
`%view`), tests ~20. About 160 lines.

### 18.17 E28a — Induction hypotheses for path-typed recursive fields

Revision 1. Required by A10b; `rec2`/`rec3`/`rec4` in the Brunerie port are trusted
primitives until this lands.

**Gap.** `check-branch` (`lib/check.cub` 983–1024) binds a hypothesis for each field whose
type is the data type (`is-data-d`); a field whose type is a path line into the data type
(`p : PathP (λ m → T2 A) x y`, or a square, at any dimension) gets none. A hand-written
recursive call `f (p @ m)` is not structurally smaller for the elaborator's termination rule,
so a typed truncation recursor cannot be written.

**Fix.** A field is recursive when its type is `D` or a path line, at any dimension, whose
fibre is `D` (the shape `positive-line` accepts). For `p : PathP L x y` with fibre `D`, the
hypothesis is `rec p : PathP (λ m → P (p @ m)) (rec x) (rec y)` with value `λ m → F (p @ m)`;
for an n-dimensional line, the n-cube over the motive by the same recursion on the line.
- `lib/check.cub`: `branch-fields` classifies such fields as recursive; `branch-hyps` binds
  the hypothesis at that type (~40 lines).
- `src/syntax.c`: `recmask` includes them; the structural-recursion rule translates a call
  `f (p @ m …)` on a path field into `(rec p) @ m …` (~80 lines).

**Result.**
```
rec2 B g f x = case x return λ w → B of
    inc2 a → f a
    squash2 x1 y1 p q i j → g (rec x1) (rec y1) (rec p) (rec q) @ i @ j
```
checks with no primitive.

**Acceptance.** The three recursors in the port check with their primitives removed;
`tests/hit.cub` gains a `Trunc` eliminator into a set whose `isSet` argument is consumed in
the `squash` branch; every suite unchanged across workers and memo on/off.

### 18.19 A4 — Bitmask formulas: a design note (2026-11-06)

**Status (2026-11-06, measured before building).** The design below was costed against the
formulas the machine actually makes, with counters added to `src/dnf.c` (printed by
`CUBICAL_PROFILE`): the Brunerie run (library and number, 3.9M steps) allocates 103,568 `Dnf`
payloads holding 134,222 clauses and 40,317 `Clause` payloads holding 50,693 literals — 1.3
clauses per formula, 1.26 literals per clause, one payload allocation per 27 steps; `g8` the
same shape (92,922 / 31,439). Under `gprof` every formula function together is below 1% of the
run (`dnf_release` 1.8M calls, `iv_subst` 654k, `dnf_meet` 87k, `dnf_join` 69k, none
registering a sample); the top of the profile is the compiler's `maxlev` (24%, 2.4M calls)
and the name `lookup` (9%), both load-time work of `src/lang.c` — A9's domain, not A4's. The
mask form would remove at most 144k small allocations and shorten merges of 1–2 literals:
below measurement. Closed by measurement, like item 7: not built, the design kept for the day
a workload makes formulas large (deep cubes at generic dimensions, A8's family, are where to
look). The counters stay.

Today a formula is a `Dnf` of clauses of literals `dim << 1 | polarity` over 64-bit dimension
names never reused (`src/dnf.c`); meet, join, negation and substitution allocate; the
profile shows Dnf allocation as the remaining payload traffic (A4's note). The bitmask form:

- **Scopes.** A dimension scope is an ordered set of at most 64 live dimensions; a formula
  in a scope is a set of clauses, each clause two 64-bit masks (the positive and the negative
  literals by index). Meet of clauses is two ORs and a contradiction test (`pos & neg`), join
  is a clause-set union with subsumption by mask inclusion, negation is De Morgan over the
  masks, a face `r = ε` is one clause. No allocation for formulas of at most a few clauses
  (inline arrays), and the DNF normal forms the machine compares (`dnf_equal`, keys `K_IV`)
  become mask comparisons.
- **Where the scope lives.** A frame's dimensions (its `plam`s and the fresh dimensions
  of its `comp`s) are the natural scope: compiled records already carry slots for them
  (`bind_dim`), so the index is the slot. A value crossing frames (a box shared by two
  readers under different substitutions) needs a renaming at the boundary: the box records
  its scope (the dimensions free in it, by index) and a reader supplies the map from the
  box's indices to its own; a substitution `Subst` is then a map from indices to formulas in
  the reader's scope, applied as a mask rewrite. Dimension names stay 64-bit and global
  (keys, printing, determinism) — the index is a per-scope cache of the name, not a
  replacement: `scope[i] = name`.
- **Keys.** A formula's key (`K_IV`, `K_FACE`) must not depend on the scope's ordering, so
  the key is computed from names (today's form), not masks: masks are the working form and
  names the canonical one; the conversion is a table lookup per literal.
- **What it costs.** `src/dnf.c` gains a mask representation beside the clause lists
  (~250 lines) and the substitution/restriction rules in `src/net.c` use it where both
  operands are in one scope (~150 lines); frames gain a scope table (~40 lines). The
  acceptance is the profile: Dnf allocation leaves the top of the payload traffic, the
  suites identical across workers and memo on/off (a per-scope index must not change any
  step count: the rewrite rules see the same formulas).
- **Not chosen.** Replacing names by indices outright (bitmasks as the only form): keys and
  printing would then depend on binding order, and a formula shared by two scopes would need
  two identities; the determinism claim (A1) rests on names.

### 18.18 E28b — Strict positivity under Π codomains and Σ components

Revision 1. Found while writing `lib/proc.cub` (D24) against `91c128b`; blocks it.

**Gap.** `positive` (`lib/check.cub` 1295) accepts a constructor field only when it is the
data type at its own parameters, does not mention it, is a later `▸ κ D` directly, or is a
path line into `D`. A field `k : Resp S c → D` (the type in the codomain of a function whose
domain does not mention it), `(c : C) × (Resp c → D)`, or `Resp c → ▸ κ D` is rejected, so
`data Proc = done (a : A) | issue (c : Cmd S) (k : Resp S c → Proc)` and its guarded form
cannot be declared. Codata has the rule (`copositive`, "under Π codomains and Σ components");
data does not. This is the standard strict-positivity rule (Coq, Agda), and `Tree = node (Nat
→ Tree)` is the textbook case.

**Fix.** In `positive d pvs A`, two more cases, with the recursive-field classification and
induction hypotheses following:
- `tpi B nm b`: when `B` does not mention `D`, positive iff the codomain `b x` is positive
  (for any `x`); the field is recursive when the codomain is, and its hypothesis is the Π over
  `B` of the motive at the applied field: `rec k : (x : B) → P (k x)`, value `λ x → F (k x)`.
- `tsigma B nm b`: positive iff `B` is positive and `b x` is positive; hypotheses for each
  recursive component.
- `▸ κ` under either: as today's later rule applied to the codomain or component (no
  hypothesis; guarded recursion supplies it).
`branch-fields`/`branch-hyps` (18.17's site) bind the new hypotheses; the elaborator's
structural-recursion rule accepts `f (k x)` for a Π-recursive field as `(rec k) x`. About 60
lines in `check.cub`, 40 in `syntax.c`.

**Why it matters beyond processes.** Rose trees, interaction trees, free monads, parsers,
game trees, any container with function-shaped children: all of ordinary functional
programming's recursive types with function fields. Every library item in D and most of F
need it.

**Acceptance.** `data Tree = leaf | node (Nat → Tree)` with a recursive `size` by the
hypothesis; `data Proc` as above with `bind` by structural recursion on the first argument;
its guarded form `GProc κ` with `k : Resp S c → ▸ κ (GProc κ S A)` and `gbind` by guarded
recursion; `lib/proc.cub` (held in the scratchpad until this lands) checks in full;
`tests/reject.cub` gains `data Bad = bad (Bad → Nat)` rejected (negative occurrence).

**Interim finding on the copattern encoding.** A codata `Proc` with `step : Sum (Cmd S) A`
and a total `cont : (c : Cmd S) → Resp S c → Proc S A` passes positivity and guardedness but
cannot type `cmd` (an inert continuation at another command has no value of the issued
command's response type), and a dependent `cont` makes the copattern guard rule fail on a
`case` applied to an argument. So the data/guarded form with 18.18 is the right encoding, not
a workaround around it.
