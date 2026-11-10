# Track C — C++ as data: Clang's AST into the checker (first half of D3)

Read `README.md` here first (rules), then `../PLAN.md` §5 and §6 Phase 4.

Files you own: `tools/clang2cub.py`, `lib/cxx/ast.cub`, `experiments/cxx/**`, this file.

## What is built

1. **`experiments/cxx/book.cpp`** (write it; ~200 lines; it is the acceptance input for D3):
   the hot path of a feed handler in the style trading code is written, compiled with
   `-std=c++20 -O2 -fno-exceptions -fno-rtti`: fixed-width integers; a packed wire message
   decoded from `const unsigned char*` with `memcpy`; an `enum class Side`; a
   `template <std::size_t N> class Book` with a `std::array` of price levels, add/cancel/
   execute methods, best bid/ask; a small class hierarchy with one virtual call (a
   `Handler` interface); references, const, inline functions, a lambda, a range-for, operator
   overloading on a `Price` struct, `static_cast`, integer conversions, a `switch`.
   `extern "C" long on_message(const unsigned char* p, long n)` as the entry. No I/O inside.
   Also `experiments/cxx/book_main.c`: reads messages from stdin and calls `on_message`
   (built with clang, it is the reference binary for the latency comparison).
2. **`tools/clang2cub.py`**: runs
   `clang++ -std=c++20 -fno-exceptions -fno-rtti -fsyntax-only -Xclang -ast-dump=json`
   (no Clang headers or Python bindings are installed; the JSON dump and
   `-Xclang -fdump-record-layouts-complete` are the sources) and writes one `.cub` file: the
   translation unit as a value of the AST type, **after Sema**: names resolved to declaration
   ids, overloads chosen, templates instantiated (take the instantiations, i.e. the
   `ClassTemplateSpecializationDecl` / specialized `FunctionDecl` bodies; skip the uninstantiated
   patterns), implicit casts and constructor calls explicit, constant expressions with their
   values (`ConstantExpr`'s `value`).
   - Only declarations reachable from the entry points are emitted (the headers bring in
     thousands that are not).
   - **Types are structured**, not strings: parse `qualType`/`desugaredQualType` into
     `builtin kind | pointer T | lvalue-ref T | rvalue-ref T | array T n | record id | enum id | function …`
     with const/volatile bits. Where the JSON gives only a string, parse the desugared
     string; a type the parser cannot read stops the import with the string printed (extend
     the parser; never emit an "unknown" node).
   - **Layouts**: for every record emitted, its size, alignment, each field's offset, the
     vtable pointer's offset, base offsets, from the record-layout dump. These are the
     implementation parameters of PLAN §5.1 for this target.
   - Value categories and cast kinds are kept (`valueCategory`, `castKind`).
3. **`lib/cxx/ast.cub`**: the AST type as plain first-order `data` (so a reader can recurse
   structurally): `CType`, `CExpr`, `CStmt`, `CDecl`, `CTU`, with ids as `Word`, integer
   literals as `Word` plus a sign/width where needed, strings as `Bytes n` packed in a Σ, lists
   as the library's `List`. One constructor per Clang node kind **that the importer has met**
   (the generator prints the kinds it met; an unmet kind is not a constructor yet). Keep the
   constructor names prefixed (`ce-`, `cs-`, `cd-`, `ct-`) so they collide with nothing.

## Acceptance

- `python3 tools/clang2cub.py experiments/cxx/book.cpp -o experiments/cxx/book.cub` runs, and
  loading the library, `lib/cxx/ast.cub`, then `experiments/cxx/book.cub` ends
  `N passed, 0 failed` with `book-tu : CTU` checked. Record: node kinds met (the list), nodes
  emitted, declarations emitted, records with layouts, steps and seconds to check.
- A second unit that adds what the first lacks from this list also imports: a class template
  with a non-type and a type parameter instantiated twice, a constructor with a member
  initializer list, a destructor, `new`/`delete`, a pointer to member function call, a
  `union`, bit-fields, a `do`/`while`, `goto`-free `break`/`continue`, `sizeof`/`alignof`,
  compound assignment and `?:`, a C array of structs, string literal, `std::span` or pointer
  arithmetic, 128-bit multiply via `__int128` or `__builtin_mul_overflow`.

## Needs from track A

(none yet)

## Done

- `experiments/cxx/book.cpp` (207 lines) and `experiments/cxx/book_main.c` written. Builds clean with
  `clang++ -std=c++20 -O2 -fno-exceptions -fno-rtti -Wall -Wextra -c book.cpp` (clang 18.1.3) and
  `clang -O2 book_main.c book.o`. On 1,000,000 random 22-byte messages (50% add, 25% cancel,
  ~25% execute, 0.1% reset, prices 994..1006) the reference binary answers `sum -11432484` at
  about 71 ns/message on this (shared, 2-core) machine.

## Found

- The checker has no mutual `data` (`mutual` followed by `data`: "unexpected data") and rejects a
  field `List D` inside `data D` ("the field type is not strictly positive (the type itself at
  its own parameters, or not mentioning it)"). So `CExpr`/`CStmt`/`CDecl` cannot be mutually
  recursive and a node's children cannot be a `List` of the same type. `mutual` over functions
  is for codata only ("a definition of a corecursive group is λs and lines over a record").
  What does check (measured on a trial file): a type that carries its own list constructors
  (`te-nil | te-cons (x : TE) (xs : TE)`), and one function recursing structurally through
  both the node and the list constructors.
- There is no byte-string literal and no `store8` in `lib/prim.cub`. `%store8 b off v` with a
  `Word` offset and a `UInt32` value works inside a `primitive` and is bounds-checked by the
  net (an out-of-range store is a no-op), so a string can be built from a list of byte words.
- A definition `f : (n : Nat) → Bytes n → Word → List Word → Bytes n` recursing on the list in
  the fourth position was rejected as not structural; with the list second (before the
  arguments that change) it checks.
- `length` of `lib/prelude.cub` is "not a bound variable or checked definition" for a
  definition loaded after the library (prelude definitions are not in the checked table).

## Next step

Dump the JSON once (`clang++ -std=c++20 -fno-exceptions -fno-rtti -fsyntax-only -Xclang
-ast-dump=json experiments/cxx/book.cpp`), list the node kinds reachable from `on_message`,
and commit that list here before writing the importer (`tools/clang2cub.py`).
