# The history of the work

Avik, 2026-11-15 (02:22 UTC, 16 November), asking for a complete timeline of the repository:

> ideally, this should be machine-generated/regenerable , but not bloated/redundant ie all internal relations preserved if that makes sense. how did organization move over time addition etc.
>
> actually "hyperactivity history" should enter history explorer in the product (eventually) . I would like to see how excellently we can do this inserting zero llm judgement at run-time. obviously, judiciousness/agency about depth/detail is essential i.e. we have entire papers, thousands of lines of code, showing the complete diff over time is inadmissible. the workflow system is part of it but that does not cover all action in the repo. this should be navigable, as CLI and web UI eventually.

Then, on a first proposal that grouped the history into eras by top-level folders and by session:

> deeply integrated perfectly fit to the intelligent design of hyperactivity. intelligent design is real and demonstrated precisely here. anything that is not the exact correct intelligent design is precisely incorrect.

What the design forces (derived with him, 2026-11-15). History is not a subsystem. An object is its
content, so nothing is overwritten, and the store already keeps a log of names bound to content keys
over time (src/store.c, the `names` log): that log is the history, as Unison's names over hashes are.
Git is the stand-in, read into the store by its one reader. Folders are the filesystem ontology the
system rejects and sessions hold no identity, so neither organizes the history; they are provenance
attached to changes. A change to a definition is a new key under its name; its users are new
programs with it (change is a square); a removal is an unbinding; a key a name held before is a
return; the same key under another name is a move: each is read off the log, never judged. Depth
comes from structure: the work's own nodes and the documents essential to them, a day, a change, one
object's versions; a diff only when one object is opened. Every sentence a view shows was written
when the work was done.

What exists (`hyperactive history`, tools/history.py; the web page at /history of `hyperactive
server`): every commit of every ref read into `build/history-store` in the store's form (objects
under the SHA-256 of their bytes; `name key time change`); files bound by path, each declaration of
a .cub file bound with its statement under `path#name`, each change itself an object keyed by its
content with its parents' keys. Views: the history a day at a time with every change that moved the
organization (namespaces, documents, the work's nodes and their states); a day; a change; what
changed at a name, with moves, returns, what came from another line of work, whether a version
changed code or only its statement, and what uses it; a node through time with the changes to its
essential context; Avik's words as they entered the work.

What it does not yet hold. The keys of the past are text keys. A change to a declaration's statement
alone is told apart from a change to its code's text, but not a change of code's text that keeps its
meaning (a renamed bound variable): that is the store's canonical key, which exists only where today's
machine reads a definition, not for past states. A change's reason is read from the task graph's
essential context and from the work's nodes; the machine's `#uses` chains (`hyperactive why`) are
not computed for past states. When the store holds the work natively, the reader has nothing to
read and the views read the store's own log.
