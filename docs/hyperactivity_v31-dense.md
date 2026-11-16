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

## 2. Types, witnesses, and identity

§1's inference rests on this: a type says what can be constructed, dependent types let the form depend on inputs, and identity is a type, so a claim about two constructions is a value later operations use. An occurrence is a position, not a label: (𝑥, 𝑥) has two positions holding equal values.

- Finite families: Fin(0) ∶= 0, Fin(𝑛+1) ∶= 1 + Fin(𝑛), 𝑣 ∶ Fin(𝑛) → 𝐴 (2.1). Permuting indices changes the enumeration, not necessarily the structure.
- Enumeration(𝐴) ∶= ∑_{𝑛∶ℕ}(𝐴 ≃ Fin(𝑛)) (2.2).
- HasCard(𝐴, 𝑛) ∶= ‖𝐴 ≃ Fin(𝑛)‖ (2.3). The truncation keeps that an equivalence exists, not one to compute with.

Types and terms are not "data" versus "code". Γ ⊢ 𝑡 ∶ 𝐴 (2.4) says 𝑡 constructs 𝐴 in Γ, and application, pairing, Σ, Π, induction and computation act on such constructions. A proposition is a type whose inhabitants are the evidence in question; a proof is an inhabitant.

### 2.1 Construction, elimination, and the difference between data and property

Routine and reasoning use the same constructors and computation rules.

- Σ_{𝑎∶𝐴} 𝑃(𝑎) with fst, snd (2.5); Π_{𝑎∶𝐴} 𝑃(𝑎) (2.6). Instances: finite maps, length-indexed vectors, a parser returning trees indexed by grammatical category, a program paired with its correctness evidence. Σ and Π express both executable data and evidence.
- Inductive types come with constructors and eliminators (Martin-Löf; Coquand–Huet's CoC, 6, 7). For ℕ: recℕ(𝑧,𝑠,0) ⟼ 𝑧 and recℕ(𝑧,𝑠,𝑛+1) ⟼ 𝑠(𝑛, recℕ(𝑧,𝑠,𝑛)) (2.7). The function is defined by its computation rules, and its properties and their proofs are terms of the same language. So the reasoning needed to build it correctly can be part of its construction, not reconstructed later in another notation.
- A proposition has at most one inhabitant up to equality; a type may carry many values and higher paths. Truncation is right when only existence matters. Where the witness will be executed, transformed or transported, truncating destroys information irreversibly, and the types state that choice exactly.

### 2.2 Relations are constructions from operations and identity

𝑎 ≤ 𝑏 ∶= ∑_{𝑐∶ℕ}(𝑎 + 𝑐 = 𝑏) and 𝑎 < 𝑏 ∶= ∑_{𝑐∶ℕ}(𝑎 + suc(𝑐) = 𝑏) (2.8). These are witness-carrying orders. Cancellation makes the witnesses unique, so both are propositions, and truncating them changes nothing. Transitivity is a function: from (𝑐,𝑝) and (𝑑,𝑞) take 𝑐+𝑑 with 𝑎+(𝑐+𝑑) = (𝑎+𝑐)+𝑑 = 𝑏+𝑑 = 𝑒 (2.9), by associativity, 𝑝 and 𝑞. Reflexivity uses 0; antisymmetry comes from addition on ℕ.

The same pattern covers semantics. Big-step Eval(𝑒,𝑣) as an inductive family of rules, or ⟦𝑒⟧ = 𝑣 when evaluation is a function, related by an equivalence of witness types proved by induction on derivations. A semantics is the defined structure and its equalities, not a layer connected to mathematics afterwards.

A relation 𝑅 ∶ 𝐴 → 𝐵 → 𝒰 has the instance space Rel(𝑅) ∶= ∑_𝑎∑_𝑏 𝑅(𝑎,𝑏) (2.10). Over fixed carriers it is the family on 𝐴×𝐵, equivalently that total space with both projections. An equivalence of presentations has to commute with the projections; a bare equivalence of total spaces forgets which inputs and outputs the witnesses relate.

## 3. Computational equality

Path operations are CCHM's (9), with cubical-set semantics (10); identity versus equivalence is the univalent treatment (8).

- refl𝑎 ∶ 𝑎 = 𝑎 (3.1). Paths: 𝑝 = ⟨𝑖⟩𝑝(𝑖) with 𝑝(0)=𝑎, 𝑝(1)=𝑏 (3.2), over 𝕀 with connections and reversal, and dependent paths over a type varying in 𝑖. (⟨𝑖⟩𝑡)@𝑟 ⟼ 𝑡[𝑖∶=𝑟] (3.3).
- What matters is that equality acts: transport𝑃(𝑝) ∶ 𝑃(𝑎) → 𝑃(𝑏) (3.4) is trivial for constant 𝑃. For dependent 𝑃 it computes the change of representation.
- Sing(𝑎) ∶= ∑_𝑥(𝑎 = 𝑥) is contractible (3.5), with center (𝑎, refl) and the contraction built cubically through 𝑖 ∧ 𝑗. Keeping a value with its equality witness adds no independent information. The eliminator is in Martin-Löf's identity-type lineage (6).
- Graph(𝑓) ∶= ∑_{𝑏∶𝐵}∑_{𝑎∶𝐴}(𝑓(𝑎) = 𝑏) (3.6) and 𝐴 ≃ Graph(𝑓) (3.7), by 𝑎 ↦ (𝑓(𝑎), 𝑎, refl) and projection: one composite reduces, the other contracts by Sing. This does not make 𝑓 invertible; it says exactly what has to be kept for its action to be reversibly represented.

The recurring point: an observation can forget information that is still present in the observed object.

### 3.1 Cubical operations are actual computation

𝕀 is generated by dimension names under ∧, ∨ and 1−𝑟 (De Morgan). Faces such as (𝑖=0) ∨ (𝑗=1) describe partial boundaries. A path is a term depending on a dimension, not a certificate issued after two evaluations. Composition comp^𝑖 𝐴[𝜑 ↦ 𝑢] 𝑎₀ ∶ 𝐴(1) (3.8), with 𝑢(𝑖=0) = 𝑎₀ on 𝜑, fills the missing end compatibly with the boundary. Its reduction is given by the per-former rules (Π, Σ, Path, universes, Glue) of the chosen calculus; the schema doesn't replace them. Substitution also substitutes interval expressions for dimensions, and composition computes the value that makes a dependent boundary coherent. So equality is something the evaluator acts on, not a proposition whose only role is to be accepted by a checker.

### 3.2 Identity is neither syntactic identity nor observational coincidence

Three relations stay distinct: definitional (same normal form), propositional (an identity-type inhabitant), and observational indistinguishability under a chosen experiment, which falls short of identity of the whole structure. Example: 𝑓(𝑛) = 𝑛+𝑛 and 𝑔(𝑛) = 2𝑛 (3.9). They are extensionally equal once the arithmetic is proved, despite different syntax and cost, and funext turns (∏_𝑛 𝑓(𝑛) = 𝑔(𝑛)) into 𝑓 = 𝑔 (3.10). That equality says nothing about equal energy or time; those are further observations. A proof is powerful because it has an exact type, and that type does not license erasing distinctions it doesn't cover.

### 3.3 Equality of representations preserves complete information

(3.6)–(3.7) are reused as they are. A represented transition keeps source, target and evidence, and observations may project any of them away. Transition ∶= ∑_𝑠∑_{𝑠′} 𝑇(𝑠,𝑠′) (3.11). Executions compose steps with matching ends, finitely or by guarded continuation. An observation keeps a final state, an event sequence or a scalar, and its map says exactly which distinctions it forgets. Whether a kept state suffices for future behavior depends on the continuation and its observations, not on what the state is called. Equivalent presentations of complete histories transport dependent constructions by ua. Equality of complete constructions, equality of observations and equivalence of representations are different claims. A projection limits what it distinguishes, not what the language can express.

### 3.4 Dimension, face, composition, and path judgments

The judgments Γ;Ψ;𝜑 ⊢ 𝐴 type, ⊢ 𝑡 ∶ 𝐴, ⊢ 𝑡 ≡ 𝑢 ∶ 𝐴 and ⊢ 𝐴 ≡ 𝐵 type are mutually defined (3.12).

- 𝕀 is the free De Morgan algebra on the dimension variables. Faces are ∧/∨ of atomic interval equations. Δ ∶= Γ;Ψ;𝜑.
- Path introduction: from Δ,𝑖 ⊢ 𝑡 ∶ 𝐴 with 𝑡[𝑖∶=0] ≡ 𝑎₀ and 𝑡[𝑖∶=1] ≡ 𝑎₁, conclude ⟨𝑖⟩𝑡 ∶ Path^𝑖.𝐴(𝑎₀,𝑎₁). Elimination: 𝑝@𝑟 ∶ 𝐴[𝑖∶=𝑟] for Ψ ⊢ 𝑟 ∶ 𝕀 (3.13). Principal reduction (3.3).
- Composition needs a filler for every type former. From Γ;Ψ,𝑖;𝜑 ⊢ 𝑢 ∶ 𝐴, 𝑎₀ ∶ 𝐴[𝑖∶=0] and 𝑢[𝑖∶=0] ≡ 𝑎₀ on 𝜑, conclude comp ∶ 𝐴[𝑖∶=1], which is ≡ 𝑢[𝑖∶=1] on 𝜑 (3.14).
- Per former:
  - Π: pointwise, after transporting the argument contravariantly.
  - Σ: the first projection, then the second over the first's filler.
  - Path: in its extra dimension.
  - Universes and Glue: by the computational univalence construction.

The rest of the Kan, universe, face and Glue equations are CCHM's [9]. The simply typed evaluator interprets one fragment; it is not the definition of the cubical foundation.

## 4. Equivalence, univalence, and structure

An equivalence becomes a path along which dependent constructions transport (8, 9). The programming consequence is that operations and their laws move together across a structural equivalence.

- 𝑒 ∶ 𝐴 ≃ 𝐵 means contractible fibers, or equivalently coherent inverse data. ua ∶ (𝐴 ≃ 𝐵) → (𝐴 = 𝐵) (4.1), built with Glue. transport^{𝑋↦𝑋}(ua(𝑒), 𝑎) =_𝐵 𝑒(𝑎) (4.2): ua-β is a path in 𝐵, and the calculus decides when it holds judgmentally. A path between values is not an equivalence of types.
- Vec(𝐴,𝑛) ≃ (Fin(𝑛) → 𝐴) (4.3) by index and tabulate, with index(tabulate 𝑔, 𝑖) = 𝑔(𝑖) and tabulate(𝜆𝑖.index(𝑣,𝑖)) = 𝑣 (4.4). Sorted ∶ Vec(ℤ,𝑛) → 𝒰 (4.5) transports to the function representation, and so does an operation with its proof of preserving sortedness. This is an operation on the dependent construction, not an external conversion.
- Structure must be included. A bare carrier equivalence does not preserve a multiplication, an order, a topology or a memory model. Monoid ∶= ∑_𝐴∑_{𝜇}∑_𝑒(Assoc(𝜇) × LeftUnit(𝜇,𝑒) × RightUnit(𝜇,𝑒)) (4.6), and identifying such structures transports operation and laws. SIP relates identities of suitably univalent structures to structure-preserving isomorphisms; it does not make every carrier bijection a monoid isomorphism.

Equality, equivalence, structured identity and observation are distinct relations, each computable under its own rules.

### 4.1 A representation change acts on a dependent operation

Take 𝑉 = Vec(𝐴,𝑛), 𝐹 = Fin(𝑛) → 𝐴, 𝑒 ∶ 𝑉 ≃ 𝐹, ℎ ∶ 𝑉 → 𝑉, 𝑃 ∶ 𝑉 → 𝒰, and 𝑞 ∶ ∏_𝑣 𝑃(𝑣) → 𝑃(ℎ(𝑣)) (4.7). Transport along ua(𝑒) gives:

- ℎ_𝐹 ∶= 𝑒 ∘ ℎ ∘ 𝑒⁻¹ (4.8);
- 𝑃_𝐹(𝑤) ∶= 𝑃(𝑒⁻¹𝑤) (4.9);
- 𝑞_𝐹 ∶ ∏_𝑤 𝑃_𝐹(𝑤) → 𝑃_𝐹(ℎ_𝐹(𝑤)) (4.10), obtained by applying 𝑞 at 𝑒⁻¹𝑤 and correcting with the inverse laws.

That is the hand derivation. Computational univalence internalizes the equivalence and supplies the general transport that does this across any construction. The point is not converting a value but moving a whole structure (carrier, operations, laws, programs, proofs) coherently; that is the difference between a conversion function and mathematics on representations.

*Package.* Package(𝑋) ∶= ∑_{𝑓∶𝑋→𝑋}∏_𝑥 𝑃_𝑋(𝑥) → 𝑃_𝑋(𝑓(𝑥)) (4.11). transport_Package(ua(𝑒)) ∶ Package(𝐴) → Package(𝐵) (4.12) yields the new operation and its preservation evidence together, computed by cubical transport for the chosen 𝑃, which fixes how the predicates correspond.

### 4.2 A graph modulo relabeling

𝐺 ∶ Fin(𝑛) → Fin(𝑛) → Bool (4.13), relabeled by 𝜋 ∶ Fin(𝑛) ≃ Fin(𝑛) as (𝜋⋅𝐺)(𝑢,𝑣) ∶= 𝐺(𝜋⁻¹𝑢, 𝜋⁻¹𝑣) (4.14). A shape is the quotient of this action. A set quotient gives the isomorphism class and discards higher identities; a groupoid presentation keeps automorphisms and their identities. These are mathematically different choices.

Positions 𝑝 ∶ Fin(𝑛) → ℝ² have to move too: (𝜋⋅𝑝)(𝑢) ∶= 𝑝(𝜋⁻¹𝑢) (4.15). Adjacency, positions, a selected node and a proof of a graph property bundle into one dependent structure. In a graph groupoid or a univalent structured-graph type, a relabeling path acts on the whole bundle; a bare index permutation does not identify arbitrary indexed data. A UI drawing the graph and an algorithm computing a path are then two interpretations of one object, with relabeling invariance stated once. Likewise adjacency lists, matrices and edge sets: conversion has to preserve graph structure, not byte count, and an equivalence of structured representations carries algorithms and their correctness evidence across.

### 4.3 Structured equivalence as the actual content of representation independence

The lineage is representation independence and parametricity (Reynolds, 36). Here the transported structure is stated with cubical identity. A representation change is 𝑒 ∶ 𝐴 ≃ 𝐵 plus preservation of the operations: 𝜇_𝐵(𝑏₁,𝑏₂) ∶= 𝑒(𝜇_𝐴(𝑒⁻¹𝑏₁, 𝑒⁻¹𝑏₂)) (4.16). Associativity for 𝜇_𝐵 is 𝑒 applied to 𝜇_𝐴's associativity path, with the inverse laws cancelling the conversions. Operation, laws and change package into one dependent type transported as one value, and this conjugation is that transport made explicit.

None of this depends on programming:

- ℤ as classes of ℕ-pairs;
- ℚ as normalized fractions or as classes of pairs;
- polynomials as coefficient sequences or as syntax trees;
- graphs as adjacency or as incidence.

Each comes with the operations and equalities that define the structure, and the object's mathematics is what makes representation independence possible.

A quotient identifies specified distinctions, and operations on it have to respect them; other observations may need another representation. A set quotient has points [𝑥], paths [𝑥] = [𝑦] for 𝑥 ∼ 𝑦, and set truncation. Its eliminator into a set 𝐵 takes a ∼-respecting 𝑋 → 𝐵; elimination into higher types needs coherence data, and HITs can keep nontrivial paths (HIT semantics, 11). For graphs, identifying up to renaming as a set loses the automorphism witnesses that a groupoid keeps. The identity asserted governs the computation: quotient, equivalence, path of structured types and observation are not rival theories of meaning but constructions with different specified equalities, composed in one language.

### 4.4 Ordering: a declaration that computes

Sorting is a complete example before any language or machine is introduced. Goal: a list with a given finite bag's multiplicities, in the order on its elements, with list and evidence as one dependent answer. This develops §9 ("Ordering — what counting discards") of *Respecting Equality* (74), including the declaration of its Remark 9.15, and its executable construction `lib/sort.cub` (75).

**The order supplies counting's equality.** Take le ∶ 𝐴 → 𝐴 → Bool that is reflexive, total, transitive and antisymmetric, with 𝑥 ≤ 𝑦 ∶= isTrue(le 𝑥 𝑦). Then eqo(𝑥,𝑦) ∶= le(𝑥,𝑦) ∧ le(𝑦,𝑥), and isTrue(eqo 𝑥 𝑦) → 𝑥 = 𝑦 is antisymmetry, with reflexivity giving eqo(𝑥,𝑥) (4.17). Multiplicity and order share one element identity, and elements are inspected only through le.

The bag is the HIT with points emp, sng(𝑎), un(𝑋,𝑌) and path constructors for left and right unit, associativity and commutativity, e.g. unitl(𝑋) ∶ un(emp,𝑋) = 𝑋, assoc, comm(𝑋,𝑌) ∶ un(𝑋,𝑌) = un(𝑌,𝑋) (4.18). The paths are kept.

- countB(𝑎, emp) = 0, countB(𝑎, sng 𝑏) = [eqo(𝑎,𝑏)], countB(𝑎, un(𝑋,𝑌)) = countB(𝑎,𝑋) + countB(𝑎,𝑌) (4.19). The path branches are the unit, associativity and commutativity paths of +, and list count(𝑎,𝐵) is analogous.
- AllLe(𝑥,[]) = 1, AllLe(𝑥, 𝑦∷𝐵) = (𝑥≤𝑦) × AllLe(𝑥,𝐵); Sorted([]) = 1, Sorted(𝑥∷𝐵) = AllLe(𝑥,𝐵) × Sorted(𝐵) (4.20).
- 𝐺_𝑋(𝐵) ∶= (∏_𝑎 count(𝑎,𝐵) = countB(𝑎,𝑋)) × Sorted(𝐵), 𝐹(𝑋) ∶= ∑_{𝐵∶List 𝐴} 𝐺_𝑋(𝐵) (4.21). In the source these are `Gs`/`Fs`, using decidable ℕ-equality, with `eqb-sound`/`eqb-complete` relating the two presentations. The answer holds the list, per-element multiplicity evidence and sortedness; projecting the list doesn't change what the whole answer establishes.

**Merge and uniqueness.** Empty clauses return the other list; otherwise merge(𝑥∷𝑋, 𝑦∷𝑌) is 𝑥∷merge(𝑋, 𝑦∷𝑌) when le(𝑥,𝑦), else 𝑦∷merge(𝑥∷𝑋, 𝑌) (4.22). Each call removes a head. In branch 1, 𝑥 ≤ 𝑦 and sortedness of 𝑦∷𝑌 with transitivity put 𝑥 below 𝑦's run, and sortedness of 𝑥∷𝑋 puts it below 𝑋. Branch 2 is symmetric, using totality for 𝑦 ≤ 𝑥. By induction, count(𝑎, merge(𝐵,𝐶)) = count(𝑎,𝐵) + count(𝑎,𝐶) and Sorted 𝐵 × Sorted 𝐶 → Sorted(merge 𝐵 𝐶) (4.23).

*sorted-unique:* two sorted lists with equal multiplicities are equal.

- One empty: the other's head has a positive count on one side and zero on the other.
- Both nonempty with heads 𝑥, 𝑦: 𝑥 has positive count in the second list, whose elements are all ≥ 𝑦, so 𝑦 ≤ 𝑥; symmetrically 𝑥 ≤ 𝑦, and antisymmetry identifies the heads.
- Cancelling the heads equates the tails' counts, and induction finishes.

The evidence in 𝐺_𝑋 is propositional (ℕ-equality, and at most one witness per Boolean test). So list equality extends to the whole answer: Fs-prop ∶ ∏_𝑋 isProp(𝐹(𝑋)), with isProp(𝑇) ∶= ∏_{𝑢,𝑣} 𝑢 = 𝑣 (4.24).

**Elimination over the bag, paths included.**

- Point branches: mempty ∶ 𝐹(emp); msng ∶ ∏_𝑎 𝐹(sng 𝑎), already sorted; munion ∶ 𝐹𝑋 → 𝐹𝑌 → 𝐹(un 𝑋 𝑌), merging the list parts with (4.23) and the incoming witnesses (4.25).
- Path branches: over each bag path 𝑞 ∶ 𝑋 = 𝑌 the eliminator needs a dependent path in 𝐹(𝑞(𝑖)), not an unrelated list equality. Every fiber is a proposition by (4.24), so endpoints determine it. For comm, PathP(𝑖 ↦ 𝐹(comm(𝑋,𝑌)(𝑖)))(munion 𝑢 𝑣, munion 𝑣 𝑢) (4.26) comes from isProp→PathP, and likewise for unit and associativity.
- Result: msort ∶ ∏_{𝑋∶Bag 𝐴} 𝐹(𝑋) (4.27). Its point equations are the ones above, and its path equations make the answer respect the input's identities instead of reading a tree presentation as extra order.

```
msort : {o : Ord} → (X : Bag (El o)) → Fs X
msort X = msort-at (Fs-prop {o})
  (λ x0 x1 → isProp→PathP {lzero} {x0} {x1}) X
```

`msort-at` is exactly the dependent elimination above. Propositionality gives coherence and merge with the eliminator gives existence, so 𝐹(𝑋) is contractible with center msort(𝑋) and contraction 𝑣 ↦ Fs-prop(𝑋)(msort 𝑋, 𝑣). Declaration, construction and rationale are one executable value, with no enumeration of permutations and no proof reconstructed afterwards.

Instance: ({3}∪{1}) ∪ ({2}∪{1}). The halves give [1,3] and [1,2], which merge to [1,1,2,3]. The evidence records two 1s, one 2, one 3 and zero of everything else. Any other association or order of the unions has an identified answer through the path branches, not through a convention that secretly enumerates the bag.

**What ordering retains; what a sum presentation costs.**

- *Information forgotten.* For 𝑛 distinct elements, the fiber of sorting over the sorted sequence is the 𝑛! enumerations: keeping the permutation recovers the input, returning only the list forgets it. With multiplicities 𝑚ⱼ there are 𝑛!/∏𝑚ⱼ! value sequences. The bag declaration keeps multiplicities and does not claim to keep an enumeration.
- *Work charged by a presentation.* For binary sum trees, 𝑛(one) = 1 and 𝑛(𝑙⊕𝑟) = 𝑛𝑙+𝑛𝑟; the output-production cost is 𝐶(one) = 0, 𝐶(𝑙⊕𝑟) = 𝐶𝑙+𝐶𝑟+𝑛𝑙+𝑛𝑟 (4.28). Then 𝐶(𝑡) = ∑_leaves depth, since each leaf is charged once per union above it. At 𝑛 = 2^𝑘 a balanced tree costs 𝑘𝑛 and a chain 2+3+⋯+𝑛: 8 versus 9 at four leaves, 24 versus 35 at eight. These counts belong to this merge-output model; they are not output copies, comparisons or evaluator rewrites.
- *Information resolved.* A merge of runs 𝑎, 𝑏 of distinct elements resolves one of C(𝑎+𝑏, 𝑎) interleavings. 𝐼(one) = 1 and 𝐼(𝑙⊕𝑟) = C(𝑛𝑙+𝑛𝑟, 𝑛𝑙)·𝐼𝑙·𝐼𝑟 = 𝑛(𝑙⊕𝑟)! (4.29), by cancelling child factorials. So log₂(𝑛!) is the same for every tree, while (4.28) depends on the tree.
- *Comparisons.* The worst-case lower bound is ⌈log₂ 𝑛!⌉; balanced merging at 𝑛 = 2^𝑘 uses at most 𝑛𝑘−𝑛+1, which is 16 versus 17 at 𝑛 = 8. Adjacent swaps measure something else, the inversion count, which drops by exactly one per corrected adjacent pair. Comparison identifies order; a swap changes an enumeration.

The tree's paths identify its value without equalizing evaluation costs. For powers of two, a balanced tree's root-to-leaf choices are binary indices and a chain is the tally presentation. Given the presentation and the elimination, the merges are determined. That is the declaration's computational content: use the identities the object carries, build the answer over them, and keep exactly the distinctions the chosen observation needs. Result, forgotten information and presentation-charged work stay separate, explicit objects.

## 5. Programming with languages and their semantics

The constructions that describe histories also describe the languages used to describe them: syntax, static judgments, interpretation, transitions and observable behavior are representable and computable. Foundations: inductive syntax, compositional interpretation, operational semantics, definitional interpreters (4, 5, 16, 27, 28). The general construction comes first, then a closed calculation.

### 5.1 Typed syntax, semantics, and source lifting

This is a dependent presentation of represented syntax and semantics, not a second primitive evaluator. The bootstrap `Code` datatype and the intrinsically checked family are different representations, and these equations describe the latter. Derivation-indexed syntax for a real source standard needs separate coverage evidence to be complete.

With 𝐽 = (Γ;Ψ;𝜑) over a core signature 𝒦 and level ℓ, the judgments are 𝐽 ctx, 𝐽 ⊢ 𝐴 ∶ 𝒰ℓ, 𝐽 ⊢ 𝑡 ∶ 𝐴 and 𝐽 ⊢ 𝑡 ≡ 𝑢 ∶ 𝐴. Raw syntax has explicit constructors and binder sorts; typed syntax carries derivations, not a promise that checking will succeed:

- Ty(𝐽,ℓ) = ∑_{𝐴∶Raw} Der(𝐽 ⊢ 𝐴 ∶ 𝒰ℓ)
- Code(𝐽,𝐴) = ∑_{𝑡∶Raw} Der(𝐽 ⊢ 𝑡 ∶ |𝐴|)
- Sub(Δ,Γ) = ∏_{𝑥∶𝐴∈Γ} Code(Δ, 𝐴[𝜎<𝑥])
- Env(∅) = 1, Env(Γ, 𝑥∶𝐴) = ∑_{𝜌∶Env Γ} ⟦𝐴⟧𝜌

De Bruijn substitution is structural, lifting under every binder:

```
subst(Var(j), σ)          = σ[j]
subst(Lam(A, body), σ)    = Lam(subst(A, σ), subst(body, lift(σ)))
subst(App(f, x), σ)       = App(subst(f, σ), subst(x, σ))
subst(Pair(a, b), σ)      = Pair(subst(a, σ), subst(b, σ))
subst(PathLam(i, t), σ)   = PathLam(i, subst(t, σ))
subst(Comp(A, φ, u, a), σ) = Comp(subst(A, σ), φ[σ], subst(u, σ), subst(a, σ))
lift(σ)[0]   = Var(0)
lift(σ)[j+1] = weaken(σ[j])
```

Every further constructor adds its own arity, binder lifts and dimension-substitution action. The lemmas are ⟦𝑡[𝜎]⟧𝜌 ≡ ⟦𝑡⟧(⟦𝜎⟧𝜌), by induction on the derivation, and (𝑡[𝜎])[𝜏] ≡ 𝑡[𝜎∘𝜏], by induction on syntax. Dimension substitution also transports face restrictions and composition's boundary systems.

**Represented source languages.** A language is a record with laws, Language_ℓ = ∑_{Syn, Ctx_𝐿, Ty_𝐿, Static, Conf, Cmd, Step, Init, Observe} Law_𝐿, where:

- Static ∶ Ctx_𝐿 → Syn → Ty_𝐿 → 𝒰ℓ (source well-formedness, not assumed to be cubical typing);
- Conf ∶ Syn → 𝒰ℓ; Cmd ∶ 𝒰ℓ; Reply ∶ Cmd → 𝒰ℓ;
- Step_𝑝 ∶ Conf(𝑝) → Return + Fault + Tau(Conf 𝑝) + ∑_{𝑐∶Cmd}(Reply 𝑐 → Conf 𝑝);
- Init ∶ (𝑝 ∶ Syn) → Input(𝑝) → Conf(𝑝); Observe ∶ Trace_𝐿 → Obs_𝐿.

Nondeterministic languages return a set, family or coinductive tree of successors. Fault follows the language's stated undefined-behavior contract and is never silently an ordinary result. The laws cover substitution and scope preservation, progress or specified fault, well-defined observations, and the chosen treatment of concurrency and environment responses. The interpreter is the guarded unfolding:

  interp_𝐿(𝑝,𝑥) = unfold_𝐿(Init(𝑝,𝑥)); unfold_𝐿(𝑠) = case Step_𝑝(𝑠) of Return 𝑣 ↦ return 𝑣 ∣ Fault 𝑒 ↦ fault 𝑒 ∣ Tau 𝑠′ ↦ delay(unfold_𝐿 𝑠′) ∣ Call(𝑐,𝑘) ↦ request(𝑐, 𝜆𝑟. unfold_𝐿(𝑘 𝑟)).

It is a host function with no authority beyond its definitions, and the syntax, static rules and dynamic rules are ordinary metaprogram inputs.

**Lifting, transformation, projection.**

- Lift(𝐿) = ∑_{𝑝∶Syn_𝐿} Well_𝐿(𝑝): source plus source derivation, not an unverified host program.
- meaning_𝐿(𝑝) = interp_𝐿(𝑝); Spec_𝐿(𝑝) = MeaningType_𝐿(𝑝) → 𝒰ℓ (schematic: a dependent predicate over the exact function or process type); Attached_𝐿(𝑝) = ∑_{𝑃∶Spec_𝐿 𝑝} 𝑃(meaning_𝐿 𝑝). A typical attachment is 𝜋 ∶ ∏_{𝑥∶Input 𝑝} 𝑄(𝑥, interp_𝐿(𝑝,𝑥)).
- Trans_{𝐿,𝑀}(𝑝) = ∑_{𝑞∶Lift 𝑀} Rel_𝑂(meaning_𝐿 𝑝, meaning_𝑀 𝑞): target syntax plus semantic comparison. If Rel is a path, transport moves specifications and proofs directly; if it is observational refinement, only refinement-closed properties transfer. The distinction is in the type.
- Proofs attach to conventional source as (𝑝, 𝜋) with erase_𝐿(𝑝,𝜋) = 𝑝 and meaning_𝐿(erase(𝑝,𝜋)) = meaning_𝐿(𝑝). No runtime language extension is needed. A typed sidecar, a structured attribute or an elaboration can carry the association; its semantics is these definitions, whatever the spelling.

**A complete intrinsically typed expression language** (a program in hyperactivity, not a built-in). It makes the construction and the substitution proof fully explicit.

- Types 𝐴,𝐵 ∶∶= Byte ∣ Unit ∣ 𝐴×𝐵 ∣ 𝐴+𝐵 ∣ 𝐴⇒𝐵; contexts Γ ∶∶= 𝜖 ∣ Γ,𝐴.
- Variables vz ∶ Var(Γ,𝐴; 𝐴) and vs ∶ Var(Γ;𝐵) → Var(Γ,𝐴;𝐵), where ";" abbreviates context extension: Var(Γ,𝐴;𝐵) = Var((Γ,𝐴),𝐵).
- Terms Tm(Γ,𝐴), generated by var, lit ∶ Byte → Tm(Γ,Byte), lam ∶ Tm(Γ,𝐴;𝐵) → Tm(Γ,𝐴⇒𝐵), app, pair, fst, snd, inl, inr, and case ∶ Tm(Γ,𝐴+𝐵) → Tm(Γ,𝐴;𝐶) → Tm(Γ,𝐵;𝐶) → Tm(Γ,𝐶).
- ⟦𝐴⇒𝐵⟧ = ⟦𝐴⟧ → ⟦𝐵⟧; Env(𝜖) = Unit, Env(Γ,𝐴) = Env Γ × ⟦𝐴⟧; lookup(vz,(𝜌,𝑣)) = 𝑣, lookup(vs 𝑖,(𝜌,𝑣)) = lookup(𝑖,𝜌).
- eval is the standard structural recursion: var by lookup, lit 𝑏 to 𝑏, lam 𝑡 to 𝜆𝑣. eval(𝑡,(𝜌,𝑣)), app by application, pairs and sums componentwise, case into its branch with (𝜌,𝑣) or (𝜌,𝑤). These are structurally recursive host terms, and a concrete evaluator may represent lam as a closure (𝑡,𝜌).

*Substitution theorem.* Renamings are 𝜎 ∶ ∏_𝐴 Var(Γ,𝐴) → Var(Δ,𝐴); substitutions are 𝜃 ∶ ∏_𝐴 Var(Γ,𝐴) → Tm(Δ,𝐴). lift(𝜃)(vz) ∶= var vz, lift(𝜃)(vs 𝑖) ∶= weaken(𝜃 𝑖); subst(𝜃, var 𝑖) ∶= 𝜃 𝑖, subst(𝜃, lam 𝑡) ∶= lam(subst(lift 𝜃, 𝑡)). Other constructors recurse, and each case branch uses lift 𝜃. evalEnv is defined by lookup(𝑖, evalEnv(𝜃,𝜌)) ∶= eval(𝜃 𝑖, 𝜌), with the binder law, an equality of extended environments:

  evalEnv(lift 𝜃, (𝜌,𝑣)) = (evalEnv(𝜃,𝜌), 𝑣)

It holds because both sides look up 𝑣 at vz, and at vs 𝑖 weakening and the weakening lemma reduce to eval(𝜃 𝑖, 𝜌); environment extensionality concludes.

**Theorem.** eval(subst(𝜃,𝑡), 𝜌) = eval(𝑡, evalEnv(𝜃,𝜌)).

*Proof.* Induction on constructors:

- var: by definition;
- app: by congruence;
- lam: for arbitrary 𝑣, the binder law plus the induction hypothesis, then funext on the closures;
- products and sums: by their constructors and eliminators, with the binder law in case.

These are all of the grammar's constructors.

### 5.2 A language as a definition

Arithmetic with let: Expr(Γ) ∶∶= var(𝑖) ∣ num(𝑛) ∣ plus(𝑒₁,𝑒₂) ∣ let(𝑒₁,𝑒₂), with let's body in Γ+1 (5.1). The evaluator is the unique structural recursion: ⟦var 𝑖⟧𝜌 = 𝜌(𝑖), ⟦num 𝑛⟧𝜌 = 𝑛, ⟦plus(𝑢,𝑣)⟧𝜌 = ⟦𝑢⟧𝜌 + ⟦𝑣⟧𝜌, ⟦let(𝑢,𝑣)⟧𝜌 = ⟦𝑣⟧(𝜌, ⟦𝑢⟧𝜌) (5.2). Meaning is computed from syntax and environment under the chosen definition; it is not intrinsic to the printed expression.

𝐷 removes let: 𝐷(let(𝑢,𝑣)) ∶= subst₀(𝐷𝑢, 𝐷𝑣) (5.3), where subst₀ is capture-avoiding substitution for the newest variable with shifting. The theorem is ∏_{Γ,𝑒,𝜌} ⟦𝐷𝑒⟧𝜌 = ⟦𝑒⟧𝜌 (5.4). The let case is §5.1's substitution theorem and the rest is immediate. This is a whole language transformation by ordinary recursion and induction, not a compiler subsystem installed into mathematics.

𝐷 is not an equivalence of syntax, because it forgets how the source was written; it preserves semantics. If source positions, sharing or cost are observed, (5.4) has to be strengthened or replaced, and the mathematics identifies exactly what is preserved.

Generally:

- a parser maps text to syntax and may reject;
- an elaborator maps surface syntax plus context to typed code, resolving implicit arguments;
- an interpreter maps code to semantics;
- a compiler maps representation to representation with an explicit behavioral relation.

Each is a definable, evaluable, transformable, provable function or relation, and nothing forces a fixed composition order. Here Γ = Fin(𝑛) and Γ+ = 1+Γ. Substitution (5.3) is §5.1's general substitution sending the newest variable to 𝑢 and the rest to themselves, and its semantic law is exactly the let case of (5.4), with the body receiving (𝜌, ⟦𝑢⟧𝜌). That is the inductive proof itself, not a second definition.

**Reusing a theorem** means applying a dependent function: ℎ ∶ ∏_𝑎 𝑃(𝑎) → 𝑄(𝑎) is used as ℎ(𝑎,𝑝). The hypotheses stay in the type, and a representation change transports the whole construction; there is no shortcut that drops hypotheses.

### 5.3 The source program and its entire semantic family

With effects and nondeterminism, Step ∶ Cfg → Event → Cfg → 𝒰 is inductively generated (5.5). Finite traces are its labeled reflexive–transitive closure, and continuing behavior is coinductive or guarded. Behavior(𝑝,𝑜) ∶= ‖∑_{𝜏∶Execution 𝑝} observe(𝜏) = 𝑜‖ (5.6). The truncation states existence only; keep it untruncated when witnesses or differences between executions matter. A probabilistic language keeps its probability structure, not just a possibility predicate. Syntax, interpretation, execution and proofs about execution are distinct related constructions in one language. They can be inputs to programs, including programs that change the interpretation of later expressions (§6.3). A fixed interpreter is one program, not a privileged boundary of meaning.

### 5.4 A dependent presentation of an existing language

Lang ∶= ∑_{Src∶𝒰}∑_{Ctx∶𝒰}∑_{Ty∶Ctx→𝒰}∑_{Tm∶∏_Γ Ty Γ→𝒰}∑_{Cfg∶Src→𝒰}∑_{Σ∶Signature} Sem(Src,Ctx,Ty,Tm,Cfg,Σ) (5.7), with universe levels suppressed. This is a presentation schema, not a new type former.

- Src keeps original source, including the distinctions needed for preprocessing, binding, locations and faithful projection.
- Tm(Γ,𝐴) holds checked expressions.
- Sem holds elaboration, configuration initialization, a labeled transition relation or guarded function, the command/response types, and the observation map.
- Standard nondeterminism is a relation of admissible transitions, and an implementation-defined choice is a parameter or an allowed set, never silently resolved by a deterministic interpreter.

Forming ⟦𝑝⟧_𝐿 and 𝑃(⟦𝑝⟧_𝐿) is ordinary host computation: build and apply the represented interpreter, with no new proof judgment. Fidelity to a published standard is a separate explicit adequacy relation, between the standard's derivations and the represented judgments, proved by translations both ways with side conditions and round trips. Expressing a language's definition is different from claiming fidelity for an unspecified or partial one. Faithful lifting of a source 𝑏 ∶ Src_𝐿 yields the checked 𝑝, elaboration evidence, and a projection recovering 𝑏 when no rewrite was asked for. Host annotations are indexed by 𝑝, so erasing a proof-only annotation is a Σ projection, while compiling a rewritten program needs that rewrite's semantic relation. Source and meaning stay separate and connected.

### 5.5 Source-language semantics as an inductive construction

For an edition and environment parameter 𝜃: Program_𝜃, Well_𝜃(𝑝), Config_𝜃(𝑝), and Step_{𝜃,𝑝} ∶ Config → Label → Config → 𝒰 (5.8). Finite executions are the labeled reflexive–transitive closure; continuing ones use §7.6's clocked guarded processes, with ▷ and clock quantification (44). Behavior is the dependent family of executions and observations, truncated only when the property forgets witnesses. Unspecified evaluation order, scheduling and other nondeterminism use the relation, never an arbitrary successor. Syntax, typing, transitions and observation are data. An encoding is complete relative to a standard when the explicit adequacy equivalence below relates the derivations, and side conditions are part of the rule instances, not prose.

**Typed command signature and guarded interpreter.** The interface is an inductive object parameterized by an explicit signature, not an unspecified `interpret`, and the language's rules are data. Nondeterministic languages get a branching family instead of `next`.

```
record Signature : Type (lsuc l) where
  field Cmd   : Type l
        Reply : Cmd → Type l
record Machine (Σ : Signature) (C A F : Type l) : Type (lsuc l) where
  field next : C → D_Σ(C,A,F)
```

- 𝐷_Σ(𝐶,𝐴,𝐹) = 𝐴 + 𝐹 + 𝐶 + ∑_{𝑐∶Cmd}(Reply 𝑐 → 𝐶) (5.9).
- Proc_Σ(𝐴,𝐹) ≅ 𝐴 + 𝐹 + ▷Proc + ∑_𝑐(Reply 𝑐 → ▷Proc), and unfold(𝛿,𝑠) is done 𝑎 / fault 𝑓 / tau(next(𝛿,𝑠′)) / call(𝑐, 𝜆𝑟.next(𝛿, 𝑘 𝑟)) by the four cases of 𝛿(𝑠) (5.10).
- `next` on the right is the guarded recursive occurrence, with ▷ in its type, not a second machine function. Guarded corecursion is required: an unguarded inductive tree cannot represent indefinitely interacting programs.

```
record Language : Type (lsuc l) where
  field
    Source : Type l
    Well   : Source → Type l
    Input  : (p : Source) → Type l
    Result : (p : Source) → Type l
    Fault  : (p : Source) → Type l
    Config : (p : Source) → Type l
    Commands : Signature
    init : (p : Source) → Well p → Input p → Config p
    step : (p : Source) → Config p → D_Commands (Config p) (Result p) (Fault p)

meaning : (L : Language) (p : L.Source) (w : L.Well p)
        → L.Input p → Proc L.Commands (L.Result p) (L.Fault p)
meaning L p w x = unfold (L.step p) (L.init p w x)
```

An existing language's syntax, typing, initialization and step clauses are values of this one record, and `meaning` is the one host function, by guarded unfolding. A complete formal semantics supplies the fields; the record neither supplies nor validates a missing clause. Languages with choice replace `step` by a relation or branching structure and Proc by its branching analogue. This is essential for C++ unspecified evaluation order, weak memory, environment responses and UB.

**Dependent properties.** With 𝑀 = meaning 𝐿 𝑝 𝑤:

- Property(𝐿,𝑝,𝑤) ∶= ∏_{𝑥∶Input 𝑝}(Proc_Commands(Result 𝑝, Fault 𝑝) → 𝒰)
- Proof(𝐿,𝑝,𝑤,𝑃) ∶= ∏_𝑥 𝑃(𝑥)(𝑀 𝑥)
- Annotated ∶= ∑_𝑃 Proof (5.11)

There is no new source proof judgment; an annotation is a cubical term whose type mentions the program's interpretation. An unchanged file has unchanged semantics, and the proof is a separate dependent object. A transformation transfers properties only through the exact equality or observation-respecting map it supplies, never by assuming every rewrite is valid.

**Exact memory-interaction instance.** Commands Read ∶ Addr → Cmd (reply Byte) and Write ∶ Addr → Byte → Cmd (reply Unit).

```
Stmt ::= Skip | ReadInto(Reg,Addr) | Write(Addr,Expr) | Seq(Stmt,Stmt) | If(Expr,Stmt,Stmt)
Expr ::= Const(Byte) | Reg(Reg) | Xor(Expr,Expr)
step(Skip, ρ, [])          = Done(ρ)
step(Skip, ρ, s :: k)      = Tau(s, ρ, k)
step(Seq(s,t), ρ, k)       = Tau(s, ρ, t :: k)
step(If(e,s,t), ρ, k)      = Tau((if eval(e,ρ)≠0 then s else t),ρ,k)
step(ReadInto(r,a), ρ, k)  = Call(Read(a), λb.(Skip,ρ[r↦b],k))
step(Write(a,e), ρ, k)     = Call(Write(a,eval(e,ρ)), λ_.(Skip,ρ,k))
eval(Const(b),ρ) = b;  eval(Reg(r),ρ) = ρ(r);  eval(Xor(a,b),ρ) = eval(a,ρ) xor eval(b,ρ)
```

Configurations are (stmt, registers, continuation stack). Memory belongs to the handler, which holds 𝜇, answers Read(a) with 𝜇[a], and handles Write(a,b) as 𝜇 ↦ 𝜇[a↦b] returning Unit. There is exactly one owner of mutable memory and the continuation carries none, so each write happens once and the trace is determined by the handler equations. For `Seq(ReadInto(r,a), Write(b,Xor(Reg(r),Const(0x5a))))` with Read(a) = 0x35, the unique finite execution is Read(a), then Write(b,0x6f), then Done. This shows the interpreter reducing an existing-language program to an explicit process; the syntax has not become a host primitive. A proof of the written value is an inhabitant of "the observed Write carries 0x6f under that read response".

**Rule coverage and adequacy as checkable data.** Presenting the rules is distinct from proving that the host faithfully represents their derivations.

- RuleSig ∶= ∑_{Inst}(concl ∶ Inst → Judg) × (prem ∶ Inst → List Judg); All(𝐷,[]) = 1, All(𝐷, 𝐽∷𝐽𝑠) = 𝐷𝐽 × All(𝐷,𝐽𝑠); StdDeriv(𝐽) ∶∶= infer(𝑟, 𝑑⃗) with concl 𝑟 = 𝐽 and 𝑑⃗ ∶ All(StdDeriv, prem 𝑟) (5.12).
- Side conditions live in Inst: an instance exists only with evidence of its conditions, so omitting one visibly changes the signature.
- Adequate(𝐻) ∶= ∏_𝐽 ∑_{𝑓_𝐽∶𝐷_𝐽→𝐻_𝐽} ∑_{𝑔_𝐽∶𝐻_𝐽→𝐷_𝐽} ((∏_𝑑 𝑔(𝑓 𝑑) = 𝑑) × (∏_ℎ 𝑓(𝑔 ℎ) = ℎ)) (5.13). 𝑓 goes by induction on source derivations, each instance with its translated premises becoming a host constructor. 𝑔 goes by induction on host derivations and must account for every constructor proving an encoded judgment. The round trips are by induction.
- If derivations differ only by administrative structure, state the equivalence after the chosen truncation or quotient, never as judgmental equality of raw trees.
- 𝑓 is representation completeness (every signature derivation has a host counterpart); 𝑔 is representation soundness (the host invents nothing). The round trips upgrade these to equivalence of presentations, which matters when proof structure, elaboration choices or diagnostics are transported as data. For derivability alone, prove both implications after truncation. Neither form licenses dropping side conditions.

*External standard boundary.* A prose standard has no pre-existing derivation system, so RuleSig formalizes it, and (5.13) checks signature against host encoding, not signature against every normative sentence. Coverage is recorded by covers_𝐸 ∶ Clause_𝐸 → List(Inst) → 𝒰 (5.13a), for edition 𝐸: which clauses have instances, their implementation parameters, and the justification of each transcription. It is a traceable audit object, not a theorem of prose fidelity; completeness relative to an edition also accounts for every relevant requirement and their interactions. Where an independent formal semantics exists, a derivation correspondence to it can be proved. Abstract-semantics-to-target-execution is §8.2.

### 5.6 A closed source-language transformation: syntax, execution, proof, projection

Fully defined here, and the same dependency pattern as lifting a large language: grammar, meaning, specification, checked proof, unchanged projection.

- Byte = Fin(256), with ⊕ as XOR and arithmetic mod 256. Expr ∶∶= Input ∣ Const(𝑏) ∣ Xor(𝑒₁,𝑒₂), with ⟦Input⟧𝑥 = 𝑥, ⟦Const 𝑏⟧𝑥 = 𝑏, ⟦Xor(𝑎,𝑏)⟧𝑥 = ⟦𝑎⟧𝑥 ⊕ ⟦𝑏⟧𝑥. The interpreter is a structurally recursive cubical term whose computation rules are exactly these (setting: Agda-style dependent programming, 29). Source constructors are data and the interpreter gives them meaning.
- 𝑝 ∶= Xor(Xor(Input, Const 0x5a), Const 0x5a); 𝑃(𝑒) ∶= ∏_𝑥 ⟦𝑒⟧𝑥 ≡ 𝑥. xorCancel ∶ ∏_{𝑥,𝑘} (𝑥⊕𝑘)⊕𝑘 ≡ 𝑥 and 𝜋(𝑥) ∶= xorCancel(𝑥, 0x5a). For independence from any byte library, XOR is bitwise over 8 Booleans with the truth table, and cancellation is an 8-fold product of the 4 Boolean cases. No unproved machine-instruction property is used.
- annotate(𝑝,𝜋) ∶ ∑_{𝑒∶Expr} 𝑃(𝑒); erase(𝑒,𝜌) ∶= 𝑒 and erase(annotate(𝑝,𝜋)) ≡ 𝑝. Neither the AST nor the interpretation changes; there is just one more checked object about the AST. No comment syntax or runtime change is involved.
- cancel ∶ Expr → Expr: cancel(Xor(Xor(𝑒, Const 𝑘), Const 𝑘)) ∶= cancel 𝑒, taking precedence; otherwise Xor(𝑎,𝑏) ↦ Xor(cancel 𝑎, cancel 𝑏), and other constructors unchanged. cancelSound ∶ ∏_𝑒∏_𝑥 ⟦cancel 𝑒⟧𝑥 ≡ ⟦𝑒⟧𝑥 by structural recursion: the first branch is IH then xorCancel, the XOR branch is ⊕-congruence, the rest is refl. On 𝑝 the first branch gives exactly Input.
- preserve(𝑒,𝜌) ∶= (cancel 𝑒, 𝜆𝑥. cancelSound(𝑒,𝑥) ⋅ 𝜌(𝑥)) ∶ ∏_𝑒 𝑃(𝑒) → ∑_{𝑒′} 𝑃(𝑒′), with ⋅ as path concatenation. The direction matters: the new meaning is identified with the old, then the old proof follows. A general 𝑄 is transported along the equality of denotations, not rewritten by hand.

Calculation: ⟦𝑝⟧𝑥 = (𝑥⊕0x5a)⊕0x5a; 𝜋(𝑥) ∶ ⟦𝑝⟧𝑥 ≡ 𝑥; cancel 𝑝 = Input; cancelSound(𝑝,𝑥) ∶ ⟦Input⟧𝑥 ≡ ⟦𝑝⟧𝑥; preserve(𝑝,𝜋) ∶ ∑_𝑒∏_𝑥 ⟦𝑒⟧𝑥 ≡ 𝑥. This is a closed instance of lifting, attachment, transformation and projection. A larger language adds constructors and clauses; these operations gain no new logical foundations.

## 6. Reflective programming and language extension

§5's definitions become operands. Reflection is the language acting on its own descriptions: changing a grammar, an interpreter clause, or the context a received expression is read in changes what later computation does. Here this is directly programmable: interpreter, source language, transformation, and the rationale connecting their behaviors are all represented and operable. Antecedents: Lisp reflection, definitional interpreters, NbE, partial evaluation, staging (3, 5, 12–15). Here they act on typed representations inside the language, with types, contexts, binding and stage respected.

- Code_𝐿(Γ,𝐴) ∶ 𝒰 and eval_𝐿 ∶ Code_𝐿(Γ,𝐴) → Env_𝐿(Γ) → ⟦𝐴⟧_𝐿 (6.1). Quoted code is typed, never a string (typed quotation and readback, 12, 13), and identifies its object language, free variables and type.
- Transformations are 𝑇 ∶ ∏_{Γ,𝐴} Code_𝐿(Γ,𝐴) → Code_𝐿(Γ,𝐴) (6.2), or, when representation or type changes, a dependent output with new type, code and relation. Example, product to vector, for fixed 𝑐: ∑_{𝐵∶Ty_𝐿}∑_{𝑒∶⟦𝐴⟧≃⟦𝐵⟧}∑_{𝑐′∶Code(Γ,𝐵)}∏_𝜌 𝑒(eval 𝑐 𝜌) = eval 𝑐′ 𝜌 (6.3). The last product is the transformation's correctness evidence.
- This is not unrestricted quotation of host expressions at their own stage. Self-interpretation and staging need explicit representations, levels, and termination or guardedness. Quotation, readback and partial evaluation are constructions over code and semantics; no inconsistent universe of all expressions interpreting itself is assumed.
- Specialization: ⟦𝑆(𝐼,𝑝)⟧(𝑥) = 𝐼(𝑝,𝑥) (6.4). The first Futamura projection is an instance (14, 15), and the higher ones specialize the specializer or interpreter under staging assumptions. They follow from the evaluator and substitution laws; they are not foundations.

### 6.1 Typed quotation, semantic readback, and self-application

Code_ℓ(Γ,𝐴) ∶= ∑_{𝑐∶Raw_ℓ} Der_ℓ(Γ,𝑐,𝐴), defined once. For a chosen model, eval_{ℓ,Γ,𝐴} ∶ Code_ℓ(Γ,𝐴) → ∏_{𝜌∶Env_ℓ Γ} ⟦𝐴⟧𝜌.

Val^nf_ℓ(Γ,𝐴) is a reifiable domain of semantic normalization values, with weakening, neutral reflection and readback. It is not the type of all host functions. Its law is reify ∶ Val^nf → Code and reifySound ∶ ∏_𝑣∏_𝜌 eval(reify 𝑣, 𝜌) = denote(𝑣, 𝜌) (6.5).

The typed Kripke NbE schema, with wk ∶ Γ → Γ▷𝐴, ren_𝑟 and neVar₀:

- Π: semantic functions are natural under renamings; reify_Π(𝑓) ∶= lam(reify_{𝐵[var₀/𝑥]}(appVal(ren_wk 𝑓, reflect_𝐴(neVar₀)))). Reflecting a neutral 𝑛 gives, at 𝑟 ∶ Γ → Δ and 𝑎 ∶ Val(Δ, 𝐴[𝑟]), reflect_{𝐵[𝑟, reify 𝑎/𝑥]}(neApp(ren_𝑟 𝑛, reify 𝑎)).
- Σ: reflect(𝑛) ∶= (𝑎, reflect_{𝐵[𝑎/𝑥]}(neSnd 𝑛)) with 𝑎 ∶= reflect_𝐴(neFst 𝑛); reify(𝑎,𝑏) ∶= (reify_𝐴 𝑎, reify_{𝐵[𝑎/𝑥]} 𝑏).
- PathP: reify 𝑝 ∶= pathLam(𝑖. reify_{𝐴(𝑖)}(pathApp(𝑝,𝑖))); reflect 𝑛 ∶= 𝑖 ↦ reflect_{𝐴(𝑖)}(nePathApp(𝑛,𝑖)), with the 𝑖 = 0,1 boundaries inherited.
- The full cubical domain also interprets faces, composition and Glue by its Kan operations. These clauses cover the indicated fragment, and their meaning is fixed by the chosen model, not by an undefined universal quotation.
- normalize_{Γ,𝐴}(𝑡) ∶= reify(eval(𝑡, reflectContext Γ)), with reflectContext built from Γ's neutral variables. The fundamental lemma is by induction on typed syntax, and its binder case is exactly the weakening/application calculation.

**Typed specialization and self-application.** 𝑆_{𝐴,𝐵} ∶ Code(∅, 𝐴→𝐵) → Code(∅,𝐴) → Code(∅,𝐵) with eval(𝑆(𝑞,𝑟)) = eval(𝑞)(eval 𝑟).

- First projection: for an interpreter 𝑒 ∶ Code(∅, 𝑃→𝐷→𝑂) and 𝑞_𝑝 ∶ Code(∅,𝑃), compiled(𝑒, 𝑞_𝑝) ∶= 𝑆_{𝑃,𝐷→𝑂}(𝑒, 𝑞_𝑝), and eval(compiled)(𝑑) = eval(𝑒)(eval 𝑞_𝑝)(𝑑) (6.6).
- Second projection: 𝐴 ∶= Code(∅, 𝑃→𝐷→𝑂), 𝐵 ∶= Code(∅,𝑃) → Code(∅, 𝐷→𝑂), and 𝐺(𝑒)(𝑞_𝑝) ∶= 𝑆(𝑒, 𝑞_𝑝). At a stage representing this 𝐺, supply 𝑞_𝐺 ∶ Code(∅, 𝐴→𝐵). Because 𝐴 is a syntax datatype, lift_𝐴(𝑒) ∶ Code(∅,𝐴) with eval(lift 𝑒) = 𝑒. Then compilerCode(𝑒) ∶= 𝑆_{𝐴,𝐵}(𝑞_𝐺, lift_𝐴 𝑒) and eval(compilerCode 𝑒)(𝑞_𝑝) = 𝑆(𝑒, 𝑞_𝑝) (6.7).
- lift_𝐴 is defined on the represented syntax datatype and its checked constructors, never as 𝐴 → Code(𝐴) for arbitrary semantic 𝐴. The same stage indices apply to further projections, and everything follows from the one specialization law plus the supplied code for 𝐺.

**Checked syntax and its interpretation.** Code_ℓ's first projection is raw syntax and its second a typing derivation. Raw syntax, derivations, semantic values and evaluator graph nodes are distinct objects connected by operations, never by implicit identification.

- An elaborator maps raw declarations to checked syntax or a precise failure. eval_{Γ,𝐴} ∶ Code(Γ,𝐴) → ∏_𝜌 𝐴[𝜌].
- Quotation reads values back, with eval(reify_𝐴 𝑣, 𝜌) ≡ denote(𝑣, 𝜌) at the stage and type where it is defined, returning neutrals when blocked on a free variable. It is stated only on the reifiable domain; it is not total quotation of external states, infinite processes, or a universe containing its own evaluator.
- The checker decides the specified decidable typing and conversion judgments of the selected core. That it exists is a separate theorem about the core, not a consequence of inductive syntax. A definition of check does not give check(c,A) = yes → Der(Γ,c,A) without a soundness argument. A universe-polymorphic Code is not a universe of its own unrestricted semantics, and the staging and universe restrictions are explicit.

**Substitution on checked syntax.** 𝜎 ∶ Δ → Γ respects dependencies, and ⟦𝑡[𝜎]⟧𝜌 ≡ ⟦𝑡⟧⟦𝜎⟧𝜌. This one equation underlies application, instantiation of dependent types, partial evaluation, theorem instantiation and staging. A type argument, a known number, an interpreter's source program and a deployment configuration are all supplied through it.

**Transformations of checked syntax** return ∑_{𝑐′∶Code(Γ,𝐴)} Path_𝐴(⟦𝑐⟧, ⟦𝑐′⟧). The path is refl when both compute alike, an instantiated theorem, or a composite. A type-changing transformation returns its equivalence, transport or other relation; it does not pretend input and output share a type. Rewriting with 𝑝 ∶ 𝑢 ≡ 𝑣 under 𝐶[−] is cong 𝐶 𝑝 ∶ 𝐶[𝑢] ≡ 𝐶[𝑣], and rewrites compose by concatenation. The rewriting program is not trusted as an oracle; its evidence is type-checked. That is reflective programming operationally: a declaration is data, and its meaning, its transformations and their evidence are terms whose results feed further computation.

**Typed stages and checking.** Three things conflated as "reflection" have different types. At a fixed stage and level:

- Raw_ℓ are codes of an object fragment; Der_ℓ(Γ,c,A) is typing as evidence; Val_ℓ(A,ρ) is semantic interpretation. Code_ℓ is the already-defined checked representation, whose raw part and derivation are projections, not another code type.
- eval_ℓ ∶ Code_ℓ(Γ,𝐴) → (𝜌 ∶ Env_ℓ Γ) → Val_ℓ(𝐴,𝜌) is a typed map, not a cast.
- A checker takes raw syntax, not a Code already holding the derivation it should establish. One total interface for a decidable fragment is check_ℓ(Γ,𝑐,𝐴) ∶ Dec(Der_ℓ(Γ,𝑐,𝐴)), with Dec 𝑃 = 𝑃 + ¬𝑃; an elaborator may instead return a checked term, a type and a structured diagnostic. A metacircular check is a program; that it accepts exactly the intended judgments is a distinct theorem about its construction and the calculus.
- Readback is reify on Val^nf, with law (6.5). For open terms the domain has neutrals with their contexts, and quotation returns code in that context.
- Code and evaluator are indexed by object stage, and metaprograms live in an enclosing stage. A code for the evaluator is a code at a suitable enclosing stage, not an inhabitant of U : U. So a metacircular interpreter is representable without identifying the universe with its own total truth predicate. The separation identifies exactly what substitution, normalization, proof construction and self-application act on: all four are programs, none an untyped operation on arbitrary objects.

**Open terms and delayed work.** Open terms evaluate as far as their structure allows. An eliminator blocked on a variable yields a neutral holding the eliminator and its arguments, and a later substitution exposing a constructor resumes it; this is neither an error nor a placeholder. Dimension substitution can select a face, change which boundary is available, or expose a composition rule, so the face context bears on a residual's validity. Branches whose premises fail must not be forced merely to build a cache key or inspect a system, and dimension and face payloads keep their distinct algebraic meanings. Delayed substitution records the operation and its inputs until needed. Call-by-need keeps one shared producer instead of copying per use. Readback and quotation should preserve sharing where possible, since flattening the graph to a tree can destroy the benefit before the next operation.

**Reuse of reductions.** Given 𝑑 ∶ Γ;𝜑 ⊢ 𝑡 ⟶* 𝑢 ∶ 𝐴 (6.8) and a 𝜎 satisfying context and faces, substitution stability gives 𝑡[𝜎] ⟶* 𝑢[𝜎]. A runtime may reuse the reduction and must then resume if 𝑢[𝜎] has newly exposed redexes; a cached whnf of 𝑢 is not a whnf of every instance. A reusable computation records:

- the producer's structure and free dependencies;
- context restrictions;
- its result or residual;
- the instantiating transformation.

A caller's route to the producer is not an input unless it changes one of these. Four operational objects are distinguished, without being made into new semantic categories: the request occurrence, the producer it refers to, the producer's reusable result, and delivery or adaptation to the occurrence. Many occurrences may share a producer, and reuse does not make delivery free. substStep(𝜃, 𝑟) is defined by induction on the reduction constructor, beta to substituted beta and congruence to congruence: substStep(𝜃,𝑟) ∶ subst(𝜃,𝑡) ⟶* subst(𝜃,𝑢) (6.9). The closure allows administrative steps from the substitution representation, with endpoints fixed by §5.1's equations.

### 6.2 Specialization as a program over the language's own definitions

The machine evaluates terms; the checker inspects the values; the language operates on representations. `Code` represents term constructors, and `eval-g : Code → Value` interprets closed codes, environment-indexed for open ones. `%view` exposes an evaluated term's head constructor and constituent values. The checker works on that view, and its inference and conversion are themselves machine-evaluated terms. Specialization uses the same codes and is accepted by conversion or a checked path, never by agreement of sample outputs. The bootstrap `Code` exists before the checker can certify its definition, so it is distinct from §6.1's intrinsically checked family, and programs may define smaller checked syntax types for their domains. The specializer works on the bootstrap representation, and its result is checked at its declared type before admission.

```
specialize : Code → Sched → Lst Rule → Code → Code × Code
specialize Tc sch rules c =
  let c0 = normalize-held (sched-holds sch) Tc c
      r = rewrite-all (sched-rewrites sch) Tc rules c0
  in (fst r , path-code (snd r) c0)
```

- `Tc` represents the type and `c` the code. `sch` says which definitions stay unknown, how arguments are staged, and the rewrite budget. The result is new code plus code for its justification.
- Normalization yields something definitionally convertible to the input, while theorem rewriting yields an explicit path. With no rewrite, path-code is refl; with rewrites, compose-p builds the composite and `#jit` asks the checker to accept it. A wrong theorem application can't be admitted just because the output is well formed.
- `normalize-held` makes partial information operational: it replaces the held definitions with bound neutral variables, abstracts, normalizes the lambda, and restores the held references in the residual. Unheld definitions compute and held ones stay unknown. This is partial evaluation of the actual program, not of an independently maintained symbolic model.
- `apply-now` stages parameters over codes, walking leading binders: an `st-now` binder receives a represented argument from the environment, and an `st-run` binder stays a variable under the residual lambdas.

```
residual : Sched → Code → Lst Rule → Code → Lst Code → Code × Code
residual sch Tc rules c env =
  specialize Tc sch rules (apply-now (sched-stages sch) c env)
```

The static environment is `Lst Code`, so inputs are representations and no 𝐴 → Code 𝐴 is assumed. Example: λ x y → (x+7)+y with x static at code for 5 residualizes to the meaning λ y → 12+y, assuming the word or ℕ operations and their reduction rules; the mechanism is binder-indexed. Two relations are involved: apply-and-normalize preserves meaning by computation and conversion, and theorem rewriting carries a checked path. Applied to a represented interpreter, this specializes it to a source program; applied to the specializer, it gives the Futamura constructions [14, 15]. No compiler primitive is needed, since interpreter, specializer, rules and outputs are all terms and data.

**Staged self-application: the three projections.** 𝐶(𝐴) is closed checked code, 𝑞_𝐴(𝑎) ∶ 𝐶(𝐴) a representation of a value, and 𝑞_{𝐶(𝐴)}(𝑐) the code package as data, not its evaluation. 𝑆_{𝐴,𝐵,𝐷} ∶ 𝐶(𝐴→𝐵→𝐷) → 𝐶(𝐴) → 𝐶(𝐵→𝐷) with ⟦𝑆(𝑒, 𝑞_𝐴 𝑎)⟧(𝑏) = ⟦𝑒⟧(𝑎)(𝑏) (6.F1), a path of meanings where required (the notation suppresses the witness, not its existence).

- (6.F2) 𝑟_𝑝 = 𝑆_{𝑃,𝑋,𝑌}(𝑒, 𝑞_𝑃 𝑝) ∶ 𝐶(𝑋→𝑌), ⟦𝑟_𝑝⟧(𝑥) = ⟦𝑒⟧(𝑝)(𝑥).
- (6.F3) With 𝐴₀ = 𝐶(𝑃→𝑋→𝑌), 𝐵₀ = 𝐶(𝑃), 𝐷₀ = 𝐶(𝑋→𝑌) and 𝑠 ∶ 𝐶(𝐴₀→𝐵₀→𝐷₀) presenting 𝑆_{𝑃,𝑋,𝑌}: 𝑐_𝑒 = 𝑆_{𝐴₀,𝐵₀,𝐷₀}(𝑠, 𝑞_{𝐴₀} 𝑒), ⟦𝑐_𝑒⟧(𝑞_𝑃 𝑝) = 𝑆_{𝑃,𝑋,𝑌}(𝑒, 𝑞_𝑃 𝑝). Static input is the interpreter code; dynamic input is the program code.
- (6.F4) With 𝐴₁ = 𝐶(𝐴₀→𝐵₀→𝐷₀), 𝐵₁ = 𝐶(𝐴₀), 𝐷₁ = 𝐶(𝐵₀→𝐷₀) and 𝑠′ at that type: 𝑔 = 𝑆_{𝐴₁,𝐵₁,𝐷₁}(𝑠′, 𝑞_{𝐴₁} 𝑠), ⟦𝑔⟧(𝑞_{𝐴₀} 𝑒) = 𝑐_𝑒.

These are typed instances of one residualization equation, not three compiler operators, and 𝑞_{𝐴₀}(𝑒), 𝑞_{𝐴₁}(𝑠) quote finite code as data at the next stage, with no unrestricted 𝐴 → 𝐶(𝐴). The equalities are semantic. Actually eliminating interpreter dispatch also depends on the specializer reducing static constructors while keeping unknowns, the same distinction apply-now and normalize-held exhibit. 𝑆(𝐸,𝑝), 𝑆(𝑆,𝐸) and 𝑆(𝑆,𝑆) are valid abbreviations only with these code-level arguments and stage indices. §9.2's universal realization applies to specializer, interpreter, compiler and residuals once evaluator adequacy and its realization hold. The first projection on a represented evaluator and a static program may remove dispatch on that program, but the equation alone guarantees neither that dispatch disappears nor that the residual lands in the directly compiled fragment.

**Finite specialization of an evaluator.** The residual has the same meaning; whether specialization terminates is a separate question.

- A sufficient condition for an offline, memoizing specializer: the reachable static configuration shapes are finite, and static reduction between memoization points terminates.
- A fixed program has finitely many code positions, but an unbounded continuation stack or a recursively varying static argument doesn't have finitely many static states. So binding-time analysis keeps the stack and runtime data dynamic, and uses code positions plus a finite set of frame constructors as static dispatch.
- Precisely: memoized shapes 𝑄 ⊆ Pos(𝑐) × FrameKind × StaticEnv, with case reduction 𝛿 ∶ 𝑄 → ResidualCase(𝑄). If 𝑄 is finite and every statically reducing call reaches a memoization point, worklist expansion reaches a fixed point after at most |𝑄| new shapes. Each 𝑞 is one residual control location and the dynamic stack is a runtime value, not part of 𝑄. Edges come from 𝛿(𝑞), including dynamic conditionals and residual calls to the general evaluator when code becomes dynamic.
- This gives a finite residual graph. It does not make every clause first-order direct instructions, and the blocks don't satisfy the structural fold's compositionality without the separate congruence proof. It is a sufficient construction under these binding-time premises, not a theorem that every specializer terminates or every residual has a finite CFG.

Non-static code may keep a call to the represented evaluator, which is an ordinary term natively realized by §9.2. Static versus dynamic belongs to the chosen specialization, not to what the language can express. Raw syntax can run as a guarded process without asserting normalization, and the typed quotation equations still need their stages and checked code. Guarded divergence doesn't make divergent terms converge. Self-reference keeps its limits: Church–Turing undecidability and the diagonal arguments of Lawvere, Gödel and Tarski (1, 2, 33–35) concern exact representability and logical premises, and reifying programs as data bypasses none of them. Concretely, runRaw ∶ Raw → Env → Proc_Σ(Value + Fault) applied to a code of runRaw is a guarded execution, not a total normalizer or a universe-valued truth predicate. It needs runRaw's code to be representable and its recursion guarded. By contrast 𝑞_{𝐴₀}, 𝑞_{𝐴₁} and lift_𝐴 in (6.F2)–(6.F4) inhabit checked, explicitly staged code types, and no same-level quotation of semantic values follows from the raw interpreter.

Two relations stay distinct. Conversion or a checked path relates the specialized term to its source. The meaning of emitted bytes under the target instruction semantics is the separate source-to-machine construction: the C route and its downstream compilation relation (§9.4), whose obligations are stated independently of source-level conversion. Appendix C keeps a separate direct-byte example.

### 6.3 Reflection applied to a language definition itself

Grammar, typing and interpreter are expressions. Extend §5.2 with mul ∶ Expr Γ → Expr Γ → Expr Γ, ⟦mul(𝑢,𝑣)⟧𝜌 = ⟦𝑢⟧𝜌·⟦𝑣⟧𝜌 (6.10). Transformations now act on the interpreter, not only on its programs. It can be made to return a value plus an expression of its symbolic dependence on the input, or to evaluate over another semiring.

For the semiring version, replace ℕ by a semiring 𝑅 with num 𝑛 ↦ the canonical ℕ → 𝑅. This gives ⟦−⟧_𝑅 ∶ Expr Γ → (Γ → 𝑅) → 𝑅 (6.11). With 𝑅 = ℕ[𝑋ᵢ ∣ 𝑖 ∈ Γ] and 𝑖 ↦ 𝑋ᵢ, evaluation ev_𝜌 ∶ ℕ[𝑋ᵢ] → ℕ is a homomorphism (6.12), and ev_𝜌(⟦𝑒⟧_{ℕ[𝑋]}(𝜆𝑖.𝑋ᵢ)) = ⟦𝑒⟧_ℕ(𝜌) (6.13) by structural induction. So the interpretation changes while the algebraic structure is preserved; this is the elementary basis of symbolic execution, using only the semiring laws and the interpreter over them. Branching, state, exceptions or world requests need their own structure preserved, and a semiring homomorphism is not enough. The object under study determines the needed structure, not a list of subsystems.

**Transforming the interpreter definition.** The interpreter is the fold of the signature Expr(Γ) = var(Γ) + num(ℕ) + plus(Expr²) + mul(Expr²). The notation names four constructor families; it is not a set equation.

- Clauses(𝑅) ∶= (Γ → 𝑅) × (ℕ → 𝑅) × (𝑅×𝑅 → 𝑅) × (𝑅×𝑅 → 𝑅) and fold_𝑅 ∶ Clauses(𝑅) → Expr Γ → 𝑅, applying the matching component at each constructor (6.14). The environment 𝜌 is the record's first component.
- For a semiring, 𝐼_𝑅(𝜌) ∶= (𝜌, 𝜂_𝑅, +_𝑅, ·_𝑅). The map (𝑅, 𝜌) ↦ 𝐼_𝑅(𝜌) produces interpreter definitions, not input programs, and fold_𝑅(𝐼_𝑅 𝜌, 𝑒) = ⟦𝑒⟧_𝑅(𝜌) by the four constructor equations.
- For a homomorphism ℎ ∶ 𝑅 → 𝑆 with 𝜌_𝑆 = ℎ ∘ 𝜌_𝑅: ℎ(fold_𝑅(𝐼_𝑅 𝜌_𝑅, 𝑒)) = fold_𝑆(𝐼_𝑆 𝜌_𝑆, 𝑒).
- Record and fold are terms. Syntactic inspection or rewriting goes through their represented code under §6.1's quotation conditions; the semantic algebra alone gives no code.

Interpretation continues across inputs: one input can change the bindings or clauses used for the next, expressed by these interpreters and §7.6's processes with no further semantics. A received expression is read under the definitions selected at that moment, never under a meaning intrinsic to its bytes. Specialization uses the available context, and §10.5's dependency and continuation relations say which earlier computations survive when the context changes.

### 6.4 A source language can inherit the mathematics of its host

The cubical representation of 𝑝 ∶ Tm_𝐿(Γ,𝐴) is a term, so functions can inspect it, build another term, and prove a dependent relation between their meanings, without extending 𝐿's runtime instruction set.

- A source annotation is a typed host expression whose context includes the exact declaration it annotates (with Ty_𝐿(Γ) when types depend on context). The association is a dependent index, not a textual convention. Annotated_𝐿(Γ,𝐴) ∶= ∑_{𝑝∶Tm_𝐿(Γ,𝐴)} Property_𝐿(𝑝) (6.15). Properties range from postconditions to statements about all executions, and the first projection recovers the source.
- Annotations may contain host paths, equivalences, HIT objects and syntax transformations, with full host power because they are host terms. Their association is a type-indexed relation, not a separate proof language.
- Transform_𝐿(Γ,𝐴,𝑝) ∶= ∑_{𝐵∶Ty_𝐿 Γ}∑_{𝑞∶Tm_𝐿(Γ,𝐵)} Relation_𝐿((𝐴,𝑝),(𝐵,𝑞)) (6.16), dependent in the fixed 𝑝. The relation is defined per case: an equivalence with its action on meanings, a refinement as an implication between observations, or a path of syntax trees for exact preservation. Transformations need not share a relation, and the program's semantics supplies the object compared.
- Source-to-source projections are functions:
  - new notation elaborates to ordinary constructs, with the law proved by induction;
  - a runtime-free annotation erases as a first projection;
  - a runtime-changing extension explicitly defines the changed expression.

**A conservative extension, calculated.** erase_{Γ,𝐴} ∶ Tm_{𝐿⁺}(Γ,𝐴) → Tm_𝐿(Γ,𝐴) (6.17) sends withProof(𝑝,𝜋), with 𝜋 ∶ Property_𝐿(𝑝), to 𝑝, and is homomorphic on the original constructors. ⟦𝑥⟧_{𝐿⁺} ∶= ⟦erase 𝑥⟧_𝐿, and ∏_𝑥 ⟦𝑥⟧_{𝐿⁺} = ⟦erase 𝑥⟧_𝐿 (6.18) by recursion on the extended syntax.

- HIT or dependent host expressions may sit in 𝜋 or in a staged transformation of 𝑥 without becoming instructions of 𝐿.
- If a staged 𝑇 produces 𝑥′, the emitted source is erase(𝑥′), and 𝑇's semantic claim is a separate term relating ⟦erase 𝑥⟧ and ⟦erase 𝑥′⟧. (6.18) says only that annotations erase; it certifies no arbitrary rewrite.

One typed statement thus recovers annotation, elaboration, projection and proof attachment. The same host terms act on the language definition: add a grammar constructor, supply its clause, derive a family of transformations, and prove things about the resulting language. That is unrestricted language extension concretely: the source language is an object under full host power, not a privileged syntax boundary.

### 6.5 Shared evaluation and partial information

Sharing belongs to the represented computation: occurrences refer to one producer, and dependent consumers request its result. A producer may be unfinished, in weak-head form, or complete, distinguished by representation and transition rules. Sharing changes the dependency graph (local graph reduction: interaction nets, 70), never typing, substitution or equality. Open environments yield residuals. In 𝑡(𝑥,𝑦) = fst(𝑥+3, 𝑦), the projection reduces to 𝑥+3 without touching 𝑦, and 𝑥 ∶= 5 then gives 8. This is one reduction relation under successive substitutions, not a second semantics for partial evaluation. The action of substitution on reduction evidence is (6.9), and retained dependencies say when that evidence applies.

## 7. Programming with machines and operating systems

Interpreters don't end at values: they issue commands, suspend, interact, and change the world later observations happen in. Machines and OSes are such interacting processes, with memory, event and device semantics (C++ memory-model literature and temporal specification, 20, 21, 38). A processor is one specified evolving system, with architectural state, transitions, observations and device interaction, which circuitry realizes under a stated physical correspondence. It is reasoned about like any process. A program may contain the machine's interpreter, and the program building the machine's executable can itself be interpreted and transformed.

- Byte = Fin(256), words Fin(2^𝑤), Mem ∶= Addr ⇀ Byte (7.1). This toy model is insufficient for C++ provenance, lifetime, allocation, atomicity and object representations, which are extra relations when C++ is the subject.
- A single core: State ∶= Registers × Memory × PC × DeviceState (7.2) and step ∶ State → Event → State → 𝒰 (7.3), or a next-state function when deterministic, with the world through labeled transitions and handlers. Single-instruction execution, interrupts, MMIO, context switches, page tables, schedulers and isolated processes are all constructions over this.
- Machine instructions need not be primitives of the cubical language, and the cubical language need not run natively to express machine semantics.

Example: summing 𝑛 u64 words with state 𝑎, 𝑖, 𝑠 has the invariant 0 ≤ 𝑖 ≤ 𝑛 ∧ 𝑠 = ∑_{𝑗<𝑖} 𝑎ⱼ (mod 2⁶⁴) ∧ Unchanged(𝑎) (7.4). Initialization establishes it at 𝑖 = 0, the body preserves it (add mod 2⁶⁴, increment), and exit at 𝑖 = 𝑛 gives the sum. With real loads, the proof also needs validity, alignment, lifetime, aliasing and the source semantics of loads. Machine facts and higher reasoning are one discipline: machine and proof are objects of one language, and their relation is expressible in it.

### 7.1 Memory is more than an address-to-byte function

(7.1) suffices for a toy load/store machine, not for faithful C++. Real memory state distinguishes allocation identity, extent, object lifetime, effective/dynamic type, initializedness, provenance and current bytes. A pointer is not an integer: dereference can depend on which allocation produced it and whether the object is alive. Load ∶ MemState → Pointer → Value → MemState → 𝒰 (7.5): the state is unchanged for ordinary loads, while volatile and atomic accesses add events or relations. Writes relate old state, pointer, value and new state, with clauses given by the represented language and memory model. "Never accesses outside the buffer" is a proposition about source semantics and its allocation/lifetime relations. A sanitizer instruments one executable and watches particular runs. Both are constructions, and neither defines memory safety.

### 7.2 Concurrency requires relations among events

𝑋 = (𝐸, tid, kind, loc, val, po, rf, mo) (7.6). A memory model is Allowed(𝑋). Races, happens-before and atomic orderings are properties of these relations. Concurrent compiler correctness relates admitted target executions to admitted source executions; per-instruction matching fails when reordering and weak memory are observable. "Thread" is not a primitive: threads and synchronization are objects with exact rules wherever the modeled system has them.

Concrete SC: Allowed_SC(𝑋) is the type of total orders <_𝑋 on 𝐸 extending po in which every read sees the latest preceding write to its location, or the initial value. This is a complete memory-model predicate, not C++'s weak one; C++ atomic and race theorems use the chosen C++ execution axioms in its place, with event structure and observations still constructions.

### 7.3 Continuing computation as a definition

Guarded recursion, transition relations, free-monad trees and coalgebras represent continuing computation with different observations and proof principles. External labels become internal interactions when a handler is composed. A representation change needs the equality, bisimulation or refinement for the selected behavior, established separately. §7.6 computes one composition.

### 7.4 Parallel processors and accelerators are other transition systems

CUDA (32) is the concrete model; the concern here is state, events and observable execution. A kernel works over indexed work items with address spaces, synchronization and numeric operations, and its meaning is a relation on global/local state plus an order on memory events. A tensor is 𝑇 ∶ Fin(𝑛₁) × ⋯ × Fin(𝑛ₖ) → 𝐴 (7.7), and a layout maps indices to addresses. Dimension permutation, tiling and packing are representation changes whose preservation laws are equations on the value at each logical index. Their costs depend on the target's memory hierarchy and execution model; correctness and performance are properties of the chosen representations and observations. Autodiff is a transformation of the construction, governed by the chain rule and the numeric semantics, and a GPU compiler can realize either a program or its derivative. A derivative is not a GPU operation and an accelerator is not a differentiation operation; they compose when types and laws allow.

### 7.5 An operating system and its guests form one program

A kernel 𝐾 is an open program whose inputs are syscalls, interrupts, device events and process requests, and each guest 𝑃ᵢ is a program. Scheduler, address-space and device handlers interpret their interactions, giving the joint computation Host(𝐾; 𝑃₁…𝑃ₙ) ∶ Proc_{Σdevice}(𝐴) (7.8), an abbreviation for repeated typed composition.

- The kernel can be programmed and proved as an open object quantified over all admitted guests, then specialized by composing particular ones.
- An isolation invariant can be stated for every permitted kernel transition and every pair of protected address spaces. It is preserved under composition when guests meet the interface premises, and it holds for the joint running program, page-table, context-switch and fault transitions included.
- Composing hosts with communication handlers gives a fleet. The kernel boundary is an interface relative to the selected component, and composing the environment internalizes more without changing the language.

### 7.6 Composition of programs with open interactions

Sig ∶= ∑_{𝐶∶𝒰}(𝐶 → 𝒰), with Cmd = 𝜋₁ and Resp(𝑆,𝑐) = 𝜋₂ 𝑆 𝑐.

- Terminating processes as a Church-encoded free monad: Proc_ℓ(𝑆,𝐴) ∶= ∏_{𝑅∶𝒰ℓ}(𝐴 → 𝑅) → (∏_{𝑐}(Resp 𝑐 → 𝑅) → 𝑅) → 𝑅, with ret, cmd and bind as terms.
- The π-calculus (67) is a different treatment of communication and changing connectivity; this handler is defined by its signature and interpretation. A handler 𝑆 → 𝑊 maps each 𝑆-command to a 𝑊-process with the original response type. Handling internalizes those interactions and leaves the rest open, so what counts as "environment" depends on the composition examined.
- The inductive tree, in the line of Coq interaction trees (39): Step(𝑆,𝐴) ∶∶= sdone(𝑎) ∣ scmd(𝑐, Resp 𝑐 → Step(𝑆,𝐴)).

```
o-handle hd (sdone a) = sdone a
o-handle hd (scmd c k) = o-bind (hd c) (λ r → o-handle hd (k r))
```

o-idh c = o-issue c and o-comph h1 h2 c = o-handle h2 (h1 c). The handler identity and composition laws are paths built by induction: o-handle-id is refl at return and congruence over the handled continuation at commands, and o-handle-bind uses o-bind associativity at commands. Handler composition is a computed property of these programs, not semantics imposed by the host.

The Church form needs explicit naturality to recover extensional equality from equality of reified trees. o-NaturalAt says each interpretation agrees with folding its tree; o-natural-path takes naturality witnesses for p and q plus `to-step p ≡ to-step q` and returns p ≡ q. The hypotheses matter: equal reifications are not assumed to identify arbitrary nonparametric inhabitants.

Distributed example: with node behaviors Beh M St, joint node-plus-network state FState M St, and environmental choices EnvS, `fleet-proc : Beh M St → Nat → FState M St → Proc EnvS (FState M St)` (levels and implicits suppressed) requests the next environmental choice, computes fleet-step, and recurses for the given number of steps. The fleet is one process over its remaining environment (precedents: actors, Kahn networks, 55, 56). Nodes and interactions stay individually programmable, and FState, Pkt and EnvS are names this program defines, not primitives.

**Guarded interaction and a two-process cut.** Continuing processes live in clocked cubical type theory: Proc^𝜅_Σ(𝐴) ∶= fix 𝑋. (𝐴 + ▷^𝜅𝑋 + ∑_𝑐(Resp 𝑐 → ▷^𝜅𝑋)) and Proc_Σ(𝐴) ∶= ∀𝜅. Proc^𝜅_Σ(𝐴), with return, silent delay, and command plus response-indexed delayed continuation.

- bind: bind(return 𝑎, 𝑓) = 𝑓 𝑎; bind(delay 𝑝, 𝑓) = delay(laterBind(𝑝,𝑓)); bind(call(𝑐,𝑘), 𝑓) = call(𝑐, 𝜆𝑟. laterBind(𝑘 𝑟, 𝑓)). laterBind applies the delayed recursive function to the delayed process and next(𝑓) via later-application; it does not eliminate an arbitrary later value.
- A handler answering a command immediately inserts a silent step before resuming, which keeps guardedness even for unbounded streams of immediately answered commands.
- These belong to a clocked extension, not to unextended CCHM.

Two continuing processes with full local configurations 𝐶_𝑃, 𝐶_𝑄 (continuations included), message types 𝑀_{𝑃𝑄}, 𝑀_{𝑄𝑃}, and FIFO queues 𝑢, 𝑣 give state 𝑆_{𝑃∥𝑄} = 𝐶_𝑃 × 𝐶_𝑄 × List 𝑀_{𝑃𝑄} × List 𝑀_{𝑄𝑃}. Transitions, plus their symmetric 𝑃/𝑄 cases:

- local: 𝑝 →τ 𝑝′ gives (𝑝,𝑞,𝑢,𝑣) → (𝑝′,𝑞,𝑢,𝑣);
- send: 𝑝 →!𝑚 𝑝′ gives (𝑝′,𝑞,𝑢++[𝑚],𝑣);
- receive: 𝑞 →?𝑚 𝑞′ takes (𝑝,𝑞,𝑚∷𝑢,𝑣) to (𝑝,𝑞′,𝑢,𝑣);
- external: 𝑝 →𝑒 𝑝′ is a visible 𝑒-step of the composite.

From an empty 𝑃→𝑄 queue, with 𝑁 sends and 𝐷 receives, the invariant is 𝑁 = 𝐷 + |𝑢|, hence 𝐷 ≤ 𝑁: a send raises 𝑁 and |𝑢|, a receive raises 𝐷 and lowers |𝑢|, and other steps fix all three. This is a property of the composite, not an assumption about either process. Trace projection and reconstruction follow by induction over the same cases, under FIFO matching and scheduling constraints. Fairness and eventual delivery do not follow. A lossy, duplicating or reordering network handler changes the state and the invariant. The queue is a chosen interaction cut, not a privileged boundary.

**Native execution of a continuing process.** §9.3's finite block ends at a postcondition; a continuing computation is observed as a guarded process. For code image 𝐶 and admitted state 𝑠, machProc_𝐶(𝑠) is return(exit 𝑠) if terminal, call(𝑞, 𝜆𝑟. ▷machProc_𝐶(resume(𝑠,𝑟))) if syscall(𝑠) = 𝑞, and delay(▷machProc_𝐶(step_𝐶 𝑠)) otherwise (7.6a), using the constructor's delayed continuation. Invalid instruction, trap and resource failure are distinct terminal observations or environment commands per the target semantics, not a hidden fourth case. A source handler is ℎ ∶ ∏_{𝑐∶Cmd(Σsrc)} Proc_{Σsys}(Resp(Σsrc, 𝑐)), and ℎ* is the guarded extension of handler composition to source processes.

Termination-sensitive weak correspondence: ignoring finite internal steps must never identify an infinitely silent target with a returning source.

- Finite-prefix judgments: 𝑥 ⇒^{τ*} 𝑥; 𝑥 →τ 𝑦 and 𝑦 ⇒^{τ*} 𝑧 give 𝑥 ⇒^{τ*} 𝑧; 𝑥 ⇒^{τ*} 𝑦 and 𝑦 →𝑒 𝑧 give 𝑥 ⇒^𝑒 𝑧 (7.6c), where →𝑒 is a visible return, fault or command/response.
- Head(𝑥) ∶= ∑_{𝑒,𝑧}(𝑥 ⇒^𝑒 𝑧). Returns and faults are terminal heads; a command head carries a continuation per permitted response.
- Div_τ(𝑥) is a coinductive infinite τ-chain. Stuck(𝑥) is nonterminal with neither a τ-step nor a head.
- Sim^𝜅 is the greatest clock-guarded relation with: every head of 𝑥 matched by an equal head of 𝑦 with ▷^𝜅 Sim^𝜅 on successors, and conversely; Div_τ 𝑥 ↔ Div_τ 𝑦; and Stuck 𝑥 ↔ Stuck 𝑦 (7.6d).
- Terminal heads relate their payloads by the specified value relation, with unit as the recursive factor. Command heads quantify dependently over the responses the common handler allows. Sim is the guarded fixed point, with (7.6c) built first (bisimulation background, 57).
- This blocks the unsound proof: a source with a finite return head is not simulated by a silently diverging target, since the heads and the divergence clauses disagree. Divergence is distinguished from deadlock.
- The two-sided form is for visible-behavior equivalence; refinement compilers use one-sided head inclusion plus an independent progress/divergence condition. Fairness and permitted environment responses are premises of the observation model, not consequences of guardedness. If internal delays are observable, use the strong labeled relation.

ContBlock_ℎ(𝑎, 𝑃, 𝑝) ∶= ∑_{𝑏∶Bytes} ∏_𝐶 Contains(𝐶,𝑎,𝑏) → ∏_𝑠 (𝑃(𝑠) × (pc 𝑠 = 𝑎)) → Sim(machProc_𝐶 𝑠, ℎ*𝑝) (7.6b). §9.3's containing-image, decoder, loader and resource premises stay in 𝑃 or the context. For a guarded fixpoint 𝑝 = fix(𝜆self. body self), a target loop realizes one turn by finite blocks and a back branch. If the blocks re-establish 𝑃 and give a guarded step from a delayed hypothesis, the turn proof has type ▷Sim → Sim, and Löb induction closes the back edge. Neither the jump nor the fixpoint establishes the simulation; the one-turn proof, the handler correspondence and the invariant do.

No-resource-failure continuation needs a re-established resource invariant in 𝑃. Example: live bound 𝐵, arena 𝐾 ≥ 2𝐵, two semispaces of 𝐾/2. At each turn boundary Live(𝑠) ≤ 𝐵, roots enumerate every reachable retained value, and Free(𝑠) covers the turn's temporaries. If a turn allocates at most 𝐴 between collections and a collection copies at most 𝐵, then 𝐾/2 ≥ 𝐵+𝐴 suffices for this schedule, with collection and root-update correctness supplied by their machine blocks. TurnSafe ∶ ∏_𝑠 (𝑃 𝑠 → ∑_{𝑠′}(FiniteTurn(𝑠,𝑠′) × 𝑃 𝑠′ × NoResourceFault(𝑠 ⇝ 𝑠′))) and LoopSafe ∶ 𝑃(𝑠₀) → Always^𝜅(NoResourceFault) (7.6e), with Always by guarded recursion over turns: induction covers every finite prefix and the guarded continuation gives ongoing safety under these bounds. The semispace figures are a sufficient example, not a claim that all programs have bounded live state. If the specification admits resource failure as observable, relate that outcome instead; no-failure is an application theorem with its own premises, not a default of guardedness.
