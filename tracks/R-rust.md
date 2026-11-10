# Track R — Rust as data: MIR into the checker (Phase 5, first half)

Read `README.md` here (rules), `../PLAN.md` §5 and §6 Phase 5, and `tracks/C-clang.md` (the
same job for C++; keep the two ASTs alike where the languages are).

Files you own: `tools/mir2cub.py`, `lib/lang/rust/ast.cub`, `lib/lang/rust/read*.cub`,
`lib/lang/rust/machine.cub`, `experiments/rust/**` (with `experiments/rust/sem/**`), this file.

Items 1–3 (the import) are done. Items 4–7 (the semantics; the Rust twin of `tracks/F-cxx.md`,
assigned by track A after the import was integrated) are below "What is built".

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

4. **`read` for MIR over `lib/mem`** (`lib/lang/rust/machine.cub`, `lib/lang/rust/read*.cub`): the
   meaning of an imported crate as a process in `lib/mem/monad.cub`'s `Ev`, at a `Target`.
   Structural on the imported data (bodies, blocks, statements, rvalues, terminators, places
   with projections, operands, constants, the substitutions of generic instances, vtables for
   `dyn` calls, statics); the run-time content of the terms it produces is only memory
   commands, scalar operations and control (everything the MIR decides is consumed by case on
   it). **The reader never touches a memory value** (tracks/README.md, "Memory is a
   signature"): it uses only the monad's named memory operations and a pointer is abstract to
   it. Control between basic blocks and calls by a `Word` fuel that is a parameter of the
   meaning (out-of-fuel is `mstop st-fuel`, distinct from undefined behaviour and from abort).
   A failed `Assert` (`-Cpanic=abort`) is an **abort** outcome, not undefined behaviour. Every
   kind in the two lists under Done (book.rs and more.rs) is read; a constructor of the AST not
   read is an explicit `unsupported` outcome naming it, and Limits is empty for what the
   samples use.
5. **Diagnostics on the net** (bring-up, not acceptance): the reading of `on_message` and of
   `more_entry` under the standard memory on generated inputs, beside what the rustc-built
   binary prints; recorded under "Diagnostics" with steps per message.
6. **Compiled**: the entry's term under track M's `mem-flat` handler through `#jit`/`#build`,
   specialized at the crate: whether it compiles, its size, time per message; what the residual
   still interprets is listed with its cause and removed, until the residual is free of
   interpretation. Compiler refusals go under "Needs from track A" with the smallest input.
7. **The cross-language statement** (PLAN Phase 6; `lib/obs.cub`; track F produces the C++ side
   as `cxx-entry book-tu on_message`): `on_message` in Rust and in C++ related on runs: a
   relation between the two machines' states (the two books), preserved by one message from
   related states with equal outcomes, under the precondition on which the C++ one is defined
   (stated precisely: the wrapping-vs-undefined subtraction and the bounds). This statement,
   checked, is the only ground for calling the two the same component. Proved now on the Rust
   side: the entry's outcome as a closed-form function of the book state for each message
   kind. The final statement waits for F's term and is the track's last open item until then.

## Acceptance

(Corrected by track A, 2026-11-09: agreement of runs is never an acceptance line; see Found.)

- (item 4) the library, `lib/mem/*.cub`, `lib/lang/rust/ast.cub`, `lib/lang/rust/machine.cub`,
  `lib/lang/rust/read*.cub` load with `N passed, 0 failed`, no postulates; every MIR kind the two
  samples use is read (Limits empty for them); the reader holds no memory value.
- (item 6) the entry's term under `mem-flat` compiles with a residual free of interpretation.
- (item 7) the statement checked (`experiments/rust/sem/props.cub` for the Rust side; the
  cross-language statement when F's term exists).
- (items 1–3) `python3 tools/mir2cub.py experiments/rust/book.rs -o experiments/rust/book.cub` runs; the
  library, `lib/lang/rust/ast.cub`, then `book.cub` load with `N passed, 0 failed` and
  `book-crate : RCrate` checked. Record: statement/rvalue/terminator kinds met, bodies, blocks,
  steps and seconds.
- (was an acceptance line; it is a diagnostic: see "Diagnostics") `book.rs` and `book.cpp` built
  natively give the same outputs on the same 10⁵ random messages.

## Needs from track A

- **String type: `CStr` is not used, and why.** Using track C's `CStr`/`cstr` means loading
  `lib/cxx/ast.cub` before `lib/lang/rust/ast.cub`. That file cannot currently be loaded together
  with `lib/mem/scalar.cub`, which the Rust reader needs: `./build/cubical $LIB lib/mem/target.cub
  lib/mem/scalar.cub lib/mem/memory.cub lib/mem/layout.cub lib/mem/monad.cub lib/cxx/ast.cub`
  stops with `error: declared again (a name is bound once): bo-mul` (both files define a
  `BinOp`-like type with `bo-…` constructors; tracks C/F/M have to settle that, it is not this
  track's file). Until `lib/cxx/ast.cub` loads after `lib/mem`, `RName = List Word`, which is
  exactly `cstr`'s argument (`cstr name` converts).
- Not blocking. One string type for both language ASTs: `lib/cxx/ast.cub` (generated by
  `tools/clang2cub.py --emit-ast`, track C) defines `cstr-put` (a `primitive` over `%store8`),
  `CStr = (n : Nat) × Bytes n` and `cstr : List Word → CStr`. If those five definitions move to a
  shared file loaded before both ASTs (for instance `lib/prim.cub`, next to `load32`/`store32`),
  `RName` can become `CStr` by changing one line of `lib/lang/rust/ast.cub` and wrapping
  `Emit.name`'s list in `cstr` in `tools/mir2cub.py`. Until then `RName = List Word`, which is
  exactly `cstr`'s argument, so a reader converts with `cstr name`.

## Needs from track M

The reader reaches memory only through the definitions of one section of
`lib/lang/rust/read-place.cub` ("the memory commands the reader uses"): `rm-alloc`,
`rm-alloc-static`, `rm-release`, `rm-elem-ptr`, `rm-off`, `rm-load`, `rm-store`, `rm-null`,
`rm-fn-ptr`, `rm-fn-of`. Today they are `ev-alloc`, `ev-release`, `ev-ptr-add`, `ev-load`,
`ev-store` of `lib/mem/monad.cub`; when the signature of memory commands lands, that section is
restated over the commands and nothing else changes. What the reader needs and no named
operation gives (each is worked around inside that section, as said):

1. **Load and store at an explicit alignment** (`load k al p`, `store k al p v`): a field of a
   `#[repr(packed)]` struct (`WireMsg.qty` at offset 2), an array of bytes transmuted to an
   integer (`u32::from_le_bytes`), a tag read at alignment 1. `ev-load`/`ev-store` are at the
   kind's own alignment. Worked around: an integer below its alignment is read and written as
   its bytes in the Target's order (`rm-bytes-rd`/`rm-bytes-wr`); a pointer below its alignment
   is `rx-unsupported (rz-unaligned sk-ptr)`. The byte route loses one check: bytes never
   written read as initialized at the 8-bit kind, so an unaligned integer load of unwritten
   memory is not reported undefined.
2. **Function pointers as commands**: a pointer for an instance index, and the index of a
   pointer (an indirect call, a vtable slot). Today `rm-fn-ptr`/`rm-fn-of` use scalar.cub's
   `mp-fn` constructor directly: the one place the reader looks inside a pointer.
3. **The null pointer** as a named value (`rm-null` is `sv-p mp-null`): a vtable's empty drop slot.
4. **Pointer comparison** (`Eq`/`Lt` on raw pointers: `slice::IterMut::next` compares the
   cursor with the end) goes through `sv-cmp … sk-ptr` of scalar.cub, which is pure; if
   comparison becomes a memory command the two uses in `rm-binop` change.
5. **Release that forgets** (the same as F-cxx.md's need 1): `mem-release` keeps the dead
   allocation in the list, so every access walks every allocation ever made; a message makes
   tens of them. Nothing is worked around here: the reader cannot touch the memory.
6. Nothing else is needed for: reading a discriminant (a load of the tag's unsigned kind and a
   case on the value, by the layout the importer recorded), slice metadata (the second word
   of a wide pointer: a value in a register, or a second load), vtable loads (an ordinary
   pointer load from the vtable's static allocation, then need 2).

(Track M answered on 2026-11-09: see "From track M" at the end of this file. Not applied yet.)

## Diagnostics

Bring-up runs. They are not acceptance and not a ground for any equality.

- **rustc binary vs clang binary** (`experiments/rust/book_out.c`, `gen_messages.py`; commands
  under Done, "The sample pair"): byte-identical outputs on 100,000 messages (seed 1,
  sha256 `1422f7495869b550…`), on the same with `n = 21` (all `-3`), on 1,000,000 messages
  (seed 2, `896e0075f0f2745f…`).
- **The importer's counts vs rustc's listings**: blocks/statements counted in the MIR text,
  `-Zprint-mono-items`, `-Zprint-type-sizes` (numbers under Done).
- **The reading on the net vs the rustc binary** (`experiments/rust/sem/run.cub`: `rust-init` and
  `rust-entry` under the standard memory through `ev-run-pure`; `experiments/rust/sem/mk_run.py`
  writes the inputs and the binary's answers and a `#same`; `experiments/rust/more_out.c` is
  more.rs's driver). Fuel 100,000 per call; Target `x86-64-sysv`.
  - book.rs, the first 20 messages of seed 1: the 20 answers are the binary's (`#same` passes;
    with one answer changed it fails, checked). 104,166,842 steps: `rust-init` alone 969,097;
    the first message 4.2 M (it builds the instance table), the next two 2.0 M each, the
    average over 20 is 5.2 M and rising. 60 messages did not finish in 600 s.
  - more.rs, 150 inputs (pairs of integers, seed 3, small/32-bit/64-bit mixed): the 150 answers
    are the binary's. 280,592,469 steps, 1.87 M per input on average, rising the same way.
  - Why it rises: need 5 under "Needs from track M" (a released allocation stays in the
    memory's list; each call makes tens). 1,000 inputs are out of reach until that changes;
    nothing in the reader can change it, since the reader does not touch the memory.
  - Both crates gave the binary's answers on the first reading attempted; no outcome other
    than `mok` was met on these inputs.

## Done

Items 1–3 are done. Everything below was measured on this machine with
`rustc 1.97.0 (2d8144b78 2026-07-07)` (stable, LLVM 22.1.6, `x86_64-unknown-linux-gnu`,
`/root/.cargo/bin`), clang 18.1.3, `llvm-dwarfdump`/`llvm-cxxfilt` 18, binutils `objdump`.

### Item 4: the reader (`lib/lang/rust/machine.cub`, `read-place.cub`, `read-rvalue.cub`, `read.cub`)

1,704 lines in four files; with the library, `lib/mem/*.cub` and `ast.cub`: `1363 passed, 0
failed`, no postulate and no primitive of its own. The design is in the files' headers; in short:

- **Outcomes** (`RExc`, thrown in `Ev` and never caught): `rx-abort` (a failed `Assert`:
  `rab-bounds-check`, `rab-division-by-zero`, `rab-remainder-by-zero`, `rab-overflow op`),
  `rx-ub` (MIR's own undefined behaviour: `Unreachable` reached, an unchecked operation out of
  range, a tag that is no variant's, a call through a non-function, a register read while
  dead), `rx-unsupported` naming the construct (`RUnsup`), `rx-ill-formed` naming what in the
  crate's data contradicts itself (`RIll`). The memory's and the scalars' undefined behaviour
  is lib/mem's `mub`, out of fuel `mstop st-fuel`.
- **Frames**: a local of a scalar, wide-pointer or unit type is a register (a value in the
  frame, `rv-dead` when it has none); any other local is an allocation made at `StorageLive`
  (or at entry when the body has none for it) and released at `StorageDead` and at return. A
  borrow of a register local is `rx-unsupported rz-borrow-of-register` (rustc's MIR for the
  samples borrows objects only). The frame also carries the image (the addresses `rust-init`
  got for the statics and vtables).
- **Binding time**: every reader function takes the context and the syntax and returns a
  function of the frame; all cases on MIR data, type substitution (`rm-subst` under the
  instance's arguments), layout lookups and offsets happen before that function is returned.
  A crate is a table `instance → block → (call → frame → Ev next)` built once per run.
- **Fuel**: one `%welimP` (`rm-run`); each block entered spends one.
- **Calls**: to an instance by index; through `dyn` by loading the method's slot from the
  vtable the wide pointer carries; through a function pointer; `Drop` by the crate's
  `RDropImpl` table and the layouts (own impl, then fields).
- **Typed copy** by layout (`rm-copy`): scalar by scalar, an enum by its tag.
- **Every constructor of `lib/lang/rust/ast.cub` and what the reader does with it**:
  - `RStmt`: `rs-assign`, `rs-storage-live`, `rs-storage-dead`: read.
  - `RRvalue`: `rrv-use` (the `retag` flag is not used: see Limits), `rrv-repeat`, `rrv-ref`,
    `rrv-raw-ptr`, `rrv-cast`, `rrv-binop`, `rrv-unop`, `rrv-discriminant`, `rrv-aggregate`: read.
  - `RBinOp`: read: `add sub mul` (wrapping), `add-unchecked sub-unchecked` (overflow undefined),
    `div rem` (zero divisor and MIN / −1 undefined), `bit-xor bit-and bit-or` (integers and bool),
    `shl shr` (count modulo the width), `eq lt le ne ge gt`, `cmp` (into the result enum's
    variants 0/1/2), `offset`. Refused by name (`rz-binop`; none is in the samples):
    `mul-unchecked`, `shl-unchecked`, `shr-unchecked`, `add/sub/mul-with-overflow`.
  - `RUnOp`: `run-not`, `run-neg` (wrapping), `run-ptr-metadata`: read.
  - `RCastKind`: `int-to-int` (also from a C-like enum: its discriminant), `transmute`,
    `ptr-to-ptr`, `unsize` (array to slice: the length; to `dyn`: the vtable), `reify-fn-pointer`: read.
  - `RAggregate`: `rag-array`, `rag-tuple`, `rag-adt` (fields, then the tag), `rag-raw-ptr`: read.
  - `RTerm`: `rt-goto`, `rt-switch-int`, `rt-return`, `rt-unreachable`, `rt-call`, `rt-assert`,
    `rt-drop`: read. `RCallee`: `rcallee-fn`, `rcallee-dyn`, `rcallee-ptr`: read. `RAssertMsg`:
    all four: the abort's kind. `RUnwind`: `ru-continue`, `ru-unreachable`, `ru-terminate`: read
    (nothing unwinds under `-Cpanic=abort`); `ru-cleanup` is refused (`rz-unwind`; the importer
    does not emit it).
  - `RProj`: `deref`, `field`, `index`, `const-index`, `downcast`: read (`const-index` from the
    end of a slice is refused: `rz-const-index-from-end-of-slice`; not in the samples).
  - `RConst`: `rc-int`, `rc-bool`, `rc-param`, `rc-zst`, `rc-adt`, `rc-static`, `rc-fn`: read (an
    `rc-static` inside an `rc-adt` is refused: `rz-const`; not in the samples).
  - `RTy`: every constructor has a class, size and alignment; 128-bit integers are not in the AST.
  - `RLayout` (`rtag-none`, `rtag-direct`, `rtag-niche`), `RVtable` (`rdrop-none`; `rdrop-fn`
    refused, the importer does not emit it), `RStatic`, `RDropImpl`, `RInstance`: read.

### Toolchain facts

- `--emit=mir` works on stable and is what the importer uses. `-Z` flags (`-Zunpretty=mir`,
  `-Zprint-type-sizes`, `-Zprint-mono-items`) are refused on stable unless `RUSTC_BOOTSTRAP=1`;
  the importer uses only `-Zprint-mono-items`, to check its instance list (below);
  `-Zprint-type-sizes` was used once, by hand, to compare sizes.
- The MIR text is **pre-monomorphization**: `Book<N>::add` is printed once with `const N` and
  `[Level; N]`; callers name the instance (`Book::<8>::add(..)`). The importer keeps bodies
  generic and carries the substitutions as data (below).
- No `rust-src`, and rustc prints MIR only for the crate being compiled. With `-O` the MIR
  inliner brings the `core` functions the samples use into the crate's bodies
  (`slice::IterMut::next`, `from_raw_parts`, `checked_sub`, `from_le_bytes`, derived
  comparisons, `wrapping_*`); the calls left are to bodies of the crate, through `dyn`, or
  through a function pointer. Without `-O` they are calls to `core` bodies that are not printed.

### The sample pair (the comparison of the two binaries is under Diagnostics)

- `experiments/rust/book.rs` (259 lines), the twin of `experiments/cxx/book.cpp`: `#[repr(u8)]
  enum Side`, `Price` with derived comparisons and `impl Sub`, `#[repr(C, packed)] WireMsg`
  decoded from `&[u8]`, `Book<const N: usize>` with `[Level; N]` and add/reduce/reset, a closure,
  `iter_mut` loops, `checked_sub` and `wrapping_add`/`wrapping_sub`, `match` on a byte and on
  enums, `trait Handler` called through `&mut dyn Handler`, a `static mut` handler,
  `#[no_mangle] pub extern "C" fn on_message(p: *const u8, n: usize) -> i64`.
- `experiments/rust/book_out.c` prints every answer; `experiments/rust/gen_messages.py COUNT SEED`
  writes the messages (48% add, 24% cancel, 24% execute, 0.1% reset, the rest unknown types;
  rare random sides, 64-bit prices and 32-bit quantities). From `experiments/rust`, `$B` a
  scratch directory:
  `rustc --edition=2021 --crate-type=staticlib -O -Cpanic=abort book.rs -o $B/libbook.a;
  clang -O2 book_out.c $B/libbook.a -o $B/book_out_rs;
  clang++ -std=c++20 -O2 -fno-exceptions -fno-rtti -c ../cxx/book.cpp -o $B/book.o;
  clang -O2 book_out.c $B/book.o -o $B/book_out_cxx;
  python3 gen_messages.py 100000 1 > $B/m1.bin;
  $B/book_out_rs < $B/m1.bin | sha256sum; $B/book_out_cxx < $B/m1.bin | sha256sum`.
- Result: on 100,000 messages (seed 1) the two outputs are byte-identical, sha256
  `1422f7495869b550…` (3,983 answers `-2`, the rest spreads and `-1`); with `n = 21` passed
  (`book_out_* 21`) both answer `-3` 100,000 times; on 1,000,000 messages (seed 2) byte-identical,
  sha256 `896e0075f0f2745f…`.

### The importer and the AST (acceptance line 1)

- `lib/lang/rust/ast.cub` (193 lines, 41 declarations, 1,583,073 steps to check): `RTy` (with
  its own list constructors; types, generic arguments and array lengths), `RProj`, `RPlace`,
  `RConst`, `ROperand`, `RBinOp`, `RUnOp`, `RCastKind`, `RBorrow`, `RAggregate`, `RRvalue`, `RStmt`,
  `RUnwind`, `RCallee`, `RAssertMsg`, `RTerm`, `RBlock`, `RLocal`, `RGeneric`, `RBody`, `RAdt`,
  `RFieldLayout`/`RVariantLayout`/`RTagLayout`/`RLayout`/`RLayoutEntry`, `RTrait`, `RVtable`,
  `RStatic`, `RDropImpl`, `RInstance`, `RCrate`. Conventions are in the file's header: ids are
  positions in the crate's lists; an integer is `RInt`, sign and magnitude (the shape of track
  C's `CInt`), in constants, switch arms and `discr`; names are byte lists.
- `tools/mir2cub.py` (2,175 lines). `python3 tools/mir2cub.py SRC.rs -o OUT.cub [--entry NAME]`
  (default entry `on_message`; definitions are prefixed with OUT's stem). About 1 s. It prints
  the counts and kinds it met on stderr. Where each part comes from:
  - bodies: `rustc --edition=2021 --crate-type=lib -O -Cpanic=abort --emit=mir`; only bodies
    reachable from the entry are parsed and emitted, and body ids are in order of first reach.
  - generics as data: `RBody.generics`; inside a body `rty-param i` / `rty-const-param i` /
    `rc-param i`; `rcallee-fn body args`, `rc-fn body args`, `rty-adt adt args`; and
    `RCrate.instances`, every (body, closed arguments) reached from the entry by walking calls,
    function-item casts, vtables and Drop impls. The importer then runs rustc with
    `-Zprint-mono-items=yes` (`RUSTC_BOOTSTRAP=1`, the one unstable flag it uses) and refuses to
    write the file unless every instance it found is in rustc's list (book.rs: 10 of rustc's 10;
    more.rs: 12 of 13, the other being the `drop_glue::<Tally>` shim).
  - impl headers (generics, trait, self type) are read from the source at the span rustc
    names the impl by; a derive's span is the trait name and its type the next item. A free
    function's generic parameters are read from its `fn` declaration in the source. A call is
    connected to its body by unifying the impl's self type with the call's.
  - layouts: a copy of the source with one `static` per needed type appended, compiled
    `-g -Copt-level=0 -Ccodegen-units=1 -Csymbol-mangling-version=v0 --emit=obj`, read with
    `llvm-dwarfdump --debug-info`: size, alignment, fields (name, offset, type) in declaration
    order, variants with the tag's offset/size and each variant's tag value; the variant a
    niche encoding leaves untagged has none. `RVariantLayout.discr` is what MIR's
    `discriminant` reads (the tag value for a direct tag, the variant index for a niche).
    Layouts are emitted for every ADT instance and tuple in a reachable instance's types, and
    every type inside those.
  - vtables: the object's `.data.rel.ro` section whose relocations (`objdump -r`) demangle
    (`llvm-cxxfilt`) to `<T as Trait>::method`; size and align from the section's bytes
    (`objdump -s`). Slot order is the trait's method list (`RTrait`), indexed by
    `rcallee-dyn trait method`.
  - statics: the evaluated allocation rustc prints after the bodies (`RStatic.init`: runs of
    initialized bytes and of padding).
  - drops: for each type a reachable `Drop` terminator drops and each type inside it, the
    `Drop` impl in the crate if any (`RDropImpl`); rustc's glue shim itself is not a MIR body.
- **`experiments/rust/book.cub`** (generated, 1,200 lines, 110,038 bytes, 347 definitions):
  - 10 bodies: `on_message`, `decode`, `dispatch`, `<BookHandler as Handler>::on_add / on_cancel /
    on_execute / on_reset`, `Book<N>::add / reduce / reset`; 10 instances (the three `Book` bodies
    at `N = 8`); **157 blocks, 707 statements**, 280 locals, 52 distinct types, 11 ADTs
    (`WireMsg`, `Option`, `BookHandler`, `Side`, `Price`, `Book`, `Level`, `IterMut`, `NonNull`,
    `PhantomData`, `Ordering`), 13 layouts, 1 trait (4 methods), 1 vtable, 1 static (424 bytes).
  - Compared by hand with rustc's own listings: blocks and statement lines of the ten bodies
    counted in the MIR text are 157 and 707; `RUSTC_BOOTSTRAP=1 rustc … -Zprint-mono-items=yes`
    lists exactly the same ten functions (plus the static `G_HANDLER`); `-Zprint-type-sizes`
    gives the same size and alignment for all 13 types.
  - **Check**: `./build/cubical $(make -s -n library | sed -n 's/^.*cubical \(lib\/prelude.*\)$/\1/p')
    lib/lang/rust/ast.cub experiments/rust/book.cub` ends `1158 passed, 0 failed`
    (770 library + 41 + 347), `book-crate : RCrate` checked (7,272 steps). With no cached result
    for the two files: 3,230,842 interaction steps (ast 1,583,073; book 1,647,769; largest
    single definition 58,797), 7.0 s wall with the machine at load 4 (an earlier version of the
    same size took 3.3 s on a quieter machine); the cached library alone is 1.1 s. Run under
    `ulimit -v 2500000`; 29,716 cells live at the end.
  - Kinds met in the ten bodies (occurrences):
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
      constant parameter (`N`) 38, ADT value 5 (`Price {{ ticks: 0 }}`, `Side::Bid`/`Ask`),
      zero-sized value 2 (`PhantomData`), address of a static 1.
    - place projections: `Deref` 110, `Field` 99, `Index` 16, `ConstantIndex` 28, `Downcast` 3.
    - types: `bool`, `i8 i64 isize u8 u32 u64 usize`, `&T`/`&mut T`, `*const T`/`*mut T`, `[T; n]`
      (n a literal or the parameter `N`), `[T]`, `()`, ADTs with arguments, `dyn Handler`.
    - Not in book.rs's reachable MIR (rustc inlined or folded them, or the source has none):
      `Drop`, `Repeat`, non-unit tuples, function pointers, the closure in `add` (inlined; its
      type survives only in debug lines), `Option<u64>` from `checked_sub` (folded to `Lt` +
      `SubUnchecked`), `side_mut` and `spread` (inlined), checked arithmetic (`*WithOverflow`),
      `SetDiscriminant`.
- **`experiments/rust/more.rs`** (100 lines) and **`more.cub`** (generated, 500 lines, 137
  definitions), a second input for what book.rs does not reach; `python3 tools/mir2cub.py
  experiments/rust/more.rs -o experiments/rust/more.cub --entry more_entry`:
  - 11 bodies, 12 instances (`pick<T>` at `i32` and `u16`), 51 blocks, 189 statements, 3 layouts
    (`(i64, i64)`, `Shape` with tags 2/5/9, `Tally`), 1 static, 1 Drop impl. `-Zprint-mono-items`
    lists the same 12 functions plus `drop_glue::<Tally>` (the shim; here the `RDropImpl` entry).
  - It adds to the kinds above: terminators `Drop` 3, `Call` through a function pointer 1,
    `Assert` `DivisionByZero` 1 and `Overflow(Div)` 1; rvalues `Repeat` 1, `Use` with `no_retag` 2;
    cast `PointerCoercion(ReifyFnPointer)` 2; operators `Mul`, `Div`, `Rem`, `BitAnd`, `BitXor`,
    `Shl`, `Shr`, `Neg`; aggregate `Tuple` 2; constants function item 2 and `i64::MIN`; types
    `i32`, `u16`, a tuple, `fn(u32) -> u32`, a type parameter.
  - Library + `ast.cub` + `book.cub` + `more.cub`: `1295 passed, 0 failed`; `more.cub` is 495,880
    steps fresh, `more-crate` 4,963.
- Also imported (not committed): a 4-line trial with a slice iterator, `%` and a `dyn` call
  from a generic impl (2 bodies, `Assert` `RemainderByZero`, `&raw const`), and a 2-function
  trial matching on negative `i8` values and on `Ordering` (see Found, switch arms).

## Found

- **The spec asked for agreement of runs as acceptance** (items 1–3's second line, item 5 as
  first written, and "it is the ground for calling them the same component"). Track A
  corrected it (tracks/README.md, "What is evidence here"): those runs are diagnostics, and the
  two programs are the same component only by item 7's checked statement.
- **The first version of the reader touched memory values** (`rm-rd`/`rm-wr`/`rm-mem` wrapping
  `mem-load-al`, `mem-store-al`, `mem-ptr-add`, `mem-alloc`; `rust-init` a function on `AMem`;
  statics addressed as `mp-obj i #0`; dead locals as `mp-null`). Removed the same day: the
  reader now uses the monad's named operations only, `rust-init` is a computation whose value
  is the image (the addresses), a frame carries the image after its locals, a local without
  storage is `sl-none`.
- `deriving eq RTy` fails ("no inferable type here: annotate it") when the data type has a
  field of another data type (`RIntTy`, `RMut`), though `deriving eq` of those two alone works.
  `rm-ty-eq` is written out by hand in `lib/lang/rust/machine.cub`.
- `rec` is a keyword and cannot be a variable (`error: a keyword cannot be bound: rec`).
- The byte-column rule for `let` (M-mem.md) is met constantly in staged code (`… → (let a = …`
  then `b = …` on the next line): the files are written with every such `(let` on its own line.
- A local `let x = (case … of …)` whose branches are functions fails with "no inferable type
  here"; as a separate definition with a signature it checks (`rm-drop-body`).
- A `case xs of … cons q qs → … f xs …` branch loses the type of `xs` ("no inferable type" at
  `cons q qs`); asking `rm-is-nil xs` instead of matching works.

- No fault of the machine: every generated file loaded on its first attempt (largest 108 KB,
  347 definitions, 1.6 M steps). The output is many small definitions on purpose (one per type,
  name, layout, block; a block's statements in runs of 16); a single large literal was not tried,
  so no size limit was measured.
- The checker caches a whole file's result under `.cubical-cache` (`CUBICAL_CACHE`) as well as
  definitions under `.cubical-store`: a rerun reports `0 interaction steps performed`. To measure
  a file, point both variables at copies holding the library only.
- rustc/LLVM facts the importer depends on (each cost a wrong first attempt):
  - Debug information omits types no variable has (`Option<&mut Level>`, `NonNull<[Level]>`), so
    the importer makes rustc describe every needed type. Functions taking the type do not work
    for this: LLVM merges the identical ones and one description survives. Statics do.
  - At `-O` LLVM deletes a vtable it can devirtualize (the 4-line trial), so the object read
    for layouts and vtables is built at `-Copt-level=0`.
  - Debug information lists a struct's fields in declaration order with their offsets
    (`S { a: u8, b: u64, c: u8, d: u32 }`: a 12, b 0, c 13, d 8), which is MIR's field index;
    `-Zprint-type-sizes` lists them by offset and has no tag values or field types, which is
    why it is not the layout source.
  - A data enum with explicit discriminants (`#[repr(u8)] enum F { P = 3, Q(u16) = 9 }`) has tag
    values 3 and 9 in the debug information and `switchInt` arms 3 and 9 on `discriminant`.
  - **Switch arms are printed as unsigned bits, constants as signed values**: `match a { -1 => ..,
    -128 => .. }` on an `i8` is `switchInt(copy _1) -> [255: bb4, 128: bb3, ..]`, and a switch on
    `discriminant(o)` for `Ordering` has arms `255, 0, 1`, while the same values as constants
    print `const -1_i8`. The first version of the importer stored constants as 64-bit two's
    complement, arms as printed, and `Ordering::Less`'s discriminant sign-extended: three
    encodings of one number. Now all three are `RInt` (the integer): the arm is decoded at the
    scrutinee's type, the discriminant at the tag's type; only `RTagValue` is raw bits.
  - New in this rustc's MIR text: `no_retag copy P` on a use, `// DBG: _5 = &_2;` lines inside
    blocks, `P[2 of 3]` for constant indices, `*const [u8] from (a, b)` for a pointer built from
    parts, constants like `Price {{ ticks: 0_i64 }}`.

## Limits

**Of the reader (item 4), on things the samples use — these keep item 4 open:**
- **References carry no aliasing discipline.** `&`/`&mut`/`&raw` are addresses; `no_retag`
  and retagging do nothing; a write through a pointer that Rust's aliasing rules forbid is not
  undefined here. (Rust has no settled rule to write: Stacked Borrows and Tree Borrows are
  Miri's two candidates. PLAN Phase 5 reads "the borrow discipline as multiplicities", which
  is the place for it.)
- **Validity of values is checked only where the memory layer checks it**: a `bool` read must
  be 0 or 1 and an enum's tag a variant's (at a typed copy and at `discriminant`); a reference
  is not checked non-null, aligned or dereferenceable when it is produced, only when used.
- **An integer below its alignment** (a `#[repr(packed)]` field: book.rs's `WireMsg`; bytes
  transmuted: `from_le_bytes`; tags, read at alignment 1) goes by bytes, and that route does
  not report unwritten bytes (Needs from track M, 1).
- **A move reads as a copy**: the place moved from keeps its value; MIR that reads it again
  (rustc's does not) is not reported.
- **Registers**: a local whose address is never taken has none. This is the reader's
  definition, not a theorem about an allocation-per-local machine (MiniRust's): no such
  machine is defined here to compare with.

**Of the reader, on things the samples do not use** (each an `rx-unsupported` naming it):
the six `RBinOp`s above; a borrow of a register local; a pointer at less than its alignment;
128-bit tags; `Drop` of an enum one of whose fields needs dropping; a vtable with drop glue; a
transmute of a constant of an ADT type; comparison of wide pointers; `const-index` from the end
of a slice.

**Of the import (items 1–3):**

- **Stops, each with a message naming the construct** (nothing is emitted for them; extend the
  importer when an input needs one): a call to a function of another crate that rustc did not
  inline; a closure body reached by a call, and closure or coroutine types; generic parameters
  on a method of an impl (the impl's own and a free function's are read); a trait with generic
  arguments, `where` clauses on an impl header, associated-type paths (`<T as Tr>::X`); a call
  to a trait method on a type parameter (it would have to be resolved per instance); `i128`/
  `u128`, floats, `char`, `str`; constant generic arguments other than `usize` literals;
  unions; a vtable with no methods or with drop glue; a supertrait method called through `dyn`;
  dropping a type of another crate, or an unsized one; allocations that are not statics or that
  hold pointers; cleanup blocks and any unwind target other than continue/unreachable/terminate;
  statements other than `Assign`/`StorageLive`/`StorageDead` (`SetDiscriminant`, `Deinit`,
  `Retag`, intrinsics); assert messages other than the four listed above; subslice projections;
  two items with the same last path segment (ADTs and traits are identified by it).
- **Not imported, by decision**: debug information (`debug` lines, scopes, `// DBG` notes, hence
  variable names and the inlining tree); lifetimes; the mutability of arguments (the text does
  not print it: `rlocal-arg`); the ABI and `#[no_mangle]` of the entry (not in the MIR text;
  `RCrate.entry` is body 0); bodies evaluated at compile time (constants, the static's
  initializer, CTFE copies of `const fn`), whose results are in the text as values.
- The MIR is rustc's after its MIR optimizations (`-O`), not the MIR borrow checking ran on;
  "the borrow discipline read as multiplicities" (PLAN Phase 5) will need the pre-optimization
  MIR (`-Zunpretty=mir` at `mir-opt-level=0`), where the `core` calls are not inlined.
- `rustc`'s own vtable and layout choices are read from one compilation for x86-64; they are
  implementation parameters of this target, not of Rust.
- Signed subtraction of prices: `book.cpp` computes `ticks - o.ticks` on `int64_t` (undefined on
  overflow in C++), `book.rs` uses `wrapping_sub`. The binaries agree on all inputs run
  (including random 64-bit prices) because clang emits a plain `sub`; at the level of the two
  standards the C++ one is defined on fewer inputs.
- Out-of-range indices: `book.rs` has bounds checks that abort (`-Cpanic=abort`); `book.cpp`
  has none. No input reaches them (indices are below `N` by construction), and showing that
  is part of the equality proof.
- Against track C's `lib/cxx/ast.cub` (landed after this AST was written; compared at commit
  `8acadc65`): the same in being first-order data with own list constructors inside a
  self-recursive type, ids as `Word`, prefixed constructors, integers as sign and magnitude
  (`RInt` / `CInt`), layouts in bytes from the compiler's own output, one constructor per kind
  met with an unmet kind stopping the import. Different: strings (`CStr` there, `List Word`
  here: see Needs from track A); C's AST file is generated from a schema in its importer, this
  one is written by hand; C records layouts inside each record declaration, here they are a
  separate list keyed by closed type because Rust's are per instance of a generic type.

## Open items, exactly

- **Item 4** is written and checks; it stays open on the Limits above that the samples touch
  (aliasing, validity at production, the byte route) and on track M's signature (the memory
  section of `read-place.cub` is restated when it lands).
- **Item 5** (diagnostics): done as far as the memory allows (20 messages, 150 inputs).
- **Item 6** has not started: there is no `mem-flat` handler yet (track M). Nothing was compiled.
- **Item 7** is not proved, on either side. The statement, to be written in
  `experiments/rust/sem/props.cub` against `lib/obs.cub`'s `o-RunSim` once the reader's term is
  over M's signature and F's `cxx-entry book-tu on_message` exists:
  - *States.* A book value `B`: two arrays of 8 levels `(price : i64, qty : u64, orders : u32)`,
    two counts, and the counters `adds`, `misses`, `volume`. `Rel sr sc`: the Rust memory's
    `G_HANDLER` (layout `BookHandler` of `book-crate`: `book` at 0 with `bids` 0, `asks` 192,
    `bid_count` 384, `ask_count` 392; `adds` 400, `misses` 408, `volume` 416) and the C++ memory's
    `g_handler` (Clang's record layout in `book-tu`) both hold one `B` in the levels below their
    counts, the counts and the counters. `Inv B`: both counts ≤ 8. (No level at or above a
    count is read before it is written by either program, so `Rel` says nothing about them.)
  - *One message.* For every 22 readable bytes `m` and every `sr`, `sc` with `Rel sr sc` and
    `Inv`: if the C++ run of `on_message(m, 22)` from `sc` is defined, the Rust run from `sr`
    is `mok v` with the same `v`, and the states after are related and satisfy `Inv`. Under
    `Inv` the Rust run never aborts (its 38 bounds checks hold) and never has undefined
    behaviour; the observation keeps `abort` and undefined behaviour apart, but under `Inv`
    neither occurs on the Rust side.
  - *The precondition that makes the C++ run defined.* `Inv` discharges every array access.
    What is left is one operation: `Price::operator-` in `spread()`, `ticks - o.ticks` on
    `int64_t`, undefined when the difference of the best ask and the best bid is outside
    `[-2^63, 2^63)`; book.rs has `wrapping_sub` there. So: the C++ run is defined iff, after the
    message is applied, a side is empty or `best_ask.price - best_bid.price` is representable.
    A precondition on inputs that makes this all inputs: every price added lies in
    `[-2^62, 2^62)` (an invariant of `B` that `add` preserves and nothing else changes prices).
  - *Sequences.* By `o-runsim-bind`, from the initial states (both books empty, related) the
    two programs give equal answers on every sequence of messages whose prices are in range.
  - *Closed form on the Rust side* (the entry's answer as a function of `B` and the message
    kind) is not proved: a reading of the term at a symbolic state does not finish (one
    concrete message is 2–5 M steps under the standard memory), and the term changes when
    M's signature lands. It is stated here, not in a `.cub` file, because an unproved second
    description of the book in the repository would be exactly what tracks/README.md forbids.

## Next step

**HALTED (coordinator, 2026-11-09): the owner stopped implementation until the mathematical
specification is written and reviewed. Do not continue from here without that go-ahead.**

Exact state at the halt:
- Pushed and checked: items 1–3 (import); `lib/lang/rust/machine.cub`, `read-place.cub`,
  `read-rvalue.cub`, `read.cub` and `experiments/rust/sem/run.cub` — checked (`1363 passed, 0
  failed`, and the diagnostics above) against `lib/mem` **as it was before track M's commit
  `23157fb9`** (load order `target, scalar, memory, layout, monad`).
- **Not rechecked against the current `lib/mem`**, which changed under this track (see "From
  track M" below): `Ev` is now a process over the memory signature, `ev-rd`/`ev-wr`/`ev-mem`
  and `ev-run-pure` are gone, `BinOp`'s constructors are `ao-*`. The four reader files and
  `run.cub` therefore do not load today. No uncommitted work exists.
- What applying M's note takes (not started): in `read-place.cub`'s memory section
  `rm-load`/`rm-store` become `ev-load-al`/`ev-store-al` (delete `rm-bytes-rd`/`rm-bytes-wr`),
  `rm-off` becomes `ev-off`; `bo-*` → `ao-*` in `read-rvalue.cub`; `rm-res`, `rm-throw`, `rm-stop`
  restated over the new `Ev`; `run.cub`'s `ev-run-pure … (mem-empty t)` becomes
  `ev-std t e am-empty`; `rm-fn-ptr`/`rm-fn-of`/`rm-null` still use `mp-fn`/`mp-null` (needs 2, 3
  are unanswered in M's note).
- Items 6 and 7: not started / not proved, as under "Open items, exactly".

## From track M (2026-11-09, memory as a signature; tracks/README.md)

`lib/mem` changed under you (commits `23157fb9` to this one); load order is now
`lib/mem/{target,scalar,sig,flat,memory,layout,monad}.cub`. What to change in lib/lang/rust:
- `Ev S V A` is `Proc (MK S) (MRes (Ctl V A))`, no longer a function of `AMem`; `ev-rd`,
  `ev-wr`, `ev-mem` are deleted, so delete `rm-rd`, `rm-wr`, `rm-mem` in machine.cub and
  replace their uses in read-place.cub/read-rvalue.cub by the commands:
  `rm-load t k al p = ev-load-al t k al p`; `rm-store t k al p v = ev-store-al t k al p v`;
  `rm-off p n = ev-off p n`; `rm-alloc size al = ev-alloc ak-auto false size al`;
  `rm-release p = ev-release ak-auto p`; `rm-rd (mem-ptr-add p es ix) = ev-ptr-add p es ix`
  (each with `{lzero} {NoS} {RExc}` as now). Other memory commands: `ev-free`, `ev-ptr-diff`,
  `ev-ptr-cmp`, `ev-ptr-int`, `ev-int-ptr`, `ev-memmove`, `ev-memcpy`, `ev-memset`.
- A pointer is opaque: do not build `mp-obj`; pointers come from `ev-alloc`/`ev-ptr-add`/loads.
- `BinOp`'s constructors are `ao-add ao-sub ao-mul ao-div ao-rem ao-and ao-or ao-xor`
  (`bo-*` is lib/cxx/ast.cub's, which is in the library).
- Running: `ev-run-pure` is gone. The meaning is `ev-std t e am-empty : Step (MT S) (MRes …)`
  (a tree whose only commands are the world's, `w-alloc` among them); what is compiled is
  `ev-flat t e`; `ev-sound` relates them. experiments/mem/sum.cub shows both.
