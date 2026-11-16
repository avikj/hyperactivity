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
