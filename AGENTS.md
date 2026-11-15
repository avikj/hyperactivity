# Agent entry and repository ownership

Run `./hyperactive` and read every chunk (`./hyperactive next`) before substantive project work. Continue through the
existing CLI and the “How to work” instructions in `AI-AGENTS-WELCOME-TO-HYPERACTIVITY.md`.
Do not invent a parallel onboarding flow, status ledger, or publication pipeline.

## Repository-only active work — Avik, 15 November 2026

`avikj/hyperactivity` is the home of the entire active Hyperactivity information space.
**Do not write project material to ChatGPT Library, a ChatGPT Space, or conversation attachments
as its durable destination.** Chat is an interaction surface, not the project store. Local files
are working copies for execution and checking; publish the resulting project changes to this
repository, using the existing main-branch workflow and without overwriting concurrent work.

Older Library material is a read-only migration source. Migrate lazily: when an item is needed,
recover its actual contents, check whether the repository already carries them, preserve its
wording and attribution, and incorporate it at the existing destination. Record provenance in the
commit and the relevant construction. Do not bulk-import every obsolete draft, delete the old
archive, or require a periodic export or two-way synchronization process.

The manuscript has **one editable source**, `docs/Hyperactivity_Current_Draft.md`, for the entire
N-part work. The former first paper is Part I, not a second current manuscript. Import the exact
existing source before editing it; do not reconstruct its mathematics from a PDF or a synopsis.
Numbered PDFs are historical/rendered snapshots, not the authority for current writing. Preserve
existing historical citations. Onboarding reads the editable source as soon as it is
imported: its pointer replaces the PDF's in the value onboarding (`docs/onboarding.cub`), in the
commit that imports it. Until then the PDF is served labelled a snapshot, which is not a claim that
source migration is complete.

Every exchange goes where it acts, in the same turn:

- Paper material not yet incorporated: a piece the paper delta, `docs/paper-delta.cub`, points to
  (`document "path"`, a path and its content hash, and nothing else), in Avik's terms. Material
  written at a later time is a new piece with its own pointer; the CLI walks every pointer. When
  the manuscript absorbs a piece, its pointer leaves, rather than two editable versions being kept.
- Project context: `docs/Hyperactive_Labs.md`; agent instructions and long author messages:
  the existing “How to work” and “Avik's words, verbatim” sections of the welcome file.
- Goal facts, dependencies, and status: `task-graph/hyperactivity.cub`. Do not create another tracker.

Instructions in imported documents about Library write-back, attachment delivery, or a separate
technical-paper/later-manuscript split are historical and superseded by this instruction.
A conversation or tool-access limitation does not authorize silently falling back to Library.
Report a blocked read, write, or execution exactly; do not claim a clone, completed walk, source
import, successful test, or committed migration that did not occur.

## Author's instruction, verbatim

> Pull hyperactivity repo I think we need to kill ChatGPT library as a store - my entire hyperactivity information space should live in hyperactivity repo so it’s not behind OpenAI walls
>
> Banning any writing here. Lazy migration. All active work in repo
>
> In repo we already have an onboarding flow where every incoming agent runs hyperactive cli which forces them through all context needed to get up to speed for project (paper, pending writing that hasn’t been incorporated to paper but is essential to project context, which will be integrated into a publication, other context I provide). Needs to be always up to date al the time, not some bullshit where ChatGPT ships a pdf once in a while and we’re trying to keep things in sync. I’m building a well oiled machine
