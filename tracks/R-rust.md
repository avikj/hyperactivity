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

- **`lib/lang/rust/ast.cub`** (first version): `RTy` (with its own list constructors), `RProj`,
  `RPlace`, `RConst`, `ROperand`, `RBinOp`, `RUnOp`, `RCastKind`, `RAggregate`, `RRvalue`, `RStmt`,
  `RUnwind`, `RCallee`, `RAssertMsg`, `RTerm`, `RBlock`, `RLocal`, `RBody`, `RAdt`, `RLayout` (fields,
  variants, tag), `RTrait`, `RVtable`, `RStatic`, `RInstance`, `RCrate`. Loads after the cached
  library in 2.2 s, every `data` checked (`data RCrate` 71,652 steps); a trial `RBody` value checks.

- **`tools/mir2cub.py`** (1,996 lines) and **`experiments/rust/book.cub`** (generated; 1,195 lines,
  108,457 bytes, 346 definitions). `python3 tools/mir2cub.py experiments/rust/book.rs -o
  experiments/rust/book.cub` runs in about 1 s and prints what it met. How each part is obtained
  (also in the tool's header):
  - bodies: `rustc --edition=2021 --crate-type=lib -O -Cpanic=abort --emit=mir`. The MIR is
    **pre-monomorphization**; bodies keep their generic parameters (`rty-param`,
    `rty-const-param`, `rc-param` by position, `RBody.generics`), calls and ADT types carry their
    generic arguments, and `RCrate.instances` lists every (body, closed arguments) reached from
    the entry by walking calls and vtables.
  - impls: read from the source text at the span rustc names the impl by (generics, trait, self
    type; a derive's span is the trait name and its type is the next item); a call is connected
    to its body by unifying the impl's self type with the call's.
  - layouts: a copy of the source with one `static` per needed type appended is compiled with
    `-g -Ccodegen-units=1 -Csymbol-mangling-version=v0 --emit=obj`; `llvm-dwarfdump --debug-info`
    gives size, alignment, fields (name, offset, type) in declaration order = MIR field index,
    and variants with tag place/size and tag values. (Functions were tried first for forcing
    the descriptions: LLVM merged the identical ones and only one kept its description.)
  - vtables: the object file's `.data.rel.ro` section whose relocations demangle
    (`llvm-cxxfilt`) to `<T as Trait>::method`: size and align words from `objdump -s`, methods
    in slot order from `objdump -r`; that order is also the trait's method list (`RTrait`),
    which `rcallee-dyn trait method` indexes.
  - statics: the evaluated allocation rustc prints after the bodies (init bytes and padding).
  **Measured on book.rs** (acceptance line 1):
  - 10 bodies (`on_message`, `decode`, `dispatch`, `<BookHandler as Handler>::on_add/on_cancel/
    on_execute/on_reset`, `Book<N>::add/reduce/reset`), 10 instances (the three `Book` bodies at
    `N = 8`), 157 blocks, 707 statements, 280 locals, 52 distinct types, 11 ADTs (`WireMsg`,
    `Option`, `BookHandler`, `Side`, `Price`, `Book`, `Level`, `IterMut`, `NonNull`, `PhantomData`,
    `Ordering`), 13 layouts, 1 trait, 1 vtable, 1 static (424 bytes).
  - The same counts taken from the MIR text by line (blocks and `;` lines of the ten bodies):
    10 / 157 / 707. `-Zprint-mono-items` lists exactly the same ten functions plus the static
    `G_HANDLER`. `-Zprint-type-sizes` gives the same size and alignment for all 13 types.
  - Check (library from the cache, then `lib/lang/rust/ast.cub`, then `book.cub`):
    `1155 passed, 0 failed`, `book-crate : RCrate` checked (7,104 steps). Fresh (no cached result
    for the two files): 2,489,691 interaction steps in all, of which `ast.cub` is 909,818 and
    `book.cub` about 1,580,000 (largest single definition 50,706); 3.3 s wall for both against
    1.1 s for the cached library alone.
  - Kinds met in the ten bodies (count of occurrences):
    - statements: `Assign` 283, `StorageLive` 184, `StorageDead` 240.
    - rvalues: `Use` 126, `BinaryOp` 82, `Cast` 28, `Ref` shared 5 / mut 18, `RawPtr` (`&raw mut`) 2,
      `Aggregate` 15, `Discriminant` 5, `UnaryOp` 2.
    - binary operators: `Add` 12, `Sub` 8, `SubUnchecked` 1, `Eq` 4, `Lt` 47, `Ge` 1, `Gt` 3,
      `Cmp` 2 (three-way, result `Ordering`), `Offset` 4. Unary: `Not` 1, `PtrMetadata` 1.
    - casts: `IntToInt` 4, `Transmute` 19, `PtrToPtr` 2, `PointerCoercion(Unsize)` 3 (array to
      slice twice, `&mut BookHandler` to `&mut dyn Handler` once).
    - aggregates: `Adt` 11 (struct and enum variant), `Array` 3, `RawPtr` 1 (`*const [u8]` from
      address and length).
    - terminators: `Goto` 64, `SwitchInt` 32, `Return` 10, `Unreachable` 3, `Call` 6 (to a body) +
      4 (through `dyn Handler`), `Assert` 38 (all `BoundsCheck`). Every unwind action is
      `unreachable` (`-Cpanic=abort`).
    - operands: `Copy` 253, `Move` 157, `Constant` 133. Constants: integer 86, bool 4, generic
      constant parameter (`N`) 38, ADT value 5 (`Price {{ ticks: 0 }}`, `Side::Bid`), zero-sized
      value 2 (`PhantomData`), address of a static 1.
    - place projections: `Deref` 110, `Field` 99, `Index` 16, `ConstantIndex` 28, `Downcast` 3.
    - types: `bool`, `i8 i64 isize u8 u32 u64 usize`, `&T`/`&mut T`, `*const T`/`*mut T`, `[T; n]`
      (n a literal or the parameter `N`), `[T]`, `()`, ADTs with arguments, `dyn Handler`.
    - not met in book.rs (rustc inlined or optimized them away, or the source has none): `Drop`,
      `Repeat`, non-unit tuples, function pointers, `SetDiscriminant`, `Len`, `CopyForDeref`,
      checked arithmetic (`*WithOverflow`), the closure (inlined into `add`), `Option<u64>`
      from `checked_sub` (folded to `Lt` + `SubUnchecked`).

## Found

- No fault of the machine found so far: every generated file loaded on the first attempt
  (largest: 108 KB, 346 definitions, 3.3 s). The output is deliberately many small definitions
  (one per type, name, block, layout); one large literal was never tried.
- (design facts measured before writing the importer, all on rustc 1.97.0)
  - `--emit=mir` prints **pre-monomorphization** MIR: `Book<N>::add` once, with `const N` and
    `[Level; N]` in it; callers name the instance in the call (`Book::<8>::add(..)`).
  - With `-O` the MIR inliner brings every `core` function the sample uses into the crate's own
    bodies (`slice::IterMut::next`, `from_raw_parts`, `checked_sub`, `from_le_bytes`, the derived
    comparisons): the calls left in the 2,680-line dump are all to bodies of the crate or
    through `dyn Handler`. Without `-O` they are calls to `core` bodies rustc does not print.
  - Bodies in impls are named by span (`<impl at book.rs:63:1: 63:29>::add`), calls by path
    (`Book::<8>::add`, `<dyn Handler as Handler>::on_add`); the importer reads the impl header at
    the span from the source to connect them.
  - `-g` debug information (read with `llvm-dwarfdump`) has, for every monomorphic type it
    describes, size, alignment, field names/types/offsets, enumerator values, and for data
    enums the tag's place and each variant's tag value (the untagged variant of a niche
    encoding has none). `-Zprint-type-sizes` lacks tag values and field types, so the
    debug information is the layout source. It omits types no variable has
    (`Option<&mut Level>`, `NonNull<[Level]>` here), so the importer compiles a copy of the
    source with one function per needed type appended to make rustc describe it.
  - The vtable is a `.data.rel.ro` section of the object file: size and align words, then one
    relocation per method to a v0-mangled symbol (`llvm-cxxfilt` gives
    `<book::BookHandler as book::Handler>::on_add`); `drop_in_place` is 0 for a type without drop glue.
  - A static's evaluated initial bytes are printed after the bodies (`alloc7 (static: G_HANDLER,
    size: 424, align: 8)`, `__` for padding).

## Limits

- Signed subtraction of prices: `book.cpp` computes `ticks - o.ticks` on `int64_t` (undefined on
  overflow in C++), `book.rs` uses `wrapping_sub`. The binaries agree on all inputs run
  (including random 64-bit prices) because clang emits a plain `sub`; at the level of the two
  standards the C++ one is defined on fewer inputs.
- Out-of-range indices: `book.rs` has bounds checks that abort (`-Cpanic=abort`); `book.cpp`
  has none. No input reaches them (indices are below `N` by construction).

## Next step

Both acceptance lines are met for `book.rs`. In progress: `experiments/rust/more.rs` (written,
not yet importable), the second input: tuples, a call through a function pointer
(`ReifyFnPointer` cast, `copy _1(..)` callee), a generic function at two types (generics of a
`fn`, which the MIR text does not print), `Drop` terminator and the `Drop` impl, a data enum
with explicit discriminants, `Repeat`, `Div`/`Rem`/shifts/`Neg`/`BitAnd`/`BitXor`/`Mul`, the
division asserts, `no_retag`, `const i64::MIN`. Run `python3 tools/mir2cub.py
experiments/rust/more.rs -o experiments/rust/more.cub --entry more_entry`, extend the importer
and `lib/lang/rust/ast.cub` at each stop, check `more.cub`, record the kinds under Done, and
write Limits.
