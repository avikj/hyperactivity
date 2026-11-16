# The Firm as One Program

What hyperactivity is worth to a trading firm: the workspace Avik and Claude develop together (2026-11-15), as an outline. Each point has a label; a label in brackets refers to that point. Section numbers refer to hyperactivity v31 (14 November 2026). The interactive form of the same outline is the Claude artifact "The Firm as One Program".


## 0. Opening (Avik)

- **O1** Open with the CitSec superorganism and the Jane Street superorganism.
  Avik, 2026-11-15: we shall open by discussing the CitSec Superorganism & Jane Street Superorganism.
- **O2** First the human element: the upper echelon of engineering and computer-science talent, concentrated in one superorganism around a well-defined problem: make money with machines.
  Avik: these concentrate the upper echelon of engineering & computer science caliber, demonstrated human intelligence, in a single superorganism oriented around a well-defined problem - make money with machines.
  - **O2.1** Most organizations dilute talent across vague goals; these point all of it at one unambiguous objective.
  - **O2.2** Every idea is tested against PnL within days: an unusually tight and honest feedback loop on intelligence.
  - **O2.3** So every part of computer science is pushed to its limit here under one measure: compilers, distributed systems, hardware, numerics, networking, formal reasoning. See [O7].
- **O3** Collaborative intelligence, the value of information interactions, variance of expertise, choice of tool, focus of problem space.
  Avik: plenty room for discussion here on collaborative intelligence, the value of information-interactions, variance of expertise/choice of tool/problem space focus.
  - **O3.1** Where these interactions go when the firm is one program: [C3], [C16], [C17].
- **O4** This leads to the unique choices of these businesses: CitSec's C++ platform, Jane Street's OCaml platform.
  Avik: this lends to the unique aspects of these businesses & their choices. for example CitSec's C++ platform and Jane Streets OCaml platform.
  - **O4.1** Jane Street: OCaml, chosen for correctness that a large group reviewing one another's code can rely on.
    - **O4.1.1** "Make illegal states unrepresentable" (Yaron Minsky): correctness is a property of the code everyone reads, not of the people who wrote it.
    - **O4.1.2** Pushed into everything: Async and Core libraries, HardCaml for FPGA design, and OxCaml (unboxed types, stack allocation, modes).
    - **O4.1.3** A firm built on correctness, reaching for control of the machine.
  - **O4.2** Citadel Securities: C++, chosen for total control over layout, allocation and latency.
    - **O4.2.1** Templates as a compile-time language for specialization. See [P5], [C4].
    - **O4.2.2** No proofs and pervasive undefined behaviour, so correctness rests on conventions, tests and review discipline.
    - **O4.2.3** A firm built on control, reaching for correctness.
  - **O4.3** Each firm bet on one half of the problem and now spends heavily to recover the other half.
- **O5** Then: what these languages are worth and what they enable.
  Avik: this lends to discussion on the value of these languages and what they enable.
  - **O5.1** OCaml enables a large group to trust shared code through types; C++ enables the last nanosecond. See [O4].
- **O6** Then: what a language requires to support these desiderata.
  Avik: this lends to discussion on what a language requires to support these desiderata.
  - **O6.1** Meaning precise enough to reason about: OCaml's types are the start, dependent types with proofs the end. [P2]
  - **O6.2** Control of the machine as defined meaning: layout, allocation, timing and hardware as definitions with semantics, not escape hatches. [P6], [P7]
  - **O6.3** Staging and specialization in the language itself: templates and HardCaml generators are partial evaluation without a correctness relation; the Futamura projections with proofs are the complete form. [P5]
  - **O6.4** One language across the whole firm: HardCaml shows Jane Street wanting strategy, tooling and circuits in one language. [C1]
  - **O6.5** End-to-end optimization. [O7]
  - **O6.6** Jane Street got correctness and is fighting for control; Citadel Securities got control and is fighting for correctness; the cubical core makes both definitions of one language. [P2], [P6]
- **O7** Nothing in computer science is not developed fully here; this was the three-year study of the industry before execution. Optimization, end-to-end optimization.
  Avik: there is nothing in computer science that is not developed fully here - where do you think I went for my 3 year study of the industry prior to execution of raw technical excellence. optimization, end-to-end optimization
  - **O7.1** You can only optimize across a boundary whose two sides you can relate. Today every boundary blocks optimization: team to team, language to language, process to process, software to silicon.
  - **O7.2** The optimum of the whole is not the sum of local optima, so the firm is locally optimal and globally wasteful.
  - **O7.3** With the whole firm as one program, computable equality to swap representations and specialization that carries proofs, strategy, parser, NIC and photon are optimized as one object. [C4], [C15], [S1]

## 1. What the language provides (from the paper)

- **P1** Observation fibers: what a reading warrants, and when a participant is safe to act (§1, §10.4).
  An observation does not tell you which history happened. It tells you which histories remain compatible with what was seen: the fiber of the observation. What a reading warrants is whatever holds for every history in that fiber, and a refined observation shrinks the fiber, so a conclusion already established stays established. Nothing is guessed and no probability is introduced to hide what is missing. The same construction says when an action is safe, what a participant in a distributed system knows, and what a debugger can still say about a run it only partly saw (hyperactivity v31, §1 and §10.4).
- **P2** Programs, relations and their justifications in one language, with the reasoning kept as data (§1–2).
  A routine, the property required of it and the reasoning that establishes the property are constructions of the same language. A requirement is a chosen family of types and its proof is an inhabitant, so the reasoning an engineer already performs to build something correctly is kept for later use instead of being thrown away or rewritten in a separate verification tool (§1, §2).
- **P3** Transport of whole structures along equivalences (§3–4).
  Equality is something the language computes with. An equivalence between two representations becomes a path between them, and transport along the path carries an entire structure across at once: its operations, their laws, the programs that use them and the proofs about those programs. Changing representation stops being a rewrite and becomes an operation (§3, §4).
- **P4** Existing languages and standards as data, with adequacy proofs (§5, §8).
  A programming language is a record of syntax, judgments, configurations, steps and observations, and its meaning is one guarded unfolding. Fidelity to a published standard is an equivalence between the standard's derivations and the represented ones. C++ in full, with its distinctions between undefined, unspecified and implementation-defined behaviour, sits inside the host as data. Proofs attach to existing source as dependent pairs and erase back to the unchanged code (§5, §8).
- **P5** Specialization (the Futamura projections) that keeps its proofs (§6).
  A specializer applied to an interpreter and a program produces a residual program with the same meaning, and the three Futamura projections are typed instances of that single equation. Specialized code arrives with the path relating it to its source, so speed is obtained without giving up correctness (§6).
- **P6** Machines, operating systems and fleets as one program through composed handlers (§7).
  A processor, a kernel, its guest processes, a network and a fleet of communicating machines are processes with explicit transitions. Composing them with handlers internalizes the interactions between them, so a kernel with its guests, or an entire fleet, is a single program whose properties can be proved. Continuing processes, with their divergence, deadlock and resource bounds, have exact semantics (§7).
- **P7** One evaluator realizes everything, through C (§9).
  Compiling one evaluator, under its stated adequacy and simulation premises, gives every represented program a native realization, including the checker, the specializer and the compiler themselves. Direct compilation of each construct becomes an optimization of that universal route. The chosen route runs through C and an existing compiler, and lifting that compiler into the language turns trust in it into proof (§9).
- **P8** Dependency certificates, effect scopes and hot swapping of running code (§10.1–10.5).
  A retained result records everything it read, including what it found absent, so the system knows exactly when it is still valid. Effects carry their scope, so pure work is reused freely while a payment, a send or a write is never issued twice. A running program can be replaced while preserving the effects it has issued and the responses it is still waiting for (§10.1 to §10.5, §10.7).
- **P9** Content-addressed, decentralized exchange of code with its proofs (§10.8).
  A definition is identified by its content. A name is an alias, a change is a new object, and nothing is overwritten. Address identity, definitional equality and semantic equivalence are kept apart, and peers exchange constructions together with their proofs and dependency closures, with no global namespace and no canonical state (§10.8).
- **P10** Edits propagate through proofs, artifacts and deployment (§10.7, §10.11).
  An edit to a definition flows through everything that depends on it: the proofs, the specialized code, the native artifact, the rendering and the deployment. Old and new versions coexist, and a rollout between them is a defined transition with its invariants proved over the mixed-version interval (§10.7, §10.11).
- **P11** A checked construction is trusted no matter who wrote it.
  Because a claim and its justification are one construction that the machine checks, a result is as trustworthy as the check, whoever or whatever produced it. Trust stops depending on the author.

## 2. What a firm built as one program can do

- **C1** Prove any provable property of the whole firm ([P2], [P6], [P7]).
  The firm can ask any question about itself and receive a proof in place of an estimate. Whether any sequence of exchange events, across every strategy and gateway, can push exposure in a name past its limit is a type over the composed fleet. Today such questions are answered by sampling: tests, simulations, monitoring and post-mortems. In one program they are answered for every run at once.
- **C2** Research and production are one definition run in two worlds ([P6], [P7]).
  A strategy is one definition run under two handlers: a historical world and the live one. The feature computed in research is the same content-addressed object computed in production, and any difference between simulated and real fills is a stated relation instead of a ghost discovered in the profit and loss. What a researcher proves about a signal carries to the deployed artifact through the source-to-machine relation, on a processor through C or on an FPGA.
- **C3** Compute and knowledge accumulate instead of being redone ([P8], [P9]).
  A result computed once is a result for everyone who asks the same question, valid exactly as long as its recorded reads still hold. Large shares of research clusters now spend their cycles recomputing results that already exist somewhere unrecognized; that spending ends. More importantly, a relationship one researcher establishes becomes a construction others build on without re-establishing it and without fear that it means something different in their context.
- **C4** The fast path is a specialization that carries its proof ([P5], [P7]).
  The hot path from packet to order is a specialization of verified definitions, carrying its proof. When the fast path cannot be broken by changing it, it can be changed as freely as research code, and the specialists who guard it become authors of specializations that apply everywhere at once. See [PE].
- **C5** Risk bounds are invariants of the whole composed system ([C1], [P6]).
  The paper's order ledger bounds filled position plus admitted outstanding orders and proves the bound over every admissible sequence of exchange events, including the interval during a rollout (§10.10). That is the SEC's market access rule, 15c3-5, established as a theorem of the running system in place of a check performed system by system. Knight Capital lost about 440 million dollars in 45 minutes on 1 August 2012 because one of eight servers kept old code behind a repurposed flag. In one program the version on each server is a content identity, a flag that means two things in two versions is two different objects, and the rollout's invariant is proved over every interleaving. That failure cannot be constructed.
- **C6** Exact reasoning under partial observation, and the value of information can be computed ([P1]).
  A market maker never sees the true state of the market; it sees its fills, the public book and the tape. The fiber of states compatible with those observations is the exact object its decisions should rest on, and an action is safe when it is safe for every state in that fiber. Because refining an observation shrinks the fiber, the value of a new data source becomes computable: it is how much that source shrinks the fibers on which decisions depend. Data purchased today by intuition can be priced before it is bought.
- **C7** Exchange changes and new venues propagate ([P4], [P10]).
  A venue's protocol and matching rules are definitions. When an exchange changes its packet format, the change flows through the parser's proof, the specialized parser, the native artifact and a rollout in which old and new formats run side by side. A new venue or asset class, such as a new crypto exchange, a prediction market or a newly opened foreign market, is integrated by writing its definition once, after which everything composes with it.
- **C8** Audit trails and compliance are queries over retained effect histories ([P8]).
  The audit trail is the effect history the evaluator already keeps, since its semantics forbids losing or replaying an issued effect. A regulator's question is a query over the information space answered with evidence.
- **C9** Debugging restricts the set of runs compatible with what was observed ([P1]).
  When something goes wrong, the debugger restricts the set of runs compatible with what was observed, holding missing packets as unknowns, and finds the cause in minutes while the strategy would otherwise sit switched off.
- **C10** AI researchers at any scale, because their output is checked, not trusted ([P11]).
  Every firm now tries to use AI models for research and engineering and hits the same limit: their output cannot be trusted, so human review is the bottleneck. When a claim is a construction the machine checks, an AI agent's strategy either satisfies the risk invariant or it does not, and its faster hot path either carries its proof or does not ship. The firm can run ten thousand artificial researchers, their output joining the information space as safely as a person's, each building on everything already established. Research capacity stops being a headcount and becomes a compute budget.
- **C11** Acquired systems and vendor software come in by equivalence and lifting ([P3], [P4]).
  When a firm acquires a team or a company it inherits systems built on different models of the same things, and integration takes years. An equivalence between the acquired representation and the firm's own carries operations, data and proofs across together. Vendor software, from order management to analytics, is lifted as data, and its outputs carry relations the firm checks instead of trusting.
- **C12** The provenance of every running byte is known ([P9]).
  Every definition is identified by content, every change is a new object with its author and rationale, and nothing is overwritten. An unauthorized change is a different identity that cannot silently replace the approved one, so the integrity of the firm's code against insiders and compromised dependencies becomes a property it can check.
- **C13** Contracts, clearing and margin are rules over states ([P2], [P6]).
  Master agreements, clearing obligations, margin terms, exchange membership rules and data licences are rules over states. A margin call is a transition, a clearing obligation is an effect with a scope, a collateral agreement is an invariant, and whether a strategy respects a data licence is a property that can be proved. The firm knows at every moment what it owes and to whom, and legal review stops being the slowest step into a new market.
- **C14** Exchange semantics are definitions the firm can reason about ([P4]).
  The firm can also reason about a matching engine's order types and priority rules as precisely as about its own code, so edge that comes from the exchange's rules is found by construction instead of by accident.
- **C15** Network and FPGA are targets of the same program ([P7]).
  Firms have spent hundreds of millions on microwave and laser links to send information along the shortest path physics allows between Chicago and New Jersey. The network, the switches, the FPGA and the custom silicon are processes with transition semantics, and latency is an observation of them. With strategy, network and circuit in one program, the firm optimizes the whole path instead of each component in its own vocabulary, and a strategy can be specialized into a verified circuit in days.
- **C16** Knowledge belongs to the firm, not to individuals, and onboarding is a walk ([P2], [P9]).
  What a person understood becomes a definition with its rationale, still valid when they go home and still valid when they leave. Key-person risk, and much of what garden leave and non-competes pay for, comes from the firm being unable to separate knowledge from the person. A new hire walks the firm's information space in order, as an agent walks this repository, and becomes productive in days instead of most of a year.
- **C17** Disagreements resolve on a presented shared basis ([P1], plus common ground).
  Two portfolio managers who disagree do so because at least one of them lacks something the other has. When the system presents the shared basis, the facts, models and constraints both accept and the exact point where their inferences part, the disagreement resolves on evidence instead of politics. A firm of brilliant, competitive people loses an enormous amount to internal friction, and that loss shrinks. The work is derived from the science of human happiness: connection, purpose and shared understanding are what people need, and an information space with its ignorance removed provides them.

## Partial evaluation and code generation

- **PE** The value is information: everything known before a packet arrives becomes instructions that never run.
  A protocol is defined once, as data. The parser, builder and validator are not written: each is the residual of one generic interpreter specialized on that definition plus everything else that is static. Only operations on the packet bytes remain, and the proofs carried with the specialization hand the C++ optimizer facts it could never infer.
  - **PE1** Why abstraction costs today: the optimizer removes only work it can prove unnecessary, and proof stops at boundaries.
    - **PE1.1** An unresolved indirect call, an unknown size, possible aliasing, an undischarged bounds check, a branch on a field of unknown range, a translation unit it cannot see.
    - **PE1.2** Templates are zero-cost only when everything static is visible inside them; the decisive static knowledge lives in the protocol specification, the consumer's use of the message and the deployment, outside the code.
  - **PE2** One protocol definition; every artifact is a projection of it. [P5]
    - **PE2.1** A generic protocol interpreter specialized on the definition is a parser (first Futamura projection).
    - **PE2.2** The specializer specialized on the interpreter is a parser generator (second projection).
    - **PE2.3** Builders, validators, book builders, replay tools, test generators and FPGA logic are all specializations of the one definition, so they cannot disagree.
  - **PE3** Binding time is where information becomes speed: push every fact possible into the static side.
    - **PE3.1** A known message set turns type dispatch into a jump table or perfect hash; unsubscribed types have no code.
    - **PE3.2** Fixed-width fields at fixed offsets become single loads with constant offsets.
    - **PE3.3** A known instrument universe turns symbol lookup into an index computed once.
  - **PE4** Specialize across the consumer: fusion. [C4]
    - **PE4.1** Fields the strategy never reads are never decoded.
    - **PE4.2** The intermediate message struct disappears; the strategy runs directly on the receive buffer (deforestation across the producer/consumer boundary).
    - **PE4.3** Impossible across teams by hand, since the parser cannot know every consumer; exact in one program. The first concrete instance of end-to-end optimization. [O7]
  - **PE5** Proofs as optimizer information: assumptions become theorems.
    - **PE5.1** Facts established during specialization: a length validated once, a field in range, buffers that do not alias, an aligned pointer, an impossible case.
    - **PE5.2** Emitted as [[assume(...)]], __restrict, std::assume_aligned, std::unreachable(), constant trip counts, final, noexcept.
    - **PE5.3** Today such hints are avoided or used in fear, because a wrong one is undefined behaviour the optimizer exploits. Here each is a proven precondition: the optimizer's appetite for undefined-behaviour assumptions becomes a source of speed instead of a hazard.
    - **PE5.4** The C++ compiler receives the most information it can be given, and all of it is sound.
  - **PE6** Representation chosen by cost: transport carries code and proofs to the representation the cost model prefers for this specialization and machine. [P3]
    - **PE6.1** Struct-of-arrays or array-of-structs, packed bits or aligned words, an order book as an array by price level or a tree.
    - **PE6.2** The source keeps the representation that suits reasoning; the binary gets the one that suits the hardware.
  - **PE7** Perfect abstraction: an abstraction whose cost in the binary is zero for every static instance.
    The minimum-cost binary for a specialization is the residual of the generic definitions at all available static information, in the representation chosen by cost, with every proven fact handed to the backend.
  - **PE8** The limit is code size, and it is part of the cost.
    Finite specialization chooses memoization points (the static shapes at which residual code is generated); cache footprint is an observation in the cost model. Hot message types get fused straight-line code; rare ones share a generic path.
  - **PE9** Late static information: specialize again, swap safely.
    Subscribed symbols, instrument parameters and mid-day venue changes arrive after compile time. Re-specialization and a swap that preserves issued orders, outstanding requests and session state (§10.5) make "static" mean "known before the next packet". [P8]
  - **PE10** One specialization, many targets: C to a CPU, or logic for an FPGA or NIC, each related to the same source meaning. [P7], [C15]

## Quantitative research (Avik)

- **Q1** "The market": which market? There is one market in the universe of information.
  Avik: citsec operates in damn near every market; certain mathematics generalize, data from one market informs the other. Separation would require proof, and physically there is none: it would take distinct physical universes with zero mutual information.
  - **Q1.1** The price of one stock is not a wise place to start. Start from the whole system: every book on every market globally, with all global data (satellite images of parking lots and much more).
  - **Q1.2** Begin with a universal space of activity and information; "the market" is the space of exchange of assets grounded in this physical universe.
- **Q2** To model markets accurately, resolve economics at large.
- **Q3** The quant's job is the reduction of uncertainty about unobserved (often future) events.
  Avik: the quant would as soon predict the parking lot population on a given day as a market price; it is not a distinct part of the job. Speaking of quants as dealing with price data alone forms the problem incorrectly.
  - **Q3.1** The problem: given environmental data, forecast subsequent environment states. The environment is the earth, the physical universe, with myriad inseparable activity within it.
  - **Q3.2** Every equity, commodity and future corresponds to physical events in the real world.
  - **Q3.3** Pricing is a byproduct, an observation, of complete world state. [P1]
- **Q4** The epistemic basis of prediction: no prior distribution, ever; exact inference over the present.
  Avik: assuming a prior is pure assumption, not reality, and cannot ground any claim of truth; a uniform prior is a worse sin than a reasoned one. Information "from tomorrow" is metaphysically nonsensical: tomorrow is undetermined now. [P1], [N2]
  - **Q4.1** The object of analysis is not a prediction of price, since every action taken influences the world and the price.
  - **Q4.2** The object is exact inference over the present: rationally exposing present information others have not.
  - **Q4.3** We infer and predict the future into existence: when brains are called prediction machines, the prediction is the action; there is no separate action.
    predict: Latin praedicere, prae "before" + dicere "to say", from the root *deyk- "to show, point out" (indicate, index, teach). infer: Latin inferre, "to carry into".
- **Q5** Alpha is exposure: present the present information to the market and be paid for the activity the exposure releases in the economy.
  - **Q5.1** The mature market looks like investigative research: never predicting a price, but asking what information the current price reflects, and whether it already reflects the exposure of this information.
  - **Q5.2** The quant's work is fundamentally studying the true underlying value of companies and realizing it: the quant operates over a market ignorant of the true value of things and is tasked with exposing it.
  - **Q5.3** Avik's example: his quantitative analysis that OpenAI and Anthropic go out of business by 2028, from information about the state of the world in November 2026 (his own work) that other quants lack.
- **Q6** Quantitative research is research into the quantitative loss of an information system: modelling it purely numerically erases fundamentals. The dollar is a metric of loss.
  - **Q6.1** Economics still relies on mathematically inept models; pure mathematics gives clear derivations (capitalism and communism understood from the elements that compose them).
  - **Q6.2** The success of this work is indivisible from the end of scalar financial systems, as of national borders and authorities. [S4]
- **Q7** Fundamental analysis, and the rationale behind it (will this succeed), can be expressed hyperactively: the actual value and interaction chains composing the world, and rational inference over them.
  - **Q7.1** Deep tech investing: the world is ignorant of a mathematical truth, and here is a company oriented to make money on it. It takes mathematical excellence to the point of norm-rejection.
  - **Q7.2** Correct norm-rejection is the only novelty and value creation.
  - **Q7.3** Forcing the outcome: invest in a set of initiatives, then hand them the construction they need to work perfectly, creating mutual value among them.
  - **Q7.4** Human ignorance of underlying value is fundamental to the problem definition; what humans have yet to discover is the economic value awaiting.

## 3. Value to the firm

- **V1** Alpha survives deployment ([C2]).
  Alpha that decays between backtest and deployment survives, because the backtest and the production strategy are one definition.
- **V2** Duplicate compute disappears ([C3]).
  Results already computed somewhere are recognized and reused while their recorded reads still hold.
- **V3** Iteration is faster ([C4], [C7]).
  The fast path and new venues move at research speed.
- **V4** Capital runs closer to its limits ([C5]).
  Capital runs at proven capacity instead of feared capacity.
- **V5** Catastrophic tail events, Knight Capital-style, are ruled out ([C5], [C12]).
  Knight Capital lost about 440 million dollars in 45 minutes on 1 August 2012 because one of eight servers kept old code behind a repurposed flag. With content identity per server and a rollout proved over every interleaving, that failure cannot be built.
- **V6** Compliance and fines ([C8]).
  Compliance shrinks to a projection of the running system, and fines approach zero because violations cannot be constructed.
- **V7** Downtime ([C9]).
  Downtime after incidents drops from days to minutes.
- **V8** Data spend ([C6]).
  Data is priced by how much it shrinks the fibers decisions depend on, before it is bought.
- **V9** Research capacity becomes a compute budget ([C10]).
  Research capacity stops being a headcount.
- **V10** First into every new market ([C7], [C14]).
  The earliest phase of a market has the widest spreads; entering every new market first captures it each time.
- **V11** Mergers and vendor dependence ([C11]).
  Acquisitions integrate in weeks; vendors are verified instead of trusted.
- **V12** Key-person risk and onboarding ([C16]).
  Knowledge stops walking out of the door; new hires are productive in days.
- **V13** Internal friction and happiness ([C17]).
  Disagreement resolves on evidence; the work is derived from the science of human happiness.
- **V14** Proof sold to clients and regulators as a product ([C1], [C8]).
  Brokers that route retail flow to a market maker must show best execution, and the arrangements are under constant scrutiny. A firm that can hand brokers and regulators a proof that each order was handled by stated rules, at a stated price, against stated market conditions, offers something no competitor can, and the trust of clients becomes something established by construction.

## 4. The structural shift

- **S1** The gains multiply instead of adding, which changes the firm's rate of learning (all of layer 3).
  The lines do not add; they multiply. Faster iteration means more strategies tested, cumulative knowledge means each test builds on all earlier ones, proven risk means more capital behind each, and exact reasoning under uncertainty means better decisions per unit of capital. Artificial researchers multiply all of it again. The result is a firm whose rate of learning is itself rising. Its advantage over competitors is a difference in kind, not a margin.
- **S2** The market concentrates toward the firm that has this ([S1]).
  Electronic trading is close to a zero-sum contest between firms over the same flow, and market share already concentrates toward the firms with the best technology. A discontinuity in technology produces a discontinuity in concentration: the firm that has this takes opportunities first, correctly, at full size, everywhere, and its competitors trade against a counterparty that is better informed and quicker to adapt. That is why a figure of several billion understates it.
- **S3** Prices converge on what is true, so the edge that profits came from shrinks ([S2]).
  Edge in markets is information not yet reflected in prices. As the firm extracts it, prices become correct sooner, and the edge available to everyone shrinks. The firm profits precisely by removing the ignorance that made profit possible. Profit in trading measures collective ignorance, and this system dispels collective ignorance at machine speed.
- **S4** What Hayek argued prices must carry becomes directly available, so prices stop being the main way the economy coordinates ([S3], [P9]).
  Hayek argued in 1945 that no central planner could hold the knowledge dispersed across a society, so prices had to carry it in compressed form. Hyperactivity is not a central planner: it has no canonical state, every participant holds its own, and constructions are exchanged between peers. But the knowledge that prices compress becomes available directly, with its evidence, to every participant. Once that holds, prices stop being the primary carrier of coordination, and an economy built around competing partial knowledge gives way to one built around shared knowledge. The trading firm is where this starts, because no institution has more money riding on the gap between what prices know and what is true. The same mechanism, aimed at knowledge instead of money, is the dispelling of ignorance across the human superorganism: thousands of minds in one firm becoming one reasoning process is the small case of billions of minds doing the same.

## Notes

- **N1** Two points carry the most weight.
  C10 is the largest multiplier: checked construction lets AI research scale without trust. S1 is what turns a list of savings into a change in kind. See [C10], [S1].
- **N2** The bridge to the rationality claim.
  P1's warrant is the formal form of "the rational actor cannot be incorrect", and that is what C17 runs on. A rational actor concludes P only when it holds a construction of P over every history compatible with what it observed. Where no such construction exists it concludes nothing, so it is never wrong. The language therefore marks exactly what is amenable to rule-based logic: what is derived, what is observed, and what is a stated premise. English could always carry perfect inference for the rational; the language makes that discipline executable and shared. See [P1], [C17].
