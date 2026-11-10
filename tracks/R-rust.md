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

(none)

## Done

- **Toolchain** (recorded): `rustc 1.97.0 (2d8144b78 2026-07-07)`, stable, LLVM 22.1.6, host
  `x86_64-unknown-linux-gnu`, in `/root/.cargo/bin`. `--emit=mir` works on stable and is what is
  used. `-Z` flags (`-Zunpretty=mir`, `-Zprint-type-sizes`, `-Zprint-mono-items`) are refused
  on stable unless `RUSTC_BOOTSTRAP=1` is set; with it they work. No `rust-src`: the MIR of
  `core`/`std` functions is not available as separate bodies, so the dump is taken with `-O`
  (MIR inlining brings the `core` bodies into the crate's own; see Limits).
- **`experiments/rust/book.rs`** (245 lines), the twin of `experiments/cxx/book.cpp`: `#[repr(u8)]
  enum Side`, `Price` with derived comparisons and `impl Sub`, `#[repr(C, packed)] WireMsg`
  decoded from `&[u8]`, `Book<const N: usize>` with `[Level; N]` and add/reduce/reset, a closure,
  `iter_mut` loops, `checked_sub` and `wrapping_add`/`wrapping_sub`, `match` on a byte and on
  enums, `trait Handler` called through `&mut dyn Handler`, a `static mut` handler,
  `#[no_mangle] pub extern "C" fn on_message(p: *const u8, n: usize) -> i64`.
- **The pair is the same function on the recorded inputs.** `experiments/rust/book_out.c` prints
  every answer; `experiments/rust/gen_messages.py COUNT SEED` writes the messages. Commands
  (from `experiments/rust`, outputs in a scratch directory `$B`):
  `rustc --edition=2021 --crate-type=staticlib -O -Cpanic=abort book.rs -o $B/libbook.a;
  clang -O2 book_out.c $B/libbook.a -o $B/book_out_rs;
  clang++ -std=c++20 -O2 -fno-exceptions -fno-rtti -c ../cxx/book.cpp -o $B/book.o;
  clang -O2 book_out.c $B/book.o -o $B/book_out_cxx;
  python3 gen_messages.py 100000 1 > $B/m1.bin;
  $B/book_out_rs < $B/m1.bin | sha256sum; $B/book_out_cxx < $B/m1.bin | sha256sum`.
  Result (clang 18.1.3, rustc 1.97.0): 100,000 messages (seed 1), the two outputs byte-identical,
  sha256 `1422f7495869b550…` (3,983 answers `-2`, the rest spreads and `-1`); with `n = 21`
  passed (`book_out_* 21`) both answer `-3` 100,000 times; 1,000,000 messages (seed 2) also
  byte-identical, sha256 `896e0075f0f2745f…`.

## Found

## Limits

- Signed subtraction of prices: `book.cpp` computes `ticks - o.ticks` on `int64_t` (undefined on
  overflow in C++), `book.rs` uses `wrapping_sub`. The binaries agree on all inputs run
  (including random 64-bit prices) because clang emits a plain `sub`; at the level of the two
  standards the C++ one is defined on fewer inputs.
- Out-of-range indices: `book.rs` has bounds checks that abort (`-Cpanic=abort`); `book.cpp`
  has none. No input reaches them (indices are below `N` by construction).

## Next step

Write `lib/lang/rust/ast.cub` (`RTy`, `RPlace`, `ROperand`, `RRvalue`, `RStmt`, `RTerm`, `RBody`,
`RCrate`; lists as own constructors, since `List D` inside `data D` and mutual data are refused:
see C-clang.md Found), check it after the library, then `tools/mir2cub.py`: run
`rustc --edition=2021 --crate-type=lib -O -Cpanic=abort --emit=mir experiments/rust/book.rs`,
parse every body reachable from `on_message`, stop on anything unhandled.
