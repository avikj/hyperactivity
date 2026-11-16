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

## 8. Existing programming languages inside the host

A lifted C++ program keeps its standard's distinctions (permitted results, unspecified choices, UB, lifetimes, concurrent events) while becoming available to the host's operations on syntax, meaning and evidence. The references are ISO/IEC 14882:2020 (19) and the formal concurrency and executable semantics of Batty et al. (20). C++'s many historically contingent distinctions are part of the specified process and representable exactly; their number creates no new kind of reasoning. The aim is that existing C++ stays the ordinary source presentation while its actual interpreted constructions are open to the full language.

A complete presentation covers lexing and grammar, name lookup, overload resolution, templates and instantiation, constant evaluation, types and conversions, lifetimes, allocation, exceptions, atomics, concurrency and the behavior categories, and it separates standard semantics from a particular compiler and target. With CppSyntax, CppJudgment and CppExec fixed by standard version and implementation parameters, CppMeaning ∶ CppSyntax → Input → Trace → 𝒰 (8.1). It is a relation on traces when nondeterministic, and UB follows the selected formal account, never an unjustified ordinary fault.

- Annotations are terms on exact constructs; proofs about a body refer to its source semantics, memory effects included; transformations output new source plus a proof of equivalence or refinement. C++ stays the surface notation while the host supplies dependent types, paths, higher metaprogramming and proofs.
- Unlike a comment, an annotation has a type, scope, interpretation and equality relation, and the checker knows which declaration it concerns. Erasure or elaboration back to C++ is a separately defined operation with a separately proved property.
- Safe(𝑝) ∶= ∏_{𝑠,𝑠′,𝑡} CppExec(𝑝,𝑠,𝑠′,𝑡) → MemoryInvariant(𝑠,𝑠′,𝑡) (8.2). A transformation 𝑇 is checked against a relation between original and transformed executions. Dependent properties transport when that relation is an equivalence or path; a refinement needs refinement reasoning, because univalence does not invert a one-way simulation.
- A full formal C++ is a large particular object. Its size is the size of a definition, not a change of foundation, and specialized decision procedures and libraries are terms to compose.

### 8.1 Source annotations are expressions attached to exact objects

For a declaration 𝑑 with elaborated type 𝜏 and body 𝑏: Annotation(𝑑) ∶= ∑_{𝑃∶Sem_𝐿(𝑑)→𝒰} 𝑃(⟦𝑑⟧_𝐿), with ⟦𝑑⟧_𝐿 ∶ Sem_𝐿(𝑑) (8.3). Sem may be a function or a relation on executions, and no single runtime value is assumed. An extended presentation carries paths, Π and proofs beside syntax that has none, and erases them on projection. Erasure is sound only for annotations that don't change runtime meaning. Generating declarations or changing behavior needs elaboration plus a preservation or refinement theorem. These two cases must not be collapsed into one "zero-runtime annotation" claim.

### 8.2 One program through source and target representations

For standard and parameters 𝐿 and build configuration 𝐵, a C++ term 𝑝 is a constituent of the enclosing hyperactivity program, and build_𝐵(𝑝) is the executable representation of that same computation. Source behavior, machine behavior and 𝑃 are constructions about one program linked by checked compilation and execution evidence, not a separately written verification model next to a separately deployed system.

- SourceBeh_𝐿(𝑝,𝑥,𝑡) and TargetBeh_𝐵(𝑝,𝑥,𝑡) are types.
- Conforms_𝐵(𝑝) ∶= ∏_{𝑥,𝑡} TargetBeh → SourceBeh.
- Correct_𝐿(𝑝,𝑃) ∶= ∏_{𝑥,𝑡} SourceBeh → 𝑃(𝑥,𝑡) (8.4).

The two compose to 𝑃 for the artifact: preservation evidence plus the source property, both part of programming the actual computation. A displayed proposition is not a proof term; the compiler and checker build or verify its inhabitant. Source, compiler, loader, machine and all their proofs can be components of the enclosing program. Precisely, TargetBeh_{𝐶,𝑀}(𝑝,𝑥,𝑜) ∶= ‖∑_{𝜏∶Exec_𝑀(load_𝑀(𝐶 𝑝), 𝑥)} obs_𝑀(𝜏) = 𝑜‖ (8.5). A refinement proof maps each observed target execution to an allowed source observation, so a property of all allowed source observations holds for the artifact by composition. Artifact, loader, machine and observation are explicit, and no new notion of meaning is introduced.

### 8.3 Representations, operations, and memory

- Word32 ∶= Fin(2³²), Bytes4 ∶= Fin(4) → Fin(256), encodeLE ∶ Word32 → Bytes4 (8.6), with encodeLE(𝑤)(𝑗) = ⌊𝑤/256^𝑗⌋ mod 256 (8.7) and decodeLE(𝑏) = ∑_{𝑗≤3} 256^𝑗 𝑏(𝑗) mod 2³² (8.8).
- Both round trips hold (8.9), so there is a path of types by ua, and word operations transport to bytes.
- add32(𝑥,𝑦) ∶= (𝑥+𝑦) mod 2³² (8.10); addBytes(𝑎,𝑏) ∶= encodeLE(add32(decodeLE 𝑎, decodeLE 𝑏)) (8.11); decodeLE(addBytes 𝑎 𝑏) = add32(decodeLE 𝑎, decodeLE 𝑏) (8.12), by the round-trip law.

This is a complete representation calculation, but not yet a theorem about a C++ load. That needs C++ semantics fixing which four bytes form a live uint32_t, the alignment and provenance required, little-endianness, and permission under the object model. It is also not a theorem about CPU electronics. Those are further premises, not a different foundation. Source syntax is the same: tokens, typed AST, interpreter closure and machine artifact are related by relations that are not automatically equivalences. Parsing may drop comments, optimization may keep observations but not instruction traces, and compiling unspecified behavior may pick one permitted behavior. Precision is stating which relation holds.

**Enumerating, forgetting labels, preserving graph structure.**

- An enumeration is (𝑛, 𝑒 ∶ 𝐴 ≃ Fin 𝑛), a chosen labeling; Card(𝐴) ∶= ∑_𝑛 ‖𝐴 ≃ Fin 𝑛‖ keeps the number only. Permutations act by 𝜎 ∘ 𝑒.
- 𝑄_𝐴 is the set quotient of enumerations by that action, with points inc(𝑛,𝑒), paths inc(𝑛,𝑒) = inc(𝑛, 𝜎∘𝑒), and set truncation. Cardinality uniqueness equates the numbers, and at the common number the equivalences differ by a permutation. So 𝑄_𝐴 is a proposition, and the recursor gives 𝑄_𝐴 ≃ Card(𝐴): forward forgets the enumeration, reverse eliminates truncation into 𝑄_𝐴. A chosen enumeration is not thereby identical to a cardinality.
- Graph_𝑛 ∶= Fin 𝑛 → Fin 𝑛 → Bool with (𝜎⋅𝐺)(𝑖,𝑗) ∶= 𝐺(𝜎⁻¹𝑖, 𝜎⁻¹𝑗). Unlabelled_𝑛 has incl(𝐺), paths incl 𝐺 = incl(𝜎⋅𝐺), and set truncation. A set-valued invariant descends exactly when supplied with the relabeling paths. This deliberately forgets automorphism paths.
- To keep them: GraphSym_𝑛 ∶= ∑_{𝑋∶𝒰} ‖𝑋 ≃ Fin 𝑛‖ × (𝑋 → 𝑋 → Bool). Its identities (𝑋,𝐺) = (𝑌,𝐻) are equivalent to 𝑒 ∶ 𝑋 ≃ 𝑌 with ∏_{𝑥,𝑥′} 𝐻(𝑒𝑥, 𝑒𝑥′) = 𝐺(𝑥,𝑥′), the truncated finiteness witness adding no identity data. Positions 𝑝 ∶ 𝑋 → ℝ² move to 𝑝 ∘ 𝑒⁻¹, and selected vertices and property proofs move along the same equivalence. Identifying shapes does not license leaving node-indexed data untouched.
- Matrix_𝑛 ∶= Fin 𝑛 → Fin 𝑛 → Bool and AdjList_𝑛 ∶= ∑_{𝐿∶Fin 𝑛 → List(Fin 𝑛)} ∏_𝑖 (Sorted(𝐿𝑖) × NoDup(𝐿𝑖)), canonical lists only, with propositional laws under the standard order on Fin 𝑛. Membership gives lists → matrix; enumerating each row's true entries in increasing order gives the inverse. matrix → list → matrix is the identity pointwise by Boolean extensionality; list → matrix → list is the identity because rows are sorted and duplicate-free, with witnesses identified by propositionality. So the equivalence is Matrix_𝑛 ≃ AdjList_𝑛, not matrices with all lists, which would only give canonical representatives. Algorithms and proofs transport across it, and relabeling and matrix/list conversion are distinct transformations.

**Indexed packing and transport of an operation.** With Vec(𝐴,𝑛) ∶= Fin 𝑛 → 𝐴: pack(𝑥,𝑦)(inl 𝑖) ∶= 𝑥𝑖, pack(𝑥,𝑦)(inr 𝑗) ∶= 𝑦𝑗, unpack 𝑧 ∶= (𝜆𝑖.𝑧(inl 𝑖), 𝜆𝑗.𝑧(inr 𝑗)), and 𝑒_{𝑚,𝑛} ∶ Vec(𝐴,𝑚) × Vec(𝐴,𝑛) ≃ (Fin 𝑚 + Fin 𝑛 → 𝐴), with composites the identity by pair equality and funext. An operation 𝑓 on pairs becomes pack ∘ 𝑓 ∘ unpack, and its dependent specification and preservation proof transport along ua(𝑒_{𝑚,𝑛}). This is a complete index-level change. Byte layout or native in-place update adds its own state and address correspondence; packing indices does not establish alias freedom or layout equivalence.

### 8.4 Executing an imperative operation is already a function

`*p += 1` on a valid mutable 32-bit word at 𝑎 is, under language premises, inc_𝑎 ∶ 𝑆_valid(𝑎) → 𝑆_valid(𝑎) with read_𝑎(inc_𝑎 𝑠) = add32(read_𝑎 𝑠, 1) (8.13). The full specification adds a frame condition for other locations and, under concurrency, the permitted interleavings and memory orders. The imperative operation is no less exact than (8.11); its domain just has more state and premises. A proof inhabits ∏_{𝑠∶𝑆_valid(𝑎)} read_𝑎(inc_𝑎 𝑠) = add32(read_𝑎 𝑠, 1) (8.14). A layout change transports state and proof given a structured equivalence of machine states. Expression, typed syntax, interpretation and proof stay separately identifiable, and no privileged sequence of attach, then lower, then realize is required; any of these objects and relations can be worked on.

**A concrete declaration.**

```cpp
#include <cstdint>
void increment(std::uint32_t* p) { *p += 1u; }
```

The parse gives a function with one pointer parameter, a compound assignment, a dereference and an unsigned literal. Lookup resolves std::uint32_t to the implementation's exact-width type when supplied. Static rules require a modifiable lvalue of that type and a well-formed compound assignment. The elaboration holds resolved types and binding identities, not just characters. Let 𝑆_𝑝 be the states where 𝑝 designates a live, writable, aligned uint32_t with no conflicting concurrent access, with load_𝑝, store_𝑝 and the frame law on disjoint objects. The sequential source semantics gives:

- increment(𝑠) ∶= store_𝑝(𝑠, load_𝑝 𝑠 +₃₂ 1);
- load_𝑝(increment 𝑠) = load_𝑝 𝑠 +₃₂ 1;
- frame_𝑞(increment 𝑠) = frame_𝑞 𝑠 for 𝑞 disjoint from 𝑝 (8.15).

The first is this statement's denotation under its premises, and the others follow from the store/load and frame laws. The event trace is a read then a write at 𝑝, in source sequencing order. The byte-level version comes from (8.6)–(8.12) by transport along the structured state equivalence. Text, elaborated syntax, abstract-machine transformation and compiled behavior are distinct representations linked by these equations and the chosen refinement theorem.

### 8.5 Machine representations and memory laws

A representation is either an equivalence of types, with transport by ua, or a stated relation from concrete to abstract states, where the relation or quotient says what is hidden. These are different objects. (3.6)–(3.7) is the information-preserving factorization, and observational refinement is §8.2.

- **Arrays** use §4's Vec ≃ Fin 𝑛 → 𝐴. Transporting update is characterized by index(update(𝑣,𝑖,𝑎), 𝑗) = 𝑎 if 𝑗 = 𝑖, else index(𝑣,𝑗) (8.16). That equation is the obligation tying an in-place or packed implementation to the array operation; maps between element containers are not enough.
- **Matrices** Fin 𝑚 × Fin 𝑛 → 𝐴 get a flat layout from a bijection with Fin(𝑚𝑛). Row-major and column-major are two such bijections, and a layout change transports data and dependent constructions. Strides and alignment add encoding and address arithmetic. Tensors are the same argument, not a separate foundation.
- **Exact scalars.** 𝑈_𝑤 = Fin(2^𝑤): + and × are mod 2^𝑤, subtraction is the residue, and bitwise operations act on the 𝑤 digits. A native primitive implements these under a checked theorem or a named primitive assumption.
  - Nat, the runtime's unbounded Word and fixed-width words like U64 are distinguished. A small-integer fast path for Nat may spill to larger storage and is not equivalent to U64. {𝑛 ∶ Nat ∣ 𝑛 < 2⁶⁴} ≃ U64, but unbounded + is not closed on it without a result bound.
  - Division, shifts and signed interpretation have explicit definitions. A host primitive may define division by zero as a distinguished result or reduce shift counts mod width, but a lifted C++ operation uses C++'s definedness conditions; a primitive's convenience convention never silently replaces source behavior. Two's-complement storage and signed arithmetic semantics are separate choices, and a bit pattern does not justify overflowing C++ signed arithmetic.
  - Floats include format, signed zeros, infinities, NaNs and observable bit distinctions. Operations depend on rounding mode and exception environment, and fused and unfused are different functions. Transformations use laws valid in the chosen semantics (real associativity doesn't license reassociating float addition), or a specification proves a bounded error relation to a real function.
- **Bytes, layouts, validity.** Byte = U8 and Bytes 𝑛 = Fin 𝑛 → Byte. An encoding is enc ∶ 𝐴 → Bytes 𝑛 with dec ∶ Bytes 𝑛 → Option 𝐴. The minimum law is dec(enc 𝑎) = some 𝑎. Canonicity adds enc 𝑎 = 𝑏 whenever dec 𝑏 = some 𝑎; without it, padding and redundant encodings stay distinguishable. A full equivalence is with valid canonical encodings plus evidence, not with all 𝑛-byte strings.
  - LE U64: enc(𝑥)(𝑖) = ⌊𝑥/256^𝑖⌋ mod 256, dec(𝑏) = ∑_{𝑖≤7} 256^𝑖 𝑏(𝑖) (8.17); range bounds give both round trips. A load from a larger region also needs the offset bound, architectural alignment, and the source semantics' permission to access the bytes as that value; integer inequalities don't establish lifetime or pointer validity.
  - Records combine field encodings at offsets with alignment and padding. Sums also encode the tag and its relation to the active payload. Recursive structures use addresses and allocation metadata. ABI layouts include every observable detail: calling convention, alignment, padding, tag placement, register classes, return convention, object and exception ABI.
- **Memory as state** (Hoare and separation logic, 17, 37).
  - Elementary model: 𝑀 ∶ Addr → Byte plus an allocated domain, with store(𝑀,𝑎,𝑣)(𝑏) = 𝑣 if 𝑏 = 𝑎, else 𝑀(𝑏) (8.18). Read-after-write and frame laws follow, as proofs about the memory-changing operation. Mutation doesn't displace functional reasoning; it picks the transition reasoned about.
  - Source memory state usually has more: allocation identities, extents, lifetimes, permissions, provenance, active union members, synchronization events. An address-only model is a useful target or abstraction when a theorem or explicit assumption connects it to the rich one. Reusing a physical address after free does not resurrect an old source pointer.
  - Allocation consumes state and returns failure or a pointer plus an updated state satisfying the allocation conditions. A region allocator keeps [base,end) and a cursor, and allocating 𝑛 aligned bytes computes the aligned cursor and advances only after checking the calculation and range. Deallocation ends the lifetime. Logical metadata can be erased from the native representation once its uses are discharged.
  - Separation is about disjoint resources: a frame theorem says a transition on one component preserves the others. Linear or affine usage can make exclusivity visible from uses but does not replace the semantic statement about aliases. Runtime graph refcounts and source ownership are distinct, even when an implementation uses both to avoid copies.
  - For persistent values, unique use licenses destructive reuse of storage. If another live reference may see the old value, it must be preserved by copying or a proved sharing discipline. The transformation is accepted because it preserves source observations, not because the store is fast.
- **The loop** is (7.4) applied to real C++ memory and integer semantics, array-access premises and unsigned wraparound included. There is no second loop theorem.

### 8.6 C++ static and dynamic semantics

The frontend keeps source structure and elaborated choices. Input includes preprocessing context, headers or module units, macros, translation options and TU boundaries. The static semantics covers at least these categories, each preserved in the lifted object and governed by the standard's clauses:

- translation and preprocessing: tokens, macro expansion, inclusion, conditional translation, modules, linkage, ODR;
- names and declarations: scopes, lookup, ADL, access, declaration identity, overload sets;
- types and expressions: value categories, cv, conversions, initialization, overload resolution, selected implicit operations;
- classes: base subobjects, inheritance, ctors/dtors, special members, virtuals, member access;
- templates and constraints: deduction, substitution, partial ordering, specialization, dependent lookup, instantiation, SFINAE, constraint satisfaction;
- constant evaluation: permitted constant expressions, diagnostics, result;
- program assembly: TU relationships, instantiated entities, required definitions, library/ABI dependencies.

A template is a program parameterized by its arguments, but its interpretation includes these rules. An unconstrained type-level lambda would lose deduction, lookup, overload selection and SFINAE, and ordinary application describes the explicit parameterization only after those rules apply. Clang's elaborated AST makes many choices explicit. Using it is a declared frontend assumption unless its output is independently validated against source semantics or the frontend has the required connection. A lifted Clang preserves what that Clang does, not standard conformance. Source mapping is kept so properties and transformations attach to the actual declarations and expressions.

**Dynamic semantics.** Process state holds the control, object, thread and external-world interface the standard requires, and an ABI realization adds layouts and runtime data. Required distinctions, not erased by lowering to byte loads and stores:

- objects and storage: allocation, alignment, storage duration, initialization, lifetime start/end, subobjects, union members, implicit creation, pointer validity/provenance;
- expressions: sequencing, permitted evaluation orders, side effects, value categories, temporary materialization, destruction;
- pointers and references: permitted arithmetic and comparisons, one-past, casts, references, pointer-to-member, invalidation;
- class execution: base/member initialization, dynamic type during construction/destruction, virtual dispatch, RTTI, destructor effects;
- control transfer: return, jumps, exceptions, handler selection, unwinding, cleanup order, termination;
- coroutines: frame creation and lifetime, suspension, resumption, completion, destruction;
- arithmetic: widths and conversions, signed definedness, FP environment, implementation-defined results;
- concurrency: threads, TLS, atomics with all ordering parameters, fences, synchronization, races, forward progress;
- library and environment: library contracts and implementation, allocation interfaces, I/O, termination, platform extensions and syscalls.

Also included: bit-fields, varargs, captures, placement and class-specific allocation, over-alignment, static and TLS init/destruction, multiple and virtual inheritance, and library operations with nonlocal control. Volatile, atomics and device memory keep their own contracts, not one generic load. The selected source and platform specifications decide each case.

The abstract machine prescribes no vtable; the ABI does. A source virtual call selects the overrider, and the ABI connection explains the vtable access, this-adjustment and call sequence. Likewise exception cleanup connects to the chosen unwinding tables and runtime. Keeping the levels apart allows both source reasoning and exact inspection of existing layouts.

**Semantic classes stay distinct.**

- Implementation-defined behavior is a documented profile parameter 𝜃.
- Unspecified behavior is permitted choices in a relation, never one branch silently chosen by a deterministic interpreter.
- UB marks a violation where the standard constrains nothing; it is not a harmless return value. In a behavior-set semantics it may be the top set, so refinement cannot exclude a target observation just because the source hit UB. Whether an observable prefix before UB is kept is an explicit choice of the formalization, not something the standard guarantees. For defined executions, the target satisfies §8.2's refinement.
- This is also the boundary of a Clang-based lift. An elaborated AST, with preprocessing context, selected overloads, instantiated templates and locations, is derivation-shaped for some static judgments. Importing it into StdDeriv needs a stated translation and evidence that each elaboration decision satisfies the selected rule instances. Until established, that is an explicit frontend assumption, not automatically (5.13)'s 𝑔, which concerns an already selected signature. Proving the frontend constructs only admitted elaborations discharges it on its input domain. Full C++ fidelity also covers dynamic semantics and the prose-to-rule correspondence; frontend soundness doesn't give those.
- An implementation's behavior after source UB can still be studied through its own semantics.
- Ill-formed programs and required diagnostics are static semantics. IFNDR keeps its classification rather than being treated as valid, and extensions are marked as extensions. This matters when a lifted program is used to claim portability, not just to preserve a deployed binary.

**A bounded C++ frame decoder.** C++20 abstract machine; a live readable byte array 𝑏 of length 𝑛; a disjoint, live, writable Frame at out; no concurrent mutation of the input.

```cpp
struct Frame { std::uint64_t tag, length; const std::uint8_t* data; };
bool decode(const std::uint8_t* b, std::size_t n, Frame* out) {
    if (n < 4) return false;
    auto len = std::uint64_t(b[2]) | (std::uint64_t(b[3]) << 8);
    if (len > n - 4) return false;
    out->tag = std::uint64_t(b[0]) | (std::uint64_t(b[1]) << 8);
    out->length = len;
    out->data = b + 4;
    return true;
}
```

With tag = 𝑏₀ + 2⁸𝑏₁ and ℓ = 𝑏₂ + 2⁸𝑏₃, the behavior, including the memory frame (8.19), is:

- false with output unchanged if 𝑛 < 4;
- false with output unchanged if 𝑛 ≥ 4 ∧ ℓ > 𝑛−4;
- true with output (tag, ℓ, 𝑏+4) if 𝑛 ≥ 4 ∧ ℓ ≤ 𝑛−4.

Why:

- The first test precedes every byte access. Afterwards 𝑛 ≥ 4, so 𝑛−4 doesn't underflow and the four header reads are in bounds.
- If ℓ > 𝑛−4, nothing has been written. Otherwise 4+ℓ ≤ 𝑛, so the payload view is inside the allocation, and the three writes touch only the distinct, live Frame.
- The arithmetic is exact since 𝑏ᵢ < 256 and tag, ℓ < 2¹⁶.

The proof is about the actual interpreted C++ declaration, not a rewritten model. It attaches by §6's erasure construction, and erasure returns this body. Replacing two byte reads by a 16-bit load is a separate transformation whose proof must establish alignment, aliasing, endianness and memory validity. This is the source-level memory proof, distinct from §9's pure bytecode example.

## 9. Native execution as a program transformation

The question is what relation makes machine behavior a realization of the source: an explicit simulation or refinement over the actual observations of both (prior art: Hoare logic, compiler correctness, verified source-to-machine refinement, 17, 18, 22; LLVM IR and infrastructure, 30). Source and target machines and their correspondence are terms and proofs here. The implementation route is C plus an existing C compiler (§9.4). The small decoded machine below exposes the compositional argument and prescribes no new ISA backend.

An executable is a machine representation of the program being built. Emitting it is a program transformation, and the correctness derivation says how its actual execution realizes the specified behavior; the bytes are not a second system modeled by the first. An encoding for a target 𝑀 maps program representations to finite byte sequences, together with decoding, loading and execution semantics. Correctness is refinement, TargetBeh_𝑀(compile 𝑝, 𝑥, 𝑡) ⟹ SourceBeh(𝑝,𝑥,𝑡) (9.1): no behavior the source forbids. Stronger hypotheses give equivalence of selected observations.

Small instance: instructions PUSHINPUT, PUSHCONST(𝑏), XOR, HALT encoded as 01, 02 b, 03, ff (9.2), with 𝐶(Input) = [01], 𝐶(Const 𝑏) = [02,𝑏], 𝐶(Xor(𝑢,𝑣)) = 𝐶𝑢 ++ 𝐶𝑣 ++ [03] (9.3). The theorem is stronger than "the result is right": from any stack 𝑆 and input 𝑥, 𝐶(𝑒) takes 𝑆 to 𝐸(𝑒,𝑥) ∷ 𝑆, leaving 𝑆 untouched; the XOR step composes both hypotheses and applies XOR. For §5.6's expression with 𝑘 = 0x5a the artifact is `01 02 5a 03 02 5a 03 ff`: push 𝑥, push 5a, XOR, push 5a, XOR, halt with 𝑥. This byte-level construction has a compositional proof, and §9.2 gives the universal evaluator-mediated realization. At the other end, targets are x86-64 processes, GPU kernels, network protocols or whole machines, with calling conventions, layouts, FP rules, loaders, world interaction and observable costs. Compilation and interpretation are representable and checkable here, and correctness is conditional on the target semantics supplied.

### 9.1 The byte compiler theorem

The source is §5.6. The decoder, machine, compiler recursion and stack-extension proof are together in §9.3 and not repeated.

### 9.2 Universal native realization by one evaluator

An interpreter is already a program. That gives a universal source-to-machine construction, not merely a fallback for constructors without instruction-selection rules. Fix the full represented 𝐿, its checked Code_𝐿, and ⟦𝑐⟧_𝐿 including value, fault, divergence and external-interaction observations. 𝑉 holds evaluator code, environment, shared-graph references, continuation frames and pending world requests. One step-result presentation, for Σ = (Cmd, Resp) and results 𝐴:

  EvalStep_Σ(𝑉,𝐴) = Continue(𝑉) + Return(𝐴) + Fault(Err) + ∑_{𝑞∶Cmd}(Resp 𝑞 → 𝑉), with next ∶ 𝑉 → EvalStep_Σ(𝑉,𝐴) (9.U1).

- Configurations and results are indexed by the checked code's return type, or 𝐴 is one universal value representation that records the type. The unindexed 𝑉 does not let a configuration of one type return an unrelated 𝐴.
- The last summand is the response-indexed continuation; issuing a command doesn't determine its response. A defunctionalized continuation id plus environment can replace the function in a first-order fragment.
- Constructors and handler interpretation agree with §7.6. Resource failure is an explicit OutOfResources case, or the theorem carries a resource-sufficiency premise.

**Universal native realization (conditional construction).** Suppose:

1. A machine-realizable fragment 𝐹 expresses the chosen evaluator `next`, with its concrete representation and its allocator, stack and handler interfaces. This is an expressibility premise, not a claim that all cubical operations compile directly.
2. Compiling that one term gives bytes 𝑏_next, an initialization/encoding relation 𝑅_𝑉 between target states and evaluator configurations, a total encoding for every admitted initial configuration satisfying 𝑅_𝑉, and a machine simulation 𝜈 for the evaluator's actual cases. Since one evaluator step may take many target steps, 𝜈 is a labeled weak simulation with the progress or divergence conditions the observation needs.
3. Evaluator adequacy 𝛼 relates the guarded or finite run of next from initial(𝑐) to ⟦𝑐⟧_𝐿, external command/response behavior included, and initialization, loading, resources and world-handler correspondence hold.

Then every 𝑐 ∶ Code_𝐿 has realize^eval_𝑀(𝑐) ∶= (𝑏_next, encode(initial 𝑐), 𝜈*; 𝛼), with TargetBeh_𝑀(realize 𝑐, 𝑜) ⟹ SourceBeh_𝐿(𝑐, 𝑜) (9.U2). Showing the intermediate relations, run_𝑀(𝑏_next, encode(initial 𝑐)) ℛ*_𝜈 evalRun(initial 𝑐) ℛ_eval ⟦𝑐⟧_𝐿 (9.U2a). The two are machine/evaluator simulation and evaluator adequacy; neither follows from the other, and ℛ means observation-preserving correspondence, not equality.

- 𝜈* is finite iteration or guarded continuation, and ";" composes evidence, not states.
- The simulation premise includes progress for each admitted transition (or a related fault/resource outcome) and matching divergence and fairness where observed. So the realization doesn't just exclude wrong observations; it follows every admitted evaluator run through the corresponding target behavior.
- One executable image may serve all 𝑐, with only the encoded initial configuration varying.
- The statement is relative to the target and world model, resources and loader included. An emitted command with no supplied response remains an explicit world obligation that the simulation cannot discharge.

**Closure under reflection.** If checker, specializer, compiler and evaluator are represented terms of 𝐿, (9.U2) covers them: the metacircle is executionally closed, and the native evaluator runs programs that inspect, transform and realize programs, their own presentations included. This needs checked representations and an evaluator implementing their rules. Checker soundness and compiler correctness keep their own obligations; realization uses only the independent 𝛼 and 𝜈.

𝛼 is used where the represented evaluator is executed. A directly compiled residual relates to its source by its checked conversion or path plus its own block evidence, with no 𝜈 obligation. A mixed artifact uses 𝜈 and the relevant 𝛼 only on stretches that keep an evaluator call on runtime code, so proof dependencies are local to the realization used. A raw-syntax evaluator is a guarded process, nontermination included, with no total normalizer or universe collapse, and typed quotation, reification and the Futamura equations keep their stages.

**Consequence.** Direct compilation of every constructor of 𝐿 is unnecessary for universal native executability, given these premises. Direct compilation becomes a semantics-preserving optimization of the general realization. 𝐹 matters for realizing the evaluator but does not limit which source terms run. §9.3 shows how machine relations are proved, §9.4 gives the chosen C route, and Appendix C is another possible realization, not the definition or a required backend. The evaluator route is a sufficient universal construction, and other realizations relate directly to the same computation.

Artifacts may mix direct segments and evaluator steps. That choice is a program transformation with a state relation, detailed after the compiler algebra below. Latency, memory, energy, rounding and observable scheduling are kept when they are part of the observation type. Logical reversibility does not establish zero dissipation; physical-cost claims depend on the realization and thermodynamic model (Landauer, Bennett, 61, 62; Jaynes on entropy, 63).

### 9.3 An exact decoded machine and a structural compiler theorem

This closes a small but nontrivial instance of the compiler algebra: arbitrarily nested source, an explicitly decoded byte target, and correctness by structural induction.

- Source (9.4): Byte = Fin 256 as 8 bits with pointwise ⊕. Expr ∶∶= Input ∣ Const 𝑏 ∣ Xor(𝑒₁,𝑒₂); 𝐸(Input,𝑥) = 𝑥, 𝐸(Const 𝑏,𝑥) = 𝑏, 𝐸(Xor(𝑢,𝑣),𝑥) = 𝐸(𝑢,𝑥) ⊕ 𝐸(𝑣,𝑥).
- Machine: State ∶= ℕ × List Byte × Byte × Status (9.5), with pc, stack (top leftmost), fixed input, and running/halted/fault. Code 𝐵 of length |𝐵| is addressed from 0, and fetch at pc ≥ |𝐵| traps. The instruction table:
  - `01` (length 1, running): (pc,S,x) ↦ (pc+1, x∷S, x).
  - `02 b` (length 2, running, pc+1 < |B|): (pc+2, b∷S, x).
  - `03` (length 1, running, S = b∷a∷R): (pc+1, (a xor b)∷R, x).
  - `ff` (length 1, running): (pc+1, S, x, halted).
  - Anything else, a truncated 02, or stack underflow faults with no further execution.
- Input and code are immutable, and 03 pops two values in LIFO order (XOR commutes, so order is immaterial here). step_𝐵 is total by the table, with halted and fault absorbing. run^𝑘_𝐵 is by primitive recursion, and trace_𝐵(𝑠,𝑡) is the list of actual decoded step states, not an asserted final result.
- Compilation: 𝐶(Input) ∶= [01], 𝐶(Const 𝑏) ∶= [02,𝑏], 𝐶(Xor(𝑢,𝑣)) ∶= 𝐶𝑢 ++ 𝐶𝑣 ++ [03], program(𝑒) ∶= 𝐶(𝑒) ++ [ff] (9.6). Each of the #(𝑒) AST nodes gives exactly one instruction. ℓ(Input) = 1, ℓ(Const) = 2, ℓ(Xor(𝑢,𝑣)) = ℓ𝑢 + ℓ𝑣 + 1, and |𝐶(𝑒)| = ℓ(𝑒) (9.7).

**Stack-extension invariant.** If 𝐶(𝑒) occupies exactly [𝑎, 𝑎+ℓ(𝑒)) of immutable 𝐵, then for every suffix 𝑆 and input 𝑥: run^{#(𝑒)}_𝐵(𝑎, 𝑆, 𝑥, running) = (𝑎+ℓ(𝑒), 𝐸(𝑒,𝑥) ∷ 𝑆, 𝑥, running) (9.8). It is stable under any surrounding stack.

*Proof.* Input reads 01 and pushes 𝑥; Const reads 02 b and pushes 𝑏. For Xor(𝑢,𝑣): IH on 𝑢 with suffix 𝑆 gives 𝐸𝑢 ∷ 𝑆; IH on 𝑣 with suffix 𝐸𝑢 ∷ 𝑆 gives 𝐸𝑣 ∷ 𝐸𝑢 ∷ 𝑆; then 03 pops both, pushes their XOR, and advances one byte, to exactly 𝑎+ℓ𝑢+ℓ𝑣+1. The trace has #𝑢+#𝑣+1 = #(Xor(𝑢,𝑣)) transitions. Nothing faults, since the hypotheses guarantee the bytes and two stack elements. □

Each decoder reads only its own encoding bytes and code is immutable, so the theorem is stable in any larger array; this is exactly the prefix-stability block concatenation needs. With 𝐵 ∶= program(𝑒): run^{#(𝑒)+1}_𝐵(0, [], 𝑥, running) = (|𝐵|, [𝐸(𝑒,𝑥)], 𝑥, halted) (9.9).

**Byte-for-byte.** 𝑒 ∶= Xor(Xor(Input, Const 0x5a), Const 0x5a) compiles to `01 02 5a 03 02 5a 03 ff` at addresses 00–07. Trace (pc, stack with top first):

| step | pc | stack | next |
|---|---|---|---|
| 0 | 0 | [] | input |
| 1 | 1 | [x] | push 5a |
| 2 | 3 | [5a, x] | XOR |
| 3 | 4 | [x⊕5a] | push 5a |
| 4 | 6 | [5a, x⊕5a] | XOR |
| 5 | 7 | [x] | halt |
| 6 | 8 | [x] | halted |

The fifth transition uses (𝑥⊕𝑘)⊕𝑘 = 𝑥, proved by exhaustive bit cases. The eight bytes implement the source term and preserve the proof attached to its meaning.

**Composition with source transformations.** For 𝑇 with 𝜏 ∶ ∏_𝑒∏_𝑥 𝐸(𝑇𝑒,𝑥) ≡ 𝐸(𝑒,𝑥): run^{#(𝑇𝑒)+1}_{program(𝑇𝑒)}(0,[],𝑥,running) = (|program(𝑇𝑒)|, [𝐸(𝑇𝑒,𝑥)], 𝑥, halted) ≡ (|program(𝑇𝑒)|, [𝐸(𝑒,𝑥)], 𝑥, halted) (9.10). The last step is a path in the result components with the same program and length. It does not identify the byte strings of 𝑒 and 𝑇𝑒, which may differ; semantic preservation is not byte identity.

**Placed blocks.** An executable block carries entry address, pre- and postcondition, decoded execution, and placement conditions. The single authoritative definition is (9.12) with composition (9.13). Concatenation alone is not a proof: the second block must sit at the first's exit, relocation must keep branch targets, and the first block's decoding must be stable inside the concatenation. The proof of (9.13) concatenates the two exact traces under these premises. Whole programs compose such blocks with the actual loader, ABI and deployment semantics, and there is no second unindexed block algebra. A verified emitter has the dependent type realize_𝑀 ∶ ∏_{𝑝∶Program} ∑_{𝑏∶Bytes} ∏_{𝑥} Rel_𝑂(run_𝑀(𝑏, init_𝑀 𝑥), ⟦𝑝⟧𝑥), where Rel_𝑂 is over the chosen observations and includes loader and ABI premises. An interpreter artifact is one realization, and specialization or native instruction selection gives others. The executable bytes appear in the proof's own type, not an unrelated model. Appendix C.1 keeps an architecture-specific frame decoder as a direct-relation example; the chosen route is §9.4.

**Decoder stability; constructor-indexed compiler algebra.** For an ISA with state 𝑆, decode ∶ Bytes → Addr → Option Instr (including instruction length and the updated ip), and exec ∶ Instr → 𝑆 → 𝑆 + Trap, fetch/execute is 𝛿_𝑇(𝑏,𝑠) = trap(decodeFault) if decode(𝑏, pc 𝑠) = none, else exec(𝑖,𝑠) (9.11). An ISA model defines registers, flags, memory accesses, traps, instruction bytes and observable events; a mnemonic is not an encoding semantics.

**Blocks.** For an immutable fetch image 𝐶:

- Contains(𝐶,𝑎,𝑏) says the bytes occur at 𝑎 in bounds. Admissible(𝐶,𝑎,𝑏) adds every decoding and placement condition the block needs: valid branch destinations, the bytes decoding depends on, executable regions. Exec*_𝐶(𝑠,𝑡,𝑠′) is finite execution with trace 𝑡.
- Block_ℛ(𝑎,𝑃,𝑄) ∶= ∑_{𝑏∶Bytes} ∏_𝐶 Admissible(𝐶,𝑎,𝑏) → ∏_𝑠 (𝑃𝑠 × pc 𝑠 = 𝑎) → ∑_{𝑡,𝑠′} Exec*_𝐶(𝑠,𝑡,𝑠′) × 𝑄𝑠′ × (pc 𝑠′ = 𝑎+|𝑏|) × ℛ(𝑠,𝑡,𝑠′) (9.12).
- The image is quantified but admissibility is an actual argument, because 𝑃(𝑠) cannot constrain an independently quantified 𝐶. Out-of-block branches, or decoders reading past the block's end, need more than Contains. A closed, position-independent straight-line block can use Contains once decoder locality is proved. 𝑃 includes memory permissions, stack and ABI conditions, and transitions preserve the image. Unit trace contracts give Hoare-style blocks.
- seq ∶ ∏_{𝐵₁∶Block_{ℛ₁}(𝑎,𝑃,𝑄)} ∏_{𝐵₂∶Block_{ℛ₂}(𝑎+|bytes 𝐵₁|, 𝑄, 𝑅)} StablePrefix(𝐵₁,𝐵₂) → RelocOK(𝐵₁,𝐵₂) → Block_{ℛ₁;ℛ₂}(𝑎,𝑃,𝑅) (9.13), where ℛ₁;ℛ₂ keeps the intermediate state and 𝑡 = 𝑡₁ ++ 𝑡₂.
- For a 𝐶 admissibly containing 𝑏₁++𝑏₂ at 𝑎, StablePrefix and RelocOK contain the proofs that this implies Admissible(𝐶,𝑎,𝑏₁) and Admissible(𝐶, 𝑎+|𝑏₁|, 𝑏₂); prefix/suffix containment is not enough when decoding or branching depends on the surroundings. Both proofs get the same 𝐶, the first ends at 𝑠₁ at 𝑎+|𝑏₁| where the second starts, and the derivations and traces concatenate, witnessed by 𝑠₁, 𝑡₁, 𝑡₂.
- Placement keeps branch destinations and decoder equations. If the second block is already certified at its final address, RelocOK is the identity witness, not a second relocation. These premises are constructed, never inferred from concatenation. Loops and continuing processes replace normal exit with a guarded step/trace correspondence.

**Exact bytes for a toy ISA** with variable-length instructions, byte registers r0 and r1, natural pc, and arithmetic mod 256:

| instruction | bytes | transition |
|---|---|---|
| MOVI r0, imm8 | `10 imm8` | r0 := imm8; pc += 2 |
| MOVI r1, imm8 | `11 imm8` | r1 := imm8; pc += 2 |
| XOR r0, r1 | `20` | r0 := r0 xor r1; pc += 1 |
| STORE r0, addr8 | `30 addr8` | mem[addr8] := r0; pc += 2 |
| HALT | `ff` | halted; pc += 1 |

The decoder cases on the first byte (and on the presence of a second byte for two-byte instructions); any other leading byte traps. `compileXor(a,b,d) = [10,a, 11,b, 20, 30,d, ff]` has the exact trace (0,_,_,𝜇) → (2,𝑎,_,𝜇) → (4,𝑎,𝑏,𝜇) → (5, 𝑎⊕𝑏, 𝑏, 𝜇) → (7, 𝑎⊕𝑏, 𝑏, 𝜇[𝑑↦𝑎⊕𝑏]) → (8, …, halted) (9.14). So the compiler returns bytes plus a trace proof that memory at 𝑑 holds the denotation. Richer ISAs supply their decoder, transitions and instruction-level block constructors. Not every source operation is one instruction: functions, allocation, exceptions, threads and syscalls need runtime and world representations with semantic clauses.

**Compilation is a homomorphism of interpreted constructors.** A compiler algebra gives, for each constructor c_𝑖, a block or continuing target process 𝐶_𝑖 with a proof relating its target interpretation to the constructor applied to compiled subterms. The fold is compile(c_𝑖(𝑡⃗)) = 𝐶_𝑖(compile 𝑡⃗) and correct(c_𝑖(𝑡⃗)) = cong_𝑅(𝐶_𝑖, correct 𝑡⃗) (9.15).

- cong_𝑅 is the constructor's specified relational congruence, not automatic congruence for any refinement: cong_{c_𝑖} ∶ ∏_{𝑗≤𝑛} 𝑅_𝑗(𝑡_𝑗, 𝑢_𝑗) → 𝑅(⟦c_𝑖(𝑡⃗)⟧, run(𝐶_𝑖(𝑢⃗))) (9.16), defined separately. Branches need matching conditions and successors, loops an invariant or coinductive simulation, external calls the chosen world relation.
- The compiler proof is induction on constructors using these functions, which locates effects, branch choices and runtime state in the constructor algebra. The recipe is exact given the algebra.
- The universal fallback is a machine interpreter of the source transition function, adequate by matching each transition, and native specialization removes interpretive structure where both semantics allow. Neither an algebra nor an adequacy theorem comes into existence by being named.

**Composition of source and target proofs.** ArtifactCorrect(𝑝,𝑃)(𝑜,ℎ) ∶= SourceCorrect(𝑝,𝑃)(CompilerRefines(𝑝)(𝑜,ℎ)) (9.17). This is function composition of evidence whose premises name the actual program, bytes, loader, machine and observation, and no further bridge is needed.

**Specializing the evaluator constructs direct compiler cases.** The evaluator is a systematic source of (9.15)'s blocks. Specialize `next` with the outer constructor c_𝑖 static and its subterms, environment and relevant world responses dynamic. By (6.F1), ⟦𝑟_𝑖⟧(𝑡⃗, 𝜌) = next(state(c_𝑖(𝑡⃗), 𝜌)) (9.U3).

- If 𝑟_𝑖 is in the realizable fragment, its block relation composes with (9.U3) and adequacy into a constructor-case machine relation. 𝜈 is needed only for segments actually run through 𝑏_next, not as an extra premise on direct residuals. Residuals that still call the evaluator get that portion from (9.U2).
- So specialization derives the candidate implementations and their semantic connections, instead of requiring unrelated per-constructor instruction implementations.
- A sufficient constructive route to the algebra: specialize the guarded evaluator run at each finite static code shape, keep environment and stack dynamic, and compile residual operations through 𝐹. 𝐶_𝑖 comes from the residual of c_𝑖's clause, with its meaning from (6.F1)/(9.U3) plus the residual's block evidence. This needs §6.2's bounded static variation and residual-composition conditions, which the specializer equation alone doesn't give. A residual evaluator call is realized through (9.U2), not a compilation failure.
- The homomorphic fold (9.15) also needs the residuals to compose with compiled-subterm representations and their state/continuation relations. Neither (6.F1) nor 𝜈 proves that, or guarantees that static dispatch is gone. Hand-picked instructions are further specializations or alternatives with their own relation to the same evaluator case. There is no mandatory interpreted/direct boundary.

**Mixed realizations, effect history, resources.**

- 𝑅_val(𝑑,𝑣) relates direct and evaluator value representations, keeping type indices and any heap-ownership or aliasing conditions. A crossing is justified by 𝒳 ∶ 𝐷 → 𝑉 → 𝒰 preserving the represented continuation, live values via 𝑅_val, outstanding requests, and observations.
- Direct-to-evaluator transfer establishes 𝑅_𝑉 for the encoded configuration; evaluator-to-direct, when used, establishes the receiving block's premises. Two representations holding a word with the same bits do not give 𝑅_val. A return to direct code needs a relation only if it actually happens.
- A mixed artifact composes direct block proofs, crossings and iterations of 𝜈, and its correctness is that composite, not a second semantics at a runtime boundary.
- A static residual evaluator call stays where code or environment is unknown at specialization. Dynamically computed type families, closures or face conditions don't force one; they compile directly once their machine relation exists.
- A dynamic exit happens when a direct block's own premise fails (for example a fast path assuming a result fits a word). It needs either a proved continuation map from direct state to evaluator configuration or a justified restart. Restart is invalid once world effects have been issued, since it could duplicate a send, write or payment. A resumable exit's relation preserves issued-effect history, outstanding command tokens, waiters, and the retained snapshot or a proved migration (§10.5).
- Direct and evaluator calls may issue commands through the same represented handler, so world interaction is not a crossing into the evaluator. A shared suspended computation keeps its producer identity and waiters across representation changes instead of being recomputed per consumer.
- Every block and crossing has a resource contract: sufficient arena and stack, or an explicit resource-failure outcome with a stated relation to source observations. Exhaustion is never unmodeled permission to stop a supposedly total computation. 𝜈 quantifies over all admitted configurations, not test cases, and its proof uses induction on evaluator code and invariants over heap, stack, loader and world, which are objects of the same kind as the programs they describe.

**Loading, native substitution, export.**

- Executable bytes are read together with format, entry, relocations, linking, initial stack, environment and OS state. An ELF parser does not implement dynamic linking. Executable-memory setup and architecture-required icache synchronization are transitions of the loading world, part of the assumptions or checked code establishing the entry condition.
- With the target-correctness witness, the evaluator may replace an applicable source computation by the native implementation under the theorem's preconditions: substitution of equals through the observation for pure closed input, and under the proved process congruence or refinement for interaction. A JIT does this when more inputs become known, and its cache stores the artifact with the evidence and dependencies licensing its use.
- A standalone export includes the selected runtime support and handlers, or records their external contracts. C is the primary target. The downstream compiler's relation is an explicit dependency or a checked construction (§9.4). A printer's correctness is about the printed text's parsed meaning; a proof about the input tree doesn't make printing infallible.
- GPUs and accelerators are no exception: ISA, memory spaces, barriers and launch are target semantics, and tiling and parallelization preserve a stated observation under the selected memory and arithmetic models.

### 9.4 Compilation through C

C is the primary target. A represented program, or its evaluator specialized at known inputs, is lowered to C together with its relation to the source interpretation, and an existing C compiler supplies instruction selection, object representation and executable construction. Language, kernel and network don't need direct emitters per architecture. The mathematics is composition of the relations already in (9.17). For source 𝑝, generated C 𝑐, compiler-plus-configuration 𝐾, artifact 𝑏 = 𝐾(𝑐), a shared observation type 𝑂, related initial states and world handlers, and the domain where 𝑐 is defined:

  𝜆_𝑝 ∶ Beh_C(𝑐,𝑜) → Beh_𝐿(𝑝,𝑜),  𝜅_𝑐 ∶ Beh_𝑀(𝐾𝑐, 𝑜) → Beh_C(𝑐,𝑜),  (𝜆_𝑝 ∘ 𝜅_𝑐)(ℎ) ∶ Beh_𝐿(𝑝,𝑜) (9.C1)

- Source-to-C supplies 𝜆_𝑝, C-definedness premises included; the downstream relation supplies 𝜅_𝑐.
- If the observation distinguishes termination, divergence, resource exhaustion or external effects, both relations carry those distinctions, and a value-only equation can't stand in for them.
- The same composition covers §9.2's C realization of the evaluator and directly lowered residuals, so the whole represented language is covered without a processor instruction per constructor.
- The generated artifact includes declarations, runtime support and the external contracts actually used. Lowering checked syntax to C text needs a printer/reader correspondence: the compiled text has to denote the C construction the relation was proved for.
- Definedness obligations (integer and FP behavior, valid lifetimes and accesses, aliasing, the actual concurrency model) are properties of the generated program, not consequences of the target being called C.

**A concrete C target:** §8.6's decoder, with exact-width types where provided, a readable input, no concurrent input mutation, and a separate writable live output. size_t is the object-size type, and no register assignment, pointer width or struct offset is prescribed.

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct { uint64_t tag, length; const uint8_t *data; } Frame;
bool decode_frame(const uint8_t *b, size_t n, Frame *out)
{
    if (n < 4) return false;
    uint64_t length = (uint64_t)b[2] | ((uint64_t)b[3] << 8);
    if (length > n - 4) return false;
    out->tag = (uint64_t)b[0] | ((uint64_t)b[1] << 8);
    out->length = length;
    out->data = b + 4;
    return true;
}
```

The first guard precedes every read. Each shifted value is ≤ 255·256 and each field ≤ 65535, preserved by the explicit unsigned conversions. After the guard 𝑛−4 is defined, and the second guard puts the whole payload inside the input. Failure leaves the output unchanged, and success writes exactly tag, length and the borrowed view. These branch facts give §8.6's source-level result, with outputs observed through the C fields, not invented offsets. The downstream compiler picks loads, stores, branches, calling convention and padding, so the source proof is reused through (9.C1), not rewritten per ISA. The native artifact still has concrete machine meaning, reached through an existing compiler.

**Reusing and later verifying the compiler.** The toolchain's compilation relation is explicit. At first it may be a stated assumption among an artifact's trusted dependencies. Lifting the compiler into the language makes its syntax, behavior and transformations analyzable, and a proof of its relation replaces the assumption where established. That is the chosen verification direction, instead of making bespoke x86/ARM backend proofs prerequisites to use (precedent: verified compilation, 18). No external compiler or configuration is silently chosen or declared verified.

- For 𝑄 ∶ 𝑂 → 𝒰 and 𝑞 ∶ ∏_𝑜 Beh_𝐿(𝑝,𝑜) → 𝑄(𝑜), the target proof is artifactQ(𝑜,ℎ) ∶= 𝑞(𝑜, 𝜆_𝑝(𝜅_𝑐 ℎ)) (9.C2).
- Changing the C compiler, configuration, runtime or a used primitive changes this result's dependency, never the source program's meaning. Specialization identities, C lowering and downstream compilation are distinct, composing constructions, not competing accounts of execution.
- General form, for represented 𝐿, 𝑀, source 𝑝, common 𝑂, and environment interpretation 𝐸 (related initial states and handlers) with obs_𝐿, obs_𝑀: ∑_{𝑎∶Artifact_𝑀} ∏_{𝜌∶Run_𝑀(𝑎,𝐸)} ∑_{𝜏∶Run_𝐿(𝑝,𝐸)} (obs_𝑀 𝜌 =_𝑂 obs_𝐿 𝜏), keeping the artifact and a source run per target run. A source property 𝑄 holding of every execution is applied at 𝜏 and transported along the inverse equality to 𝑄(obs_𝑀 𝜌). That is the general property-transfer operation, and each compiler inhabits it for the semantics it handles. Equality of specialized source terms and the source-run/machine-bytes relation stay distinct, composable results.

The contract is the same whether the middle is C or a direct byte block. Appendix C keeps a direct x86 instance for comparison, not as a required route. C is the chosen path, and the general relation explains why reusing its compiler preserves work already established at the source.

## 10. Persistent, interactive, and distributed programs

A computation that changes its descriptions also has to decide which earlier conclusions still apply. §§2–9 supply identities, dependency-sensitive properties, executable transformations and continuing interactions, and persistence and exchange extend these same relations across time and participants with no second meaning. Retaining what is established, acquiring observations, changing interpretations and making constructions available elsewhere are transitions and relations inside the continuing process, not services around a finished program, and the programs implementing them are part of what is described. Antecedents: process calculi, causal order, temporal specification, refinement, incremental computation, bidirectional views (21–26, 38). The terms below compose into the actual distributed program, not an external verification model.

- Processes may request input, await messages, mutate state, emit events, or run forever. Process_Σ(𝐴) ∋ return(𝑎) ∣ request(𝑞,𝑘) ∣ delay(𝑝) (10.1), with 𝑘 continuing on the response to 𝑞. Handlers interpret requests in a particular world, and different handlers give different observations of one description. A network is not an opaque service: it is communicating process states and transitions, including loss, ordering, failure and partial knowledge.
- Persistence brings identity over time: a definition can stay stable while names, inputs or observed dependencies change. For pure 𝑞 and data 𝑑, Maintained(𝑞,𝑑) ∶= ∑_{𝑟∶𝑅}(𝑟 = 𝑞(𝑑)) (10.2). A valid incremental update takes a maintained result for 𝑑 to one for 𝑑′ with evidence that it denotes 𝑞(𝑑′). This holds whatever the representation: table, spreadsheet cell, widget or compiled cache (Codd's relational model for one of them, 60).
- A cached evaluation records every lookup it depends on, absences included. If those observations still hold, a deterministic pure segment is reusable. A computation suspended on an external request keeps its continuation and request distinct from a completed pure value. Failures, waiter delivery and invalidation are transitions of represented state, not grounds for a foundational "task queue".
- UIs likewise: a view is an interpretation of state, and an edit relates displayed representation to underlying state. Bidirectional laws say what round trips preserve and lose. Diagram, text editor, table, proof visualization and immersive spatial interface are all views, none privileged (statecharts, Harel, 66).
- Distributed systems (local states, messages, causal relations, partial observations), GPUs (parallel execution, memory, numerics) and OSes (scheduling, isolation, devices) are distinct constructions, not metaphors or primitive divisions of computation, and their composition decides which information and interactions become available next.

**Handler composition and safety transfer.** Handlers ℎ₁, ℎ₂ interpret requests in world relations, with a simulation 𝑅 ∶ 𝑊₁ → 𝑊₂ → 𝒰. If for every 𝑅(𝑤₁,𝑤₂) each ℎ₂-transition with response 𝑟 has a matching ℎ₁-transition with the same selected observation and related successors, the relation lifts coinductively to the interpreted processes, by cases on return, delay and request (the request case uses the step simulation plus the hypothesis on the continuation). A safety property invariant under the observation transfers from source to target. Progress also needs the fairness condition the scheduling semantics actually uses: safety is induction over reachable states, progress concerns continuing executions and does not follow from a safety invariant (temporal logic, 58).

### 10.1 Exact reuse requires recorded information

A dependency certificate records what was observed at each key, absence included, not just the keys. valid(𝑆,𝐷) ∶= ∏_{(𝑘,𝑜)∈𝐷}(lookup(𝑆,𝑘) = 𝑜) (10.3). For a deterministic pure segment with complete read trace 𝐷, validity in a new store gives the same reductions up to the next unrecorded lookup or world request, by induction on steps: pure reductions are fixed by term and environment, and recorded lookups return equal observations. A suspended computation additionally keeps its residual, dependency observations, pending request and waiting continuations. Failure is not success, and a world response is never silently replayed as a pure lookup. These are exact distinctions about computation and interaction, not arbitrary product divisions into caching, queueing and workflow.

### 10.2 A maintained view is a proof about an update

- For query 𝑞 ∶ 𝐷 → 𝑅 and update 𝛿, Δ_𝑞 ∶ 𝑅 → 𝐷 → 𝑅 is correct when ∀𝑑. Δ_𝑞(𝑞𝑑, 𝑑) = 𝑞(𝛿𝑑) (10.4), with edit data 𝑒 threaded through 𝛿_𝑒 and Δ_𝑞(−,−,𝑒). It is a property of the functions, instantiated by materialized views, spreadsheet dependencies and rendered graph visualizations.
- Lenses: get(put(𝑠,𝑣)) = 𝑣 and put(𝑠, get 𝑠) = 𝑠 (10.5) characterize total view/update pairs. Lossy views and partial edits get weaker or differently typed laws.
- Mat(𝑞,𝑑) ∶= ∑_𝑟(𝑟 = 𝑞(𝑑)) keeps a value with its equality to the query at the same input version.

**An actual incremental edit.** Take 𝑣 ∶ Vec(𝑈₆₄, 𝑛), 𝑞(𝑣) = foldl(+₆₄, 0, 𝑣), 𝑢_{𝑖,𝑥}(𝑣) = 𝑣[𝑖 ∶= 𝑥]. Define:

- Δ_{𝑖,𝑥}(𝑣,𝑟) ∶= 𝑟 −₆₄ 𝑣ᵢ +₆₄ 𝑥;
- 𝜆_{𝑖,𝑥,𝑣} ∶ Δ_{𝑖,𝑥}(𝑣, 𝑞𝑣) = 𝑞(𝑢_{𝑖,𝑥} 𝑣), by splitting the fold into prefix, 𝑣ᵢ and suffix, then using associativity, commutativity and inverses mod 2⁶⁴ to cancel 𝑣ᵢ and insert 𝑥;
- maint_{𝑖,𝑥}(𝑣, (𝑟,𝑒)) ∶= (Δ_{𝑖,𝑥}(𝑣,𝑟), ap(Δ_{𝑖,𝑥}(𝑣,−), 𝑒) ⋅ 𝜆_{𝑖,𝑥,𝑣}) ∶ Mat(𝑞, 𝑢_{𝑖,𝑥} 𝑣).

That costs one old element and two modular operations, with no refold. Publishing the edited vector under a new immutable identity selects this result for a subscribed sum view. A later 𝜃 ∶ 𝑞 = 𝑞′ makes the same 𝑟′ maintained for 𝑞′ via 𝑒′ ⋅ ap(𝜆𝑓. 𝑓 𝑣′, 𝜃). A local compiled artifact or a remote evaluator consumes the definition and value at the same type. Device reads supplying later edits remain separate world interactions, and reusing the pure sum doesn't repeat them.

**A source edit changes proof, bytes and view.** 𝑝₀ = Xor(Xor(Input, Const 0x5a), Const 0x5a) has 𝐸(𝑝₀,𝑥) = 𝑥 by the involutive XOR law and bytes `01 02 5a 03 02 5a 03 ff` under §9.3's decoder. Change only the last constant: 𝑝₁ = …Const 0x5b, with 𝐸(𝑝₁,𝑥) = (𝑥⊕5a)⊕5b = 𝑥⊕0x01 and unsimplified bytes `01 02 5a 03 02 5b 03 ff`. The old proof isn't a proof for 𝑝₁ (at 𝑥 = 0 the output is 1), so dependencies, any simplified residual and the rendering must refer to the new version. Byte type, XOR, interpreter and compiler definitions are unchanged. This differs from the vector update above and the decoder/live-upgrade below, each a different actual dependency path through the same language.

### 10.3 A distributed credit invariant, with no global scheduler as an axiom

Peers 𝑁, local credit 𝑐ᵢ ∈ ℕ, and in-flight credit 𝑚ᵢⱼ ∈ ℕ per directed channel. The specified global state is useful for stating an invariant even though no participant observes it: 𝑆 ∶= (∏_𝑖 ℕ) × (∏_{𝑖,𝑗} ℕ), total(𝑆) ∶= ∑𝑐ᵢ + ∑𝑚ᵢⱼ (10.6). Allocation of 𝑞 ≤ 𝑐ᵢ: (𝑐ᵢ, 𝑚ᵢⱼ) ↦ (𝑐ᵢ−𝑞, 𝑚ᵢⱼ+𝑞) (10.7). Delivery, when 𝑞 ≤ 𝑚ᵢⱼ: (𝑚ᵢⱼ, 𝑐ⱼ) ↦ (𝑚ᵢⱼ−𝑞, 𝑐ⱼ+𝑞) (10.8). Both preserve total by cancellation, and so does every finite sequence, by induction on length. Elementary, exact, and independent of any centralized execution algorithm.

A real protocol also defines message identities, duplication resistance, acknowledgment, failure, reallocation and when credit can return. With duplication and no idempotency rule, conservation fails: one allocation delivered twice. The real distinction is between the stated invariant and the actual transition rules, not between mathematics and networking.

Observer 𝑖 has Obs_𝑖 ∶ 𝑆 → 𝑂_𝑖 and Possible_𝑖(𝑜) ∶= ∑_𝑠(Obs_𝑖 𝑠 = 𝑜) (10.9). Local knowledge is a construction over partial observation: a peer proves an action safe for every compatible state without holding the global state. This is a precise setting for local invariants, distributed verification and computation by message exchange, and obligations, receipts, debt transfer and cancellation are expressible once their transitions are defined.

*Receipt-bearing delivery.* Messages carry a unique transfer id 𝑢, source, destination and amount, and state records sent, delivered and acknowledged sets.

- Delivery of 𝑢 requires 𝑢 ∉ delivered. It consumes the in-flight credit exactly once, credits the receiver, and adds 𝑢 to delivered.
- Acknowledgment requires 𝑢 ∈ delivered and records a receipt; it moves no credit.
- A return is a new uniquely identified reverse transfer, subject to available credit.

Each transfer conserves credit, and the delivered-set premise excludes duplicate delivery. Failures leave in-flight credit represented until an explicit recovery or cancellation transition acts on it.

### 10.4 Observations, information, and locally available actions

§1's fibers applied to available actions in a running system. For 𝑜 ∶ 𝑆 → 𝑂, possibly itself computed by an interacting process: Fiber_𝑜(𝑦) ∶= ∑_𝑠(𝑜𝑠 = 𝑦) (10.10). If an action needs 𝑃 ∶ 𝑆 → 𝒰, the observer establishes safety without guessing the hidden state through SafeAt(𝑦) ∶= ∏_{(𝑠,𝑝)∶Fiber_𝑜(𝑦)} 𝑃(𝑠) (10.11). That is stronger than exhibiting one safe compatible state. A refinement 𝑜′ with 𝑜 = ℎ ∘ 𝑜′ makes the fiber no larger, so coarse safety evidence is reused under the refinement by precomposition; the converse generally fails.

- *Information is constraints on possible states and actions.* Given a value function on admissible trajectories, one can compare the optimum over trajectories compatible with one observation against a refined one. Monotonicity holds when refinement expands the available contingent policies with objective and feasibility held fixed; it doesn't follow from arbitrary differences between observations.
- The same construction governs distributed systems, debugging, interfaces and inference. The observer can be part of the process. Combined observations constrain a joint fiber, and a participant uses another's observation only when interaction supplies it. No global knower and no separate theory of "knowledge" is needed. The observer's own state, including the interpretation recording an event, can be among the observed states.
- That is how the account reaches communication and language. An utterance is an event in a causal history, a response depends on retained context and interpretation, and responses change the context for later utterances. A population's conventions arise from these interacting histories, with no fixed intrinsic meaning and no separate collective mind needed to say what each participant can infer and how information passes between them. Scientific instruments and the processes interpreting them contribute their transitions and observations in the same way.

*Updating possible worlds after a response.* With 𝐾 ∶ 𝑊 → 𝒰 the compatible worlds and 𝐻_𝑐 ∶ 𝑊 → 𝑅_𝑐 → 𝑊 → 𝒰: 𝐾′(𝑤′) ∶= ‖∑_𝑤 𝐾(𝑤) × 𝐻_𝑐(𝑤,𝑟,𝑤′)‖ (10.12), and action safety is ∏_{𝑤′} 𝐾′(𝑤′) → 𝑃(𝑤′). This is the same dependent reasoning as source semantics and machine invariants, with only the world relation changed.

### 10.5 Dependency certificates and persistent evaluation

Key (definition identities), Version, and Store ∶ Key → Option(Version × Value), a finite partial map. A negative lookup is a dependency on absence at a revision.

- Dep ∶= Present(𝑘,𝑣) + Absent(𝑘,𝑒). valid(𝑆, Present(𝑘,𝑣)) ∶= version(𝑆[𝑘]) = 𝑣; valid(𝑆, Absent(𝑘,𝑒)) ∶= 𝑆[𝑘] = none ∧ epoch(𝑆) = 𝑒; validAll ∶= ∏ (10.13).
- (10.13) is a version-based sufficient condition for observational validity (10.17), not the same predicate by definition. The certificate carries version(𝑆[𝑘]) = version(𝑆′[𝑘]) ⇒ Lookup(𝑆,𝑘) = Lookup(𝑆′,𝑘) for present entries, and epoch(𝑆) = epoch(𝑆′) ⇒ both lookups are none for absent ones. Epochs for negative lookups may be coarse, and a finer index-version certificate is equally valid if it proves the result unchanged. What matters is soundness, not minimal invalidation. The full producer relation, with suspension, failure and waiters, uses the same validity predicate.

**Incremental updates are programs with proofs.** For 𝑓 ∶ 𝐴 → 𝐵 with edits 𝛿𝐴, applyA and applyB: derive(𝑓) ∶ ∏_𝑎∏_{𝑑∶𝛿𝐴}∑_{𝑒∶𝛿𝐵} Path_𝐵(applyB(𝑓𝑎, 𝑒), 𝑓(applyA(𝑎,𝑑))) (10.14). For a sum over a finite map into an abelian group 𝐺, replacing 𝑚[𝑘] = 𝑥 by 𝑦 has derivative 𝑒 = 𝑦−𝑥 with ∑𝑚 + (𝑦−𝑥) = ∑(𝑚[𝑘↦𝑦]) (10.15). A generic correct fallback (recompute, and return the difference if the edit algebra allows) and a specialized constant-work update are both legitimate; the specification includes derivative and evidence, and no incremental-computation oracle is assumed.

**A dependency-tracked evaluator.** Each next operation is a reduction, a store lookup, a world request, a return or a fault. State:

- Outcome(𝐴) ∶= Value 𝐴 + Error Err; Read ∶= ∑_{𝑘∶Key} LookupResult(𝑘); Certificate ∶= List Read.
- EffectScope ∶= InvocationId × WorldEpoch; RequestToken ∶= InvocationId × StepId.
- Producer(𝐴) ∶= Absent + Running(Residual, Snapshot, Certificate, Waiters) + Suspended(Residual, Snapshot, Certificate, Waiters, Request) + Ready(𝐴, Certificate, EffectScope) + Failed(Err, Certificate, EffectScope) (10.16).

Observations are present values (with the required version or identity evidence) or absence. A recorded lookup holds the observed value or absence; a version or epoch may stand in only if it soundly identifies the result. valid(𝑆′,𝐷) ∶= ∏_{(𝑘,𝑜)∈𝐷}(Lookup(𝑆′,𝑘) ≡ 𝑜) (10.17).

- Snapshots: Running(𝑟,𝐷,𝑊) abbreviates Running(𝑟, 𝑆₀, 𝐷, 𝑊) for the retained snapshot 𝑆₀, and likewise Suspended. Lookups read 𝑆₀ and append their actual result to 𝐷. Resuming under the same snapshot needs no validation against an unrelated latest store; only reuse under a different snapshot requires the certificate to validate there. An invocation that has issued effects keeps 𝑆₀ unless a history-preserving migration is supplied.
- Scopes: the table holds a state per semantic key 𝑘 and effect scope 𝜔. Pure computations may share 𝜔₀. Effectful ones get a fresh invocation identity, and two consumers share a result only if they refer to the same effect occurrence 𝜔. validReuse(𝑆,𝐷,𝜔,𝜔′) ∶= valid(𝑆,𝐷) × (𝜔 = 𝜔′) (10.18).
- Running and suspended lookups are already indexed by (𝑘,𝜔), so they need no second scope check; their extra 𝑆 = 𝑆₀ guard is a snapshot condition. A new producer from Absent retains 𝑆. A completed result's certificate test guards reuse under 𝑆.
- For running or suspended producers, agreement on reads so far doesn't authorize a consumer with a different snapshot, since later reads may differ. So the join rule requires the consumer's snapshot to be 𝑆₀, and a different-snapshot join needs a proved simulation or a new pure producer. A suspended effectful invocation cannot restart against 𝑆.
- Completed states also compare their stored scope. A new effect occurrence gets a new scope even for identical source, and a pure store lookup creates no world-effect occurrence.

Transitions (10.19):

- demand(𝑘,𝜔,𝑆,𝑤):
  - Ready(𝑣,𝐷,𝜔′) with validReuse → deliver(𝑤,𝑣);
  - Failed(𝑒,𝐷,𝜔′) with validReuse → deliverError(𝑤,𝑒);
  - Running(𝑟,𝐷,𝑊) with 𝑆 = 𝑆₀ → Running(𝑟,𝐷,𝑊++[𝑤]);
  - Suspended(𝑟,𝐷,𝑊,𝑞) with 𝑆 = 𝑆₀ → Suspended(𝑟,𝐷,𝑊++[𝑤],𝑞);
  - Absent → Running(initial 𝑘, [], [𝑤]).
- step(𝑘,𝜔):
  - Running(𝑟,𝐷,𝑊) →^{lookup(𝑞)=𝑜} Running(𝑟′, 𝐷++[(𝑞,𝑜)], 𝑊);
  - Running →^{request(𝑞,𝜄)} Suspended(𝑟′,𝐷,𝑊,(𝑞,𝜄));
  - Suspended(𝑟,𝐷,𝑊,(𝑞,𝜄)) →^{response(𝑞,𝜄,𝑎)} Running(continue(𝑟,𝑎),𝐷,𝑊);
  - Running →^{return 𝑣} Ready(𝑣,𝐷,𝜔); deliverAll(𝑊,𝑣);
  - Running →^{fault 𝑒} Failed(𝑒,𝐷,𝜔); deliverAllError(𝑊,𝑒).

";" is one state update together with waiter notification. Requests carry a fresh per-occurrence token 𝜄 within the scope, and a response is accepted only for the matching outstanding token. A consumed or retired token cannot resume a continuation twice, and source-syntax equality never authorizes reuse of an effect occurrence.

**Snapshot changes and effect history.** An invocation evaluates against its retained immutable snapshot. A changed certificate may invalidate pure reuse under a later snapshot but never retroactively changes interactions already issued.

- With no world request yet, an invocation may restart from initial(𝑘) under the new snapshot, keeping every waiter; a proved incremental update is another permitted transition.
- After a request, it continues under the original snapshot or moves under a supplied simulation preserving issued effects, outstanding tokens and pending obligations. Restarting from the initial residual needs that same proof, and retiring a token doesn't undo a payment, send or write.
- A consumer needing the new snapshot, with neither reuse nor a preserving migration available, gets a typed invalidation result or waits for the specified migration, and the old effect is never reissued. Invalidation notifies every waiter while the outstanding interaction stays represented until completion, acknowledged cancellation or defined recovery.
- Stale pure entries can be recomputed. Effectful results stay records of their original occurrences, reusable only with matching scope and certificate. So the safe fallback for effectful work is not a pure producer's unconditional stale restart.

**Re-specializing an active invocation.** A JIT may specialize a continuing process further as static facts arrive. A new artifact 𝑎₁ may sit beside 𝑎₀, but replacing an active occurrence migrates its current continuation; it does not restart the original program.

- Boundary_𝑖(𝑘) is the set of target states of 𝑎ᵢ where the correspondence with source continuation 𝑘 is known, such as proved turn boundaries or suspended command/response points. LiveRel_𝑖(𝑑,𝑘,𝐻,𝑇,𝑆) says target state 𝑑 realizes 𝑘 with issued-effect history 𝐻, outstanding tokens 𝑇 and snapshot 𝑆.
- Swap_{𝐷₁}(𝑎₀,𝑎₁) ∶= ∏_{𝑘,𝐻,𝑇,𝑆} ∏_{𝑑₀∶Boundary₀(𝑘)} LiveRel₀(𝑑₀,𝑘,𝐻,𝑇,𝑆) → valid(𝑆,𝐷₁) → ∑_{𝑑₁∶Boundary₁(𝑘)} LiveRel₁(𝑑₁,𝑘,𝐻,𝑇,𝑆) (10.S1).
- 𝐻, 𝑇 and 𝑆 are deliberately kept. A migration changes representation or snapshot only with a separate simulation relating old and new observations and preserving issued effects. Responses to outstanding requests go to the new continuation under the same token and never trigger a second issue.
- An arbitrary instruction is not a safe swap point unless its source-continuation relation has been constructed. Turn and response boundaries are examples, not universal privileged cut points.
- If the specialization certificate later fails, an active effectful artifact uses a proved resume exit to a less specialized or evaluator-mediated realization. Every guard whose failure is reachable during execution either has such an exit or is proved invariant under permitted environment changes. Pure work may restart when discarded work is unobservable; issued effects are never replayed to rebuild a convenient initial configuration. This is the persistent-producer effect-history theorem applied to a program changing its own executable representation.

*Read-dependency lemma.* For a deterministic pure segment with complete lookup trace 𝐷, any 𝑆′ with valid(𝑆′,𝐷) produces the same segment until its next new lookup or world request. Proof: induction on steps, with store reads equal by (10.17) and other pure steps fixed by term and environment. It proves sound reuse, not a bound on recomputation. 𝐷 selects a fiber of stores agreeing on the recorded observations (§10.4), and the recorded reductions stay valid across that whole fiber without reconstruction. A certificate describes the applicability of retained work, not its storage location. These equations define this particular evaluator, and other strategies may use other representations.

**Producer requests, suspension, waiter delivery** are canonical in (10.16)–(10.19): a request to a running or suspended producer appends to its waiters; completed pure work is reused under a valid certificate; a snapshot change permits pure restart or a proved derivative; effectful invocations obey the effect-history and migration conditions; external requests resume only on their matching response. The maintained update follows from (10.2): if 𝑒 ∶ 𝑟 = 𝑞𝑑 and 𝜆_𝑑 ∶ 𝑣(𝑑, 𝑞𝑑) = 𝑞(𝑢𝑑), then maint(𝑑,(𝑟,𝑒)) ∶= (𝑣(𝑑,𝑟), ap(𝑣(𝑑,−), 𝑒) ⋅ 𝜆_𝑑) ∶ Mat(𝑞, 𝑢𝑑) (10.20). Without a specialized update, recompute 𝑞(𝑢𝑑) with refl; one dependent type certifies both branches.

Editing in a typed context 𝐾[−]:

1. inspect(𝐾[𝑡]) = (Γ_ℎ, 𝐴_ℎ, 𝑡, 𝐾);
2. check(Γ_ℎ, 𝐴_ℎ, 𝑡′) ⇝ 𝑑′ ∶ Der(Γ_ℎ ⊢ 𝑡′ ∶ 𝐴_ℎ);
3. plug(𝐾, (𝑡′,𝑑′)) ∶ Raw;
4. checkEnclosing ⇝ Code(Γ,𝐴).

The new code is a new immutable definition. Rebinding a name publishes it in a new snapshot, and read validation decides which retained computations survive. Editor, renderer, query and proof are definitions in the same space.
