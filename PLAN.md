# The system: direction, scope and build plan

Revision 8, 2026-11-09. Repo avikj/math, `scoped-machine` at `ccafee2d` on `main` (revision 7.9 was at `a774e2ee`). This document lives at `scoped-machine/PLAN.md`; it is the single plan and status document — a fresh agent starts here.
Each revision replaces the last. Section 17 records changes and retractions.

**Revision 8 in one paragraph.** The compile path built between 2026-11-05 and 2026-11-09 (`lib/fo.cub`, `lower`, the C, C++ and x86 printers, `repr`, `stage`, their tests and harnesses) is removed. It was a sequence of passes between syntaxes the checker could not relate — a first-order form with its own evaluator, printers mapping machine primitives to target operations through a table nobody checked — tied to the machine by comparing answers. That is a second description of the system with a convention between them, the one thing the premise forbids, and it produced wrong answers twice in a day (32-bit arithmetic lowered to 64-bit; unbounded `Word` printed as `uint64_t`). The ~14k-line checked model of the pipeline (`foproof*`) removed the same morning was the same error at a larger size. What replaces them is one definition: the *meaning of the target* in the language (section 5), against which a compiled program carries a path the checker checks, and the same definition is how every language, OS and processor is lifted (sections 9–10). The test every piece must pass from now on: **can the checker reject this piece's output when it is wrong?** If not, the piece is outside the theory and is not built. Section 0 states the endgame this serves; section 12's queue is the order of work.

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
- **Programs about programs are ordinary programs, and their outputs carry paths.** The
  specializer, the matcher and navigator, the derivation of decidable equality from the
  data table, the three verified decision procedures: every one is a definition in the
  language over codes. A proof by reflection (`ring-eq`) is one evaluation of the
  normalizer; the proof term is a constant path. A program about programs needs no proof of
  its own: what it produces carries a path the checker checks (a rewrite carries `ap` of the
  theorem it applied; a compiled program carries `⟦p⟧ ≡ f`), so a wrong program about
  programs produces an output that does not check, and nothing it produces is trusted.
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
- **The hot path is the machine code an engineer writes, and it carries its path.** A
  compiled definition is a pair: the bytes, and the path `⟦p⟧ ≡ f` from the bytes' meaning
  to the term, checked by the checker (section 5). Words, pairs and flat records are
  machine words and registers because the environment's layout is a representation and the
  reading is transport along it; loops are counted loops because a word elimination's
  meaning is defined to be one. The same term also runs on the net with open variables,
  where it is a proof. (Until 2026-11-09 this bullet described printers whose output had no
  path; see revision 8's paragraph above.)
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
certificates (`static_assert` by exhaustion). (The readings that showed this through the
removed back end are gone with it; the rule stands and section 5.3 makes the assumed fact a
field of the access's meaning.) The owner also asked whether anything provable
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

**What went wrong between 2026-11-05 and 2026-11-09, so that it is not done again.** The
back end was built the way compilers are built elsewhere: a first-order form with its own
evaluator (`lib/fo.cub`), lowering from codes into it, printers mapping its primitives to C
and x86 through a table, a C++ image with re-import as its check, and representations
consumed by rewriting after lowering. Each stage was related to the next by comparing
answers (`run.py`, `tests/fo.cub`, `#same` readings), never by anything the checker could
see. When the owner asked that compiling be transport, a ~14k-line *checked model* of that
pipeline was built beside it (`lib/foproof*.cub`): duplicate syntaxes, evaluators and
translations with simulation proofs, tied to the real compiler by readings. The owner's
verdict, which is the rule: never manufacture a copy, prove things about the copy, and tie
it to the real thing with tests; correctness holds of the thing that runs or it does not
hold. The model was deleted (`d72a42f3`), and a day later the compile path itself was
found to be the same error at a smaller size: the printers claimed, in a table, that the
machine's unbounded `Word` is `uint64_t`, and compiled programs answered wrongly past
2^64; a day earlier 32-bit operations had been lowered to 64-bit ones. The owner's
diagnosis was exact: two representations held and assumed equal where the type system
relates them by nothing. Everything built on that path is removed in revision 8. Three
further errors made while correcting it, each caught by the owner within the hour, and
each the same error in a new costume: proposing to write a C++ semantics from the standard
(a model beside the implementation that runs); calling a lifted implementation "the
complete semantics" (an exact object is not a correctness theorem); and adopting a
proof-assistant frame in which code is written and then proved by "obligations" and
"proof search" (here every equality is a path the computation produces, and the cost of
correctness is the cost of writing the representation; there is no proof library). The
single test that excludes all of these, applied to every line before it is written: *is
this a definition, a representation with its laws, a transport, or a declared assumption?*
Anything else — a model, a separate proof, a test standing in for checking, a pass
connected to the next by comparison — is not built.

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
- **Every piece's output is checkable, or the piece is not built** (2026-11-09). A program
  about programs — the specializer, the reader that compiles, an importer, a lift — carries
  with each output the path that relates it to its input (the `ap` of the theorem a rewrite
  applied; `⟦p⟧ ≡ f` for a compiled program). The program itself is never proved; a wrong
  one produces an output that does not check. Consequently there are no passes between
  forms the checker cannot relate, and no tests as evidence of correctness: for everything
  above the trusted base the checker is the evidence. Tests remain only where they are the
  only evidence that exists, for the trusted base itself (the rules against CCHM, workers
  against one thread, race and memory detectors, an imported model against hardware traces,
  an encoder's bytes against a reference assembler), and nowhere else.
- **Semantics are lifted, not written** (2026-11-09). The meaning of a processor is its
  formal ISA model, imported; of a compiler, kernel or interpreter, its binary under that
  model; of a language, its implementation's elaboration with a structured reading that
  keeps the abstractions, related by a checked path per program. A semantics written from a
  prose standard is a specification object, admissible only when related by a path to the
  implementation that runs, and needed only for conformance and portability claims. The
  three are different types and are kept distinct: what a pinned implementation does, that
  it conforms to a standard, and that an application has its intended properties.
- **Imports carry coverage** (2026-11-09). An imported model is complete only on the
  instructions, terminators or syscalls it implements; what it lacks is an inventory, and a
  program using a missing item does not lift until the model is extended. An importer is in
  the trusted base until its outputs carry their own evidence; round-tripping proves nothing.
- **The artifact is the bytes.** A correctness claim about compiled code states where it
  starts and ends: source and build context → elaborated program → structured machine
  program → bytes → the loaded process. Each link is a transport, a per-output checked path,
  or a declared assumption, named as such. The loader and the OS contract are assumptions
  until the kernel is lifted.

### Vocabulary you must have before reading code

`%view`/quote/eval (reflection: a value read as a code and back), `celim`/`ccase` (eliminations
with hypotheses for recursive fields, the hypothesis a suspended recursion read with
`cforce`), `cwelim` (word elimination; what every word function normalizes to), `cglob` (a
checked definition by index; `(code name)` quotes to it), `%gty`/`%gval`/`%gusage` (a
global's type, value, usages), `%dent`/`%dtab`/`%rtab` (the data, case and representation
tables), telescopes `tx/ty/tz`, `Lst`/`lcons`/`lnil` (the untyped list used everywhere before
the checker), `some`/`none` (`Opt`), "typed" versus "untyped" definitions (a definition with
a signature is checked; one without is not, and a checked definition may not mention an
unchecked one), `specialize` (normalize, rewrite along the store's theorems, each rewrite
carrying its path), `M` (section 5: the structured machine program, a data type whose
meaning `⟦_⟧` is defined construct by construct), `read` (a normal form into `M`, its path
`⟦read c⟧ ≡ eval-g c` by construction), `encode` (`M` to bytes by fixed templates),
`Lew`/`le` (order as a witnessed sum, over Word and over Nat), `fromNat`/`toNat` (the Word ≃
Nat used to carry proofs across). `Word` is the machine's *unbounded* natural (`src/big.c`
past 64 bits); `U64` (section 5) is the modular 64-bit type; they are different types and
nothing prints one as the other.

### The elaborator's current behaviour (these cost hours each when unknown)

1. Definitions must be declared before use; a mutually recursive pair is written with the
   later function passed as an argument.
2. A name declared by a later module rebinds the references of every module parsed *after*
   it — silently. Until revision 8 a script (`tools/names.py`) policed this; the elaborator
   refuses a redeclaration of a top-level name or constructor instead (revision 8's
   deletion item), so the machine reports it where it happens.
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
    checker already accepted; `CUBICAL_NOCACHE=1` turns it off. A `#native` reading under
    a private `CUBICAL_CACHE` alone is read from the cache and runs nothing; use NOCACHE to
    see the native path run.

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

Built and standing (revision 8, 2026-11-09): the net running CCHM with univalence
computing and canonicity (Brunerie's number −2 at 21× less CPU than Agda from nothing);
the composition clauses as terms; the checker as a term over `%view`, quote and `eval-g`;
keys, structural retention, pending substitution and call by need in the net (A8e, CBN);
multiplicities (B11); `represent` and the byte layer (B12); content keys and the store
(F34); the elaborator (instances, records, modules, fording, `by-lin`, `--serve`, an LSP);
the mathematics library; three verified certificate checkers over words and `Int`;
processes, worlds, handlers, supervision, replay, session types and choreographies with
projection as specialization; the store's theorems as rewrite rules with matching and
navigation. About 23k lines, all of it either a definition, a transport, or the trusted
machine, and none of it depends on what is removed.

Removed (revision 8): the whole back end from codes to C, C++ and x86 (`lib/fo.cub`,
`lower`, `emitc`, `emitu`, `emitt`, `emitx86`, `image`, `reimport`, `repr`, `stage`; the
C in `src/` that served it; `tests/`, `experiments/fo`, the tools), ~12k lines, and
autodiff's `PCode` mirror. Section 5 states what is built in its place and section 12's
queue the order. Nothing compiles to native code at this revision; the first compiled
program with its checked path is the first item of the queue.

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
5. These files, in this order: `lib/code.cub` (the Code type and `eval-g`, which is not a
   second evaluator but the denotation of a code as the term it names), `lib/check.cub`'s
   `quote`/`qneu` (what a normal form looks like as a code: this is what section 5's reader
   reads), `lib/specialize.cub`, `lib/match.cub`, `lib/rules.cub`, `lib/prim.cub` (B12:
   `represent Nat by Word` is the model for every representation), `lib/linproof.cub` (how
   proofs are written in this elaborator), `lib/proc.cub`, `lib/world.cub`,
   `lib/region.cub`.
6. Build the machine (`make build/cubical`) and load the library once. Then the queue.

### Before taking an item, you should be able to answer these without looking

Why is no retention policy added? Why is erasure by usage and not by binder? Why are floats
not approximations? Why do laws never reach the emitted image as comments, and what does?
Why does a stuck primitive operation without a code block specialization of anything over the
byte layer? Why is the checker's decision not first-order? What does "work proportional to
result size" promise and not promise? Why is the inductive process type in Church form, and
what exactly waits on 18.18? Why are open dimensions and higher inductive types not
limitations of the compiler? What is section 16's ledger for, and what does an unanswered
row become? Where exactly does univalence compute in the path from a definition to the
bytes that run — name the equivalence, the path, and the transport? What is the one test
every new piece must pass, and which four kinds of thing pass it? Why is `Word` not
`uint64_t`, and what does a `Nat` program compile to? Why does the compiler need no proof of
its own correctness, and what does that make the compiler's output? What is the difference
between lifting an implementation and proving it conforms, and which of the two does
lifting deployed code need? If any answer is missing, read the section that holds it.

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

### 0.1 The endgame: the tower

The scope is every computing process, stated as a tower in which every layer that runs is a
term and every property is a type, with trust ending at formal hardware models and the
physics beneath them. Everything in the tower is produced by one operation, normalization on
the net, and related by one kind of evidence, a path. There are no passes the checker cannot
relate, no models beside the thing that runs, no tests as evidence. Bottom to top:

- **Hardware.** `⟦x86⟧`, `⟦Armv9⟧`, `⟦RISC-V⟧`: the meaning of machine code as a function of
  the initial state and the environment's responses, *imported* from the formal models that
  exist: Arm's own ASL for Armv9.4/9.3/8.5-A through Sail (complete, able to boot an OS,
  113k lines, tested against Arm's validation suite); Sail RISC-V (complete, the official
  specification); for x86 the ACL2 `x86isa` model (Goel, Hunt, Kaufmann; 413 instructions
  with semantics as of 2018, decoding of every opcode map including AVX/AVX2/AVX-512, IA-32e
  64-bit focus, user and system views, Linux syscalls for user programs, validated by
  co-simulation against hardware), of which Sail holds an automatic translation. One
  importer from Sail's first-order imperative language into our data types covers all three;
  what each model lacks is its inventory (what x86isa decodes but does not implement; what
  Sail x86 carries of x86isa). Memory models: x86-TSO and the Armv8 model as published
  axiomatic definitions, imported as predicates over executions, with their integration with
  the instruction models as the work it is (Isla is the precedent). Below this is physics.
- **Binaries.** With `⟦x86⟧`, every ELF program has a meaning: the model applied to its
  bytes, under a loader (relocation, linking, initial state) and the OS world. This is the
  ground truth for every layer above: a compiler, an interpreter, a kernel, a NIC's firmware
  (its own ISA, imported the same way) each has an exact meaning without anyone writing it.
- **The OS.** The syscall interface as a world: each syscall a command with its response type
  and the contract the kernel honours. A trading box's hot path uses a few dozen, plus the
  kernel-bypass NIC APIs (ef_vi, DPDK), which are worlds too. The contract is an assumption
  until the kernel, which is C, is lifted; then it is a path.
- **Languages.** For each language L: the exact behaviour of its pinned implementation
  (Clang at a version and flags; CPython; V8), obtained by lifting that implementation's
  binary under `⟦x86⟧`; and a *structured reading* of the implementation's elaborated
  program into terms that keep L's abstractions (a template a λ over types, a class a record
  at its ABI layout, Python's `int` as ℤ), related to the exact behaviour by a path checked
  per program (section 9). Most languages' semantics *are* their reference implementation
  and that implementation is C or C++, so lifting C and C++ lifts the interpreters of
  Python, JavaScript, Java, Lua, Ruby, PHP and R; languages with precise cores of their own
  (Haskell, OCaml, Rust via rustc, Go via its own compiler, Lean and Coq as programs) follow
  the same pattern with their own readings. Conformance to a language's standard is a
  separate type, needed for portability claims and not for lifting deployed code.
- **Our own compiler** (section 5): the case where the source is already a term. The normal
  form is read into a structured machine program whose meaning is defined construct by
  construct, so `⟦read c⟧ ≡ eval-g c` holds by construction; the bytes are produced by fixed
  templates; `compile f` returns the bytes with the path, and the checker checks it. The JIT
  is the same at run time, cached by the content key of the specialized term.
- **Programs.** Terms, checked: the library as it stands.
- **Systems.** A node is a process over its world signature; the network is a world (a
  handler connecting sends and receives, with loss, reordering, delay, partition and the
  scheduling model as its responses); a fleet is the parallel composition of nodes under
  that handler, again a process over the residual world (clients, exchanges, time, the
  adversary). Properties are types over that term: safety as `Π (ρ : responses), Inv (trace
  ρ)`; liveness under a fairness predicate on `ρ`; refinement to a sequential specification
  as a path in the quotient of processes by observation; replica convergence by defining a
  replica's state as a function of the *set* of updates it has received (order-independence
  by construction) with eventual delivery as the hypothesis; Byzantine behaviour as a class
  of worlds; protocol conformance by projection from one choreography under its
  realizability conditions. Equivalence of two implementations relates their executions by
  a simulation, not by feeding both the same internal choices.
- **Infrastructure.** `deploy : Corpus → Config → Fleet` is a term: a box's binary is the
  corpus specialized against its configuration and role (10.2), its placement part of the
  fleet value. An observation of the running fleet is partial and delayed, so what is stated
  is that the observation satisfies the deployment's contract and that the execution follows
  the reconciliation protocol; a finite configuration comparison decides drift, behavioural
  equality does not. Configuration languages are lifted like any other, with the cloud API
  as a world. A rolling upgrade is two versions coexisting, compatible on the observed
  interface by an equivalence or a retraction where they differ; state migration is a
  relation between state types; hot loading is rebinding a key *and* accounting for the
  frames, closures and in-flight messages of the running instance.
- **The quotient.** An observation signature `Obs` and a projection `obs` from processes over
  the full world to processes over `Obs`. Two programs, in any two languages, are equivalent
  when `obs ⟦p⟧ ≡ obs ⟦q⟧`, a path in the quotient; both lie in the fiber over one term `r :
  Proc Obs`, the program with the execution details unspecified (C23's fiberization applied
  to programs). The path is a witness written or computed for the equivalence at hand; the
  quotient has a canonical form only where a normalization for *that* relation is defined
  (`ZQ` by `Int` is one); program equivalence in general is not decided by anything.
- **Identity across machines.** A content key names a closed definition under a semantic
  environment, and nothing more: not behavioural equivalence, not an effect's result, not
  authority, not attestation of what a remote process runs. Within that, two machines
  holding the same key hold the same meaning, so code mobility is sending keys with a fetch
  protocol, and a cache keyed by content cannot be stale. Distribution by keys (item 35) is
  the substrate of the tower across machines and is essential, not optional.

At every layer, what was trusted becomes a path when the layer below is lifted, so the
trusted base shrinks monotonically and ends at the ISA models and the hardware matching
them. The trading fleet (section 10) is the first instance because every layer appears in it
and its standard is exact. The order of work is section 12's queue.

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

## 3. State of the machine (revision 8, 2026-11-09, `main` at the header's commit)

Built, standing, and depending on nothing that was removed:
- The net running CCHM: paths, univalence via Glue, higher inductive types including
  truncations, `%hcompU`, generalized composition, constructor-matching composition for
  path-free data; canonicity enforced (a closed value of a path-free data type is a
  constructor). Composition clauses as terms (`lib/prelude.cub`). Coinduction by
  copatterns; guarded recursion with clocks and ticks.
- The checker as a term (`lib/check.cub`): conversion with η at Π, Σ and paths; levels;
  positivity, boundaries, coverage, termination (structural, course-of-values, measured);
  it reads the machine's values through `%view` and quotes them to codes at their types
  (`quote`/`qneu`), and `eval-g` (`lib/code.cub`) maps a code to the term it names, clause
  by clause (`clam b → λ x → …`): reflection, not a second evaluator.
- The surface language and elaborator: holes, dependent matching on path constructors,
  `decreasing by … using`, copatterns, `calc`, records, instances, parameterized modules,
  fording, `by-lin`, `--serve`, `#retract`, an LSP.
- Sharing and cost in the net: full laziness, memo by content keys, retention by
  reachability, wires as variables (no locks), per-worker deques; results identical at every
  worker count; a dimension substitution pending on every value with parts and applied one
  level when read (A8e, 2026-11-09); call by need in `build` (CBN, 2026-11-09). Steps of a
  read over the normal form's cells: g8 106 per cell, the corner 146, the generic 6-cube
  ~24 flat, the number and the typed port the computation's own work (18.16).
- Multiplicities computed after checking (B11); `represent` with its laws, the primitive
  types, `Bytes n` with proved accesses, `#layout`, `#trusted`, `#safe` (B12); the store by
  content keys (F34); arrays over the byte layer (C22); regions (C20).
- The mathematics library (`lib/std.cub`, `fiber`, `sort`), the verified certificate
  checkers (`linproof`, `eqproof`, `ring`, over `Int` through `intproof`), `derive-eq` over
  the data table, autodiff's typed theorems (`fwd-der`, `taylor`), the store's theorems as
  rewrite rules (`rules`, `match`, `nav`, `cost`).
- Processes in Church and guarded form, worlds, handlers, outcomes, supervision, replay,
  the drivers' signature `W` with the memory interface, named worlds, `foreign`, session
  types, choreographies with projection as specialization (`proc`, `world`, `session`,
  `choreo`, `decode`, `region`).
- Measured: Brunerie's `brunerie'` −2, 230,326 steps of the read after A8e; Agda 2.8.0 on
  the same machine 116 s CPU / 870 MB from nothing against 5.4 s / 139 MB (item 10).

Removed at revision 8 (section 17 has the reasons): the back end from codes to C, C++ and
x86 and everything that served it. At this revision nothing compiles to native code.
Section 5 is what replaces it and section 12's queue is the order.

Open, measured: g8 and the corner at 100–160 steps per result cell against the 6-cube's
~24 (the remainder is evaluation, not substitution); the 6-cube's normal form past 2.06M
cells does not fit a 2.5 GB cap; `surfc` with every dimension open does not finish. A8 is
the property and stays on the queue.

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
| at a consumer's build | a template or `constexpr` parameter of the image (section 10) |
| at run time, at a known moment | a specialization point, cached by the specialized term's key (the JIT) |
| never fixed | a parameter |

There is one residual term; the only decision per free variable is its binding time, read
off the term.

### 5.2 What compilation is here

Four things, all already one operation. *Binding time*: what is known is substituted when it
is known; the specializer is the machine normalizing a term with some inputs fixed and the
rest left as variables (`fibw #30` is `#832040`; `fibw w` is a stuck word elimination with
`w` free, and that stuck elimination *is* the loop). *Laws*: a rule is a theorem of the store
read as a direction (`rule-of`), and applying it is substitution along its path, so the
specializer can only produce a term equal to the one it was given; `plus x 0 → x` is
consumed before anything is emitted. *Representation*: an equivalence between a type and its
encoding, declared with its laws (section 6); the program is transported along it, and the
conversions cancel by the laws. *Control and the world*: a case is a switch, a constructor a
record at its layout, a word elimination a counted loop, structural recursion a recursive
function, a process's command a call into a driver.

The normal form that results is already correct: every step that produced it was a path.
What a compiler adds is only this: **spell the normal form in the target, with a path from
the target program's meaning back to the term.** That path is the one thing the removed
back end never had.

### 5.3 The target is a data type with a meaning

`M` is a data type in the language: the structured machine program. Its constructs are
exactly the forms a first-order normal form has, each paired with the meaning the hardware
gives it:

- word operations on `U64` (modular, section 6.2), `U32`, `I64` (two's complement through
  the unsigned operations, so no signed overflow is ever emitted), `F64` (IEEE binary64, the
  machine's `%f64*`, under pinned rounding and no contraction), comparisons;
- `mcheck op a b`: a word operation on *unbounded* `Word` operands, whose meaning is `ok (a
  op b)` when the result is below 2^64 and `overflow` otherwise;
- loads and stores at offsets in a region, each carrying its bound as a field (the
  `ASSUME` the source proof discharged is a hypothesis of the meaning, not a comment);
- `mloop n init step`, whose meaning *is* the word eliminator `welim n init step`;
- `mswitch d x alts`, whose meaning is the eliminator of `d`; `mrec d alts`, structural
  recursion; `mwhile m body`, recursion along a measure, the language's own;
- records as tuples of slots with the automatic layouts (6.1); `mclos env code`, a closure
  record whose meaning is the λ it closes (closure conversion is this one clause);
- `mcall g args`, application of a definition of the program; `mcmd c`, a command of the
  world (its meaning the process constructor, its bytes a call into a driver).

`⟦_⟧ : M → meaning` is one definition by recursion on `M`, with the meanings above. The
fragment is total by construction: its loops are the eliminators the checker already
requires to terminate; its division is written total (`d = 0 → 0`) and printed so; its
shifts carry the width bound. Undefined behaviour is not avoided at the end; `M` cannot
express it.

### 5.4 The reader, and why the path is by construction

A definition `f : T` is quoted by the checker at its type (`quote`), giving a code `c`.
`specialize` normalizes and rewrites `c` to `c'`, each rewrite carrying `ap (λ h → …) (thm
σ)` and the rewrites composed, so `eval-g c ≡ eval-g c'` is a term the checker checks. The
**reader** walks `c'` through the same view the checker uses and emits one `M` construct per
normal-form construct, under an **environment representation**: the variables in scope are
laid out in slots (registers and a frame), an equivalence between the code's environment
and a machine environment declared like any other representation, with its round trips by
computation on the structural cases. Reading is transport along it. Because `read` and
`⟦_⟧` are inverse construct by construct, `⟦read c'⟧ ≡ eval-g c'` is `refl`: the checker's
conversion normalizes both sides to the same normal form, and nothing is proved. The reader
is partial, and its partiality is B13's first-order criterion, produced where it is needed
rather than by a separate pass: a free dimension, a composition over an unknown dimension, a
higher inductive constructor without a canonical-form representation, a type depending on a
run-time value, have no `M` construct, and the reader names the construct and the reason
(`dim`, `kan`, `higher`, `dep-type`, `reflect`). What it refuses runs on the net, which is
always correct.

A `Nat` program meets the reader with unbounded `Word` operations. There is no `M` construct
whose meaning is unbounded addition on a 64-bit value, so either the program is written
over `U64` (as an engineer writes the hot path) and reads directly, or it reads into
`mcheck` and compiles to `p` with the source `f` as its fallback: the entry is `run p f a =
case ⟦p⟧ a of ok b → b; overflow → f a`, and the path checked is `run p f ≡ f`, `refl` on
each branch. A standalone binary with no machine behind it has no fallback, aborts on
`overflow`, and its meaning is partial, which is in its type. Both are forced by `⟦_⟧`; the
bug that ended the old back end cannot be written in this one.

### 5.5 Bytes, the boundary, and what is trusted

`encode : M → Bytes` maps each `M` construct to a fixed x86-64 instruction template under a
fixed frame and calling convention: about fifteen templates (the word operations, the two
comparisons and branch, the counted loop as `dec/jnz`, the switch as a jump on the tag, the
call and return, load and store, the checked operation as `add` plus `jc`, the SSE2 scalar
operations). The compiled definition is the pair `(encode (read c'), π)`. The trusted base
of the compiler is the fifteen templates, each one line in section 13 ("`add r, s` on
64-bit registers is addition mod 2^64", "`dec rcx; jnz top` over the body computes the word
eliminator"), and the frame convention. When `⟦x86⟧` is imported (item 36), each template's
line becomes a short equation between `⟦_⟧`'s construct and `⟦x86⟧` of the template's bytes
— one computation on literals for the straight-line templates, one induction on the counter
for the loop — written once, fifteen in all, after which the compiler's trust is the ISA
model's alone. No C compiler is in the path at any point.

The **call boundary**: a closed application of a compiled definition to constructor-built
data runs `p` in executable memory (the machine already has `mmap`/`mprotect` for this and
the two x86-64 call families of `foreign`), marshalling along the environment representation
and the ABI (a `U64` a register, a flat record a struct by value, a recursive value a
pointer into a region). The marshalling C in `src/lang.c` is in the trusted base beside the
drivers, listed. The **JIT**: when a value becomes known at run time, `specialize` runs on
the term with it substituted and the result compiles and is stored under the specialized
term's key; a key is the content and cannot go stale; a term that is not first-order after
substitution keeps running on the net. Allocation in compiled code is a region threaded as
state (`lib/region.cub`), compiled like any other definition; no allocator is trusted.
Output and input are commands of `W`, run by the drivers.

### 5.6 Multiplicities: erasure, uniqueness, linearity

Unchanged from revision 7 (18.1, built): usage is computed after a definition checks;
erasure is by computed usage, never by annotation; 1-linear values are introduced only by
primitives; uniqueness is structural in the net (box holder counts) and read by the
compiled code's in-place updates. A proof argument reaches nothing read at run time, so it
is erased and its fact becomes the bound field of the access it protected.

### 5.7 Schedules

Everything that fixes cost without fixing meaning (strictness, binding-time assignment,
representation choice, fusion depth, inlining depth, which definitions are held neutral,
register assignment) is a value the specializer and reader take, sound by construction, so
tuning is search over schedules with nothing to verify afterwards. Better register
allocation is rewriting along laws in `⟦M⟧`, still by construction.

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

## 9. Lifting: every language, by its implementation

To lift a language L is to give every L-program a meaning as a term and every L-abstraction
a counterpart that keeps it. Two objects, related by a path, and neither is written from a
standard.

**The exact behaviour** of L is the behaviour of its pinned implementation: Clang at a
version and flags, CPython, V8, HotSpot. Each is a binary, so under `⟦x86⟧` (0.1) each has
a meaning, and `⟦impl⟧ (p)` is what L does with `p`, including every undefined and
implementation-defined behaviour resolved the way the implementation resolves it. This is
the object that preserves deployed behaviour exactly, quirks included, and it is obtained
by lifting, not by transcription. It is *not* a correctness theorem: that the implementation
conforms to L's standard is a separate type, needed for portability claims, and stated only
when a formal object for the standard is related by a path to the implementation.

**The structured reading** is where L's abstractions become terms. For C++, Clang's parser
and Sema produce the elaborated AST: names resolved, overloads chosen, templates
instantiated *and kept* as declarations with their specializations, deduced arguments,
implicit conversions and `constexpr` results present as nodes. That AST becomes a data type
here, and the reading maps it into terms: a class template is a λ over types and
instantiation is β; a function template is a Π; a concept is a predicate on types and
`requires` a case on whether a code checks; a class is a record at its Itanium-ABI layout;
a virtual function is a field of the vtable record at the ABI's offset; `this` is an
address; a member access is a load at an offset in the byte tier; a reference is a pointer;
exceptions are explicit control flow with destructors at each unwinding step; coroutines a
frame record and a state machine; threads processes over the memory world (`w-load`,
`w-store`, `w-cas`, `w-fence` at the four orderings, `lib/world.cub`); allocation a region.
Every detail of memory interaction is in the byte tier with exact layouts; nothing is a
supported subset. The reading's λ for a template *is* the meaning assigned to the template;
no claim is made about the implementation's behaviour on instantiations not present.

**The path** between them is checked per program: `⟦clang(p)⟧_x86 ≡ obs (read (ast p))`,
the compiled binary's meaning under the ISA model against the reading's, both normalized
by the machine, under the observation that forgets the binary's registers and frame. Where
the two agree by computation nothing more is written; where a construct's agreement is not
computational, its equivalence is written once as a representation with its laws (section
6), which is the only mathematics in the lift. If the reading is wrong, the path does not
check; the reading needs no proof of its own. This also settles the one place the C++
standard itself is unsettled (relaxed atomics and out-of-thin-air executions): the ground
truth is the compiled code on the hardware's memory model, which is fully specified, so no
model is chosen.

**Bootstrap.** Lifting Clang's source needs the C++ lift, so Clang is lifted from its
binary (or its own bitcode) under the ISA model once the machine runs binaries of that size
(the measurement in 12's queue). Until then Clang's AST dump is an oracle for the elaborated
AST, with trust exactly in "the dumped AST is the one CodeGen consumed", and that oracle is
removed when the binary lift runs. The same applies to every reference implementation.

**The order of languages** follows from one fact: most languages' semantics are their
reference implementation and that implementation is C or C++. Lifting C and C++ lifts the
interpreters of Python, JavaScript, Java, Lua, Ruby, PHP and R; their structured readings
are small because the exactness comes from the interpreter. Languages with their own
precise cores (Haskell via GHC, OCaml, Rust via rustc with MIR as the structured level, Go
via its self-hosted compiler's binary) follow the same pattern. Lean and Coq lift exactly
*as programs*, so "accepted by Lean" has a meaning here; their theorems do not transfer
automatically (UIP contradicts univalence; impredicative `Prop` has no counterpart) and are
translated per fragment where the theories agree: a ledger row with a trade.

**Coverage.** A lift is complete on the constructs its models implement. The ISA model's
unimplemented instructions, the loader's unsupported relocations, a syscall without a
contract: each is an inventory item, and a program using one does not lift until it is
added. The inventories live with the imports (item 36).

## 10. The C++ image, and lifting a trading system

### 10.1 The C++ image

The image is the lift run backwards: a term's reading as C++ at the binding-time assignment
"as late as C++ can express" (dependencies on types and constants stay template parameters,
dependencies on run-time values become values with contracts, proofs are erased, and what
C++ cannot express at any stage — paths as objects, HITs without a canonical form,
dimensions, levels — is normalized away first). Form by form it is the reading of section 9
inverted: a function uniform in a type → a template; a record of operations with laws → a
concept and a constrained template; inductive data → a tagged struct, or the declared
representation's layout; codata and processes → senders or awaitables; paths → erased at
multiplicity 0; transport along a representation → the conversion pair; multiplicities → by
value, `const&`, absent, or move-only with `[[nodiscard]]`. Laws reach C++ only as
`[[assume]]` where an instance of a proven law is a C++ expression, and as contracts at
consumer boundaries; never as comments (revision 6.3). What makes the image *correct* is the
same path as the lift: the emitted C++, compiled by the pinned Clang, has a meaning under
`⟦x86⟧`, and `⟦clang(image f)⟧ ≡ obs f` is checked per export. The image is therefore not
built before the lift's machinery exists (item 39 in 12's queue); the earlier `lib/image.cub`
with re-import as its check was removed at revision 8.

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

**The queue as of revision 8 (2026-11-09).** Dependency order; each item is sized in lines
written at the calibrated rate (about 1.5 h per thousand) where its content is fixed, and
marked *measured* where a number decides it. Nothing is optional.

0. **Deletion** (~1 h): `lib/fo.cub`, `lower`, `emitc`, `emitu`, `emitt`, `emitx86`,
   `image`, `reimport`, `repr`, `stage`; autodiff's `PCode`, `cev`, `dpc`, `pl-of-pc`,
   `D-correct*` and the untyped bridge (the typed `D`, `fwd`, `taylor` stand); derive's
   `eq-fo` reading; `src/syntax.c`'s induced-record generation (`x_rep_induce`) and the
   `%rops`/`%rlaws` tables; `src/lang.c`'s `%natfb`/`%natsfb`/`%natcv`, `rep_chain`, and the
   `#native` path that emitted C through gcc (the `mmap`/`foreign` call machinery stays for
   item 18); `tests/`, `experiments/fo`, `tools/mtest.py`, `tools/names.py` (the elaborator
   refuses redeclaration instead, ~20 lines), the Makefile's lines for them, and every
   `#same` reading in `lib/` of an untyped definition. Kept: `experiments/brunerie` and
   `experiments/agda` (the A8 and item-10 measurements), the readings that are the only
   evidence for the trusted base (section 13, items 1–2), and the removed harness's programs
   as the acceptance list of item 19. README rewritten to the same state.
1. Items 14–18 of 12.B in order: `U64` and the machine types; `M` and `⟦_⟧`; the
   specializer's paths; the reader and `compile` with its **gate** (the conversion cost on
   `fibw` and the decoder); `encode` and the boundary. About 1.9k lines, ~3 h, then the
   measurement.
2. Item 19: the acceptance programs compiled with their paths and run.
3. **Item 36, the ISA import** (*measured*): the importer from Sail into our data types,
   covering the Sail translation of ACL2 `x86isa`, Sail Armv9 and Sail RISC-V in one; the
   coverage inventory of each (x86isa's implemented opcodes against the instructions our
   templates and our workloads' binaries use); the loader (ELF, relocations, initial state);
   x86-TSO as a predicate over executions. Then section 13 item 3's fifteen lines become
   equations. Size: the importer by Sail's grammar (first-order imperative with
   dependent bitvector types), a few thousand lines; the inventory by reading the model's
   `implemented-opcodes`. **Gate:** a small real binary's observed output under the imported
   `⟦x86⟧` on the net, and the net's steps per instruction, which sizes everything in items
   5–7.
4. **Item 37, the OS world**: Linux's syscalls as commands with contracts, the ones a trading
   box's hot path uses first; the kernel-bypass NIC APIs as worlds. ~3k lines for the first
   set.
5. **Item 38, the C++ lift** (section 9): Clang's AST as a data type; the reading, node kind
   by node kind, into terms at the Itanium ABI's layouts, exceptions, coroutines, threads
   over the memory world; the per-program path against the compiled binary under `⟦x86⟧`;
   Clang's AST dump as the oracle until Clang's own binary runs under the model (item 3's
   measurement decides when). The standard library lifted as the C++ it is, its OS-bound
   parts to worlds. Sized when the AST's node kinds are inventoried against Clang's
   `ASTNodeKinds`; the reading is large and fixed in content.
6. **Item 39, the image** (10.1), on item 38's machinery: the reading inverted, checked per
   export against the pinned Clang's binary.
7. **Item 40, the systems layer** (0.1): the network world with its delivery model and
   scheduling; composition of processes under a handler; the observation quotient and the
   fiber over `Proc Obs`; replicated state as a function of the received set; `deploy` and
   the deployment contract; migration relations; distribution by keys (item 35, essential).
   On the existing `proc`, `world`, `session`, `choreo`. ~6k lines.
8. **The fleet** (10.2): the existing C++ lifted through item 38 with its abstraction
   representations, the strategies as processes, the per-box specializations, the OS and
   NIC as worlds, latency against a cost model over `⟦x86⟧`.
9. **Other languages** (section 9's order), each by its reference implementation's binary
   under the model once item 3's measurement allows, with its structured reading.

Throughout: A8 (item 8) stays on the queue as the property every lift's cost depends on,
since specializing an interpreter's binary on a program is normalization with open inputs
at the scale of the binary; the 6-cube's memory per result cell and g8's steps per cell are
its current numbers.

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
decisions B11–B13, which are made; the compiler (12.B items 14–19) is definitions over them
with no proof work of its own, and the lifts (36–40) are imports, readings and their
inventories. D and E are independent of B. F follows B and E. The typed
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
   **Status (2026-11-09, A8e, 18.16):** a dimension substitution is now pending on every value with
   parts (held in a box, a reference carrying it, applied one level when read, cut to the value's
   free dimensions), not walked: neutrals re-run their operation and systems decide their faces
   only when read; the operations built per branch of a system (`distribute`, a composition over a
   system of families, a binder over a system of lines, `sys+`'s forces) and the operands read
   only on some outcomes (`%@`'s endpoints, a composition's base, an hcomp's cap) are `V_DEFER`
   cells, run by their reader and erased unrun. Steps of the read alone (memo on, 0 workers),
   before → after, over the normal form's cells: g8 305,036 → 161,006 (1,566 cells; 195 → 103 per
   cell), surfc's corner 121,261 → 6,322 (39; 3,109 → 162), the number 443,066 → 230,326 (4), the
   typed port 999,755 → 352,234 (1). The generic 6-cube still passes the 2.5 GB cap, but now reads
   back at a flat ~27 steps per result cell (before ~300): its normal form is past 2.06M cells, so
   the cap is the result's size, not work beyond it. Not flat: the number and the typed port
   (computing a winding number: the work is the computation's, not substitution's), and g8 and the
   corner at 100–160 steps per cell against the 6-cube's 27 (what remains is evaluation: β, the
   composition clauses, erasing the unread parts of what was read; 18.16 lists it).
   **Status (2026-11-09, CBN, 18.16):** call by need in `build`: every operation in an operand
   position is a deferred cell (`V_DEFER`), its operands deferred too; strict only where always
   read (a principal, a plain path application's path, a meet or join's second operand, a
   transport's value, a call's always-read parameters), run with the deferred operation without a
   step; one named exception (a word operation's second operand: deferred, native calls reorder the
   compiled-code cache's printed evictions). Steps of the read alone over the normal form's cells:
   g8 161,006 → 166,158 (103 → 106 per cell), the corner 6,322 → 5,710 (162 → 146), the number
   230,326 → 233,734, the typed port 352,234 → 358,609; the generic 6-cube's read-back rate 24.9 →
   23.6 steps per cell (10M–50M steps, 2.5 GB cap). Each read operand costs its reader one
   interaction, and in g8 the deferred operands are nearly all read (a composition's family above
   all), so g8 does not fall; what remains unread is built by rules (`distribute`, composition
   clauses) and the reads' own erasure of unread parts.
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
    0.3 s of it, the rest library checking); 619,852 steps, −2 at every count. Brunerie's
    original number at 1–16 workers is open. Demonstration.
    **Same-machine comparison with Agda: measured 2026-11-09** (`experiments/agda/measure.sh`
    reproduces it: `setup`, `agda`, `machine`; logs under `/tmp/agda/logs`). Machine: 2 cores
    (Intel Xeon @ 2.10 GHz), 8 GB, shared with other agents (load average 1.4–4.4 during the
    runs, so wall times vary by up to 2×; CPU time, user+system over all threads, is the steadier
    figure). Peak RSS is VmHWM and CPU time utime+stime, both polled from `/proc` every 50 ms;
    every run went through `heavy.sh` (`--big` for Agda). Neither the 3 h nor the 7 GB cap was
    approached.
    *What each side computes.* The same formulation on both: Ljungström's `brunerie'`,
    `g10 (g9 (g8 λ i j → f7' (η₃ (push (loop i) (loop j)))))`, which starts from π₃(S³) via η₃
    and is not the original `brunerie` (the π₃(S²) pipeline f3…g10, marked "don't run me" in the
    library; not run on either side). Agda checks agda/cubical's own definition and its
    `brunerie'≡-2 = refl`; this machine runs the untyped port (`experiments/brunerie/brunerie.cub`)
    and the typed port (`typed.cub`). Both read −2 (Agda: refl accepted; machine:
    `negsuc (suc zero)`). The libraries differ: from nothing Agda checks 202 modules (BrunerieExperiments, the 182
    agda/cubical modules it depends on, 19 builtins), this machine its own `prelude`/`code`/`check`/`std`.
    *Versions.* Agda 2.8.0, the prebuilt `Agda-v2.8.0-linux.tar.xz` from the GitHub release
    (static, GHC 9.8.2, `optimise-heavily`; sha256 `824081b8…2632faf`), default RTS options;
    v2.8.0 is the `AGDA_BRANCH` of agda/cubical's CI at the commit used, `c37782bb` (2026-10-08,
    library `cubical-0.9`; flags from its `.agda-lib`). This machine at `d72a42f3`,
    `build/cubical` by `make build/cubical` (gcc -O2).
    *Agda* (`AGDA_DIR` with `libraries` and `defaults` naming `cubical-0.9`):
    | run | wall | CPU | peak RSS |
    |---|---|---|---|
    | `agda BrunerieExperiments.agda`, no interfaces (2 runs) | 120.3 / 118.0 s | 116.3 / 116.7 s | 870 / 868 MB |
    | the same in two parts: its 20 imports alone | 124.2 s | 122.8 s | 863 MB |
    | then BrunerieExperiments on their interfaces | 5.3 s | 5.2 s | 408 MB |
    | interfaces present: a module importing it (load only, 3 runs) | 4.45–4.76 s | 4.28–4.62 s | 395 MB |
    | interfaces present: `again : brunerie' ≡ -2; again = refl` (3 runs) | 4.85–5.13 s | 4.71–5.02 s | 418–419 MB |
    Agda's profiler (`--profile=definitions`): `brunerie'≡-2` takes 296 ms of the module's 5,066 ms;
    `again` 328 ms of 4,647 ms in the refl module, the other 4,319 ms loading interfaces.
    *This machine*, `CUBICAL_NOCACHE=1 CUBICAL_WORKERS=w ./build/cubical lib/prelude.cub lib/code.cub
    lib/check.cub lib/std.cub experiments/brunerie/brunerie.cub experiments/brunerie/run-brunerie.cub`,
    and for the typed port `… experiments/brunerie/typed.cub experiments/brunerie/run-typed.cub`
    (the Makefile's lines), 3 runs each, median (range):
    | run | w | wall | CPU | peak RSS | steps for the number | checks |
    |---|---|---|---|---|---|---|
    | untyped, end to end | 0 | 7.4 s (6.7–13.2) | 5.4 s (4.7–5.7) | 137–139 MB | 443,066 | 175, 0 failed |
    | | 1 | 10.5 s (9.8–11.5) | 5.6 s (5.4–6.5) | 166–172 MB | 473,691 | 175, 0 failed |
    | | 2 | 9.4 s (8.6–12.9) | 6.6 s (5.9–6.7) | 172–180 MB | 473,691–473,793 | 175, 0 failed |
    | typed, end to end | 0 | 11.6 s (9.2–18.4) | 8.5 s (8.0–8.6) | 171 MB | 999,755 | 242, 0 failed |
    | | 1 | 16.5 s (14.3–19.0) | 9.5 s (9.4–10.1) | 229–231 MB | 1,068,392–1,068,398 | 242, 0 failed |
    | | 2 | 16.8 s (15.3–22.4) | 11.0 s (10.9–12.1) | 235–249 MB | 1,068,398 | 242, 0 failed |
    | untyped, library from the module cache and store, number read afresh | 0 | 0.54–1.22 s | 0.34–0.46 s | 43–45 MB | 443,066 | |
    | | 1 | 0.98–1.29 s | 0.35–0.39 s | 41–46 MB | 473,691 | |
    | | 2 | 0.94–1.55 s | 0.41–0.50 s | 44–47 MB | 473,691 | |
    | the cached library alone (no driver) | 0 | 0.37–0.43 s | 0.16–0.26 s | 38 MB | | |
    Workers bring no gain on two shared cores: median CPU time rises from 0 to 1 to 2 workers
    (5.4, 5.6, 6.6 s untyped; 8.5, 9.5, 11.0 s typed), and median wall time at 1 and 2 is above 0's.
    *Reading the comparison.* End to end from nothing: Agda 116 s CPU / 870 MB, this machine
    5.4 s / 139 MB untyped and 8.5 s / 171 MB typed — about 21× and 14× less CPU, 6× and 5×
    less memory, though over different libraries (agda/cubical's general modules against this
    machine's smaller prelude and std), so this is the cost of getting to the number on each
    system, not a like-for-like checking benchmark. With the library already processed:
    Agda 4.7–5.0 s / 418 MB, of which loading interfaces is about 4.3 s and the number 0.30–0.33 s;
    this machine 0.34–0.46 s / 45 MB in all, of which the cached library is 0.16–0.26 s and the
    number about 0.1–0.3 s (the difference of the two rows, at the 50 ms resolution of the polling).
    The number's own computation is the same order on both (a few hundred ms); the gap is in
    loading and checking, where A9 (load time) applies. **10b. DONE (`546bdb8`):**
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
Revision 8 replaces items 14–19 of revisions 7.x (the first-order form, the specializer's
lowering, the C back end and JIT, the call boundary over emitted C, the C++ image). What
they built is removed and section 5 is the specification of what replaces them. B11–B13
stand.
11. ★ DONE (`0729899`, 18.1): multiplicities, static side.
12. ★ DONE (`01c839c`, 18.2): representation equivalences as a construct, the primitive
    types, `Bytes n`, layouts, `#trusted`; quotients by canonical forms (`ZQ` by `Int`).
13. ★ DONE as a criterion (`1c5282a`, 18.3); at revision 8 the verdict is produced by the
    reader where a construct has no `M` form (5.4), with the same reasons; `lib/stage.cub`
    as a separate pass is removed. `Sched` moves to the specializer's and reader's schedule
    (5.7).
14. **`U64` and the machine types** (5.3): `U64` modular with its operations and ring laws
    beside `Int64`/`UInt32`; `F64`'s operations the machine's `%f64*` with pinned rounding
    and no contraction, their specifications listed by `#trusted`. ~300 lines. Essential.
15. **`M` and `⟦_⟧`** (5.3): the structured machine program as a data type and its meaning,
    one clause per construct, total by construction. ~500 lines. Essential.
16. **The specializer's paths** (5.4): each rewrite carries `ap` of its theorem, composed;
    `specialize` returns the code with the path, and the checker checks it. `match`,
    `rules` and `nav` become certificate producers. ~150 lines in `lib/specialize.cub`.
    Essential.
17. **The reader and `compile`** (5.4): the environment representation; `read` from the
    quoted normal form into `M` through `%view`, with its failure reasons; `compile`
    returning `(encode (read c'), π)` with `⟦read c'⟧ ≡ eval-g c'` by `refl`; the `mcheck`
    route for `Word` programs with `run p f ≡ f`. ~500 lines. Essential. **Gate:** the cost
    of the checker's conversion on the path, measured on `fibw` and the tagged-message
    decoder before anything is built on it (expected linear in the normal form; it is
    checking, not proving).
18. **`encode` and the boundary** (5.5): the fifteen templates and the frame convention,
    each a line in section 13; the bytes into executable memory through the machine's
    existing `mmap`/`mprotect` and `foreign` call families; marshalling along the environment
    representation and the ABI; the JIT by the specialized term's key; allocation through
    regions; I/O through `W`. ~400 lines in `lib/` and `src/lang.c`, reusing the crossing
    code that matches and deleting the rest. Essential.
19. **Acceptance programs**, the workloads kept from the removed harness, each compiled
    with its path and run at the boundary: fib by word elimination, the tagged-message
    decoder with its bounds, the region program with in-place stores, the record crossings
    (`Pt`, `Shape`, `Seg`), a process against its world, a `Nat` program through `mcheck`
    with the fallback taken past 2^64. These are the first compiled programs of the system;
    their answers are read on the net and natively from the same term, and the path is what
    says they agree.

### C. Memory and state
(The "compiled" and "in place" readings recorded in items 20–22 below went through the back
end removed at revision 8 and are history; the byte layer, the regions, the arrays and their
laws stand, and compiling them is item 19's acceptance list.)
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
31. DONE in parts (revision 8: autodiff's `PCode` mirror — a copy of a fragment of `Code`
    with its own evaluator `cev`, the theorem `D-correct` stated over the copy and tied to
    `Code` by readings — is removed as an instance of the error section 17 records; the
    typed theorems `fwd-der` and `taylor` over `Pl` stand): `deriving eq D` (`1c5282a`), `deriving show`/`derive-show` (`896713ea`),
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

### G. Scale, and the lifts
35. Distribution by keys over wires. Essential (0.1: the substrate of the tower across
    machines; a key names a closed definition under a semantic environment, with a fetch
    protocol, dependency resolution and execution authority as their own definitions). The
    net on GPUs: later.
36. **The ISA import** (queue item 3; 0.1, 5.5): the importer from Sail; x86isa's, Armv9's
    and RISC-V's models as data types with `⟦_⟧`; coverage inventories; the ELF loader;
    x86-TSO. Essential; *measured* (steps per instruction on the net).
37. **The OS world** (queue item 4): syscalls as commands with contracts; kernel-bypass NIC
    APIs as worlds. Essential.
38. **The C++ lift** (queue item 5; section 9): Clang's AST as a data type, the structured
    reading at the ABI's layouts, the per-program path against the binary under `⟦x86⟧`,
    the oracle and its removal, the standard library. Essential; the largest fixed-content
    item in the plan.
39. **The image** (queue item 6; 10.1): the reading inverted, checked per export.
40. **The systems layer** (queue item 7; 0.1): the network world, composition under a
    handler, the observation quotient, replicated state over the received set, `deploy`
    and its contract, migration relations. Essential.

**After the build.** The system extends itself: new components are terms, judged by the same
normalization as user programs, accepted into the store under their keys; the theory contains
its own theory, so no layer is outside it, and for what counts as a valid term the rules are
the only thing trusted, a few thousand lines (what else is trusted today is section 13). That is the autopoiesis the premises ask
for, and the trajectory after the build is the system doing this across machines.

## 13. The trusted surface

Everything not checked by the checker, in full. Each item names how it leaves the list.
1. The rewrite rules (`src/net.c`) implement CCHM, terminate on checked terms, and satisfy
   canonicity. Tested, not proved: the readings of the library, workers against one thread,
   TSan and ASan. Leaves the list when the metatheory is done in the system.
2. The checker term is correct relative to the rules; the quote/eval laws hold. Tested by
   the `#fail` suites and the laws' readings. Same.
3. The fifteen `encode` templates and the frame convention (5.5), one line each: `add r, s`
   on 64-bit registers is addition mod 2^64; `sub` likewise; `imul`; `shl`/`shr` with the
   count below 64; `cmp` then `jb`/`je` is the comparison; `dec rcx; jnz top` over the body
   computes the word eliminator at the counter; a jump table on the tag is the switch;
   `call`/`ret` under the frame is application; `mov [base+off], r` and its load are the
   store and load at the offset; `add; jc` is the checked operation; `addsd`/`subsd`/
   `mulsd`/`divsd` under round-to-nearest-even with no contraction are IEEE binary64;
   `cvt` and the bit moves are the identities they are. Leaves the list when `⟦x86⟧` is
   imported (item 36) and each line is written as the equation between `⟦_⟧`'s construct
   and the model's meaning of the template.
4. The marshalling C at the call boundary (`src/lang.c`), the drivers (`src/drivers.c`), and
   each `foreign` declaration, listed by `#trusted`. The drivers and `foreign` are the
   boundary with the OS and leave the list as the kernel is lifted (0.1).
5. The specification postulated for each floating-point primitive (`%f64add` is IEEE
   binary64 addition, …), until the operations are defined in the language.
6. Imported models and their importers (item 36): the ISA model describes the hardware
   (validated by its authors against silicon; the floor), and the importer preserves it (in
   the list until its outputs carry their own evidence; round-tripping proves nothing).
7. Oracles named per lift and removed per lift (section 9): a reference implementation's
   elaborated-program dump, until that implementation's binary runs under the ISA model.
8. The OS contract and the loader, as hypotheses of every compiled program's meaning, until
   lifted.

Nothing in any compiler, reader, specializer, matcher, importer or lift is on this list:
each produces outputs that carry their paths, and a wrong one produces an output that does
not check.

## 14. Measured facts to keep
Brunerie simplified: 1.26M steps, −2; g8 1.136M; 6,706 unkeyed transports; 291 hits;
substitution+erasure 53%; library load 1.0 s cached, 1.4 s uncached (one thread); compute
about 0.3 s; end to end 3.1 s (one worker) / 2.7 s (one thread) on the builder's box; memo
hits 248 on one thread, 9 with workers; Agda on the same machine (item 10, 2026-11-09, Agda 2.8.0): the
same construction's refl check 0.30–0.33 s by Agda's profiler, after 4.3 s of interface loading; from
nothing 116 s CPU / 870 MB against this machine's 5.4 s / 139 MB (untyped port). Nested-loop transport
linear: 4.2k/9.1k/19.0k/78.4k (left), 5.1k/12.0k/27.0k/146k (right) at 4/8/16/64 loops.
`fibw 90` by word elimination over a pair reads in 2,416 steps on the net (words are exact:
`fibw 1000` reads its 209 digits); its compilation as a counted loop with its path is item
19's first acceptance program (the earlier C-loop figure went through the removed back end).
Reflection (`lib/ring.cub`): `(x+1)^n` against its right-nested product checks in 9.6k / 15.5k /
24.2k / 37.3k steps at n = 2/3/4/5, `(x+y)^n` in 18.7k / 30.3k at n = 3/4 — the normalizer's
cost, roughly the number of monomial insertions; the proof term is constant.

## 15. Open threads
- The memory model for relaxed atomics in *source* C++ (out-of-thin-air, RC11 and the
  promising semantics): dissolved for lifted code, whose ground truth is the compiled
  binary on the hardware's model (x86-TSO, the Armv8 model), which is fully specified; open
  only as a conformance claim about a compiler, if one is ever wanted.
- Two measurements the queue depends on (section 12): the checker's conversion cost on a
  compiled program's path (item 17), and the net's steps per instruction running a real
  binary under the imported `⟦x86⟧` (item 36), which sizes every lift by implementation.
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
Revision 8 (2026-11-09): the back end from codes to C, C++ and x86 removed with everything
that served it (section 12's queue item 0 lists the files); section 5 rewritten as the
target's meaning `⟦_⟧` over a structured machine program `M`, the reader with its path by
construction, `encode` by fifteen trusted templates, and the boundary; section 0.1 added
(the tower); section 9 rewritten as lifting by implementation with structured readings and
a checked path per program; 10.1 restated as the lift inverted; 12.B replaced by items
14–19 and the queue by items 0–9 and 36–40; section 13 enumerated in full with how each
item leaves the list; 18.5–18.9 withdrawn. **Retracted, with the record of how each was
stated and corrected:**
- "Compiling is transport" was stated as a property of the code from revision 6 to 7.9 and
  in item 15's reports of 2026-11-09 ("lowering as transport", "representations consumed in
  the term"). At no point did any compiled program carry a path to its term; the compile
  path was passes between syntaxes related by comparing answers. The statements were
  false of the code throughout.
- The checked model of the compiler (`lib/foproof*.cub`, 2026-11-07 to 11-09, ~14k lines)
  proved properties of copies of the compiler's syntaxes and tied them to the compiler by
  readings: the opposite of the premise; removed at `d72a42f3`.
- The printers' primitive table (`P-add` as `uint64_t +`, `add rax, rcx`) asserted that the
  machine's unbounded `Word` is a 64-bit word; compiled programs answered wrongly past 2^64.
  The 32-bit operations had been lowered to the 64-bit ones the day before (fixed in
  `4d2f7e4f`, but the class was not fixed). Floating point was compiled by `cc -O2` with no
  `-ffp-contract=off`, so contraction was permitted and compiled float results could differ
  from the machine's; not demonstrated, recorded as what the flags allowed.
- Item 15's status of 2026-11-09 ("the specializer is consistent with the premise") was
  partly true (its rewrites are along theorems) and partly not (it emitted no path); the
  rest of the status described passes.
- Proposed and withdrawn the same morning (2026-11-09): a C++ semantics written from the
  standard (a model beside the implementation that runs); "lifting Clang gives the complete
  C++ semantics" (an exact object, not a theorem; conformance is a separate type); a
  proof-assistant frame with "proof obligations", "simulation certificates", a
  "CompCert-shaped backend proof" and "proof-search cost" (there is no proof library; every
  equality is a path the computation produces, and the compiler's one-time trust is fifteen
  template lines); "`vector ≃ sequence`" as an equivalence (a representation with retained
  state, or a quotient, chosen by what is observed); CRDT convergence from merge laws as
  HIT paths (the state is a function of the received set; convergence needs delivery); a
  general "computable quotient" of processes by observation (canonical forms only where a
  normalization for the relation is defined); `observed ≡ deploy …` as drift (an
  observation is partial; the claim is a contract); "round-tripping an importer removes it
  from trust" (it does not); an eight-hour import of Vellvm as a complete LLVM semantics
  (Vellvm covers the sequential core and lacks `invoke`/`landingpad` and concurrency;
  `⟦LLVM⟧` is not on the critical path at all, the per-program path for lifts is against
  the binary under `⟦x86⟧`); a 70-hour total (priced writing, not the imports' inventories
  or the measurements; withdrawn in favour of per-item sizes and two gates).
- Process, 2026-11-07 to 11-09: agents given open-ended goals; work left uncommitted
  through a server restart; a suite run on an agent's own test list rather than the full
  one. Rules now: commits at every green point and at most 15 minutes of uncommitted work;
  the full suite before anything reaches `main`; no agent is given a goal without a
  specification it can be checked against.
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


### 18.5–18.9 Withdrawn at revision 8

The specifications of the first-order form (B14), the specializer's lowering (B15), the C back end and JIT (B16), the call boundary over emitted C (B18) and the C++ image (B19) described passes between syntaxes related by comparing answers; what was built against them is removed and section 5 is the specification that replaces them. The git history before `ccafee2d` holds the text.

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

**Status (2026-11-09, CBN: call by need in `build`).** A8e left the operands that compilation
builds at once: the operations in operand positions that `build` (`src/net.c`) made as running
cells (a composition's family, a system's branch and rest, a typed path application's path,
`sysmap`'s function, a glue's type, an unglue's value, an hcomp's cap and family, a transport's
lines, a case's environment, a primitive's operands, a constructor's dimensions, a data type's
parameters), A8e's `build_deferred` covering three of them one level deep. **Built:** `build_m`
builds every operation in an operand position as a `V_DEFER` cell, its own operands deferred the
same way, so nothing below an unread operand runs; values are built as they are, their parts
deferred. Built strict are only what is read as soon as the building is: a body's result, and
through it the inputs its operation always reads in every outcome but a stuck one (`syn_reads`,
`defer_reads`): the principal, a plain path application's path, a meet or join's second operand,
a transport's value (read, or the result itself where ψ holds), a call's parameters its
definition always reads (lang.c's analysis, `G.strict`, through `global_strict`). In a deferred
operation those inputs are deferred too and are run with it by `defer_build`, without a step,
as strict building would have built them; every interaction is still a step, a deferred cell's
reader included. `defer_build` hands the deferred cell's key and pending memo entry to the
operation (memo keys name the same values: the cell carries the operation's `synkey`, registered
at build as before); a duplicator holds a deferred cell once (`rule_dup` boxes unbuilt cells, as
before); `box_fd` is unchanged (one worker only); `fd_taint_of` refuses a deferred operation
exactly as `fd_pending` refuses a pending one (`O_MKDIM`, `O_GLOB`, `O_ERA`, a tainted case table
or native); a deferred operation read under a substitution is substituted one level when its
value arrives (`sub_read`), not held and read again. lang.c's operand positions that were already
unbuilt (application and non-strict call arguments, fields, `let` values, lifted subterms: `V_THUNK`
records) stay records. **Named exception:** a word operation's second operand (`%weq %wlt %wadd
%wmax %wsub`; read after the first unless the first is neutral) stays strict: deferred,
`tests/native-at.cub`'s native calls in it run after the first operand's instead of beside them,
and the compiled-code cache's evictions, which the run prints, come in another order (the
answers are the same; the output line is not).

Measured (steps of the read alone, memo on, `CUBICAL_WORKERS=0`; size = cells of the normal form):

| read | steps before (A8e) | steps after | size | per cell before | per cell after |
|---|---|---|---|---|---|
| g8 | 161,006 | 166,158 | 1,566 | 103 | 106 |
| surfc's corner (`@ 0 @ 0`) | 6,322 | 5,710 | 39 | 162 | 146 |
| the number | 230,326 | 233,734 | 4 | 57,582 | 58,434 |
| the typed port | 352,234 | 358,609 | 1 | 352,234 | 358,609 |
| the generic 6-cube (2.5 GB cap) | 50M steps: 1,861,602 cells | 50M: 1,960,694 | past 1.96M | 26.9 (24.9 from 10M to 50M) | 25.5 (23.6) |

**Why g8 does not fall:** call by need costs one interaction per operand read (the reader meeting
the deferred cell), and in these reads the deferred operands are nearly all read. In g8's read the
forced deferred cells are a composition's family read by `COMPA` 1,295 times (the tube is partial
in 1,433 of 1,454 compositions), a typed path application's path 286 (`PAPPP`), a system's rest
387 (`SYSCONS2`); `COMPA` steps go 1,433 → 2,726 and `SUBST` 38,183 → 40,018 (the family held and
read under substitutions as a deferred cell). The corner, where more operands go unread, falls
10%. What the reads build that the result does not contain is now what rules build (`distribute`,
the composition clauses' applications) and the reads' own cost (`ERA·REF`: the unread parts of read
cells), not compiled operands. **Tried and not kept:** (a) lang.c's operand positions compiled in
place as deferred cells rather than `V_THUNK` records (`compile_lazy` without the record, explicit
`delay` and definitions' bodies kept as records): g8 165,192, the corner 6,021, the number 238,892,
the typed port 368,010; better than records on g8 only. (b) The same with definitions' bodies in
place too: g8 138,280 and the number 152,649, but only because each definition's body is then
evaluated at load, outside the read; not a saving. (c) Formulas over bound dimensions (`~ ∧ ∨ =`
on dimension cells and constants) built at once rather than deferred: no change on any read.
**Correctness:** every line of `make test` gives the same output as main's binary with step
counts stripped (separate copies; the only difference is the compiler's warning line numbers in
the `native-main` build, from the one line added to `lang.c`), Brunerie −2 untyped and typed at 0
and 4 workers among them; g8's and the corner's normal forms identical with and without the memo
and to main's; without the memo the output is identical across 0, 1, 2, 4, 8, 16 workers on the
determinism list; TSan clean on the `make tsan` lines (three rounds, one at a time); ASan clean on
the `make test` ASan lines.

**Status (2026-11-09, A8e: substitution pending on every value with parts).** The diagnosis:
`rule_subst` applied a dimension substitution at once and structurally — (A) built values (λ
environments, pairs, Π, Σ, PathP) cloned with the substitution pushed into every part, whether or
not the dimension occurs or the part is read; (B) neutrals turned back into their operations at
once (`V_NAPP`→`O_APP`, `V_NCOMP`→`O_COMP`, `V_NCASE`, `V_NTRANS`, `V_HCOMP`, `V_TRANSH`, `V_NPRIM`,
`V_NWELIM`, `V_CON` through `con_build`), what was computed erased and the operation dispatched
again; (C) `V_SYS` split per clause of every substituted face, every branch substituted; (D)
`O_COMPH` on a system type and `distribute` building an operation for every branch; (E) the
collapse decisions (a tube the substitution makes total, a branch it entails, the constructor
boundary's `O_FACEOF`/`O_CONCHK`) deciding from formulas first, but after A–C had walked
everything. Boxes already had the mechanism (a reference carries its pending substitution,
composes new ones, drops them where the box's free dimensions do not meet them, applies them one
level when read); it is now the rule for every value. **Built** (`src/net.c`, `src/core.h`,
`src/lang.c`): (1) a substitution meeting a value with parts — anything but a reference, an
unbuilt subterm, a variable, a formula, a global — holds the value in a box and returns a
reference carrying the substitution (`rule_subst`); nothing is walked, cloned or dispatched.
(2) Reading such a reference copies one level and applies the substitution to it (`ref_read` →
`sub_read`, an `O_SUBST` with x = 1, `subst_level`: the old structural rule, one level), each part
a reference carrying the substitution on. Reading a neutral re-runs its operation then; reading a
system decides from its substituted faces first: a branch the substitution entails is that value,
the rest are erased unread, and the others get the substitution pending. (3) At the hold, on one
worker with the memo, the value's free dimensions are walked (`fd_cell`, as `box_fd` does) and
kept on the box, and a substitution that does not meet them is dropped there (the value passed on
unchanged). The walk never runs with workers (the race 6007e3f4 fixed); with workers or without the
memo the substitution is held uncut, so step counts without the memo stay schedule-independent.
(4) The operations built per branch are `V_DEFER` cells (a new value kind: an operation not yet
run — its kind, payload and inputs — run by its reader, `defer_build`; held once by a duplicator;
erased unrun; its free dimensions its inputs' and payload's less a binder's, refused when running
it may make a dimension): `distribute`'s per-branch eliminators, `O_COMPH`'s per-branch
compositions over a system of families and the lines they compose in, `rule_mkplam`'s per-branch
binders over a system of lines (it read every branch one level to decide whether it was itself a
system), `O_SYSCONS2`'s per-clause forces. (5) Operands read only on some outcomes are built with
their top operation deferred (`build_deferred`): a typed path application's endpoints (read only
when the path is neutral at an end), a composition's base and an hcomp's cap (erased when the
tube is total). E follows: the decisions now come before anything is read. (6) Keys: a value
substituted at a read is named `K_SUB` over its box's key and the substitution (`key_under`); the
clone used to keep the unsubstituted value's key. (7) The readback size counts the result's cells,
not the references, thunks and deferred cells it is read through (earlier counts included
references: g8's 2,189 was 1,566 cells and 623 references, the corner's 47 was 39; the figures
below are counted the new way, before and after). `CUBICAL_PROGRESS` also prints the cells read
back so far; the profile prints holds and drops (`subst-held`, `subst-dropped`) and rule-origin
erasures above 100. Every interaction is still a step, a read's level included.

Measured (steps of the read alone, memo on, `CUBICAL_WORKERS=0`; size = cells of the normal form):

| read | steps before | steps after | size | per cell before | per cell after |
|---|---|---|---|---|---|
| g8 | 305,036 | 161,006 | 1,566 | 195 | 103 |
| surfc's corner (`@ 0 @ 0`) | 121,261 | 6,322 | 39 | 3,109 | 162 |
| the corner at `@ a @ 0` | 121,260 | 6,324 | 40 | 3,032 | 158 |
| the number | 443,066 | 230,326 | 4 | 110,767 | 57,582 |
| the typed port | 999,755 | 352,234 | 1 | 999,755 | 352,234 |

The corner's `%comp-path` entries went 3,420 → 172. **The generic
6-cube** (the corner's read with `@ a @ b` over six λ dimensions, polled under a 2.5 GB resident
cap): killed at the cap both before and after, but `CUBICAL_PROGRESS` now shows why: its normal
form is large. After: 2,062,458 cells read back at 55M steps (29 s), ~27 steps per cell read back
and flat through the run (255,636 at 10M steps, 1,058,703 at 30M), 8.5 live cells per cell read
back. Before: 518,846 cells read back at 160M steps (45 s), ~300 steps per cell, 33 live cells per
cell. So the cap is the result's size (past 2.06M cells; how far past is not known), not work
beyond it. **Tried and not kept:** a substitution applied one level to a constructor at once, its
parts pending (g8 158,174, the corner 5,917: 2–6% fewer steps, but it builds the substituted cell
whether or not it is read, against (1)); `sys+`'s rest built deferred (g8 +2.4%, the corner +1%);
`sys+`'s branch and a pair's components built deferred (no change: they are values already).
**Correctness:** every line of `make test` gives the same output as before with step counts
stripped (`build/native-main` fails to link on both, `memend_report`, fixed on main by 6ba4acec);
Brunerie −2 untyped and typed with 0 and 4 workers; g8's and the corner's normal forms identical
with and without the memo; without the memo the output is identical across 0, 1, 2, 4, 8, 16
workers on the determinism list; TSan clean on the `make tsan` lines (three rounds, one at a time);
ASan clean on the `make test` ASan lines. **What is not flat:** the number and the typed port are
a winding number's computation, a 4-cell and a 1-cell result: the work is the computation's.
g8 and the corner sit at 100–160 steps per cell against the 6-cube's 27; in g8's read the steps
are a quarter `SUBST` (half of them a substitution composed onto a reference when a level is
read), a fifth `ERA` (two thirds of it references: the parts of read cells that are not read),
then `MKPLAM`, `APP`, `PAPPP`, `HCOMP` — evaluation and the reads' own cost, with no substitution
walking a value or a branch built for nothing. What the read builds that the result does not
contain is now what evaluation itself builds (β-reduced bodies, the composition clauses' terms);
making that pending too would mean building every operand unbuilt (call by need throughout), a
change to compilation, not to the substitution rules.

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
