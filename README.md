# Hyperactive — status now (2026-11-12)

This directory implements the language specified by `docs/Hyperactive_Final_Paper.pdf` (the
specification; read it in full before changing anything — `tracks/PAPER-READING.md` is a
section-by-section account of it, with what exists here of each construction). The directory name
is only a directory name.

## What is happening now

The immediate goal is that anyone arriving with no context finds a repository that states its own
reality (goal `self-aware`). That means: every file read against the paper and the published
calculus, every finding recorded as a ticket at the place it concerns, and this document saying
only what is true. The work's goals and tickets are code (below). The priority after that is the
language core: the implementation exactly the published calculus with a minimal trusted part
(`core-correct`), then partial evaluation with the Futamura projections (`partial-evaluation`,
`typed-codes`), and realization as native code through C (`native`).

**Done (2026-11-11/12, one Claude session):** the paper read in full; every file of `src/`,
`lib/`, `experiments/`, `tools/` and the track documents read for content; the findings recorded
as tickets t1–t37 at their sites; the measurements below.
**Not done:** the rule-by-rule reading of the evaluator, the composition clauses, the generated
per-data composition code and the checker against the published rules (t33, t23, t22, t35), and
of the translation of terms into the evaluator's graph (t34). Until that is done, whether the
implementation is exactly the calculus is not known.

**Typed codes and normalization by evaluation (2026-11-12, `lib/nbe.cub`, goal `typed-codes`):**
checked for the fragment Π, Σ, Bool with dependent case: typing derivations over the one code
representation, indexed by meaning (paper §6.1, Code(Γ, A) = Σ c. Der); Kripke computability over
renamings; reify/reflect; the fundamental lemma; normalize; reifySound (6.5), which holds by the
indices. `experiments/nbe/examples.cub` runs it (a β-redex to `true`, a variable `f` to `λ x. f x`).
Not yet built: PathP and the interval, Kan operations, Glue, universes (plan at the head of the
file), and its use by the specializer (t11). Findings from it: t46, t47, t48.

## The work, as code

- `lib/work.cub` — what a ticket and a goal are (general).
- `work/goals.cub` — the goals; each goal's statement is the comment above its `serves` clause.
- `work/tickets.cub` — one checked record per ticket: its kind (`wrong`, `maybe-wrong`,
  `needs-work`, `question`), the goals it serves, `opened` or `(closed #YYYYMMDD)`.
- A ticket's text is the one comment `tN (YYYY-MM-DD): ...` at the place in the code it concerns,
  so a reader of that place sees it without knowing the ticket exists.

```
python3 tools/work.py list            # open tickets: kind, goals, place, first line
python3 tools/work.py show t6 t7      # full text and place
python3 tools/work.py goals           # the goals as a tree, with ticket counts
python3 tools/work.py check           # records and comments correspond; the work files check
python3 tools/work.py next            # the next free ticket number
```

The rule: whenever anything in the code is seen to be wrong, possibly wrong, owed work, or not
known, it becomes a ticket at once, its dated text at the site. Closing one: fix it, delete the
comment, set the record `(closed #date)`, in one commit.

## Status of knowledge (measured 2026-11-12 unless dated)

- `make library` (the `LIB` list): 1,907 passed, 52 failed, 64 s. The 52 are untyped definitions
  refused by the checker (t16–t21), which refuses untyped code outside the bootstrap files since
  2026-11-09, deliberately.
- `make brunerie`: Brunerie's number −2 with 0 and 4 workers (the typed port; 6.6 s alone).
- `make work`: 233 passed, 0 failed.
- The memory description files (`lib/mem/` other than `atomics`, `rc11`), loaded after `LIB`:
  385 definitions, 0 failures. Other files outside `LIB` are checked by no target (t30).
- `experiments/jit/workloads.cub` after `LIB`: all 33 `#jit` workloads compile and bind (`ok: jit`);
  the only failures are the library's 52.

**What is proved** (checked definitions): univalence's computation rule; Hedberg's theorem;
semiring laws on ℕ; machine words equivalent to ℕ, and the machine's addition, subtraction,
equality, order and maximum equal to their inductive definitions; soundness of decision procedures
for linear arithmetic, equality, integers and ring normalization; forward differentiation; sorting
out of a higher inductive bag; handler identity and composition laws, naturality of processes,
bisimulation as path equality, a quotient of behaviours; the abstract and flat memories agree for
every process; a replicated key-value store over a network with loss, duplication and partition
is safe under every adversary, refines a sequential map, is live under every fair schedule and
converges.

**What is trusted:** the C evaluator (`src/`, with t1–t5 against it); the composition clauses
(`lib/prelude.cub`, t23) and the composition code generated per data type (t22); the checker, an
untyped term (t24) and its services exposed to typed code (t25); the primitives' computations and
their postulated laws (t32); the x86 translator and its boundary, whose output is bound without a
relation to the definition (t6–t9); the Python importers' faithfulness (`tools/`); the kernel
contract (`lib/os/contract.cub`).

**Not known:** conformance of the evaluator and checker to the published rules (t33–t35); the
cost and stack defects recorded on 2026-11-09 (t4, t5); specialization cost through handlers (t12).

## What the repository contains, by kind

1. **The formal language** — the paper's subject: CCHM with the clocked extension.
2. **The trusted evaluator** — `src/` (C): the interaction-net machine, the surface front end and
   elaborator (`syntax.c`), the translation of terms into the machine's graph (`lang.c`), the
   store, the operating-system driver.
3. **Procedures on the language, written in the language** — terms as data (`lib/code.cub`), the
   checker (`lib/check.cub`), matching and rewriting (`lib/match.cub`, `lib/rules.cub`), the
   specializer (`lib/specialize.cub`), the x86 translator (`lib/machine.cub`, `lib/x86.cub`,
   `lib/elf.cub`). The first files of `CORE` are untyped by necessity (bootstrap).
4. **General definitions** — `lib/std.cub`, `lib/prim.cub`, processes and handlers
   (`lib/proc.cub`, `lib/obs.cub`, `lib/world.cub`), decision procedures, `lib/work.cub`.
5. **Descriptions of particular computations** — networks and fleets (`lib/net.cub`,
   `lib/fleet.cub`, `lib/deploy.cub`), Linux (`lib/os/`), x86 (`lib/isa/`, generated), memory
   models (`lib/mem/`), C++ and Rust (`lib/cxx/`, `lib/lang/rust/`), and `experiments/`.
6. **Translators from outside artifacts** — `tools/` (Sail, Clang, MIR, Agda importers; editor
   server; `work.py`).
7. **Text** — this file, `LANGUAGE.md` (the language reference), `tracks/PAPER-READING.md`;
   `archive/` is history (earlier plans and track logs; nothing there is in force).
   `python3 tools/tree.py` prints an index of every file and its top-level constructs.

## How to run

```
make build/cubical      # the machine
make library            # the LIB list
make brunerie           # Brunerie's number
make work               # the goals and tickets
```

Every reading prints its steps; `CUBICAL_NOCACHE=1` checks without the module cache.

## Working rules

- The specification is the paper; where the published calculus (CCHM, the clocked extension)
  fixes a rule, it governs.
- Nothing is added to the evaluator or the checker to make something go through. If the normalizer
  does not identify two terms, the answer is a proof, not a new rule.
- Every definition has a type; every transformation's result carries its correctness; no tests as
  evidence; no second model of a thing proved in place of the thing.
- Findings go into the code as tickets at once. Comments added are dated.
- Commit in small pieces with the trailers `Co-Authored-By: Claude Opus 5.5
  <noreply@anthropic.com>` and the session line.
