# Hyperactive

The design of Hyperactive: what it is, what it is for, and what its constructions force beyond the
language. The paper (`docs/Hyperactive_Final_Paper.pdf`) lays the language's foundations and
deliberately bounds the ambition it states: it says what the calculus computes and declines to say
what follows for perception, intelligence, agency, economics or the shape of a planet's computing.
This document is the other register. It covers what the paper excludes, and it anchors each step to
the paper section it rests on.

Held by the Claude session working with Avik, who owns this document. It is revised in place, unlike
`docs/`, whose streams of Avik's words are append-only and verbatim. Every decision here is Avik's,
and his words are cited where they are the source:

- `docs/what-hyperactive-is-for-in-avik-words.md`
- `docs/state-change-collaboration-design.md`
- `docs/task-graph-design.md`

## How to read a sentence here

This document is held to the discipline it describes (§7): nothing is asserted without saying what
stands behind it. Each claim is one of three kinds.

- **Constructed.** A term or theorem in the paper, cited by section and equation. These are as
  certain as the paper.
- **Decided.** A design decision of Avik's, cited to his words. These are not open to relitigation.
- **Thesis.** A claim that the constructions reach further than the paper states. Each thesis names
  what would witness it: the term that, once in the repository and checked, turns the thesis into a
  construction. A thesis is not a hedge. It is a statement whose witness is still to be written.

The document has sections, but Hyperactive has no parts. Avik: "there is no ontology in this
product… everything discussed w.r.t product needs to interoperate perfectly to the point of not
being collaborating organisms but a single organism." Each section below is the same construction
seen through a different observation. In the vocabulary of §3, the sections are fibers, not
components.

---

## 1. The product

> the product is every screened device in the world simply runs hyperactive kernel. the term
> "application" becomes fully concrete - does not refer to a silo, but literally to a particular
> application of computing.

**Decided.**
- *The aim.* The aim is not a programming language. It is "deploying the proper form of internet,
  global computation, dissolving everything distasteful of every aspect/layer from UX to networking
  to application boundaries to data ownership to malleability of software, economics of
  development."
- *Where the language came from.* The language is what fell out of solving that problem first:
  "i solved the PL problem by solving the product problem first and solving data/code/interaction
  all the way down."
- *What it is for.* The motive under it is pedagogy and agency: "any child can master AP calculus by
  age 10 and that is a very straightforward world to construct in my lifetime".
- *Everything serves the product.* "*all* of this is to serve *product*."

**The order of work.** The task graph computes it (`task-graph/hyperactivity.cub`, rendered at
`hyperactive server`).
- The language implemented completely and correctly as the paper specifies.
- The live interactive collaboration system over the language and its terms.
- Only after both: the lifting of particular languages (x86, C, C++, Rust, OCaml, Python and others)
  as terms. "x86 is just a programming language expressing behavior of an abstract machine with its
  semantics, as C++ is a programming language."
- The paper's own worked examples serve nothing and come last.

## 2. Constructions, and filling a boundary

Everything below specializes two things: constructions, and the filling of a boundary.

**Constructions.** A thing, a state, a self, an object, a value or an observation is a
fiber of a map applied to a construction. It carries no information the construction lacks (§3).

**Filling a boundary.** Given the boundary of a cell, it produces the coherent
interior. Constructed: this is cubical composition,

```
comp i A [φ ↦ u] a0 : A[i := 1]     (paper §3.4, eq. 3.14)
```

which takes a type varying along a dimension, a partial element on a face and a compatible starting
endpoint, and computes the missing face so that the whole box is coherent. The paper calls this the
principal computation of the cubical host and requires it for every type former: Π, Σ, Path,
universes and Glue (§3.4). Avik began on this operation at fourteen, as discrete Stokes: the integral
over a region is determined by data on its boundary, and the circulation around a boundary is the
interior curvature accumulated.

**Where filling fails, and why that is the information.** The account needs this precision
throughout. In the cubical calculus every *open* box fills: composition always succeeds, which is
the Kan condition. What need not fill is a *closed* boundary, such as a loop with nothing missing
that may or may not bound a disc.

Constructed: a square with sides p, q, r, s is equivalent to a path between the composites of its
boundary, Square(p, q, r, s) ≃ (p · q = r · s). So the interior of a cell is exactly a witness that
the circulation around its boundary vanishes. That is the exact cubical form of Stokes, with the
interior and the boundary circulation determining each other.

It follows that **the operation is filling, and the information is the obstruction to filling.** A
loop that bounds a disc carries nothing: transport around it returns the identity. A loop that does
not bound one carries exactly what distinguishes it from a disc: its holonomy, a nontrivial element
of the loop space. §4 builds perception on this. Composition propagates local fillers to global
coherence, and what cannot be filled is what is there.

## 3. Identity is the construction; a thing is a fiber of it

**Constructed.**
- *An observation projects; the construction keeps everything.* "A representation of a transition
  retains its source, target, and evidence, while an observation may project away any of those
  components" (§3.3, eq. 3.11). And: "identify the complete construction before comparing
  representations. The properties of a chosen projection are properties of that projection" (§3.3).
- *Carrying identity costs nothing.* Carrying a value together with its identity witness adds no
  information: Sing(a) := Σ (a = x) is contractible (§3, eq. 3.5).
- *Reversible without invertible.* Keeping a function's input alongside its output makes the
  representation reversible without making the function invertible: A ≃ Graph(f) (eqs. 3.6–3.7).
- *Knowledge is a fiber.* For an observation o : C → O, the things compatible with what it
  registers form the fiber Fiber_o(y) := Σ_{c:C} (o(c) = y) (§10.4).

**Thesis: there is no binding problem and no separate problem of personal identity.**
- "The object", "the state at time t" and "the self" are fibers: a path in the total space collapsed
  onto a base the observation can read.
- Avik's first instance was a coffee table and a stool. Across two moments there is one construction
  with a path in it, and "the persisting table" is the fiber of the observation that forgets the
  path and keeps the endpoint.
- The Madhyamaka analysis of own-being is this, and §2–3 of the paper give it a type.
- Binding and personal identity are each the question of which observation map is applied to one
  construction and what its fiber retains. Each is answered by naming the map.
- *Witnessed by:* a term presenting a percept's construction and two observations of it, one that
  keeps the path and one that forgets it, with the fibers computed. The identification is then a
  calculation rather than an interpretation.

### 3.1 Identity is semantic; its representations are mechanisms

This is where the account meets the implementation, and it is the first instance of a pattern the
whole system follows.

**Decided** (Avik, 2026-11-13). Two terms are the same exactly when their content is the same.
Normal interpretation does not compute SHA-256. The language "can still respect 2 terms are
considered identical exactly when SHA256 matches; and then prove via totally different machinery".

Identity is therefore one thing with several representations, each admitted by its relation to it:

- **Inside a run: interning.** Terms built from equal parts share one node, so pointer equality is
  content identity. That is proved once, about the interning, and no hash is computed.
- **Across a boundary: a digest.** Another run, another machine or the store is a place a pointer
  cannot cross, and there identity needs a digest. Constructed: a digest identifies a representation
  "under the store's collision-resistance or injectivity assumptions; it does not prove semantic
  equivalence" (§10.8). The paper keeps digest(d₁) = digest(d₂), d₁ ≡ d₂ and SemEq(d₁, d₂) as three
  different statements.

**The pattern.** The paper admits every efficient mechanism only with its relation to the defined
meaning:
- native code replaces a definition only with the relation to its meaning (§9);
- a machine-word primitive implements its definition under a theorem or a named assumption (§8.5);
- a pointer stands for content identity under the interning proof;
- a hash stands for content identity under collision resistance, which is a named assumption.

Meaning is defined once, and any number of mechanisms realize it, each admitted by a proof that it
does. That is how the language can be fast without being trusted: speed lives in mechanisms, truth
lives in definitions, and proofs connect them.

## 4. Perception is composition; the object is holonomy

**Thesis.**
- *The field and the percept.* The sensory field is a type varying over a base of space, time and
  abstraction, and a percept is a dependent path in it.
- *Sameness is transport.* No successive points are compared. What is present across each cut is a
  mode of sameness, which is transport, transport_P(p) : P(a) → P(b) (§3, eq. 3.4). The requirement
  that adjacent modes agree where they overlap is composition with a specified boundary (eq. 3.14).
- *The differentials are boundary data.* The directly sensed differentials are the boundary data
  along the path:
  - stillness, at zeroth order;
  - constant velocity, as a flat first-order mode;
  - deceleration, as its variation;
  - the jerk of leaf-shadows, sensed without a reasoned step.
- *The object is the obstruction.* Following §2, the perceived object is where sameness, transported
  around a region, fails to return to itself: the holonomy, nontrivial exactly where there is an
  edge, a surface or an object. The local differential is the boundary datum, the object is the
  curvature integrated over the region it encloses, and Square ≃ (composite = composite) is the
  exact statement that each determines the other.
- *Why the format is field-native.* Composition carries local fillers to global coherence, up
  through space, time and abstraction, the same at every level. That is why the format is native to
  the field and the computation is local-first and recursive.

*Witnessed by:* a term over a sampled field that computes, for each cell, whether transport around
its boundary returns the identity. It is checked to report exactly the cells a defined notion of
feature contains, with no parameter fitted to data.

## 5. Univalence makes an identification usable; learning is constructing identifications

**Constructed.** Univalence turns an equivalence into an identity of types, ua : (A ≃ B) → (A = B),
and transport along it computes as the equivalence: transport_{X↦X}(ua(e), a) ≃ e(a) (§4,
eqs. 4.1–4.2). It refuses to register a distinction that no construction exhibits, and it carries
whatever holds of one presentation to every equivalent one, along with the witness.

**The precision this needs.** Univalence does not *find* the identification. It makes an
equivalence, once constructed, usable everywhere at no further cost. What finds it is constructing
the equivalence. So the compression is the construction of equivalences, and univalence is what
makes each one count everywhere at once.

**Thesis.**
- *Learning.* Learning is the accumulation of witnessed identifications: equivalences constructed
  and kept as content-addressed objects (§10.8), each one local, exact and composing, with no loss
  surface and no weights.
- *Intelligence.* Intelligence is the minimization of the construction, the presentation that has
  quotiented every distinction no completion can exhibit. It is not the fitting of a reified
  surface.
- *Gradient descent.* Gradient descent is what is run without the construction. It keeps samples and
  a parameter vector, discards the connection, and recovers by minimizing one global scalar the
  manifold that is the integral of a connection the field presented directly. What it memorizes is
  the uncompressed shadow of a structure that composes exactly when it is had.
- *The end state.* Perception, language and action are reachable as construction and composition,
  with no gradient step. The end state of the subject is reasoning over equality and higher paths,
  which is this language, not a larger model.

**What this thesis does not say.** It does not say that descent cannot be expressed. Constructed:
automatic differentiation is a program transformation whose correctness refers to the derivative of
the program's meaning, and optimization is a program like any other (§10.6). Descent is a term in
Hyperactive. The thesis is about when descent is necessary: only when the construction is missing.

*Witnessed by:*
- the §4 term;
- then, for a fixed domain, a constructed equivalence between two presentations that a model would
  otherwise be trained to relate, with every dependent result transported along it and checked.

## 6. Observation is a map; agency is its inaccessible complement

**Constructed** (§10.4).
- An observer has o : S → O. What it knows on seeing y is the fiber Fiber_o(y).
- An action permitted when P holds is safe for that observer when it constructs
  SafeAt(y) := Π_{(s,p) : Fiber_o(y)} P(s). That is established over everything compatible with what
  is accessible, without guessing the hidden state, and it is "stronger than exhibiting one
  compatible state in which the action is safe."
- Refining the observation shrinks the fiber, and evidence for the coarser observation is reused by
  precomposition.
- The paper "does not assign a scalar 'value of information' without an objective."
- And: "An observation is a map; knowledge is the fiber it leaves; a proof is a construction valid
  over that fiber; an action is a transition whose admissibility can be checked against it."

**Thesis.**
- *From outside.* Seen from outside, the next state is underdetermined over the fiber and attributable
  to no bearer. That is the no-fault induction: no responsibility at the base and no step that
  injects any, so none anywhere.
- *From inside.* Some information is accessible only to the interior, the complement of every
  exterior observation map, and that is where the sense of agency sits. Faggin's point about
  information private to the first person is this inaccessible complement.
- *Point and fiber.* Determinism from outside and agency from inside are the point and the fiber of one map.
  Both are exact and they are not in tension: the two-truths split, rendered as observation geometry.
- *The hard problem.* Posed as a separate mystery, it dissolves. The first person is the information
  private to a construction that no exterior observation resolves, and there is nothing further to
  locate.

This is the interpretive register at its furthest. Its witness is not one term but the absence of
any remainder: every question about the first person, posed precisely, turns out to be a question
about a named map and its fiber.

## 7. Witness is the admission discipline

**Constructed.** Three statements are kept apart (§2.1, A.2):
- a witness, Sat(f) := Σ_x (f(x) = true);
- its existence, ‖Sat(f)‖;
- a claim of existence with no witness.

A term of Sat(f) can be executed, transported and transformed. Its truncation records only that some
witness exists. Asserting the truncation without an inhabitant is admitting a term of a type that may
be empty. The checker is itself a term that admits a construction only through the checking relation
(§1.2, §6.1).

**Thesis.** Illusion, hallucination and unwitnessed safety are one failure.
- *Illusion.* A cough completing into a grey coughing figure, or a shadow into a man, is a filler
  claimed where the field supports none: inhabiting a truncation without the witness.
- *Hallucination.* A trained model emitting a fluent assertion and treating it as true because it is
  fluent makes the same move at the scale of a system: output at the level of ‖·‖, existence claims
  with no inhabitants. That is why it hallucinates and why it cannot be trusted as computation.
- *Unwitnessed safety.* The current paradigm's "safety", meaning statistics over outputs and
  reinforcement on preferences, is unwitnessed assertion about the system's own behaviour. It is the
  same failure turned on the alignment claim itself.
- *Alignment, done correctly.* It is witnessed computation: a behavioural property is admitted only
  with its inhabitant, the construction that exhibits it. A producer of existence claims it cannot
  witness is not neglecting the discipline by accident. Without the construction, statistics is all
  it can run.

This document holds itself to the same discipline. That is why its claims are sorted into
constructed, decided and thesis, and why each thesis names its witness. A design document that
asserted truncations without inhabitants would commit the failure it names.

## 8. The running system

What follows is Hyperactive running: the kernel, its objects, the way they change, and the way
machines share them. It is the language seen as a running system rather than a second thing built on
it.

### 8.1 The kernel and its guests are one program

**Constructed.** A kernel is an open program, and with its user processes it composes into one
program, Host(K; P₁, …, Pₙ), by repeated typed process composition. "The kernel's boundaries are real
mathematical interfaces; they are not boundaries of the programming language" (§7.5). Composing
programs with open interactions internalizes one collection of interactions through a handler while
leaving another open, and handler composition is a computed property, not a special semantics (§7.6).
A fleet is a further composition (§7.6, §10.10).

**Decided.**
- *Applications.* An application is "a particular application of computing", not a silo. It is a
  term reached from a root.
- *Storage.* It is the object set, not a file system beneath it: "no file tree, only a collection of
  objects relations etc. … filesystem data is trivially expressed within it" (state-change §1).
- *The network.* Processes exchanging immutable objects.

### 8.2 State is not text, and nothing is mutated

**Decided** (state-change §1, §5).
- "our state is not text."
- Code is immutable and content-addressed: "There is no sense of a file mutated over time."
- "an entire program, hell an entire distributed system, any arbitrary construction, is referenced
  by single hash."
- Unison is the precedent, as the paper also says (§10.8); git and patch theory are rejected.

**Constructed.** "The new code is a new immutable definition. Rebinding a name publishes it in a new
snapshot" (§10.5). "A name is a view or alias; a stable reference identifies a particular
definition" (§10.8).

### 8.3 A change is new code, and change is a square

**Decided** (state-change §5, §7).
- "if I 'change' some function f called within g. I have changed g."
- "f, f', g, g' are related by a square. this models every case where a construction is invoked
  inside another construction, and we ask of the outer construction corresponding to a change in the
  inner construction. all that nesting/mutation falls to this."

**How the square is computed.** Write a dependent as G applied to what it uses, so g = G(f). Then:
- *The new dependent.* g′ = G(f′) is computed by applying the same abstraction to the new object,
  which is specialization over the language's own definitions (§6.2). No one rewrites call sites.
- *Changes that keep behaviour.* When the change is a path or an equivalence, applying G to it gives
  the square for free (ap, functoriality), and every dependent property transports along it (§4).
  Nothing is re-checked.
- *Changes that alter behaviour.* When the change is a refinement or another stated relation, G
  carries that relation only where G is proved to preserve it. "Where the relation is only
  refinement, the proof must use refinement; univalence does not make a one-way simulation
  invertible" (§8). The old proof "does not silently become a proof about" the new object; "Its
  dependent type changes and must be inhabited" (§10.11).

So the language is the change-propagation mechanism. There is no build system and no "rebuild what
depends on this" step, only the square, computed.

**Two changes to one object.** Suppose one change makes f → f′ and another makes f → f″.
- With no canonical state, nothing forces them to be merged.
- A merge is a third change f‴, related to both, made by whoever wants both.
- Conflict is not a state of the system. It is two changes nobody has related yet.

### 8.4 No canonical state; views; what is kept

**Decided** (state-change §4, §6).
- "there is no canonical state. … it's a lie and it slows things down and is the source of
  confusion. there is no canonical set of patches; no mechanism of promoting an incoming patch to
  canonical. any incoming change is compatible with all existing code: it is new code."
- And on the old objects: "Responsibility to mark dead … 'here's another system doing X correctly,
  and which is incorrect in the prior system; the prior system is undesirable and I throw it in the
  trash in favor of this one.'"

**What follows.**
- *A view.* A view is a choice of roots and of which retirement statements to honour.
- *Names.* A name is an alias inside a view. The same name can denote different objects in different
  views, and that is not ambiguity, because references are by identity.
- *Liveness.* An object stays alive by being reachable from some view's roots, so liveness is
  purpose.
- *Marking dead.* Marking something dead is a statement, itself an object, that the honouring views
  follow.
- *No global deletion.* There is no global garbage collector and no global archive: each store keeps
  what its roots reach. Something nobody roots is gone, which is the correct meaning, and archiving
  is someone choosing to root it.

### 8.5 A working session: read, write, act

**Decided** (state-change §2).
- "to run something: it's executable+input … materialize it locally if I need to act on it
  locally … cause that's all you can do; read, write, act, already entirely through command
  interface this is fucking clean."
- Every write, "including thinking raising questions, tracking questions & resolutions", happens "at
  high frequency hyperactively".

### 8.6 Visibility

**Decided** (state-change §3). "there's a sense that there is something internally in-progress,
which should be externally invisible."

**What follows from §3.1.** Visibility cannot be the secrecy of addresses. The address of
low-entropy content, such as a short message, a small edit or a number, can be guessed by hashing
candidates.
- *Where it is decided.* Visibility is decided at the boundary, the same place where identity inside
  a run becomes portable identity.
- *Restricted audiences.* An object whose audience is restricted gets a portable key that only that
  audience can compute. That gives up sharing storage with identical objects outside the audience,
  and leaves its semantic identity unchanged.

### 8.7 Results across machines

**Constructed.**
- *Identity is not execution.* "Content identity establishes which definition is requested. It does
  not establish that an arbitrary remote machine executed it. A result can be checked directly,
  accompanied by evidence, or accepted under an explicitly selected execution assumption" (§10.6).
- *Exact reuse.* A cached result is valid exactly when its dependency certificate validates: what
  was observed at each key, absence included (§10.1, §10.5).
- *Effects.* Effectful results carry an effect scope and are never replayed as pure lookups (§10.5).

**What follows.** Every evaluation anywhere is reusable everywhere, within each reader's accepted
assumptions. Results are of two kinds:
- *Self-evidencing.* A typing derivation, a path, or a normal form with its derivation is cheaper to
  check than to produce, so it travels with no trust needed.
- *Assumption-tagged.* Anything else carries "accepted because machine M produced it" as an
  assumption, itself a term, and each view chooses which assumptions it accepts.

The planet's computation needs no global trust. Each result is either self-evidencing or tagged with
the assumption it rests on.

### 8.8 Upgrades, including the kernel's own

**Constructed.**
- *A program changing a running program is an ordinary program.* Its correctness is
  Π_s I(s) → I′(u(s)) over the actual executions (§10.7).
- *Mixed versions.* Mixed-version transitions, consisting of install, drain, reroute and retire, are
  defined by the application, "without introducing a privileged Live, Deployment, or Message
  construct" (§10.7).
- *One computation.* An edit's propagation through proofs, residual parsers, native artifacts,
  renderings and the live rollout is one computation (§10.11).

**What follows.** The evaluator is a mechanism, admitted by its relation to meaning (§3.1). So:
- two machines on different evaluators agree on every self-evidencing result;
- the old and new definitions coexist, because objects are immutable;
- each running process moves at its own step, through a migration term.

There is no flag day and no version skew. "Which version are you on" stops being a question about
data and becomes a question about which roots a view follows.

## 9. The interface is a program

**Constructed.** "The interface itself is a program: a visualization of a data structure, a proof
tree, a graphical state machine, a source editor, and a distributed trace explorer are functions on
the same mathematical objects. The conventional text-editor-centered IDE is one possible
presentation, not the definition of programming" (§10.9).
- A source rendering is one view of a typed object.
- A debugger works on the fiber of compatible runs and leaves what is unknown as residual
  computation, "not a log-search interface pretending to have reconstructed the program" (§10.9).
- Bidirectional editing obeys lens laws where they hold, and weaker laws where they don't (§10.2).

**Decided.**
- "i have now decided that I do not plan on ever opening a text editor or IDE again. assume no such
  thing even exists."
- "this is a masterwork of malleable software. i.e. beyond the dreams of ink&switch. the endgame of
  a global operating system of such."

**The organization of work is the first instance.** The organization of Hyperactive's own work is
the first place the system is used on itself.
- *The work is a term.* It is a graph of goals named by sentences (`lib/task-graph.cub`,
  `task-graph/hyperactivity.cub`).
- *Order is computed.* The order of work is computed from dependencies and from supersession over
  prerequisite sets, never assigned as a scalar priority.
- *Expansion happens when work begins.* A goal is expanded into subgoals only when work on it begins.
- *Questions are subgoals.* A question is "a local subgoal about management of one's own uncertainty
  on information relevant to a higher goal" (state-change §8).
- *One human.* "this is a system for one human being. agents should have no agency" (task-graph
  §15).
- *For now.* The rendering at `hyperactive server` is the interim interface. Until the work moves
  onto the system itself, git holds it, with one session, on main only, and every change pushed the
  moment it is made.

## 10. Every language, lifted

**Constructed.**
- *Lifting a language.* An existing language is presented by its syntax and its semantics as terms,
  and its programs are operated on without pretending that source syntax, host terms and machine
  bytes are identical (§5, §8).
- *Annotations gain meaning.* "Unlike comments, such an annotation has a defined type, scope,
  interpretation, and equality relation" (§8). A transformation of C++ source carries a proof of
  equivalence or refinement, and erasing it back to C++ is "a separately defined operation with a
  separately proved property."

**Decided.** The language becomes a way to lift and project code from those languages with all the
Hyperactive value:
- their sources and semantics become Hyperactive terms;
- proofs are written and checked against them;
- Hyperactive features project back into the source language by language extension.

"any existing language can become a language supporting proof, metaprogramming", at "basically
zero-cost and pure-value-add". There is no "hardware" category: x86 is a language like any other.

## 11. Content is identity; value is the open interaction

**Constructed.**
- *Content identity.* A definition's identity is its content. A peer exchanges a definition, its
  dependency closure and its proofs, and the recipient checks them against their mathematical
  definitions, so that "No centralized human-readable namespace is needed to establish the identity
  of the received definition" (§10.8).
- *Conserved invariants without a scheduler.* A conserved distributed invariant holds with no global
  scheduler as an axiom, and local knowledge is a construction over partial observation (§10.3).
- *Open interactions.* A composite of interacting processes is itself one process, with some
  interactions internalized and the rest left open (§7.6).

**Where the paper stops.** The paper stops on purpose: this "does not make every distributed trust
problem disappear: the authenticity of external observations and the semantics of physical machines
remain explicit premises" (§10.8). The substrate "does not prescribe a social governance or
participant-authority protocol" (§10.6). The precise form of what follows keeps that line.

**Thesis.** The intermediary has no forced function.
- *What it does now.* An intermediary names, authenticates definitions, verifies, ranks and meters.
  Once identity is content and verification is local, naming and authenticating definitions have no
  technical necessity left, and nor does verifying anything that carries its evidence.
- *What remains.* The authenticity of external observations, and executions that are not
  self-evidencing, remain premises. Even those are assumptions each view selects (§8.7), so no one
  is forced to be the center for them either. Whatever role an intermediary keeps is one that some
  view elected.
- *Value.* Value is the open interaction and its continuations, not a stock: a process's worth is its
  live, open interface.
- *Rent.* The extractive order has to reify value into an owned, transferable stock: the user as
  inventory, attention as a priced good, data as an asset, a model behind a meter. Rent needs a thing
  to charge for, and that reification is the self-standing table of §3 at the scale of an economy.
  Content identity and local verification remove what the rent taxed.
- *What remains after that.* A medium that is peer-to-peer, interdependent and locally verified, with
  value living in interaction. That is the condition for human agency, self-sufficiency and
  independence from misaligned actors, which was the point.

*Witnessed by:* the system running between peers, with no service in the path, for the functions
listed. Each view's elected assumptions are then visible as terms, and what is left of the center is
exactly what those terms say.

## 12. Gradient descent and the corporate-AI form

**Thesis.**
- *What both are.* Gradient descent and the corporate-AI form are both what is run without the
  construction:
  - descent memorizes the integral of a connection it cannot see (§5);
  - the lab meters access to that memorized integral, shapes its outputs, and calls the unwitnessed
    shaping safety (§7, §11).
- *How both end.* Both end when the construction is available: composition replaces descent, and
  local verification replaces the meter. They end together, because the missing construction is
  the cause of both.
- *Witnessed by:* the witnesses of §5, §7 and §11, which are this document's witnesses taken
  together.

## 13. Physics in the same frame

**Constructed.** Source-free Maxwell evolution is exactly the complex field equation i∂ₜF = ∇ × F,
with F := E + iB and ∇ · F = 0: "an exact algebraic change of representation" (paper A.4, eq. A.9).

**Thesis.**
- *The same frame.* The relational actualization of a value only in interaction, and a symmetry of
  association that makes directed causality a projection, are constructions under the same
  composition.
- *Relational time.* That keeps the account in tune with relational time.

*Witnessed by:* the A.4 change of representation as a checked equivalence. Then the projection that
yields directed causality, defined as an observation in the sense of §3, with its fiber computed.

---

## Sources

Avik's words are in `docs/`, verbatim and append-only.
- *Where §2–§7 and §11–§13 come from.* Material Avik provided on 2026-11-13, kept verbatim in
  `docs/what-hyperactive-is-for-in-avik-words.md`.
- *What this document adds.* This document sharpens that material where the paper requires it: filling
  versus its obstruction (§2), univalence using an identification rather than finding it (§5), and
  the paper's own boundary on trust and governance (§11). It also adds identity and its mechanisms
  (§3.1) and the running system (§8).
