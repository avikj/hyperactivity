# hyperactivity — dense form

*A decentralized operating system language for collective superintelligence.* Avik Jain, v31, 14 Nov 2026.

v31 rewritten for a reader fluent in everything published through 2025: named prior work is a pointer, not exposition. Every definition, equation, theorem, premise, example, dependency and claim of v31 is kept, with its section and equation numbers.

**Abstract.** A description of the universe is an event in it, so reasoning is part of what happens. Constructively:

- routines, relations and rationales act on one another, the interpreting definitions included;
- knowledge moves along proved equivalences and keeps the dependencies that govern its reuse;
- from states, actions and interactions come histories, observations and evidence of what they warrant;
- incomplete information stays a family of possibilities, never a preferred history;
- utterances change the context of later ones, and interpreter and history are inside the account;
- dependent types hold routine, property and rationale in one language, and computational univalence transports operations with their proofs;
- reasoning is kept, not transcribed into a second proof language. Example: sorting, a sorted answer with multiplicity evidence obtained by elimination over a bag and its identities.

Reflection: existing languages keep their specified semantics while their interpreters, checkers and transformations become operands. Under evaluator-adequacy and machine-simulation premises, one realized evaluator executes every represented program, those operations included. The native route is C plus a downstream compiler, with correctness relations composed along it. Symbolic residuals keep unknown inputs, dependency evidence governs reuse, continuing interactions keep their history, and decentralized exchange shares constructions with their rationales. *Hyperactivity* is this conductance of knowledge into activity: a persistent, interactive language of processes that includes understanding and changing them.

## 1. The universe and its descriptions

The universe has no outside: it contains this text, its author and reader, the history of the words, and the present interpretation. Observer, instrument, language and reasoning are histories within it, not external standpoints. A description therefore has to keep three dependencies: an action on its participant's information, an interpretation on its receiving context, and a conclusion on its evidence. They decide what is established, not just how it is written. Hyperactivity makes them explicit and operable, including by activity that changes the descriptions themselves, with no unexplained intelligence outside the account.

**Process.** A state type 𝑆, actions 𝐴(𝑠), and a relation 𝑇(𝑠,𝑎,𝑠′) give a labeled transition with evidence. Determinism selects the successor; nondeterminism keeps the alternatives; an open interaction includes the response its continuation depends on. Policy, transition and observation are distinct constructions, and none of them implies a scalar reward or a distribution.

The subject is the same for a processor, an organism or a conversation. Composition internalizes chosen interactions, and moving the boundary changes which inputs stay open. ODEs, supplied stochastic laws and guarded interaction are further evolution descriptions; no transition presentation replaces them or makes a machine the source of their significance. A participant's information is less than the history, and inference asks what follows from what was obtained, not which unseen history is preferred.

**Observation.** For histories 𝐻 and 𝑜 ∶ 𝐻 → 𝑂:

  Fib𝑜(𝑦) ∶= ∑_{ℎ∶𝐻}(𝑜(ℎ)=𝑦)  (1.1)
  𝐻 ≃ ∑_{𝑦∶𝑂} Fib𝑜(𝑦), ℎ ↦ (𝑜(ℎ), ℎ, refl)  (1.2)
  Warrants(𝑦,𝑃) ∶= ∏_{(ℎ,𝑝)∶Fib𝑜(𝑦)} 𝑃(ℎ), for 𝑃 ∶ 𝐻 → 𝒰  (1.3)

- An observation supports 𝑃 exactly when (1.3) is inhabited. It can be inhabited vacuously on an empty fiber, unless the reading is known to have occurred, which supplies a compatible history.
- Refinement shrinks fibers, so conclusions persist.
- The observer, instrument and interpretation can join a larger history under the same construction.
- If 𝑃 is an action condition, it is established without choosing a hidden state, and responses change the fiber through the action–response relation (§10.4).
- Probability is never introduced to hide what is missing. A supplied probabilistic description is extra information.

**Example (a law that does not descend).** 𝐽 = Bool², 𝑠(𝑎,𝑏) = (𝑎⊕𝑏, 𝑏), 𝑜(𝑎,𝑏) = 𝑎. Then 𝑠 is an involution, yet no 𝑔 has 𝑔∘𝑜 = 𝑜∘𝑠: the points (T,F) and (T,T) force 𝑔(T) = T and 𝑔(T) = F. The visible coordinate has no autonomous next state although the whole loses nothing. A law's observer-relative content is the existence of a commuting map on observed states. The same descent question recurs for source projections and distributed views.

**Language.** Knowledge is inherited through language. A word, with its uses and etymology, carries distinctions no present speaker originated and opens onto the activity of the human superorganism. That inheritance is distributed across participants and artifacts, not a stock any one knower holds, and language sits inside the history studied, not in a dictionary fixed beforehand. Utterances are produced in one state and received in another, and each exchange becomes context, so meaning depends on receiver, history and interpretation; there is no context-free payload (antecedents 48–51; here the interpreter itself evolves).

  Receive𝑖 ∶ 𝑆𝑖 → 𝑈 → 𝑆𝑖 → 𝐴𝑖 → 𝒰  (1.4)

At 𝑠, the compatible readings of 𝑢 are ∑_{𝑠′}∑_{𝑎} Receive𝑖(𝑠,𝑢,𝑠′,𝑎); other participants and later states give other families. Send, delivery and receipt are transitions of a larger process. A claim of understanding is relative to available history and an observation of the response, with no shared hidden interpretation assumed. Gestures, images and instrument readings enter the same way, through observation and interaction, never by labelling. So perception and natural-language interpretation are part of the process account, not applications added on top.

**Routines, relations, rationales.** A rationale maps evidence to evidence, a routine transforms relations or routines, and a relation says what a rationale establishes. Their mutual application reaches the language's own definitions and interpreters, so an established relation enters later computation with its dependencies intact. A requirement on 𝑓 ∶ 𝐴 → 𝐵 is a chosen family 𝑃 ∶ ∏_{𝑎∶𝐴} 𝐵 → 𝒰, and

  𝑞 ∶ ∏_{𝑎∶𝐴} 𝑃(𝑎, 𝑓(𝑎))  (1.5)

Correctness is relative to 𝑃. Building conforming behavior already contains this reasoning; writing it in the same language keeps it, and the design objective is zero marginal proof-writing. Tests sample executions, while 𝑞 covers the whole domain (Dijkstra, 68). With 𝑒 ∶ 𝐴 ≃ 𝐵, ua(𝑒) transports dependent constructions, and for structured equivalences operations travel with their laws' rationales (§4). Forgetful observation, refinement and full structural equivalence differ in type and consequence. Only a retained relation licenses the next operation; saying "the same" gives no evidence.

**Self-application.** Grammars, judgments, interpreters and transformations are represented definitions. Transforming an interpreter's represented clauses changes how later inputs are read (§6.3). Source languages keep their meaning while becoming operands, and syntax, interpretation, transformed source and machine code are related by constructed evidence, with no separate semantic authorities. A machine is one process. Under §9.2's premises a realization of one evaluator runs every represented term; direct compilation gives other realizations, each with its own evidence. Implementation is a dimension of the construction, not its premise.

**Conductance.** A result is useful while its dependencies hold. Retained observations govern reuse, symbolic residuals keep unknowns, continuations keep issued effects, and a definition sent with its dependencies and rationale becomes usable by others (§10). Conductance is the availability of established relations to have consequences, not a substance or a scalar objective. Order of the paper: §§2–4 constructions and identity, §§5–6 languages and reflection, §§7–9 machines and realization, §10 retained inference and distribution. Each stage acts on the earlier ones; nothing changes "from nature to software".

### 1.1 Computational cubical foundations

The judgments are Γ ⊢ 𝐴 ∶ 𝒰ℓ, Γ ⊢ 𝑡 ∶ 𝐴 and Γ ⊢ 𝑡 ≡ 𝑢 ∶ 𝐴, with interval variables and face conditions in Γ. The calculus is CCHM [9]: paths, composition, filling, Glue, transport and univalence, plus inductive and higher-inductive types. It adds clock-indexed guarded computation [44] for continuing processes. These rules are how the language computes with the identities of its own constructions. transport𝑃(𝑝) ∶ 𝑃(𝑎) → 𝑃(𝑏) for 𝑝 ∶ 𝑎 = 𝑏, and ua(𝑒) ∶ 𝐴 = 𝐵 for 𝑒 ∶ 𝐴 ≃ 𝐵. Transport along it computes the object over 𝐵; these are language operations, not annotations on another program.

### 1.2 Programs about programs

- An inductive `Code` with `eval-g` from codes to denotations.
- The checker is a term. It observes evaluator values through the structured view `%view`, checks types and conversion, and returns its judgment.
- The specializer normalizes and rewrites by equality; its outputs pass the same checker as ordinary definitions.
- The bootstrap holds the initial code representation and the checker. The checked, context-and-type-indexed code family is separate (§6.1). Neither assumes unrestricted quotation of semantic values.

A represented language gives grammar, contexts, judgments, transitions, effects and observations. Interpreting, transforming, compiling and proving are computations on it: an interpreter is a routine, its adequacy is a property, and the adequacy proof is a program. The resulting equalities, equivalences and refinements are reusable. A language's meaning is its specified interpretation, never one privately assigned by the checker or compiler. Metacircularity therefore covers the whole of interpretation and transformation, not only self-inspection of syntax.

### 1.3 Generality and examples

An open computation is a program, and supplying inputs specializes it. On top of Church–Turing (1, 2), descriptions and their interpretations are constructions available to further computation. Process and handler composition (§7.6) applies at every scale: function, machine, OS, fleet, organism, speakers. The laws give the content, and no name is a primitive boundary. The section order is a dependency order, and the examples are calculations inside the account, not a feature list. OS scope is the whole continuing activity: retained definitions and observations, running processes, interaction, and the operations that change all of these. Personal and interpersonal computation share one foundation. Participants start from local state, and exchange needs no universal owner or authoritative view (§10.8). *Collective superintelligence* is exact, reusable reasoning across those interactions: conclusions keep their evidence and dependencies and stay operable. The constructions fix what is retained, communicated and required, not a visual interface.

### 1.4 Scientific context and antecedents

- **Nature as computation:** Wolfram, NKS (64), and the ruliad, with observers inside (65). Here the build is from cubical type theory and specified interpretations; Wolfram situates the question, and dependencies are stated in the results.
- **Information:** Shannon (45), Barwise–Seligman (46), Pearl (47), Barwise–Perry (69) answer different questions. The fibers state exactly which histories remain and what a conclusion must cover.
- **Language use:** Lewis, Grice, Clark (48–50) and grounding, Harnad (51). (1.4) specifies a changing interpreter with its causal history; it is not a full theory of natural language.
- **Propositions as types:** (52), extended by §§3–4 with computational identity and transport.
- **Cybernetics and life:** Wiener (53), Ashby (54), Maturana–Varela (71). Their definitions can be examined in this language; their results are not premises of program meaning.
