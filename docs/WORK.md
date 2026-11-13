# The work: how it is organized, and why

Written 2026-11-13 from Avik's explanations to a Claude session, in his words where quoted. Avik is
the source of every decision in this repository and in the paper; agents are used for throughput.
Read this before touching `work/graph.cub`, `lib/work.cub` or `tools/work.py`, and before deciding
what to work on. If anything here is unclear or seems wrong, ask; do not reinterpret it.

## 1. Why this document exists

Every agent that enters this repository arrives knowing nothing, and until now each one had to be
taught the same things in conversation, at length, by Avik. That knowledge was then lost at the next
context reset or hand-off, and the next agent made the same mistakes. A previous session recorded
the *mechanics* of the system below (a data type, some flags, a script) but not its *meaning*, so
the mechanics sat among large volumes of generated code with no sign that thought went into them,
and the next agent misread them. In Avik's words: "for things that we humans take time to define, it
needs to be fully clear exactly the rationale (that was intelligence work product passed to an agent
and discarded)… if you enter this repo tomorrow, nothing we've just been over should be raised… an
agent should bootstrap just as they read the code; I shouldn't be forced to feed each one."

The goal graph is **not** the place for ideas like these. It records concrete consequences (what
must be done, and why each piece exists). Ideas, rationale and vision live in English documents
like this one, which hold value on their own for an agent whose experience is ordinary software
work and who is trying to infer the project's norms from its code.

## 2. What the work graph is for

This is not a project-management tool bolted onto a programming language. The language is meant to
be an information and computing environment with maximal bandwidth for continuous flows of
understanding and contribution, with the friction to cognition brought to zero — Avik's driver for a
decade. The work itself (the code, its history, what is known, what is open, what everyone is
doing) is one object in the language (`work-object` in `work/graph.cub`), and the graph of goals is
the first part of that object built. Soon agents and people will not interact with this work by
grepping and reading files: they will declare and read views, terms of the language evaluated by
the machine over the work. Text operations on the work are a stage to be left, not the method.

The HCI vision itself (the interactive product that is the language's delivered value) is to be
written by Avik and installed here; until it is, the `interface` node's statement and this section
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

A node's value is exactly what it serves: its ancestors, up to the root. Nothing else. In Avik's
words: "there is zero scalar sense of how much value… 'how much value' is simply what does it serve.
There is no point in the rational process where a scalar quantification of value determines a
prioritization. There is no way to construct a rational system as such. Its value is entirely its
reason for existence. Its reason for existence is fully specified, unless it's top-level, which
means 'Avik wants this' — it doesn't matter to Claude how badly Avik wants it. Claude's job is to see
what Avik has declared he wants and get it all done."

A previous session added a "cost" number to each ticket (a count of tickets on its cheapest path to
the root) and sorted by it. That was wrong and has been removed. Do not reintroduce any score,
weight, rank or magnitude.

## 5. Order of work: dependencies, then supersession

"Prioritization determines only what must be done before what else — many sequential topological
sorts; beyond that, there is a sense of what can be done before what else. That rational choice is
determined entirely by the reasons for each (what value it unblocks)." The order of work is chosen
to maximize throughput: do the work that unblocks work.

**Dependencies.** Some work cannot start until other work is resolved (`after` in the graph: a node
waits for other nodes; waiting for a node means waiting for every leaf under it). Every valid order
is a topological sort of these dependencies.

**Enabling is conjunctive.** A piece of work becomes possible only when *all* its prerequisites are
resolved. If Z needs both X and Y, finishing Y alone enables nothing; finishing X and Y together
enables Z. What a step enables therefore depends on what is already done; it is not a fixed
property of the step.

**Supersession.** Among pieces of work that could be done in either order: "if X and Y can be done in
any order, and both are dependencies of Z, but X additionally unblocks other goals, then X should be
done first. 'X then Y' and 'Y then X' both unblock Z at the same time (assuming sequential and
roughly fixed time on each task); the only difference in the continuations is what else either may
unblock. As soon as X is done, N other things can be started, in parallel with Y." So X supersedes Y
when everything that becomes possible by finishing Y first also becomes possible by finishing X
first, and more does. Where neither contains the other, the graph expresses no preference. This is
the whole of prioritization.

## 6. Reasons for existence, and placing a node

Everything in the code is a node of the same graph: "every top-level object in the code is used
somewhere or states its purpose for being in the codebase: 'what justifies you being here'." Where
use is implicit (something with a reason uses it) nothing is written; otherwise the reason is
stated, `(part of NODE)` in the comment above it. A thing with no reason has no justification
reachable from the root.

Avik's direction for the graph (2026-11-12), every point as given:

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

## 7. The current top-level shape (2026-11-13)

Under the root: `publish`, the languages, hardware and OS goals, and the work object.
`publish` has two children. `initial-deliverable` (the companion repository with the
implementation complete: every essential construction of the paper, worked examples excluded) is
the top priority: it contains `core`, the language completion that every other goal's value also
waits on. `full-coverage` (every construction the paper displays, worked examples included) comes
last; it serves nothing but publication.

## 8. Mistakes that have happened, so they are not repeated

- Mechanics recorded without meaning (§1).
- A scalar cost per ticket and a sort by it (§4).
- Two node kinds ("all-of", "each-of") and a flag on each node saying whether its children
  suffice ("sufficient", "not-known"). Under the invariant there is one kind of node, and
  uncertainty is an explicit residual child, which is schedulable work rather than an invisible flag.
- A relation "speeds" between nodes, a vague stand-in for unblocking. Unblocking is dependencies.
- A catch-all top node ("everything currently pursued serves this") that unrelated work was
  attached to.
- Agents producing an account of the project before reading its specification
  (`docs/Hyperactive_Final_Paper.pdf`), and answering with confident structure instead of reasoning.
