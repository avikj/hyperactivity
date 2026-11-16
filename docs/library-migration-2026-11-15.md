# Library-to-repository consolidation (2026-11-15)

This is a migration inventory and reconciliation record, not a second project plan. The repository is the intended canonical information space; do not treat ChatGPT Library as an authoritative working copy.

## Completed on this branch

- Imported the complete 15 November 2026 v32 manuscript, `docs/Hyperactivity_Current_Draft.md`, from Library `/Hyperactive/Hyperactivity_Current_Draft.md` (3,994 lines). The imported text is unchanged.
- Kept the existing v31 PDFs and other repository materials intact. No Library source was deleted.

## Existing repository mechanisms to preserve

- `hyperactive` is the existing entry point and onboarding walk.
- `AI-AGENTS-WELCOME-TO-HYPERACTIVITY.md` preserves agent instructions and the author's verbatim words.
- `docs/to-be-incorporated-into-paper.md` contains material awaiting incorporation into the paper.
- `docs/Hyperactive_Labs.md` is the initiative document.
- `task-graph/hyperactivity.cub` carries task status.

## Known divergence requiring reconciliation

- `hyperactive` still sets `PAPER` to `docs/hyperactivity_v31.pdf`.
- The welcome file and incorporation notes also reference v31.
- `docs/Hyperactive_Labs.md` includes historical Library paths and statements about the earlier manuscript.
- The imported v32 manuscript explicitly contains Parts II–IV as unfinished draft sections. Do not silently replace those notes or mistake the technical Part I for the complete four-Part work.

## Next migration operations

1. Reconcile the agent onboarding and paper references to v32, retaining historical references as history rather than deleting them.
2. Inventory the Library's full `/Hyperactive` folder against this repository, with paths, content hashes, source dates, and provenance. Include research notes, author verbatim records, earlier manuscripts, and audit reports. Preserve distinct versions and never overwrite distinct content solely because names match.
3. Import missing materials in batches, with a manifest showing each Library source and repository destination. Keep derivative assistant analysis distinguishable from author-origin text.
4. Check relative links and onboarding commands, then merge only after validating the inventory and content preservation.
5. Retire Library as a working source only after verifying the repository contains the intended corpus. No deletion is implied by this migration record.

The goal is one editable, version-controlled information space accessible to coding and conversational agents. Do not introduce a parallel canonical handoff or status document.
