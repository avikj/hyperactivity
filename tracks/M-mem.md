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

## Measured

- target.cub + scalar.cub after the cached library: `949 passed, 0 failed` (the cached library alone is 767, so 182 checks are this track's).

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
- Pointer equality treats pointers into different allocations as different: "one past the end
  of A equals the start of B" (unspecified in C++, possible on real hardware) never holds.

## Next step

Write `lib/mem/memory.cub`: `Alloc` (size, alignment, live, kind, base address, `Bytes` data,
`Bytes` initialized mask), `Memory` (a list of allocations and the next id/address), `mem-alloc`,
`mem-free`, `mem-load t k p`, `mem-store t k p v` (byte order from the Target), `mem-memcpy`,
`mem-memmove`, `mem-memset`, `ptr-add`, `ptr-diff`, `ptr-to-int`, `int-to-ptr`. Byte access goes
through `load64`/`store64` of lib/prim.cub with the proof built from a decided bound.
