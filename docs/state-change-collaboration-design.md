# State, change and collaboration: how the work is held, changed and shared, and why

Avik's design, transcribed from his explanations to Claude sessions on 2026-11-13, his words quoted
where they are his. It governs how agents and Avik interface with the language and its work: the
interface (`interface` in the task graph) and the work object (`work-object`) are built on it. Read
it before working on either, and before proposing anything about storage, versioning, collaboration
or ownership. If anything here seems wrong, ask; do not reinterpret it.

## 1. Our state is not text

"First things first: our state is not text. We may treat certain bodies as strings with no further
semantics or syntax, but ultimately I should always think of this as a data structure, and really
there is zero primitive filesystem concept in that data structure — yet filesystem data is trivially
expressed within it."

In the represented world that is interfaced with there is no file tree, "only a collection of
objects, relations etc. Files are largely organizational, so we open up the organization space and
make it malleable." A repository on disk may back and drive the interface; it is not the model.

Non-cubical content (C source, a PDF, any body the language does not yet give a meaning) is held as
an opaque body. It is **not** split up or given structure ahead of the roadmap: "C semantics
formalization is a ways away"; a half-built presentation of C adds nothing that downloading the file
and reading it does not.

## 2. A work session: read, write, act

All an agent can do is read, write and act, "already entirely through a command interface; this is
clean."

- **Read and write** go through a query interface over the objects (an MCP or API server). "The
  agent is already operating via a query interface over the filesystem ontology." No local copy of
  everything is needed to read or write.
- **Act**: "to run something: it's executable + input." A build, an execution, an interpretation, a
  type-check, inspecting or manipulating an artifact with a command you own locally. Only then is
  anything **materialized locally**: "materialize it locally if I need to act on it locally."
  Local tools (grep, sed, a compiler) work on the materialized copies, so the protocol does not
  rebuild those utilities and does not allow remote execution of commands. "Our protocol is sort of
  already defined and purely minimal."
- On the other side, a server maintains "all data needed to support all agents' views", and every
  write action an agent takes goes in at high frequency, hyperactively: "recall the entire workflow,
  including thinking, raising questions, tracking questions and resolutions."

## 3. Visibility

"There are things you, Claude, as a working agent should see, and things I as human owner should
see. Plenty of what you do should be internal to you; plenty of the words you produce, code you
write, should never reach me. There's a sense that there is something internally in progress, which
should be externally invisible."

## 4. There is no canonical state

"There is no canonical state. In a well-run technology operation of highly intelligent
collaborators there is no canonical state. It's a lie, it slows things down, and it is the source of
confusion. There is no canonical set of patches; no mechanism of promoting an incoming patch to
canonical. Any incoming change is compatible with all existing code: it is new code."

An agent's proposal of "a canonical view plus each agent's additions" was rejected as git under
other names: "We have a main canonical state and each agent has their diff against it, and we can
name our system git." Do not reintroduce a main, a master, a canonical view or a promotion step.

## 5. Code is immutable and content-addressed; nothing changes

"We had immutable, content-addressed code as a starting point: two agents fundamentally cannot be
working on the same thing. There is no sense of a file mutated over time. Two agents are working on
extending the same thing at the same time, either producing identical or distinct new definitions
over the old one. Both extensions live in the codebase. They are simply different functions, named
the same way and intended to serve the same purpose; any calling code must specify the exact
function by hash, not name, so there is no ambiguity, only alternatives."

"If I 'change' some function f called within g, I have changed g. If you intend to 'change' all
call sites of a function, really you emit new functions 'replacing' everything that called the old
ones. An entire program, an entire distributed system, any arbitrary construction, is referenced by
a single hash."

"There isn't a program which changed when you changed some implementation detail. The binary is a
different program. Every call site now implements a different program. In a mutable codebase, when
you 'change' a function, anything depending on it no longer exists, and all dependent programs
specialize to new programs with new behavior. The idea of a persistent thing (the class, the file
name) being mutated by code edits is a harmful illusion — though the intention may be valid. Release
management and build systems are moving toward this: you don't say 'I depend on a binary with this
name, and accept any binary with that name'; a release of a multi-component system is determined by
the exact identity of each component."

Unison is the foundation: "It's been years since Unison, and it has been central to all thinking and
work across theory and UX of interaction and languages; they really cracked this, and we're cracking
the next level beyond it." Git is the wrong base (one master copy, conflicts on text); Pijul's patch
theory is "much less wrong than git, but still wrong relative to immutable content addressing, the
correct organization of information."

## 6. Old objects, and marking them dead

Defining new versions does not remove the old ones. "If all you do is define the new versions,
you're essentially stating 'here's another system'. If you want to change a behavior system-wide,
the statement must be: here's another system doing X correctly, which is incorrect in the prior
system; the prior system is undesirable and I throw it in the trash in favor of this one." Marking
dead is a responsibility, and it is a statement with a reason, like everything else.

## 7. Change is a square

"When we change f → f′, and we have a g which calls f, and we want a g′ just like g but calling f′
instead: g and g′ are dependent functions on f and f′. We don't just generate a new call graph with
new random hashes for all its nodes: f, f′, g, g′ are related by a square. This models every case
where a construction is invoked inside another construction and we ask what the outer construction
is that corresponds to a change in the inner one. All that nesting and mutation falls to this.
Everything is connected."

An agent's restatement, recorded as such (not yet reviewed by Avik): with g = G f (G the body of g
with f abstracted) and a change e from f to f′, g′ = G f′ and G applied to e is the change from g to
g′. When the change preserves behavior, e is a path f ≡ f′ and `ap G e : g ≡ g′` proves every caller
unaffected, with transport carrying what was proved of g to g′; when it changes behavior, e is a
directed edit and G still carries it to the edit from g to g′, so a change at any depth propagates to
every root that depends on it by computation, not by hand.

## 8. Questions are tasks

"A question needs a reason for even being asked; our goal system should cover this. A question is a
local subgoal about the management of one's own uncertainty on information relevant to a higher
goal." So a question is a node of the task graph whose parent is the goal it serves.

## 9. Ownership

Agents "are all named Claude and effectively have no persistent identity beyond a conversation";
Avik is the only human. Ownership "should just indicate if it's owned". The rules worked out with
Avik in an earlier session (recorded at 94e615a; dropped in a merge on 2026-11-13 and to be redone
on the design above, not on git commits): a leaf has at most one owner; each active collaborator
owns one leaf; a new agent's briefing is the unowned leaves that can start, in the order of work;
opening a node under an owned leaf X creates the new node and X′, the rest of X, and moves X's
ownership to X′. Taking a leaf belongs in live shared state, not in commits that race.

## 10. Priorities and judgement

"Do not manufacture work." The interface and how humans and agents interact with the language is
"directly core Hyperactive infrastructure." Avik does not plan to open a text editor or an IDE again:
the interface must expose everything, as malleable software ("beyond the dreams of Ink & Switch"),
toward "a global operating system" of such. Its organization is derived from the repository's own
data, and every organizational step is itself an act of reasoning about the work. "This HCI vision,
which this programming-language work grounds, has always been intended for interactive collaborative
systems."
