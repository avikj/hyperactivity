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
  on 2026-11-13. Essential context for `work-object` and `interface`.
