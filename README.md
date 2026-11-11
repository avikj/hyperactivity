# What is in this directory

State as of 2026-11-10, commit `18475816`. Every file here was read line by line before this
was written, and every status below is either a load that was run on this commit or a line
that can be pointed at. This file says what exists, whether it checks, what it is for, and what
it depends on. It does not plan the work; the plan is the owner's.

The directory name is only a directory name.

## The project, in the owner's words

A cubical type theory language; whose normalization procedure and type check are terms of the
language; capable of expressing fully general computation (anything expressible in C++, as a
practical reference point); capable of interpreting any other language or computational system
(its syntax, interpreter and semantics as mathematical objects); capable of projecting
first-order terms to non-cubical languages, including machine code.

**The one rule.** No code is admissible unless it is proven correct by type check, correct by
construction and end to end. The only exceptions are a minimal trusted core and the untyped
definitions the checker itself is built from (bootstrap). No tests as evidence, no second copy
of a thing proved in place of the thing.

## Status words used below

| word | meaning |
|---|---|
| **checks** | typed; loads with 0 failures on this commit in the load named |
| **fails** | typed; loads with failures on this commit (count given) |
| **bootstrap** | untyped, loaded before the checker exists; allowed by the rule |
| **refused** | untyped, loaded after the checker; switched off by commit `18475816`. The file is intact and worked before that commit. Untyped is "has no type yet", not "wrong" |
| **trusted** | C, Python or assembly outside the language |
| **generated** | written by a tool in `tools/`; regenerate, do not edit |

## How to run

```
make build/cubical          # the machine
make library                # loads the LIB list of the Makefile: 1,711 passed, 406 failed
```

401 of the 406 are refused untyped definitions; the other 5 (`cstr-fill`, `cstr`, `cstr-at`,
`cstr-from`, `cstr-is` in `lib/cxx/ast.cub`) depend on two refused primitives.
`make library` does **not** load `lib/net.cub`, `lib/fleet.cub`, `lib/deploy.cub`,
`lib/bits.cub`, most of `lib/mem/`, `lib/isa/x86/`, or the C++ and Rust readers; their loads
are given per row below.

## Files at the top

| file | what it is |
|---|---|
| `README.md` | this map |
| `LANGUAGE.md` | the language and the machine described with examples (was `README.md` until this commit) |
| `PLAN.md` | plan revision 9 of 2026-11-09. The owner is replacing it; read it for history, not direction |
| `tracks/` | one log per parallel track (A, B, C, D, F, G, H, I, M, N, R, S), halted 2026-11-09/10. Each records what was built and measured. Some status lines there are older than this file and no longer hold (F and R say the readers check; they do not on this commit) |
| `Makefile` | builds the machine; `library` and `brunerie` targets. The `brunerie` target names the untyped port and fails since the refusal; the typed port is the one that runs |
| `archive/` | the previous machine's specification and plan (October). Old lineage, not current |
| `playground/README.md` | 13 lines referring to files that no longer exist |

## 1. The machine — `src/` (C, trusted, 16,549 lines)

| file | lines | what it is |
|---|---|---|
| `net.c` | 4,264 | the evaluator: an interaction net with sharing, memoisation by key, lazy copying, parallel workers. Every term, proof and the checker run on it |
| `lang.c` | 4,918 | core forms, compilation of terms to records, the top-level commands, the `#jit` value boundary |
| `syntax.c` | 5,753 | the surface language: parser, layout, elaborator, clause compiler to eliminators, printer; the `#jit`, `#build`, `#run` handlers |
| `store.c`, `sha256.c` | 444 | the content-addressed store: a definition's key is the hash of its elaborated type and term |
| `dnf.c` | 378 | the interval and face algebra |
| `big.c` | 142 | numerals past 64 bits |
| `drivers.c` | 153 | the loop that performs a process's commands against the real OS (`#run`) |
| `core.h`, `libcubical.h` | 497 | cell, value and operation definitions; the embedding API |

**Why it stays:** it is the trusted core everything else runs on. Checking it is the last thing
to do, by the owner's ordering.

**Known defects** (each with its place):
- A numeral is `suc` applied n times; 10^6 overflows the C stack in `ds`.
- `synkey` is called at every node by `build_m`: quadratic on deep terms.
- `net_start` does not check `pthread_create`.
- `lang.c` still has handlers for `layout`, `fo`, `native`, `native-c`, `native-at` from removed back ends.

## 2. The checker and bootstrap — `lib/` (untyped by necessity)

| file | lines | status | what it is |
|---|---|---|---|
| `prelude.cub` | 117 | bootstrap | the Kan operations per type former as terms (`%comp-pi`, `%comp-sigma`, `%comp-path`, `%comp-glue`, `%comp-u`) |
| `code.cub` | 266 | bootstrap | `Code`: terms as data (60 constructors, de Bruijn), and `eval-g`, the evaluator of codes as a term |
| `match.cub` | 384 | bootstrap | the one traversal over `Code`, matching, rewriting with a path for each step |
| `check.cub` | 2,382 | bootstrap | **the type checker as a term of the language**: `infer`, `check`, conversion, multiplicities, data and codata declarations, guardedness, and `quote` (values back to codes at a type) |
| `nav.cub`, `cost.cub`, `lin.cub` | 401 | bootstrap | choice among rewrites; cost models; linear arithmetic certificates used by the elaborator's `by-lin` |

**Why it stays:** this is the second line of the project statement. `normalize T c =
quote-top T (eval-g lnil c)` is normalization as a term.

**The structural fact to know:** `Code` is declared before the checker exists, so a typed
definition cannot mention `Code`, `eval-g` or `quote` (`code: data type not checked: Code`).
Anything written over `Code` is therefore untyped. Every checked procedure about programs in
this library works over its own typed code type instead (`LinCode`, `ETm`, `Ex`, `Pl`,
`IntCode`). The language as it stands does check an indexed code type with a checked evaluator
(tried: 32 lines, context- and type-indexed terms, `refl` proves a code's meaning).

## 3. The typed library — `lib/`

All **check** in `make library` unless a load is named.

| file | lines | what it is | depends on |
|---|---|---|---|
| `std.cub` | 578 | paths, equivalences, univalence (`ua`, `uaβ`), `Nat`, `Int`, Hedberg, `Word ≃ Nat` and `Word ≡ Nat`, machine word operations equal to their definitions, guarded recursion, streams | CORE |
| `prim.cub` | 595 | `Int64`, `UInt32`, `UInt64`, `Float64`, `Float32`, `Bytes n` with proof-carrying loads and stores; arrays; `Fin n → A ≃ Array n`. 72 primitives; the load/store and array-extensionality laws are trusted; and what the bitwise word operations compute, stated: eight laws (multiplication by recursion; and, or, xor, shifts on a word written by its bits; a word below 2^k is its k lowest bits), each a constant with no computation | std |
| `sort.cub`, `fiber.cub`, `decode.cub`, `region.cub` | 525 | sorting with its specification; fibres; a decoder with bounds discharged; regions as a state monad | std, prim, linproof |
| `linproof.cub`, `eqproof.cub`, `ring.cub`, `intproof.cub`, `autodiff.cub` | 1,298 | decision procedures proved sound over their own typed codes (linear arithmetic, equational, ring, integer; forward-mode derivative). 15 untyped helper definitions at the ends of `eqproof`, `ring`, `autodiff` are refused | std |
| `proc.cub` | 390 | signatures, processes (free monad), worlds, handlers, supervision, recording and replay, choreographies, guarded processes, transactions | std, prim |
| `world.cub` | 83 | the commands a process can issue to the real world (`WCmd`), and step trees. New commands must be appended: `elf.cub` dispatches on constructor index | proc |
| `session.cub`, `choreo.cub` | 103 | guarded session types; guarded choreographies with endpoint projection | proc |
| `obs.cub` | 747 | **behaviour of a process as a tree; bisimilarity is path equality; one program under two handlers gives equal behaviour from one obligation per command.** No untyped definition | proc, world |
| `net.cub` | 194 | a network with an adversary (drop, duplicate, reorder, partition) | proc |
| `fleet.cub` | 451 | nodes over that network; `Safe`, `Live`, `Refines`, `Converges` stated of the processes that run | net, proc |
| `deploy.cub` | 120 | wire format, codec, node configuration (definitions) | fleet |
| `word.cub` | 356 | word laws proved from those: addition on bits (ripple carry), bits that are known or a variable, and reflected word expressions (`BE`) whose word is their bits' word (`be-sound`) — how a statement about the x86 semantics' bit arithmetic is proved | std, prim |
| `bits.cub` | 312 | bit-vector operations the imported x86 semantics calls | std, prim |

Loads run: fleet + KV experiment 931 passed, 3 failed (the 3 are `eqproof`'s refused helpers);
net + fleet + `experiments/fleet/ping.cub` without `eqproof` 683 passed, 0 failed;
obs + its two experiments 683 passed, 0 failed.

**Why it stays:** this is "fully general computation" and the vocabulary every semantics below
is written in.

## 4. Semantics of other systems

| path | lines | status | what it is | depends on |
|---|---|---|---|---|
| `lib/os/` (`platform`, `linux`, `contract`, `replay`) | 864 | checks (`make library`) | **Linux as a semantics**: 23 system calls, kernel state and objects, what each call does; the handler `linux-on-w` that performs them on the real kernel; replay of a recorded run. The same process term is reasoned about under the contract and run on the kernel. Assumption: Linux behaves as the contract says | prim, proc, world |
| `lib/isa/tso.cub`, `tso-acyclic.cub` | 884 | checks | x86-TSO, operational and axiomatic, with the acyclicity theorems | proc |
| `lib/mem/atomics.cub`, `rc11.cub` | 285 | checks | C/C++ atomics and the RC11 axiomatic model. `rc11` has one statement without a proof | tso |
| `lib/mem/` (`target`, `scalar`, `sig`, `flat`, `memory`, `layout`, `monad`) | 2,185 | checks (own load, with `obs`) | **C/C++-level memory**: targets (x86-64, i386, s390x), scalar values and 21 kinds of undefined behaviour, memory as a signature of commands, an abstract memory and a flat byte memory, record layout, the evaluation monad with its laws. `mem-sound` is proved: both memories agree for every process. `target.cub` has four `#show` lines left in it | obs, prim |
| `lib/isa/x86/` | 73,906 | checks: 3,156 passed, 0 failed (with `run.cub` and `experiments/isa/add.cub`, 4 m 44 s) | generated. The x86-64 instruction semantics from the Sail model `rems-project/sail-x86-from-acl2` at `0c6c023d`; `run.cub` (21 lines, hand-written) is the step function. `add rax, rcx` with both operands as variables steps to the 64-bit sum (28,111 steps). Integer instructions have semantics; x87, most SSE/MMX, and every VEX and EVEX instruction end in a fault or "unimplemented" | bits |
| `lib/cxx/ast.cub` | 354 | checks except its 2 refused primitives and the 5 definitions that use them | generated. The C++ syntax tree as data | prim |
| `lib/cxx/machine.cub`, `read-expr.cub` | 1,845 | **fails**: 46 | the meaning of C++ expressions over `lib/mem`. Written against an earlier `lib/mem` and an earlier elaborator; not in `make library` | mem, cxx/ast |
| `lib/lang/rust/` (5 files) | 1,897 | `ast` checks; readers **fail**: 75 | Rust MIR as data and its meaning over `lib/mem`. Refers to `cx-align` in `lib/cxx/machine.cub` and to older operator names in `mem/scalar` | mem, cxx/machine |

**Why it stays:** this is "interpreting any other language or computational system". A
semantics is definitions; that a file here has no theorems is not a defect.

## 5. Machine code — the compiler (untyped, refused, worked)

| file | lines | what it is |
|---|---|---|
| `lib/specialize.cub` | 172 | normalization of a code at a type; rewriting along checked theorems, each step with its path |
| `lib/rules.cub` | 17 | a checked theorem's type read as a rewrite rule |
| `lib/machine.cub` | 130 | which codes are first-order (the fragment), and a definition unfolded into a program |
| `lib/x86.cub` | 832 | the fragment to x86-64 bytes: the instruction table, the three passes (`x-push`, `x-lift`, erasure of unused arguments), the encoder, layout, `jit-plan` |
| `lib/elf.cub` | 126 | a process as a standalone ELF binary (`#build`, `#build-loop`) |
| `tools/asm/` | 406 | assembly sources of the closure-application routine, the start stub and the command loop, and the script that turns them into byte lists |

Before the refusal this compiled closures, codata, word loops, floats and whole processes;
the log records the built feed handler matching the C program on 1M messages, and 33 `#jit`
workloads in `experiments/jit/`.

**What is already enforced by computation** (`jit-plan`, `lib/x86.cub:816–831`): the specialized
code is compared, by the checker's own conversion, with the definition's value, and a rewrite
along a theorem carries a path the checker is given.

**What is not:** everything after that comparison — the unfolding into a program, `x-push`,
`x-lift`, erasure, the encoder and its instruction table, layout, the ELF image. Nothing states
that the bytes compute what the specialized code computes. That stretch is the missing types.

**Known behavioural defect:** `x-rel-fix` (`lib/x86.cub:778`) stops after 64 rounds and returns
the set it has; arguments not yet found needed are then erased to 0.

**Why it stays:** it is the reference for every decision the typed compiler needs — value
layout, calling convention, pass order, encodings. It is deleted when the typed one covers it.

Also refused: `lib/derive.cub` (122 lines; `deriving eq` in the surface language is off with
it) and `lib/intlin.cub` (20).

## 5a. The typed compiler — `lib/compile/` (checks, in progress)

| file | lines | status | what it is |
|---|---|---|---|
| `tm.cub` | 197 | checks | source terms (`JTm`: words, booleans, naturals, pairs, sums, functions, `let`, `if`, `case`, recursion on Nat, loops on Word) at types `JTy`, and their meaning `jev` at every type, written with the language's own eliminators so a definition and its term can be shown equal by `refl` |
| `stack.cub` | 924 | checks (281, no store, ~1 min) | a stack machine of 64-bit words (`JCode`, `jrun`); values of first-order types flattened onto it (a pair its parts, a sum its tag and both payload areas); the compiler `jcx`; and `jcx-ok`: from any stack where the context's values are placed, the code ends as its continuation run on the value's words, or leaves (`jout`: a result past 2^64, or a function, which this level does not compile) |

Each instruction of the machine is one or a few of `lib/x86.cub`'s templates (push imm, push
[rsp+d], pop/op/push, a slide of the top words). Proved: every term form except `jlam`/`japp`
(those compile to `kfail`, the run leaves — never a wrong answer).

**Next:** closures (`jlam`/`japp`, with a heap); the x86 level — each instruction's bytes stepped
by `lib/isa/x86`'s semantics to the same stack (by computation, as `experiments/isa/e1.cub` does for one program;
memory byte round trips); the blob and ELF layout; then `#jit`/`#build` use it and `lib/x86.cub`'s
unchecked stretch is deleted.

## 6. Importers and tools — `tools/` (Python, trusted)

| file | lines | what it is | produces |
|---|---|---|---|
| `sail2cub.py` | 2,702 | reads the Sail x86 model | `lib/isa/x86/` |
| `clang2cub.py` | 3,774 | reads a C++ translation unit from Clang's AST, record layouts and vtables | `lib/cxx/ast.cub`, `experiments/cxx/*.cub` |
| `mir2cub.py` | 2,175 | reads Rust MIR from `rustc` | `experiments/rust/*.cub` |
| `agda2cub.py` | 287 | line-by-line translation of Cubical Agda to this surface syntax | used for the Brunerie port |
| `lsp.py` | 600 | editor server over `build/cubical --serve` | — |

What an importer writes type-checks as data. Nothing in the language connects that data to
the text it was read from; the importer is the trusted reader. `sail2cub.py` makes these
choices silently: a match's last arm loses its test, `undefined` becomes 0, recursion past a
fuel bound returns a default, Sail's constraints are not checked.

## 7. Experiments — `experiments/`

| directory | files | status | what it shows |
|---|---|---|---|
| `brunerie/` | 10 | `typed.cub` checks: −2 in 358,609 steps, 9 s. `brunerie.cub` (untyped port) refused | Brunerie's number computes |
| `os/` | 24 | tour checks: 595 passed; ran on the real kernel in 5,243 steps (mmap, UDP, epoll, futex, nanosleep) | processes against Linux and against its semantics; UDP echo with replay. `build-cost*.cub` are refused untyped probes; `item5-wip.patch` is an unapplied diff |
| `fleet/` | 11 | checks (931 / 3, above) | a replicated key-value store: safety, liveness under fairness, refinement of a sequential map, replay |
| `obs/` | 2 | checks | two implementations of a counter, and two "compilers", with equal behaviour |
| `tso/` | 8 | `sb` checks: 601 passed; 4 outcomes over 80 schedules | litmus tests decided by computation |
| `mem/` | 1 | — | `int sum(const int*, int)` given its meaning by hand, with its undefined-behaviour cases |
| `cxx/` | 20 | generated data checks; needs the readers of §4 to mean anything | seven C++ programs (order book, member pointers, virtual inheritance, exceptions, coroutines) and their imports. `sem/gen/` is a stale copy of four of them |
| `rust/` | 9 | same | two Rust programs and their MIR imports |
| `isa/` | 7 | `add.cub`, `pushpop.cub`, `e1.cub`, `flags.cub`, `anywhere.cub`, `template-xor.cub` check | one instruction stepped with symbolic operands; `push rax; pop rcx` returns every word below 2^64; a compiled program (`(x and 255) xor y`, `lib/compile/` to bytes) leaves `jev`'s value in rax for every two words below 2^64; `add; setc` leaves the carry in cl; `add; jc` leaves rip at one of two addresses chosen by the carry; and with an instruction on each side of the branch, three steps leave rip and rax as the carry decides. All by computation (refl): the evaluator computes bitwise operations, shifts, addition and comparisons on words given by their bits; a case on a computed bit computes both branches, words merging bit by bit and other values staying a conditional value that eliminations distribute over, a conditional on the same bit inside a branch taking that branch's side; a term and its negation fold (x ∧ ¬x, x ∨ ¬x); a computed bit that is the same for every value of what it is built on is that constant (its decision diagram, before a case splits on it). `anywhere.cub`: `push rax; pop rcx` with the code at any address below 2^44 and the stack anywhere in a zone above it. `template-xor.cub`: the compiler's xor template on a generic state (code and stack anywhere in their zones, any two top words, any memory below, any scratch registers and flags). Conversion compares computed bits as functions (`%bsame`: one decision diagram), so a sum computed one way equals the same sum computed another. Memory (`lib/bits.cub`) is a function from addresses to bytes, a write comparing its argument with the address written, so a read at a computed address is decided by comparing addresses and two memories that read alike are equal |
| `jit/`, `native/` | 4 | need the compiler | compiled workloads; processes built to ELF |
| `surface/` | 9 | checks | inputs for elaborator items 1–8 of track S |
| `machine/`, `agda/` | 2 | — | the 10^6 numeral; a script timing Agda on Brunerie (reads outside this directory) |

## What depends on what

```
src/  ──►  prelude, code, match, nav, cost, lin, check  ──►  std  ──►  prim
                                                                 │
        ┌──────────────┬───────────────┬─────────────┬───────────┴────────┐
      proc, world     bits          linproof…      sort, decode…       (Code, untyped)
        │               │                                               │
   ┌────┼──────┬────┐  isa/x86 ◄── sail2cub.py            specialize ─► machine ─► x86 ─► elf
  obs  os   tso   net                                                     ▲                ▲
   │         │     │                                               syntax.c #jit      tools/asm
  mem    atomics  fleet ─► deploy                                  lang.c boundary    world (cmd order)
   │      rc11
 cxx readers ◄── cxx/ast ◄── clang2cub.py
   │
 rust readers ◄── rust/ast ◄── mir2cub.py
```

## Everything that is not right, in one place

1. The compiler's last stretch has no types (§5); its typed replacement (§5a) reaches a proved
   stack machine for first-order terms, not yet closures or bytes.
2. The C++ and Rust readers do not check against the current `lib/mem` (§4).
3. `x-rel-fix` gives up silently at 64 rounds (§5).
4. Unary numerals; quadratic `synkey`; unchecked `pthread_create` (§1).
5. Switched off by the refusal and still referenced: `deriving eq`, the `brunerie` Makefile
   target, `experiments/os/build-cost*`, `experiments/jit`, `experiments/native`.
6. Stale: `playground/README.md`, `experiments/cxx/sem/gen/`, `experiments/os/item5-wip.patch`,
   four `#show` lines in `lib/mem/target.cub`, removed-back-end handlers in `lang.c`, status
   lines in `tracks/F-cxx.md` and `tracks/R-rust.md`.
7. Trusted without statement: `prim.cub`'s byte and array laws; a primitive's computation
   (stated, for the word operations: the eight laws at the end of `prim.cub`)
   against its type; the importers; `rc11`'s one unproved statement.
