---
title: "hyperactivity"
subtitle: "A decentralized operating system language for collective superintelligence"
date: "14 November 2026"
author: "Avik Jain"
version: 31
---

**Abstract.** Every description of the universe is an event in the universe. Its interpretation and use belong to the same reality as its subject; reasoning is part of what happens, not a standpoint outside what happens. This paper gives that self-inclusion a constructive form: routines, relations, and rationales operate on one another, including the definitions through which their own activity is interpreted. Established knowledge is transported through proved equivalences of representation and retained with the dependencies that govern its further use.

Beginning with states, actions, and interactions, we construct histories, observations, and evidence for the conclusions warranted by them. Incomplete information remains a family of possibilities, not a preferred history. An utterance changes the context in which later utterances are interpreted; the interpreter and its history are themselves within the account. Dependent types express a routine, its required property, and the rationale establishing that property in the same language. Computational univalence turns an equivalence into an identity that transports dependent operations and their proofs. The reasoning establishing a construction is retained for subsequent computation rather than transcribed into a second proof language. Sorting supplies a complete example: a sorted answer with its multiplicity evidence is constructed by elimination over a bag and the identities that bag carries.

Reflection extends this activity to the language's own definitions. Existing languages retain their specified syntax and semantics while their interpreters, checkers, and transformations become operands of further computation. Under the stated evaluator-adequacy and machine-simulation premises, one machine-realized evaluator executes every represented program, including these operations on programs. The chosen native route is through C and a downstream compiler, with the correctness relations composed across that route. Symbolic residuals retain unknown inputs; dependency evidence governs reuse; continuing interactions retain their history; and decentralized exchange makes constructions and their rationales available to other participants. *Hyperactivity* names this conductance of knowledge into further activity: a persistent, interactive language of processes that includes the work of understanding and changing them.

# 1. The universe and its descriptions

The universe is not one of the objects outside which its description takes place. It includes this text, its author and reader, the events through which its words acquired their uses, and the activity of interpreting them now. A description selects distinctions within that totality. An observer, an instrument, a language, and an act of reasoning do not supply successive external standpoints; each has its own history within the same subject.

The consequence concerns what a description must retain. An action depends on the information available to its participant; an interpretation depends on the context in which an expression is received; a conclusion depends on the relations and evidence establishing it. These dependencies determine what a description establishes about the activity, not merely how it is written. The constructions developed here make those dependencies explicit and available to further activity, including activity that changes the descriptions and interpretations themselves. This is the subject of *hyperactivity*: routines, relations, and rationales acting on one another, rather than code acted upon by an unexplained intelligence outside the account.

**Process, action, and information.** A state type \(S\), a family of available actions \(A(s)\), and a relation \(T(s,a,s')\) specify an action-labeled transition: a source state, an admitted action, a successor, and evidence of their relation. A deterministic update selects its successor; a nondeterministic description retains the admitted alternatives. An open interaction includes the response on which its continuation depends. A policy for selecting an action, the transition relating that action to its consequences, and an observation of those consequences are different constructions. Neither a scalar reward nor a probability distribution is implicit in merely specifying them.

This is the same subject whether the selected state belongs to an executing processor, an organism acting within its surroundings, or participants in a conversation. Their particular descriptions determine the transitions and observations involved. Composing participants internalizes selected interactions; changing the selected boundary changes which inputs remain open. Differential equations, stochastic laws when supplied, and guarded interaction give further descriptions of evolution. No particular transition presentation replaces those descriptions or makes a machine the origin of their significance.

Histories retain the admitted transitions and their relationships. A participant's available information, however, is not the entire history selected for description. That distinction is where inference enters: the question is what follows from the information actually obtained, not which unseen history the interpreter prefers.

Let \(H\) be the type of admissible histories of a specified process and \(o:H\to O\) an observation operation. A reading \(y:O\) does not identify a history; it determines the type of histories that could have produced that reading, with evidence of their agreement:

\[
\mathsf{Fib}_o(y):=\sum_{h:H}(o(h)=y).
\tag{1.1}
\]

The full history space can be recovered without loss when both the observed value and the corresponding history-with-evidence are retained:

\[
H\simeq\sum_{y:O}\mathsf{Fib}_o(y),\qquad
h\mapsto(o(h),h,\mathsf{refl}).
\tag{1.2}
\]

For a proposed conclusion \(P:H\to\mathcal U\), what that observation warrants is expressed by a construction of

\[
\mathsf{Warrants}(y,P):=\prod_{(h,p):\mathsf{Fib}_o(y)}P(h).
\tag{1.3}
\]

The observation supports a proposed conclusion exactly when the requisite dependent construction is available over its compatible histories. If the reading is known to have occurred, a compatible history is also part of that information; otherwise the dependent product may be inhabited vacuously because the fiber is empty. Refining the observation can shrink the compatible fiber, so a conclusion established for every history in a coarser fiber remains established for the finer one. These are relations between specified descriptions and their evidence. The observer's state, instrument, and interpretation may themselves be included in a larger history, and the same construction then applies to them.

The same construction applies when \(P\) states a condition for an action. Evidence over the compatible family establishes that condition without first selecting a hidden state. A response changes the family through the specified action-and-response relation (§10.4). These operations do not introduce probability to conceal missing information: they retain the missing distinctions and establish exactly what follows despite them. When a probabilistic description is supplied, its probability structure is additional information on which further reasoning operates.

**An observation does not inherit every property of what it observes.** The following elementary calculation isolates that distinction; it is an example of the account, not its definition.

**A reversible whole with no observed law.** Take \(J=\mathsf{Bool}\times\mathsf{Bool}\), the global step \(s(a,b)=(a\oplus b,b)\), and the observation \(o(a,b)=a\). The step is its own inverse: \(s(s(a,b))=(a,b)\). But there is no \(g:\mathsf{Bool}\to\mathsf{Bool}\) satisfying \(g(o(j))=o(s(j))\) for every \(j:J\). At \(j=(\mathsf{true},\mathsf{false})\), such a \(g\) would give \(g(\mathsf{true})=\mathsf{true}\); at \(j=(\mathsf{true},\mathsf{true})\), it would give \(g(\mathsf{true})=\mathsf{false}\). The contradiction proves that the visible coordinate has no autonomous next-state function under this observation, although the joint evolution loses nothing. The problem is not the absence of a global law but the failure of that law to *descend* through the observation. The example also makes the observer-relative content of a law precise: what matters is the existence of a commuting map on the observed states, not merely the existence of a law on the whole. The same question about descent recurs for source-language projections and distributed views.

**Language, history, and inherited knowledge.** Human knowledge is inherited through language. Words, their uses, and their etymological histories transmit distinctions that no present speaker originated alone. Taken across its meanings and history, a word opens onto the intellectual activity of the human superorganism: generations of interpretation, communication, and action remain implicated in its present use. That inheritance is distributed among participants and artifacts; it is not a stock of facts simultaneously available to a single knower. Language belongs inside the history under investigation, not in a fixed dictionary supplied before the investigation begins.

An utterance is a particularly direct instance. A participant produces an expression in one state; another receives it in a different state, interprets it, and responds. The interaction becomes part of the context of later expressions. What an expression means in this interaction therefore depends on the receiving participant, the retained history, and the interpretation in use. The account concerns those relationships, rather than a context-independent semantic payload imagined to travel inside the utterance. The study of conventions, language use, and symbolic grounding supplies relevant antecedents ([48](#ref-48)–[51](#ref-51)); here the participating interpreter is itself an evolving construction.

The claim has an elementary executable form. For a participant \(i\), let \(S_i\) retain its context and history, \(U\) be the received expressions, and \(A_i\) the possible interpreted responses. A reception is specified by a relation

\[
\mathsf{Receive}_i:S_i\to U\to S_i\to A_i\to\mathcal U.
\tag{1.4}
\]

At a particular state \(s\), the interpretations and updates compatible with an utterance \(u\) form \(\sum_{s':S_i}\sum_{a:A_i}\mathsf{Receive}_i(s,u,s',a)\). Another participant or a later state can yield a different family. Sending, delivery, and receipt are transitions of a larger process, and a history of those transitions changes the states on which later interpretations depend. A claim about what a participant understood must be established relative to its available history and the selected observation of its response. The account does not assume that an expression carries a context-independent semantic object, nor that different participants share one hidden interpretation.

The event being interpreted need not be a sentence: a gesture, an image, or an instrument reading enters through its specified observation and interaction. Their interpretation is not established merely by assigning a label. The relation to the participant's context determines what has been observed, what remains possible, and what is warranted. Natural-language interpretation and perception consequently belong to the process account itself, rather than entering as software applications after its foundations have been fixed.

**Routines, relations, and rationales.** The reasoning establishing a relation is itself a construction. A rationale takes evidence as input and constructs evidence as output; a routine transforms a relation or another routine; a relation specifies what a rationale establishes. Their mutual application continues when the operands are the definitions of the language or its interpreters. The point is not that three kinds of artifact coexist, but that an established relationship participates in subsequent computation with its actual dependencies intact.

For example, given \(f:A\to B\), a requirement is not the unqualified predicate *correct*. It is a chosen dependent family \(P:\prod_{a:A}B\to\mathcal U\), and the reasoning that establishes the requirement is a term

\[
q:\prod_{a:A}P(a,f(a)).
\tag{1.5}
\]

Correctness is relative to the selected \(P\), not a verdict attached to \(f\) in isolation. Constructing behavior that satisfies a requirement already includes the reasoning that establishes the requirement. Expressing that reasoning in the language of the construction preserves it for further use rather than discarding it or translating it into a separate verification model. Zero marginal proof-writing beyond the reasoning already required is the design objective. Tests observe selected executions, whereas \(q\) concerns every input in its stated domain. Correctness conditions and the reasoning required to establish them have a long programming-methodology lineage, including Dijkstra's treatment of program construction ([68](#ref-68)).

Computational identity gives this retained reasoning an action across representations. An equivalence \(e:A\simeq B\) supplies a path \(\mathsf{ua}(e):A=B\); transport along it transforms constructions dependent on the carrier. For a structured equivalence, the operations and the rationales for their laws move together (§4). An observation that forgets information, a refinement of behavior, and an equivalence of complete structures have different types and consequences. Retaining the actual relationship is what licenses the subsequent operation; calling two things “the same” supplies none of that evidence.

**The language acts on its own constructions.** Grammars, judgments, interpretations, and transformations are represented definitions. A language's interpreter is an operand of the same activity as the expressions it interprets; transforming its represented clauses changes the interpretation of subsequent inputs (§6.3). Existing source languages retain their specified meaning while their definitions and programs become available to richer operations. Their syntax, interpretation, transformed source, and machine realization are related by the constructed evidence, not by separate semantic authorities.

A machine is one particular process in this account. Under the evaluator-adequacy and machine-simulation premises of §9.2, a realization of one evaluator executes every represented term, including the represented interpreters and transformations. Direct compilation supplies other realizations with their own evidence. This is the implementation dimension of the same construction, not the premise from which the study of nature, interpretation, or rational action begins.

What has been established remains useful only where its dependencies remain satisfied. Retained observations govern reuse; unknown inputs remain in symbolic residuals; a continuing interaction retains its continuation and already-issued effects. Communicating a definition together with its dependencies and rationale makes that construction available to another participant. Persistence, interaction, and decentralized exchange in §10 express this continuing use of knowledge. *Conductance* names the availability of established relations to have consequences in further activity, rather than a second substance or a scalar objective imposed on intelligence.

The development proceeds from constructions and identity (§§2–4), through language definitions and reflection (§§5–6), to open machine processes and their realization (§§7–9), and finally to retained inference and distributed interaction (§10). The latter are instances of the general subject, not a change from nature to software. Each stage operates on constructions already introduced, including the language through which those operations are expressed.

## 1.1 Computational cubical foundations

The constructions just described are expressed through judgments of the form

\[
\Gamma\vdash A:\mathcal U_\ell,\qquad
\Gamma\vdash t:A,\qquad
\Gamma\vdash t\equiv u:A.
\]

A cubical context also contains interval variables and face conditions. A path is a term indexed by an interval coordinate with specified endpoints. The operations of composition, filling, Glue, and transport give computation rules for dependent paths; univalence turns an equivalence into an identification of types. Inductive and higher-inductive definitions introduce data and paths by their constructors. The clocked extension permits guarded continuing computation. The computational rules follow the CCHM cubical calculus [9], extended with clock-indexed guarded computation [44]. Their relevant operations and judgments are developed below. These rules specify how the language computes with the identities of its own constructions, including the relations used to carry reasoning across representations.

For a family \(P:A\to\mathcal U\), a path \(p:a=_A b\) gives a function \(\mathsf{transport}_P(p):P(a)\to P(b)\). For an equivalence \(e:A\simeq B\), univalence gives \(\mathsf{ua}(e):A=B\). Transport of a dependent object along this path computes the corresponding object over \(B\). These are operations of the language, not annotations attached to a different program.

## 1.2 Programs about programs

Represented terms form an inductive type `Code`, with an interpretation `eval-g` mapping codes to the terms they denote. The checker is itself a term: it analyzes the evaluator's values through a structured observation `%view`, checks their types and conversion, and returns its judgment. The specializer operates on these representations by normalization and equality-directed rewriting. Its results are admitted through the same checking relation as ordinary definitions. The initial code representation and checker are part of the bootstrap; a checked, context-and-type-indexed code family is a separate construction, developed in §6.1. Neither construction assumes unrestricted quotation of arbitrary semantic values.

A represented language specifies grammar, contexts, judgments, transitions, effects, and observations appropriate to that language. Interpretation, source transformation, compilation, and the construction of rationales are computations on these definitions. An interpreter is a routine; its relation to the language's specified behavior is a property; the derivation of that property is itself a program. The resulting equalities, equivalences, or refinements are available to subsequent computations. The source language's meaning is its specified interpretation, not a second interpretation privately assigned by the checker or compiler. This is why metacircularity concerns the whole activity of interpretation and transformation, not merely the fact that the checker can inspect its own syntax.

## 1.3 Generality and examples

A computation with open inputs remains a program; substituting known inputs specializes it. The historical notions of computability and effective procedure were established by Church and Turing ([1](#ref-1), [2](#ref-2)); the present language represents the descriptions and their interpretations as constructions available to further computation. Composing processes and handlers can internalize selected interactions while leaving others open (§7.6). The same composition can be examined at the scale of a function, machine, operating system, communicating fleet, organism, or interaction among speakers. The particular evolution and observation laws determine the content; none of these names marks a primitive boundary of what can be described.

The following sections develop the language's operations in a dependency order: types and relations, computational identity, equivalence and transport, represented languages and interpreters, reflection, machines and their realizations, and persistent, interactive, distributed activity. The worked examples are concrete calculations within this account. They do not turn its subject into a list of unrelated software facilities.

The operating-system scope is the whole continuing activity: a participant's retained definitions and observations, running processes, interaction with other participants, and the operations that change any of these. Personal and interpersonal computation are not separate foundations. Each participant begins with locally held state; exchange relates selected information and constructions without requiring a universal owner or a single authoritative view (§10.8). The subtitle's *collective superintelligence* concerns the exact, reusable reasoning available through these interactions: established conclusions retain the evidence and dependencies that govern their use, and those constructions are themselves available to further reasoning. The constructions below say what is retained, what is communicated, what a conclusion requires, and how that conclusion becomes available to further activity. They do not prescribe a particular visual interface.

## 1.4 Scientific context and antecedents

The scientific program of understanding nature through computation is developed at the scale of the universe in Wolfram’s *A New Kind of Science* ([64](#ref-64)); his later account of the ruliad considers computational descriptions and the place of observers within them ([65](#ref-65)). The present paper develops its constructions from computational cubical type theory and the specified interpretations of particular systems. Wolfram's work situates the scientific question; the formal dependencies of the results are stated in the results themselves.

The distinction between information received and possibilities left open has several established treatments. Shannon's communication theory analyzes information through messages and channels ([45](#ref-45)); Barwise and Seligman study information flow through classifications and distributed constraints ([46](#ref-46)); Pearl's causal models distinguish observational from interventional relationships ([47](#ref-47)); Barwise and Perry study situations and context-dependent content ([69](#ref-69)). These theories answer different questions. The fiber constructions of §§1–2 state exactly which histories remain compatible with a selected observation and what a dependent conclusion must establish over them.

Conventions, conversational implicature, and coordinated language use have established accounts in Lewis, Grice, and Clark ([48](#ref-48), [49](#ref-49), [50](#ref-50)); Harnad identifies the problem of grounding symbolic expressions in their interaction with the world ([51](#ref-51)). The reception relation in §1 specifies a changing interpreter and its causal history; it is not a derivation of a full theory of natural language from a single relation. The propositions-as-types correspondence connects logical inference with typed programs ([52](#ref-52)); the cubical operations of §§3–4 extend that setting with computational identity and transport. Cybernetics and control theory have long treated the observer and controller as participants in a changing system (Wiener and Ashby, [53](#ref-53), [54](#ref-54)); Maturana and Varela develop a related account of living organization and cognition ([71](#ref-71)). These works supply concrete accounts of communication, control, and living organization whose particular definitions can be examined in the same language. Their individual results are not premises for the meaning of a represented program.

# 2. Types, witnesses, and identity

The inference over histories in §1 depends on a more elementary fact: a type expresses what can be constructed, and dependent types express constructions whose form depends on their inputs. Identity is itself such a type, so a claim about two constructions can be retained as a value and used by later operations. Begin with an occurrence \(x\) of a type \(A\). An occurrence is not a label. Two occurrences may be distinguished by their positions in a construction even when they carry equal values. A pair \((x,x)\) has two positions, though the values in them agree. A finite family is a function out of a finite index type:

\[
\operatorname{Fin}(0):=\mathbf 0,\qquad
\operatorname{Fin}(n+1):=\mathbf 1+\operatorname{Fin}(n),
\qquad
v:\operatorname{Fin}(n)\to A.
\tag{2.1}
\]

Permuting the indices changes an enumeration but need not change the structure under consideration. For a set-like \(A\), an explicit enumeration is

\[
\operatorname{Enumeration}(A)
:=\sum_{n:\mathbb N}(A\simeq\operatorname{Fin}(n)).
\tag{2.2}
\]

A cardinality assertion forgets the chosen enumeration. For example, it can be expressed by truncating the evidence of an equivalence:

\[
\operatorname{HasCard}(A,n):=
\left\|A\simeq\operatorname{Fin}(n)\right\|.
\tag{2.3}
\]

Propositional truncation retains the existence of an equivalence but not a particular equivalence to compute with.

Types and terms are consequently not two species of artifact, one called data and one called code. The judgment

\[
\Gamma\vdash t:A
\tag{2.4}
\]

says that \(t\) is a construction of \(A\) in context \(\Gamma\). Function application, pairing, dependent sums and products, induction, and computation are operations on such constructions. A proposition is a type when its inhabitants are the evidence under discussion; a proof is an inhabitant. 


## 2.1 Construction, elimination, and the difference between data and property

Dependent types make the roles of routine, relation, and rationale explicit without dividing them into independent languages. The expression of a routine and the expression of the reasoning about it use the same term constructors and computation rules. A dependent pair stores an element together with a construction whose type depends on that element:

\[
\Sigma_{a:A}P(a),\qquad
\mathsf{fst}(a,p):=a,\quad \mathsf{snd}(a,p):=p:P(a).
\tag{2.5}
\]

A dependent function accepts each input with its corresponding output type:

\[
\Pi_{a:A}P(a),\qquad f(a):P(a).
\tag{2.6}
\]

A finite map, a vector indexed by its length, a parser returning a syntax tree indexed by its grammatical category, and a program paired with its correctness evidence are all particular instances. Dependent sums and products can express both executable data and evidence. 

Inductive types are specified by constructors and eliminators. The dependent-type-theoretic foundations are developed by Martin-Löf and the Calculus of Constructions of Coquand and Huet ([6](#ref-6), [7](#ref-7)). For naturals, the eliminator is primitive recursion, with its computational equations:

\[
\begin{aligned}
\mathsf{rec}_{\mathbb N}(z,s,0)&\longmapsto z,\\
\mathsf{rec}_{\mathbb N}(z,s,n+1)&\longmapsto
s(n,\mathsf{rec}_{\mathbb N}(z,s,n)).
\end{aligned}
\tag{2.7}
\]

This defines the function by its computation rules. A dependent property of the function and the construction establishing that property are terms of the same language. The reasoning required to produce the function with its intended behavior is therefore expressible as part of the function's construction, rather than discarded and later reconstructed in a separate notation.

The difference between a *proposition* and a general type matters. A proposition has at most one inhabitant up to equality; a type may carry many distinct values and nontrivial higher paths. When the only question is whether a witness exists, propositional truncation discards which witness it was. When a witness must be executed, transformed, or transported, discarding it is an irreversible change of information. The type discipline lets that choice be stated exactly.



## 2.2 Relations are constructions from operations and identity

A relation is a family of types constructed from operations and identities. On natural numbers, for example,

\[
 a\le b\;:=\;\sum_{c:\mathbb N}(a+c=b),\qquad
 a<b\;:=\;\sum_{c:\mathbb N}(a+\mathsf{suc}(c)=b).
\tag{2.8}
\]

These are witness-carrying versions of the order relations. Because addition on natural numbers has the relevant cancellation property, the witnesses are unique when they exist; the relations are propositions. One can instead take their propositional truncations without changing whether they hold. The relation is defined by addition, successor, dependent sum, and equality.

Transitivity is a function. Given witnesses \((c,p):a\le b\) and \((d,q):b\le e\), take \(c+d\) and compose the identities

\[
 a+(c+d)=(a+c)+d=b+d=e.
\tag{2.9}
\]

The equalities use associativity, \(p\), and \(q\). Reflexivity uses \(0\); antisymmetry follows from the properties of addition on \(\mathbb N\). 

The same construction pattern applies to a semantic judgment. For example, a big-step relation \(\mathsf{Eval}(e,v)\) can be an inductive family whose constructors are the evaluation rules; alternatively, when evaluation is a function, it can be defined as \(\llbracket e\rrbracket=v\). The two presentations may be related by an equivalence of their witness types, established by induction on derivations. A semantics is the defined structure and its equalities, not a second layer that has to be connected to mathematics afterward.

A representation of a relation is another object. If \(R:A\to B\to\mathcal U\), its complete space of instances is

\[
 \operatorname{Rel}(R):=\sum_{a:A}\sum_{b:B}R(a,b).
\tag{2.10}
\]

The relation over fixed carriers is the dependent family \(R:A\times B\to\mathcal U\), equivalently its total space equipped with both projections to \(A\) and \(B\). An equivalence of presentations must commute with those projections. A bare equivalence of total spaces forgets which inputs and outputs the witnesses relate.



# 3. Computational equality

The previous section treats identity as a construction; the next question is how that construction computes. The computational path operations used here are those of CCHM ([9](#ref-9)), grounded in cubical-set semantics ([10](#ref-10)); the distinction between identity and equivalence follows the univalent treatment of types ([8](#ref-8)).


For \(a,b:A\), the identity type \(a=_A b\) has reflexivity

\[
\mathsf{refl}_a:a=a.
\tag{3.1}
\]

In cubical type theory, a path is described by an interval parameter:

\[
\mathsf{Path}_A(a,b)
\;\ni\;p=\langle i\rangle p(i),
\quad p(0)=a,\quad p(1)=b.
\tag{3.2}
\]

The interval \(\mathbb I\) has endpoints \(0,1\), connections and reversal. The dependent path type allows the ambient type itself to vary with \(i\). Path application computes:

\[
(\langle i\rangle t)@r\;\longmapsto\;t[i:=r].
\tag{3.3}
\]

The important operation is not merely checking whether two printed expressions look alike. Given a family \(P:A\to\mathcal U\) and \(p:a=b\), equality acts on dependent constructions:

\[
\mathsf{transport}_P(p):P(a)\to P(b).
\tag{3.4}
\]

For a constant family, transport does nothing of interest. For a dependent family, it changes the type in which a value is situated and computes the corresponding change of representation.

One elementary but powerful fact is that retaining a value together with its equality witness does not add independent information:

\[
\operatorname{Sing}(a):=\sum_{x:A}(a=x),
\qquad
\operatorname{isContr}(\operatorname{Sing}(a)).
\tag{3.5}
\]

The center is \((a,\mathsf{refl}_a)\). A path \(p:a=x\) contracts \((x,p)\) to this center. Cubically, a contraction can be built using the interval connection \(i\wedge j\), so that the equality witness varies coherently with the point. This is a computational construction, not an informal appeal to uniqueness. Its dependent eliminator belongs to the same type-theoretic lineage as Martin-Löf’s identity types ([6](#ref-6)).

A particularly useful application is to an arbitrary function \(f:A\to B\). Define its graph as a dependent sum retaining both input and output with the proof of their relation:

\[
\operatorname{Graph}(f)
:=
\sum_{b:B}\sum_{a:A}(f(a)=b).
\tag{3.6}
\]

Then

\[
A\simeq\operatorname{Graph}(f).
\tag{3.7}
\]

The forward map sends \(a\) to \((f(a),a,\mathsf{refl})\); the inverse projects \(a\). The inverse-after-forward composite reduces to the identity, and the other composite is identified by singleton contraction. This **does not** make \(f\) invertible as a function \(A\to B\). It shows precisely what information must be retained for the representation of its action to be reversible.

This distinction recurs everywhere: information can be forgotten by a chosen observation without disappearing from the object being observed.


## 3.1 Cubical operations are actual computation

The interval in a CCHM-style calculus is generated by dimension names and operations \(r\wedge s\), \(r\vee s\), and \(1-r\), subject to De Morgan algebra equations. Faces constrain dimension expressions; for example \((i=0)\vee(j=1)\) describes a partial boundary. A path is a term depending on a dimension, not merely a certificate attached after two expressions have been evaluated.

The central filling operation is composition. In schematic form, for a family \(A(i)\), a partial element \(u\) defined on a face \(\varphi\), and an initial endpoint \(a_0\) compatible with that partial element,

\[
\mathsf{comp}^{i}A[\varphi\mapsto u]a_0:A(1).
\tag{3.8}
\]

The boundary condition is \(u(i=0)=a_0\) wherever \(\varphi\) holds. Composition fills the missing endpoint in a way compatible with the supplied boundary. The computation rules for dependent functions, pairs, paths, universes, and Glue types determine how this operation reduces. Those rules are part of the actual selected cubical calculus; the displayed schema does not replace them.

Why does this matter computationally? Ordinary substitution replaces a variable with an expression. Cubical substitution also replaces dimension variables with interval expressions. A dependent construction may change along a path of types, and composition determines the value required to make its boundary coherent. Thus equality is something the evaluator acts on, not an external proposition whose only operational role is to be accepted by a checker.

## 3.2 Identity is neither syntactic identity nor observational coincidence

There are at least three relations worth keeping distinct. Two expressions may be definitionally equal because they reduce to the same expression under the calculus. They may be propositionally equal because an inhabitant of their identity type is constructed. They may be observationally indistinguishable under some chosen experiment even when no equality of their complete structures has been established.

For example, the two functions

\[
f(n)=n+n,\qquad g(n)=2n
\tag{3.9}
\]

may be extensionally equal over natural numbers after proving the arithmetic identity, even if their syntax differs and their operational costs differ in a particular evaluator. Function extensionality permits a pointwise proof to induce a path of functions:

\[
\Bigl(\prod_{n:\mathbb N}f(n)=g(n)\Bigr)\to(f=g).
\tag{3.10}
\]

The same identity does not assert that two physical executions dissipate equal energy or take equal time. Those quantities must be represented as additional observations. An equality proof is powerful because it has an exact type; it is not a license to erase distinctions not covered by that type.

## 3.3 Equality of representations preserves complete information

The graph equivalence and its two inverse calculations were established in (3.6)–(3.7). They are used here without redefining them: a representation of a transition retains its source, target, and evidence, while an observation may project away any of those components.

For a transition relation \(T:S\to S\to\mathcal U\), a single step with its evidence is

\[
 \mathsf{Transition}:=\sum_{s:S}\sum_{s':S}T(s,s').
\tag{3.11}
\]

An execution history composes such steps with matching endpoints, finitely or by guarded continuation. An observation may retain its final state, event sequence, or scalar result; the observation map specifies exactly which historical distinctions it forgets. Whether a retained state suffices for subsequent behavior depends on the continuation and its observations, not on the name assigned to that state. Equivalent presentations of complete histories transport dependent constructions by univalence.

Thus equality of complete constructions, equality of selected observations, and equivalence of representations are different claims. The selected projection determines what it can distinguish, not what the language can express.

## 3.4 Dimension, face, composition, and path judgments

The cubical host extends the preceding judgments with dimensions \(\Psi\), face formulas \(\varphi\), and dependent types. The following judgments must be mutually defined:

\[
\begin{aligned}
&\Gamma;\Psi;\varphi\vdash A\ \mathsf{type},\\
&\Gamma;\Psi;\varphi\vdash t:A,\\
&\Gamma;\Psi;\varphi\vdash t\equiv u:A,\\
&\Gamma;\Psi;\varphi\vdash A\equiv B\ \mathsf{type}.
\end{aligned}
\tag{3.12}
\]

The interval is the free De Morgan algebra generated by dimension variables, modulo its equations. A face restriction is built from atomic equations of interval expressions by conjunction and disjunction. Write \(\Delta:=\Gamma;\Psi;\varphi\) and \(\Delta,i:=\Gamma;\Psi,i;\varphi\). Path introduction and elimination are:

\[
\begin{gathered}
\frac{\begin{gathered}
 \Delta,i\vdash t:A,\quad \Delta\vdash a_0:A[i:=0],\\
 \Delta\vdash a_1:A[i:=1],\quad
 t[i:=0]\equiv a_0,\quad t[i:=1]\equiv a_1
\end{gathered}}
{\Delta\vdash\langle i\rangle t:\mathsf{Path}_{i.A}(a_0,a_1)},\\[1.1em]
\frac{\Delta\vdash p:\mathsf{Path}_{i.A}(a_0,a_1)
 \quad\Psi\vdash r:\mathbb I}
{\Delta\vdash p@r:A[i:=r]}.
\end{gathered}
\tag{3.13}
\]

Their principal computation is \((\langle i\rangle t)@r\longmapsto t[i:=r]\).
Dependent composition requires a filling operation for every type former, with its face compatibility and boundary equations:

\[
\begin{gathered}
\Gamma;\Psi,i;\varphi\vdash u:A,\qquad
\Delta\vdash a_0:A[i:=0],\\
\Gamma;\Psi;\varphi\vdash u[i:=0]\equiv a_0:A[i:=0],\\
\Delta\vdash \mathsf{comp}^{\,i}A[\varphi\mapsto u] \,a_0:A[i:=1],\\
\Gamma;\Psi;\varphi\vdash
\mathsf{comp}^{\,i}A[\varphi\mapsto u] \,a_0
\equiv u[i:=1]:A[i:=1].
\end{gathered}
\tag{3.14}
\]

For \(\Pi\), composition is pointwise after transporting the argument contravariantly; for \(\Sigma\), compose the first projection, then compose the dependent second projection over the resulting first-component filler. For \(\mathsf{Path}\), composition is performed in its additional interval dimension. For universes and \(\mathsf{Glue}\), the computational univalence construction supplies the universe composition rules.

The displayed rules are selected computations of CCHM; the remaining Kan, universe, face, and Glue equations are those of the CCHM calculus of Cohen–Coquand–Huber–Mörtberg [9]. The simply typed evaluator is an interpretation of one fragment, not a definition of the cubical foundation.

# 4. Equivalence, univalence, and structure

Identity becomes a means of changing description when an established equivalence of representations induces a path along which dependent constructions can be transported. Univalence and its dependent computational action belong to the cubical foundation ([8](#ref-8), [9](#ref-9)); the programming consequence below is that operations and their laws can be transported together across a specified structural equivalence.


An equivalence \(e:A\simeq B\) consists of a function with contractible fibers (equivalently, suitable inverse data with coherence). Computational univalence supplies

\[
\mathsf{ua}:(A\simeq B)\to(A=B).
\tag{4.1}
\]

In a cubical presentation, this path is constructed using Glue types, and transport along it computes the equivalence's forward action, with the precise reduction behavior determined by the selected cubical rules:

\[
\mathsf{transport}_{X\mapsto X}(\mathsf{ua}(e),a)
=_{B} e(a).
\tag{4.2}
\]

The displayed equality is a path in \(B\) (the univalence computation law, often written \(\mathsf{ua}\beta\)); the particular cubical reduction rules determine when the equality computes judgmentally. A path between values is not an equivalence between types.

Consider length-indexed vectors:

\[
\operatorname{Vec}(A,n)\simeq
\bigl(\operatorname{Fin}(n)\to A\bigr).
\tag{4.3}
\]

The maps are `index` and `tabulate`. Their laws are

\[
\mathsf{index}(\mathsf{tabulate}(g),i)=g(i),\qquad
\mathsf{tabulate}(\lambda i.\mathsf{index}(v,i))=v.
\tag{4.4}
\]

These establish an equivalence, hence a path between the representation types. A dependent predicate such as

\[
\mathsf{Sorted}:\operatorname{Vec}(\mathbb Z,n)\to\mathcal U
\tag{4.5}
\]

can be transported to the function representation. So can an operation together with its proof of preserving sortedness. The transport is not merely an external conversion routine; it is an operation on the dependent construction.

The qualification *structure* is essential. A bare equivalence between carrier types does not automatically preserve a monoid multiplication, an order, a topology, or a memory model. The structured type must include its operations and laws:

\[
\operatorname{Monoid}
:=
\sum_{A:\mathcal U}\sum_{\mu:A\to A\to A}
\sum_{e:A}
\bigl(\mathsf{Assoc}(\mu)\times
\mathsf{LeftUnit}(\mu,e)\times
\mathsf{RightUnit}(\mu,e)\bigr).
\tag{4.6}
\]

An identification of monoid structures transports the operation and its laws. The structure identity principle relates identities of appropriately univalent structured objects to structure-preserving isomorphisms. It does not declare every carrier bijection to be a monoid isomorphism.

**Equality, equivalence, structured identity, and observation are distinct relations, each available for computation under its specified rules.**


## 4.1 A representation change acts on a dependent operation

Let \(V=\operatorname{Vec}(A,n)\), \(F=\operatorname{Fin}(n)\to A\), and \(e:V\simeq F\). Suppose we have a vector operation \(h:V\to V\) and a property \(P:V\to\mathcal U\) together with

\[
q:\prod_{v:V}P(v)\to P(h(v)).
\tag{4.7}
\]

Transport along \(\mathsf{ua}(e)\) gives the corresponding data over \(F\). Written using the explicit equivalence, the transformed operation is

\[
h_F:=e\circ h\circ e^{-1}:F\to F,
\tag{4.8}
\]

and the transformed predicate is

\[
P_F(w):=P(e^{-1}(w)).
\tag{4.9}
\]

The transported preservation proof has the concrete form

\[
q_F:\prod_{w:F}P_F(w)\to P_F(h_F(w)).
\tag{4.10}
\]

Given \(w\) and \(p:P(e^{-1}w)\), apply \(q\) to \(e^{-1}w\); the result has type \(P(h(e^{-1}w))\), which is identified with the desired output using the inverse laws. This is the ordinary hand-written derivation. Computational univalence internalizes the equivalence as an identification of types and provides the general dependent transport operation that performs the corresponding change across constructions.

The important case is not a single value conversion. It is an entire structure containing a carrier, operations, laws, programs, and proofs, transformed coherently. That is the difference between writing a conversion function and operating mathematically on representations.

### A dependent package transported by equivalence

For instance, let \(e:A\simeq B\) and let

\[
\mathsf{Package}(X):=
\sum_{f:X\to X}
\prod_{x:X}P_X(x)\to P_X(f(x))
\tag{4.11}
\]

for a specified family \(P_X\). The transport

\[
\mathsf{transport}_{\mathsf{Package}}(\mathsf{ua}(e)):
\mathsf{Package}(A)\to\mathsf{Package}(B)
\tag{4.12}
\]

produces the new operation and its dependent preservation evidence together. The exact computation follows from the cubical transport rules for the chosen family \(P\), which specifies the correspondence between the dependent predicates.


## 4.2 A graph modulo relabeling

Consider a finite graph on \(n\) nodes, represented by an adjacency relation

\[
G:\operatorname{Fin}(n)\to\operatorname{Fin}(n)\to\mathsf{Bool}.
\tag{4.13}
\]

A permutation \(\pi:\operatorname{Fin}(n)\simeq\operatorname{Fin}(n)\) acts by relabeling:

\[
(\pi\cdot G)(u,v):=G(\pi^{-1}u,\pi^{-1}v).
\tag{4.14}
\]

A graph shape can be presented by quotienting this action, using a set quotient when only the isomorphism class matters, or a higher groupoid presentation when automorphisms and their identities are to be retained. These choices are mathematically different: a set quotient discards higher identity information, while a groupoid retains it.

Suppose positions are stored as \(p:\operatorname{Fin}(n)\to\mathbb R^2\). Relabeling must transport positions too:

\[
(\pi\cdot p)(u):=p(\pi^{-1}u).
\tag{4.15}
\]

The adjacency relation, the positions, a selected node, and a proof of a graph property can all be bundled into one dependent structure. In a graph groupoid or a suitable univalent structured-graph type, a path representing relabeling acts on that bundle; a bare permutation of node indices does not by itself identify arbitrary indexed data. A user interface showing the graph and a graph algorithm computing a path are then different interpretations of the same object, with relabeling invariance expressed once.

The same issue appears when adjacency lists, matrices, and edge sets represent one graph. A matrix/list conversion must preserve the graph structure, not merely the number of bytes. Equivalence of structured graph representations can carry algorithms and their correctness evidence across the representation change.

## 4.3 Structured equivalence as the actual content of representation independence

Representation independence and parametricity have a separate established lineage, including Reynolds’s relational account of polymorphism ([36](#ref-36)). Here the transported structure is expressed using the dependent identity operations of the cubical language.

Suppose \(A\) and \(B\) are two representations of the same algebraic structure. A representation change consists of an equivalence \(e:A\simeq B\) and preservation of the operations that define the structure. For a binary operation \(\mu_A:A\to A\to A\), define

\[
 \mu_B(b_1,b_2):=
 e\bigl(\mu_A(e^{-1}(b_1),e^{-1}(b_2))\bigr).
\tag{4.16}
\]

The corresponding associativity proof is computed by applying \(e\) to the associativity path for \(\mu_A\), with the inverse laws cancelling the inserted conversions. The operation, its laws, and their representation change can be packaged into a dependent type and transported as a single value. The elementary conjugation calculation is the explicit action of that transport.

This includes program representations, but it does not depend on programming. An integer can be represented by an equivalence class of pairs of natural numbers, a rational by a normalized fraction or an equivalence class of numerator–denominator pairs, a polynomial by a coefficient sequence or a syntax tree, and a finite graph by adjacency data or an incidence relation. Each choice comes with the operations and equalities defining the represented structure. The mathematics of the object is what makes representation independence possible.

A quotient identifies specified distinctions; an operation on it must respect those identifications. Other observations may require a different representation. For a type \(X\) and an equivalence relation \(\sim\), a set quotient has point constructors \([x]\), path constructors \([x]=[y]\) for \(x\sim y\), and a set-truncation constructor. For a set-valued codomain \(B\), its elimination rule constructs a function from the quotient to \(B\) from a function \(X\to B\) that respects \(\sim\). Elimination into a higher type requires the corresponding coherence data; a higher inductive presentation can retain nontrivial path structure; higher-inductive semantics have been studied explicitly ([11](#ref-11)). The difference matters for graph symmetries: identifying graphs up to renaming as a set discards the automorphism witnesses that a groupoid-level representation can retain.

The computation of a representation change is therefore governed by the identity being asserted. A quotient, an equivalence, a path of structured types, and an observation are not competing theories of meaning. They are constructions with different specified equalities, composed in the same language.

## 4.4 Ordering: a declaration that computes

Sorting gives a complete example before any source language or machine is introduced. The question is to construct a list with a given finite bag's multiplicities, in the order supplied on its elements. The list and the evidence for that requirement are one dependent answer. This develops §9, “Ordering — what counting discards,” of *Respecting Equality* ([74](#ref-74)), including its declaration in Remark 9.15, together with the executable construction in `lib/sort.cub` ([75](#ref-75)).

### The order supplies the equality used by counting

Let \(A\) have a Boolean comparator \(\mathsf{le}:A\to A\to\mathsf{Bool}\), with reflexivity, totality, transitivity, and antisymmetry. Write \(x\le y\) for \(\mathsf{isTrue}(\mathsf{le}(x,y))\). The element-equality test is not an additional oracle:

\[
\mathsf{eqo}(x,y):=\mathsf{le}(x,y)\wedge\mathsf{le}(y,x),
\qquad
\mathsf{isTrue}(\mathsf{eqo}(x,y))\longrightarrow x=_A y.
\tag{4.17}
\]

The map on the right is antisymmetry; reflexivity gives the test for \(x=x\). Thus multiplicity and order use the same notion of element identity. Elements are inspected only through the comparator.

Use the higher-inductive bag presentation with points \(\mathsf{emp}\), \(\mathsf{sng}(a)\), and \(\mathsf{un}(X,Y)\), and with left and right units, associativity, and commutativity as path constructors. For example,

\[
\begin{aligned}
\mathsf{unitl}(X)&:\mathsf{un}(\mathsf{emp},X)=X,\\
\mathsf{assoc}(X,Y,Z)&:
 \mathsf{un}(\mathsf{un}(X,Y),Z)=\mathsf{un}(X,\mathsf{un}(Y,Z)),\\
\mathsf{comm}(X,Y)&:\mathsf{un}(X,Y)=\mathsf{un}(Y,X).
\end{aligned}
\tag{4.18}
\]

This is the presentation used by the construction below; its path data need not be discarded. For each \(a:A\), multiplicity is an elimination to \(\mathbb N\):

\[
\begin{aligned}
\mathsf{countB}(a,\mathsf{emp})&=0,\\
\mathsf{countB}(a,\mathsf{sng}(b))&=[\mathsf{eqo}(a,b)],\\
\mathsf{countB}(a,\mathsf{un}(X,Y))
 &=\mathsf{countB}(a,X)+\mathsf{countB}(a,Y).
\end{aligned}
\tag{4.19}
\]

Here \([\mathsf{true}]=1\) and \([\mathsf{false}]=0\). The path branches are the corresponding unit, association, and commutation paths for addition. List multiplicity \(\mathsf{count}(a,B)\) is the analogous recursion over the list.

Define \(\mathsf{AllLe}(x,B)\) to require that \(x\) is below every element of \(B\). Sortedness is the recursion

\[
\begin{aligned}
\mathsf{AllLe}(x,[]) &= \mathbf 1, &
\mathsf{AllLe}(x,y::B)&=(x\le y)\times\mathsf{AllLe}(x,B),\\
\mathsf{Sorted}([])&=\mathbf 1, &
\mathsf{Sorted}(x::B)&=\mathsf{AllLe}(x,B)\times\mathsf{Sorted}(B).
\end{aligned}
\tag{4.20}
\]

For \(X:\mathsf{Bag}(A)\), the complete requested answer is

\[
\begin{aligned}
G_X(B)&:=
 \left(\prod_{a:A}\mathsf{count}(a,B)=\mathsf{countB}(a,X)\right)
 \times\mathsf{Sorted}(B),\\
F(X)&:=\sum_{B:\mathsf{List}(A)}G_X(B).
\end{aligned}
\tag{4.21}
\]

The source calls these families `Gs` and `Fs`, using the decidable natural-number equality test for the displayed count equality. Its `eqb-sound` and `eqb-complete` connect the two presentations. The output contains a list, the multiplicity evidence for every element, and sortedness; projecting the list does not change what the full answer establishes.

### Merge and the uniqueness of the complete answer

The empty-list clauses return the other list. With both lists nonempty, merge compares their heads:

\[
\mathsf{merge}(x::X,y::Y)=
\begin{cases}
x::\mathsf{merge}(X,y::Y),&\mathsf{le}(x,y)=\mathsf{true},\\
y::\mathsf{merge}(x::X,Y),&\mathsf{le}(x,y)=\mathsf{false}.
\end{cases}
\tag{4.22}
\]

Every recursive call removes one head. In the first branch, \(x\le y\), and the sortedness of \(y::Y\) plus transitivity places \(x\) below its entire run. The sortedness of \(x::X\) already places \(x\) below \(X\). The second branch uses totality to obtain \(y\le x\) and is symmetric. Induction therefore supplies both laws:

\[
\begin{aligned}
\mathsf{count}(a,\mathsf{merge}(B,C))
 &=\mathsf{count}(a,B)+\mathsf{count}(a,C),\\
\mathsf{Sorted}(B)\times\mathsf{Sorted}(C)
 &\longrightarrow\mathsf{Sorted}(\mathsf{merge}(B,C)).
\end{aligned}
\tag{4.23}
\]

**Uniqueness.** Two sorted lists with the same multiplicities are equal. If just one is empty, the nonempty list's head has positive count on that side and zero on the other, a contradiction. Otherwise write the heads as \(x\) and \(y\). Positive count of \(x\) in the second list, whose every element is at least \(y\), gives \(y\le x\). The symmetric argument gives \(x\le y\), so antisymmetry identifies the heads. Cancelling their equal contributions to every count gives equality of the tails' multiplicities, and induction identifies the tails. This is `sorted-unique`.

The evidence in \(G_X(B)\) is propositional: natural-number equality is propositional, and each Boolean order test has at most one witness. The list equality therefore extends to equality of complete dependent answers:

\[
\mathsf{Fs\mbox{-}prop}:\prod_{X:\mathsf{Bag}(A)}
 \mathsf{isProp}(F(X)),
\qquad
\mathsf{isProp}(T):=\prod_{u,v:T}(u=v).
\tag{4.24}
\]

### The declaration is eliminated over the bag, including its paths

The empty answer is the empty list with its evidence. A singleton is already sorted. For union, merge the two list components and use (4.23) and the incoming multiplicity witnesses. These are the source's `mempty`, `msng`, and `munion`:

\[
\begin{aligned}
\mathsf{mempty}&:F(\mathsf{emp}),\\
\mathsf{msng}&:\prod_{a:A}F(\mathsf{sng}(a)),\\
\mathsf{munion}&:F(X)\to F(Y)\to F(\mathsf{un}(X,Y)).
\end{aligned}
\tag{4.25}
\]

The dependent eliminator also has a branch for every bag identity. On a path \(q:X=Y\), the result must be a dependent path in \(F(q(i))\), not an unrelated equality between lists. Each such fiber is a proposition by (4.24), so the already constructed endpoint answers determine the required dependent path. In the commutativity branch, for example,

\[
\begin{aligned}
&\mathsf{PathP}\bigl(i\mapsto F(\mathsf{comm}(X,Y)(i))\bigr)\\
&\qquad
 \bigl(\mathsf{munion}(u,v),\mathsf{munion}(v,u)\bigr)
\end{aligned}
\tag{4.26}
\]

is supplied by `isProp→PathP`. Unit and associativity branches use the same operation over their respective paths. The complete elimination gives

\[
\mathsf{msort}:\prod_{X:\mathsf{Bag}(A)}F(X).
\tag{4.27}
\]

Its point equations are the empty, singleton, and merge equations above. Its path equations ensure that the answer respects the identities of the input rather than treating an arbitrary binary-tree presentation as additional element order. The executable declaration closes as follows:

```text
msort : {o : Ord} → (X : Bag (El o)) → Fs X
msort X = msort-at (Fs-prop {o})
  (λ x0 x1 → isProp→PathP {lzero} {x0} {x1}) X
```

Here `msort-at` is precisely the dependent elimination with the point and path branches just given. Propositionality supplies coherence between answers; merge and the eliminator supply an answer. Together they make \(F(X)\) contractible, with center \(\mathsf{msort}(X)\) and contraction \(v\mapsto\mathsf{Fs\mbox{-}prop}(X)(\mathsf{msort}(X),v)\). The declaration, its construction, and its retained rationale are consequently one executable value. No enumeration of candidate permutations or later reconstruction of a separate proof is involved.

For a concrete instance, take the union of \(\{3\}\cup\{1\}\) and \(\{2\}\cup\{1\}\). The two recursive answers have lists \([1,3]\) and \([1,2]\); their merge is \([1,1,2,3]\). Its multiplicity evidence records two occurrences of \(1\), one each of \(2\) and \(3\), and zero for other naturals. The same bag presented with another association or order of unions has an identified complete answer, by the path branches, not by a convention that secretly chooses an enumeration of the bag.

### What ordering retains, and what a sum presentation costs

For \(n\) distinct elements, the fiber of list sorting over their sorted sequence consists of the \(n!\) enumerations. Retaining that permutation allows the input enumeration to be recovered; returning only the sorted list forgets it. For repeated values with multiplicities \(m_1,\ldots,m_r\), the number of distinct value sequences is instead \(n!/\prod_jm_j!\). The bag declaration intentionally keeps multiplicities without claiming to keep a particular original enumeration.

The predecessor also distinguishes the information determined by sorting from the work of a selected sum presentation. Let a binary sum tree \(t\) have singleton leaves, size \(n(t)\), and an output-production cost that charges the total run length at each merge:

\[
\begin{aligned}
n(\mathsf{one})&=1,&
 n(l\oplus r)&=n(l)+n(r),\\
C(\mathsf{one})&=0,&
 C(l\oplus r)&=C(l)+C(r)+n(l)+n(r).
\end{aligned}
\tag{4.28}
\]

Induction gives \(C(t)=\sum_{\text{leaves }a}\mathsf{depth}_t(a)\): each leaf is charged once by every union above it. For \(n=2^k\), the balanced tree has \(C=kn\); a chain has \(C=2+3+\cdots+n\). Thus four leaves give costs \(8\) and \(9\), while eight give \(24\) and \(35\). These are counts in the stated merge-output model, not an identification of output copies, comparisons, and evaluator rewrites.

For distinct elements, a merge of runs of sizes \(a,b\) resolves one of \(\binom{a+b}{a}\) interleavings. Their counts multiply over any sum tree:

\[
I(\mathsf{one})=1,\qquad
I(l\oplus r)=\binom{n(l)+n(r)}{n(l)}I(l)I(r)=n(l\oplus r)!.
\tag{4.29}
\]

The last equality follows inductively by cancellation of the two child factorials. The information \(\log_2(n!)\) is thus independent of the tree, while (4.28) depends on it. Binary comparisons have a worst-case lower bound \(\lceil\log_2(n!)\rceil\); balanced merging at \(n=2^k\) uses at most \(nk-n+1\). At \(n=8\), these are \(16\) and \(17\), respectively. Adjacent swaps measure a different quantity: the inversion count, lowered by exactly one when an adjacent out-of-order pair is corrected. The comparison identifies order; the swap changes an enumeration.

The tree's paths identify its value without making every evaluation cost identical. For powers of two, the root-to-leaf choices in a balanced tree are binary indices; a chain is the corresponding tally presentation. Once the input presentation and the elimination are given, the recursive merges are determined. This is the declaration's computational content: use the identities the object carries, construct the answer over them, and retain precisely the distinctions needed by the selected observation. The result, the information forgotten, and the work charged by a presentation remain separate, explicit objects.

# 5. Programming with languages and their semantics

The same constructions that describe histories and their properties can describe the *languages used to describe those histories*. A language definition comprises its source syntax, static judgments, interpretation, transitions, and observable behavior; these can be represented and computed with. Inductive syntax, compositional interpretation, operational semantics, and definitional interpreters have established foundations ([4](#ref-4), [5](#ref-5), [16](#ref-16), [27](#ref-27), [28](#ref-28)). The general construction comes first; a closed source-language calculation follows it.

## 5.1 Typed syntax, semantics, and source lifting

The following is a dependent presentation of represented syntax and semantics, not a second primitive evaluator. A bootstrap code datatype and an intrinsically checked-syntax family are different representations; the equations below describe the latter construction. A derivation-indexed syntax can be used for a specified language, but its completeness for a real source standard requires separate coverage evidence.

Fix a universe level \(\ell\), a core signature \(\mathcal K\), a finite context of term variables \(\Gamma\), dimension variables \(\Psi\), and a face condition \(\varphi\). Write \(J=(\Gamma;\Psi;\varphi)\). The four principal judgments are

\[
J\;\mathsf{ctx},\qquad
J\vdash A:\mathcal U_\ell,\qquad
J\vdash t:A,\qquad
J\vdash t\equiv u:A.
\]

A raw presentation has explicit constructors and binder sorts. A typed presentation includes a derivation rather than merely a claim that checking will succeed:

\[
\begin{aligned}
\mathsf{Ty}(J,\ell)&=\sum_{A:\mathsf{Raw}}\mathsf{Der}(J\vdash A:\mathcal U_\ell),\\
\mathsf{Code}(J,A)&=\sum_{t:\mathsf{Raw}}\mathsf{Der}(J\vdash t:|A|),\\
\mathsf{Sub}(\Delta,\Gamma)&=\prod_{x:A\in\Gamma}\mathsf{Code}(\Delta,A[\sigma_{<x}]),\\
\mathsf{Env}(\varnothing)&=1,\\
\mathsf{Env}(\Gamma,x:A)&=\sum_{\rho:\mathsf{Env}(\Gamma)}\llbracket A\rrbracket\rho.
\end{aligned}
\]

For de Bruijn variables the substitution algorithm is structural, with a lifted substitution under every binder:

```text
subst(Var(j), σ)          = σ[j]
subst(Lam(A, body), σ)    = Lam(subst(A, σ), subst(body, lift(σ)))
subst(App(f, x), σ)       = App(subst(f, σ), subst(x, σ))
subst(Pair(a, b), σ)     = Pair(subst(a, σ), subst(b, σ))
subst(PathLam(i, t), σ)  = PathLam(i, subst(t, σ))
subst(Comp(A, φ, u, a), σ) = Comp(subst(A, σ), φ[σ], subst(u, σ), subst(a, σ))
lift(σ)[0]               = Var(0)
lift(σ)[j+1]             = weaken(σ[j])
```

The displayed cases determine the recursion scheme; every additional constructor contributes its own arity, binder lifts, and dimension-substitution action. The semantic substitution lemma is
\[
\llbracket t[\sigma]\rrbracket\rho
\equiv
\llbracket t\rrbracket(\llbracket\sigma\rrbracket\rho),
\qquad
(t[\sigma])[\tau]\equiv t[\sigma\circ\tau].
\]

The first equation follows by induction on the derivation of \(t\); the second by induction on its syntax. Dimension substitution additionally transports the face restrictions and the boundary systems required by composition.

### Represented source languages

A source language is a record of syntax, static judgments, configurations, labeled transitions, and observations. The syntax is indexed by its own source contexts and types; source well-formedness is *not* assumed to coincide with cubical typing.

\[
\begin{aligned}
\mathsf{Language}_\ell=\sum_{\substack{\mathsf{Syn},\mathsf{Ctx}_L,\mathsf{Ty}_L,\\
                         \mathsf{Static},\mathsf{Conf},\mathsf{Cmd},\\
                         \mathsf{Step},\mathsf{Init},\mathsf{Observe}}}
 &\mathsf{Law}_L,\\
\mathsf{Static}&:\mathsf{Ctx}_L\to\mathsf{Syn}\to\mathsf{Ty}_L\to\mathcal U_\ell,\\
\mathsf{Conf}&:\mathsf{Syn}\to\mathcal U_\ell,\\
\mathsf{Cmd}&:\mathcal U_\ell,\qquad \mathsf{Reply}:\mathsf{Cmd}\to\mathcal U_\ell,\\
\mathsf{Step}_p&:\mathsf{Conf}(p)\to
 \mathsf{Return}+\mathsf{Fault}+\mathsf{Tau}(\mathsf{Conf}(p))
 +\sum_{c:\mathsf{Cmd}}(\mathsf{Reply}(c)\to\mathsf{Conf}(p)),\\
\mathsf{Init}&:(p:\mathsf{Syn})\to\mathsf{Input}(p)\to\mathsf{Conf}(p),\\
\mathsf{Observe}&:\mathsf{Trace}_L\to\mathsf{Obs}_L.
\end{aligned}
\]

For a nondeterministic language, \(\mathsf{Step}_p\) returns a set, family, or coinductive tree of admissible successors instead of a single successor. For an undefined operation, \(\mathsf{Fault}\) is interpreted according to the language's specified undefined-behavior contract; it is not silently identified with an arbitrary ordinary result. The laws include substitution and scope preservation, progress or specified fault, well-defined observations, and the chosen treatment of concurrency and environment responses.

The interpreter is the guarded unfolding of the transition relation:

\[
\begin{aligned}
\mathsf{interp}_L(p,x)&=\mathsf{unfold}_L(\mathsf{Init}(p,x)),\\
\mathsf{unfold}_L(s)&=\mathsf{case}\ \mathsf{Step}_p(s)\ \mathsf{of}\\
&\quad \mathsf{Return}(v)\mapsto\mathsf{return}(v),\\
&\quad \mathsf{Fault}(e)\mapsto\mathsf{fault}(e),\\
&\quad \mathsf{Tau}(s')\mapsto\mathsf{delay}(\mathsf{unfold}_L(s')),\\
&\quad \mathsf{Call}(c,k)\mapsto\mathsf{request}(c,\lambda r.\mathsf{unfold}_L(k(r))).
\end{aligned}
\]

The interpreter is a function in the host calculus. It has no privileged authority beyond its explicit definitions. Source-language syntax, static rules, and dynamic rules are ordinary inputs to metaprograms.

### Lifting, transformation, and projection

Let \(\mathsf{Well}_L(p)\) be the source well-formedness type. The lifted program is the source syntax with its source derivation, not an unverified host program:

\[
\begin{aligned}
\mathsf{Lift}(L)&=\sum_{p:\mathsf{Syn}_L}\mathsf{Well}_L(p),\\
\mathsf{meaning}_L(p)&=\mathsf{interp}_L(p),\\
\mathsf{Spec}_L(p)&=\mathsf{MeaningType}_L(p)\to\mathcal U_\ell,\\
\mathsf{Attached}_L(p)&=\sum_{P:\mathsf{Spec}_L(p)}P(\mathsf{meaning}_L(p)).
\end{aligned}
\]

Here \(\mathsf{Spec}_L(p)\) is schematic when \(\mathsf{meaning}_L(p)\) is a function or process: \(P\) is a dependent predicate over that precise type. For a function of input \(x\), a typical attachment has type

\[
\pi:\prod_{x:\mathsf{Input}(p)}Q(x,\mathsf{interp}_L(p,x)).
\]

A semantic transformation is a dependent function returning both target syntax and its semantic comparison:

\[
\mathsf{Trans}_{L,M}(p)=
\sum_{q:\mathsf{Lift}(M)}
\mathsf{Rel}_O(\mathsf{meaning}_L(p),\mathsf{meaning}_M(q)).
\]

If the relation is a path, transport maps specifications and proofs directly. If the relation is observational refinement, a property transfers only when it is closed under that refinement. The distinction is part of the transformation's type.

A structured proof attachment to conventional source is a pair \((p,\pi)\), where \(p\) is the source declaration and \(\pi\) is a host proof of a predicate over \(\mathsf{meaning}_L(p)\). Its source projection is

\[
\mathsf{erase}_L(p,\pi)=p,
\qquad
\mathsf{meaning}_L(\mathsf{erase}_L(p,\pi))
=\mathsf{meaning}_L(p).
\]

No source-language extension to runtime behavior is necessary to attach the proof. A source-language editor or parser can associate the proof with the declaration through a typed sidecar, a structured attribute, or a language-specific elaboration; the semantics of the association is the preceding definitions, independent of its concrete spelling.

The preceding dependent presentation specifies how source syntax, derivations, and interpretation are related. A smaller intrinsically typed language makes the construction and the substitution proof completely explicit. It is a *program defined in hyperactivity*, not an additional primitive language built into it.

### A complete intrinsically typed expression language

Fix a universe of simple types with finite products, sums, and function types:

\[
\begin{aligned}
A,B&::=\mathsf{Byte}\mid\mathsf{Unit}\mid A\times B\mid A+B\mid A\Rightarrow B,\\
\Gamma&::=\epsilon\mid\Gamma,A,\\
\mathsf{vz}&:\mathsf{Var}(\Gamma,A;A),\\
\mathsf{vs}&:\mathsf{Var}(\Gamma;B)\to\mathsf{Var}(\Gamma,A;B).
\end{aligned}
\]

Here \(\mathsf{Var}(\Gamma,A;B)\) abbreviates \(\mathsf{Var}((\Gamma,A),B)\), a variable of type \(B\) in the context \(\Gamma\) extended by \(A\); the semicolon is an abbreviation for context extension, not a third independent index. The variable family is generated by the two displayed constructors.
The intrinsically typed terms are generated by the following constructors (the semicolon denotes context extension):

\[
\begin{aligned}
\mathsf{var}&:\mathsf{Var}(\Gamma,A)\to\mathsf{Tm}(\Gamma,A),\\
\mathsf{lit}&:\mathsf{Byte}\to\mathsf{Tm}(\Gamma,\mathsf{Byte}),\\
\mathsf{lam}&:\mathsf{Tm}(\Gamma,A;B)\to\mathsf{Tm}(\Gamma,A\Rightarrow B),\\
\mathsf{app}&:\mathsf{Tm}(\Gamma,A\Rightarrow B)\to
              \mathsf{Tm}(\Gamma,A)\to\mathsf{Tm}(\Gamma,B),\\
\mathsf{pair}&:\mathsf{Tm}(\Gamma,A)\to\mathsf{Tm}(\Gamma,B)
              \to\mathsf{Tm}(\Gamma,A\times B),\\
\mathsf{fst}&:\mathsf{Tm}(\Gamma,A\times B)\to\mathsf{Tm}(\Gamma,A),\\
\mathsf{snd}&:\mathsf{Tm}(\Gamma,A\times B)\to\mathsf{Tm}(\Gamma,B),\\
\mathsf{inl}&:\mathsf{Tm}(\Gamma,A)\to\mathsf{Tm}(\Gamma,A+B),\\
\mathsf{inr}&:\mathsf{Tm}(\Gamma,B)\to\mathsf{Tm}(\Gamma,A+B),\\
\mathsf{case}&:\mathsf{Tm}(\Gamma,A+B)\to
   \mathsf{Tm}(\Gamma,A;C)\to\mathsf{Tm}(\Gamma,B;C)
   \to\mathsf{Tm}(\Gamma,C).
\end{aligned}
\]

Here \(\mathsf{Tm}(\Gamma,A;B)\) abbreviates
\(\mathsf{Tm}((\Gamma,A),B)\), i.e. a term in the context extended by \(A\).

Interpret types into host types, with \(\llbracket A\Rightarrow B\rrbracket=
\llbracket A\rrbracket\to\llbracket B\rrbracket\).
Environments and lookup are defined recursively:

\[
\begin{aligned}
\mathsf{Env}(\epsilon)&:=\mathsf{Unit},&
\mathsf{Env}(\Gamma,A)&:=\mathsf{Env}(\Gamma)\times\llbracket A\rrbracket,\\
\mathsf{lookup}(\mathsf{vz},(\rho,v))&:=v,&
\mathsf{lookup}(\mathsf{vs}(i),(\rho,v))&:=\mathsf{lookup}(i,\rho).
\end{aligned}
\]

### Interpretation by structural recursion

\[
\begin{aligned}
\mathsf{eval}(\mathsf{var}(i),\rho)
  &:=\mathsf{lookup}(i,\rho),\\
\mathsf{eval}(\mathsf{lit}(b),\rho)&:=b,\\
\mathsf{eval}(\mathsf{lam}(t),\rho)
  &:=\lambda v.\mathsf{eval}(t,(\rho,v)),\\
\mathsf{eval}(\mathsf{app}(f,x),\rho)
  &:=\mathsf{eval}(f,\rho)(\mathsf{eval}(x,\rho)),\\
\mathsf{eval}(\mathsf{pair}(x,y),\rho)
  &:=(\mathsf{eval}(x,\rho),\mathsf{eval}(y,\rho)),\\
\mathsf{eval}(\mathsf{fst}(p),\rho)&:=\mathsf{fst}(\mathsf{eval}(p,\rho)),\\
\mathsf{eval}(\mathsf{snd}(p),\rho)&:=\mathsf{snd}(\mathsf{eval}(p,\rho)),\\
\mathsf{eval}(\mathsf{inl}(x),\rho)&:=\mathsf{inl}(\mathsf{eval}(x,\rho)),\\
\mathsf{eval}(\mathsf{inr}(y),\rho)&:=\mathsf{inr}(\mathsf{eval}(y,\rho)),\\
\mathsf{eval}(\mathsf{case}(s,l,r),\rho)
  &:=
  \begin{cases}
   \mathsf{eval}(l,(\rho,v)),&\mathsf{eval}(s,\rho)=\mathsf{inl}(v),\\
   \mathsf{eval}(r,(\rho,w)),&\mathsf{eval}(s,\rho)=\mathsf{inr}(w).
  \end{cases}
\end{aligned}
\]

These equations are structurally recursive terms of the host calculus. A concrete evaluator may represent the function value in the lambda case as a closure \((t,\rho)\), with application extending its stored environment.

### The substitution theorem

A renaming is a family of typed variable maps
\(\sigma:\prod_A\mathsf{Var}(\Gamma,A)\to\mathsf{Var}(\Delta,A)\).
A substitution is a family of typed terms
\(\theta:\prod_A\mathsf{Var}(\Gamma,A)\to\mathsf{Tm}(\Delta,A)\).
The lift of \(\theta\) under a binder maps the new variable to itself and
each old variable to the weakened image of \(\theta\):

\[
\begin{aligned}
\mathsf{lift}(\theta)(\mathsf{vz})&:=\mathsf{var}(\mathsf{vz}),\\
\mathsf{lift}(\theta)(\mathsf{vs}(i))
  &:=\mathsf{weaken}(\theta(i)),\\
\mathsf{subst}(\theta,\mathsf{var}(i))&:=\theta(i),\\
\mathsf{subst}(\theta,\mathsf{lam}(t))
  &:=\mathsf{lam}(\mathsf{subst}(\mathsf{lift}(\theta),t)).
\end{aligned}
\]

For every other constructor in the preceding equations, substitution recurses into its arguments; for each branch of \(\mathsf{case}\), it uses \(\mathsf{lift}(\theta)\). The semantic substitution environment is the unique environment whose lookup at each typed variable is the interpretation of its substitution image:

\[
\mathsf{lookup}(i,\mathsf{evalEnv}(\theta,\rho))
:=\mathsf{eval}(\theta(i),\rho).
\]

Its binder law is an equality of extended environments, not just a statement about their newest entry:

\[
\mathsf{evalEnv}(\mathsf{lift}(\theta),(\rho,v))
= (\mathsf{evalEnv}(\theta,\rho),v).
\]

For the newest variable, both sides look up to \(v\); for an older variable \(\mathsf{vs}(i)\), weakening and the evaluation lemma for weakening reduce the lookup to \(\mathsf{eval}(\theta(i),\rho)\). Environment extensionality establishes the displayed equality. This is the precise lemma used in the lambda and sum-branch cases below.

**Substitution theorem.**

\[
\boxed{
\mathsf{eval}(\mathsf{subst}(\theta,t),\rho)
=
\mathsf{eval}(t,\mathsf{evalEnv}(\theta,\rho)).
}
\]

**Derivation.** Induction on the constructors in the preceding equations. In the variable case, both sides reduce to the defining lookup equation. For application, the induction hypotheses on the function and argument give equality after application congruence. In the lambda case, extend both environments with an arbitrary \(v:\llbracket A\rrbracket\); the displayed binder law identifies the lifted substitution environment with the original substituted environment extended by \(v\), and the induction hypothesis gives equality of results. Function extensionality concludes equality of the closures. Product and sum cases follow by their respective constructors and eliminators. These are all constructors of the stated expression grammar.

## 5.2 A language as a definition

Fix a small language of arithmetic expressions with variables. Its syntax is an inductive family:

\[
\begin{aligned}
\mathsf{Expr}(\Gamma)&::=
  \mathsf{var}(i)\ (i:\Gamma)
  \mid \mathsf{num}(n)\ (n:\mathbb N)\\
 &\quad\mid \mathsf{plus}(e_1,e_2)
  \mid \mathsf{let}(e_1,e_2).
\end{aligned}
\tag{5.1}
\]

The second argument of `let` is in the context extended by one variable. Environments assign natural numbers to the variables of a context. The evaluator is the unique structurally recursive function determined by the constructors:

\[
\begin{aligned}
\llbracket\mathsf{var}(i)\rrbracket\rho&:=\rho(i),\\
\llbracket\mathsf{num}(n)\rrbracket\rho&:=n,\\
\llbracket\mathsf{plus}(u,v)\rrbracket\rho
 &:=\llbracket u\rrbracket\rho+\llbracket v\rrbracket\rho,\\
\llbracket\mathsf{let}(u,v)\rrbracket\rho
 &:=\llbracket v\rrbracket(\rho,\llbracket u\rrbracket\rho).
\end{aligned}
\tag{5.2}
\]

These equations define this interpretation, including the environment supplied to `let`. Meaning here is computed from syntax and environment under the selected definition; it is not an intrinsic property of the printed expression.

Now consider two syntactic representations. The first has a `let` constructor; the second uses only variables, numerals, addition, and explicit substitution. Define a translation \(D\) by replacing

\[
D(\mathsf{let}(u,v))
:=\mathsf{subst}_{0}(D(u),D(v)),
\tag{5.3}
\]

where \(\mathsf{subst}_{0}\) is capture-avoiding substitution for the newest variable, shifting older variables correctly. The theorem is

\[
\prod_{\Gamma,e,\rho}
\llbracket D(e)\rrbracket\rho
=
\llbracket e\rrbracket\rho.
\tag{5.4}
\]

The `let` case uses the semantic substitution theorem established in §5.1; the other cases are immediate from (5.2) and the induction hypotheses. This is already a complete language transformation, obtained by ordinary recursion and induction. It is not a compiler subsystem that needs to be installed into mathematics.

But the transformation is generally **not** an equivalence of syntax types: eliminating `let` forgets how the source expression was written. It is semantics-preserving. If source positions, sharing, or execution costs are part of the intended observation, the equality in (5.4) must be strengthened or replaced by an appropriate relation. The mathematics identifies exactly what is preserved.

The distinction generalizes to language translation. A parser maps a text representation to syntax and may reject malformed text. An elaborator maps surface syntax and a context to a typed term, perhaps with implicit arguments resolved. An interpreter maps a term to a semantic construction. A compiler maps one representation to another with an explicit behavioral relation. Each map is a function or relation that can itself be defined, evaluated, transformed, and proved about. **There is no universal necessity that these maps be composed in a fixed order.**

For this example the context is a finite set of variable positions, \(\Gamma=\mathsf{Fin}(n)\), and extension is \(\Gamma^+=\mathbf1+\Gamma\). The substitution in (5.3) is an instance of the general substitution in §5.1 sending the newest variable to \(u\) and the old variables to themselves. Its semantic law is exactly the \(\mathsf{let}\) case of (5.4): the interpreted substituted body receives \((\rho,\llbracket u\rrbracket_\rho)\). This is the inductive proof, not an additional translation or a second definition of substitution.

**Reusing a theorem.** A proved statement is a dependent function from its assumptions to its conclusion. If a library theorem has type \(h:\prod_{a:A}P(a)\to Q(a)\), importing it into another construction consists of supplying \(a:A\) and \(p:P(a)\), then applying \(h(a,p):Q(a)\). The assumptions are retained in the type. A change of representation uses the previously defined transport operation on the complete dependent construction; there is no separate shortcut that discards hypotheses.

## 5.3 The source program and its entire semantic family

A language's semantics need not be a single function returning a value. For a language with effects and nondeterminism, let \(\mathsf{Cfg}\) be configurations and \(\mathsf{Event}\) observable labels. A small-step semantics is an inductively generated relation

\[
\mathsf{Step}:\mathsf{Cfg}\to\mathsf{Event}
   \to\mathsf{Cfg}\to\mathcal U.
\tag{5.5}
\]

Its finite traces are the reflexive transitive closure with labels; its continuing behaviors require a coinductive or guarded construction. A program's possible observations are the image of its admitted executions under a chosen observation map:

\[
\mathsf{Behavior}(p,o):=
\left\|\sum_{\tau:\mathsf{Execution}(p)}
  \mathsf{observe}(\tau)=o\right\|.
\tag{5.6}
\]

Here the truncation says only that an execution with observation \(o\) exists; retain the untruncated type when witnesses or distinctions between executions matter. A probabilistic language would retain probability structure rather than merely a predicate of possible outcomes.

Syntax, interpretation, execution, and proofs about execution are distinct but related constructions in the same language. Their definitions can themselves be inputs to programs, including programs that change the interpretation used for subsequent expressions (§6.3). A fixed interpreter is one program, not a privileged boundary of meaning.


## 5.4 A dependent presentation of an existing language

A language is not specified by a bare set of syntax trees. Its static and dynamic judgments are indexed by the contexts and objects to which they apply. The following is an internal *presentation type* in the cubical host, with universe levels suppressed:

\[
\begin{aligned}
\mathsf{Lang}:=\sum_{\mathsf{Src}:\mathcal U}
\sum_{\mathsf{Ctx}:\mathcal U}
\sum_{\mathsf{Ty}:\mathsf{Ctx}\to\mathcal U}
\sum_{\mathsf{Tm}:\prod_{\Gamma:\mathsf{Ctx}}\mathsf{Ty}(\Gamma)\to\mathcal U}
\sum_{\mathsf{Cfg}:\mathsf{Src}\to\mathcal U}
\sum_{\Sigma:\mathsf{Signature}}
\mathsf{Sem}(\mathsf{Src},\mathsf{Ctx},\mathsf{Ty},\mathsf{Tm},\mathsf{Cfg},\Sigma).
\end{aligned}
\tag{5.7}
\]

Here \(\mathsf{Src}\) retains original source representations, including distinctions needed for preprocessing, binding, locations, and faithful projection. The family \(\mathsf{Tm}(\Gamma,A)\) contains checked expressions. The dependent record \(\mathsf{Sem}\) contains the source-to-checked elaboration judgment, configuration initialization, labeled transition relation or guarded transition function, environmental command/response types, and the observation map on executions. A standard with nondeterminism has a relation of admissible transitions; an implementation-defined choice is represented by its parameter or its allowed alternatives, not silently resolved by a deterministic interpreter. This is a dependent presentation *schema*, not a new primitive type former.

For \(L:\mathsf{Lang}\), interpreting a checked program is ordinary host computation. The fact that the host can form a term \(\llbracket p\rrbracket_L\) and a dependent property \(P(\llbracket p\rrbracket_L)\) follows by constructing and applying the represented interpreter; it requires no new proof judgment. Fidelity to a published source standard is a separate, explicit adequacy relation between that standard's derivations and the represented judgments. The relation can be established by translations in both directions with their side conditions and round-trip properties. This distinguishes *being able to express a language's definition* from claiming fidelity for an unspecified or partial definition.

For a source file \(b:\mathsf{Src}_L\), a faithful lifting result contains its checked representation \(p\), elaboration evidence, and a source projection that recovers \(b\) when no source rewrite was requested. A host annotation is indexed by \(p\) and interpreted as a host term. Erasing a proof-only annotation is therefore a projection of a dependent pair; compiling a rewritten program requires the additional semantic relation for that rewrite. The source and its meaning remain available as separate, connected constructions rather than being conflated.

## 5.5 Source-language semantics as an inductive construction

A language's syntax and static judgments are inductive constructions; its dynamic semantics is an indexed transition relation or guarded process. For a chosen edition and environment parameter \(\theta\), let \(\mathsf{Program}_\theta\), \(\mathsf{Well}_\theta(p)\), and \(\mathsf{Config}_\theta(p)\) be the corresponding types. A transition is a term of

\[
\mathsf{Step}_{\theta,p}:\mathsf{Config}_\theta(p)
\to\mathsf{Label}_\theta\to\mathsf{Config}_\theta(p)\to\mathcal U.
\tag{5.8}
\]

Finite executions are the inductively generated reflexive transitive closure with labels; continuing executions use the clocked cubical guarded process construction of §7.6, with its explicit later modality and clock quantification ([44](#ref-44)). Source behavior is the dependent family of executions and their observations, with truncation applied only when the property under consideration forgets witness identity. The deterministic command/response instance is defined below by a single signature and its guarded unfolding. For languages with unspecified evaluation orders, scheduling, or other nondeterminism, use the relation \(\mathsf{Step}\) rather than selecting an arbitrary successor.

Source syntax, typing, transitions, and observation are data. An encoding of a source-language standard is complete relative to that standard when its source derivations and represented derivations are related by the explicitly proved adequacy equivalence given below. The standard's side conditions are part of its rule instances, not annotations left to prose.


### Guarded transition semantics and worked memory interpreter


The following development makes the source-language interface an inductive object rather than an unspecified function named `interpret`. It is parameterized by an explicit signature; the source language's rules are data supplied to the construction. A language with nondeterministic behavior uses a branching transition family instead of a single `next` function.

### A typed command signature and guarded interpreter

Fix universes `Type l`, source configurations `C`, result type `A`, fault type `F`, and a command signature `Σ : Signature` with fields `Cmd : Type l` and `Reply : Cmd → Type l`. Define the one-step functor and its guarded unfolding:

```agda
record Signature : Type (lsuc l) where
  field Cmd   : Type l
        Reply : Cmd → Type l

record Machine (Σ : Signature) (C A F : Type l) : Type (lsuc l) where
  field
    next : C → D_Σ(C,A,F)
```

The dependent sum in `D_Σ` is defined explicitly as follows:

\[
D_\Sigma(C,A,F)=A+F+C+\sum_{c:\mathsf{Cmd}}(\mathsf{Reply}(c)\to C).
\tag{5.9}
\]

Define a guarded process by the final-coalgebra-style equations

\[
\begin{aligned}
\mathsf{Proc}_\Sigma(A,F)&\cong A+F+\triangleright\mathsf{Proc}_\Sigma(A,F)
 +\sum_{c:\mathsf{Cmd}}(\mathsf{Reply}(c)\to\triangleright\mathsf{Proc}_\Sigma(A,F)),\\
\mathsf{unfold}(\delta,s)&=\begin{cases}
\mathsf{done}(a)&\delta(s)=\mathsf{in}_0(a),\\
\mathsf{fault}(f)&\delta(s)=\mathsf{in}_1(f),\\
\mathsf{tau}(\mathsf{next}(\delta,s'))&\delta(s)=\mathsf{in}_2(s'),\\
\mathsf{call}(c,\lambda r.\mathsf{next}(\delta,k(r)))&\delta(s)=\mathsf{in}_3(c,k).
\end{cases}
\end{aligned}
\tag{5.10}
\]

Here `next` on the right denotes the guarded recursive occurrence, not a second machine function. Its type carries the delay modality. The definition uses guarded corecursion; an ordinary unguarded inductive tree would not represent indefinitely interacting programs.

A source language `L` is a record containing *source syntax and static evidence*, a configuration family, an initial configuration, and the machine step function. Its semantics is not a second, primitive notion:

```agda
record Language : Type (lsuc l) where
  field
    Source    : Type l
    Well      : Source → Type l
    Input     : (p : Source) → Type l
    Result    : (p : Source) → Type l
    Fault     : (p : Source) → Type l
    Config    : (p : Source) → Type l
    Commands  : Signature
    init      : (p : Source) → Well p → Input p → Config p
    step      : (p : Source) → Config p →
                  D_Commands (Config p) (Result p) (Fault p)

meaning : (L : Language) (p : L.Source) (w : L.Well p)
        → L.Input p → Proc L.Commands (L.Result p) (L.Fault p)
meaning L p w x = unfold (L.step p) (L.init p w x)
```

The syntax, typing, initialization, and step clauses of an existing language are therefore values of one record. `meaning` is the single defined host function obtained by guarded unfolding. A complete formal semantics for the source supplies those record fields. The record does not itself supply or validate a missing semantic clause. For languages with choice, replace `step` by a relation or branching structure and `Proc` by its guarded branching analogue. The distinction is essential for C++ unspecified evaluation order, weak memory, environment responses, and undefined behavior.

### Dependent properties of interpreted programs

For a source program `p` with source derivation `w`, write `M = meaning L p w`. A property of its complete behavior is a dependent family indexed by input:

\[
\begin{aligned}
\mathsf{Property}(L,p,w)
  &:=\prod_{x:\mathsf{Input}(p)}
       (\mathsf{Proc}_{\mathsf{Commands}}(\mathsf{Result}(p),\mathsf{Fault}(p))\to\mathcal U),\\
\mathsf{Proof}(L,p,w,P)
  &:=\prod_x P(x)(M(x)),\\
\mathsf{Annotated}(L,p,w)
  &:=\sum_{P:\mathsf{Property}(L,p,w)}\mathsf{Proof}(L,p,w,P).
\end{aligned}
\tag{5.11}
\]

There is no new source-language proof judgment. An annotation is a cubical term whose type mentions the source program's interpretation. If a conventional source file is unchanged, its semantics is unchanged; the attached proof is a separate dependent object. If a transformation changes the source program, the property transfers by the exact equality or observation-respecting map supplied by the transformation, not by assuming every source rewrite is valid.

### An exact memory-interaction instance

Consider a byte-memory language with commands `Read : Addr → Cmd` and `Write : Addr → Byte → Cmd`, whose replies are `Byte` and `Unit` respectively. Its source instructions are

```text
Stmt ::= Skip | ReadInto(Reg,Addr) | Write(Addr,Expr)
       | Seq(Stmt,Stmt) | If(Expr,Stmt,Stmt)
Expr ::= Const(Byte) | Reg(Reg) | Xor(Expr,Expr)
```

A configuration is `(stmt, registers, continuation)`. The memory belongs to the command handler; this representation has exactly one owner of mutable memory. Define evaluation of pure expressions structurally and source steps by the following equations (here `k` is a continuation stack):

```text
step(Skip, ρ, [])                  = Done(ρ)
step(Skip, ρ, s :: k)              = Tau(s, ρ, k)
step(Seq(s,t), ρ, k)               = Tau(s, ρ, t :: k)
step(If(e,s,t), ρ, k)              = Tau((if eval(e,ρ)≠0 then s else t),ρ,k)
step(ReadInto(r,a), ρ, k)          = Call(Read(a), λb.(Skip,ρ[r↦b],k))
step(Write(a,e), ρ, k)             = Call(Write(a,eval(e,ρ)), λ_.(Skip,ρ,k))
eval(Const(b),ρ)                  = b
eval(Reg(r),ρ)                    = ρ(r)
eval(Xor(a,b),ρ)                  = eval(a,ρ) xor eval(b,ρ)
```

The handler state is \(\mu\). It responds to `Read(a)` with \(\mu[a]\), and to `Write(a,b)` by updating its state to \(\mu[a\mapsto b]\) and returning `Unit`. The source continuation carries no memory. Thus each write occurs exactly once, and the resulting trace is determined by the handler equations.

For the program

```text
Seq(ReadInto(r,a), Write(b,Xor(Reg(r),Const(0x5a))))
```

and a world in which `Read(a)` returns `0x35`, the unique finite execution has a `Read(a)` request, then a `Write(b,0x6f)` request, then `Done`. This is an example of the source-language interpreter reducing an existing-language program to an explicit process, rather than a claim that its syntax has become a host primitive. A proof of its write value is an inhabitant of the proposition that the observed `Write` command carries `0x6f` under the given read response.


### Rule coverage and adequacy as checkable data

A source-language formalization must distinguish a *presentation of the rules* from a proof that the host encoding faithfully represents their derivations. Let \(\mathsf{Judg}\) be the source judgments. A rule signature consists of a family of rule instances, with a finite list of premise judgments for each instance:

\[
\begin{aligned}
\mathsf{RuleSig}&:=
  \sum_{\mathsf{Inst}:\mathsf{Type}}
  \bigl(\mathsf{concl}:\mathsf{Inst}\to\mathsf{Judg}\bigr)
  \times\bigl(\mathsf{prem}:\mathsf{Inst}\to\mathsf{List}(\mathsf{Judg})\bigr),\\
\mathsf{All}(D,[])&:=\mathbf 1,\\
\mathsf{All}(D,J::Js)&:=D(J)\times\mathsf{All}(D,Js),\\
\mathsf{StdDeriv}(J)&::=
  \mathsf{infer}(r,\vec d)
  \quad\bigl(r:\mathsf{Inst},\
  \mathsf{concl}(r)=J,\
  \vec d:\mathsf{All}(\mathsf{StdDeriv},\mathsf{prem}(r))\bigr).
\end{aligned}
\tag{5.12}
\]

Side conditions are included in the type of rule instances: an instance exists only with evidence of its required conditions. This makes omitted side conditions a visible change of the rule signature, not a harmless implementation detail.

For a host presentation \(H:\mathsf{Judg}\to\mathcal U\), write
\(D_J:=\mathsf{StdDeriv}(J)\) and \(H_J:=H(J)\). Define adequacy as the following data, with explicit round-trip laws:

\[
\begin{aligned}
\mathsf{Adequate}(H):=
\prod_{J:\mathsf{Judg}}\sum_{f_J:D_J\to H_J}
\sum_{g_J:H_J\to D_J}\Bigl(&
  (\prod_{d:D_J} g_J(f_J(d))=d)\\[-2pt]
  &\times(\prod_{h:H_J} f_J(g_J(h))=h)
\Bigr).
\end{aligned}
\tag{5.13}
\]

An encoding \(f\) is defined by induction on source derivations, mapping each source rule instance and its already translated premises to a host constructor. To define \(g\) by induction on host derivations, every constructor that proves an encoded source judgment must be accounted for. The two round trips are then proved by induction and the equations of the respective presentations. If derivations differ only by administrative proof structure, state the equivalence after the chosen truncation or quotient; do not assert judgmental equality of raw proof trees.

The forward map \(f_J:D_J\to H_J\) establishes *representation completeness*: every derivation of the selected formal rule signature has a host counterpart. The reverse map \(g_J:H_J\to D_J\) establishes *representation soundness*: a host derivation does not invent a derivation absent from that signature. The round trips in (5.13) strengthen these two implications to equivalence of the chosen derivation presentations; they matter when proof structure, elaboration choices, or diagnostics are transported as data. When only proposition-level derivability is observed, one may instead prove the two implications after truncation. Neither formulation licenses discarding side conditions.

**The external standard boundary.** A published language standard may be prose rather than a pre-existing formal derivation system. In that case \(\mathsf{RuleSig}\) is a *formalization* of the standard, not a formal object already furnished by the standard. The equivalence (5.13) checks the relationship between the selected formal signature and the host encoding; it does not, by itself, prove that the signature faithfully transcribes every normative sentence. A clause-indexed coverage relation can record, for a selected edition \(E\), which normative clauses have been assigned rule instances, their implementation parameters, and the justification of each transcription:

\[
\mathsf{covers}_E:\mathsf{Clause}_E\to
\mathsf{List}(\mathsf{Inst})\to\mathcal U.
\tag{5.13a}
\]

Coverage is a traceable audit object, not a theorem of prose fidelity; completeness relative to an edition additionally requires accounting for all relevant normative requirements and their interactions. When an independently established formal semantics exists, a further derivation correspondence may be proved against it. The separate relation between the selected abstract language semantics and a target execution is defined in §8.2.

## 5.6 A closed source-language transformation: syntax, execution, proof, projection

The abstract lifting interface can be instantiated completely on a small language without assuming any built-in understanding of its source syntax. This example is deliberately small enough that every operation is defined here. It exhibits the same dependency pattern as lifting a larger language: a source grammar, a meaning function, a specification about that meaning, a checked proof, and an unchanged source projection.

Fix a byte type \(\mathsf{Byte}=\mathsf{Fin}(256)\), with \(\oplus\) bitwise XOR and all byte arithmetic modulo \(256\). Define expressions over one input byte:

\[
\begin{aligned}
\mathsf{Expr}&::=\mathsf{Input}\mid\mathsf{Const}(b)
                  \mid\mathsf{Xor}(e_1,e_2),\\
\llbracket\mathsf{Input}\rrbracket(x)&:=x,\\
\llbracket\mathsf{Const}(b)\rrbracket(x)&:=b,\\
\llbracket\mathsf{Xor}(a,b)\rrbracket(x)&:=
   \llbracket a\rrbracket(x)\oplus\llbracket b\rrbracket(x).
\end{aligned}
\]

The interpreter is a structurally recursive cubical term. Its computation rules are precisely the three equations above. The Agda tradition of programming with dependent types supplies an established executable setting for such definitions ([29](#ref-29)). Source syntax is *not* host syntax: the constructors are data, and the interpreter assigns them meaning.

Define the source program and its desired property:

\[
\begin{aligned}
p&:=\mathsf{Xor}(\mathsf{Xor}(\mathsf{Input},
       \mathsf{Const}(0x5a)),\mathsf{Const}(0x5a)),\\
P(e)&:=\prod_{x:\mathsf{Byte}}
        \llbracket e\rrbracket(x)\equiv x.
\end{aligned}
\]

A proof of the property is the following term, using the XOR cancellation law:

\[
\begin{aligned}
\mathsf{xorCancel}&:\prod_{x,k:\mathsf{Byte}}
             (x\oplus k)\oplus k\equiv x,\\
\pi&:\ P(p),\\
\pi(x)&:=\mathsf{xorCancel}(x,0x5a).
\end{aligned}
\]

For complete independence from an external byte library, define XOR by eight Boolean bits and the Boolean truth table:

\[
\begin{array}{c|cc}
\oplus&0&1\\\hline
0&0&1\\
1&1&0
\end{array}
\qquad
(a\oplus b)_i:=a_i\oplus b_i,\quad 0\leq i<8.
\]

The cancellation law is an eight-fold product of the four Boolean cases; no unproved property of a machine instruction is being used.

A typed annotation is a dependent pair:

\[
\mathsf{annotate}(p,\pi):
\sum_{e:\mathsf{Expr}}P(e),\qquad
\mathsf{erase}(e,\rho):=e,\qquad
\mathsf{erase}(\mathsf{annotate}(p,\pi))\equiv p.
\]

The annotation changes neither the source AST nor the interpretation. It introduces an additional checked object *about* that source AST. This construction does not depend on a comment syntax or on altering the original language's runtime rules.

Now define a semantics-preserving transformation that eliminates a repeated XOR constant:

\[
\begin{aligned}
\mathsf{cancel}: \mathsf{Expr}&\to\mathsf{Expr},\\
\mathsf{cancel}(\mathsf{Xor}(\mathsf{Xor}(e,\mathsf{Const}(k)),
                                \mathsf{Const}(k)))&:=\mathsf{cancel}(e),\\
\mathsf{cancel}(\mathsf{Xor}(a,b))&:=
        \mathsf{Xor}(\mathsf{cancel}(a),\mathsf{cancel}(b))
        \quad\text{otherwise},\\
\mathsf{cancel}(e)&:=e\quad\text{for the remaining constructors}.
\end{aligned}
\]

The first equation has precedence over the second. The equality proof is structural recursion:

\[
\mathsf{cancelSound}:
\prod_{e:\mathsf{Expr}}\prod_{x:\mathsf{Byte}}
\llbracket\mathsf{cancel}(e)\rrbracket(x)
\equiv\llbracket e\rrbracket(x).
\]

For the first branch, the induction hypothesis for \(e\), followed by \(\mathsf{xorCancel}\), establishes the equation. For the general XOR branch, apply congruence of \(\oplus\) to the two induction hypotheses. For constructors left unchanged, use reflexivity. In the first branch the result is exactly \(\mathsf{Input}\) when applied to \(p\).

Transport the source proof to the transformed program:

\[
\begin{aligned}
\mathsf{preserve}(e,\rho)
&:=\Bigl(\mathsf{cancel}(e),
\lambda x.\,
  \mathsf{cancelSound}(e,x)\mathbin{\cdot}\rho(x)\Bigr),\\
\mathsf{preserve}&:
  \prod_{e:\mathsf{Expr}}P(e)
  \to\sum_{e':\mathsf{Expr}}P(e').
\end{aligned}
\]

Here \(\cdot\) is path concatenation. The direction of the displayed calculation is important: it identifies the transformed program's meaning with the original meaning, so the original proof \(\rho(x)\) composes after it. A more general property \(Q\) is transported along the equality of denotations rather than manually rewritten.

The complete calculation is therefore

\[
\boxed{
\begin{aligned}
p&=\mathsf{Xor}(\mathsf{Xor}(\mathsf{Input},0x5a),0x5a),\\
\llbracket p\rrbracket(x)&=(x\oplus0x5a)\oplus0x5a,\\
\pi(x)&:\llbracket p\rrbracket(x)\equiv x,\\
\mathsf{cancel}(p)&=\mathsf{Input},\\
\mathsf{cancelSound}(p,x)&:
       \llbracket\mathsf{Input}\rrbracket(x)\equiv\llbracket p\rrbracket(x),\\
\mathsf{preserve}(p,\pi)&:
       \sum_{e:\mathsf{Expr}}\prod_x\llbracket e\rrbracket(x)\equiv x.
\end{aligned}}
\]

This is a closed instance of lifting, proof attachment, transformation, and projection. A larger language adds constructors and their semantic clauses; the lifting, proof attachment, transformation, and preservation operations do not acquire new logical foundations.

# 6. Reflective programming and language extension

The language definitions of §5 are now available as inputs to further computation. Reflection is the point at which the language operates on its own descriptions: changing a grammar, an interpreter clause, or the context in which a received expression is interpreted changes what subsequent computations do. Reflection makes that activity directly programmable: the interpreter, its source language, its transformation, and the rationale connecting their behaviors can all be represented and operated on. Lisp reflection, definitional interpreters, normalization by evaluation, partial evaluation, and staged programming are foundational antecedents ([3](#ref-3), [5](#ref-5), [12](#ref-12), [13](#ref-13), [14](#ref-14), [15](#ref-15)). Here the relevant operations act on typed representations in the language itself.

An evaluator acts on a representation of an expression. A reflective language also makes such representations available to programs written in the language. This must be done with attention to types, contexts, binding, and stage.

For a chosen object language \(L\), define typed code:

\[
\mathsf{Code}_L(\Gamma,A):\mathcal U,\qquad
\mathsf{eval}_L:
\mathsf{Code}_L(\Gamma,A)\to
\mathsf{Env}_L(\Gamma)\to\llbracket A\rrbracket_L.
\tag{6.1}
\]

A quoted term is not an untyped string; typed quotation and semantic readback build on normalization by evaluation and staged interpretation ([12](#ref-12), [13](#ref-13)). It carries enough structure to identify the object language, the free variables, and the represented type. A transformation can therefore have the form

\[
T:\prod_{\Gamma,A}
\mathsf{Code}_L(\Gamma,A)\to
\mathsf{Code}_L(\Gamma,A),
\tag{6.2}
\]

or, when it changes representation or type, a dependent output containing the new type, code, and an explicit relation to the old meaning.

For example, a transformation that changes a product representation to an equivalent vector representation may return

\[
\sum_{B:\mathsf{Ty}_L}
\sum_{e:\llbracket A\rrbracket_L\simeq\llbracket B\rrbracket_L}
\sum_{c':\mathsf{Code}_L(\Gamma,B)}
\prod_{\rho:\mathsf{Env}_L(\Gamma)}
 e(\mathsf{eval}_L(c,\rho))=\mathsf{eval}_L(c',\rho).
\tag{6.3}
\]

Here \(c:\mathsf{Code}_L(\Gamma,A)\) is fixed, and \(e\) is an equivalence of the interpreted types. The final dependent product is the correctness evidence of the represented transformation.

Reflection must not be confused with unrestricted quotation of arbitrary host expressions at their own stage. Self-interpretation and staging require explicit representations, levels, and termination or guardedness conditions. Typed quotation, readback, and partial evaluation are constructions over code and semantics, not license to assume an inconsistent universe of all expressions interpreting itself without restriction.

Specialization illustrates the point. Given an interpreter \(I\), a program \(p\), and input \(x\), one can construct a residual expression \(S(I,p)\) satisfying

\[
\llbracket S(I,p)\rrbracket(x)
=
I(p,x).
\tag{6.4}
\]

The first Futamura projection is an instance of this equality ([14](#ref-14), [15](#ref-15)). Higher projections specialize the specializer or interpreter representation under suitable staging assumptions. They are consequences of the defined evaluator and substitution laws, not foundations of the language.


## 6.1 Typed quotation, semantic readback, and self-application

Let \(\mathsf{Raw}_{\ell}\) be the inductive object syntax at stage \(\ell\), and let \(\mathsf{Der}_{\ell}(\Gamma,c,A)\) be its typing-derivation family. Define checked code **once**:
\[
\mathsf{Code}_{\ell}(\Gamma,A)
 :=\sum_{c:\mathsf{Raw}_{\ell}}\mathsf{Der}_{\ell}(\Gamma,c,A).
\]
For a chosen semantic model of the selected typed calculus, evaluation is
\[
\mathsf{eval}_{\ell,\Gamma,A}:
 \mathsf{Code}_{\ell}(\Gamma,A)
 \to\prod_{\rho:\mathsf{Env}_{\ell}(\Gamma)}\llbracket A\rrbracket_{\rho}.
\]
A reifiable semantic domain \(\mathsf{Val}^{\mathrm{nf}}_{\ell}(\Gamma,A)\) is equipped with weakening, neutral reflection, and normal-form readback. It is a domain of semantic *normalization values*, not the unrestricted type of all functions from host values to host values. Its fundamental equation binds every parameter:
\[
\begin{aligned}
\mathsf{reify}_{\ell,\Gamma,A}&:
\mathsf{Val}^{\mathrm{nf}}_{\ell}(\Gamma,A)\to\mathsf{Code}_{\ell}(\Gamma,A),\\
\mathsf{reifySound}_{\ell,\Gamma,A}&:
\prod_{v:\mathsf{Val}^{\mathrm{nf}}_{\ell}(\Gamma,A)}
\prod_{\rho:\mathsf{Env}_{\ell}(\Gamma)}
\mathsf{eval}_{\ell}(\mathsf{reify}_{\ell}(v),\rho)
 =\mathsf{denote}_{\ell}(v,\rho).
\end{aligned}
\tag{6.5}
\]

The following is the *typed Kripke schema* for normalization by evaluation. Write \(\mathsf{wk}:\Gamma\to\Gamma\mathbin{\triangleright}A\) for the weakening renaming; \(\mathsf{ren}_{r}\) for its action on neutrals and values; and \(\mathsf{neVar}_0\) for the newest neutral variable. A semantic function \(f:\mathsf{Val}(\Gamma,\Pi_{x:A}B)\) is natural under renamings and can be applied at an extended context. Its reification is
\[
\mathsf{reify}_{\Pi_{x:A}B}(f)
 :=\mathsf{lam}\Bigl(
 \mathsf{reify}_{B[\mathsf{var}_0/x]}
 \bigl(\mathsf{appVal}(\mathsf{ren}_{\mathsf{wk}}f,
       \mathsf{reflect}_{A}(\mathsf{neVar}_0))\bigr)\Bigr).
\]
For a neutral dependent function \(n\), reflection is the semantic function which, at any renaming \(r:\Gamma\to\Delta\) and value \(a:\mathsf{Val}(\Delta,A[r])\), returns
\[
\mathsf{reflect}_{B[r,\mathsf{reify}_{A[r]}(a)/x]}
 \bigl(\mathsf{neApp}(\mathsf{ren}_r n,
              \mathsf{reify}_{A[r]}(a))\bigr).
\]
For a dependent pair \(\Sigma_{x:A}B\), the neutral projections are indexed by the first component:
\[
\begin{aligned}
a&:=\mathsf{reflect}_A(\mathsf{neFst}(n)),\\
\mathsf{reflect}_{\Sigma_{x:A}B}(n)
&:=\bigl(a,\mathsf{reflect}_{B[a/x]}(\mathsf{neSnd}(n))\bigr),\\
\mathsf{reify}_{\Sigma_{x:A}B}(a,b)
&:=\bigl(\mathsf{reify}_A(a),\mathsf{reify}_{B[a/x]}(b)\bigr).
\end{aligned}
\]
For \(p:\mathsf{Val}(\Gamma,\mathsf{PathP}(A,a_0,a_1))\), reification is \(\mathsf{pathLam}(i.\mathsf{reify}_{A(i)}(\mathsf{pathApp}(p,i)))\); neutral reflection is the corresponding family \(i\mapsto\mathsf{reflect}_{A(i)}(\mathsf{nePathApp}(n,i))\), with the boundary equations \(i=0,1\) inherited from the path type. The full cubical semantic domain also interprets face restrictions, composition, and Glue using its specified Kan operations. These clauses describe the indicated fragment; their meaning is fixed by the chosen semantic model, rather than by an undefined universal quotation operation.

Normalization of checked syntax is the composite
\[
\mathsf{normalize}_{\Gamma,A}(t)
 :=\mathsf{reify}_{\Gamma,A}
 \bigl(\mathsf{eval}^{\mathrm{nf}}(t,\mathsf{reflectContext}(\Gamma))\bigr),
\]
with \(\mathsf{reflectContext}\) defined recursively from the neutral variables of \(\Gamma\). The fundamental lemma is proved by induction on typed syntax; its binder case is precisely the weakening/application calculation above.

**Typed specialization and self-application.** For object types \(A,B\), a staged specializer has the exact type and law
\[
\begin{aligned}
S_{A,B}&:\mathsf{Code}(\varnothing,A\to B)\to
  \mathsf{Code}(\varnothing,A)\to\mathsf{Code}(\varnothing,B),\\
\mathsf{eval}(S_{A,B}(q,r))&=\mathsf{eval}(q)(\mathsf{eval}(r)).
\end{aligned}
\]
Choose object types \(P,D,O\) and a represented interpreter \(e:\mathsf{Code}(\varnothing,P\to D\to O)\). For **represented** source input \(q_p:\mathsf{Code}(\varnothing,P)\), the first projection is

\[
\mathsf{compiled}(e,q_p):=S_{P,D\to O}(e,q_p),\qquad
\mathsf{eval}(\mathsf{compiled}(e,q_p))(d)
 =\mathsf{eval}(e)(\mathsf{eval}(q_p))(d).
\tag{6.6}
\]

For the second projection let \(A:=\mathsf{Code}(\varnothing,P\to D\to O)\), \(B:=\mathsf{Code}(\varnothing,P)\to\mathsf{Code}(\varnothing,D\to O)\), and \(G:A\to B\) be \(G(e)(q_p):=S_{P,D\to O}(e,q_p)\). At a stage that represents this *particular* \(G\), supply checked code \(q_G:\mathsf{Code}(\varnothing,A\to B)\). Because \(A\) is a syntax datatype, a typed stage-lifting function may represent an element \(e:A\) by \(\mathsf{lift}_A(e):\mathsf{Code}(\varnothing,A)\), with \(\mathsf{eval}(\mathsf{lift}_A(e))=e\). Then

\[
\begin{aligned}
\mathsf{compilerCode}(e)&:=S_{A,B}(q_G,\mathsf{lift}_A(e)),\\
\mathsf{eval}(\mathsf{compilerCode}(e))(q_p)&=S_{P,D\to O}(e,q_p).
\end{aligned}
\tag{6.7}
\]

The operation \(\mathsf{lift}_A\) is defined on the *represented syntax datatype* and its checked constructors; it is not a map \(A\to\mathsf{Code}(A)\) for arbitrary semantic types. The same stage and representation indices apply to any further projection. The equations follow from the one specialization law and the supplied typed code for \(G\), rather than an unrestricted quotation of arbitrary semantic values.



### Checked syntax and its interpretation

The checked-code family is the one defined above as \(\mathsf{Code}_{\ell}(\Gamma,A)\). Its first projection is raw syntax; its second is a typing derivation. The distinction between syntax, typing evidence, semantic values, and evaluator graph nodes is therefore explicit.

This is a presentation of checked syntax; raw syntax, typing derivations, semantic values, and evaluator graph nodes are distinct objects. Their connections are operations, not implicit identifications. An elaborator acts on raw declarations and produces checked syntax or a precise failure. Interpretation acts on checked syntax and an environment:

\[
\operatorname{eval}_{\Gamma,A}:\mathsf{Code}(\Gamma,A)\to\prod_{\rho:\operatorname{Env}(\Gamma)}A[\rho]. 
\]

A quotation operation reads a semantic value back into an appropriate typed presentation. At the stage and type where quotation is defined, its correctness is expressed by

\[
\operatorname{eval}(\operatorname{reify}_A(v),\rho)\equiv\operatorname{denote}(v,\rho),
\]

with the appropriate interpretation of free variables and semantic environments. A quotation algorithm returns a neutral presentation when computation is blocked on a free variable. The quotation correctness equation is stated only for values in the *reifiable semantic domain* of the selected type and stage; it is not a total quotation function on arbitrary external states, arbitrary infinite processes, or a universe containing its own evaluator.
The checker is a program deciding the *specified decidable typing and conversion judgments* for the selected core. Its existence is a separate theorem about the core, not a consequence of making syntax inductive. The metacircular checker and the derivability judgment are distinct: a definition of `check` does not establish `check(c,A)=yes → Der(Γ,c,A)` without a soundness argument. In particular, a universe-polymorphic `Code` is not a universe of its own unrestricted semantics. The staging and universe restrictions are made explicit in the reflection and self-application definitions.

### Substitution on checked syntax

A well-typed substitution \(\sigma:\Delta\to\Gamma\) assigns each variable of \(\Gamma\) a term in \(\Delta\), respecting dependencies. Interpretation satisfies

\[
\llbracket t[\sigma]\rrbracket_\rho
\equiv
\llbracket t\rrbracket_{\llbracket\sigma\rrbracket_\rho}. 
\]

This is the equation behind ordinary function application, instantiation of dependent types, partial evaluation, theorem instantiation, and staging. A type argument, a known numerical input, an interpreter's source program, or a deployment configuration is a value supplied through the same operation.

### Transformations of checked syntax

A metaprogram acts on typed representations and constructs another representation. A transformation can return a dependent pair

\[
 \sum_{c':\mathsf{Code}(\Gamma,A)}
 \operatorname{Path}_A(\llbracket c\rrbracket,\llbracket c'\rrbracket). 
\]

The path may be reflexivity because both representations compute to the same value; it may be an instantiated theorem; or it may be a composite of earlier paths. If the transformation changes types, its result contains the corresponding equivalence, transport, or other semantic relation rather than pretending that its input and output inhabit the same type.

A rewrite with theorem \(p:u\equiv v\) under a context \(C[-]\) gives \(\mathsf{cong}\ C\ p:C[u]\equiv C[v]\). Rewrites compose by path concatenation. The program performing the rewrite need not be trusted as an oracle: its returned evidence is checked by the language's typing rules.

This is the operational meaning of reflective programming. A source declaration is data. Its meaning is a term. Its transformations are terms. Their evidence is a term. Their results are available to further computation.

### Typed stages and checking

The three operations often conflated under *reflection* have different types. At a fixed stage and universe level, let `Rawl` be codes for terms in a selected object-language fragment; let `Derl(Γ,c,A)` be the typing judgment represented as evidence; and let `Vall(A,ρ)` be the semantic interpretation of that type in environment `ρ`. The checked representation is the already defined \(\mathsf{Code}_{\ell}(\Gamma,A)\); its raw syntax and typing derivation are projections, not another code type.

The interpretation is a typed map, not a cast between syntactic and semantic objects:

\[
\mathsf{eval}_{\ell}:\mathsf{Code}_{\ell}(\Gamma,A)
\to(\rho:\mathsf{Env}_{\ell}\Gamma)\to\mathsf{Val}_{\ell}(A,\rho).
\]

A checker accepts *raw* syntax, not a `Code` that already contains the derivation it is meant to establish. One possible total interface for a decidable fragment is

\[
\mathsf{check}_{\ell}(\Gamma,c,A):
\mathsf{Dec}(\mathsf{Der}_{\ell}(\Gamma,c,A)),
\]

where `Dec P = P + (P → Empty)`. An elaborator may instead return a checked term, a type, and a structured diagnostic. A metacircular definition of `check` is a program, but the theorem that it accepts exactly the intended judgments is distinct from the definition. The latter is a property of its construction and the underlying calculus.

The quotation/readback operation is \(\mathsf{reify}\) defined above on \(\mathsf{Val}^{\mathrm{nf}}\); its correctness equation is (6.5). The checked-syntax presentation here specifies the codomain as a dependent pair of raw syntax and its typing derivation.

For open terms, the semantic domain includes neutrals with their contexts and quotation returns code in that context. The representation of code and the evaluator are indexed by object stage; metaprograms operate in an enclosing stage. A code for the evaluator is itself a code in a suitable enclosing stage, not an inhabitant of a contradictory type `U : U`. Thus a metacircular interpreter can be represented without identifying the language's universe with its own total truth predicate.

This separation is not bureaucratic: it identifies the precise objects on which substitution, normalization, proof construction, and self-application act. All four are programs, but none is an untyped magical operation on arbitrary objects.

### Open terms and delayed work

An open term is evaluated as far as its known structure permits. If a variable blocks an eliminator, the result is a neutral value containing that eliminator and its arguments. A later substitution can expose a constructor and resume the computation. The result is not an error or an arbitrary placeholder.

Dimension substitution can select a face of a system, change which boundary is available, or expose a composition rule. The face context therefore participates in the validity of a residual. Branches whose premises do not hold must not be forced merely to build a cache key or inspect a system. Dimension and face payloads retain their distinct algebraic meanings.

Delayed substitution records an operation and its inputs until a consumer needs the relevant structure. Call-by-need retains a shared producer rather than immediately copying and evaluating each use. Readback and quotation must preserve that sharing where possible; forcing the entire graph to a tree can destroy the benefit before the next operation begins.

### Reuse of reductions

Suppose a checked reduction has the form

\[
d:\Gamma;\varphi\vdash t\longrightarrow^{*}u:A.
\tag{6.8}
\]

For a substitution `σ` satisfying the context and face conditions, substitution stability gives a reduction from `t[σ]` to `u[σ]` with the corresponding type. A runtime may therefore reuse the already performed reduction under that substitution. It must then resume evaluation if `u[σ]` contains newly exposed redexes. A cached weak-head result for `u` is not automatically a weak-head result for every substitution instance.

A reusable computation records enough information to establish this applicability: the producer's structure, free dependencies, context restrictions, result or residual, and the transformation needed to instantiate it. A particular caller's route to the producer is not itself an input unless it changes one of those semantic dependencies.

There are four objects worth distinguishing operationally, without turning them into new semantic categories: a request occurrence, the producer it refers to, the reusable result of that producer, and delivery or adaptation of that result to the occurrence. Multiple occurrences may share one producer. Reusing the producer's work does not make delivery and adaptation free.

**Substitution acts on reductions.** If \(r:t\longrightarrow u\) is a generating reduction and \(\theta\) is a well-typed substitution, define \(\mathsf{substStep}(\theta,r)\) by induction on the reduction constructor: a beta step is sent to the corresponding substituted beta step, and a congruence step is sent to the congruence of the recursively substituted step. Thus

\[
\mathsf{substStep}(\theta,r):
\mathsf{subst}(\theta,t)\longrightarrow^{*}\mathsf{subst}(\theta,u).
\tag{6.9}
\]

The reflexive-transitive closure permits administrative reductions introduced by the chosen substitution representation. Source and target endpoints are fixed by the substitution equations of §5.1.

## 6.2 Specialization as a program over the language's own definitions

A definition has a type and a term. The machine evaluates the term; the checker can inspect the resulting value, and the language can operate on a representation of that term. The code datatype `Code` represents the term constructors, and `eval-g : Code → Value` interprets a closed code under the evaluator's environment (with the corresponding environment-indexed interpretation for open codes). The observation `%view` exposes the head constructor and constituent values of an evaluated term. The checker operates on this observed structure: its inference and conversion operations are themselves terms evaluated by the machine. Specialization uses the same codes and is accepted by conversion or a checked path, rather than by agreement of sample outputs.

The initial `Code` datatype is available during bootstrap, before the checker can certify its own definition. It must therefore be distinguished from the intrinsically checked family \(\mathsf{Code}_{\ell}(\Gamma,A)\) of §6.1. Programs may also define smaller checked syntax datatypes for their particular domains. The specializer below operates on the initial code representation; the result is checked at its declared type before it is admitted as a definition.

The specializer is defined by the following equations:

```text
specialize : Code → Sched → Lst Rule → Code → Code × Code
specialize Tc sch rules c =
  let c0 = normalize-held (sched-holds sch) Tc c
      r  = rewrite-all (sched-rewrites sch) Tc rules c0
  in (fst r , path-code (snd r) c0)
```

Here `Tc` represents the type of the term, `c` its code, and `sch` specifies which definitions remain unknown, how arguments are staged, and the rewrite budget. The result consists of a new code and a code for its justification. The two operations are different: normalization computes a representation definitionally convertible to the input, while theorem-directed rewriting produces an explicit path between the interpreted terms. When no theorem rewrite is applied, `path-code` constructs reflexivity. When a rewrite is applied, `compose-p` constructs the path composition, and `#jit` asks the checker to accept the resulting equality. Thus a wrong theorem application cannot be admitted merely because the transformed code is well formed.

The operation `normalize-held` makes partial information operational. Given the identifiers of definitions to retain, it replaces those references with bound neutral variables, abstracts over them, normalizes the resulting lambda term, then restores the held references in the residual code. A definition not held may unfold and compute; a held definition remains an unknown in the residual computation. This is **partial evaluation of the actual program**, not an approximation of it by an independently maintained symbolic model.

Staging of parameters is also computed on codes. `apply-now` traverses the leading binders of a represented function. A binder designated `st-now` receives a represented argument from the supplied environment; a `st-run` binder is retained as a variable beneath the residual lambdas. Staging and specialization compose as follows:

```text
residual : Sched → Code → Lst Rule → Code → Lst Code → Code × Code
residual sch Tc rules c env =
  specialize Tc sch rules (apply-now (sched-stages sch) c env)
```

The static environment here is `Lst Code`: the inputs supplied to the specializer are *representations*. No function of type `A → Code A` for arbitrary semantic `A` is being assumed. For a simple illustration, let the represented function compute `λ x y → (x + 7) + y`. Mark the first binder static and supply code for `5`. `apply-now` constructs the application under a lambda retaining `y`; normalization reduces the known arithmetic and leaves code for a one-argument computation with meaning `λ y → 12 + y`. The illustrative arithmetic assumes the corresponding word or natural-number operations and their reduction rules; the mechanism is the binder-indexed operation above.

There are two distinct semantic relationships in this calculation. First, applying a represented argument to a represented function and normalizing preserves meaning by the evaluator's computation and conversion. Second, rewriting the residual using a theorem carries a path, which is checked against the original and transformed values. For a represented source-language interpreter, the same operations specialize the interpreter with respect to a particular represented source program. For a represented specializer, further specialization gives the familiar Futamura constructions [14, 15]. None of these requires a new primitive for compilers: the interpreter, specializer, rewrite rules, and resulting programs are terms and data available to the language.

### Staged self-application: the three projections

The implementation-level `residual` operation above acts on represented arguments. Its general calculation is the staged specialization of represented functions. To keep the levels explicit, write \(C(A)\) for closed checked code of type \(A\), \(q_A(a):C(A)\) for an *available* representation of a value, and \(q_{C(A)}(c)\) for a representation of the **code package as data**, not its evaluation. Let
\[
S_{A,B,D}:C(A\to B\to D)\to C(A)\to C(B\to D)
\]
be a specializer equipped with the equation
\[
\llbracket S_{A,B,D}(e,q_A(a))\rrbracket(b)
=\llbracket e\rrbracket(a)(b).
\tag{6.F1}
\]
The equality is a path of meanings where required; this notation suppresses that witness, not its existence. For an interpreter \(e:C(P\to X\to Y)\) and represented source program \(q_P(p)\), the **first projection** is
\[
r_p=S_{P,X,Y}(e,q_P(p)):C(X\to Y),\qquad
\llbracket r_p\rrbracket(x)=\llbracket e\rrbracket(p)(x).
\tag{6.F2}
\]
To specialize the *specializer* rather than the interpreted program, set
\[
A_0=C(P\to X\to Y),\quad B_0=C(P),\quad D_0=C(X\to Y),
\]
and take a checked presentation \(s:C(A_0\to B_0\to D_0)\) of the selected algorithm \(S_{P,X,Y}\). The **second projection** is
\[
c_e=S_{A_0,B_0,D_0}(s,q_{A_0}(e)):C(B_0\to D_0),
\quad
\llbracket c_e\rrbracket(q_P(p))=S_{P,X,Y}(e,q_P(p)).
\tag{6.F3}
\]
The static input here is *the interpreter code*; the dynamic input is *the source-program code*. For the **third projection**, let
\[
A_1=C(A_0\to B_0\to D_0),\quad B_1=C(A_0),\quad D_1=C(B_0\to D_0),
\]
and let \(s':C(A_1\to B_1\to D_1)\) present the specialization algorithm at this next code type. Then
\[
g=S_{A_1,B_1,D_1}(s',q_{A_1}(s)):C(B_1\to D_1),
\quad
\llbracket g\rrbracket(q_{A_0}(e))=c_e.
\tag{6.F4}
\]
These are typed instances of one residualization equation, not three new compiler operators. In particular, \(q_{A_0}(e)\) and \(q_{A_1}(s)\) quote finite code *as data* at the next stage; they do not posit an unrestricted function \(A\to C(A)\). The displayed equalities express semantics; the actual elimination of interpreter dispatch additionally depends on the selected specializer reducing operations on static constructors while retaining unknown inputs. This is the same distinction exhibited concretely by `apply-now`, `normalize-held`, and the arithmetic residual above. The familiar abbreviations \(S(E,p)\), \(S(S,E)\), and \(S(S,S)\) are valid only with these code-level arguments and stage indices understood.

The universal native realization in §9.2 applies to the specializer, interpreter, compiler, and their residuals as represented programs, once the evaluator's adequacy and its own machine realization are established. In particular, the first projection can be applied to a represented evaluator and a static source program; the resulting residual may remove dispatch on that program. The projection's equation alone does **not** guarantee that dispatch disappears or that the residual belongs to the directly compiled fragment. The code-level stages \(q_{A_0}(e)\) and \(q_{A_1}(s)\) remain essential: they are representations of code as data, not arbitrary quotation of semantic values.

**Finite specialization of an evaluator.** Applying the first projection to a represented evaluator yields a residual with the same represented meaning, but termination of the *specialization algorithm* is an additional question. For an offline specializer that memoizes static configurations, a sufficient condition is that the reachable static configuration shapes form a finite set and that static reduction between memoization points terminates. A fixed source program has finitely many syntactic code positions, but an unbounded continuation stack or recursively varying static argument does **not** thereby have finitely many static states. A binding-time analysis may therefore retain the stack and runtime data dynamically, while using code positions and a finite set of frame constructors as static dispatch information. The residual's control locations then arise from the reachable static shapes; their transitions are computed by specializing the evaluator's cases. More explicitly, let \(Q\subseteq\mathsf{Pos}(c)\times\mathsf{FrameKind}\times\mathsf{StaticEnv}\) be the set of memoized static shapes and \(\delta:Q\to\mathsf{ResidualCase}(Q)\) the partial evaluator's case reduction. If \(Q\) is finite and every call that performs static reduction before returning to a memoization point terminates, then worklist expansion reaches a fixed point after at most \(|Q|\) newly discovered shapes. Each \(q\in Q\) supplies one residual control location; the dynamic continuation stack is a runtime value, not part of \(Q\). The resulting finite control graph has edges obtained from \(\delta(q)\), including dynamic conditional edges and residual calls to the general evaluator when code becomes dynamic. These premises yield a finite residual graph; they do not establish that all evaluator clauses become first-order direct instructions or that the resulting blocks satisfy the structural fold's compositionality without the separate congruence proof. This is a sufficient construction under the stated binding-time premises, not a theorem that every specializer terminates or that every residual has a finite control-flow graph.

When the residual encounters a code component that is not static, it may retain a call to the represented evaluator. That call is still an ordinary program term and is natively realized by §9.2. The distinction between static and dynamic code belongs to the chosen specialization, not to the set of programs that the language can express. Raw syntax can be evaluated by a guarded process without asserting normalization of arbitrary raw programs; the typed quotation and self-application equations above still require their stated stage indices and available checked code. Guarded representation of divergence does not make an arbitrary diverging term converge. Self-reference also has established limits: Church and Turing’s undecidability results and the diagonal constructions of Lawvere, Gödel, and Tarski concern the precise representability and logical premises involved ([1](#ref-1), [2](#ref-2), [33](#ref-33), [34](#ref-34), [35](#ref-35)); none is bypassed by reifying a program as data. Concretely, an inductive raw-code datatype \(\mathsf{Raw}\) can be consumed by a guarded interpreter \(\mathsf{runRaw}:\mathsf{Raw}\to\mathsf{Env}\to\mathsf{Proc}_{\Sigma}(\mathsf{Value}+\mathsf{Fault})\); its self-application to a code presentation of \(\mathsf{runRaw}\) is a *guarded execution*, not an assertion of a total normalizer or a universe-valued truth predicate. This requires the interpreter's code to be representable as raw syntax and its recursive calls to satisfy the guardedness rule. In contrast, \(q_{A_0}(e)\), \(q_{A_1}(s)\), and \(\mathsf{lift}_A\) in (6.F2)–(6.F4) inhabit checked, explicitly staged code types. No map quoting an arbitrary semantic value at its own level follows from the raw interpreter.

The native realization of the residual is treated in §9. It is important to keep the two relations distinct: accepting the specialized term by conversion or a checked path establishes its relation to the source definition; establishing the meaning of emitted bytes under the target instruction semantics is the separate source-to-machine construction. The selected C route and its downstream compilation relation are defined in §9.4; their correctness obligations are stated independently of the source-level conversion judgment. Appendix C retains a separate direct-byte example.

## 6.3 Reflection applied to a language definition itself

A language implementation is not merely a set of programs: its grammar, typing judgments, and interpreter are themselves expressions. Consider an extension of the arithmetic language in §5.2 by multiplication:

\[
\mathsf{mul}:\mathsf{Expr}(\Gamma)\to
\mathsf{Expr}(\Gamma)\to\mathsf{Expr}(\Gamma),
\qquad
\llbracket\mathsf{mul}(u,v)\rrbracket\rho
=\llbracket u\rrbracket\rho\cdot\llbracket v\rrbracket\rho.
\tag{6.10}
\]

One can now define a transformation on the *interpreter*, not just on its input programs. For example, the interpreter can be transformed into an interpreter returning both a value and an expression witnessing its symbolic dependence on the input, or into an evaluator over a different semiring. To make the latter exact, replace the natural-number carrier by a semiring \(R\) and interpret `num(n)` using the canonical homomorphism \(\mathbb N\to R\). The same syntax then admits a family of interpretations:

\[
\llbracket-\rrbracket_R:
\mathsf{Expr}(\Gamma)\to
(\Gamma\to R)\to R.
\tag{6.11}
\]

For a context \(\Gamma\), take the polynomial semiring \(R=\mathbb N[X_i\mid i\in\Gamma]\), interpreting variable \(i\) as \(X_i\). The numerical interpretation takes \(R=\mathbb N\) and environment \(\rho:\Gamma\to\mathbb N\). Evaluation at that assignment is the semiring homomorphism

\[
\mathsf{ev}_{\rho}:\mathbb N[X_i\mid i\in\Gamma]\to\mathbb N.
\tag{6.12}
\]

Consequently the following commutes, by structural induction on expressions:

\[
\mathsf{ev}_{\rho}(\llbracket e\rrbracket_{\mathbb N[X_i\mid i\in\Gamma]}(\lambda i.X_i))
=\llbracket e\rrbracket_{\mathbb N}(\rho).
\tag{6.13}
\]

This is a concrete instance of changing the interpretation of a language while preserving the algebraic structure it expresses. It is also the elementary basis for symbolic execution of arithmetic expressions. The construction uses the semiring laws and the interpreter defined over them.

For expressions containing branching, mutable state, exceptions, or world requests, the interpretation must preserve the corresponding additional structure. A semiring homomorphism alone is insufficient. Again the necessary structure is determined by the object under study, not by a predefined list of software subsystems.

**A transformation of an interpreter definition.** For a context \(\Gamma\), the interpreter is the fold of the inductive expression signature
\[
\mathsf{Expr}(\Gamma)=\mathsf{var}(\Gamma)+\mathsf{num}(\mathbb N)
 +\mathsf{plus}(\mathsf{Expr}(\Gamma)^2)
 +\mathsf{mul}(\mathsf{Expr}(\Gamma)^2),
\]
where the notation describes its four constructor families, not a recursive equation in ordinary sets. The **represented algebra** of clauses over a carrier \(R\) and environment \(\rho:\Gamma\to R\) is the typed record
\[
\begin{aligned}
\mathsf{Clauses}(R):=\;&
(\Gamma\to R)\times(\mathbb N\to R)\\
&\times(R\times R\to R)\times(R\times R\to R),\\
\mathsf{fold}_{R}:\;&\mathsf{Clauses}(R)\to\mathsf{Expr}(\Gamma)\to R.
\end{aligned}
\tag{6.14}
\]
The fold is defined on each constructor by applying the corresponding record component. In the first component the input \(
\rho:\Gamma\to R\) is part of the record; the second, third, and fourth components interpret constants, addition, and multiplication. For a semiring \(R\), construct the *represented interpreter definition*
\[
I_R(\rho):=(\rho,\eta_R,+_R,\cdot_R):\mathsf{Clauses}(R).
\]
The construction \((R,\rho)\mapsto I_R(\rho)\), for a specified semiring structure on \(R\), supplies an interpreter definition, not merely an input program. Its meaning is
\[
\mathsf{fold}_{R}(I_R(\rho),e)=\llbracket e\rrbracket_R(\rho),
\]
proved by the four constructor equations. If \(h:R\to S\) is a semiring homomorphism and \(\rho_S=h\circ\rho_R\), the constructor proof gives
\[
 h(\mathsf{fold}_{R}(I_R(\rho_R),e))
 =\mathsf{fold}_{S}(I_S(\rho_S),e).
\]
The record and its fold are terms available for further computation. Syntactic inspection or rewriting uses their represented code and the quotation/readback conditions of §6.1; the semantic algebra alone does not supply a code representation.



Interpretation can continue across inputs: one input may change the bindings or represented clauses used for the next. The interpreters above and the processes of §7.6 express this without another semantics. A received expression is interpreted under the definitions then selected, not under a meaning intrinsic to its bytes. Specialization uses the available context; the dependency and continuation relations of §10.5 specify which earlier computations remain applicable when it changes.

## 6.4 A source language can inherit the mathematics of its host

Take a typed source-language term \(p:\mathsf{Tm}_L(\Gamma,A)\). Its representation in the cubical language is itself a term. A function may inspect that representation, construct another term, and construct a dependent proof about the relation between their meanings. There is no need to extend the source language's runtime instruction set merely to express this calculus.

For an existing language, a source annotation denotes a typed host expression whose context includes the exact source declaration it annotates. If source types depend on the context, use \(\mathsf{Ty}_L(\Gamma)\) and index the resulting term by the elaborated context and type. The association is not a textual convention: it is a dependent index. One elementary shape is

\[
 \mathsf{Annotated}_L(\Gamma,A)
 :=\sum_{p:\mathsf{Tm}_L(\Gamma,A)}\mathsf{Property}_L(p).
\tag{6.15}
\]

The property family can be as simple as a postcondition or as rich as a statement about all executions of the program. The first projection recovers the original source term. An annotation language can include host expressions for paths, equivalences, higher inductive objects, or transformations of source syntax; these expressions have the host's expressive power because they are host terms. Their association with a source construct is a type-indexed relation, not an invention of a separate proof language.

A general source transformation can change the output type as well as the syntax:

\[
 \mathsf{Transform}_L(\Gamma,A,p)
 :=\sum_{B:\mathsf{Ty}_L(\Gamma)}\sum_{q:\mathsf{Tm}_L(\Gamma,B)}
 \mathsf{Relation}_L((A,p),(B,q)).
\tag{6.16}
\]

Here \(p:\mathsf{Tm}_L(\Gamma,A)\) is fixed as an input parameter, so the displayed type is a dependent result family in \(p\). The chosen relation is defined mathematically. For a representation equivalence it includes the equivalence and its action on meanings; for a refinement it includes the specified implication between observations; for exact source preservation it may include a path of syntax trees. Nothing requires all transformations to use the same relation. The program's semantics supplies the relevant object to compare.

A source-to-source projection is itself a function. If an extension introduces a new notation, the notation can elaborate to ordinary source constructs, with the elaboration law proved by induction on its syntax. If an annotation has no runtime meaning, erasing it is the first projection of a dependent pair. If an extension changes runtime meaning, the translation explicitly defines the changed source expression. These are ordinary cases of construction and interpretation.

**A conservative source-language extension, calculated.** Fix an original language \(L\), a typed extension \(L^+\), and an erasure on checked expressions

\[
\mathsf{erase}_{\Gamma,A}:\mathsf{Tm}_{L^+}(\Gamma,A)
\longrightarrow\mathsf{Tm}_L(\Gamma,A).
\tag{6.17}
\]

For an annotation constructor \(\mathsf{withProof}(p,\pi)\), where \(\pi:\mathsf{Property}_L(p)\), erasure returns \(p\); it is homomorphic on original constructors. The interpretation of this *conservative* extension is \(\llbracket x\rrbracket_{L^+}:=\llbracket\mathsf{erase}(x)\rrbracket_L\). The preservation law is then an equality calculated by recursion on the extended syntax:

\[
\prod_{x:\mathsf{Tm}_{L^+}(\Gamma,A)}
\llbracket x\rrbracket_{L^+}
=\llbracket\mathsf{erase}(x)\rrbracket_L.
\tag{6.18}
\]

A higher-inductive or dependent host expression may appear in \(\pi\) or in a staged transformation of \(x\); it need not become an instruction of \(L\). If a staged transformation \(T\) produces \(x'\), the actual emitted source is \(\mathsf{erase}(x')\), and the transformation's *additional* semantic claim must be constructed as a term of the appropriate relation between \(\llbracket\mathsf{erase}(x)\rrbracket_L\) and \(\llbracket\mathsf{erase}(x')\rrbracket_L\). Equation (6.18) alone says only that annotations erase; it does not incorrectly certify arbitrary source rewrites. This recovers the source-annotation, elaboration, projection, and proof-attachment construction in a single typed statement.

The same host terms can operate on the language definition itself. A grammar constructor may be added, a semantic clause supplied, a family of transformations derived, and proofs about the resulting language constructed. This is the concrete meaning of unrestricted language extension: the source language is an object available to the full expressive power of the host, not a privileged syntax boundary.

## 6.5 Shared evaluation and partial information

Sharing is a property of a represented computation: two occurrences may refer to one
producer, and a dependent consumer requests a result from that producer. A producer
may contain an unfinished computation, a weak-head result, or a completed value.
These states are distinguished by the computation's representation and transition
rules. Sharing changes the graph of dependencies; graph-based local reduction has an established account in interaction nets ([70](#ref-70)). It does not change the typing,
substitution, or equality judgments of the represented terms.

For a term with an open environment, a reduction may expose a residual rather than a
closed value. Consider

\[
 t(x,y)=\mathsf{fst}(x+3,y).
\]

The projection reduces to \(x+3\) without inspecting \(y\). Substituting
\(x:=5\) permits the remaining addition to reduce to \(8\). This is one
reduction relation acting under successive substitutions, not a second semantics
for partially evaluated programs. The substitution action on reduction evidence
was defined in (6.9); retained dependencies specify when that evidence applies.

# 7. Programming with machines and operating systems

A represented interpreter does not end at a returned value: it can issue commands, suspend, interact with other processes, and change the world in which later observations occur. Machines and operating systems are particular such interacting processes, with memory, event, and device semantics. State-transition models and concurrency have established semantics; the treatment of concurrent events here is informed by the formal C++ memory-model literature and temporal system specifications ([20](#ref-20), [21](#ref-21), [38](#ref-38)).

A processor is one specified evolving system: its architectural state, permitted transitions, observations, and interaction with devices are part of its description, and circuitry realizes that description under a stated physical correspondence. The same form of reasoning used for other processes applies to its executions. A program may contain the machine's interpreter or its representation, and the program constructing the machine's executable realization can itself be interpreted and transformed.

A byte can be modeled as \(\operatorname{Fin}(256)\). A fixed-width word as \(\operatorname{Fin}(2^w)\). An address space as a type \(\mathsf{Addr}\), and memory as a stateful mapping with a domain of valid locations:

\[
\mathsf{Mem}:=\mathsf{Addr}\rightharpoonup\mathsf{Byte}.
\tag{7.1}
\]

This toy memory model is insufficient for C++ pointer provenance, lifetime, allocation, atomicity, and object representations; those are additional relations and structures when C++ is the subject.

A single-core instruction machine can be described by

\[
\mathsf{State}
:=
\mathsf{Registers}\times\mathsf{Memory}
\times\mathsf{PC}\times\mathsf{DeviceState},
\tag{7.2}
\]

and a transition relation

\[
\mathsf{step}:\mathsf{State}\to\mathsf{Event}\to\mathsf{State}\to\mathcal U.
\tag{7.3}
\]

A deterministic machine may have a next-state function instead; interaction with an external world can be represented by labeled transitions and handlers. A processor executing one instruction at a time, interrupts, memory-mapped I/O, context switches, page tables, a scheduler, and isolated processes are all constructions over this state and its transition relation.

A machine instruction does not have to be a primitive of the cubical language. Nor does the cubical language need to execute itself as a native machine instruction in order to express the machine's semantics.

For example, consider an imperative loop summing \(n\) unsigned 64-bit words. Its state includes the array \(a\), index \(i\), and accumulator \(s\). The loop invariant is

\[
0\le i\le n
\quad\land\quad
s=\sum_{j<i}a_j\pmod{2^{64}}
\quad\land\quad
\mathsf{Unchanged}(a).
\tag{7.4}
\]

Initialization establishes the invariant at \(i=0\). The loop body updates \(s\) by addition modulo \(2^{64}\) and increments \(i\), preserving it. Termination at \(i=n\) yields the required sum. If the program performs actual memory loads, a complete proof also requires validity, alignment, lifetime, aliasing, and source-language semantics for those loads.

The machine-level facts and the higher reasoning are not two unrelated disciplines. The machine and the proof are objects in the same language, and their relation is expressible there.


## 7.1 Memory is more than an address-to-byte function

The partial function in (7.1) is enough to demonstrate a simple load/store machine, but not enough to model C++ faithfully. A more informative memory state distinguishes allocation identity, storage extent, object lifetime, effective or dynamic type, initializedness, provenance, and the bytes currently stored. A pointer is not simply an integer address: the semantics of dereference may depend on which allocation produced it and whether the designated object remains alive.

A memory operation can be modeled as a relation

\[
\mathsf{Load}:\mathsf{MemState}\to\mathsf{Pointer}\to
\mathsf{Value}\to\mathsf{MemState}\to\mathcal U,
\tag{7.5}
\]

where the returned state may equal the input state for an ordinary load, while volatile and atomic operations require additional events or relations. A write similarly relates old state, pointer, value, and new state. The exact clauses depend on the language and memory model being represented.

A source-level property such as “this function never accesses outside the allocated buffer” is therefore a proposition about the source semantics and its allocation/lifetime relations. An address sanitizer instead instruments a particular executable and observes particular executions. Both are constructions; neither is the definition of memory safety itself.

## 7.2 Concurrency requires relations among events

A concurrent execution can be represented by events \(E\), thread identifiers, reads and writes, program order \(\mathsf{po}\), reads-from \(\mathsf{rf}\), and modification order \(\mathsf{mo}\):

\[
X=(E,\mathsf{tid},\mathsf{kind},\mathsf{loc},\mathsf{val},
\mathsf{po},\mathsf{rf},\mathsf{mo}).
\tag{7.6}
\]

A memory model is a predicate \(\mathsf{Allowed}(X)\) selecting permitted executions. A data race, a happens-before relation, or an atomic ordering constraint is a property of these relations. A compiler correctness theorem for concurrent programs must relate the admitted target executions to the admitted source executions. Matching the behavior of each isolated instruction is insufficient when reordering and weak memory are observable.

Again, no extra foundational primitive called *thread* is required in cubical type theory. Thread and synchronization are objects whose exact rules are specified when the system being modeled has them.



A concrete **sequentially consistent** event model makes the admissibility predicate explicit. Let \(E\) be a finite set of read/write events, \(\mathsf{po}\) the per-thread program order, and \(\mathsf{rf}\) the reads-from relation. Define \(\mathsf{Allowed}_{SC}(X)\) to be the type of a total order \(<_X\) on \(E\) extending \(\mathsf{po}\) such that every read observes the latest preceding write to its location in \(<_X\) (or its specified initial value). This is a complete example of a memory-model predicate, not the C++ weak-memory predicate. A C++ atomic or data-race theorem uses the actual chosen C++ execution axioms in place of \(\mathsf{Allowed}_{SC}\); the event structure and observation operations remain constructions.

## 7.3 Continuing computation as a definition

Guarded recursion, transition relations, free-monad trees, and coalgebras give different representations of continuing computation, with different available observations and proof principles. A transition system specifies states and labeled steps; an external label becomes an internal interaction when a suitable handler is composed with it. The equality, bisimulation, or refinement needed for a representation change must be established for the selected behavior. Section 7.6 computes one such composition.

## 7.4 Parallel processors and accelerators are other transition systems

For GPU execution, CUDA supplies a concrete programming and execution model ([32](#ref-32)); the treatment below concerns the corresponding state, events, and observable execution.

A GPU kernel operates over indexed collections of work items with defined address spaces, synchronization, and numerical operations. Its meaning may be expressed by a relation on global and local state, together with an ordering relation among memory events. A tensor layout is a representation of an indexed family:

\[
 T:\operatorname{Fin}(n_1)\times\cdots\times\operatorname{Fin}(n_k)\to A.
\tag{7.7}
\]

A concrete layout supplies a map from indices to addresses. A permutation of dimensions, a tiled layout, and a packed layout are representation changes whose preservation laws are equations about the values read at each logical index. Their costs depend on the target machine's memory hierarchy and execution model. Correctness and performance are properties of the chosen representations and observations.

For a differentiable numerical program, automatic differentiation is a transformation on its construction, governed by the chain rule and the chosen numeric semantics. A GPU compiler may subsequently realize either the original program or its derivative on the accelerator. The derivative is not fundamentally a GPU operation; the accelerator is not fundamentally a differentiable-programming operation. Both are constructions that compose when their types and semantic laws permit it.



## 7.5 An operating system and its guests form one program

Let \(K\) denote the kernel's transition system and \(P_i\) its user processes. A kernel by itself is an open program: system calls, interrupts, device events, and process requests are inputs to its transition relation. Each \(P_i\) is also a program. A scheduler, address-space handler, and device handlers interpret their interactions, constructing the joint computation

\[
\mathsf{Host}(K;P_1,\ldots,P_n):\mathsf{Proc}_{\Sigma_{\mathrm{device}}}(A).
\tag{7.8}
\]

The notation abbreviates repeated typed process composition. The same kernel may be programmed and proved as an open object quantified over arbitrary admitted user processes; it may then be specialized by composing it with particular guests. For instance, an isolation invariant can be stated for every permitted kernel transition and every pair of protected address spaces. The invariant is preserved under composition when the guest interactions satisfy the kernel's interface premises. The theorem applies to the **joint running program**, including page-table, context-switch, and fault transitions. Composing hosts with communication handlers gives a fleet. The kernel boundary is an interface relative to the selected component; composing its environment internalizes further interactions without changing the underlying language.

## 7.6 Composition of programs with open interactions

Consider a command signature consisting of a command type and its dependent response type:

\[
\mathsf{Sig}:=\sum_{C:\mathcal U}(C\to\mathcal U),\qquad
\mathsf{Cmd}(S):=\pi_1 S,\quad
\mathsf{Resp}(S,c):=\pi_2 S(c).
\]

One representation of terminating processes is a Church-encoded free monad, parameterized by the interpretation's universe level:

\[
\mathsf{Proc}_\ell(S,A):=
\prod_{R:\mathcal U_\ell}(A\to R)\to
\bigl(\prod_{c:\mathsf{Cmd}(S)}(\mathsf{Resp}(S,c)\to R)\to R\bigr)\to R.
\]

The definitions `ret`, `cmd`, and `bind` are terms of this type. The π-calculus gives a distinct formal treatment of communication and changing connectivity ([67](#ref-67)); the present handler is defined by the stated command/response signature and interpretation. A handler from `S` to `W` assigns each `S` command a process over `W` with the original response type. Handling internalizes those interactions and leaves the remaining signature open. What counts as the process's environment therefore depends on the composition being examined.

Inductive/coinductive interaction-tree accounts of impure and recursive programs have also been developed in Coq ([39](#ref-39)). The corresponding inductive tree presentation is

\[
\mathsf{Step}(S,A)::=\mathsf{sdone}(a:A)
\mid\mathsf{scmd}(c:\mathsf{Cmd}(S),\mathsf{Resp}(S,c)\to\mathsf{Step}(S,A)).
\]

Define `o-handle` by structural recursion:

```text
o-handle hd (sdone a) = sdone a
o-handle hd (scmd c k) =
  o-bind (hd c) (λ r → o-handle hd (k r))
```

Let `o-idh c = o-issue c`, and `o-comph h₁ h₂ c = o-handle h₂ (h₁ c)`. Induction constructs paths witnessing the identity and composition laws for handlers. In particular, the inductive proof of `o-handle-id` has the return case by reflexivity and the command case by congruence over the recursively handled continuation; `o-handle-bind` is proved by induction, using associativity of `o-bind` at command nodes. Thus handler composition is a computed property of these programs, not a special semantics imposed by the host language.

The Church representation requires an explicit naturality condition to recover extensional equality from equality of reified trees. `o-NaturalAt` says that each interpretation of a Church term agrees with folding its tree; `o-natural-path` takes naturality witnesses for `p` and `q` and a path `to-step p ≡ to-step q`, and constructs a path `p ≡ q`. The hypotheses matter: equality of reifications is not silently assumed to identify arbitrary nonparametric inhabitants of a Church encoding.

As a distributed example, let `Beh M St` assign behavior to nodes, `FState M St` hold the joint state of those nodes and a network, and `EnvS` expose environmental choices. Define

```text
fleet-proc : Beh M St → Nat → FState M St → Proc EnvS (FState M St)
```

(with universe levels and implicit parameters suppressed). `fleet-proc` requests the next environmental choice, computes `fleet-step`, and recurses for the specified number of steps. The complete fleet is therefore one process over its remaining environment. Message-passing actors and deterministic stream-process semantics are precedents for representing communicating agents as programs ([55](#ref-55), [56](#ref-56)); the fleet in this section is the particular defined program, not an additional primitive. Its nodes and their interactions are still individually programmable. Nothing about the language makes `FState`, `Pkt`, or `EnvS` distinguished primitive forms; they are names defined by this particular program.

### Guarded interaction and a concrete two-process cut

The finite Church/inductive process presentations above express the handler algebra. Continuing processes can instead be represented in clocked cubical type theory. For a clock \(\kappa\), a later modality \(\triangleright^\kappa\), and an interaction signature \(\Sigma=(\mathsf{Cmd},\mathsf{Resp})\), a guarded process presentation is

\[
\mathsf{Proc}^\kappa_\Sigma(A)
 :=\mathsf{fix}^\kappa X.\left(A+\triangleright^\kappa X+
 \sum_{c:\mathsf{Cmd}}(\mathsf{Resp}(c)\to\triangleright^\kappa X)\right),
\qquad \mathsf{Proc}_\Sigma(A):=\forall\kappa.\mathsf{Proc}^\kappa_\Sigma(A).
\]

The alternatives are return, a silent delayed step, and an external command with a response-indexed delayed continuation. Guarded recursion defines `bind` by the return clause \(\mathsf{bind}(\mathsf{return}(a),f)=f(a)\), the silent clause \(\mathsf{bind}(\mathsf{delay}(p),f)=\mathsf{delay}(\mathsf{laterBind}(p,f))\), and the command clause \(\mathsf{bind}(\mathsf{call}(c,k),f)=\mathsf{call}(c,\lambda r.\mathsf{laterBind}(k(r),f))\). Here `laterBind` is obtained by applying the delayed recursive function to the delayed process and \(\mathsf{next}(f)\), using the later applicative operation; it is **not** an elimination of an arbitrary later value. A handler for a command already answered by the environment inserts a silent step before recursively resuming, preserving guardedness even when an unbounded stream of commands receives immediate responses. These are constructions in a clocked extension, not unmentioned computation rules of unextended CCHM.

A particular handler can combine two continuing processes. Let their *complete* local configurations \(C_P,C_Q\) include continuations, and let \(M_{PQ},M_{QP}\) be the directed message types. With FIFO queues \(u:\mathsf{List}(M_{PQ})\), \(v:\mathsf{List}(M_{QP})\), the composite program has state

\[
S_{P\parallel Q}=C_P\times C_Q\times\mathsf{List}(M_{PQ})\times\mathsf{List}(M_{QP}).
\]

Its transitions include local steps, sending, receiving, and unhandled environmental steps, together with their \(P/Q\) symmetric cases:

\[
\begin{array}{ll}
\mathsf{local}:&p\xrightarrow{\tau}p'\ \Rightarrow\ (p,q,u,v)\to(p',q,u,v),\\
\mathsf{send}:&p\xrightarrow{!m}p'\ \Rightarrow\ (p,q,u,v)\to(p',q,u{+\!+}[m],v),\\
\mathsf{receive}:&q\xrightarrow{?m}q'\ \Rightarrow\ (p,q,m::u,v)\to(p,q',u,v),\\
\mathsf{external}:&p\xrightarrow{e}p'\ \Rightarrow\ (p,q,u,v)\xrightarrow{e}(p',q,u,v).
\end{array}
\]

For an initially empty \(P\to Q\) queue, let \(N\) and \(D\) count successful sends and receives. The inductive invariant is \(N=D+|u|\), hence \(D\le N\): a send increments both \(N\) and \(|u|\), a receive increments \(D\) and decrements \(|u|\), and the other cases preserve all three. It is a property of the **composed program**, not an assumption about the individual processes. Trace projection and reconstruction follow by induction over the same cases, subject to FIFO matching and scheduling constraints. Neither fairness nor eventual delivery follows from the transition rules. Changing the network handler to permit loss, duplication, or reordering changes the composite state and invariant accordingly. The queue is a chosen example of an interaction cut, not a privileged architectural boundary.


### Native execution of a continuing process

The finite block contract of §9.3 ends at a postcondition. A continuing computation instead has a guarded process as its observation. For a code image \(C\) and admitted target state \(s\), the target's instruction transition system defines a process over its *remaining* system-command signature \(\Sigma_{\mathrm{sys}}\):

\[
\begin{aligned}
\mathsf{machProc}_C(s)&:=
\begin{cases}
\mathsf{return}(\mathsf{exit}(s)),&\mathsf{terminal}(s),\\
\mathsf{call}(q,\lambda r.\triangleright\mathsf{machProc}_C(\mathsf{resume}(s,r))),
  &\mathsf{syscall}(s)=q,\\
\mathsf{delay}(\triangleright\mathsf{machProc}_C(\mathsf{step}_C(s))),
  &\text{ordinary machine step}.
\end{cases}
\end{aligned}
\tag{7.6a}
\]

This schematic notation uses the guarded constructor's delayed continuation; it does not eliminate an arbitrary later value. An invalid instruction, trap, or resource failure is a distinct terminal observation or environmental command according to the *selected target semantics*, not an unmentioned fourth case. A source-command handler has the dependent type \(h:\prod_{c:\mathsf{Cmd}(\Sigma_{\mathrm{src}})}\mathsf{Proc}_{\Sigma_{\mathrm{sys}}}(\mathsf{Resp}(\Sigma_{\mathrm{src}},c))\): it interprets each source command as a system process returning that command's response type. Its extension \(h^*\) to source processes is the guarded version of the handler composition already defined above.

A correspondence that ignores finite internal steps must not identify an infinitely silent target with an arbitrary returning source. The following defines a **termination-sensitive weak correspondence**, with the observation convention explicit. Let \(x\xrightarrow{\tau}x'\) be an internal transition and \(x\xrightarrow{e}x'\) a visible return, fault, or command/response transition in the chosen source or machine process. Define the *inductive* finite-prefix judgments

\[
\frac{}{x\xRightarrow{\tau *}x}\qquad
\frac{x\xrightarrow{\tau}y\quad y\xRightarrow{\tau *}z}
{x\xRightarrow{\tau *}z}\qquad
\frac{x\xRightarrow{\tau *}y\quad y\xrightarrow{e}z}
{x\xRightarrow{e}z}.
\tag{7.6c}
\]

For a process state \(x\), its weak observable heads are the inductively witnessed events \(\mathsf{Head}(x):=\sum_{e,z}(x\xRightarrow{e}z)\). A return or fault is a terminal head; a command head includes its command and a continuation indexed by **each** permitted response. Define \(\mathsf{Div}_{\tau}(x)\) coinductively as an infinite chain of internal transitions, and \(\mathsf{Stuck}(x)\) as a nonterminal state with neither an internal transition nor a visible head. A guarded relation \(\mathsf{Sim}^{\kappa}(x,y)\) is the greatest clock-guarded relation satisfying the following *two-sided* clauses:

\[
\begin{aligned}
\mathsf{Sim}^{\kappa}(x,y)\;\Longrightarrow\;&
  \bigl(\forall (e,x')\in\mathsf{Head}(x),\ 
      \exists (e,y')\in\mathsf{Head}(y).\ \triangleright^{\kappa}\mathsf{Sim}^{\kappa}(x',y')\bigr)\\
&\times\bigl(\forall (e,y')\in\mathsf{Head}(y),\ 
      \exists (e,x')\in\mathsf{Head}(x).\ \triangleright^{\kappa}\mathsf{Sim}^{\kappa}(x',y')\bigr)\\
&\times\bigl(\mathsf{Div}_{\tau}(x)\leftrightarrow\mathsf{Div}_{\tau}(y)\bigr)
\times\bigl(\mathsf{Stuck}(x)\leftrightarrow\mathsf{Stuck}(y)\bigr).
\end{aligned}
\tag{7.6d}
\]

For terminal heads the recursive factor is the unit type and their return/fault payloads must be related by the specified value relation. For command heads the quantification over matching continuations is dependent on the command's response type: for every response allowed by the common handler, the respective continuations are related one clock tick later. The displayed greatest relation is understood as the guarded fixed point of these clauses; the finite-prefix relation in (7.6c) is constructed **before** the guarded recursion. The established theory of bisimulation and coinduction supplies the background for such greatest relations ([57](#ref-57)); the guarded clauses and their observation conditions are the ones explicitly stated here. This prevents the unsound proof that silently diverging target code simulates a source that must return: the source has a finite return head and the target does not, and the divergence clauses also disagree. It distinguishes divergence from deadlock, rather than hiding both as absence of a head. The two-sided form is appropriate for equivalence of visible behavior; a compiler that promises only refinement uses the corresponding one-sided head inclusion with an independent progress/divergence condition. Fairness and permitted environmental responses remain premises of the selected observation model; they are not inferred from guardedness. When the chosen semantics regards internal delays themselves as observable, replace the weak-head relation by the stronger labeled relation instead.

For an entry point \(a\), state predicate \(P\), and source process \(p\), a continuing block is the dependent object

\[
\begin{aligned}
\mathsf{ContBlock}_h(a,P,p):=\sum_{b:\mathsf{Bytes}}\prod_C
&\mathsf{Contains}(C,a,b)\\
&\to\prod_{s:S}(P(s)\times(pc(s)=a))\\
&\to\mathsf{Sim}(\mathsf{machProc}_C(s),h^*(p)).
\end{aligned}
\tag{7.6b}
\]

The containing-image, decoder, loader, and resource premises from §9.3 remain in \(P\) or the block's contextual hypotheses. For a guarded source fixpoint \(p=\mathsf{fix}(\lambda\,\mathsf{self}.\mathsf{body}(\mathsf{self}))\), a target loop can realize one turn of \(\mathsf{body}\) by finite blocks and return to the loop entry by a backward branch. **If** those blocks re-establish \(P\) and supply a guarded step of the relation from a delayed hypothesis, the turn proof has type \(\triangleright\mathsf{Sim}\to\mathsf{Sim}\); guarded fixed-point (Löb) induction closes the back edge. Neither a backward jump nor the source fixpoint by itself establishes the simulation: the one-turn proof, handler correspondence, and invariant do the work.

For a process specified to continue without resource failure, the continuing-block premise \(P\) must include a **re-established resource invariant**. For example, fix a live-state bound \(B\), an arena capacity \(K\ge 2B\), and two equal-size semispaces of capacity \(K/2\). At each turn boundary, let \(\mathsf{Live}(s)\le B\), the roots enumerate every reachable retained value, and \(\mathsf{Free}(s)\) contain enough room for the next turn's temporary allocations. Suppose the turn allocates at most \(A\) bytes between collections and collection copies at most \(B\) live bytes; a sufficient capacity condition for this specific schedule is \(K/2\ge B+A\), with collection and root-update correctness supplied by their machine blocks. The one-turn evidence then has the form

\[
\begin{aligned}
\mathsf{TurnSafe}:\ &\prod_{s:S}\Bigl(P(s)\to\sum_{s':S}
(\mathsf{FiniteTurn}(s,s')\times P(s')\times
\mathsf{NoResourceFault}(s\rightsquigarrow s'))\Bigr),\\
\mathsf{LoopSafe}:\ &P(s_0)\to
\mathsf{Always}^{\kappa}(\mathsf{NoResourceFault})
\end{aligned}
\tag{7.6e}
\]

where \(s'\) is bound by the finite-turn witness and \(\mathsf{Always}^{\kappa}\) is defined by guarded recursion on successive turns. Induction proves the invariant for every finite prefix; the guarded continuation establishes the ongoing safety property, subject to the stated allocation and collection bounds. The semispace numbers are a *sufficient example*, not a theorem that all programs have bounded live state. If the process specification explicitly admits resource failure as an observable outcome, that outcome can instead be related; the stronger no-failure guarantee is an application theorem with its own resource premises, not a default consequence of guardedness.

# 8. Existing programming languages inside the host

The universal host does not erase the specificity of existing languages. A lifted C++ program must retain the distinctions made by its own standard—among permitted results, unspecified choices, undefined behavior, object lifetimes, and concurrent events—while becoming available to the host's operations on syntax, meaning, and evidence. The C++ language is specified by ISO/IEC 14882:2020 ([19](#ref-19)). Formal accounts of its concurrency and executable semantics, including the work of Batty and colleagues, provide relevant prior definitions ([20](#ref-20)). 

C++ is a revealing instance of a language whose source notation, static judgments, object model, permitted executions, and target-machine realizations have accumulated many historically contingent distinctions. Those distinctions are part of the specified process and can be represented exactly; their quantity does not create a new kind of reasoning. The objective is to let existing C++ remain an ordinary source presentation while its actual interpreted constructions are available to the full language.

A complete C++ presentation must account for lexical and grammatical analysis, name lookup, overload resolution, templates and instantiation, constant evaluation, types and conversions, object lifetimes, allocation, exceptions, atomics, concurrency, and the standard's various categories of behavior. It must distinguish language-standard semantics from a particular compiler and target.

Let \(\mathsf{CppSyntax}\) be the parsed representation, \(\mathsf{CppJudgment}\) the static judgments, and \(\mathsf{CppExec}\) the source executions under a fixed standard version and implementation parameters. Then interpretation is not merely a function from AST to a number:

\[
\mathsf{CppMeaning}:
\mathsf{CppSyntax}\to
\mathsf{Input}\to\mathsf{Trace}\to\mathcal U.
\tag{8.1}
\]

For a nondeterministic semantics, the output is a predicate or relation on traces. For undefined behavior, the meaning must implement the selected formal account of the standard; it cannot simply substitute an ordinary fault unless that account is justified.

An annotation can be a term associated with an exact source construct. A proof concerning a function body can refer to its source semantics, including its effects on memory. A transformation can produce a new source construct together with a proof of observational equivalence or refinement. The original language can remain the surface notation while the surrounding language supplies dependent types, paths, higher metaprogramming, and proofs.

Unlike comments, such an annotation has a defined type, scope, interpretation, and equality relation. The annotation is attached to a typed syntactic or semantic object; the checker knows which declaration, type, scope, and interpretation it concerns. Its erasure or elaboration back to C++ is a separately defined operation with a separately proved property.

For example, a source-level memory function might carry a dependent specification

\[
\mathsf{Safe}(p):=
\prod_{s,s',t}
\mathsf{CppExec}(p,s,s',t)\to
\mathsf{MemoryInvariant}(s,s',t).
\tag{8.2}
\]

A transformation \(T\) is checked against a relation between the original and transformed executions. The resulting proof can be used to transport dependent properties when the relation supplies the appropriate equivalence or path. Where the relation is only refinement, the proof must use refinement; univalence does not make a one-way simulation invertible.

A precise formal presentation of the complete C++ standard is a large particular object. Its many clauses affect the size of that definition, not the foundational language used to express it. Specialized decision procedures and libraries are themselves terms available for construction and composition.


## 8.1 Source annotations are expressions attached to exact objects

Suppose a C++ source file contains a function with a parsed declaration \(d\), an elaborated type \(\tau\), and a body \(b\). A proof annotation should not be an arbitrary comment interpreted by a text-processing convention. It can be a term indexed by the elaborated declaration and its meaning:

\[
\mathsf{Annotation}(d):=
\sum_{P:\mathsf{Sem}_L(d)\to\mathcal U}
P(\llbracket d\rrbracket_L),
\qquad\llbracket d\rrbracket_L:\mathsf{Sem}_L(d).
\tag{8.3}
\]

Here \(\mathsf{Sem}_L(d)\) is the type of the semantic object of declaration \(d\); it may itself be a function or a relation on executions. The annotation is scoped and typed by that exact object. No assumption that the semantics returns a single runtime value is made.

The original source language may have no syntax for cubical paths, dependent products, or proofs. An extended presentation can carry those constructions alongside the source syntax and erase them when projecting to the original language. Erasure is sound for annotations that do not change runtime meaning. A metaprogram that generates new declarations or changes behavior instead requires elaboration and a semantic preservation or refinement theorem. These are different situations; they must not be collapsed into a single “zero-runtime annotation” claim.

## 8.2 One program through source and target representations

Let \(L\) denote the specified C++ standard and its parameters, and \(B\) a build configuration. A C++ source term \(p\) is a constituent of the larger hyperactivity program in which it is used. Compiling it produces an executable representation \(\mathsf{build}_B(p)\) of that same programmed computation. Source behavior, emitted-machine behavior, and a dependent property \(P\) are different constructions **about the same program**, connected by checked compilation and execution evidence—not an independently written verification model and a separately deployed system.

\[
\begin{aligned}
\mathsf{SourceBeh}_L(p,x,t)&:\mathcal U,\\
\mathsf{TargetBeh}_B(p,x,t)&:\mathcal U,\\
\mathsf{Conforms}_B(p)&:=\prod_{x,t}
\mathsf{TargetBeh}_B(p,x,t)\to\mathsf{SourceBeh}_L(p,x,t),\\
\mathsf{Correct}_L(p,P)&:=\prod_{x,t}
\mathsf{SourceBeh}_L(p,x,t)\to P(x,t).
\end{aligned}
\tag{8.4}
\]

The two proof terms compose to establish \(P\) for the target artifact. The first is the semantic-preservation evidence constructed for compilation and execution; the second is the property established for the source representation. Both are part of programming the actual computation. A displayed proposition alone is not a proof term; the compiler and checker must construct or verify its inhabitant. The source program, compiler, loader, target machine, and their proof terms can all be components of the encompassing hyperactivity program.



For a source program \(p\), target machine \(M\), build function \(C\), loader \(\mathsf{load}_M\), and target observation \(\mathsf{obs}_M\), the target behavior is the image of the loaded machine's execution relation:

\[
\mathsf{TargetBeh}_{C,M}(p,x,o):=
\left\|\sum_{\tau:\mathsf{Exec}_M(\mathsf{load}_M(C(p)),x)}
\mathsf{obs}_M(\tau)=o\right\|.
\tag{8.5}
\]

A compiler-refinement proof maps each such observed target execution to an allowed source observation. A source property holding for every allowed source observation therefore holds for the target artifact by function composition. The definition identifies the artifact, loader, machine, and observation explicitly; it introduces no new foundational notion of meaning.

## 8.3 Representations, operations, and memory

Consider a 32-bit unsigned integer. Three objects already appear:

\[
\begin{aligned}
\mathsf{Word}_{32}&:=\operatorname{Fin}(2^{32}),\\
\mathsf{Bytes}_4&:=\operatorname{Fin}(4)\to\operatorname{Fin}(256),\\
\mathsf{encodeLE}&:\mathsf{Word}_{32}\to\mathsf{Bytes}_4.
\end{aligned}
\tag{8.6}
\]

Define the little-endian representation by

\[
\mathsf{encodeLE}(w)(j)
=\left\lfloor\frac{w}{256^j}\right\rfloor\bmod256,
\quad j\in\{0,1,2,3\},
\tag{8.7}
\]

and decoding by

\[
\mathsf{decodeLE}(b)=
\sum_{j=0}^{3}256^j b(j)\pmod{2^{32}}.
\tag{8.8}
\]

The round-trip equations are

\[
\mathsf{decodeLE}(\mathsf{encodeLE}(w))=w,
\qquad
\mathsf{encodeLE}(\mathsf{decodeLE}(b))=b.
\tag{8.9}
\]

Thus the types are equivalent. By univalence the equivalence induces a path of types, and a dependent operation on words can be transported to the byte representation.

For example, define addition on words by

\[
\mathsf{add}_{32}(x,y):=(x+y)\bmod 2^{32}.
\tag{8.10}
\]

Its four-byte representation is defined by:

\[
\mathsf{addBytes}(a,b):=
\mathsf{encodeLE}\bigl(
  \mathsf{add}_{32}(\mathsf{decodeLE}(a),\mathsf{decodeLE}(b))
\bigr).
\tag{8.11}
\]

The commuting equation follows by the round-trip law:

\[
\mathsf{decodeLE}(\mathsf{addBytes}(a,b))
=\mathsf{add}_{32}(\mathsf{decodeLE}(a),\mathsf{decodeLE}(b)).
\tag{8.12}
\]

This is a complete representation calculation. It is not yet a theorem about an actual C++ load: the C++ semantics must establish which four bytes constitute a live `uint32_t` object, what alignment and provenance are required, whether the implementation uses little-endian representation, and whether the access is permitted under the language's object model. Nor is it a theorem about the electrical implementation of a CPU. Those are additional *premises*, not a different foundational language.

The same pattern applies to source syntax. A C++ declaration may have one representation as source tokens, another as a typed abstract syntax tree, another as an interpreter closure, and another as a target-machine artifact. The relevant relations are not automatically equivalences: parsing can discard comments; optimization can preserve observations without preserving instruction traces; compiling a source with unspecified behavior may select one permitted behavior. Precision consists in stating which relation actually holds.

### Enumerating, forgetting labels, and preserving graph structure

An enumeration of a finite set \(A\) is a pair \((n,e)\) with \(e:A\simeq\mathsf{Fin}(n)\). It retains a chosen labeling. A cardinality assertion
\(\mathsf{Card}(A):=\sum_{n:\mathbb N}\|A\simeq\mathsf{Fin}(n)\|\)
retains the number but not a chosen enumeration. The action of a permutation \(\sigma:\mathsf{Fin}(n)\simeq\mathsf{Fin}(n)\) changes the enumeration to \(\sigma\circ e\). Let \(Q_A\) be the **set quotient** of enumerations by this action, with points \(\mathsf{inc}(n,e)\), paths \(\mathsf{inc}(n,e)=\mathsf{inc}(n,\sigma\circ e)\), and set truncation. Finite cardinality uniqueness identifies the numbers of any two enumerations; at the common number their equivalences differ by a permutation. Hence \(Q_A\) is a proposition and the quotient recursor constructs an equivalence \(Q_A\simeq\mathsf{Card}(A)\). The forward map forgets the enumeration, and the reverse map eliminates truncation into the proposition \(Q_A\). This is a concrete quotient construction, not a claim that a chosen enumeration is identical to its cardinality.

For finite directed graphs, take \(\mathsf{Graph}_n:=\mathsf{Fin}(n)\to\mathsf{Fin}(n)\to\mathsf{Bool}\). Relabeling by \(\sigma\) acts as \((\sigma\cdot G)(i,j):=G(\sigma^{-1}i,\sigma^{-1}j)\). The set quotient \(\mathsf{Unlabelled}_n\) has a constructor \(\mathsf{incl}(G)\), paths \(\mathsf{incl}(G)=\mathsf{incl}(\sigma\cdot G)\), and set truncation. A set-valued invariant of graphs descends through the quotient precisely when it is supplied with the corresponding relabeling paths. This quotient deliberately forgets automorphism paths. To retain them, use the univalent structured-graph type

\[
\mathsf{GraphSym}_n:=\sum_{X:\mathcal U}\|X\simeq\mathsf{Fin}(n)\|\times(X\to X\to\mathsf{Bool}).
\]

Its identity between \((X,G)\) and \((Y,H)\) is equivalent to an equivalence \(e:X\simeq Y\) together with adjacency preservation \(\prod_{x,x':X} H(e(x),e(x'))=G(x,x')\). The truncated finiteness witness carries no further identity data. Node-indexed positions \(p:X\to\mathbb R^2\) move to \(p\circ e^{-1}:Y\to\mathbb R^2\); a selected vertex and dependent property proofs must move along the same structured equivalence. In particular, identifying graph shapes does not authorize leaving their node-indexed data untouched.

For a fixed labeled vertex type, put \(\mathsf{Matrix}_n:=\mathsf{Fin}(n)\to\mathsf{Fin}(n)\to\mathsf{Bool}\) and define the **type of canonical adjacency lists**, not merely the type of arbitrary lists, by
\[
\mathsf{AdjList}_n:=\sum_{L:\mathsf{Fin}(n)\to\mathsf{List}(\mathsf{Fin}(n))}
\prod_{i:\mathsf{Fin}(n)}
  (\mathsf{Sorted}(L(i))\times\mathsf{NoDup}(L(i))).
\]
Here \(\mathsf{Sorted}\) uses the standard decidable order on \(\mathsf{Fin}(n)\), and the displayed law components are propositions. Row membership gives \(\mathsf{AdjList}_n\to\mathsf{Matrix}_n\); enumerating each matrix row's true entries in increasing order gives the inverse. The matrix-to-list-to-matrix composite is pointwise identical by Boolean extensionality. The list-to-matrix-to-list composite is identical **because** each row is already sorted and duplicate-free, with the law witnesses identified by propositionality. Thus the equivalence is between \(\mathsf{Matrix}_n\) and \(\mathsf{AdjList}_n\), not between matrices and *all* lists. Without the canonicality laws, the latter composite yields a canonical representative rather than identity of arbitrary lists. Algorithms and correctness proofs are transported across the structured equivalence; graph relabeling and matrix/list conversion are distinct transformations.


### Indexed packing and transport of an operation

A representation equivalence can be constructed rather than merely postulated. With \(\mathsf{Vec}(A,n):=\mathsf{Fin}(n)\to A\), a pair of indexed families and a family over the disjoint union of their indices are equivalent:

\[
\begin{aligned}
\mathsf{pack}(x,y)(\mathsf{inl}(i))&:=x(i),&
\mathsf{pack}(x,y)(\mathsf{inr}(j))&:=y(j),\\
\mathsf{unpack}(z)&:=
(\lambda i.z(\mathsf{inl}(i)),\lambda j.z(\mathsf{inr}(j))),\\
e_{m,n}&:
\bigl(\mathsf{Vec}(A,m)\times\mathsf{Vec}(A,n)\bigr)\\
&\hspace{2em}\simeq\bigl(\mathsf{Fin}(m)+\mathsf{Fin}(n)\to A\bigr).
\end{aligned}
\]

The two composites are identities by pair equality and function extensionality. Given an operation \(f\) on the pair representation, its operation on the sum-indexed representation is \(\mathsf{pack}\circ f\circ\mathsf{unpack}\). A dependent specification of \(f\), including its preservation proof, is transported along \(\mathsf{ua}(e_{m,n})\) in the corresponding family of operations and specifications. This is a complete index-level change; a byte layout or native in-place update adds its own state and address correspondence. In particular, packing the index family does not silently establish alias freedom or machine-layout equivalence.

## 8.4 Executing an imperative operation is already a function

Consider a state containing a valid mutable 32-bit word at location \(a\), with the value \(w\). A source statement `*p += 1` is interpreted, under appropriate language premises, as a state transformation

\[
\mathsf{inc}_a:S_{\mathsf{valid}(a)}\to S_{\mathsf{valid}(a)},
\qquad
\mathsf{read}_a(\mathsf{inc}_a(s))
=\mathsf{add}_{32}(\mathsf{read}_a(s),1).
\tag{8.13}
\]
The complete specification must also state a frame condition for other locations and, for concurrency, which interleavings and memory-order relations are permitted. The familiar imperative operation is not less exact than the function in (8.11). Its domain simply includes more state and more premises.

A typed proof about this statement may inhabit

\[
\prod_{s:S_{\mathsf{valid}(a)}}
\mathsf{read}_a(\mathsf{inc}_a(s))
=\mathsf{add}_{32}(\mathsf{read}_a(s),1).
\tag{8.14}
\]

A change from one byte layout to another can transport the state representation and the dependent proof if a suitable equivalence of *structured machine states* has been established. The actual source expression, its typed syntax, its interpretation, and the proof remain separately identifiable constructions. Nothing requires a privileged sequence in which the proof is “attached,” then “lowered,” then “realized.” We may work on any of these objects and their relationships.

### A concrete C++ declaration and its interpretation

Consider the following source declaration in a specified C++ edition:

```cpp
#include <cstdint>
void increment(std::uint32_t* p) { *p += 1u; }
```

The parser yields a function declaration with one pointer parameter, a compound-assignment statement, a unary dereference, and an unsigned integer literal. Name lookup resolves `std::uint32_t` to the implementation's exact-width unsigned type when supplied; the static judgment requires a modifiable lvalue of that type and a well-formed compound assignment. A successful elaboration contains those resolved types and binding identities, not merely the source characters.

Let \(S_{p}\) be the states in which \(p\) designates a live, writable, correctly aligned `std::uint32_t` object, with no conflicting concurrent access during the call. Write \(\mathsf{load}_p:S_p\to\mathsf{Word}_{32}\) and \(\mathsf{store}_p:S_p\to\mathsf{Word}_{32}\to S_p\), with the frame law on disjoint objects. The selected sequential source semantics gives

\[
\begin{aligned}
\mathsf{increment}(s)&:=\mathsf{store}_p(s,\mathsf{load}_p(s)+_{32}1),\\
\mathsf{load}_p(\mathsf{increment}(s))&=\mathsf{load}_p(s)+_{32}1,\\
\mathsf{frame}_q(\mathsf{increment}(s))&=\mathsf{frame}_q(s)
\quad(q\text{ disjoint from }p).
\end{aligned}
\tag{8.15}
\]

The first equation is the denotation of this particular statement under its stated premises; the next two follow from the store/load and frame laws. The memory event trace contains a read at \(p\) and a write at \(p\), in the source sequencing order. An equivalent byte-level representation of the object is obtained from (8.6)–(8.12) by transport along the structured state equivalence. The source text, its elaborated syntax, its abstract-machine state transformation, and any compiled target behavior are distinct representations linked by the displayed equations and the chosen compiler refinement theorem.

## 8.5 Machine representations and memory laws

Values admit concrete representations used by existing programs and machines. The following definitions make explicit the distinction between a reversible representation change and a one-way refinement. 

### Value representations and their laws

A representation changes a type by an equivalence or relates a concrete state to an abstract state by a stated relation. These are different objects. When an equivalence is available, univalence supplies transport. When information is hidden or discarded, the relation or quotient must say what is hidden. The graph equivalence (3.6)–(3.7) provides the information-preserving factorization; observational refinements are defined in §8.2.

For a finite array, use the equivalence between length-indexed sequences and \(\mathsf{Fin}(n)\to A\) already established in §4, with its `index`/`tabulate` round trips. Its application to a mutable array is specified by the following update law.

Transport of an update operation is characterized by

\[
\mathsf{index}(\mathsf{update}(v,i,a),j)
=\begin{cases}a&j=i,\\\mathsf{index}(v,j)&j\ne i.\end{cases}
\tag{8.16}
\]

That equation is the obligation connecting an in-place or packed implementation to the array operation. Merely having maps between the element containers is insufficient.

For a matrix `Fin m × Fin n → A`, a bijection between the product index and `Fin(mn)` specifies a flat layout. Row-major and column-major layouts are two such bijections. Changing the layout transports the indexed data and any dependent construction over it. Choosing strides and alignment for actual bytes adds the encoding and address arithmetic below. The same argument applies to tensor layouts; it is not a separate tensor foundation.

### Exact scalar types

The unsigned word type is

\[
U_w=\mathsf{Fin}(2^w).
\]

Addition and multiplication take their natural-number result modulo `2^w`; subtraction uses the corresponding residue. Bitwise operations act on the `w` binary digits. A native primitive implements these definitions under a checked theorem or a named primitive assumption.

The language distinguishes `Nat`, the runtime's unbounded natural representation `Word`, and fixed-width words such as `U64`. A fast small-integer representation of `Nat` may spill into larger storage. That does not make it equivalent to `U64`. A bounded subset `{n : Nat | n < 2^64}` is equivalent to `U64`, but unbounded addition is not a closed operation on that subset without a bound on its result.

Division, shifts, and signed interpretation have explicit definitions. A host-language fixed-width operation may define division by zero as a distinguished result or reduce a shift count modulo the width. A lifted C++ operation uses C++'s specified definedness conditions instead. The primitive's convenience convention must never silently replace the source language's behavior. Signed two's-complement storage and signed arithmetic semantics are separate choices; the bit pattern alone does not justify an overflowing C++ signed operation.

Floating-point values include the specified format, signed zeros, infinities, NaNs, and their bit-level distinctions when observed. Operations depend on the selected rounding and exception environment. Fused and unfused operations are different functions. Algebraic transformations must use laws valid for the selected semantics: associativity over real numbers does not license unrestricted reassociation of floating-point addition. A numerical specification may instead prove a bounded error relation to a real-valued function.

### Bytes, layouts, and validity

Let `Byte = U8` and `Bytes n = Fin n → Byte`. A fixed-size encoding of `A` consists of

\[
\mathsf{enc}:A\to\mathsf{Bytes}(n),\qquad
\mathsf{dec}:\mathsf{Bytes}(n)\to\mathsf{Option}(A).
\]

The minimum round-trip law is `dec(enc(a)) = some(a)`. If canonical byte representations are required, also supply `enc(a)=b` whenever `dec(b)=some(a)`. Without the second law, padding or redundant encodings remain distinguishable at the byte level. A full equivalence is with the type of valid canonical encodings, including any necessary evidence, not automatically with all `n`-byte strings.

For little-endian `U64`, the concrete encoding is

\[
\mathsf{enc}(x)(i)=\left\lfloor x/256^i\right\rfloor\bmod256,
\quad
\mathsf{dec}(b)=\sum_{i=0}^{7}256^i\,b(i).
\tag{8.17}
\]

The range bounds establish both round trips. A load from a larger byte region additionally needs the offset bound, the architecture's alignment condition where applicable, and the source memory semantics' permission to access those bytes as the requested value. An integer inequality alone does not establish object lifetime or pointer validity.

Records combine field encodings at specified offsets with alignment and padding rules. Sums additionally encode the tag and its relation to the active payload. Recursive structures use addresses and allocation metadata rather than an infinite flat record. ABI layouts include all observable details: calling convention, alignment, padding, tag placement, register classes, return convention, and the applicable object and exception ABI.

### Memory as state

Axiomatic reasoning about mutable memory has established accounts in Hoare logic and separation logic ([17](#ref-17), [37](#ref-37)); the state construction below specifies the concrete memory operations being discussed.

An elementary memory model is a map `M : Addr → Byte` together with a domain of allocated addresses. A store is a state transformer:

\[
\mathsf{store}(M,a,v)(b)=
\begin{cases}v&b=a,\\M(b)&b\ne a.\end{cases}
\tag{8.18}
\]

The read-after-write and frame laws follow immediately. They are proofs about the operation that changes memory. Manual mutation does not displace functional reasoning; it chooses the state transition being reasoned about.

A source-language memory state usually contains more than bytes: allocation identities, extents, lifetimes, permissions, pointer provenance, active union members, and synchronization events as required by that language. An address-only model is a useful target or abstract model when a theorem or explicit assumption connects it to the richer one. Reusing the same physical address after deallocation does not by itself resurrect an old source pointer.

An allocation operation consumes a state and returns either an allocation failure or a pointer and an updated state satisfying the allocation conditions. A region allocator can maintain an interval `[base,end)` and a cursor. Allocation of `n` aligned bytes computes the aligned cursor and advances it only after checking that the calculation and resulting range are valid. Deallocation ends the applicable lifetime. The logical state can contain metadata erased from the native representation after its uses have been discharged.

Separation is a statement about disjoint resources. A frame theorem says that a transition operating on one disjoint component preserves the other. Linear or affine usage information can make exclusive access apparent from the term's uses; it does not replace the semantic statement about aliases. Runtime graph reference counts and source-level ownership are distinct objects, even when the implementation uses both to avoid copying.

For an update to a persistent value, unique use can license destructive reuse of its storage. If another live reference may observe the old value, the implementation must preserve it, for example by copying or by a proved sharing discipline. The transformation is accepted because it preserves the source observations, not because a store happens to be fast.

### A loop with its actual property

The modular-sum loop and its invariant were established in (7.4). In the C++ interpretation below, the same invariant is applied to the actual source-language memory and integer semantics, including the array-access premises and unsigned wraparound law. No second loop theorem is introduced.

## 8.6 C++ static and dynamic semantics

### C++ static semantics

The C++ frontend retains both source structure and elaborated choices. Its source input includes preprocessing context, headers or module units, macro definitions, translation options, and translation-unit boundaries. Its static semantics includes at least the following complete categories, each governed by the selected standard's clauses:

| Category | Information preserved in the lifted object |
|---|---|
| Translation and preprocessing | Tokenization, macro expansion, source inclusion, conditional translation, modules, linkage, and one-definition requirements |
| Names and declarations | Scopes, lookup, argument-dependent lookup, access, declaration identity, and overload sets |
| Types and expressions | Value categories, cv qualification, conversions, initialization, overload resolution, and the implicit operations selected |
| Classes | Base subobjects, inheritance, constructors, destructors, special members, virtual operations, and member access rules |
| Templates and constraints | Deduction, substitution, partial ordering, specialization, dependent lookup, instantiation, SFINAE, and constraint satisfaction |
| Constant evaluation | The source rules for permitted constant expressions, diagnostics, and the result of evaluation |
| Program assembly | Translation-unit relationships, instantiated entities, required definitions, and library/ABI dependencies |

A template is represented as a program parameterized by its template arguments, but its interpretation includes these C++ rules. Treating every template declaration as an unconstrained type-level lambda would lose deduction, lookup, overload selection, and substitution-failure behavior. Ordinary function application describes the resulting explicit parameterization after those source rules have been accounted for.

An elaborated Clang AST is useful because many choices are explicit there. If Clang supplies those choices, its use is a declared frontend assumption unless its output is independently validated against the source semantics or the relevant frontend implementation has the required connection. Running a lifted Clang preserves what that Clang does; it does not alone establish standard conformance. Source mapping is retained so that properties and transformations attach to the actual declarations and expressions.

### C++ dynamic semantics and memory

The C++ process state contains the control state, object state, thread state, and external-world interface required by the standard. A concrete ABI realization additionally contains the selected layouts and runtime data. The following distinctions are not erased by mapping operations to byte loads and stores:

| Category | Required semantics |
|---|---|
| Objects and storage | Allocation, alignment, storage duration, initialization, lifetime start/end, subobjects, union members, implicit creation, and pointer validity/provenance |
| Expressions | Sequencing, permitted evaluation orders, side effects, value categories, temporary materialization, and destruction |
| Pointers and references | Permitted arithmetic and comparisons, one-past values, casts, references, pointer-to-member representations, and invalidation |
| Class execution | Base/member initialization, dynamic type during construction/destruction, virtual dispatch, RTTI, and destructor effects |
| Control transfer | Return, jumps, exceptions, handler selection, stack unwinding, cleanup order, and termination behavior |
| Coroutines | Creation and lifetime of the frame, suspension points, resumption, completion, and destruction |
| Arithmetic | Widths and conversions, signed definedness conditions, floating-point environment, and implementation-defined results |
| Concurrency | Threads, thread-local state, atomics and all their ordering parameters, fences, synchronization, data races, and forward-progress requirements |
| Library and environment | Standard-library contracts and implementation, allocation interfaces, I/O, termination, and platform extensions including system calls |

These categories include bit-fields, variadic calls, captures, placement and class-specific allocation, over-alignment, static and thread-local initialization and destruction, multiple and virtual inheritance, and library operations with nonlocal control effects. Volatile access, atomics, and device memory retain their respective contracts rather than being merged into one generic load. The selected source and platform specifications determine each case.

The source abstract machine does not prescribe a vtable layout. The ABI realization does. A virtual call in the source semantics selects the specified overriding operation; its ABI connection explains the vtable access, `this` adjustment, and call sequence. Similarly, an exception's source cleanup behavior is connected to the chosen unwinding tables and runtime. Keeping these levels distinct permits both source reasoning and exact inspection of existing machine layouts.

The semantic classes must remain distinct. **Implementation-defined behavior** is selected by a documented implementation/profile parameter \(\theta\). **Unspecified behavior** is represented by the permitted choices in a transition relation, not by silently choosing one branch in a deterministic interpreter. **Undefined behavior** marks a violation for which the selected standard places no corresponding behavioral constraint on the program execution; it is not a distinguished, harmless return value. In a behavior-set semantics, a source execution classified as undefined may be represented by a top set of permitted observations, so a refinement theorem cannot rule out a target observation merely because the source reached that classification. Whether the semantics retains an observable prefix before undefined behavior is an *explicit choice of the formalization*: one cannot infer that the standard guarantees that prefix. For defined source executions, the target behavior must satisfy the ordinary observation refinement of §8.2.

This classification is also the boundary for a Clang-based lift. An elaborated AST, together with its preprocessing context, selected overloads, instantiated templates, and source locations, is a derivation-shaped object for *some* static judgments. Importing it into \(\mathsf{StdDeriv}\) requires a stated translation and evidence that each imported elaboration decision satisfies the selected source-rule instances. The translation is an explicit frontend assumption until established; it is not automatically the inverse map \(g\) of (5.13), which concerns an already selected formal signature. Proving that the actual frontend constructs only admitted elaborations would discharge that assumption for its specified input domain. A complete C++ fidelity claim additionally covers dynamic semantics and the external prose-to-rule correspondence; frontend soundness alone does not establish those.

A particular implementation's behavior after an undefined source execution can still be studied through its own semantics.

Ill-formed programs and required diagnostics belong to static semantics. Ill-formed-no-diagnostic-required cases must retain that classification rather than being silently treated as valid source. Extensions are identified as extensions. These classifications matter when the lifted term is used to claim portability rather than merely preserve a deployed binary.

### A bounded C++ frame decoder: interpreted source and proof

Fix the C++20 abstract machine, a live readable byte array \(b\) of length \(n\), a disjoint writable live \(\mathsf{Frame}\) object at \(\mathsf{out}\), and no concurrent mutation of the input. The source program is:

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

Let \(\mathsf{tag}=b_0+2^8b_1\) and \(\ell=b_2+2^8b_3\). Its interpreted source behavior is the dependent specification, including the memory frame condition:

\[
\begin{cases}
\mathsf{false}\text{ with output unchanged},&n<4,\\
\mathsf{false}\text{ with output unchanged},&n\ge4\ \land\ \ell>n-4,\\
\mathsf{true}\text{ with output }(\mathsf{tag},\ell,b+4),&n\ge4\ \land\ \ell\le n-4.
\end{cases}
\tag{8.19}
\]

The first branch precedes every byte access. In the remaining branches \(n\ge4\), so \(n-4\) is defined without unsigned underflow, and the four header reads are in bounds. If \(\ell>n-4\), no output write has occurred. Otherwise \(4+\ell\le n\), so the returned payload view lies within the input allocation; the three output writes touch only the distinct, live \(\mathsf{Frame}\). Header arithmetic is exact since \(0\le b_i<256\) and \(\mathsf{tag},\ell<2^{16}\). The source proof is a term about the *actual interpreted C++ declaration*, not an independently rewritten decoder model. A source annotation can attach the proof through the erasure construction in §6, and its erasure returns this C++ body. Replacing two byte reads by a 16-bit load is a separate transformation whose proof must establish alignment, aliasing, endian interpretation, and memory-validity premises. This example supplies the source-level memory proof distinct from the pure bytecode example in §9.

# 9. Native execution as a program transformation

Having represented a language's meaning and a machine's transitions, the next question is not whether both can be called programs, but what relation makes the machine's behavior a realization of the source. The answer must be an explicit simulation or refinement relating the actual observations of the two. Hoare logic, compiler correctness, and verified source-to-machine refinement have extensive prior art ([17](#ref-17), [18](#ref-18), [22](#ref-22)). LLVM provides an influential reusable intermediate representation and transformation infrastructure ([30](#ref-30)). The examples here formulate source and target machines and their correspondence as terms and proofs. The implementation route is C followed by an existing C compiler (§9.4). The small decoded machine below exposes the compositional argument; it does not prescribe a new instruction-set backend.

An executable artifact is a machine representation of the program being constructed. Emitting the artifact is a program transformation; the accompanying correctness derivation establishes how the artifact's actual machine execution realizes the source program's specified behavior. The emitted bytes are not a second system merely modeled by the first.

For a chosen target machine \(M\), an encoding is a function from a program representation to a finite sequence of machine bytes, together with its decoding, loading, and execution semantics. A correctness statement relates observations:

\[
\mathsf{TargetBeh}_M(\mathsf{compile}(p),x,t)
\;\Longrightarrow\;
\mathsf{SourceBeh}(p,x,t).
\tag{9.1}
\]

The direction shown is refinement: a compiled artifact must not exhibit behavior forbidden by its source semantics. Under stronger hypotheses one may prove equivalence of selected observations.

A complete small instance makes this concrete. Let the stack machine have instructions

\[
\mathsf{PUSHINPUT},\quad
\mathsf{PUSHCONST}(b),\quad
\mathsf{XOR},\quad
\mathsf{HALT},
\tag{9.2}
\]

encoded respectively by bytes `01`, `02 b`, `03`, `ff`. The expression compiler is

\[
\begin{aligned}
C(\mathsf{Input})&=[01],\\
C(\mathsf{Const}(b))&=[02,b],\\
C(\mathsf{Xor}(u,v))&=C(u)++C(v)++[03].
\end{aligned}
\tag{9.3}
\]

The exact correctness theorem is stronger than “the result is right”: for any initial stack \(S\), running \(C(e)\) with input \(x\) transforms \(S\) into \(E(e,x)::S\), leaving the prior stack untouched. The induction step for XOR composes the two induction hypotheses and applies the stack XOR instruction.

For the XOR expression of §5.6 with \(k=0x5a\), the complete artifact is

```text
01 02 5a 03 02 5a 03 ff
```

The execution pushes \(x\), pushes `5a`, XORs, pushes `5a`, XORs again, and halts with \(x\). This byte-level construction has a compositional proof; §9.2 establishes the universal evaluator-mediated realization under its stated premises.

At the other end of the spectrum, a target may be an x86-64 process, GPU kernel, network protocol, or whole machine. Their semantics include calling conventions, layouts, floating-point rules, loader behavior, world interactions, and observable costs. Compilation and interpretation can be represented and checked in the same language; their correctness is conditional on the actual target semantics supplied.


## 9.1 The byte compiler theorem

The source syntax and interpreter are defined in §5.6. The exact decoder, machine transition system, compiler recursion, and stack-extension proof appear together in §9.3; they are not redefined here.

## 9.2 Universal native realization by one evaluator

An interpreter is already a program. This observation gives a universal source-to-machine construction rather than merely a fallback for source constructors lacking direct instruction-selection rules.

Fix the full represented language \(L\), its checked code presentations \(\mathsf{Code}_L\), and an observation semantics \(\llbracket c\rrbracket_L\) that includes the permitted value, fault, divergence, and external interaction observations. Let \(V\) contain the evaluator's code, environment, shared-graph references, continuation frames, and pending world requests. For a command signature \(\Sigma=(\mathsf{Cmd},\mathsf{Resp})\) and returned values \(A\), one possible step-result presentation is

\[
\begin{aligned}
\mathsf{EvalStep}_\Sigma(V,A)
={}&\mathsf{Continue}(V)+\mathsf{Return}(A)
+\mathsf{Fault}(\mathsf{Err})\\
&+\sum_{q:\mathsf{Cmd}}\bigl(\mathsf{Resp}(q)\to V\bigr),\\
\mathsf{next}:&\ V\to\mathsf{EvalStep}_\Sigma(V,A).
\end{aligned}
\tag{9.U1}
\]

For this schematic presentation, either the configurations and results are indexed by the checked code's return type, or `A` is a single universal value representation whose interpretation records that type. The displayed unindexed `V` must not be read as allowing a configuration of one result type to return an unrelated `A`. The last summand records the response-indexed continuation; it does not pretend that issuing a command determines its response. A defunctionalized continuation identifier and environment can replace the displayed function when representing \(V\) in a first-order machine fragment. The constructors and their handler interpretation agree with the open-process definitions of §7.6. If resource failure is modeled separately, the step result also carries an explicit \(\mathsf{OutOfResources}\) case, or the theorem below is restricted by a resource-sufficiency premise.

**Universal native realization (conditional construction).** Suppose:

1. A machine-realizable fragment \(F\) expresses the *chosen evaluator algorithm* \(\mathsf{next}\), including its concrete representation and required allocator, stack, and handler interfaces. This is an expressibility premise, not a conclusion that all cubical operations are directly compiled.
2. Compiling that one term gives bytes \(b_{\mathsf{next}}\), an initialization/encoding relation \(R_V\) between target states and evaluator configurations, **a total encoding operation for every admitted initial configuration** satisfying \(R_V\), and a machine simulation \(\nu\) for the evaluator's actual transition cases. A machine segment may take multiple target steps per evaluator step, so \(\nu\) is a labeled weak simulation with progress or divergence conditions appropriate to the observation.
3. Evaluator adequacy \(\alpha\) relates the guarded or finite run of \(\mathsf{next}\), beginning at \(\mathsf{initial}(c)\), to \(\llbracket c\rrbracket_L\), including the same external command/response behavior. Initialization, loading, resource conditions, and world-handler correspondence hold.

Then **every represented term** \(c:\mathsf{Code}_L\) has a target realization

\[
\begin{aligned}
\mathsf{realize}^{\mathsf{eval}}_M(c)
&:=\bigl(b_{\mathsf{next}},\
\mathsf{encode}(\mathsf{initial}(c)),\
\nu^{\!*}\mathbin{;}\alpha\bigr),\\
\mathsf{TargetBeh}_M
(\mathsf{realize}^{\mathsf{eval}}_M(c),o)
&\Longrightarrow\mathsf{SourceBeh}_L(c,o).
\end{aligned}
\tag{9.U2}
\]

Equivalently, displaying the intermediate relations rather than only their composite, the construction has the form

\[
\mathsf{run}_M(b_{\mathsf{next}},\mathsf{encode}(\mathsf{initial}(c)))
\;\mathrel{\mathcal R_{\nu}^{*}}\;
\mathsf{evalRun}(\mathsf{initial}(c))
\;\mathrel{\mathcal R_{\mathsf{eval}}}\;
\llbracket c\rrbracket_L .
\tag{9.U2a}
\]

The first relation is machine/evaluator simulation and the second is evaluator adequacy; neither is inferred from the other. The relation notation abbreviates observation-preserving correspondence, not literal equality of the three objects.

Here \(\nu^{\!*}\) denotes finite iteration or guarded continuation of the *labeled* machine/evaluator simulation, and the semicolon is composition of evidence, not equality of machine states. The machine-simulation premise includes progress for each admitted evaluator transition (or an explicit related fault/resource outcome), together with matching divergence and fairness conditions whenever the selected observation distinguishes them. Under those premises, the realization does not merely exclude incorrect target observations; it can actually follow each admitted evaluator run through the corresponding finite or guarded target behavior. An executable image may be common to all \(c\); the encoded initial configuration varies. The statement is relative to the target machine and world model, including resource and loader conditions. An emitted command with no supplied response remains an explicit world obligation; the machine simulation cannot silently discharge it. 

**Closure under reflection.** If the checker, specializer, compiler, and evaluator are themselves represented terms of \(L\), they are among the \(c\) covered by (9.U2). This is *executional* closure of the metacircle: the native evaluator can run programs that inspect, transform, and realize programs, including their own presentations. The inclusion requires checked representations and an evaluator implementing their computation rules. Checker soundness and compiler correctness retain their separate proof obligations; the realization theorem uses the independently established premises \(\alpha\) and \(\nu\).

The adequacy premise \(\alpha\) is used when a realization *executes the represented evaluator*. A directly compiled residual can instead be related to the source term by its checked source-level conversion/path and its own machine-block evidence, without inserting the retained evaluator's machine simulation \(\nu\) as an extra proof obligation. If a mixed artifact retains an evaluator call on runtime code, that stretch uses \(\nu\) and the relevant instance of \(\alpha\). The proof dependencies are local to the realization actually used. An evaluator over raw syntax can be expressed as a guarded process, including nontermination, without a total normalizer or a universe-level collapse; typed quotation, reification, and the Futamura code equations still obey their explicit stages.

The consequence is precise: **direct compilation of every constructor of \(L\) is unnecessary for universal native executability**, provided the displayed evaluator and machine premises are established. Direct compilation becomes a semantics-preserving optimization of this general realization. The native fragment \(F\) still matters for realizing the evaluator, but it does not delimit the set of represented source terms that can be run. The small byte-machine construction in §9.3 illustrates how machine relations are proved; §9.4 supplies the selected route through C. The architecture-specific operations retained in Appendix C illustrate another possible realization, not the definition of the general construction or a required backend. The evaluator-mediated realization is a *sufficient universal construction*; other realizations can be related directly to the same represented computation.

An executable artifact may instead mix direct segments and evaluator-mediated steps. Such a choice is a program transformation with a relation on its source and target states; the details appear after the structural compiler algebra below. Differences in latency, memory, energy, rounding, and observable scheduling are retained when they belong to the selected observation type. Logical reversibility does not itself establish zero physical dissipation. Any physical-cost assertion depends on the actual realization and the thermodynamic assumptions of the chosen model. Landauer’s and Bennett’s work distinguishes the logical operation of erasure or reversal from the thermodynamics of its physical realization ([61](#ref-61), [62](#ref-62)); information-theoretic entropy in statistical mechanics has a further established account in Jaynes ([63](#ref-63)).

## 9.3 An exact decoded machine and a structural compiler theorem

This construction closes a small but nontrivial instance of the general compiler algebra: the source has arbitrarily nested expressions, the target has an explicitly decoded byte language, and the compiler's correctness is proved by structural induction. 

### Source syntax and meaning

Fix `Byte = Fin(256)`, represented by eight bits, and write \(a\oplus b\) for pointwise Boolean exclusive-or. The source expressions are:

\[
\begin{aligned}
\mathsf{Expr}&::=\mathsf{Input}\mid\mathsf{Const}(b)
  \mid\mathsf{Xor}(e_1,e_2),\\
E(\mathsf{Input},x)&:=x,\\
E(\mathsf{Const}(b),x)&:=b,\\
E(\mathsf{Xor}(u,v),x)&:=E(u,x)\oplus E(v,x).
\end{aligned}
\tag{9.4}
\]

### Target machine, decoder, and execution

A state consists of a program counter, a byte stack, a fixed input byte, and either `running`, `halted`, or `fault`:

\[
\mathsf{State}:=\mathbb N\times\mathsf{List}(\mathsf{Byte})
\times\mathsf{Byte}\times\mathsf{Status}.
\tag{9.5}
\]

The **top of the stack is the leftmost element**. Let \(B\) be a finite byte array of length \(|B|\), addressed from zero. Instruction fetch reads \(B[pc]\), trapping when \(pc\geq|B|\). The complete instruction table is:

| Encoding | Length | Preconditions | State transition |
|:--|--:|:--|:--|
| `01` | 1 | running | `(pc,S,x) ↦ (pc+1,x::S,x)` |
| `02 b` | 2 | running; `pc+1 < |B|` | `(pc,S,x) ↦ (pc+2,b::S,x)` |
| `03` | 1 | running; `S=b::a::R` | `(pc,S,x) ↦ (pc+1,(a xor b)::R,x)` |
| `ff` | 1 | running | `(pc,S,x) ↦ (pc+1,S,x,halted)` |

Every other opcode, truncated `02`, or stack underflow transitions to `fault` without further instruction execution. The input byte is immutable. Code bytes are immutable during a run. The `03` instruction consumes the top two stack values in last-in-first-out order. XOR is commutative, so the order is immaterial for this source language.

Define \(\mathsf{step}_B:\mathsf{State}\to\mathsf{State}\) by this exhaustive table, with `halted` and `fault` states absorbing. Define \(\mathsf{run}_B^k\) by primitive recursion on \(k\), and let \(\mathsf{trace}_B(s,t)\) be a finite list of successive `step` states starting at \(s\) and ending at \(t\). The latter records the actual decoded transitions, not merely an asserted final result.

### Compilation as concatenation of bytes

\[
\begin{aligned}
C(\mathsf{Input})&:=[01],\\
C(\mathsf{Const}(b))&:=[02,b],\\
C(\mathsf{Xor}(u,v))&:=C(u)\mathbin{++}C(v)\mathbin{++}[03],\\
\mathsf{program}(e)&:=C(e)\mathbin{++}[ff].
\end{aligned}
\tag{9.6}
\]

Let \(\#(e)\) be the number of source AST nodes. Every node contributes exactly one target instruction, so the number of target instructions in \(C(e)\) is \(\#(e)\). The byte length is computed structurally:

\[
\begin{aligned}
\ell(\mathsf{Input})&=1,&\ell(\mathsf{Const}(b))&=2,\\
\ell(\mathsf{Xor}(u,v))&=\ell(u)+\ell(v)+1,&
|C(e)|&=\ell(e).
\end{aligned}
\tag{9.7}
\]

### The stack-extension invariant

For any stack suffix \(S\), input \(x\), and code prefix position \(a\), assume the bytes \(C(e)\) occupy exactly the interval \([a,a+\ell(e))\) of an immutable code array \(B\). The following theorem is stronger than a top-level output claim because it is stable under arbitrary surrounding stack contents:

\[
\boxed{
\begin{gathered}
\mathsf{run}_B^{\#(e)}
 (a,S,x,\mathsf{running})\\
=\bigl(a+\ell(e),E(e,x)::S,x,\mathsf{running}\bigr).
\end{gathered}}
\tag{9.8}
\]

**Proof by structural induction.** For `Input`, the decoder reads `01` and pushes \(x\). For `Const(b)`, it reads `02 b` and pushes \(b\). For `Xor(u,v)`, apply the induction hypothesis to \(u\) with suffix \(S\), obtaining \(E(u,x)::S\); then apply it to \(v\) with suffix \(E(u,x)::S\), obtaining \(E(v,x)::E(u,x)::S\). The final byte `03` pops both and pushes their XOR. Its program counter advances one byte, producing exactly \(a+\ell(u)+\ell(v)+1\). The trace contains \(\#(u)+\#(v)+1=\#(\mathsf{Xor}(u,v))\) transitions. No branch can fault because the induction hypotheses guarantee sufficient bytes and the final stack has two elements. \(\square\)

The theorem is stable under placement in a larger byte array because each instruction's decoder reads only its own encoding bytes and the code is immutable. It therefore supplies the precise prefix-stability condition required by block concatenation.

Appending `ff` yields the complete program theorem:

\[
\begin{aligned}
B&:=\mathsf{program}(e),\\
\mathsf{run}_B^{\#(e)+1}
 (0,[],x,\mathsf{running})
&=\bigl(|B|,[E(e,x)],x,\mathsf{halted}\bigr).
\end{aligned}
\tag{9.9}
\]

### A byte-for-byte execution

Let
\[
 e:=\mathsf{Xor}
  (\mathsf{Xor}(\mathsf{Input},\mathsf{Const}(0x5a)),
   \mathsf{Const}(0x5a)).
\]
Then

```text
source:  Xor(Xor(Input, Const(5a)), Const(5a))
bytes:   01 02 5a 03 02 5a 03 ff
address: 00 01 02 03 04 05 06 07
```

For arbitrary input \(x\), the machine trace is:

| Step | PC | Stack (top first) | Next operation |
|--:|--:|:--|:--|
| 0 | 0 | `[]` | input |
| 1 | 1 | `[x]` | push `5a` |
| 2 | 3 | `[5a,x]` | XOR |
| 3 | 4 | `[x xor 5a]` | push `5a` |
| 4 | 6 | `[5a,x xor 5a]` | XOR |
| 5 | 7 | `[x]` | halt |
| 6 | 8 | `[x]` | halted |

The fifth transition uses the Boolean identity \((x\oplus k)\oplus k=x\), proved by exhaustive bit cases. Thus the *exact eight bytes* implement the source term and preserve the proof attached to its meaning.

### Composition with source transformations

Let \(T:\mathsf{Expr}\to\mathsf{Expr}\) and
\(\tau:\prod_e\prod_x E(T(e),x)\equiv E(e,x)\) be a verified source transformation. The native correctness equation composes with \(\tau\):

\[
\begin{aligned}
\mathsf{run}_{\mathsf{program}(T(e))}^{\#(T(e))+1}
 (0,[],x,\mathsf{running})
&=\bigl(|\mathsf{program}(T(e))|,
   [E(T(e),x)],x,\mathsf{halted}\bigr)\\
&\equiv\bigl(|\mathsf{program}(T(e))|,
   [E(e,x)],x,\mathsf{halted}\bigr).
\end{aligned}
\tag{9.10}
\]

The final equality is a path in the *result components with the same compiled program and byte length*. It does not identify the byte strings for \(e\) and \(T(e)\), which may differ. This is the precise distinction between semantic preservation and byte-for-byte identity.

### Placed blocks, relocation, and compositional execution

An executable byte block must carry its entry address, precondition, postcondition, decoded machine execution, and the placement conditions needed for composition. The **single authoritative definition** is the address-indexed \(\mathsf{Block}(a,P,Q)\) of (9.12), with composition (9.13). Concatenation alone is not a proof: the second block must be placed at the first block's exit address, relocation must preserve the branch targets, and decoding the first block inside the concatenated artifact must be stable. The proof of (9.13) is the concatenation of the two exact machine traces under those premises. A compiled whole-program artifact is obtained by composing such blocks and the actual loader, ABI, and deployment semantics; no second unindexed block algebra is introduced here.

A verified machine emitter has a dependent result type rather than an assumed source/target equality:
\[
\mathsf{realize}_M:\prod_{p:\mathsf{Program}}
\sum_{b:\mathsf{Bytes}}
\prod_{x:\mathsf{Input}(p)}
\mathsf{Rel}_{O}\bigl(\mathsf{run}_M(b,\mathsf{init}_M(x)),
                         \llbracket p\rrbracket(x)\bigr).
\]
The relation \(\mathsf{Rel}_O\) is defined over the chosen source and target observations, and includes the required loader and ABI premises. An interpreter artifact is one realization; specialization or native instruction selection gives others. The executable bytes, rather than an unrelated model, occur in the proof's own type.

The architecture-specific frame-decoder calculation is retained in Appendix C.1. It is an example of a direct machine relation, not the compilation route selected here; the primary C route is developed in §9.4.

### Decoder stability and constructor-indexed compiler algebra


Fix a target instruction set whose machine state is a type `S`, and a byte decoder `decode : Bytes → Addr → Option Instr`. Define `exec : Instr → S → S + Trap` and a fetch/execute transition

\[
\delta_T(b,s)=\begin{cases}
\mathsf{trap}(\mathsf{decodeFault})&\mathsf{decode}(b,pc(s))=\mathsf{none},\\
\mathsf{exec}(i,s)&\mathsf{decode}(b,pc(s))=\mathsf{some}(i).
\end{cases}
\tag{9.11}
\]

`decode` includes instruction length and the updated instruction pointer. An ISA model must define registers, flags, memory accesses, traps, instruction bytes, and the machine's observable events. Merely writing an instruction mnemonic is not an encoding semantics.

### A block is bytes with a contextual execution proof

Fix a target machine with an immutable instruction-fetch image \(C\). Let \(\mathsf{Contains}(C,a,b)\) state that bytes \(b\) occur at address \(a\) with valid bounds, and let \(\mathsf{Admissible}(C,a,b)\) be the **explicit image predicate** asserting containment and all additional instruction-decoding and placement conditions required by the block. These may include valid branch destinations, the bytes on which instruction decoding depends, and permitted executable regions. Let \(\mathsf{Exec}^{*}_C(s,t,s')\) be the finite execution relation with event trace \(t\). For state precondition \(P\), postcondition \(Q\), and trace contract \(\mathcal R\), define

\[
\begin{aligned}
\mathsf{Block}_{\mathcal R}(a,P,Q):=
\sum_{b:\mathsf{Bytes}}\prod_C
\mathsf{Admissible}(C,a,b)\to
\prod_{s:S}(P(s)\times(pc(s)=a))\to\\
\sum_{t,s'}\mathsf{Exec}^{*}_C(s,t,s')
\times Q(s')\times(pc(s')=a+|b|)
\times\mathcal R(s,t,s').
\end{aligned}
\tag{9.12}
\]

The code image is quantified over rather than fixed to an isolated block, but **admissibility is an actual argument of the block proof**. The distinction is necessary: the state predicate \(P(s)\) alone cannot constrain an independently quantified code image \(C\). A branch whose target lies outside the block, or a decoder whose result depends on bytes beyond its end, therefore needs an admissibility premise stronger than \(\mathsf{Contains}\). A closed, position-independent straight-line block may use containment alone when the decoder's locality has been proved. The premises of \(P(s)\) include memory permissions, stack and ABI conditions; the transition preserves the immutable code image. Unit trace contracts recover ordinary Hoare-style blocks.

For trace contracts \(\mathcal R_1,\mathcal R_2\), let their sequential composition retain the intermediate state and the decomposition \(t=t_1{+\!+}t_2\). If \(B_1\) is certified at \(a\) and \(B_2\) is certified at \(a+|\mathsf{bytes}(B_1)|\), composition is

\[
\begin{aligned}
\mathsf{seq}:&
\prod_{B_1:\mathsf{Block}_{\mathcal R_1}(a,P,Q)}\\
&\prod_{B_2:\mathsf{Block}_{\mathcal R_2}(a+|\mathsf{bytes}(B_1)|,Q,R)}\\
&\mathsf{StablePrefix}(B_1,B_2)
\to\mathsf{RelocOK}(B_1,B_2)
\to\mathsf{Block}_{\mathcal R_1;\mathcal R_2}(a,P,R).
\end{aligned}
\tag{9.13}
\]

Take a single \(C\) admissibly containing \(b_1{+\!+}b_2\) at \(a\). The **content** of the \(\mathsf{StablePrefix}\) and \(\mathsf{RelocOK}\) arguments in (9.13) includes proofs that this composite admissibility implies \(\mathsf{Admissible}(C,a,b_1)\) and \(\mathsf{Admissible}(C,a+|b_1|,b_2)\), respectively. Mere prefix/suffix containment is insufficient for an instruction whose decoding or branch behavior depends on its surroundings. These implications supply the *same* \(C\) to both block proofs. The first returns an intermediate state \(s_1\) at \(a+|b_1|\), and the second starts there; concatenate their execution derivations and trace lists. The composed trace contract is witnessed by \(s_1,t_1,t_2\). Placement or relocation must preserve branch destinations and the decoder equations; when the second block has already been certified at its final address, \(\mathsf{RelocOK}\) can be the identity placement witness, not a second relocation operation. These premises must be constructed, not inferred from concatenating bytes. For loops and continuing processes, the normal-exit block is replaced by a guarded step/trace correspondence.

### Exact bytes for a small machine

For clarity, use a completely defined byte-oriented toy ISA with variable instruction lengths. Registers `r0`, `r1` are bytes, `pc` is a natural address, and arithmetic is modulo 256:

```text
Instruction          Bytes       Transition
MOVI r0, imm8        10 imm8     r0 := imm8; pc := pc+2
MOVI r1, imm8        11 imm8     r1 := imm8; pc := pc+2
XOR r0, r1           20          r0 := r0 xor r1; pc := pc+1
STORE r0, addr8      30 addr8    mem[addr8] := r0; pc := pc+2
HALT                 ff          halted := true; pc := pc+1
```

The decoder is a case split on the first byte and, for two-byte instructions, on the existence of the following byte; all other leading bytes trap. A complete compiler for the straight-line expression `Xor(Const(a),Const(b))` with destination address `d` is

```text
compileXor(a,b,d) = [10,a, 11,b, 20, 30,d, ff]
```

Its exact trace is

\[
\begin{array}{rcl}
(pc,r_0,r_1,\mu)&=&(0,\_,\_,\mu)\\
&\longrightarrow&(2,a,\_,\mu)\\
&\longrightarrow&(4,a,b,\mu)\\
&\longrightarrow&(5,a\oplus b,b,\mu)\\
&\longrightarrow&(7,a\oplus b,b,\mu[d\mapsto a\oplus b])\\
&\longrightarrow&(8,a\oplus b,b,\mu[d\mapsto a\oplus b],\mathsf{halted}).
\end{array}
\tag{9.14}
\]

Thus the compiler returns a byte string **and** a trace proof that the target memory at `d` contains the source expression's denotation. The same construction scales to a richer ISA by supplying its decoder, transition equations, and instruction-level block constructors; it does not follow that all source operations are single instructions. Functions, allocation, exceptions, threads, and system calls require runtime and world representations with corresponding semantic clauses.

### General compilation is a homomorphism of interpreted constructors

Let `Syn` be an inductively presented source language with constructors `cᵢ`, and let `⟦-⟧` be its interpretation. A compiler algebra supplies for each constructor `cᵢ` a machine block or continuing target process `Cᵢ` and a proof that its target interpretation is related to the source constructor applied to already compiled subterms. The fold of this algebra is the compiler:

\[
\begin{aligned}
\mathsf{compile}(c_i(t_1,\ldots,t_n))
 &=C_i(\mathsf{compile}(t_1),\ldots,\mathsf{compile}(t_n)),\\
\mathsf{correct}(c_i(t_1,\ldots,t_n))
 &=\mathsf{cong}_{R}(C_i,\mathsf{correct}(t_1),\ldots,\mathsf{correct}(t_n)).
\end{aligned}
\tag{9.15}
\]

Here `cong_R` denotes the constructor's *specified* relational congruence, not automatic congruence for an arbitrary refinement relation. Branches require matching branch conditions and successor states; loops require an invariant or a coinductive simulation; external calls require the chosen world relation. This is an exact structural recipe **conditional on supplying the algebra**. The universal fallback is a machine interpreter of the source's transition function, whose adequacy is established by matching each transition. Native specialization removes interpretive structure where the source and target semantics permit it. Neither a compiler algebra nor an adequacy theorem is manufactured by naming it.

For a constructor \(c_i\) of arity \(n\), the relational congruence used in (9.15) is a separately defined function

\[
\mathsf{cong}_{c_i}:\prod_{j=1}^{n}R_j(t_j,u_j)
\to R\bigl(\llbracket c_i(t_1,\ldots,t_n)\rrbracket,
\mathsf{run}(C_i(u_1,\ldots,u_n))\bigr).
\tag{9.16}
\]

The structural compiler proof is the induction on the source constructor using these functions. This explicitly locates the semantics of effects, branch choices, and runtime state in the chosen constructor algebra.

**Composition of source and target proofs.** Let \(\mathsf{SourceCorrect}(p,P)\) prove \(P(o)\) for every source observation \(o\) of \(p\), and let \(\mathsf{CompilerRefines}(p)\) map every observation of the loaded compiled artifact to an allowed source observation. Then

\[
\mathsf{ArtifactCorrect}(p,P)(o,h)
:=\mathsf{SourceCorrect}(p,P)
   (\mathsf{CompilerRefines}(p)(o,h)).
\tag{9.17}
\]

This is ordinary function composition of evidence. Its premises mention the actual source program, the compiled bytes, the loader, the target machine, and the selected observation. No further bridge is needed.

### Specializing the evaluator constructs direct compiler cases

Equations (9.15)–(9.16) describe a constructor-indexed compiler algebra *if* its blocks and congruences have been supplied. The evaluator provides a systematic source of candidates for these blocks. For a source constructor \(c_i\), specialize the represented \(\mathsf{next}\) with its outer code constructor static and its subterms, environment, and relevant world responses dynamic. The specialization equation (6.F1) gives a path relating the residual \(r_i\) to that evaluator case:

\[
\llbracket r_i\rrbracket(\vec t,\rho)
= \mathsf{next}(\mathsf{state}(c_i(\vec t),\rho)).
\tag{9.U3}
\]

When \(r_i\) lies in the machine-realizable fragment, its *own* block-correctness relation composes with the path (9.U3) and evaluator adequacy to yield a constructor-case machine relation. The separate simulation \(\nu\) is needed for segments actually executed through \(b_{\mathsf{next}}\), not as an extra premise for every directly compiled residual. When the residual still invokes the evaluator, the retained realization (9.U2) supplies that portion. Thus specialization can **derive the candidate implementations and their semantic connections**, instead of requiring unrelated instruction implementations for each source constructor.

A sufficient constructive route to the compiler algebra is to specialize the *guarded evaluator run* at each finite static code shape, retain dynamic environment/stack state, and compile the residual operations through the fragment \(F\) that realizes the evaluator. In particular, for each constructor \(c_i\), the candidate block \(C_i\) is obtained from the residual of its evaluator clause, and its meaning relation is supplied by (6.F1)/(9.U3) together with the block evidence of the chosen residual. This construction requires the bounded-static-variation and residual-composition conditions stated in §6.2. The specializer's equation alone does not establish them; if a residual still invokes the general evaluator, that invocation is natively realized through (9.U2) instead of making compilation fail.

The exact homomorphic fold (9.15) requires an additional property: the selected residuals must compose correctly with representations of compiled subterms and their state/continuation relations. Neither (6.F1) nor \(\nu\) alone proves that compositionality, nor does either guarantee that static dispatch has been eliminated. Hand-selected instructions are further specializations or alternative implementations supplied with their own relation to the same evaluator case. There is no mandatory interpreted/direct boundary in the language.

### Mixed realizations, effect history, and resources

Let \(R_{\mathrm{val}}(d,v)\) relate a direct value representation \(d\) to an evaluator value representation \(v\), with type indices and any required heap ownership or aliasing conditions retained. For direct target states \(D\) and evaluator configurations \(V\), a crossing is justified by a relation \(\mathcal X:D\to V\to\mathcal U\) preserving the represented continuation, live values through \(R_{\mathrm{val}}\), outstanding requests, and its observations. A direct-to-evaluator transfer establishes \(R_V\) for the encoded configuration; an evaluator-to-direct transfer, when used, establishes the receiving block's value and state premises. The existence of \(R_{\mathrm{val}}\) does not follow merely from both representations containing a word with the same bits. A direct segment ending at \(d\) may continue through the evaluator at \(v\) when \(\mathcal X(d,v)\) holds; returning to direct code requires a further relation only if that return is actually performed. A complete mixed artifact is a composition of direct block proofs, crossing relations, and finite or guarded iterations of \(\nu\). Its correctness is their composite evidence, not a claim that a runtime boundary introduces a second semantics.

A *static* residual call to the evaluator can remain where code or environment is not known at specialization time. A dynamically computed type family, closure, or face condition does not itself force such a call: it can be compiled directly whenever the corresponding machine relation has been constructed. A *dynamic* exit occurs when a direct block's own stated premise fails, for example an arithmetic fast path that assumes a result fits in a machine word. Its correctness requires either a proved continuation map from the current direct state to an evaluator configuration, or a justified restart of the computation.

The latter is **not** valid for an invocation that has already emitted world effects: restarting could duplicate a send, write, or payment. The relation at a resumable exit must preserve issued-effect history, outstanding command tokens, waiters, and the retained snapshot or a proved migration of it, as specified in §10.5. A direct call and an evaluator call may both issue commands through the same represented handler; world interaction is not inherently a crossing into the evaluator. A shared suspended computation similarly retains its producer identity and waiters across representation changes, rather than being recomputed independently by each consumer.

Finally, each block and crossing has a resource contract. It may require sufficient arena and stack capacity, or return an explicit resource-failure outcome whose relation to source observations is stated. Exhaustion is not an unmodeled permission to stop a supposedly total computation. The simulation \(\nu\) quantifies over all admitted evaluator configurations, not just finitely many test cases: its proof can use induction on evaluator code and invariants over heap, stack, loader, and world state. These are the same kinds of objects as the programs they describe.

### Loading, native substitution, and export

Executable bytes are interpreted together with format, entry point, relocations, linking, initial stack, environment, and OS state. An ELF parser alone does not implement dynamic linking. A native function's execution also depends on executable-memory setup and the instruction-cache synchronization required by the architecture. These are transitions of the loading world and part of the assumptions or checked code that establishes the entry condition.

Once the stated target-correctness witness is available, the evaluator may replace an applicable source computation with the native implementation under the theorem's preconditions. For pure closed input this is ordinary substitution of equals through the observation. For interaction it is substitution under the proved process congruence or refinement rule. A JIT performs this construction when additional inputs become known; its cache stores the resulting artifact with the evidence and dependencies that license its use.

A standalone export includes the selected runtime support and handlers or records their external contracts. C is the primary target presentation in this work. Its downstream compiler's relevant compilation relation is either an explicit dependency or supplied by a checked construction, as developed in §9.4. Correctness of a printer is a statement about the printed text's parsed meaning; a proof about its input tree does not make printing infallible.

The same applies to GPU kernels and other accelerators. Their instruction sets, memory spaces, barriers, and launch behavior are target semantics. Tiling and parallelization preserve a stated observation under the selected memory and arithmetic models. There is no special exemption from the artifact connection because the target is parallel.



## 9.4 Compilation through C

C is the primary target presentation. A represented program, or a specialization of its evaluator at known inputs, is lowered to C together with its relation to the source interpretation. An existing C compiler then supplies the target instruction selection, object representation, and executable construction. The language, kernel, and network do not require independent direct emitters for every processor architecture.

The mathematical operation is composition of the relations already used in (9.17). Let \(p\) be the represented source program, \(c\) its generated C artifact, \(K\) the selected C compiler and configuration, and \(b=K(c)\) its target artifact. Fix a shared observation type \(O\), related initial states and world handlers, and the domain on which the C program has defined behavior. The two required maps have the form

\[
\begin{aligned}
\lambda_p &: \mathsf{Beh}_{C}(c,o)\to\mathsf{Beh}_{L}(p,o),\\
\kappa_c &: \mathsf{Beh}_{M}(K(c),o)\to\mathsf{Beh}_{C}(c,o),\\
(\lambda_p\circ\kappa_c)(h)&:
 \mathsf{Beh}_{L}(p,o)
 \quad\text{for }h:\mathsf{Beh}_{M}(K(c),o).
\end{aligned}
\tag{9.C1}
\]

The source-to-C construction supplies \(\lambda_p\), including the C-definedness premises; the downstream compilation relation supplies \(\kappa_c\). If the observation distinguishes termination, divergence, resource exhaustion, or external effects, those distinctions belong to both relations. A value-only equation must not be substituted for that stronger statement. The same composition applies to the C realization of the evaluator in §9.2 and to directly lowered residuals. It therefore covers the whole represented language without demanding a processor instruction for each of its constructors.

The generated artifact includes declarations, runtime support, and the external contracts actually used. Lowering a checked syntax tree to C text also requires its printer/reader correspondence: the compiled text must denote the C construction for which the relation was established. The definedness obligations include the selected integer and floating-point behavior, valid object lifetimes and memory accesses, aliasing, and the actual concurrency model. They are properties of the generated program, not consequences of the target merely being called C.

### A concrete C target

The frame decoder of §8.6 admits the following C presentation. It uses exact-width integer types on implementations providing them, with a readable input array, no concurrent mutation of that input, and a separate writable live output object as its precondition. `size_t` is the implementation's object-size type; no particular register assignment, pointer width, or structure offset is prescribed here.

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint64_t tag, length;
    const uint8_t *data;
} Frame;

bool decode_frame(const uint8_t *b, size_t n, Frame *out)
{
    if (n < 4) return false;
    uint64_t length = (uint64_t)b[2]
                    | ((uint64_t)b[3] << 8);
    if (length > n - 4) return false;
    out->tag = (uint64_t)b[0] | ((uint64_t)b[1] << 8);
    out->length = length;
    out->data = b + 4;
    return true;
}
```

The initial guard precedes every input read. Each shifted value is at most \(255\cdot256\), each completed field at most \(65535\), and the explicit unsigned-width conversions preserve those values. After the guard, \(n-4\) is defined; the second guard implies that the complete payload lies within the input. Failure leaves the output fields unchanged. Success writes exactly the tag, length, and borrowed payload view. These branch facts give the same source-level result as the interpreted decoder in §8.6, with output observations taken through the C fields rather than invented byte offsets.

The downstream compiler selects the machine loads, stores, branches, calling convention, and padding for its target. The proof of the source-level property is therefore reused through (9.C1); it is not rewritten as a new decoder proof for each instruction set. A native artifact still has a concrete machine meaning, but the route to it passes through an already existing compiler.

### Reusing and later verifying the compiler

The relevant compilation relation of the chosen toolchain is explicit. It may initially be a stated assumption in an artifact's trusted dependencies. Lifting that compiler into the language makes its syntax, behavior, and transformations available to the same analysis, so a proof of its compilation relation replaces that assumption where established. This is the selected verification direction, rather than making bespoke x86, ARM, and other backend proofs prerequisites to using the system. Verified compilation has an established precedent in [18](#ref-18); no particular external compiler or configuration is silently selected or declared verified by this construction.

For a required property \(Q:O\to\mathcal U\) and source proof \(q:\prod_{o:O}\mathsf{Beh}_L(p,o)\to Q(o)\), the target proof is the actual composite

\[
\mathsf{artifactQ}(o,h):=q\bigl(o,\lambda_p(\kappa_c(h))\bigr).
\tag{9.C2}
\]

Changing the C compiler, target configuration, runtime, or a used primitive changes the relevant dependency of this result. It does not change the meaning of the represented source program. The source-level identities used during specialization, the C-lowering relation, and the downstream compilation relation remain distinct constructions that compose, rather than competing accounts of execution.

For represented languages \(L\) and \(M\), fix the source program \(p\), a common observation type \(O\), and an environment interpretation \(E\) supplying the related initial states and world handlers used by their execution semantics. Write \(\mathsf{obs}_L\) and \(\mathsf{obs}_M\) for the corresponding observations into \(O\). The complete result can retain the executable artifact and the source run related to each target run:

\[
\sum_{a:\mathsf{Artifact}_M}
\prod_{\rho:\mathsf{Run}_M(a,E)}
\sum_{\tau:\mathsf{Run}_L(p,E)}
\bigl(\mathsf{obs}_M(\rho)=_O\mathsf{obs}_L(\tau)\bigr).
\]

If a source property \(Q:O\to\mathcal U\) holds of every source execution, apply it to the retained source run \(\tau\) and transport the resulting evidence along the inverse of the displayed observed equality to obtain \(Q(\mathsf{obs}_M(\rho))\). This is the general property-transfer operation; a particular compiler supplies an inhabitant for the source, target, and environment semantics it actually handles. Equality between specialized source terms and the relation between source runs and executed machine bytes remain distinct, composable results.

This is the same source-to-target contract whether the middle representation is C or a direct byte block. Appendix C retains a concrete direct-x86 instance for comparison; it is not a requirement to implement that route. C is the selected realization path, and the general relation explains why reusing its downstream compiler preserves the work already established in the source.

# 10. Persistent, interactive, and distributed programs

A computation that changes its descriptions must also determine which earlier conclusions remain applicable. The constructions of §§2–9 already supply identities, dependency-sensitive properties, executable transformations, and continuing interactions. Persistence and exchange extend these same relations across time and participants rather than introduce a second meaning for them. The activity of computing includes retaining what has been established, acquiring new observations, changing interpretations, and making constructions available elsewhere. These are not additional services surrounding a completed program. They are transitions and relations within the continuing process, and the programs that implement them are themselves part of what is being described. Process calculi, causal ordering, temporal specification, refinement, incremental computation, and bidirectional views provide established results for particular constructions ([21](#ref-21), [22](#ref-22), [23](#ref-23), [24](#ref-24), [25](#ref-25), [26](#ref-26), [38](#ref-38)). The following terms compose into the actual distributed program, rather than an external model used only to verify it.

A closed pure expression is not the only computational object. A process may request input, await a message, mutate a state, emit an event, or continue indefinitely.

One presentation is an inductive/coinductive interaction tree, indexed by a signature \(\Sigma\) of requests and their response types:

\[
\mathsf{Process}_{\Sigma}(A)
\ni
\mathsf{return}(a)
\;\mid\;
\mathsf{request}(q,k)
\;\mid\;
\mathsf{delay}(p),
\tag{10.1}
\]

where \(k\) continues the computation with the response to \(q\). A handler supplies an interpretation of requests in a particular world. Different handlers yield different observations of the same process description. A network is not intrinsically an opaque external service; it can be represented by communicating process states and transition relations, including packet loss, ordering, failure, and partial knowledge.

Persistence introduces identity over time. A definition may remain stable while names, inputs, or observed dependencies change. For a pure computation \(q\) and data \(d\), a maintained result can be packaged with its correctness witness:

\[
\mathsf{Maintained}(q,d)
:=
\sum_{r:R}\bigl(r=q(d)\bigr).
\tag{10.2}
\]

A valid incremental update must transform a maintained result for \(d\) into one for \(d'\), including evidence that the updated result still denotes \(q(d')\). The statement is independent of whether the representation happens to be a database table, a spreadsheet cell, a UI widget, or a compiled cache. Codd’s relational model supplies an established foundation for one of those representations ([60](#ref-60)).

A cached evaluation may record every lookup on which it depends, including an observation that a key was absent. If the same observations remain valid, a deterministic pure evaluation segment can be reused. If a computation is suspended on an external request, its continuation and the request must remain distinguishable from a completed pure value. Failures, waiter delivery, and invalidation are transitions of the represented state, not reasons to invent a separate foundational category called a task queue.

The same treatment applies to user interfaces. A view is an interpretation of state; an edit is an operation relating the displayed representation and the underlying state. Bidirectional view laws express what round trips preserve and what information is lost. A view can be a diagram, a text editor, a table, a proof visualization, or an immersive spatial interface. Harel’s statecharts supply an established formal example of visual representations of stateful behavior ([66](#ref-66)). No one of these is privileged.

A distributed system specifies local states, messages, causal relations, and partial observations; a GPU specifies parallel execution, memory, and numerical behavior; an operating system specifies scheduling, isolation, and devices. These are distinct constructions, not metaphors or primitive divisions of computation. Their composition determines which information and interactions become available to subsequent operations.


**Handler composition and safety transfer.** A command handler interprets each request in a world transition relation. Let \(h_1,h_2\) be two handlers, and \(R:W_1\to W_2\to\mathcal U\) a simulation relation. If for every related pair \(R(w_1,w_2)\), each \(h_2\)-transition with response \(r\) has a matching \(h_1\)-transition with the same selected observation and related successor worlds, then the relation lifts coinductively to the processes interpreted by the handlers. The proof proceeds by cases on `return`, internal delay, and `request`; the request case uses the supplied step simulation and the induction/coinduction hypothesis on the continuation.

For a safety property \(P\) invariant under the chosen observation, safety of the source process transfers along this simulation to the target process. For a progress statement, the premises additionally include the fairness condition actually used by the chosen scheduling semantics. Safety is an induction over reachable states; progress is a statement about continuing executions and cannot be derived from the safety invariant alone. Temporal logic provides a classical account of these different kinds of properties ([58](#ref-58)).

## 10.1 Exact reuse requires recorded information

A dependency certificate is not merely a set of keys. It must record what was observed at each key, including absence. Let \(S\) be a store and let \(D\) be a list of observations \((k,o)\). Then

\[
\mathsf{valid}(S,D):=
\prod_{(k,o)\in D}(\mathsf{lookup}(S,k)=o).
\tag{10.3}
\]

For a deterministic pure evaluation segment whose complete read trace is \(D\), validity of \(D\) in a new store implies the same sequence of internal reductions until the next unrecorded lookup or world request. The proof is induction on the evaluation steps: pure reductions are determined by the current term and environment, and recorded lookups return equal observations.

A suspended computation needs additional state: its residual expression, dependency observations, pending request, and the continuations waiting on its result. A failure is not a successful result. A response from the outside world cannot be silently replayed as though it were a pure lookup. These are exact distinctions about computation and interaction, not arbitrary divisions into caching, queueing, and workflow products.

## 10.2 A maintained view is a proof about an update

Let \(q:D\to R\) be a query and let \(\delta:D\to D\) be an update. A maintained view update \(\Delta_q:R\to D\to R\) is correct when

\[
\forall d.\quad
\Delta_q(q(d),d)=q(\delta(d)).
\tag{10.4}
\]

If the update depends on additional edit data \(e\), include it in \(\delta_e\) and \(\Delta_q(-,-,e)\). The exact theorem is a property of the functions. A database's materialized view, a spreadsheet dependency, and a rendered graph visualization can all instantiate it.

For bidirectional editing, a view \(\mathsf{get}:S\to V\) and update \(\mathsf{put}:S\to V\to S\) may satisfy

\[
\begin{aligned}
\mathsf{get}(\mathsf{put}(s,v))&=v,\\
\mathsf{put}(s,\mathsf{get}(s))&=s.
\end{aligned}
\tag{10.5}
\]
These lens laws characterize the specified total view/update operations. Lossy views and partial edits require their corresponding weaker or differently typed laws.

For a proof-carrying result, write \(\mathsf{Mat}(q,d):=\sum_{r:R}(r=q(d))\). It retains the computed value and its equality to the query at the same input version.

**An actual incremental edit.** Let \(v:\mathsf{Vec}(U_{64},n)\), \(q(v)=\mathsf{foldl}(+_{64},0,v)\), and \(u_{i,x}(v)=v[i:=x]\). For \(i:\operatorname{Fin}(n)\), \(x:U_{64}\), and a maintained pair \((r,e)\) with \(e:r=q(v)\), define

\[
\begin{aligned}
\Delta_{i,x}(v,r)&:=r-_{64}v_i+_{64}x,\\
\lambda_{i,x,v}&:\Delta_{i,x}(v,q(v))=q(u_{i,x}(v)),\\
\mathsf{maint}_{i,x}(v,(r,e))&:=
\bigl(\Delta_{i,x}(v,r),\ \mathsf{ap}(\Delta_{i,x}(v,-),e)\cdot\lambda_{i,x,v}\bigr).
\end{aligned}
\]

To construct \(\lambda\), split the fold into the prefix before \(i\), the element \(v_i\), and the suffix after \(i\). Associativity and commutativity of modular addition and additive inverses cancel \(v_i\) and insert \(x\). The result is an inhabitant of \(\mathsf{Mat}(q,u_{i,x}(v))\), computed from one old element and two modular operations rather than by refolding the vector. Publishing the edited vector under a new immutable identity selects this maintained result for a subscribed sum view. If a subsequent transformation gives \(\theta:q=q'\), the same \(r'\) becomes a maintained result for \(q'\) through \(e'\cdot\mathsf{ap}(\lambda f.f(v'),\theta)\). A local compiled artifact or a remote evaluator can consume the resulting definition and value at the same type. Device reads that supply later edits remain separate world interactions; reuse of the pure sum does not repeat them.



### A source edit changes the proof, executable bytes, and view

The closed byte example of §9.3 also has a complete source-edit lifecycle. Let
\[
 p_0=\mathsf{Xor}(\mathsf{Xor}(\mathsf{Input},\mathsf{Const}(0x5a)),
                         \mathsf{Const}(0x5a)).
\]
Its meaning is \(E(p_0,x)=(x\oplus 0x5a)\oplus 0x5a=x\), with proof by the involutive XOR law, and its emitted bytes are `01 02 5a 03 02 5a 03 ff` under the §9.3 stack-machine decoder. Now change only the final constant to `0x5b`:

\[
\begin{aligned}
p_1&=\mathsf{Xor}(\mathsf{Xor}(\mathsf{Input},\mathsf{Const}(0x5a)),\mathsf{Const}(0x5b)),\\
E(p_1,x)&=(x\oplus0x5a)\oplus0x5b=x\oplus0x01.
\end{aligned}
\]

The corresponding unsimplified compiled bytes become `01 02 5a 03 02 5b 03 ff`. The previous proof of \(E(p_0,x)=x\) is not a proof of \(E(p_1,x)=x\); for example, at \(x=0\) the new output is \(1\). Its dependencies, any simplified residual, and the rendered output must therefore refer to the new program version. The definitions of the byte type, XOR, interpreter, and compiler are unchanged. This is distinct from the vector-update proof above and from the packet-decoder/live-upgrade example below: each exhibits a different actual dependency path through the same language.


## 10.3 A distributed credit invariant, with no global scheduler as an axiom

Let a finite set of peers be \(N\). For each \(i\in N\), let \(c_i\in\mathbb N\) be locally held credit; for each directed channel \(i\to j\), let \(m_{ij}\in\mathbb N\) be credit carried by in-flight messages. A global *specified state* is useful for specifying an invariant even when no participant can observe the entire state:

\[
S:=\left(\prod_{i:N}\mathbb N\right)
\times\left(\prod_{i,j:N}\mathbb N\right),
\qquad
\mathsf{total}(S):=
\sum_i c_i+\sum_{i,j}m_{ij}.
\tag{10.6}
\]

An allocation from \(i\) to \(j\) of \(q\le c_i\) transforms

\[
(c_i,m_{ij})\mapsto(c_i-q,m_{ij}+q).
\tag{10.7}
\]

Delivery transforms

\[
(m_{ij},c_j)\mapsto(m_{ij}-q,c_j+q)
\quad\text{when }q\le m_{ij}.
\tag{10.8}
\]

Both transitions preserve \(\mathsf{total}\), by cancellation in \(\mathbb N\). Hence any finite sequence of these transitions preserves total credit, by induction on its length. The proof is elementary, but it is exact and independent of a centralized execution algorithm.

A real protocol must also define message identities, duplication resistance, receipt acknowledgment, failure, reallocation, and the conditions under which credit can be returned. If messages may be duplicated without an idempotency rule, the conservation theorem fails: one in-flight allocation can be delivered twice. The required distinction is between *the stated invariant* and *the actual protocol transition rules*, not between mathematics and networking.

For an observer \(i\), define \(\mathsf{Obs}_i:S\to O_i\). The observer's compatible states form the fiber

\[
\mathsf{Possible}_i(o):=
\sum_{s:S}\bigl(\mathsf{Obs}_i(s)=o\bigr).
\tag{10.9}
\]

Local knowledge is therefore a construction over partial observation. A peer can prove an action safe for every state compatible with its observations without possessing the global state. This supplies a precise setting for local invariants, distributed verification, and computation through message exchange. More elaborate protocols can express obligations, receipts, debt transfer, or cancellation in the same language, provided their transitions are defined.


**Receipt-bearing delivery.** Enrich the in-flight credit message with a unique transfer identifier \(u\), source, destination, and amount. A state records sets \(\mathsf{sent}\), \(\mathsf{delivered}\), and \(\mathsf{acknowledged}\). Delivery of \(u\) requires \(u\notin\mathsf{delivered}\), consumes its in-flight credit exactly once, credits the receiver, and inserts \(u\) into \(\mathsf{delivered}\). Acknowledgment requires \(u\in\mathsf{delivered}\) and records a receipt; it transfers no credit. A return is a new uniquely identified transfer in the reverse direction, subject to available credit. Each transfer preserves the conservation invariant, and duplicate delivery is excluded by the delivered-set premise. Failures leave in-flight credit represented until an explicitly defined recovery or cancellation transition acts on it.

## 10.4 Observations, information, and locally available actions

Section 1 introduced observation fibers for histories and established what a dependent rationale over a fiber means. Here the same construction is applied to *available actions* in a running system: a participant must establish a property for every state compatible with what it has actually observed. For a state type \(S\), an observation is a function \(o:S\to O\), which may itself be computed by an interacting process. The fiber

\[
\mathsf{Fiber}_o(y):=\sum_{s:S}(o(s)=y)
\tag{10.10}
\]

is the space of states compatible with observation \(y\). If an action is permitted only when a predicate \(P:S\to\mathcal U\) holds, an observer who sees \(y\) can establish the action safe without guessing the hidden state by constructing

\[
\mathsf{SafeAt}(y):=
\prod_{(s,p):\mathsf{Fiber}_o(y)}P(s).
\tag{10.11}
\]

This is stronger than exhibiting one compatible state in which the action is safe. A refinement of observations \(o':S\to O'\), with \(o=h\circ o'\), makes the compatible-state fiber no larger; safety evidence for a coarser observation can be reused under the refined one by precomposition. The converse is not generally possible.

This gives a precise account of information as constraints on possible states and actions. Given a value function on admissible trajectories, one can compare the optimum over trajectories compatible with one observation to the optimum over a refined observation. Monotonicity follows when refinement expands the available *contingent policies* and the objective and feasibility assumptions are held fixed; it does not follow from arbitrary differences of observations alone.

The same construction governs distributed systems, debugging, interfaces, and inference. The fiber records states compatible with an observation; a dependent proof over it establishes what follows for every compatible state. The observer can itself be part of the process. Combining observations constrains a joint fiber; a participant can use another's observation only when its interaction supplies the relevant information. No global knower or separate theory of “knowledge” is required. The observer's own state, including the interpretation through which it records an event, can be included among the states being observed. This is how the same account reaches the evolution of communication and language use. An utterance is an event in a causal history; a recipient's response depends on its retained context and interpretation; the response changes the context for later utterances. A population's evolving conventions arise from these interacting histories. One need not posit a fixed meaning intrinsic to the utterance or a separate collective mind to describe what each participant can infer and how information becomes available between them. A scientific instrument and the process that interprets its measurements are treated the same way: they contribute their specified transitions and observations to the subject of reasoning.



**Updating possible worlds after a response.** Let \(K:W\to\mathcal U\) describe worlds compatible with an observer's information. For a command \(c\), response \(r\), and world-transition relation \(H_c:W\to R_c\to W\to\mathcal U\), the updated knowledge is the dependent image

\[
K'(w'):=\left\|\sum_{w:W}K(w)\times H_c(w,r,w')\right\|.
\tag{10.12}
\]

If a proposed action requires \(P:W\to\mathcal U\), its safety evidence is \(\prod_{w'}K'(w')\to P(w')\). This is exactly the same dependent reasoning used for source semantics and machine-state invariants; only the chosen world relation changes.

## 10.5 Dependency certificates and persistent evaluation

A persistent evaluator retains a result together with the facts read during its computation. Let `Key` be a type of definition identities, `Version` a type of revisions, and `Store : Key → Option (Version × Value)` a finite partial map. A dependency is a pair `(key, observedVersion)`; a negative lookup is a dependency on the *absence* of a key at a given store revision.

\[
\begin{aligned}
\mathsf{Dep}&:=\mathsf{Present}(k,v)+\mathsf{Absent}(k,e),\\
\mathsf{valid}(S,d)&:=
\begin{cases}
\mathsf{version}(S[k])=v&d=\mathsf{Present}(k,v),\\
S[k]=\mathsf{none}\ \wedge\ \mathsf{epoch}(S)=e&d=\mathsf{Absent}(k,e),
\end{cases}\\
\mathsf{validAll}(S,D)&:=\prod_{d\in D}\mathsf{valid}(S,d).
\end{aligned}
\tag{10.13}
\]

Equation (10.13) is a *version-based sufficient condition* for observational validity (10.17), not the same predicate by definition. The certificate carries the invariant \(\mathsf{version}(S[k])=\mathsf{version}(S'[k])\Rightarrow\mathsf{Lookup}(S,k)=\mathsf{Lookup}(S',k)\) for present entries; for absent entries it carries \(\mathsf{epoch}(S)=\mathsf{epoch}(S')\Rightarrow\mathsf{Lookup}(S,k)=\mathsf{Lookup}(S',k)=\mathsf{none}\). Thus a version certificate yields the corresponding equality of observed lookup results. An epoch for negative lookups may be coarse; a finer index-version certificate is equally valid if it proves that the lookup result is unchanged. The important property is *soundness*, not minimal invalidation.

The full producer transition relation, including suspension, failure, and waiter delivery, is defined below using the same validity predicate.

An incremental update is itself a program with a proof. For `f : A → B`, edits `δA`, `applyA : A → δA → A`, and `applyB : B → δB → B`, a maintained derivative has type

\[
\mathsf{derive}(f):
\prod_{a:A}\prod_{d:\delta A}
\sum_{e:\delta B}
\mathsf{Path}_B\bigl(\mathsf{applyB}(f(a),e),
                       f(\mathsf{applyA}(a,d))\bigr).
\tag{10.14}
\]

For a sum over a finite map with values in an abelian group \(G\), `f(m)=Σ_{k∈dom(m)}m[k]`, replacing one entry `m[k]=x` by `y` has derivative `e=y−x` in \(G\); the path is the additive-group identity

\[
\sum m+(y-x)=\sum(m[k\mapsto y]).
\tag{10.15}
\]

This illustrates the distinction between a *generic* correct fallback (recompute `f` and return the difference if the target edit algebra supports it) and a *specialized* incremental program (update in constant work). The specification of maintained computation includes both the derivative and its evidence; no external incremental-computation oracle is assumed.


### Example: a dependency-tracked evaluator

As an example of programming over persistent computations, define an evaluator whose next operation is an internal reduction, a store lookup, a world request, a return, or a fault. Its state is a tagged sum retaining a suspended computation, its waiters, and its dependencies:

\[
\begin{aligned}
\mathsf{Outcome}(A)&:=\mathsf{Value}(A)+\mathsf{Error}(\mathsf{Err}),\\
\mathsf{Read}&:=\sum_{k:\mathsf{Key}}\mathsf{LookupResult}(k),\\
\mathsf{Certificate}&:=\mathsf{List}(\mathsf{Read}),\\
\mathsf{EffectScope}&:=\mathsf{InvocationId}\times\mathsf{WorldEpoch},\\
\mathsf{RequestToken}&:=\mathsf{InvocationId}\times\mathsf{StepId},\\
\mathsf{Producer}(A)&:=
   \mathsf{Absent}\\
 &\quad+\mathsf{Running}(\mathsf{Residual}(A),\mathsf{Snapshot},
                         \mathsf{Certificate},\mathsf{Waiters})\\
 &\quad+\mathsf{Suspended}(\mathsf{Residual}(A),\mathsf{Snapshot},
                           \mathsf{Certificate},\mathsf{Waiters},
                           \mathsf{Request})\\
 &\quad+\mathsf{Ready}(A,\mathsf{Certificate},\mathsf{EffectScope})\\
 &\quad+\mathsf{Failed}(\mathsf{Err},\mathsf{Certificate},\mathsf{EffectScope}).
\end{aligned}
\tag{10.16}
\]

An observation is a sum of present values (with the required version or identity evidence) and absence. A recorded lookup includes the observed value **or absence**; an implementation may record a version/epoch in place of a value only if that version soundly identifies the observed lookup result. The validation predicate is

\[
\mathsf{valid}(S',D)
  :=\prod_{(k,o)\in D}\bigl(\mathsf{Lookup}(S',k)\equiv o\bigr).
\tag{10.17}
\]

In the transition schemata below, the snapshot component is held implicit when unchanged, and `Running(r,D,W)` abbreviates `Running(r,S_0,D,W)` for the producer's retained snapshot `S_0` (similarly for `Suspended`). A lookup step reads from `S_0` and appends its actual result to `D`. A producer resumed under that same snapshot does not require validation against an unrelated latest store; only reuse under a *different* snapshot requires the recorded certificate to validate there. An already-effectful invocation retains `S_0` unless a history-preserving migration is supplied. For each semantic producer key \(k\) and effect scope \(\omega\), the table stores the state above. A pure computation may use a shared scope \(\omega_0\). An effectful computation receives a fresh invocation identity; two consumers may share its result only if they explicitly refer to the **same effect occurrence** \(\omega\). Define
\[
\mathsf{validReuse}(S,D,\omega,\omega')
 :=\mathsf{valid}(S,D)\times(\omega=\omega').
\tag{10.18}
\]
The table lookup for running or suspended states is **already indexed by** \(k,\omega\), so these cases need no second scope comparison. The additional \(S=S_0\) guard is a snapshot condition, not an effect-scope condition; the state abbreviations retain \(S_0\) as described above. For an absent entry, the newly created producer's retained snapshot is \(S\). The store-certificate test for a **completed** result is the guard for reuse under the displayed store \(S\). For a **running or suspended** producer, agreement on reads already made is insufficient to authorize a consumer requiring a different snapshot: later reads may differ. The ordinary join rule therefore requires the consumer's snapshot to be the producer's retained \(S_0\). A different-snapshot join requires a separate proved simulation or a transition to a new pure producer; it cannot be inferred merely from \(\mathsf{valid}(S,D)\). Nor can a suspended effectful invocation restart against \(S\). The completed-state clauses additionally compare their stored scope. A new **effect occurrence** obtains a new scope even when its source expression is identical; an ordinary pure store lookup does not thereby create a world-effect occurrence. A transition has one of these exact forms:

\[
\begin{array}{rcl}
\mathsf{demand}(k,\omega,S,w)&:&
 \mathsf{Ready}(v,D,\omega')\land\mathsf{validReuse}(S,D,\omega,\omega')
 \to\mathsf{deliver}(w,v),\\
&&\mathsf{Failed}(e,D,\omega')\land\mathsf{validReuse}(S,D,\omega,\omega')
 \to\mathsf{deliverError}(w,e),\\
&&\mathsf{Running}(r,D,W)\land S=S_0
 \to\mathsf{Running}(r,D,W{+\!+}[w]),\\
&&\mathsf{Suspended}(r,D,W,q)\land S=S_0
 \to\mathsf{Suspended}(r,D,W{+\!+}[w],q),\\
&&\mathsf{Absent}\to\mathsf{Running}(\mathsf{initial}(k),[],[w]);\\
\mathsf{step}(k,\omega)&:&
 \mathsf{Running}(r,D,W)\xrightarrow{\mathsf{lookup}(q)=o}
 \mathsf{Running}(r',D{+\!+}[(q,o)],W),\\
&&\mathsf{Running}(r,D,W)\xrightarrow{\mathsf{request}(q,\iota)}
 \mathsf{Suspended}(r',D,W,(q,\iota)),\\
&&\mathsf{Suspended}(r,D,W,(q,\iota))
 \xrightarrow{\mathsf{response}(q,\iota,a)}
 \mathsf{Running}(\mathsf{continue}(r,a),D,W),\\
&&\mathsf{Running}(r,D,W)\xrightarrow{\mathsf{return}(v)}
 \mathsf{Ready}(v,D,\omega)\ ;\ \mathsf{deliverAll}(W,v),\\
&&\mathsf{Running}(r,D,W)\xrightarrow{\mathsf{fault}(e)}
 \mathsf{Failed}(e,D,\omega)\ ;\ \mathsf{deliverAllError}(W,e).
\end{array}
\tag{10.19}
\]

The semicolon in the return/fault cases denotes one state update together with waiter notification. The producer table is indexed by the semantic key and effect scope \( (k,\omega) \); an external request has a fresh per-occurrence token \(\iota\) within that scope, and a response is accepted only for the corresponding outstanding token. A consumed or retired token cannot resume a continuation twice. Source-syntax equality does not authorize reuse of an effect occurrence.

**Snapshot changes and effect history.** An invocation evaluates against its retained immutable snapshot. A changed lookup certificate may invalidate reuse of *pure work* under a later snapshot, but does not retroactively change the meaning of interactions already issued. If an invocation has issued no world request, it may restart from \(\mathsf{initial}(k)\) under the new snapshot, retaining every waiter; a proved incremental update is another permitted transition. Once an invocation has issued a world request, it continues under its original snapshot or moves under a supplied simulation that preserves its issued effects, outstanding tokens, and pending obligations. Restarting it from the initial residual requires that same effect-history preservation proof. Merely retiring a token does not undo a payment, send, or write.

A consumer requiring the new snapshot, when neither reuse nor a preserving migration is available, receives a typed invalidation result or remains waiting for the specified migration; it does **not** cause the previous effect to be reissued. Delivering invalidation notifies every waiter while leaving the outstanding interaction represented until completion, acknowledged cancellation, or a defined recovery transition. Stale *pure* ready or failed entries can be recomputed. Effectful results remain records of their original occurrences, reusable only with the corresponding scope and certificate. Thus the safe fallback for an effectful invocation is not the unconditional stale-state restart of a pure producer.

### Re-specializing an active invocation

A JIT may specialize a continuing process further when new static facts become available. A new artifact \(a_1\) may be installed beside an old artifact \(a_0\), but replacing an *active* occurrence is a migration of its current continuation, not a restart of its original source program. For a source continuation \(k\), let \(\mathsf{Boundary}_i(k)\) contain target states of artifact \(a_i\) at which the correspondence with \(k\) is known (for example a proved turn boundary or a suspended command-response boundary). Let \(\mathsf{LiveRel}_i(d,k,H,T,S)\) state that target state \(d\) realizes \(k\) with already-issued effect history \(H\), outstanding token set \(T\), and retained snapshot \(S\). Given a dependency certificate \(D_1\), the swap evidence is a map

\[
\begin{aligned}
\mathsf{Swap}_{D_1}(a_0,a_1):={}&
\prod_{k,H,T,S}\prod_{d_0:\mathsf{Boundary}_0(k)}
\mathsf{LiveRel}_0(d_0,k,H,T,S)\\
&\to\mathsf{valid}(S,D_1)
\to\sum_{d_1:\mathsf{Boundary}_1(k)}
\mathsf{LiveRel}_1(d_1,k,H,T,S).
\end{aligned}
\tag{10.S1}
\]

The displayed relation deliberately retains \(H,T,S\); an actual migration may change the state representation or snapshot only with a *separate* simulation relating the old and new observations and preserving the issued effects. A response to an outstanding request must be delivered to the new continuation under the **same request token**, and cannot trigger a second issue of that request. A state at an arbitrary machine instruction is not a safe swap point unless its source-continuation relation has been constructed. Turn and response boundaries are useful examples, not universal privileged cut points.

If the specialization certificate later becomes invalid, an active effectful artifact must use a proved *resume* exit to a less specialized or evaluator-mediated realization. Every guard whose failure is reachable during that artifact's execution must either have such an exit or be proved invariant under the permitted environment changes. Pure work may restart when the selected observation makes the discarded work unobservable; already-issued effects cannot be replayed merely to reconstruct a convenient initial configuration. This is the same effect-history theorem used above for persistent producer states, now applied to a program that changes its own executable representation.

**Read-dependency lemma.** For a deterministic pure evaluation segment, if its complete lookup trace is \(D\), any store \(S'\) satisfying \(\mathsf{valid}(S',D)\) produces the same segment until its next new lookup or world request. Proof: induct on the evaluation steps; each store-read step sees the same value by (10.17), and every other pure step is determined by its current term and environment. This lemma proves sound reuse, not a bound on recomputation.

The certificate \(D\) selects a fiber of stores agreeing on the recorded observations (§10.4). Under the lemma's conditions, the recorded reductions remain valid throughout that fiber; the evaluator need not reconstruct them. A certificate describes the applicability of retained work, not merely its location in storage.

These equations define this particular dependency-tracked evaluator. Other evaluation strategies can use other state representations.

### Producer requests, suspension, and waiter delivery

The canonical producer state and transitions are (10.16)–(10.19). A request for an already running or suspended producer appends the consumer to its retained waiters. Completed pure work may be reused under a valid read certificate. A snapshot change permits restarting pure work or applying a proved derivative; effectful invocations obey the effect-history and migration conditions above. An external request resumes only upon its matching world response.

The maintained-result update is derived from (10.2): if \(e:r=q(d)\) and \(\lambda_d:v(d,q(d))=q(u(d))\), then

\[
\mathsf{maint}(d,(r,e))
:=\bigl(v(d,r),\mathsf{ap}(v(d,-),e)\cdot\lambda_d\bigr)
:\mathsf{Mat}(q,u(d)).
\tag{10.20}
\]

If no specialized update is available, recompute \(q(u(d))\) and return its reflexivity witness. The same dependent type certifies both branches.

For a typed editing context \(K[-]\), replacing a selected subterm is the composition

\[
\begin{aligned}
\mathsf{inspect}(K[t])&=(\Gamma_h,A_h,t,K),\\
\mathsf{check}(\Gamma_h,A_h,t')&\leadsto d':\mathsf{Der}(\Gamma_h\vdash t':A_h),\\
\mathsf{plug}(K,(t',d'))&:\mathsf{Raw},\\
\mathsf{checkEnclosing}(\mathsf{plug}(K,(t',d')))&
\leadsto \mathsf{Code}(\Gamma,A).
\end{aligned}
\]

The new code is a new immutable definition. Rebinding a name publishes it in a new snapshot; the read-validation equation determines the retained computations that can be reused. The editor, renderer, query, and proof are themselves definitions in the same space.



## 10.6 Views, distribution, and complete systems


### Queries and updates

A query over persistent data is a program `q : D → R`. Its expressive interface is the language itself: dependent functions, recursion, reflection, chosen observations, and proofs. A specialized query engine, relational algebra, graph traversal, or numerical kernel is a representation and compilation of that program.

For a materialized view, the maintained state is `(d,r)` with `r=q(d)`. An update `u : D → D` and incremental view update `v : D → R → R` are connected by

\[
\prod_{d:D}\mathsf{Path}\,R\,(v(d,q(d)))\,(q(u(d))).
\tag{10.21}
\]

Thus updating the materialization preserves its defining equation. Indexes, caches, and incremental queries are useful because they realize this equation with different work and storage costs. There is no separate foundational “database correctness” object.

Transactions are processes over a storage world whose admitted executions state the chosen isolation and durability guarantees. Serializability is a relation to a serial execution; crash recovery relates persisted states to the permitted logical states. A data model represented by a quotient must make its queries respect that quotient. A query that intentionally examines a concrete encoding instead operates before that abstraction or retains the relevant representation information.

This is the precise code/data consequence. Data is not forced into a language-external schema, and code is not excluded from the data being queried. The types and operations needed to describe a program are themselves available to programs. Higher paths can express relations between presentations when those relations are part of the data.

### Distribution and local execution

A remote invocation is a process that transmits the required definition identities and input values, obtains or fetches dependencies, executes under a specified world, and returns a result. Code mobility is meaningful because code and its dependencies are data. The receiving machine can check the received construction and the evidence relevant to its use.

Content identity establishes which definition is requested. It does not establish that an arbitrary remote machine executed it. A result can be checked directly, accompanied by evidence, or accepted under an explicitly selected execution assumption. Those choices are expressed by the processes and propositions already defined in the language.

Local-first execution keeps required definitions and state locally available, with synchronization as an explicit process. Replication is itself a program whose merge operations, delivery assumptions, and observable consistency conditions are specified by the selected protocol. A network partition changes the interactions available to that program; it does not change the identity of a locally retained definition. Shared computation and data can consequently cross machines and organizations without imposing a single global execution order or one central application database.

The long-term computational environment is a persistent space of such typed objects and interactions: locally inspectable and editable, networked when appropriate, and capable of accepting generated computations with checked properties. This description concerns the computational substrate. It does not prescribe a social governance or participant-authority protocol.

### Numerical programs and accelerators

Tensor shapes are dependent indices, layouts are index equivalences, and a tensor operator is a function on those indexed values. Fusion and loop reordering are equations about the selected operations. Parallel reduction requires the relevant algebraic laws or a specification that permits its changed floating-point behavior. A GPU schedule chooses a target realization with explicit memory spaces, synchronization, and launch semantics.

Automatic differentiation is a transformation on programs whose correctness refers to the derivative of their meaning. For an expression language over differentiable scalar primitives, forward differentiation maps an input pair `(x,ẋ)` to `(f(x),Df(x)·ẋ)`; each primitive clause contains its derivative law, and the composition clause uses the chain rule. Reverse differentiation additionally constructs and executes the appropriate adjoint accumulation. Mutation, control flow, nondifferentiable points, and floating-point evaluation require the semantics stated by that transformation. A separate derivative syntax is acceptable only with its checked connection to the actual program.

Optimization may search among proved equivalent realizations using a cost model. The model itself is a function or relation over the relevant program and machine state. Instruction counts, cache behavior, bandwidth, and physical latency are different observations. The semantic theorem and the performance claim are therefore distinct typed statements about the same artifact. This keeps tensor compilation and performance engineering within the existing framework without pretending equality determines hardware timing.

### Whole-system examples

A program may include a kernel, driver, client, server, network, and the computations that install or change them. The handler construction in §7.6 gives one precise way to internalize selected interactions and leave the rest open. A compiled realization can consist of multiple artifacts; its relation to the source computation depends on the semantics and correctness evidence for those artifacts. The trading-fleet example is one possible application of this general construction, not an additional language facility.

## 10.7 Programs that change running programs

A program may compute an executable representation, start it, record observations, replace part of its state, or change how its constituent computations interact. Dynamic software updating has an established technical literature ([40](#ref-40), [41](#ref-41)); the update below is a defined transition of the particular represented program. Such operations are themselves programs over the relevant machine and environment definitions. Their correctness is a predicate over the actual before-and-after executions. For example, if a particular application represents a live state by \(s:S\), an update by \(u:S\to S'\), and an invariant by \(I:S\to\mathcal U\), preservation is a term of type

\[
\prod_{s:S}I(s)\to I'(u(s)).
\]

This dependent program specifies the update and its preservation condition. If an update is allowed while messages are pending, the application's state and transition rules must account for those interactions. If the update changes representation along an equivalence, dependent transport can move the corresponding structure. If it changes observable behavior, a refinement or other stated relation is needed. In either case, the operation and its evidence are written in the language. No universal state layout, message representation, or migration architecture is assumed.

### A mixed-version transition, defined by an application

For a concrete example, a program may represent installed versions by a finite set \(V\), a routing function by \(R\), pending authenticated events by \(Q\), and a durable ledger by \(\ell\). These are fields chosen by this application, not types supplied by the language. Its upgrade is a sequence of transitions:

\[
\begin{aligned}
(\{v_0\},R,Q,\ell)
&\xrightarrow{\mathsf{install}(v_1)}
(\{v_0,v_1\},R,Q,\ell)\\
&\xrightarrow{\mathsf{routeNew}}
(\{v_0,v_1\},R',Q,\ell)\\
&\xrightarrow{\mathsf{drainOld}}
(\{v_0,v_1\},R',Q',\ell')\\
&\xrightarrow{\mathsf{retireOld}}
(\{v_1\},R',Q',\ell').
\end{aligned}
\]

Suppose every event has a stable authenticated identifier, the ledger atomically records applied identifiers before acknowledging them, and the new version can decode every event that may be routed to it. Installing changes no ledger entry. **Routing new admissions to \(v_1\) occurs before draining**: the routing transition forbids creation of further \(v_0\)-owned obligations while preserving the assignment of existing old-version work. Draining applies the idempotent ledger transition; retirement is enabled only when no route, active frame, queued delivery, or in-flight request still refers to \(v_0\). Induction over these four transition constructors preserves the at-most-once ledger invariant, including the mixed-version interval.

For progress, let \(D_0\) be the finite set of old-version obligations at the instant routing changes. Assume no handling transition creates another member of \(D_0\), each obligation eventually reaches a terminal state under the selected delivery and processing fairness conditions, and terminal processing removes it. Finite induction on the number of remaining obligations gives eventual emptiness of \(D_0\): select a member of a nonempty finite set, await its terminal processing, and continue on the strictly smaller set. Under fairness for the retirement transition, \(v_0\) can then retire. Without the admission stop, the finite-decrease argument would not apply. Crash recovery must restore the durable applied-identifier set and ledger consistently before accepting replay; retiring a process does not itself reverse or finish external effects. This example retains the historical mixed-version calculation without introducing a privileged `Live`, `Deployment`, or `Message` construct into hyperactivity.

## 10.8 Content-addressed definitions, identity, and a decentralized codebase

Unison provides a concrete precedent for treating program definitions as structured, content-addressed objects rather than source files whose mutable names supply their identity ([42](#ref-42), [43](#ref-43)). In a content-addressed codebase, a definition is stored with its typed syntax tree and dependency references. A name is a view or alias; a stable reference identifies a particular definition. The immediate advantage is that dependencies and reuse refer to the actual definitions involved, so renaming does not rewrite the computation. A change to a definition produces a new object and determines precisely which dependent constructions may require recomputation.

For a typed code store \(\mathsf{Def}\), write \(\mathsf{deps}(d)\) for the dependency family of definition \(d\), and \(\mathsf{digest}(d)\) for its content address. The digest identifies a representation under the store's collision-resistance or injectivity assumptions; it does not prove semantic equivalence. In particular,

\[
\mathsf{digest}(d_1)=\mathsf{digest}(d_2),\qquad
 d_1\equiv d_2,\qquad
 \mathsf{SemEq}(d_1,d_2)
\tag{10.22}
\]

are different statements: address identity, definitional equality, and a specified relation between interpreted meanings. When the definitions denote structured types, \(\mathsf{SemEq}\) may be an equivalence of those structures; for programs it may instead be a contextual or observational relation. Its type is determined by the interpretations involved. The store can record each relation with its actual evidence and use the strongest available one for the operation being performed. For example, replacing an algebraic representation by an equivalent one can transport dependent laws and operations, whereas a mere hash match is evidence about the stored representation. A proof of a source/target refinement can relate different definitions without identifying their complete semantics.

A codebase may consequently contain not only programs but their typing derivations, semantics, equivalences, transformations, dependency certificates, and reusable proofs. An edit is a program transforming this structure. Its effect on dependent definitions can be computed by the same dependency operations used for persistent evaluation (§10.1). Matching the recorded dependencies and observations is a sufficient certificate for reusing a deterministic pure result; it is not necessary, since a changed dependency may leave the result unchanged, or an independently established equivalence may justify reuse. A proof of a representation change may permit reuse even when the stored syntax differs. There is no need for an independently maintained table of which textual files, generated artifacts, test expectations, and deployment descriptors are supposed to correspond to one another. They are constructions connected by the operations that actually produce them.

The same references support decentralized exchange. A peer can send a definition, its dependency closure, and its proofs; the recipient checks and uses them in its own context. The consequence of communication is not merely delivery of bytes but availability of a construction for further computation. A shared global namespace is unnecessary for definition identity; authenticity of external observations and machine semantics remain explicit premises. Distribution of programs and reuse of knowledge are thus instances of typed exchange.

## 10.9 Interactive symbolic programming over the actual computational object

A programming environment operates on retained objects rather than repeatedly reconstructing them from source text. Source, proof, dependency graph, executable bytes, live state, and causal trace are selected representations or observations, connected by the definitions and transformations that produce them. Their relationships—not matching filenames or line numbers—determine what can be reused or changed.

A program interaction may be represented by a typed query or edit operation. For instance, given \(t:\mathsf{Code}(\Gamma,A)\), the environment can compute its type, free-variable dependencies, a symbolic residual under an environment substitution, a proof-relevant transformation, or a selected executable realization. An edit is a function on checked code, accompanied where required by a semantic relation between its input and output. A change to a dependency invalidates exactly those retained results whose observation certificates no longer hold. The object may be inspected at any chosen cut: an expression, function, thread, OS-hosted process, distributed service, or complete fleet.

A debugger can similarly operate on a running program and its *actual* recorded observations. Suppose a ledger state appears inconsistent with an exchange acknowledgment. The debugger can retain as variables the missing packets, scheduling decisions, and recovery events; the compatible executions form the fiber of the observation map. Symbolic evaluation specializes the program with the known evidence and leaves the unresolved interactions as residual computations. Enlarging the cut to include the network or exchange simulator internalizes some of those interactions; shrinking it exposes them again as commands at the boundary. The same program can be viewed at source level, through compiled instructions, or as distributed event dependencies, using the corresponding semantic relations and execution evidence. This is interactive symbolic computing, not a log-search interface pretending to have reconstructed the program.

**One symbolic debugging operation.** Let \(\mathsf{Run}(F)\) be an execution family for a particular program under its specified environment and machine semantics, and let \(o:\mathsf{Run}(F)\to O\) record the observed event prefix. For a recorded prefix \(y:O\), the debugger's input is the dependent type
\[
\mathsf{CompatibleRun}(y):=\sum_{\rho:\mathsf{Run}(F)}(o(\rho)=y).
\tag{10.23}
\]
If a new authenticated event \(e\) arrives, its observation-update operation \(u_e:O\to O'\) and extended observation \(o':\mathsf{Run}(F)\to O'\) define a restriction
\[
\mathsf{CompatibleRun}'(y,e):=
 \sum_{(\rho,p):\mathsf{CompatibleRun}(y)}(o'(\rho)=u_e(y)).
\tag{10.24}
\]
A residual evaluator maps this family to the still-possible ledger values or causal predecessors; it does not invent the missing packets. When a packet is recovered, the new equality witness narrows the fiber, and cached symbolic results are reused only under their recorded observation dependencies. Navigating from source to machine instructions uses the compiler's established trace relation; enlarging the cut to include a network handler uses the handler interpretation defined in §7.6. Thus an actual interactive operation has typed inputs, an observation update, and a defined output family.

The interface itself is a program: a visualization of a data structure, a proof tree, a graphical state machine, a source editor, and a distributed trace explorer are functions on the same represented constructions. The interaction need not begin with a text file: a question about a running system, an observation of a physical process, or a transformation of a dependent relation can be the immediate operand. A rationale can be inspected or executed, a routine can be used inside a rationale, and a relation can be transformed along a proved equivalence. The display is a chosen observation of the computation, not its privileged source of identity. The conventional text-editor-centered IDE is one possible presentation, not the definition of programming. This is why the programming environment is as consequential as the language's verification power: the programmer can operate directly on definitions, meanings, dependencies, and running computations, rather than manually coordinating several partial representations of them.

## 10.10 A distributed program as an ordinary term

For the distributed example of §7.6, take network choices to include delivery, duplication, loss, and partition. Define `NView`, `FState`, `fleet-step`, `fleet-run`, and `fleet-proc` by the indicated transition rules. A safety property is the dependent type

\[
\mathsf{Safe}(I,b,g):=
\prod_{cs:\mathsf{List}(\mathsf{Adv})}
I(\mathsf{fleetRun}(b,cs,g)).
\]

The induction principle `safe-ind` constructs an inhabitant from a proof of \(I(g)\) and preservation of \(I\) by every adversarial step. Liveness and convergence require the corresponding assumptions about choices and fairness; those properties are not consequences of safety alone. The network, its states, the adversary and the safety predicate are all definitions of this example. Replicated data types supply a separate established family of convergence and exchange protocols ([59](#ref-59)); the safety theorem here follows from the specific transition relation, not from a generic replication guarantee. A different distributed program may use entirely different mathematics. The general language has not acquired a special `Fleet` primitive by expressing this one.

**A concrete long-only order/ledger program.** Fix a natural-number bound \(B\), an identifier type \(\mathsf{Id}\) with decidable equality, and positive quantities. Let \(p\in\mathbb N\) be filled long position, \(O:\mathsf{FinMap}(\mathsf{Id},\mathbb N_{>0})\) the outstanding buy orders, \(U:\mathsf{FinSet}(\mathsf{Id})\) the identifiers ever allocated, \(L:\mathsf{List}(\mathsf{Id}\times\mathbb N_{>0})\) the accepted-fill ledger, and \(Q\) a FIFO queue of authenticated exchange events. The gateway version \(v\) is another state component. Define
\[
R(p,O):=p+\sum_{i\in\operatorname{dom}(O)}O(i),\qquad
I_B(p,O):=R(p,O)\le B.
\tag{10.T1}
\]
This bounds the already filled long position **plus** the maximum additional position represented by admitted outstanding buys. It is not a bound on only open orders. The restricted transition relation is generated by the following operations, with all unmentioned components unchanged:
\[
\begin{array}{ll}
\mathsf{Admit}(i,q):&i\notin U,\ q>0,\ R(p,O)+q\le B
\Rightarrow O':=O[i\mapsto q],\ U':=U\cup\{i\};\\
\mathsf{Receive}(m):&Q':=Q\mathbin{+\!+}[m];\\
\mathsf{Fill}(i,q):&Q=\mathsf{FullFill}(i,q)::Q_0,\ O(i)=q
\Rightarrow O':=O\setminus\{i\},\ p':=p+q,\\
&\hspace{12mm}L':=L\mathbin{+\!+}[(i,q)],\ Q':=Q_0;\\
\mathsf{Cancel}(i):&Q=\mathsf{CancelAck}(i)::Q_0,\ i\in\operatorname{dom}(O)
\Rightarrow O':=O\setminus\{i\},\ Q':=Q_0;\\
\mathsf{Stale}(m):&Q=m::Q_0,\ \text{no matching open order}
\Rightarrow Q':=Q_0.
\end{array}
\tag{10.T2}
\]
An admission also emits the corresponding order request to the external exchange; `Receive` is the input event from its handler. These transitions describe a deliberately restricted full-fill, long-only lifecycle, with no partial fills, no position-reducing sells, and no unmodeled preexisting position. For the claimed relation to actual exchange behavior, the handler must authenticate events, preserve per-order exchange sequencing, and make a terminal cancel acknowledgment exclude unaccounted earlier or later fills. Otherwise dropping a stale fill could make the ledger undercount real position. The handler contract is a premise of that correspondence, not a consequence of the gateway's state invariant.

The initial state has \(p=0\), \(O=\varnothing\), \(U=\varnothing\), \(L=Q=[]\), so \(I_B\) holds for \(B\ge0\). The invariant-based reasoning follows the classical discipline of proving preservation over program transitions ([68](#ref-68)). Admission preserves \(I_B\) by its guard; a full fill moves \(q\) from \(O\) to \(p\), hence leaves \(R\) **unchanged**; cancellation removes a nonnegative outstanding summand; receive and stale handling change neither \(p\) nor \(O\). Therefore induction over finite traces constructs
\[
\mathsf{safe}_B:\prod_{\tau:\mathsf{FiniteRun}(F_B,s_0)}
R(p(\mathsf{last}(\tau)),O(\mathsf{last}(\tau)))\le B.
\tag{10.T3}
\]
The state carries the separate invariant \(\operatorname{dom}(O)\subseteq U\). To prove that each order identifier appears **at most once** in the accepted-fill ledger, maintain simultaneously
\[
\begin{aligned}
J(O,U,L):={}&\mathsf{NoDup}(\mathsf{ids}(L))\\
&\times(\mathsf{set}(\mathsf{ids}(L))\subseteq U)\\
&\times(\mathsf{set}(\mathsf{ids}(L))\cap\operatorname{dom}(O)=\varnothing).
\end{aligned}
\tag{10.T4}
\]
The initial state satisfies \(J\). Admission adds an identifier outside \(U\), so it is neither in the ledger nor an existing open order. A fill removes an open identifier that, by the third conjunct, is absent from the ledger before appending it; the second conjunct follows from \(\operatorname{dom}(O)\subseteq U\). Cancellation shrinks the open domain, and receive/stale transitions leave these sets unchanged. Thus a *second* finite-trace induction establishes \(J\) and \(\mathsf{count}_L(i)\le1\). The risk bound alone does not establish ledger uniqueness: both properties require their own invariant components and preservation arguments.

A composed network/exchange implementation has a larger state than \((v,p,O,U,L,Q)\). Under the stated handler correspondence, projection of each composed transition onto this gateway state either follows (10.T2) or stutters; finite-run induction transfers \(I_B\) and \(J\). A mixed-version gateway may route new identifiers between \(v_0\) and \(v_1\) while sharing authoritative \(p,O,U,L,Q\); routing and installation leave those fields unchanged, and both versions must preserve (10.T2). A representation migration must preserve \(R(p,O)\), identifier uniqueness, and outstanding obligations. Progress of an accepted order additionally assumes a consistent terminal exchange response, eventual delivery, and fair processing of finite queue prefixes. These are properties of this program and its handler, not primitives of hyperactivity or automatic consequences of safety.


## 10.11 A program edit propagates through its dependent constructions

Consider the decoder in §8 and its lifted, checked source representation \(d:\mathsf{Tm}_{C++}(\Gamma,A)\). The programmer changes the accepted packet format from a two-byte payload length to a four-byte length. This is not a textual operation on unrelated files: it is a transformation \(E\) of the represented source declaration and its dependent constructions. The new program \(d':=E(d)\) reads four length bytes and requires a six-byte header. Its source-level behavior is obtained by the same interpreted C++ semantics used for \(d\), not by substituting a separate informal model. A typed source transformation records the new source, its elaboration, and the relation between the old and new semantics—here *not* equivalence, because the accepted packet language intentionally changes.

For example, the bytes `01 00 02 00 00 00 aa bb` decode to tag \(1\) and length \(2\) in either format, but select payloads `00 00` and `aa bb`, respectively. The version selected by the invocation's context determines the interpretation. Changing that selection governs subsequent invocations; an active invocation retains its original definition and snapshot unless a migration satisfying §10.5 is supplied.

Let \(D\) be a persistent definition graph. A node has a checked definition and a dependency certificate; its dependent nodes may include the packet schema, decoder correctness proof, a specialized parser, a native artifact, a UI rendering, and the live gateway deployment. Write \(\mathsf{deps}(x)\) for the exact dependencies of a node and \(\mathsf{valid}(x,D')\) for the certificate check in the updated graph. The edit operation computes a new graph \(D'\), retaining a derived value \(x\) only if its dependency certificate still validates:

\[
\mathsf{edit}(D,d,d')=D',\qquad
\mathsf{reuse}(x,D'):\mathsf{valid}(x,D')\to\mathsf{Value}(x).
\tag{10.25}
\]

The old decoder proof, whose statement mentions the four-byte header, does not silently become a proof about the six-byte header. Its dependent type changes and must be inhabited for \(d'\). The residual parser is recomputed using the new fixed header layout and the still-unknown incoming packets. The C lowering and downstream compiler construct the new executable artifact, with the composed source-to-machine relation of §9.4. A rendering of the definition graph changes as a derived view, while the old source and old artifact remain immutable, addressable constructions. The deployment operation can then install the new artifact, arrange compatibility for in-flight old-format messages, and change routing through the mixed-version states of §10.7.

The edit, proof, residualization, emitted artifact, dependency update, and rollout are operations on one represented computation. The two protocol versions can coexist; compatibility is determined by their actual interpreters and routing transitions. A language server can expose the dependent changes, while a debugger can interpret an observed packet under either version and retain unknown bytes symbolically. Their results are determined by the available definitions and recorded observations.

# 11. Conclusion

A whole can obey a reversible rule while a selected observation of it admits no autonomous rule at all. The two-bit calculation in §1 makes the distinction explicit: the question is whether the whole-system step descends through the observation. The same form of question recurs whenever a description forgets information, a representation is changed, or a conclusion is claimed from partial evidence. An observation identifies a fiber of compatible histories; a rationale establishes what follows over that fiber; an equivalence of representations transports the dependent constructions that have actually been established.

These are not operations performed from an unexplained vantage outside the system. The observer, its interpretation, and its reasoning can be included in the described evolution. The definitions of the language that expresses those relations can themselves be represented as terms, and operations on those definitions can themselves be interpreted, checked, specialized, and transformed. A routine, a relation on its behavior, and a rationale for that relation are constructions in one language, not three authorities between which the engineer must translate.

This closure has a precise executional consequence. Under the evaluator-adequacy, world-correspondence, and machine-simulation premises stated in §9, one machine-realized evaluator executes every represented program, including interpreters and transformations of programs. The primary realization composes a C lowering with the chosen downstream compilation relation; direct instruction-level realizations are alternatives. Source syntax, interpreted behavior, and target bytes remain different objects: the established relations among them, rather than a shared name, justify their interchange. Self-application alone does not establish checker soundness or compiler correctness; those remain properties to construct in the same language.

A description need not be frozen when its subject changes. Guarded interactions keep processes open; recorded read dependencies say when retained computations still apply; effect histories constrain restarts; content-addressed definitions and their evidence can be inspected and exchanged by participants without a global interpreter of their meaning. This makes the continuing activity of description, inference, and transformation itself available to computation. The result is a language for nature that includes the work of understanding nature, and an executable environment in which that work can act on its own products. This returns to the scientific horizon of *A New Kind of Science* ([64](#ref-64)): the universe and the activity of investigating it are both within the subject of computation. *Hyperactivity* names the activity, not a separate layer standing over it.

# Appendix A. Constructions within the language

The preceding machine and software constructions are particular instances of the language's general capacity to express objects, operations, relations, and rationales. The same terms express arithmetic, analysis, fields, and their evolution; the distinction lies in the specified laws, not in an additional foundation. Natural numbers are inductive. Integers can be constructed by identifying pairs of naturals under

\[
 (a,b)\sim(c,d)\quad:\Longleftrightarrow\quad a+d=c+b.
\tag{A.1}
\]

The equivalence class of \((a,b)\) represents \(a-b\). Addition is induced by \((a,b)+(c,d)=(a+c,b+d)\), and its well-definedness is proved from the defining equality. Rationals arise from integer pairs \((p,q)\) with \(q\ne0\), identified when \(ps=rq\) for \((p,q)\) and \((r,s)\). The sign and nonzero-denominator conditions belong to the construction; they are not omitted in the name of abstraction.

Reals may be constructed from Cauchy sequences with moduli of convergence, Dedekind cuts, or another specified constructive presentation. An equivalence between two presentations is a theorem to be constructed, and univalence permits transporting dependent mathematics along it. Complex numbers can be represented as pairs of reals with multiplication

\[
 (a,b)(c,d)=(ac-bd,ad+bc),\qquad i:=(0,1),\qquad i^2=-1.
\tag{A.2}
\]

These constructions are computational objects, just as the expression syntax in §5 is. Their laws are terms of dependent types, and algorithms operating on their representations are ordinary functions.

Consider a Fourier transform. On an appropriate function space—such as the square-integrable functions on \(\mathbb R\), with the necessary quotient by almost-everywhere equality—the Fourier transform extends to a unitary equivalence of Hilbert spaces:

\[
 \mathcal F:L^2(\mathbb R)\simeq_{\mathrm{unitary}}L^2(\mathbb R),
 \qquad \|\mathcal F f\|_2=\|f\|_2.
\tag{A.3}
\]

Its defining integral on suitable functions is

\[
 (\mathcal Ff)(\xi)=\int_{\mathbb R}f(x)e^{-2\pi i x\xi}\,dx.
\tag{A.4}
\]

The transform is a change of representation with an inverse and additional preserved structure. Differentiation, translation, convolution, and multiplication have corresponding transformed operations. A proof, a symbolic transform program, and a numerical FFT implementation are different constructions related by specified identities and approximation bounds. The general language can express all of them without privileging the finite array representation used by a particular FFT.

For example, on Schwartz functions,

\[
 \mathcal F(f')(\xi)=2\pi i\xi\,\mathcal F(f)(\xi).
\tag{A.5}
\]

The identity is obtained by integration by parts, using the vanishing boundary term. For polynomials interpreted as tempered distributions, the transform instead involves derivatives of the Dirac delta. The different domains matter because they are types with different operations. The statement of the identity determines the computation and its proof.

A physical field theory is likewise a construction involving spaces, fields, differential operators, evolution laws, and observations. Maxwell's equations, Schrödinger evolution, and relativistic geometry are not exceptions to computation. Their definitions may be represented, transformed, and proved about; their numerical approximations and physical measurements have their own stated relations to those definitions.

Arithmetic, Fourier analysis, and field evolution are expressed through the same operations and proof discipline used for represented programs. A machine is one of the processes that this language can describe and transform.



## A.1 Boolean fibers, search, and the exact witness object

For \(n:\mathbb N\), let \(X:=\mathsf{Fin}(2^n)\) represent Boolean assignments to \(n\) variables, and let \(f:X\to\mathsf{Bool}\) evaluate a Boolean formula. The full output partition is

\[
X\simeq\sum_{b:\mathsf{Bool}}\sum_{x:X}(f(x)=b).
\tag{A.6}
\]

The satisfying assignments form the fiber \(\mathsf{Sat}(f):=\sum_{x:X}(f(x)=\mathsf{true})\). Its truncation \(\|\mathsf{Sat}(f)\|\) states satisfiability, while a term of \(\mathsf{Sat}(f)\) supplies an actual witness. Since \(X\) is finite and equality on Boolean values is decidable, the recursion

\[
\mathsf{search}_f:\mathsf{Dec}(\mathsf{Sat}(f))
\tag{A.7}
\]

is defined by finite enumeration and case analysis. Each unsuccessful branch constructs a proof that its assignment does not inhabit the satisfying fiber; a successful branch returns an assignment and its equality witness. This is the elementary complete finite-search construction. Its running cost depends on the representation and evaluation strategy being measured.

## A.2 Arithmetic relations and divisibility

The divisibility relation is constructed from multiplication and equality:

\[
a\mid b:=\sum_{k:\mathbb N}(a\cdot k=b).
\tag{A.8}
\]

On positive natural numbers, divisibility induces a partial order; its meet and join are respectively \(\gcd\) and \(\operatorname{lcm}\), since their universal properties are exactly the corresponding divisibility implications. This is another example of a relation defined through operations and identity, with no primitive semantic-relation category.

## A.3 Complex field evolution

For a source-free electromagnetic field in units \(c=1\), define \(F:=E+iB\). Maxwell's evolution equations \(\partial_tE=\nabla\times B\), \(\partial_tB=-\nabla\times E\) give, by direct substitution,
\[
i\partial_tF=i(\nabla\times B-i\nabla\times E)
=\nabla\times(E+iB)=\nabla\times F.
\tag{A.9}
\]
The constraints \(\nabla\cdot E=\nabla\cdot B=0\) combine as \(\nabla\cdot F=0\). The displayed equation is the complex form of source-free Maxwell evolution ([72](#ref-72)); the Riemann–Silberstein/photon-wave-function representation has an established treatment in Bialynicki-Birula ([73](#ref-73)). It is an exact algebraic change of representation of the same field equations.

# Appendix B. The name *hyperactivity*

*Hyperactivity* is an ordinary English word for unusually intense activity. Here it also names the continuing activity of constructing, interpreting, and acting on relations. A question can become a computation; an equivalence can carry a construction into another representation; an interaction can make a definition, observation, or proof available for further use. The formal language developed in this paper makes that activity programmable, including the descriptions and interpretations through which participants recognize and use relations. The activity is not bounded by one language implementation, machine, or participant.

The word also describes rapid thought and speech, in which expressions may traverse dense networks of relations. Whether those relations are intelligible to a listener depends on the content, the listener's available context, and the interaction—not on the rate of expression alone. In computational settings, the corresponding emphasis is on conductance: how established relations become available to affect subsequent activity across representations and processes.

Greek *hypér* (ὑπέρ), *over*, *above*, or *beyond*, appears in *hypercube*, *hyperplane*, *hypergraph*, *hypertext*, *hypermedia*, *hypervisor*, and *hyperparameter*, among many other words. The associations differ. Here they include cubical dimensions, higher paths and their coherences, higher groupoids, higher-order operations, reflection, metaprogramming, and interpretation of represented languages. English *activity* carries *action*, *acting*, *actuality*, *enactment*, *agency*, and *reactivity*, with roots in Latin *actus* and *agere*. Active objects, actor systems, reactive programming, active databases, and transactions supply further associations. The corresponding mathematics includes transport, evaluation, transformation, and continuing interaction.

These connections illuminate the word without reducing it to a technical expansion.

# Appendix C. An architecture-specific realization

The primary route in §9.4 reuses a C compiler. This appendix retains the direct-x86 example and interface presentation from the earlier manuscript as a supplementary instance of source-to-target reasoning. Its chosen ABI, byte encodings, and interface names are not requirements for the language, its operating-system scope, or its C-first implementation. A direct encoder's correctness still requires the stated machine relation; source-level specialization alone does not supply it.

## C.1 A complete decoder and its byte vector

Instruction encoding in this example is read against the relevant x86-64 architecture specification ([31](#ref-31)); the abstract decoder and proof are the particular constructions defined below.

Consider a byte buffer \(b\), its length \(s\), and an output record \(o\). The source operation decodes a four-byte header consisting of a 16-bit tag and 16-bit payload length, followed by a borrowed payload slice. The result is failure when \(s<4\) or when the payload length exceeds \(s-4\). Otherwise it writes \(\mathsf{tag},\mathsf{len},\mathsf{ptr}\) into the output record and returns success. **Header reads are defined only after the length guard succeeds**; the source semantics must not perform an out-of-bounds read merely to decide whether a short input is valid. Write \(b\) for the readable buffer and \(\mu\) for the initial memory.

For the branches with \(s\ge4\), define \(n:=\mathsf{read16le}(b,2)\) and
\(t:=\mathsf{read16le}(b,0)\); neither read is evaluated in the short-input branch. Then

\[
D(b,s,o):=
\begin{cases}
(\mathsf{false},\mu),&s<4,\\
(\mathsf{false},\mu),&s\ge4\ \land\ n>s-4,\\
(\mathsf{true},\mathsf{writeFrame}(\mu,o,t,n,b+4)),
&s\ge4\ \land\ n\le s-4.
\end{cases}
\]

Equivalently, a Boolean `good` is a *short-circuiting* dependent case split on \(s\ge4\), with the header reads confined to its true branch. The memory-read preconditions for \(\mathsf{read16le}\) are therefore available whenever the reads occur. The two failure branches leave \(\mu\) unchanged.

The input premise includes readable \([b,b+s)\), writable output frame \([o,o+24)\), their disjointness, **\([o,o+24)\cap[sp,sp+8)=\varnothing\)** for the return-address slot, valid pointer arithmetic, and the selected 64-bit little-endian ABI. The input buffer and return-address slot are readable and unchanged by the stores. Without the output/stack separation the first store could overwrite the address consumed by `ret`, so the return theorem would be false. Under these conditions the following 46 bytes realize the function. Offsets are hexadecimal:

```text
00  48 83 fe 04       cmp   rsi, 4
04  72 25             jb    2b
06  0f b7 4f 02       movzx ecx, word [rdi+2]
0a  48 83 ee 04       sub   rsi, 4
0e  48 39 f1          cmp   rcx, rsi
11  77 18             ja    2b
13  0f b7 07          movzx eax, word [rdi]
16  48 89 02          mov   [rdx], rax
19  48 89 4a 08       mov   [rdx+8], rcx
1d  48 8d 47 04       lea   rax, [rdi+4]
21  48 89 42 10       mov   [rdx+16], rax
25  b8 01 00 00 00    mov   eax, 1
2a  c3                ret
2b  31 c0             xor   eax, eax
2d  c3                ret
```

The proof consists of the following intermediate assertions, with \(\mu\) the initial memory, \(n=\mathsf{len}(b)\), and \(t=\mathsf{tag}(b)\):

\[
\begin{array}{c|l}
\mathsf{PC}&\text{assertion after reaching the indicated offset}\\\hline
00&rdi=b,\ rsi=s,\ rdx=o,\ \mathsf{Mem}=\mu\\
06&s\ge4\\
0a&rcx=n\\
0e&rsi=s-4,\ rcx=n\\
13&n\le s-4\\
16&rax=t\\
19&\mathsf{Mem}=\mu[o+0:=t]\\
1d&\mathsf{Mem}=\mu[o+0:=t,o+8:=n]\\
21&rax=b+4\\
25&\mathsf{Mem}=\mathsf{writeFrame}(\mu,o,t,n,b+4)\\
2a&rax=1\\
2b&(s<4\lor(s\ge4\land n>s-4)),\ \mathsf{Mem}=\mu\\
2d&rax=0,\ \mathsf{Mem}=\mu.
\end{array}
\]

For each instruction the target decoding equation and state-transition equation establish the corresponding row. In the failure-row assertion, \(n\) is meaningful only under the \(s\ge4\) disjunct; no header read is performed along the \(s<4\) branch. The two relative branches target \(0x2b\): \(0x06+\mathsf{sext}_8(0x25)=0x2b\) and \(0x13+\mathsf{sext}_8(0x18)=0x2b\). The unsigned `ja` branch is taken precisely when \(n>s-4\). The `ret` steps restore the calling frame under the ABI premise. Concatenating the block derivations yields

\[
\chi_b:
\prod_{z:\mathsf{Input}}
\mathsf{Pre}(z)\to
\sum_{\sigma'}\mathsf{Exec}_M(b,\mathsf{init}(z),\sigma')
\times\mathsf{Path}
  (\mathsf{Bool}\times\mathsf{Mem})
  (\mathsf{observe}(\sigma'))(D(z)).
\]

The return-path equality in the byte-decoder derivation uses the observed return value and the output frame; it does not assert identity of all registers, stack bytes, or execution traces. The failure path leaves the output frame untouched, and the success path writes its three fields in order. This example makes the byte-encoding contract explicit without confusing one concrete program with the definition of the general compiler.

## C.2 A direct-byte interface and its separate obligations

For a checked definition \(f:T\), the operation `#jit f` quotes its code, specializes it, and checks the result against \(f\): conversion suffices for normalization, while theorem-directed rewrites supply an explicit path. The particular direct byte encoder then maps its supported first-order operations and certain closure forms to x86-64 instructions. This describes the directly compiled cases of that encoder, not a restriction on the terms admitted by the universal evaluator realization (9.U2). The operations `#build` and `#build-loop` construct standalone executable artifacts for specified terminating and continuing processes, respectively. These are executable transformations with defined input and output objects.

The machine interpretation of emitted bytes is a function of the byte vector and its calling convention. `mn-init bs args` places the bytes, arguments, call context, arena, stack, and return address into an x86 machine state. `run-to` iterates the machine transition `x86-step` until the return address is reached or the supplied fuel is exhausted. `mn-run` extracts the result only when execution reaches that address with a clear overflow flag. The resulting operations have types:

```text
mn-run : Lst Word → Word → Lst Word → Opt Word
Correct1 : Lst Word → (Word → Word) → Type
Correct1 bs f = (n a : Word) → mn-call1 bs f n a ≡ f a
```

Here `mn-call1` uses the source definition `f` as its fallback if the modeled machine run does not return an ordinary result. Consequently `Correct1` by itself does **not** assert that all machine runs terminate, that all bytes have an ordinary return, or that the target semantics is fully proved correct for arbitrary programs. Its definition is precise; the separate correctness argument must address the machine cases that matter. The relation between specialized terms and their original definitions is checked at the term level. A theorem connecting emitted bytes to the instruction semantics additionally requires proofs of the encoder templates under the chosen machine model. These are distinct composable proof obligations over the same interpreted computation.

The general source-to-target result type and its composition with source properties are given in §9.4. These supplementary operations illustrate one such target, not a second meaning for the computation.

# References

1. []{#ref-1} Church, A. (1936). “An Unsolvable Problem of Elementary Number Theory.” *American Journal of Mathematics* 58(2), 345–363. [doi:10.2307/2371045](https://doi.org/10.2307/2371045).
2. []{#ref-2} Turing, A. M. (1936–1937). “On Computable Numbers, with an Application to the Entscheidungsproblem.” *Proceedings of the London Mathematical Society* s2-42(1), 230–265. [doi:10.1112/plms/s2-42.1.230](https://doi.org/10.1112/plms/s2-42.1.230).
3. []{#ref-3} McCarthy, J. (1960). “Recursive Functions of Symbolic Expressions and Their Computation by Machine, Part I.” *Communications of the ACM* 3(4), 184–195. [doi:10.1145/367177.367199](https://doi.org/10.1145/367177.367199).
4. []{#ref-4} Plotkin, G. D. (1981). *A Structural Approach to Operational Semantics*. Technical Report DAIMI FN-19, Aarhus University.
5. []{#ref-5} Reynolds, J. C. (1972). “Definitional Interpreters for Higher-Order Programming Languages.” *Proceedings of the ACM Annual Conference*, 717–740. [doi:10.1145/800194.805852](https://doi.org/10.1145/800194.805852).
6. []{#ref-6} Martin-Löf, P. (1984). *Intuitionistic Type Theory*. Bibliopolis.
7. []{#ref-7} Coquand, T., and G. Huet (1988). “The Calculus of Constructions.” *Information and Computation* 76(2–3), 95–120. [doi:10.1016/0890-5401(88)90005-3](https://doi.org/10.1016/0890-5401(88)90005-3).
8. []{#ref-8} The Univalent Foundations Program (2013). *Homotopy Type Theory: Univalent Foundations of Mathematics*. Institute for Advanced Study. [Book](https://homotopytypetheory.org/book/).
9. []{#ref-9} Cohen, C., T. Coquand, S. Huber, and A. Mörtberg (2018). “Cubical Type Theory: A Constructive Interpretation of the Univalence Axiom.” *21st International Conference on Types for Proofs and Programs (TYPES 2015)*, LIPIcs 69, Article 5, pp. 5:1–5:34. [doi:10.4230/LIPIcs.TYPES.2015.5](https://doi.org/10.4230/LIPIcs.TYPES.2015.5).
10. []{#ref-10} Bezem, M., T. Coquand, and S. Huber (2014). “A Model of Type Theory in Cubical Sets.” *TYPES 2013*, LIPIcs 26, 107–128. [doi:10.4230/LIPIcs.TYPES.2013.107](https://doi.org/10.4230/LIPIcs.TYPES.2013.107).
11. []{#ref-11} Lumsdaine, P. L., and M. Shulman (2020). “Semantics of Higher Inductive Types.” *Mathematical Proceedings of the Cambridge Philosophical Society* 169(1), 159–208. [arXiv:1705.07088](https://arxiv.org/abs/1705.07088).
12. []{#ref-12} Berger, U., and H. Schwichtenberg (1991). “An Inverse of the Evaluation Functional for Typed λ-Calculus.” *Proceedings of LICS 1991*, 203–211.
13. []{#ref-13} Danvy, O. (1996). “Type-Directed Partial Evaluation.” *Proceedings of POPL 1996*, 242–257. [doi:10.1145/237721.237784](https://doi.org/10.1145/237721.237784).
14. []{#ref-14} Futamura, Y. (1971). “Partial Evaluation of Computation Process—An Approach to a Compiler-Compiler.” *Systems, Computers, Controls* 2(5), 45–50.
15. []{#ref-15} Jones, N. D., C. K. Gomard, and P. Sestoft (1993). *Partial Evaluation and Automatic Program Generation*. Prentice Hall.
16. []{#ref-16} Moggi, E. (1991). “Notions of Computation and Monads.” *Information and Computation* 93(1), 55–92. [doi:10.1016/0890-5401(91)90052-4](https://doi.org/10.1016/0890-5401(91)90052-4).
17. []{#ref-17} Hoare, C. A. R. (1969). “An Axiomatic Basis for Computer Programming.” *Communications of the ACM* 12(10), 576–580. [doi:10.1145/363235.363259](https://doi.org/10.1145/363235.363259).
18. []{#ref-18} Leroy, X. (2009). “Formal Verification of a Realistic Compiler.” *Communications of the ACM* 52(7), 107–115. [doi:10.1145/1538788.1538814](https://doi.org/10.1145/1538788.1538814).
19. []{#ref-19} ISO/IEC (2020). *ISO/IEC 14882:2020: Programming Languages—C++*. International Organization for Standardization.
20. []{#ref-20} Batty, M., S. Owens, S. Sarkar, P. Sewell, and T. Weber (2011). “Mathematizing C++ Concurrency.” *Proceedings of POPL 2011*, 55–66. [doi:10.1145/1926385.1926394](https://doi.org/10.1145/1926385.1926394).
21. []{#ref-21} Lamport, L. (1978). “Time, Clocks, and the Ordering of Events in a Distributed System.” *Communications of the ACM* 21(7), 558–565. [doi:10.1145/359545.359563](https://doi.org/10.1145/359545.359563).
22. []{#ref-22} Abadi, M., and L. Lamport (1991). “The Existence of Refinement Mappings.” *Theoretical Computer Science* 82(2), 253–284. [doi:10.1016/0304-3975(91)90224-P](https://doi.org/10.1016/0304-3975(91)90224-P).
23. []{#ref-23} Milner, R. (1980). *A Calculus of Communicating Systems*. Lecture Notes in Computer Science 92. Springer.
24. []{#ref-24} Acar, U. A., G. E. Blelloch, and R. Harper (2006). “Adaptive Functional Programming.” *ACM Transactions on Programming Languages and Systems* 28(6), 990–1034. [doi:10.1145/1186632.1186634](https://doi.org/10.1145/1186632.1186634).
25. []{#ref-25} Hammer, M. A., K. Y. Phang, M. Hicks, and J. S. Foster (2014). “Adapton: Composable, Demand-Driven Incremental Computation.” *Proceedings of PLDI 2014*, 156–166. [DOI](https://doi.org/10.1145/2594291.2594324).
26. []{#ref-26} Foster, J. N., M. B. Greenwald, J. T. Moore, B. C. Pierce, and A. Schmitt (2007). “Combinators for Bidirectional Tree Transformations: A Linguistic Approach to the View-Update Problem.” *ACM TOPLAS* 29(3), Article 17. [doi:10.1145/1232420.1232424](https://doi.org/10.1145/1232420.1232424).
27. []{#ref-27} Roşu, G., and T. F. Şerbănuţă (2010). “An Overview of the K Semantic Framework.” *Journal of Logic and Algebraic Programming* 79(6), 397–434. [doi:10.1016/j.jlap.2010.03.012](https://doi.org/10.1016/j.jlap.2010.03.012).
28. []{#ref-28} Sewell, P., F. Zappa Nardelli, S. Owens, G. Peskine, T. Ridge, S. Sarkar, and R. Strniša (2010). “Ott: Effective Tool Support for the Working Semanticist.” *Journal of Functional Programming* 20(1), 71–122. [doi:10.1017/S0956796809990293](https://doi.org/10.1017/S0956796809990293).
29. []{#ref-29} Norell, U. (2009). “Dependently Typed Programming in Agda.” *Advanced Functional Programming*, LNCS 5832, 230–266. [doi:10.1007/978-3-642-04652-0_5](https://doi.org/10.1007/978-3-642-04652-0_5).
30. []{#ref-30} Lattner, C., and V. Adve (2004). “LLVM: A Compilation Framework for Lifelong Program Analysis & Transformation.” *Proceedings of CGO 2004*, 75–86.
31. []{#ref-31} Intel Corporation. *Intel 64 and IA-32 Architectures Software Developer’s Manuals*. Instruction-set and system-programming specifications. [Manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html).
32. []{#ref-32} NVIDIA Corporation. *CUDA C++ Programming Guide*. Programming and execution model. [Guide](https://docs.nvidia.com/cuda/cuda-c-programming-guide/).
33. []{#ref-33} Lawvere, F. W. (1969). “Diagonal Arguments and Cartesian Closed Categories.” In *Category Theory, Homology Theory and Their Applications II*, Lecture Notes in Mathematics 92, 134–145.
34. []{#ref-34} Gödel, K. (1931). “Über formal unentscheidbare Sätze der Principia Mathematica und verwandter Systeme I.” *Monatshefte für Mathematik und Physik* 38, 173–198.
35. []{#ref-35} Tarski, A. (1936). “Der Wahrheitsbegriff in den formalisierten Sprachen.” *Studia Philosophica* 1, 261–405.
36. []{#ref-36} Reynolds, J. C. (1983). “Types, Abstraction and Parametric Polymorphism.” *Information Processing 83*, 513–523.
37. []{#ref-37} Reynolds, J. C. (2002). “Separation Logic: A Logic for Shared Mutable Data Structures.” *Proceedings of LICS 2002*, 55–74.
38. []{#ref-38} Lamport, L. (2002). *Specifying Systems: The TLA+ Language and Tools for Hardware and Software Engineers*. Addison-Wesley.

39. []{#ref-39} Xia, L.-Y., Y. Zakowski, P. He, C.-K. Hur, G. Malecha, B. C. Pierce, and S. Zdancewic (2020). “Interaction Trees: Representing Recursive and Impure Programs in Coq.” *Proceedings of the ACM on Programming Languages* 4(POPL), Article 51. [doi:10.1145/3371119](https://doi.org/10.1145/3371119).
40. []{#ref-40} Hicks, M., J. T. Moore, and S. Nettles (2001). “Dynamic Software Updating.” *Proceedings of PLDI 2001*, 13–23. [doi:10.1145/378795.378798](https://doi.org/10.1145/378795.378798).
41. []{#ref-41} Hicks, M., and S. Nettles (2005). “Dynamic Software Updating.” *ACM Transactions on Programming Languages and Systems* 27(6), 1049–1096. [doi:10.1145/1108970.1108971](https://doi.org/10.1145/1108970.1108971).
42. []{#ref-42} Unison Computing. “The Big Idea: Content-Addressed Code.” *Unison Language Documentation*. [Documentation](https://www.unison-lang.org/docs/the-big-idea/).
43. []{#ref-43} Unison Computing. “A Tour of Unison.” *Unison Language Documentation*. [Documentation](https://www.unison-lang.org/docs/tour/).

44. []{#ref-44} Kristensen, M. B., R. E. Møgelberg, and A. Vezzosi (2022). “Greatest HITs: Higher Inductive Types in Coinductive Definitions via Induction under Clocks.” [arXiv:2102.01969](https://arxiv.org/abs/2102.01969).
45. []{#ref-45} Shannon, C. E. (1948). “A Mathematical Theory of Communication.” *Bell System Technical Journal* 27(3), 379–423; 27(4), 623–656. [Part I](https://doi.org/10.1002/j.1538-7305.1948.tb01338.x); [Part II](https://doi.org/10.1002/j.1538-7305.1948.tb00917.x).
46. []{#ref-46} Barwise, J., and J. Seligman (1997). *Information Flow: The Logic of Distributed Systems*. Cambridge University Press. [Publisher](https://www.cambridge.org/core/books/information-flow/).
47. []{#ref-47} Pearl, J. (2009). *Causality: Models, Reasoning, and Inference*, 2nd ed. Cambridge University Press.
48. []{#ref-48} Lewis, D. (1969). *Convention: A Philosophical Study*. Harvard University Press.
49. []{#ref-49} Grice, H. P. (1975). “Logic and Conversation.” In P. Cole and J. L. Morgan (eds.), *Syntax and Semantics*, vol. 3: *Speech Acts*, 41–58. Academic Press.
50. []{#ref-50} Clark, H. H. (1996). *Using Language*. Cambridge University Press.
51. []{#ref-51} Harnad, S. (1990). “The Symbol Grounding Problem.” *Physica D* 42(1–3), 335–346. [doi:10.1016/0167-2789(90)90087-6](https://doi.org/10.1016/0167-2789(90)90087-6).
52. []{#ref-52} Wadler, P. (2015). “Propositions as Types.” *Communications of the ACM* 58(12), 75–84. [doi:10.1145/2699407](https://doi.org/10.1145/2699407).
53. []{#ref-53} Wiener, N. (1948). *Cybernetics: Or Control and Communication in the Animal and the Machine*. MIT Press.
54. []{#ref-54} Ashby, W. R. (1956). *An Introduction to Cybernetics*. Chapman & Hall.
55. []{#ref-55} Hewitt, C., P. Bishop, and R. Steiger (1973). “A Universal Modular ACTOR Formalism for Artificial Intelligence.” *Proceedings of IJCAI 1973*, 235–244. [Proceedings](https://www.ijcai.org/proceedings/1973/31).
56. []{#ref-56} Kahn, G. (1974). “The Semantics of a Simple Language for Parallel Programming.” *Information Processing 74: Proceedings of IFIP Congress 1974*, 471–475. North-Holland. [Original paper](https://www.cs.columbia.edu/~sedwards/papers/kahn1974semantics.pdf).
57. []{#ref-57} Sangiorgi, D. (2011). *Introduction to Bisimulation and Coinduction*. Cambridge University Press.
58. []{#ref-58} Pnueli, A. (1977). “The Temporal Logic of Programs.” *18th Annual Symposium on Foundations of Computer Science*, 46–57. IEEE. [doi:10.1109/SFCS.1977.32](https://doi.org/10.1109/SFCS.1977.32).
59. []{#ref-59} Shapiro, M., N. Preguiça, C. Baquero, and M. Zawirski (2011). “Conflict-Free Replicated Data Types.” *Stabilization, Safety, and Security of Distributed Systems*, LNCS 6976, 386–400. Springer. [doi:10.1007/978-3-642-24550-3_29](https://doi.org/10.1007/978-3-642-24550-3_29).
60. []{#ref-60} Codd, E. F. (1970). “A Relational Model of Data for Large Shared Data Banks.” *Communications of the ACM* 13(6), 377–387. [doi:10.1145/362384.362685](https://doi.org/10.1145/362384.362685).
61. []{#ref-61} Landauer, R. (1961). “Irreversibility and Heat Generation in the Computing Process.” *IBM Journal of Research and Development* 5(3), 183–191. [doi:10.1147/rd.53.0183](https://doi.org/10.1147/rd.53.0183).
62. []{#ref-62} Bennett, C. H. (1973). “Logical Reversibility of Computation.” *IBM Journal of Research and Development* 17(6), 525–532. [doi:10.1147/rd.176.0525](https://doi.org/10.1147/rd.176.0525).
63. []{#ref-63} Jaynes, E. T. (1957). “Information Theory and Statistical Mechanics.” *Physical Review* 106(4), 620–630. [doi:10.1103/PhysRev.106.620](https://doi.org/10.1103/PhysRev.106.620).
64. []{#ref-64} Wolfram, S. (2002). *A New Kind of Science*. Wolfram Media. [Book](https://www.wolfram.com/books/profile.cgi?id=4046).
65. []{#ref-65} Wolfram, S. (2021). “The Concept of the Ruliad.” *Stephen Wolfram Writings*, 10 November. [Essay](https://writings.stephenwolfram.com/2021/11/the-concept-of-the-ruliad/).
66. []{#ref-66} Harel, D. (1987). “Statecharts: A Visual Formalism for Complex Systems.” *Science of Computer Programming* 8(3), 231–274. [doi:10.1016/0167-6423(87)90035-9](https://doi.org/10.1016/0167-6423(87)90035-9).
67. []{#ref-67} Milner, R. (1999). *Communicating and Mobile Systems: The π-Calculus*. Cambridge University Press.
68. []{#ref-68} Dijkstra, E. W. (1976). *A Discipline of Programming*. Prentice-Hall.
69. []{#ref-69} Barwise, J., and J. Perry (1983). *Situations and Attitudes*. MIT Press.
70. []{#ref-70} Lafont, Y. (1990). “Interaction Nets.” *Proceedings of POPL 1990*, 95–108. ACM. [doi:10.1145/96709.96718](https://doi.org/10.1145/96709.96718).
71. []{#ref-71} Maturana, H. R., and F. J. Varela (1980). *Autopoiesis and Cognition: The Realization of the Living*. Boston Studies in the Philosophy of Science 42. D. Reidel. [doi:10.1007/978-94-009-8947-4](https://doi.org/10.1007/978-94-009-8947-4).
72. []{#ref-72} Maxwell, J. C. (1865). “A Dynamical Theory of the Electromagnetic Field.” *Philosophical Transactions of the Royal Society of London* 155, 459–512. [doi:10.1098/rstl.1865.0008](https://doi.org/10.1098/rstl.1865.0008).
73. []{#ref-73} Bialynicki-Birula, I. (1996). “Photon Wave Function.” *Progress in Optics* 36, 245–294. [doi:10.1016/S0079-6638(08)70316-0](https://doi.org/10.1016/S0079-6638(08)70316-0).

74. []{#ref-74} Jain, A. (2026). *Respecting Equality*, §9, “Ordering — what counting discards,” Definitions 9.1 and 9.6, Theorems 9.2 and 9.7–9.8, and Remark 9.15. Historical source: `avikj/math`, `docs/index.html`, revision `b2387e82fb82` (8 October 2026 source). [Pinned source](https://github.com/avikj/math/blob/b2387e82fb826840c8c572028013f434543b3a4d/docs/index.html).
75. []{#ref-75} Jain, A. (2026). *Hyperactivity*, `lib/sort.cub`: total order, sorted multiplicity, merge, `Fs-prop`, and dependent bag elimination. Repository `avikj/hyperactivity`, revision `1e5b1105dac8`, blob `c48b2751bdcd`. [Pinned construction](https://github.com/avikj/hyperactivity/blob/1e5b1105dac8713755eccf17b132dbe269c8f33a/lib/sort.cub).