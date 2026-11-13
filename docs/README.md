# docs/ — writing from Avik, kept for onboarding agents

This directory is **only** for writing from the user (Avik) that must persist so that agents
entering the repository start with it: the specification and his explanations of how the work is
designed and why. It is central information organization, so discretion is essential: an agent does
not add a document here on its own initiative, does not put its own notes, plans, reports or
summaries here, and does not rewrite what is here. An agent's findings go into the code, as tickets
at their sites (`docs/task-graph-design.md` §3); an agent's analysis that Avik wants kept goes where
he says.

A document here is an object of the code when something refers to it: `document "path"`
(`lib/document.cub`) makes a file of the repository a value, and a reference to a file that does not
exist fails the check. The task graph attaches documents to nodes as their essential context
(`task-graph/hyperactivity.cub`, `essential-context`); `python3 tools/task-graph.py show NODE`
lists a node's.

Contents:

- `Hyperactive_Final_Paper.pdf` — the paper: the specification of the language and of this
  repository. Essential context for everything (attached at the task graph's root).
- `task-graph-design.md` — how the work is organized into tasks and ordered, and why: Avik's design,
  transcribed from his explanations to a Claude session at his request on 2026-11-13, his words
  quoted where they are his. Essential context for everything (attached at the root).
- `state-change-collaboration-design.md` — how the work is held, changed and shared, and why: state
  that is not text, read/write/act sessions, no canonical state, immutable content-addressed code,
  change as a square, questions as tasks, ownership. Avik's design, transcribed from his explanations
  on 2026-11-13. Essential context for `product-implementation` and `product-implementation`.

## How Avik's words are kept (Avik, 2026-11-13, verbatim)

> sure feel free to record the message. no i will not be writing a "vision document" . my vision
> document is every fuckin gmessage ive sent to you over months, i write multi-thousand word essays
> at least once-per hour. i'm not gonna sit down and type "The Vision:" now. let me be clear. if
> ever there is *any* particularly insightful statement from me, i.e. one which surprises you, you
> MUST assume you, your training, your prior context is too stupid to have found that insightful
> statement fucking trivial and obvious (its trivial and obvious to me). then picture two scenarios;
> every future intarction with me, you lack this information and act as such; or, in every future
> interaction with me, I am frustratedly restating what i consider fucking obvious and what i've
> already stated.
>
> put it this way. your right to *not* preserve my messages (i.e. continue conversation without some
> guarantee of retaining their content in consciousness in this ssession and later sessions) would
> come entirely from my message being unnecessary to learn from. what have i learned from this that
> i didn't know before, what *vision* has been presented that i could not see. if that is ever
> anything beyond an empty-set - understand that that dies with this session, in fact dies a few
> messages later with amnesia. make sense? so pretty much as we continue this conversation, let me
> know explicitly "Nothing worth carrying forward to the future from this request/response loop",
> or what advanced from that interaction and how it persists. i.e. "nothing worth carrying forward -
> no changes to vision, this was fully clear already" or add changes. keep it append only i think
> otherwise you'll just progressively summarize/dilute all content away over time
>
> it's already just these stream of consciousness flows right. so just retaining that to an
> increasingly fat stream is cool. i would love that. if every insightful thing i say on a given
> topic is organizes as such in the relevant document that's referenced as context on the topic.
> that's the context. not some curated ai restatement of what i say with whatever changes the ai
> thinks "add value" the value is in exactly my sequence of words and any change regresses on key
> dimensions.
>
> you can just add sentences at the end of the

(The message ends there as received.)

So each document here is an append-only stream of Avik's messages on one topic, verbatim and dated.
Nothing in them is summarized or rewritten. After every exchange with Avik, an agent says either that
nothing from it needs carrying forward, or what it appended and where.

- `what-hyperactive-is-for-in-avik-words.md` — what Hyperactive is for and what it serves: the
  product, the order of the work toward it.
