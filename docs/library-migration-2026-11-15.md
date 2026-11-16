# Library-to-repository consolidation (2026-11-15)

This is a migration inventory and reconciliation record, not a second project plan. The repository is the intended canonical information space; do not treat ChatGPT Library as an authoritative working copy.

## Completed on this branch

- Imported the complete 15 November 2026 v32 manuscript, `docs/Hyperactivity_Current_Draft.md`, from Library `/Hyperactive/Hyperactivity_Current_Draft.md` (3,994 lines). The imported text is unchanged.
- Kept the existing v31 PDFs and other repository materials intact. No Library source was deleted.
- One source (Avik, 2026-11-15: "ONE SOURCE OF INFORMATION"; "Nothing is ever allowed to be stale"): the v32 manuscript became one value of the language, `hyperactivity` (`docs/hyperactivity.cub` over `lib/paper.cub`), whose human projection reproduces it (runs of blank lines collapsed to one, a blank line after each heading). The Markdown copies (`docs/Hyperactivity_Current_Draft.md`, `docs/hyperactivity_v31.md`), the v31 and 11 November PDFs and the dense form left the tree; git history keeps them. The Library copy of the Labs note was merged into `docs/Hyperactive_Labs.md` (the repository copy's U050 section, the 101 series, retained) and its copy left the tree.

## Existing repository mechanisms to preserve

- `hyperactive` is the existing entry point and onboarding walk.
- `AI-AGENTS-WELCOME-TO-HYPERACTIVITY.md` preserves agent instructions and the author's verbatim words.
- `docs/to-be-incorporated-into-paper.md` contains material awaiting incorporation into the paper.
- `docs/Hyperactive_Labs.md` is the initiative document.
- `task-graph/hyperactivity.cub` carries task status.

## Known divergence requiring reconciliation

- The v32 manuscript explicitly contains Parts II–IV as unfinished draft sections. Do not silently replace those notes or mistake the technical Part I for the complete four-Part work.
- `docs/Hyperactive_Labs.md` includes historical Library paths (Research/, Corpus_2026-11-14/) whose files are not in the repository.

## Next migration operations

1. Inventory the Library's full `/Hyperactive` folder against this repository, with paths, content hashes, source dates, and provenance. Include research notes, author verbatim records, earlier manuscripts, and audit reports. Preserve distinct versions and never overwrite distinct content solely because names match.
2. Import missing materials in batches, with a manifest showing each Library source and repository destination. Keep derivative assistant analysis distinguishable from author-origin text.
3. Check relative links and onboarding commands, then merge only after validating the inventory and content preservation.
4. Retire Library as a working source only after verifying the repository contains the intended corpus. No deletion is implied by this migration record.

The goal is one editable, version-controlled information space accessible to coding and conversational agents. Do not introduce a parallel canonical handoff or status document.
