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

(nothing yet)

## Found

## Next step

Write `experiments/cxx/book.cpp` and `book_main.c`, confirm they compile and run with clang,
commit. Then dump the JSON once, list the node kinds reachable from `on_message`, and commit
that list here before writing the importer.
