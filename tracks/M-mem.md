# Track M — the abstract machines' common ground: values, memory, objects (for C++, C, Rust, Go)

Read `README.md` here (rules), `../PLAN.md` §5 and §6 Phases 4–5, and `lib/prim.cub`,
`lib/decode.cub`, `lib/region.cub`, `lib/proc.cub`.

Files you own: `lib/mem/**`, `experiments/mem/**`, this file.

Every language's abstract machine is written over this layer, so it is language-neutral and
takes the implementation-defined choices as a **parameter** (PLAN §5.1), never as constants.

## What is built

1. **`lib/mem/target.cub`**: `Target`, a record of the implementation's choices: byte order;
   size and alignment of each scalar kind (bool, char, short, int, long, long long, pointer,
   float, double, 128-bit); signedness of plain `char`; and the value `x86-64-sysv`.
2. **`lib/mem/scalar.cub`**: scalar kinds `SK` (signed/unsigned integers of 8/16/32/64/128
   bits, bool, f32, f64, pointer) and values `SV`; total semantics as functions into
   `Res A = ok A | undefined (why : UB) | …`: integer conversions between every pair of kinds
   (modular for unsigned targets, implementation-defined/modular for signed per C++20's two's
   complement), usual arithmetic conversions and integer promotions as functions of `Target`,
   `+ - * / %` (signed overflow, division by zero, `INT_MIN / -1` are `undefined` with their
   reason), shifts (count out of range undefined), bitwise ops, comparisons, float
   arithmetic and float/integer conversions (out-of-range undefined), pointer comparison.
   Use `UInt64`/`Int64`/`Float64` primitives underneath; 128-bit as two `UInt64`.
3. **`lib/mem/memory.cub`**: the memory of an abstract machine as a value:
   allocations (id, size, alignment, liveness, kind: automatic/static/dynamic), a pointer as
   `(allocation id, offset)` plus the null pointer and function pointers (a `Word` id);
   `alloc`, `free` (double free, free of non-dynamic: undefined), `load k p`, `store k p v`
   (out of bounds, dead allocation, misalignment: undefined; reading uninitialized bytes of a
   non-character kind: undefined), `memcpy`/`memmove`/`memset`, pointer arithmetic (one past
   the end allowed, beyond undefined), pointer-to-integer and back as the implementation
   parameter allows. Bytes are stored per allocation in `Bytes n` with an initialized mask.
   All operations are pure functions `Mem → Res (A × Mem)`.
4. **`lib/mem/layout.cub`**: record layouts as data (size, alignment, fields with offsets and
   kinds, bases, vtable pointer offset), and typed access to a field/element of an object at a
   pointer through them; arrays; unions (active member tracking is NOT modelled in v1: say so
   under "Limits" and what it would add).
5. **`lib/mem/monad.cub`**: the evaluation monad every reader targets:
   `M A = Mem → Res (A × Mem)` extended with **control outcomes** (`return v`, `break`,
   `continue`, a thrown value) and **world commands** (I/O through `lib/proc.cub`'s `Proc` over
   a signature parameter), with `bind`, `ret`, loops by fuel-free structural means where
   possible and otherwise by a `Word` fuel that is a parameter of the meaning, and its laws
   (left/right identity, associativity) as checked paths.
   Design constraint (PLAN §5.4, binding time): everything that depends only on the program
   (kinds, layouts, offsets) must be computed by case on program data, so that specializing a
   reader at a fixed program leaves only loads, stores and arithmetic on run-time values.

## Acceptance

- `make library` passes, and `lib/mem/*.cub` loaded after it end `N passed, 0 failed`.
- `experiments/mem/sum.cub`: by hand, the meaning of a C function summing an `int[n]` through a
  pointer (as a term in `M`), `#jit`-compiled through the project's compiler when applied to a
  memory with a concrete layout: record whether it compiles, the bytes, and what the
  specialized code still contains that should have been computed away (that list is the input
  to the binding-time design). Also a `#show` of an out-of-bounds load giving `undefined`.

## How to check (every file of this track)

```
LIB=$(make -s -n library | sed -n 's/^.*cubical \(lib\/prelude.*\)$/\1/p')
(ulimit -v 2500000; timeout 600 ./build/cubical $LIB lib/mem/target.cub lib/mem/scalar.cub lib/mem/memory.cub lib/mem/layout.cub lib/mem/monad.cub | tail -3)
```
The files load in that order (each needs the ones before it). Names are prefixed (`tg-`, `ck-`,
`sk-`, `sv-`, `ub-`, …) because `Res`, `ok`, `refused`, `Mem`, `Opt` are taken by the library:
the result type of this layer is `MRes`, not `Res`.

## Needs from track A

- (not blocking; cost of the compiler) `#jit sum-at` of `experiments/mem/sum.cub` takes
  645,757,892 net steps for 283,927 bytes: 2,274 steps a byte, against 40 to 60 for the
  workloads of PLAN §4.3 (`st8`: 22,489 steps, 414 bytes). The residual it is given normalizes
  in 41,126 steps (14,806 cells), so the cost is in `jit-plan` after normalization (the
  conversion check, `program-of`, `x-erase`, `x-lift`, the encoder), and it is not linear in
  the code's size. The input reproduces it: load the five `lib/mem` files and
  `experiments/mem/sum.cub`.
- (not blocking; needed for section "What the specialized code still contains") the items
  under A need case-of-case in the specializer; 9 needs `load64`/`store64` at a `Word` (or
  `UInt64`) offset in lib/prim.cub, with the bound stated on words (`Lew`), so that
  `by-fit`'s conversion to `Nat` and `toNat off` disappear; 13 needs `load64u`/`store64u` at
  `UInt64` (or `u64`↔`i64` casts as primitives that are no code); 12 needs a process over a
  signature with no commands to specialize to its answer (`ev-run-pure` under a loop).
- (not blocking; cost only) primitives `f32add f32sub f32mul f32div f32eq f32lt : Float32 → …`,
  `f64off32 : Float32 → Float64`, `f32off64 : Float64 → Float32` (SSE `addss … cvtss2sd
  cvtsd2ss`), so that `f32-op`, `f32-widen`, `f64-narrow` of `lib/mem/scalar.cub` can be one
  instruction each; with them, the three definitions become the primitives and `fp-pack` stays
  for the integer conversions.

## Done

- `lib/mem/target.cub`: `Endian`, `CK` (the ten scalar kinds C leaves open), `PtrInt`
  (`pi-refused | pi-flat`), record `Target` (`tg-endian`, `tg-size`, `tg-align`,
  `tg-char-signed`, `tg-ptr-int`, `tg-alloc-align`, `tg-base`), and three values: `x86-64-sysv`,
  `i386-sysv` (ILP32, no 128-bit integer), `s390x-elf` (big-endian, char unsigned). Loads after
  the cached library: 785 passed, 0 failed (18 new checks, 23,789 steps).
- `lib/mem/scalar.cub`, first part (879 passed, 0 failed after target.cub; 94 new checks):
  `UB`, `Stop`, `MRes` (`mok | mub | mstop`, `mr-bind`, `mr-map`); `IW`, `SK`, `MPtr`, `SV`;
  64-bit helpers (`uq-*`: canonical forms `uq-zx`/`uq-sx`/`uq-cn`); 128-bit arithmetic on
  `DQ = UInt64 × UInt64` (`dq-add/sub/mul/shl/shr/sar/udivmod/sdiv/srem`, `uq-mulw`);
  integers: `int-op` (`BinOp`: add sub mul div rem and or xor, at every width and sign, with
  `ub-signed-overflow`, `ub-div-zero`, `ub-div-overflow`), `int-neg`, `int-not`, `int-shl`
  (modular), `int-shl-c` (C17's), `int-shr`, `int-cmp` (`CmpOp`), `sv-int-dq`/`dq-to-int`
  (the two halves of every integer conversion). Read by `#show`: INT_MAX + 1 at i32 is
  `mub ub-signed-overflow`; 2^32·2^32 at i64 overflows and −2^32·2^31 is −2^63; i32 MIN / −1 is
  `mub ub-div-overflow`; 2^126·2 at i128 overflows; `1 << 32` at i32 is `mub ub-shift-count`;
  a 128-bit division is about 43,000 net steps.

- `lib/mem/scalar.cub`, complete (target.cub + scalar.cub: see the count under "Measured").
  Floats: `fp-pack` (an integer magnitude · 2^e rounded to nearest-even into binary32 or
  binary64 bits, subnormals and overflow to infinity included), `f32-widen`, `f64-narrow`,
  `f32-op` (binary64's operation on the widened operands, narrowed), `f64-op`, `f64-cmp`,
  `int-to-f64`, `int-to-f32` (64- and 128-bit sources through `fp-pack`, so one rounding),
  `f64-to-int` (`ub-float-int-range`; 128-bit targets from the bits). Pointers: `mp-eq`,
  `ptr-cmp` (an order between different allocations is `mstop st-unspecified`). By kind:
  `sv-conv` (every pair of kinds; pointer↔integer is memory.cub's), `sv-truthy`, `sv-arith`,
  `sv-neg`, `sv-not`, `sv-shl`, `sv-shl-c`, `sv-shr`, `sv-cmp`. Kinds in memory: `sk-size`,
  `sk-align`. The C family: `CTy`, `ct-sk`, `ct-size`, `ct-align`, `ct-rank`, `ct-promote`,
  `ct-uac`, `ct-arith`. Read by `#show`, each equal to the IEEE/C answer computed
  independently (Python's `struct` for the float bits): 1/3, 0.1, 2^24+1 (tie to even),
  2^24+3, DBL→FLT overflow to infinity, the smallest subnormal, the smallest normal;
  (int64)2^53+2^29+1 → float is 0x5a000001 (through double it would be 0x5a000000);
  0.1f+0.2f is 0x3e99999a; (int)2147483648.0 is `mub ub-float-int-range` and
  (int)-2147483648.5 is INT_MIN; `unsigned short` 65535·65535 at x86-64 is
  `mub ub-signed-overflow` (promotion to int); `long + unsigned` is `long` at x86-64 and
  `unsigned long` at i386.

- `lib/mem/memory.cub` (the memory type is `AMem`: `Mem` and `Memory` are lib/proc.cub's):
  `AKind`, record `Alloc` (id, size, alignment, live, kind, base address, zeroed, `al-data` and
  `al-init : Bytes (by-cap al-size)`, the allocation's bytes and one written-mark byte per
  byte), `AMem` (allocations, next id, next address). Access proofs: `by-chk s size` (a word
  equation), `by-lew`, `by-drop`, `by-fit : Lew s size → le (plus (toNat s) 8) (by-cap size)`:
  the proof `load64`/`store64` need is made from the bounds check that reports
  `ub-out-of-bounds`, so there is one check per access and no unproved access. Operations:
  `mem-alloc`, `mem-release` (kind-checked end of storage), `mem-free` (`ub-double-free`,
  `ub-free-not-dynamic`, `ub-free-interior`), `mem-load[-al]`, `mem-store[-al]`
  (`ub-out-of-bounds`, `ub-dead`, `ub-misaligned`, `ub-uninit` except at 8-bit kinds,
  `ub-null`, `ub-bad-bool`; byte order from the Target), `mem-memmove`, `mem-memcpy`
  (`ub-overlap`), `mem-memset`, `mem-ptr-add` (one past the end allowed), `mem-ptr-diff`,
  `mem-addr`, `mem-ptr-of-addr`, `mem-conv` (pointer↔integer under `tg-ptr-int`),
  `mem-ptr-cmp` (order by address under `pi-flat`). Read by `#show` on a 12-byte and a 16-byte
  allocation: a fresh int load is `mub ub-uninit`; store −2 then load is −2, its byte 3 is 255
  at x86-64, and byte 3 of 258 stored at s390x is 2; offset 12 is `ub-out-of-bounds`, offset 2
  `ub-misaligned`; load after free `ub-dead`, second free `ub-double-free`; a pointer stored
  and loaded is the same pointer; memcpy of 12 bytes of which 8 were written leaves the copy's
  last int `ub-uninit`; `p+3` of `int[3]` is one past the end and `p+4` is `ub-ptr-arith`;
  `(uintptr_t)` of allocation 1 offset 4 is 65556, 65548 converts back to allocation 0 offset
  12 and 65549 (the gap) to `mp-addr #65549`.

- `lib/mem/layout.cub`: `MTy` (`ty-sk` a kind with its alignment, `ty-c` a C type, `ty-arr`,
  `ty-rec` a record by id), records `Field`, `Base`, `Layout` (id, size, alignment, union,
  fields, bases, vtable-pointer offset), `LayEnv` the table; `ty-size`, `ty-align`,
  `ty-scalar`; `lay-struct` and `lay-union` (the C rule from member types and the Target);
  `lay-ok` (a layout given as data is well formed: offsets aligned, fields inside the size and,
  in a struct, pairwise disjoint; bases and the vtable pointer inside); typed access
  `obj-alloc`, `obj-field`, `obj-base`, `obj-derived`, `obj-elem`, `obj-load`, `obj-store`,
  `obj-copy`, `obj-vptr`, `obj-set-vptr`. Read by `#show`:
  `struct S { char c; double d; int a[3]; short s; }` is offsets 0, 8, 16, 28, size 32, align 8
  at x86-64 and 0, 4, 12, 24, size 28, align 4 at i386 (gcc's); `struct T { S s; char z; }` is
  40 bytes; `t.s.a[2] = 9` then reading `t.s.a[2]` is 9, `t.s.a[1]` is `mub ub-uninit`,
  `&t.s.a[7]` is `mub ub-ptr-arith`; a layout with two ints at offset 0 is not `lay-ok`.

- `lib/mem/monad.cub`, core and laws: `Ctl V A` (`c-val | c-ret | c-break | c-continue |
  c-throw`), `EvR V A = MRes (Ctl V A × AMem)`, `Ev {l} S V A = AMem → Proc {l} S (EvR V A)`,
  `ev-ret`, `ev-step`, `ev-bind`, `ev-then`; the laws checked as paths: `ev-left-id` (by
  computation), `ev-right-id` (from `ev-step-ret`, by cases on the result and the control
  outcome), `ev-assoc` (from `ev-step-assoc`, likewise). 1077 passed, 0 failed with the four
  files before it (864,583 steps for monad.cub's 14 checks, nearly all in the two laws).

- `lib/mem/monad.cub`, the rest: `ev-out`, `ev-res`, `ev-rd`, `ev-wr`, `ev-mem`, `ev-cmd` (a
  world command of the signature parameter), `ev-ub`, `ev-stop`, `ev-return`, `ev-break`,
  `ev-continue`, `ev-throw`; `ev-on` (a continuation that sees the control outcome), `ev-body`,
  `ev-fn` (a call boundary), `ev-catch`, `ev-finally`; `ev-for`/`ev-while`/`ev-do-while` (a
  `Word` fuel, `mstop st-fuel` when spent), `ev-for-n` (structural); `NoS`, `ev-run-pure`,
  `ev-value`; `ev-load`, `ev-store`, `ev-alloc`, `ev-release`, `ev-ptr-add`, `ev-local`.
  The five files after the cached library: `1107 passed, 0 failed`.
- `experiments/mem/sum.cub`, on the net: `sum-c` is by hand the meaning of
  `int sum(const int *p, int n) { int s = 0; for (int i = 0; i < n; i++) s += p[i]; return s; }`
  in `Ev` at a Target (locals as automatic objects, `ev-for` with fuel). Read by `#show`:
  {1,2,3,4}, n = 4 is `mok (c-val (sv-i #u64:10))` at x86-64 (33,770 steps) and at s390x
  (37,993); n = 5 is `mub ub-out-of-bounds`; the out-of-bounds load alone
  (`mem-load … (mp-obj #0 #16)`) is `mub ub-out-of-bounds`; {INT_MAX,1,0,0} is
  `mub ub-signed-overflow`; fuel 3 is `mstop st-fuel`; n = 0 is 0.

- `experiments/mem/sum.cub`, through the compiler. **It compiles**:
  `ok: jit sum-at   [283927 bytes, the checker's conversion, 645757892 steps]`.
  `sum-at : Bytes 24 → Word → Word → Word × Word` takes the bytes of `int a[4]`'s allocation,
  n and the fuel, builds the one-allocation memory, runs `sum-c` at `x86-64-sysv` and returns
  (status, value). Through the bytes (8 net steps a call): {1,2,3,4}, n = 4 is `(#0 , #10)`;
  n = 5 is `(#1 , #0)` (out of bounds); {INT_MAX,1,0,0} is `(#1 , #0)` (signed overflow);
  fuel 3 is `(#2 , #0)`; n = 0 is `(#0 , #0)`: the same answers as on the net (28,990 steps a
  call there). The file also prints the residual (`#show λ b n fuel → sum-at b n fuel`):
  41,126 steps to normalize, 14,806 cells. Whole file: 2 m 46 s CPU (9 m 10 s wall on the
  loaded machine, inside `timeout 600` with little to spare; cached afterwards).

## What the specialized code still contains (the input to the binding-time design)

The residual of `sum-at` (the program, the Target and the layout fixed; the array's bytes, n
and the fuel open) has 194 `case`s, 16 `%load64`, 4 `%store64`, 289 `%welim`, and compiles to
283,927 bytes for a loop a C compiler emits in about ten instructions. Everything below
depends only on the program and the layout, and is still there:

A. Left because normalization does not commute a case over a case (in each non-error branch
   the scrutinee's constructor is known, but the outer case is stuck, so the inner one stays):
   1. the `MRes` dispatch after every fallible step (`mok` 231, `mub` 202, `mstop` 208
      occurrences);
   2. the `Ctl` dispatch after every bind in the loop (35 to 40 occurrences of each of
      `c-val c-ret c-break c-continue c-throw`), though nothing in this loop returns, breaks,
      continues or throws;
   3. the `SV` constructor check (`sv-as-i` on a value `sv-decode` has just built as `sv-i`):
      8 six-way cases.
B. Left because inside the loop the memory is a run-time value (it is an argument of the
   loop's induction hypothesis), though its shape (three allocations, their ids, sizes,
   alignments, liveness) is the program's:
   4. the allocation looked up by id on every access (`al-find`: a list walk comparing
      `al-id` with the literals 0, 1, 2; 15 recursive references);
   5. per access: the liveness test, the zeroed test, the alignment test
      `%weq (%wmod (%wadd base off) #4) #0` (8), the bounds equation with the size read from
      the allocation;
   6. the written marks: every scalar load reads a second window (`al-init`) and compares a
      mask; every store writes one (16 `%load64` for 6 scalar loads and 2 stores, 4
      `%store64` for 2 stores);
   7. at every store the allocation's record rebuilt field by field and the allocation list
      rebuilt (`al-replace`), and the stored bytes copied (the primitive store is pure);
   8. the locals `s` and `i` are objects in memory whose address never escapes: 5 of the 6
      loads and both stores of an iteration are theirs (`i` is loaded three times an
      iteration, as C's semantics reads it).
C. Left by the representation:
   9. `toNat` of every offset, `%welim off zero (λ k h → suc h)`: `load64`/`store64` take a
      unary `Nat` offset, so each access counts its offset up from zero;
   10. types: each case's motive carries `Bytes (suc (suc … (%welim size zero …)))` (`by-cap`
       of a run-time size): 268 occurrences of `Bytes (` and 2,432 `suc`. The compiler erases
       them, but they are most of the term it processes;
   11. the access proof's plumbing: at each access a `λ h → …` applied to a path
       `λ (i : I) → …` (16), the proof itself erased;
   12. the Church encoding of `Proc` across the loop: the loop's step is
       `λ k again m R ret h → …`, every exit is a closure `λ R ret h → ret (…)`, and the answer
       type `R` and the handler `h` of a signature with no commands (`NoS`) are passed at run
       time;
   13. primitive conversions by way of `Word`: `%u64ofw (%wofi64 (%load64 …))` at each window
       (17) and `%i64ofw (%wofu64 …)` at each store (4), because the byte layer is `Int64`
       and the scalar layer `UInt64`.
D. A constraint found: the fuel must be a run-time value. At a literal fuel the specializer
   unrolls the loop that many times (the recursion is on the fuel).

What is genuinely run-time and stays: the loads of `p[i]`, the sign extension
`((x & 0xffffffff) ^ 0x80000000) − 0x80000000`, the additions with their overflow tests, the
comparison `i < n`, the bounds test of `p[i]` against the array, the fuel's count.

## Measured

- target.cub + scalar.cub + memory.cub after the cached library: `1017 passed, 0 failed`; with
  layout.cub `1063 passed, 0 failed` (the cached library alone is 767, so 296 checks are this
  track's).
- Net steps of single operations: a load about 600, a store about 800, an 8-byte memcpy about
  9,000 (each byte is a window read and a window write, and a write copies the allocation's
  bytes: stores are pure).

## Found

- `u64 w` (and `%u64ofw`) on a `Word` literal ≥ 2⁶⁴ does not reduce: `#show u64 #18446744073709551617`
  prints `%u64ofw #18446744073709551617`. A conversion from `Word` must reduce modulo 2⁶⁴ first.
- `u64shl`/`u64shr` take the count modulo 64 (`u64shl (u64 #1) (u64 #64)` is `#u64:1`), so a
  shift by 64 or more must be handled by the caller (it matters for the 128-bit kinds).
- The same for `f64`: `f64 #18446744073709551616` stays `%f64ofw #18446744073709551616`, and a
  `case` on a comparison with it is stuck. Class: every `u64 #n`, `i64 #n`, `f64 #n` in lib/mem
  has n < 2⁶⁴ (2⁶⁴ as a Float64 is `f64mul (f64 #4294967296) (f64 #4294967296)`).
- Layout columns are counted in bytes, not characters: in `true → (let a = …` followed by a
  second binding on the next line aligned under `a` by eye, the `→` (3 bytes) puts `a` two
  columns right of where it looks, and the parser reports `expected in, found <name>`; a
  multi-line `case` after `→ (let x = (case …` fails with `error: (case e branch...)` and no
  position. Written around: a multi-binding `let` starts on its own line.
- Structural recursion is accepted only in one shape, and the messages do not say which:
  `f o n (cons b bs) a = … f o (wsub n #1) bs …` (an argument before the matched one changes)
  is refused with "a recursive call is not on a constructor field or a field's field", though
  `bs` is a field; a partial application `f bs o n` passed as a function is refused with "a
  recursive call must give every argument after the one it recurses on". Accepted: the matched
  argument first, the call fully applied (under a λ is fine).
- A catch-all branch that returns its scrutinee (`case r of mstop w → …; r2 → r2`) fails with
  "no inferable type here: annotate it"; written out per constructor it checks.
- A record literal bound by `let` without a signature (`let a = Alloc { … }`) fails the same
  way; `a : Alloc` on its own line before it fixes it.
- `#show uq-sx` (a definition by `case` on a data type, applied to nothing) prints
  `λ x0 x1 → <cprim>`: the printer has no form for what is inside.
- `rem` is not usable as a local name (`expected in, found rem`).
- There is no `Float32` arithmetic and no `Float32`↔`Float64` conversion among the primitives
  (only the bits). Both are written on the bits in scalar.cub, so nothing is missing from the
  semantics; the compiled code for a `float` operation is therefore integer code (see "Needs").

## Limits

- `long double` (x87 80-bit, or binary128) is not a kind: no primitive carries it, and `CTy`
  has no constructor for it. Adding it means a 16-byte value (`DQ`) with its own `fp-pack`
  format and arithmetic written on the bits.
- Floating-point operations follow IEEE 754 with round-to-nearest-even and no exception flags;
  the rounding mode and `fenv` are not state of the machine.
- `load64`/`store64` are the host's: lib/prim.cub does not state their byte order. memory.cub
  reads the 8-byte window as a little-endian number (x86-64); on a big-endian host the same
  file would give `mem-load` at 1, 2 and 4 bytes the wrong bytes. A Target's byte order is
  independent of this and is honoured (`en-fix`).
- Unions: every member is at offset 0 and the active member is NOT tracked (v1): reading a
  member other than the one last written gives the bytes reinterpreted (C's rule) instead of
  C++'s undefined behaviour. Tracking it would add to `Alloc` a map from a union object's
  offset to the name of its active member, set by `obj-store` through a union field and
  checked by `obj-load`; the per-byte type tag below would subsume it.
- Subobject bounds are not enforced: `mem-ptr-add` checks against the whole allocation, so a
  pointer walked past the end of a member array into the next member is not reported. It
  would need the pointer to carry the subobject's span (two more words in `mp-obj`).
- `lay-struct` is C's rule only: no bit-fields, no C++ base-class layout (empty-base
  optimization, tail-padding reuse, virtual bases), no `[[no_unique_address]]`, no packed or
  over-aligned members. Such layouts are given as data (Clang's) and judged by `lay-ok`;
  virtual bases have no representation in `Layout` (their offset is in the vtable).
- Types of objects in memory are not tracked: there is no effective type, so strict-aliasing
  violations are not undefined here (a `float` stored and an `int` loaded gives the bits). It
  would add a per-byte type tag to `Alloc` and a check in `mem-load-al`.
- An indeterminate value is not a value: an 8-bit load of a byte never written returns the
  byte the allocation holds (0 for a fresh one) instead of an indeterminate byte that would
  make later arithmetic on it undefined. It would add an `sv-indet` constructor to `SV`.
- Allocation never fails and addresses are never reused; the address space is unbounded
  (`Word`), except that a pointer stored at a 4-byte pointer size must have an address below
  2^32 (else `mstop st-unsupported`).
- A function pointer's address is its id + 1 and must be below `tg-base` (65536 in the three
  Targets), which bounds the number of functions.
- A memory operation costs a walk of the allocation list (`al-find`), and a store copies the
  allocation's bytes (the primitive store is pure).
- Pointer equality treats pointers into different allocations as different: "one past the end
  of A equals the start of B" (unspecified in C++, possible on real hardware) never holds.

## Next step

Every acceptance line is met; nothing is in progress. Final measurement to repeat after any
change to a library file: `make library`, then the "How to check" command without the cache
(`CUBICAL_NOCACHE=1`), then `experiments/mem/sum.cub` after the five files (about 3 minutes of
CPU for its `#jit`). If the track continues, the order of value is: (1) with track A, the
items of "What the specialized code still contains" (A needs case-of-case; B needs the memory's
shape static in loops, e.g. allocations addressed by position in a known list or locals held
outside the memory when their address does not escape; C needs the primitives named under
"Needs"); (2) the Limits, in the order: effective types (which subsumes the union's active
member), indeterminate values, subobject bounds, C++ base-class layout computation,
`long double`.
