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

## Needs from track A
## Done
## Found
## Limits

## Next step

Read the files named above, then write `lib/mem/target.cub` and `lib/mem/scalar.cub`
(conversions first), commit when they check.
