# Track R — Rust as data: MIR into the checker (Phase 5, first half)

Read `README.md` here (rules), `../PLAN.md` §5 and §6 Phase 5, and `tracks/C-clang.md` (the
same job for C++; keep the two ASTs alike where the languages are).

Files you own: `tools/mir2cub.py`, `lib/lang/rust/ast.cub`, `experiments/rust/**`, this file.

`rustc` and `cargo` are installed (`/root/.cargo/bin`); no network for crates, so the sample
uses `core`/`std` only. Find which MIR output this toolchain gives (`rustc --emit=mir`, or
`-Zunpretty=mir` if nightly) and record the version.

## What is built

1. **`experiments/rust/book.rs`**: the same component as `experiments/cxx/book.cpp` (track C
   writes it; pull to get it): decode a fixed wire message from `&[u8]`, an `enum Side`, a
   generic `Book<const N: usize>` with array levels and add/cancel/execute, a trait with one
   dynamically dispatched call, an iterator loop, checked and wrapping arithmetic, a `match`,
   and `#[no_mangle] pub extern "C" fn on_message(p: *const u8, n: usize) -> i64` as entry.
   It must be observably the same function as the C++ one (same inputs, same outputs): the
   pair is the acceptance input of the cross-language equality (PLAN Phase 6).
2. **`tools/mir2cub.py`**: runs rustc, reads the MIR of every function reachable from the
   entry (monomorphized instances; use `-Clink-dead-code`/`--emit=mir` with generics
   instantiated — record how you obtained monomorphic bodies; if the textual MIR is
   pre-monomorphization, say so and carry the substitutions as data), and writes one `.cub`:
   - bodies: locals with types, basic blocks, statements (`Assign` of every `Rvalue` kind met,
     `StorageLive/Dead`, …), terminators (`Goto`, `SwitchInt`, `Call`, `Return`, `Drop`,
     `Assert`, `Unreachable`, …), places with projections (deref, field, index, downcast),
     operands (copy/move/constant);
   - types structured (ints, bool, refs with mutability, raw pointers, arrays, slices, tuples,
     ADTs by id with their variants and fields, fn pointers, dyn Trait with its vtable);
   - layouts of every ADT met (size, align, field offsets, discriminant) — from
     `-Zprint-type-sizes` if available, else computed by the documented `repr(Rust)`-free
     route: mark the sample's types `#[repr(C)]` and compute, and say so under Limits.
   A statement, rvalue, terminator or type the parser does not handle stops the import naming
   it (extend the parser; never emit an "unknown").
3. **`lib/lang/rust/ast.cub`**: the MIR as plain first-order `data` (`RTy`, `RPlace`,
   `ROperand`, `RRvalue`, `RStmt`, `RTerm`, `RBody`, `RCrate`), constructor names prefixed `r`.

## Acceptance

- `python3 tools/mir2cub.py experiments/rust/book.rs -o experiments/rust/book.cub` runs; the
  library, `lib/lang/rust/ast.cub`, then `book.cub` load with `N passed, 0 failed` and
  `book-crate : RCrate` checked. Record: statement/rvalue/terminator kinds met, bodies, blocks,
  steps and seconds.
- `book.rs` and `book.cpp` built natively (rustc, clang) give the same outputs on the same
  10⁵ random messages (this is a run of the two reference binaries, recorded; it is the
  ground for calling them the same component).

## Needs from track A
## Done
## Found
## Limits

## Next step

Record `rustc --version` and which MIR flags work; write `book.rs` once track C's `book.cpp`
is in the repo (pull; it is at `experiments/cxx/book.cpp`); commit.
