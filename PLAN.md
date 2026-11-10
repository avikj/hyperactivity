# The system: what it is, what is built, and everything that will be built

Revision 9, 2026-11-09. Repo avikj/math, `scoped-machine` on `main` (revision 9 written at
`e809a6f8`). This is the single plan and status document; each revision replaces the last
whole. Revision 8 and everything before it are in git history; nothing in them is in force
unless restated here.

**Revision 9 in one paragraph.** The compiler exists and its output is checked by the checker:
`#jit f` quotes a checked definition, specializes it, encodes the specialized normal form as
x86-64 bytes by fixed templates, and binds `f` to the bytes only after the checker has accepted
the specialized code as `f` (by conversion, or, when theorems were used, by checking the path
along them as the definition `f·jit`). Twenty-eight workloads compile and run with the
machine's answers. Everything that related a second description of the system to the first by
comparing outputs is deleted: the native-C path, declared representations, layouts, handles,
the old printers, the tests. The plan from here is one compiler, made trustworthy below
(Phase 2: the encoder's templates become equations under an imported x86 model) and pointed
upward (Phases 4–8: C++ and every other language lifted into terms, compiled by the same
compiler, related across languages by a quotient, and the OS, fleets and deployments stated as
types over those terms).

---

## 0. The premise and the one rule

Computation is substitution along identities. Here the identities are paths of cubical type
theory (CCHM: De Morgan interval, generalized composition, Glue, univalence computing), the
runtime is an interaction net that normalizes terms with open variables and open dimensions,
and the checker is itself a term on that net. Compiling is transport: the specialized program
is a term equal to the original, and its machine code is a spelling of that term whose meaning
is defined construct by construct.

**The rule every piece obeys: the checker can reject this piece's output when it is wrong.**
Consequences, all binding:

- No second description. Nothing is built as a copy or model of something else and connected
  to it by tests or by comparing answers. If two things are meant to be the same, there is a
  path, and the checker checks it.
- No tests as evidence. The library is definitions; the evidence for a definition is that it
  checks. Measurements (steps, bytes, seconds) are kept as numbers, not as assertions.
- No proof libraries. The cost of proving is essentially zero beyond writing the correct code:
  a path comes from construction (the compiled code *is* the normal form), from rewriting
  along theorems the code's author already needed, or from the checker's conversion. A pass
  that needs a large separate proof effort is designed wrong and is redesigned, not proved.
- Nothing optional. Every phase below is scope. Nothing implementable is deferred, and a
  limitation found is a gap to close, recorded where the work is, not a caveat.
- Trust is stated exactly (section 7) and shrinks monotonically: each item leaves the trusted
  base when the layer under it is lifted.

## 1. The endgame: the tower

Every layer that runs is a term; every property is a type; every relation between layers is a
path. Bottom to top:

1. **Hardware.** `⟦x86⟧` (then Armv9, RISC-V): the meaning of machine code as a function of
   initial state and the environment's responses, imported from existing formal models (ACL2
   `x86isa` through Sail for x86; Arm's ASL through Sail; Sail RISC-V), with the memory model
   (x86-TSO; the Armv8 axiomatic model) as a predicate over executions. Below this is physics.
2. **Binaries.** With `⟦x86⟧` and a loader (ELF, relocation, initial state), every binary has a
   meaning. This is the ground truth for everything above.
3. **Our compiler.** For a term `f`, bytes whose meaning under `⟦x86⟧` is `f`'s. Today the bytes
   are checked against `f` up to the encoder's templates (section 3); after Phase 2, up to the
   ISA model alone.
4. **The OS.** Each syscall a command of a world with its response type and the contract the
   kernel honours; kernel-bypass NIC interfaces the same. Assumed until the kernel (C) is
   lifted; then a path.
5. **Languages.** For each language, its abstract machine written as a term with its
   implementation-defined choices as parameters, programs read into terms, and compilation by
   the one compiler of layer 3 (section 5 states the distinctions this rests on).
6. **The quotient.** Behaviours are compared by observation: two programs in any languages are
   equal when their projections to an observation signature are equal, a path in the quotient.
7. **Systems.** A node is a process over its world; the network is a world; a fleet is the
   composition of nodes under it; safety, liveness, refinement and convergence are types over
   that term.
8. **Deployment.** `deploy : Corpus → Config → Fleet` is a term; a box's binary is the corpus
   specialized to its configuration and role and compiled by layer 3; deploying is checking it.

The trading fleet is the first full instance because every layer appears in it and its standard
(hand-written C++ latency, exact behaviour) is unforgiving.

## 2. The core (unchanged across revisions)

1. **The rewrite rules** of the interaction net (`src/net.c`): a step rewrites two cells at their
   principal ports and consults nothing else. Parallel workers cannot change a result.
2. **The composition clauses as terms** (`lib/prelude.cub`): how transport and composition
   compute at each type former; univalence computes because transport is a term the net runs.
3. **The checker as a term** (`lib/check.cub`): it reads the machine's values through `%view`,
   quotes them to codes at their types, and `eval-g` (`lib/code.cub`) maps a code back to the
   term it names, clause by clause. Reflection, not a second evaluator.
4. **Keys**: a producer is named by its body over the keys of what it binds; one term is one
   pointer; equal work reached twice while retained is done once.
5. **Retention by reachability**: a shared value lives exactly while referenced; no collector,
   no policy.

Programs about programs are ordinary definitions over codes, and their outputs carry paths, so
a wrong program about programs produces output that does not check. The specializer, the
matcher, the rule reader, derived equality, the certificate checkers, and the compiler are all
this kind of program.

## 3. State at revision 9

**Built and standing.** The net running CCHM (paths, Glue, univalence, higher inductive types
with truncations, `%hcompU`, generalized composition, canonicity on path-free data,
coinduction by copatterns, guarded recursion with clocks); the checker as a term with levels,
η, positivity, boundaries, coverage and termination; the surface language and elaborator;
sharing and memo by content keys; multiplicities computed after checking; the primitive types
(`Int64`, `UInt32`, `UInt64`, `Float64`, `Float32`, `Bytes n` with proved accesses); the store
by content keys; the mathematics library (`std`, `fiber`, `sort`), the certificate checkers
(`lin`, `linproof`, `eqproof`, `ring`, `intproof`, `intlin`), derived equality, autodiff's
theorems; processes, worlds, handlers, supervision, replay, session types, choreographies,
regions; and the compiler (section 4).

**The elaborator binds each name once.** A second binding of a definition, data type or
constructor by the source is refused, as is a clause no case reaches. (System-generated forms,
`#jit`'s rebinding of `f` to its bytes, and `#retract` are the exceptions.) Turning this on
found and removed nine silent rebindings in the library and one in the Brunerie port.

**Loading.** `make library` checks every file in order: 830 checks, 0 failures, 21.7M steps,
about 15 s. `make brunerie` reads Brunerie's number, −2, with 0 and with 4 workers.

**Deleted at revision 9** (in git history; none of it returns): `#native` (C emitted and
compiled by gcc, loaded as `.so`, specialized at run time, records by value), boxed values
and `V_NATIVE` handles and the `Native` type, `represent`/`repops`/`replaws` and their tables,
`#layout` and the checker's layouts, `#fo`, libcubical's boxed and struct calls, and the
example fixtures that sat in library files. Earlier (revision 8): the first-order back end, its
printers and evaluator, `repr`, `stage`, the test suites and harnesses.

**Measured** (kept as numbers): Brunerie's `brunerie'` −2 in 233,628 steps of the reading; against Agda 2.8.0 on the same
machine, 116 s CPU / 870 MB from nothing against this machine's 5.4 s / 139 MB end to end
(`experiments/agda/measure.sh`). Open: g8 and the corner at 100–160 net steps per result cell
against the generic 6-cube's ~24 (the remainder is evaluation, not substitution); the 6-cube's
normal form beyond 2.06M cells exceeds a 2.5 GB cap; `surfc` with every dimension open does
not finish. Normalizing a term with open dimensions should do work proportional to its result;
this stays a goal of the net (it is the cost of every program about programs).

## 4. The compiler (Phase 1): what is built

### 4.1 The operation

`#jit f [using t₁ … tₙ]` on a checked definition `f : T`:

1. **Quote and specialize** (`lib/specialize.cub`). The checker's code for `f` is normalized
   with the held definitions abstracted (a schedule value `Sched` says what is held, the rewrite
   bound, the binding-time stages and the register assignment). If theorems are given, the
   oriented ones are applied to a fixpoint and the unoriented ones a bounded number of times
   (`lib/match.cub`, `lib/rules.cub`): each rewrite step is the path `λ i → c[(t σ) @ i]` of the
   theorem's instance in context, and steps compose by `concat`'s code, each half annotated
   with its type so the composite infers. The result is `(c' , π)`.
2. **Decide the fragment** (`lib/machine.cub` `fragment`, `lib/x86.cub` `x-check`). A construct
   with no machine form names itself and its reason, and the definition stays on the net, which
   is always correct: a free dimension or path (`dim`), composition (`kan`), a higher
   inductive constructor (`higher`), a type at run time (`dep-type`), a literal ≥ 2⁶⁴
   (`mult`), an application whose head is not a known definition (`reflect`).
3. **Check.** Without theorems, `c'` is `f`'s normal form, and the checker's conversion
   `conv T (%gval f) (eval-g c')` must hold. With theorems, `c'` is only propositionally `f`,
   and the path `π` is checked as the definition `f·jit : f ≡ eval-g c'`. Nothing is bound
   unless this check passes.
4. **Encode** (`lib/x86.cub`, the encoder). Two passes on the code, then fixed templates:
   `x-push` (β, thunks dropped: a checked term terminates and compiled code writes nothing
   outside its arena, so computing a delayed value early changes no answer) and `x-lift` (each
   structural recursion named as a function, outermost first; induction hypotheses become a
   let when the result is first-order and calls at full application when it is a function).
   Then one fixed x86-64 template per construct: word and primitive arithmetic, comparisons,
   the counted loop for the word eliminator, the case as a tag dispatch, calls, constructors
   and pairs as arena cells, loads and stores on byte strings, SSE2 scalar floating point,
   checked `Word` arithmetic with an overflow exit.
5. **Bind** (`src/lang.c` `jit_bind`). The bytes are mapped executable and `f` is rebound to
   `(%native k g a…)`: a call whose arguments all cross (below) runs the bytes; any other call,
   and every overflow exit, runs `g`, the definition itself, on the net.

### 4.2 Values in compiled code, the boundary, and why the overflow exit is sound

Every value is 64 bits: a word or primitive by its bits; a fieldless constructor by its index;
a constructor with fields a cell `[index][field]…` in a thread-local arena (1 GB reserved,
discarded after the call); a pair `[0][a][b]`; a byte string `[length][bytes]`; a `Nat` as the
count of its successors (successor past 2⁶⁴ takes the overflow exit). How an argument or
result crosses is read off `f`'s checked type (`JT`, `src/net.c` `jit_in`/`jit_read`): a word,
a primitive, a value converted by its type (read whole first), or a proof parameter the code
never reads (erased, not passed).

Compiled code is pure: stores copy, the arena is private and discarded, and no command of the
world is compiled. So an overflow can restore the stack and return, and the net computes the
same call from the start (`run p f ≡ f` by cases). Processes and their commands run on the net;
compiling them is not part of Phase 1 and is withdrawn as a design (the machine-command
construct `mcmd` of revision 8 is gone): a process's hot inner computations are pure functions
and compile; its commands are driver calls the net makes.

### 4.3 Acceptance workloads (`experiments/jit/workloads.cub`)

Each line: bytes, how the check was made, net steps for specialize + check + encode.

| f | bytes | check | steps | f | bytes | check | steps |
|---|---|---|---|---|---|---|---|
| addw | 123 | conv | 5,361 | sdiv | 134 | conv | 6,926 |
| powl | 214 | conv | 11,056 | smod | 134 | conv | 6,957 |
| plusN | 250 | conv | 15,744 | udiv | 116 | conv | 6,062 |
| isZ | 190 | conv | 11,838 | umod | 117 | conv | 6,097 |
| swapPt | 256 | conv | 14,214 | sar | 98 | conv | 5,672 |
| sumL | 287 | conv | 17,221 | u32w | 97 | conv | 5,252 |
| upto | 244 | conv | 14,103 | u32l | 103 | conv | 5,373 |
| mix | 165 | conv | 9,744 | i64l | 104 | conv | 5,306 |
| umix | 147 | conv | 9,591 | feq | 125 | conv | 5,663 |
| fmix | 176 | conv | 8,950 | flt | 125 | conv | 5,675 |
| pairUp | 176 | conv | 6,004 | w2f | 125 | conv | 4,928 |
| decode | 202 | conv | 10,932 | f2i | 87 | conv | 3,966 |
| st8 | 414 | conv | 22,489 | eqN | 437 | conv | 38,888 |
| pz | 77 | path (plus-zero) | 7,115 | pzz | 77 | path (2 steps) | 10,870 |

Every answer read through the bytes equals the machine's, including the edges: `powl 63` is
2⁶³ natively and `powl 64`, `powl 70` are exact through the overflow exit; signed division of
INT64_MIN by −1 and by 0; unsigned division by 0; arithmetic shift of negatives; 32-bit wrap;
NaN comparisons; `Word`→`Float64` at and above 2⁶³; a store leaving its argument unchanged.

### 4.4 What Phase 1 still builds

Each item is done when the workloads gain a definition exercising it, it compiles, the check
passes, and its answers equal the machine's.

1. **Functions as values.** A λ in a body and a call through a variable: a closure is an arena
   cell `[code address][captured…]`, a call through it an indirect call. Removes `reflect` for
   higher-order code.
2. **Codata and records.** Projections (`cproj`) and copattern definitions observed to a known
   depth compile; an unobserved stuck head stays on the net.
3. **Recursion beyond structural.** Definitions accepted by `decreasing by` (a measure) compile
   as ordinary recursive calls; mutual recursion as a group of functions.
4. **In-place stores under uniqueness.** When B11's computed usage shows a byte string is used
   once, `store` writes in place instead of copying. The path is unchanged (the meaning of a
   store is the same); only the template differs, chosen by the usage the checker computed.
5. **Specialization when values arrive.** When an argument becomes known at run time, the term
   with it substituted is specialized, checked and compiled, and stored under the specialized
   term's content key, so a cache hit cannot be stale. This is the JIT proper; `#jit` at load is
   its first case.
6. **Schedules used.** The register assignment and binding-time stages in `Sched` drive the
   encoder (today every value lives in the frame); register allocation is a choice of
   template, so it changes cost, never meaning.
7. **Compile cost.** Steps per compiled byte is measured per workload and driven down as a net
   property (it is the same cost as every program about programs).

## 5. The distinctions everything above x86 rests on

1. **The abstract machine is written; implementations are lifted.** A language's standard
   defines an abstract machine with implementation-defined choices (type sizes, alignment,
   evaluation order where unspecified, the memory model's allowed executions). That machine is
   written here once as a term, `⟦_⟧_std`, with those choices as *parameters*. A particular
   implementation with particular flags (Clang 19, `-O2`, x86-64 SysV) is a different object:
   a binary, lifted under `⟦x86⟧`. Neither is mistaken for the other.
2. **Correctness under an assumed compiler is not correctness of the binary.** A statement
   about a C++ program at `⟦_⟧_std` holds for every implementation that conforms, which is an
   assumption about the external compiler. A statement about the binary needs the binary's
   meaning. This system produces binaries by its own compiler, so the second kind of statement
   is the one it makes: the binary's meaning is the term's, checked (up to the templates in
   Phase 1, up to the ISA model after Phase 2).
3. **No translation validation of an external compiler.** Relating each external compiler
   output to its source program ("relation 4" in the earlier discussion) is not built: it
   would be a second compiler's output checked against ours, a model beside the thing that
   runs. External binaries that must be understood (a vendor library, a kernel, an
   interpreter) are lifted as binaries under `⟦x86⟧` and are then terms like any other.
4. **One compiler for terms (Futamura).** A C++ program read into a term is a term; compiling it
   is the compiler of section 4, nothing else. Specializing the abstract-machine term
   `⟦_⟧_std` to a fixed program is the first Futamura projection: the residual is the program's
   meaning with the interpretive overhead normalized away, and it compiles like any other
   definition. The requirement this places on `⟦_⟧_std` is a binding-time discipline: its
   dispatch on syntax must be static (a case on the program's constructors, which is known),
   and only the program's run-time data dynamic, so specialization removes the interpreter
   entirely. Interpreters lifted from binaries (CPython) meet the same projection.
5. **The target is x86-64**, then Arm and RISC-V by their imported models. Not LLVM IR, not C:
   an intermediate language would need its own meaning and its own check, and the hardware
   model is the meaning that matters.

## 6. Phases 2–8: everything that will be built

### Phase 2 — The ISA model, and the encoder leaves the trusted base

Built:
1. **The Sail importer.** Sail's first-order imperative language (registers, memory, bitvectors
   of fixed widths, structured control) read into data types and definitions here. One
   importer serves x86 (via the ACL2 `x86isa` translation), Armv9 (Arm's ASL via Sail) and
   RISC-V. The importer preserves control flow and outcomes: every Sail construct becomes a
   construct with the same meaning (a `match` a case, a loop a recursion on its bound, an
   exception an outcome), so the imported model is the model, not a transcription of it.
2. **The inventory.** Which instructions the model implements, which it only decodes; a program
   using an unimplemented instruction does not lift until it is added. The inventory is data
   beside the import.
3. **The loader.** ELF64 segments, relocations used by static and PIE binaries, the initial
   stack and registers under SysV, as a definition producing the model's initial state.
4. **The memory model.** x86-TSO as an axiomatic predicate over executions, integrated with the
   instruction model's memory events (Isla is the precedent).
5. **The template equations.** For each encoder template, the equation between the construct's
   meaning and `⟦x86⟧` of the template's bytes: straight-line templates by computation on
   literals under the model, the counted loop by induction on the counter, the overflow exit
   by cases on the carry. Then `x-push` and `x-lift` are stated as code-to-code functions whose
   output is checked equal to their input by the checker (they rewrite by β and by naming), so
   the passes are checked per use like the specializer. After this, compiling `f` yields a
   path from `⟦x86⟧ (bytes)` to `f`, and the encoder is out of the trusted base.
6. **Measurement.** Net steps per simulated instruction on the imported model, which decides how
   large a binary can be lifted (Clang's binary is the large case of Phase 4).

Acceptance: every workload's bytes, run under the imported model from the loader's state,
produce the machine's answers *as a checked equation*, and the trusted base lists the ISA
model and no templates.

### Phase 3 — The OS as a world

Built:
1. **The syscall world.** Each Linux syscall a hot path uses (read, write, mmap, munmap,
   mprotect, clock_gettime, futex, epoll, socket family, sendmsg/recvmsg, io_uring setup and
   submission) as a command with its response type and its contract as a type; errors as
   outcomes. The drivers (`src/drivers.c`) are the machine side of this world and are listed
   as trusted until the kernel is lifted.
2. **Kernel-bypass interfaces** (ef_vi, DPDK) as worlds of the same shape.
3. **Platform parameters in program types.** A program's type names the platform facts it
   relies on (page size, cache line, core count, clock source) as parameters, so a binary is a
   specialization at stated values and moving it is re-specializing, not hoping.

Acceptance: a process using the world (receive packet, decode, update state, send) runs on the
net against the world's model and, compiled, against the real kernel, with the same outcome
trace on recorded inputs (replay).

### Phase 4 — C++: the abstract machine, reading, and compilation

Built:
1. **`⟦_⟧_std` for C++**, written as a term with parameters: object model and lifetime;
   storage durations; layout as a function of the implementation-defined sizes and alignments
   (parameters, instantiated to the Itanium ABI for x86-64); integer conversions and the
   undefined cases as refusals in the outcome type; evaluation order where unspecified as a
   choice parameter; exceptions as explicit control flow with destructors at each unwinding
   step; coroutines as a frame and a state machine; threads as processes over the memory world
   with the C++ memory model's orderings; allocation through `operator new` as a world command
   with its contract. Templates are λ over types and constants; instantiation is β;
   `constexpr` evaluation is normalization; concepts are predicates on codes. Written to the
   binding-time discipline of section 5.4.
2. **The AST.** Clang's elaborated AST (names resolved, overloads chosen, templates
   instantiated, implicit conversions explicit, `constexpr` values present) as a data type
   here. Until Clang's binary is lifted, Clang's Sema is an oracle for this AST, trusted
   exactly as "the AST dumped is the AST Sema produced", and listed.
3. **`read`.** The AST mapped into terms that keep C++'s abstractions (a class a record at its
   layout, a virtual call a load from the vtable record and an indirect call, a member access a
   load at an offset in the byte layer). `read p` *is* the meaning assigned to `p` at
   `⟦_⟧_std`; there is nothing to check it against at this layer, and a mistake in `read` is a
   mistake in the definition of C++'s meaning here, fixed in one place.
4. **Compilation** by the section 4 compiler, via the Futamura specialization of `⟦_⟧_std` at
   the program; the result runs under the platform parameters of Phase 3.
5. **Lifting Clang itself** from its binary under `⟦x86⟧` once Phase 2's measurement says the
   net can run a binary of that size; then the Sema oracle is removed (the AST is Clang's
   lifted computation on the source).

Acceptance: a trading codec, order book and session layer written in C++ are read, compiled by
our compiler, and run with the latency of the same code compiled by Clang (within a stated
factor, measured), every assumption listed by `#trusted`.

### Phase 5 — Every major language

Built, in this order because it reuses the most:
1. **C** (a restriction of the C++ machine).
2. **Interpreted languages through their C implementations**: CPython, CRuby, Lua, PHP, R,
   V8 and HotSpot's interpreters lifted as programs; a program in one of them is the
   interpreter term applied to it, and specializing the interpreter at the program is the
   Futamura projection that compiles it. Their structured readings (Python's `int` as ℤ, a dict
   as a finite map) are written where an abstraction is wanted, as a definition of meaning,
   like `read`.
3. **Languages with their own cores**: Rust (MIR as the AST, the borrow discipline read as
   multiplicities), Go, Haskell (Core), OCaml (Lambda), Swift, and the configuration languages
   used for infrastructure.
4. **Lean and Coq as programs**, so "accepted by Lean" has a meaning here; their theorems
   transfer only per fragment where the theories agree (UIP and impredicative `Prop` have no
   counterpart under univalence).

Each language is a `⟦_⟧_std` with parameters (where a standard exists) or the lifted reference
implementation (where it is the semantics), a reader, and the one compiler. Each is easier than
the last because the world, the memory model, the byte layer and the compiler are shared.

Acceptance per language: its conformance suite's programs read and run with the reference
implementation's observable behaviour, as checked traces.

### Phase 6 — The quotient: equal behaviour across languages

Built:
1. **Observation signatures.** `Obs` (the commands a program's environment can see: I/O,
   messages, time as observed) and `obs : Proc W A → Proc Obs A` forgetting the rest.
2. **Equality by observation.** `p ~ q` is `obs ⟦p⟧ ≡ obs ⟦q⟧`, a path in the quotient of
   processes by observation; a program in C++ and one in Rust equal in this sense are
   interchangeable for every property stated over `Obs`.
3. **Witnesses.** Equivalences are proved by simulations written or computed for the case at
   hand; where a normal form for the relation exists (an encoding of the quotient into
   path-free data), equality is decided by computing it. Program equivalence in general is not
   decided, and nothing pretends it is.

Acceptance: a C++ and a Rust implementation of the same order book shown equal under the
exchange-facing observation, and the trading strategy's properties transported from one to the
other.

### Phase 7 — Systems: nodes, networks, fleets

Built:
1. **The network world**: delivery, loss, reordering, duplication, delay, partition, with the
   scheduling model as responses.
2. **Fleets**: a fleet is the parallel composition of node processes under the network world,
   itself a process over the residual world (clients, exchanges, clocks, the adversary).
3. **Property types**: safety as an invariant over every response sequence; liveness under a
   fairness predicate on responses; refinement to a sequential specification as a path in the
   quotient; replica convergence by defining replica state as a function of the *set* of
   updates received, with eventual delivery as the hypothesis; Byzantine behaviour as a class
   of worlds; protocol conformance by projection from a choreography under its realizability
   conditions (`lib/choreo.cub` is the start).

Fleet acceptance: the trading fleet (market data handlers, strategies, order gateways, risk,
drop copy, recovery) as a term; stated and checked: no order is sent that risk has not
admitted (safety); every acknowledged order reaches a terminal state under fair delivery
(liveness); the gateway refines the exchange's sequential order protocol (refinement); the
position kept by risk equals the sum of fills from the drop copy (convergence); replay of a
day's recorded responses reproduces the day's orders exactly.

### Phase 8 — Deployment as a checked term

Built:
1. **`deploy : Corpus → Config → Fleet`**: per box, the corpus specialized at its configuration
   and role and compiled by the one compiler; placement and wiring are part of the fleet value.
2. **Checking a deployment** is checking that term: the fleet's properties of Phase 7 hold at
   this configuration, and every binary carries its path.
3. **Change**: a rolling upgrade is two versions coexisting, compatible on the observed
   interface by an equivalence (or a retraction where they differ); a state migration is a
   relation between state types; hot loading rebinds a key and accounts for frames, closures
   and messages in flight. Drift is a finite comparison of configuration values;
   behavioural equality of running fleets is not claimed.
4. **Distribution by keys**: machines exchange content keys and fetch what they lack; a key
   names a closed definition and nothing more (not attestation, not authority).

Acceptance: the trading fleet deployed from one corpus and one configuration value, every
binary produced by `deploy`, the deployment's checks passing, and an upgrade carried out as a
checked transition.

## 7. The trusted base, exactly (revision 9)

What a reader of `ok: jit f` must believe, and when each item leaves:

| trusted | where | leaves at |
|---|---|---|
| the rewrite rules | `src/net.c` | never (they are the theory's computation) |
| the composition clauses | `lib/prelude.cub` | never (they are the theory) |
| the checker | `lib/check.cub` | never; it is small and itself checked against the theory's rules by use |
| the primitive operations' semantics (integer, IEEE binary64/32, byte loads and stores) and their trusted laws | `src/net.c`, `lib/prim.cub` (73 names, listed by `#trusted`) | Phase 2: each primitive becomes the ISA model's instruction meaning |
| the encoder: `x-push`, `x-lift`, the templates, the frame and calling convention | `lib/x86.cub` | Phase 2 (template equations; passes checked per use) |
| crossing the boundary: `jit_in`, `jit_read`, `jit_bind` | `src/net.c`, `src/lang.c` | Phase 2: marshalling is read as code under the model |
| guarded recursion's `dfix`, `pfix`, `forceκ` | `lib/std.cub` | never (rules of the theory, stated as primitives) |
| the drivers and `foreign` symbols | `src/drivers.c`, each `foreign` line | Phase 3 contracts; Phase 4/5 when the C is lifted |
| Clang's Sema as AST oracle | Phase 4 | Phase 4 step 5 |

Nothing else. A definition whose check used none of these beyond the first three reports
nothing under `#safe f`.

## 8. Working rules for anyone building this

- Read this document and `lib/code.cub`, `lib/check.cub` (the view and quote), `lib/specialize.cub`,
  `lib/machine.cub`, `lib/x86.cub` before changing the compiler.
- Before acting on a direction change, write the full picture (what changes, what is deleted,
  what each item's acceptance is) and get it confirmed.
- The evidence for a change is the checker: `make library` (0 failures), the workloads (every
  `#jit` ok, answers unchanged), `make brunerie`. Do not add tests, harnesses or models.
- Treat each error as a class: find every instance of the same mistake before fixing one.
- Commit granularly with what each commit changes and why; push.
- Report findings (what happened, measured), not plans; own every delay and every wrong claim.
