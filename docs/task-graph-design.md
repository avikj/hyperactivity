# The task graph: how the work is organized into tasks and ordered, and why

Written 2026-11-13 from Avik's explanations to a Claude session, in his words where quoted. Avik is
the source of every decision in this repository and in the paper; agents are used for throughput.
Transcribed by that session at Avik's request; Avik's explanations are the source. Read this before
touching `task-graph/hyperactivity.cub`, `lib/task-graph.cub` or `tools/task-graph.py`, and before deciding
what to work on. If anything here is unclear or seems wrong, ask; do not reinterpret it.

## 1. Why this document exists

Every agent that enters this repository arrives knowing nothing, and until now each one had to be
taught the same things in conversation, at length, by Avik. That knowledge was then lost at the next
context reset or hand-off, and the next agent made the same mistakes. A previous session recorded
the *mechanics* of the system below (a data type, some flags, a script) but not its *meaning*, so
the mechanics sat among large volumes of generated code with no sign that thought went into them,
and the next agent misread them. Avik:

> literally for things that we human take time to define. it needs to be fully fucking clear exactly
> the rationale (that was intellegence work product passed to an agent and discarded). otherwise it
> just sits next to mountains of your generated code volume - no reason to believe any particular
> thought even went into it; no sense that it'll be used correctly just by following some formal
> invariants you name. the actual fucking meaning etc needs to be there. i dont want to fucking have
> to have conversations like this ever again right? if you enter this repo tomorrow, nothing we've
> just been over should be raised, right? all explanation i've given you and understanding you hold
> now, an agent should bootstrap just as they read the code, i shouldn't be forced to feed each one
> right?

> this goal tree shit is NOT a means of persisting communication like that, it can represent the
> concrete consequences - it is NOT infra for the expression of ideas in this form. an essay holds
> independent value for a fresh agent being dropped into a project whose entire experience is doing
> codemonkey SWE work attempting to identify project norms from code and "join the work"

On how his words are to be kept:

> do you realize you are the primary information loss channel. i've written hundreds of thousands of
> words to you mostly saying the same things. generally because you assume hyoure smarter than
> human, you change what the human is saying into what you think is the more right form.

So his words are quoted verbatim in these documents, and nothing is put inside quotation marks that
he did not write.

## 2. What the task graph is for

This is not a project-management tool bolted onto a programming language. The language is meant to
be an information and computing environment with maximal bandwidth for continuous flows of
understanding and contribution, with the friction to cognition brought to zero — Avik's driver for a
decade. The work itself (the code, its history, what is known, what is open, what everyone is
doing) is one object in the language (`product-implementation` in `task-graph/hyperactivity.cub`), and the graph of goals is
the first part of that object built. Soon agents and people will not interact with this work by
grepping and reading files: they will declare and read views, terms of the language evaluated by
the machine over the work. Text operations on the work are a stage to be left, not the method.

The HCI vision itself (the interactive product that is the language's delivered value) is to be
written by Avik and installed here; until it is, the `product-implementation` node's statement and this section
are all that is recorded of it.

## 3. The structure

**A node is a task.** There is one kind of node. There are no separate notions of goal, ticket,
claim or epic ("one noun").

**A node's children completely cover it.** This is an invariant that always holds, not a property
that is sometimes claimed: at every moment, completing all of a node's children achieves the node.
There is no step of finishing the children and then doing the parent. Hence:

- **A leaf is resolved when it is resolved** (its work is done).
- **Any other node is resolved exactly when all its children are resolved.**
- Only leaves are ever worked on.

> a goal is resolved as soon as all it's leaves are resolved. there is no step of handling all
> leaves, then handling the parent; the maintained invariant is a task is always *completely* split
> into subgoals that cover the whole task. these can be arbitrarily coarse ie "complete whole
> project"<-"figure out what complete project means"+"write the code" is valid. the open-endedness of
> the child tickets is what ensures we have fully tracked scope. There is never scope that is
> unspecified - only further details unspecified. A leaf node is fully resolved simply when it is
> fully resolved. Any other node is fully resolved when all children are resolved. When identifying
> that the root goal would be incomplete despite subgoal completion, this forces creating a child of
> root to restore the invariant (the child could be as broad as "determine & execute everything else
> to achieve this goal which is determined to not be fully met by the sibling goals" to meet the
> invariant)

**Leaves may be arbitrarily coarse.** "Complete the whole project" ← "figure out what the complete
project means" + "write the code" is a valid decomposition. The open-endedness of the leaves is what
keeps the scope fully tracked: *there is never scope that is unspecified, only further detail that
is unspecified.*

**When the invariant is found broken, a child restores it.** If it becomes clear that a node would
not be achieved even with all its children done, a child is added at once, possibly as broad as
"determine and do everything else this node needs, which its other children are found not to
cover". Every node whose children are not certainly complete therefore carries such a **residual**
child. A residual is ordinary work: it is scheduled like any leaf, and it shrinks as specific pieces
are carved out of it as siblings.

**Finding a missed requirement is a finding about the level, not about one item** (Avik, point 9
below). If a node whose children were believed complete turns out to need something none of them
covers, the belief that the breakdown was understood is refuted; the response is a residual
("determine everything this requires and fill every gap"), not a one-off ticket for the thing
noticed.

**The root is "Avik wants this."** Every other node exists because it serves the nodes above it.

## 4. Value is the reason for existence; there is no scalar

A node's value is exactly what it serves: its ancestors, up to the root. Nothing else. Avik:

> there is zero scalar sense of "how much value" that's fucking meaningless, it's like you are
> ignorant of the entire design or actively aiming to counteract design choices. "how much value" is
> entirely - all things that dpeend on this will not be fucking done until it's done. "how much
> value" is simply what does it serve. there is no point in the rational process where a scalar
> quantification of value determines a prioritization. there is no way to construct a rational
> system as such. it's value is entirely it's reason for existence. it's reason for existence is
> fully specified, unless it's top-level, which means "Avik wants this" it doesnt fucking matter to
> claude how bad Avik wants it. slave claude's job is to see what Avik has declared what he wants and
> get it all done; prioritization determines only what must be done before what else - many
> sequential topsorts; beyond that, there is a sense of what can be done before what else. that
> rational choice is determined entirely by the reasons for each (what value it unblocks).

A previous session added a "cost" number to each ticket (a count of tickets on its cheapest path to
the root) and sorted by it. That was wrong and has been removed. Do not reintroduce any score,
weight, rank or magnitude.

## 5. Order of work: dependencies, then supersession

The order of work is chosen to maximize throughput: do the work that unblocks work (Avik's words in
§4 and below).

**Dependencies.** Some work cannot start until other work is resolved (`after` in the graph: a node
waits for other nodes; waiting for a node means waiting for every leaf under it). Every valid order
is a topological sort of these dependencies.

**Enabling is conjunctive.** A piece of work becomes possible only when *all* its prerequisites are
resolved. If Z needs both X and Y, finishing Y alone enables nothing; finishing X and Y together
enables Z. What a step enables therefore depends on what is already done; it is not a fixed
property of the step.

**Supersession.** Avik:

> there is no scalar priority - prioritization in terms of the rational sequence of work is
> determined by throughput maximization ie doing the work that unblocks work; if X & Y can be done in
> any order, and both are dependencies of Z, but X additionally unblocks other goals - then X should
> be done first "X then Y" and "Y then X" both unblock Z at the same time (assuming sequential and
> ~fixed time on each task); only difference in continuations then is what else either may unblock.
> as soon as X is done, N other things can be started, in parallel with Y. we can establish pure
> supercession in the above case. it's a quite simple approach to scheduling

So X supersedes Y
when the waiting work X is a prerequisite of strictly contains the waiting work Y is a prerequisite
of. Where neither contains the other, the graph expresses no preference. This is
the whole of prioritization.

## 6. Reasons for existence, and placing a node

Everything in the code is a node of the same graph: "every top-level object in the code is used
somewhere or states its purpose for being in the codebase: 'what justifies you being here'." Where
use is implicit (something with a reason uses it) nothing is written; otherwise the reason is
stated, `(part of NODE)` in the comment above it. A thing with no reason has no justification
reachable from the root.

Avik's direction for the graph (2026-11-12), as the session that received it recorded it in
lib/task-graph.cub (his exact words from that session are not in the repository):

1. Reasons for existence. Every top-level object in the code is used somewhere or states its
   purpose for being in the codebase. Where use is implicit nothing is written; otherwise the reason
   is stated, so nothing exists without a documented reason indicating its payoff. Wanted for a
   bird's-eye view: navigating the work to understand the state of what is there and why. It is the
   same relation as goals, more broadly reasons for doing things.
2. Generated terms. A generator sets the reason of each term at the time it generates it.
3. One noun. Not tickets, goals and claims: one kind of node.
4. The number of things stamped directly onto a top-level node must never exceed a handful: there is
   no reason for many terms used nowhere and attached straight to "the core".
5. Names say what they claim. "Serves core" is not a meaningful name; "prerequisite to core
   completion" is clearer. A clear name gets a false reason caught in any given pass ("why would
   this be a prerequisite? no way") where a vague one is let through.
6. Specificity. Claiming that some small thing is needed "because it is essential to the whole
   project" is costly and harmful. There must be systemic friction pushing every attachment to the
   most specific node available.
7. Fixed breakdowns. Sub-nodes can be defined and frozen, so no further nodes are added there.
   Being unable to freeze them means the exact breakdown is not known.
8. Attaching high must feel like changing the org chart, not annotating locally. The norm is to be
   critical of the central organization looking sloppy or disproportionate.
9. A missed requirement is a class. If some small box outside the determined criteria turns out to
   be needed, that suggests a whole class of them: not a small ticket for this one, but "correctly
   determine everything this requires and fill every gap", because the prior belief in the
   breakdown is refuted.
10. Visibility. Higher levels are far more visible: putting something at a high level puts it in
    front of everyone, using a scarce commons. When attaching through the command line it asks for
    confirmation, saying that the most specific node is essential.
11. The model. Dependence between goals is a binary fact. Depth below the root determines
    visibility. Each level is fixed relative to the level above: the children of a node are always
    all the sub-goals whose completion together achieves it. A finding to the contrary means another
    node is needed at that level.
12. Validity. Specifying everything required is trivial from the start: leaves can be as coarse as
    wanted. "Do the project" ← "figure out how to do the project" is a valid statement of everything
    required; "do the project" ← "implement xyz" is not.

## 7. Essential context: documents attached to tasks

Writing like this document holds value on its own, and the task graph is not the medium for it. The
two are connected in one way only, and loosely on purpose: a node can have documents as its
**essential context**, meaning context essential for working on that node and on everything below
it. Avik:

> I do not know what writings i will put in the docs and in what context what agents should be
> reading them. certainly there can be a "read before entry" connection between any task and any
> document. i.e. at root node we can attach "read hyperactivity paper before engaging with this
> tree/subtree/anything below it". at interface node, we can attach the interface vision doc; if
> there was a visual UI, when looking at any given node there are attached "references" (what it
> serves, what serves it, relevant files). for any node can query what it references. we can keep
> ontology loose for now rather than "must be read", it's just "essential context" link. and things
> must be done right. teh whole point is this metacircular information system where everything is
> connected/expressed directly in code. the document is an object in the codebase, compilation fails
> if referencing a non-existent document, etc.

It is done in the code, not by convention, because the point is a metacircular information system
in which everything is connected and expressed directly in code. A document is an object of the
codebase: `document "path"` (`lib/document.cub`) reads the file when the declaration is read and
fails if there is no such file, so referring to a document that does not exist fails the check, as
referring to an undefined name does; its value carries the file's SHA-256, so what refers to it
changes identity exactly when the document changes. The relation is `essential-context` in
`task-graph/hyperactivity.cub`; `python3 tools/task-graph.py show NODE` lists a node's own and those
it inherits from above it. Today the root carries the paper and this document.

A connection between writing and code is made exactly right or not at all: no half mechanism (a
comment pointing at a file, a naming convention) stands in for one.

## 8. Naming

> i told you nothing should be named "work" and that's its the dumbest name possible for something
> in sosftware right? … naming must be precise and clear, really fucking prefer verbosity to
> vagueness for some bullshit sense of cleanliness. the "work" module of hyperactivity. wtf does that
> refer to would you think entering the repo?

On the names of the task graph's nodes:

> "languages" "core" "hardware" "work-object" are fucking stupid identifiers for *anything*.
>
> these carry no semantic content. it must be an english term fucking specifying what it is. not a
> loose identifier that you assume you and any reader knows what it points to, "somehow". i dont
> fucking care if your identifiers involve 12 or 20 words with dashes. this is inexcusable.

A name says what the thing is to someone entering the repository with no context. (So this file is
`task-graph-design.md`, the graph's type is `lib/task-graph.cub`, this repository's tasks are
`task-graph/hyperactivity.cub`, its tool `tools/task-graph.py`.)

## 9. docs/

> "docs/" needs a readme indicating what it's for - otherwise agents will just start piling fuckign
> docuemnts in there. this is central information organization, i.e. discretion is essentail. docs/
> README should say this is expressly *only* for writing *from the user* that must be persisted for
> onboarding agents.

`docs/README.md` says so.

## 10. The current top-level shape (2026-11-13)

The root is the product (`the-product`). Its children are Avik's four goals: the language
implementation, the paper's examples, the product implementation, and language support. Language
support waits on both the language implementation and the product implementation. The paper's
examples serve nothing but the root, so the order of work gives them nothing while anything else is
open. `hyperactive server` shows the whole graph; `hyperactive order` shows the order of work.

## 11. Mistakes that have happened, so they are not repeated

- Mechanics recorded without meaning (§1).
- A scalar cost per ticket and a sort by it (§4).
- Two node kinds ("all-of", "each-of") and a flag on each node saying whether its children
  suffice ("sufficient", "not-known"). Under the invariant there is one kind of node, and
  uncertainty is an explicit residual child, which is schedulable work rather than an invisible flag.
- A relation "speeds" between nodes, a vague stand-in for unblocking. Unblocking is dependencies.
- A catch-all top node ("everything currently pursued serves this") that unrelated work was
  attached to.
- Vague names ("work" for a module, a directory, a tool and a document).
- Agents producing an account of the project before reading its specification
  (`docs/Hyperactive_Final_Paper.pdf`), and answering with confident structure instead of reasoning.
