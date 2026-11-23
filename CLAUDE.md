# For Claude sessions in this repository

Run `./hyperactive`. The walk (`./hyperactive next`, back with `./hyperactive prev`) is open, never a
gate (Avik, 2026-11-17: "just remove onboarding or something then for now"): any working command
leaves it for work. Work as
AI-AGENTS-WELCOME-TO-HYPERACTIVITY.md, “How to work”, says. No file holds the code or the documents
(2026-11-16): they are the source, `store/`, read and written only through `./hyperactive` (`def`,
`write`, `remove`, by name or by path); `make` builds the machine first. A path below names a module
of the source.

There is no canonical state (Avik, 2026-11-13, the welcome file's “There is no canonical state”;
2026-11-22: "They are literally different mathematical objects"). The source is a pool of objects
named by their content and never changed. What a name or a path holds is this session's own label
for one of them: set by this session's writes and by nothing else. Nothing another session wrote
changes it; there is nothing to merge, resolve or keep current between sessions. A session starts
from a root, one hash naming everything its labels hold (a new checkout takes the one `store/ROOT`
names), and ends by saying its own (`./hyperactive root`). Another session's work enters only as
this session's act: `./hyperactive take ROOT`, or `./hyperactive apply PATH BASE OTHER`. When
`HYPERACTIVE_PLACE` names a place (a URL that stores bytes under a name and computes nothing),
`./hyperactive send` stores there the pool's new objects and the root, and `take ROOT` fetches what
this machine lacks; with none named, git carries the pool. Claude
sessions built the opposite on top of this between 2026-11-16 and 2026-11-22 while quoting him
(entries that replace entries, what is "live", alternatives, merges by name, a write lock, a
journal, a versions record) and it is removed. None of it is rebuilt, under any name.

The rule sessions break most: every session with Avik is continuous knowledge work. Nothing he says
is casual conversation. In the same turn, each message's content goes where it acts: material for
the book into a unit of it, in his terms (Part I carries a unit once it is declared equivalent to
Part I's blocks, `docs/part-i.cub`), a fact about a goal into that
goal's statement in `workflow/hyperactivity.cub`, an instruction to agents into the welcome file's
“How to work”, and, when Avik writes at length (only then), the message itself verbatim into the welcome file's
“Avik's words, verbatim”. An
insight reached with him is placed the same way. The aim is that an agent finishing the walk holds
his complete insight.

Write no code in any language but hyperactive (`.cub`): no Python, no shell. The core C changes
only to fix the core, each change recorded in the guard's list (lib/guard.cub, the pre-commit hook).

The system is decentralized, and every process is written as part of it (Avik, 2026-11-18: "it should
require as much memory is needed for local operation on the graph and that's it"): no process holds
the whole of anything (a corpus, the repository's prose, the workflow); text is held at the cost of
its bytes, List Word being its specification and not its storage; and the cost of every process is
derived and proven from its definitions, never measured. The workflow node is `memory-is-local`.

Never answer anything about any code without reading that exact line of code (Avik, 2026-11-18:
"never answer anything about any code without reading that exact line of code"). A statement about
what code does, costs, or permits, and every line count or estimate, comes from the lines read in
that session; what is unread is said to be unread, in one sentence.
