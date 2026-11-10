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

(nothing yet)

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

## Found

- `u64 w` (and `%u64ofw`) on a `Word` literal ≥ 2⁶⁴ does not reduce: `#show u64 #18446744073709551617`
  prints `%u64ofw #18446744073709551617`. A conversion from `Word` must reduce modulo 2⁶⁴ first.
- `u64shl`/`u64shr` take the count modulo 64 (`u64shl (u64 #1) (u64 #64)` is `#u64:1`), so a
  shift by 64 or more must be handled by the caller (it matters for the 128-bit kinds).

## Limits

(none recorded yet)

## Next step

Continue `lib/mem/scalar.cub` after `int-cmp`: floats (`fp-pack`: an integer magnitude times a
power of two rounded to nearest-even into binary32/binary64 bits; f32↔f64; f32 arithmetic through
f64; float↔integer conversions with `ub-float-int-range`), then `sv-conv` (every pair of kinds),
`sv-arith`/`sv-cmp`/`sv-shl`/`sv-shr`/`sv-neg`/`sv-not` by kind, pointer comparison, then the C
types `CTy` with promotions and the usual arithmetic conversions as functions of `Target`.
