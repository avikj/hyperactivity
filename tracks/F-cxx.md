# Track F — C++ semantics: the reader from Clang's AST into terms (D3, D4)

Read `README.md` here (rules), `../PLAN.md` §5 (all five distinctions) and §6 Phase 4, then
`tracks/C-clang.md` (the AST and what the importer guarantees), `lib/cxx/ast.cub`,
`tracks/M-mem.md` and `lib/mem/*.cub` (targets, scalars with undefined behaviour as outcomes,
memory, layouts, the evaluation monad and its control outcomes), `experiments/cxx/*.cpp` and
their `.cub` imports, `experiments/mem/sum.cub` (a C function's meaning written by hand in
the monad: your reader must produce terms of that shape), and `tracks/A-compiler.md` (what
`#jit` and `#build` accept).

Files you own: `lib/cxx/read*.cub`, `lib/cxx/machine*.cub`, `experiments/cxx/sem/**`, this file.

## What is built

`read` is the definition of the meaning of a C++ translation unit (after Clang's Sema) at the
abstract machine with its implementation parameters (`Target`, the layouts the importer
recorded). There is nothing to check `read` against at this layer: it IS the meaning assigned,
so write it to the standard's wording, and where the standard leaves a choice, take it from a
parameter. Structure (PLAN §5.4, binding time): `read` is structural recursion on the AST
(program data: static), producing a term in `lib/mem/monad.cub`'s monad whose only run-time
content is loads, stores, scalar operations and control; types, layouts, offsets, cast kinds,
overload choices are all consumed by case on the AST.

1. **`lib/cxx/machine.cub`**: the environment of a reading (declaration id → what it denotes:
   a local's allocation, a global's, a function, a field offset, an enumerator's value), the
   mapping `CType → MTy` (lib/mem/layout.cub) using the unit's recorded layouts, lvalues as
   pointers, the calling convention *of the abstract machine* (parameters are fresh objects
   initialized from arguments; the result object; temporaries with their lifetimes ending at
   the full expression), and fuel: recursion and loops take a `Word` fuel that is a parameter
   of the meaning (`out-of-fuel` an outcome distinct from undefined behaviour).
2. **`lib/cxx/read-expr.cub`**: every `ce-…` constructor of the AST: literals; `DeclRef`;
   member access (`.`/`->`, through bases); unary and binary operators and compound
   assignment over lib/mem/scalar.cub (usual arithmetic conversions already explicit as
   implicit casts in the AST: implement each **cast kind** the importer emits:
   LValueToRValue, IntegralCast, IntegralToBoolean, FloatingCast, IntegralToFloating, NoOp,
   ArrayToPointerDecay, FunctionToPointerDecay, NullToPointer, BitCast, DerivedToBase,
   UncheckedDerivedToBase, ConstructorConversion, UserDefinedConversion, …, exactly the list
   in C's track file); `?:`, comma, `sizeof`/`alignof`; calls (direct, member, operator, through
   a function pointer, **virtual** through the vtable pointer stored in the object and the
   unit's vtables); constructors and destructors (member initializers, base construction
   order, destruction in reverse at scope exit and at the end of the full expression for
   temporaries); `new`/`delete`; lambdas (closure object with captures); `this`;
   initializer lists; array subscripts and pointer arithmetic; `ConstantExpr` by its recorded
   value; `__builtin_…` the units use.
3. **`lib/cxx/read-stmt.cub`**: compound statements with scopes and destructors, declarations
   with initializers, `if`, `switch` (case labels, fallthrough, default), `for`, range-`for`
   (as Clang desugars it), `while`, `do`, `break`, `continue`, `return`, expression
   statements, null statement.
4. **`lib/cxx/read.cub`**: functions and the unit: `cxx-fun : CTU → (id : Word) → args → M Val`
   and `cxx-entry` for an `extern "C"` function taking scalars and pointers into a caller-
   provided memory; static storage initialization (constant-initialized statics from the
   unit; dynamic initialization order within the unit).
5. An AST constructor the reader does not handle is a **checked refusal naming it**
   (`unsupported ce-…` outcome) and is listed under Limits until written; none may be silently
   mapped to something else.

## Acceptance

- `make library` passes; `lib/cxx/*.cub` load after it with `N passed, 0 failed`, no postulates.
- **On the net**: `cxx-entry book-tu on_message` applied to the bytes of each of 1,000
  messages (generated as `experiments/cxx/book_main.c`'s driver does; `experiments/rust/
  gen_messages.py` writes them) gives the outcome `ok v` with `v` the value the Clang-built
  binary prints, for every message. This is a reading of the term on recorded inputs,
  recorded with steps per message. Any `undefined`/`unsupported`/`out-of-fuel` outcome is a
  finding: either the unit has undefined behaviour on that input (say which rule) or the
  reader is wrong.
- Same for `second.cpp` and `third.cpp`'s entry points on their drivers' inputs.
- **Compiled (D3)**: `#jit` (or `#build` with a driver process reading messages) of
  `cxx-entry book-tu on_message` specialized at the unit: record whether it compiles, its
  size, and its time per message against the Clang -O2 binary on 10⁶ messages. What the
  specialized code still interprets (AST cases, environment lookups, layout arithmetic left
  at run time) is listed with its cause: that list is the binding-time work, and it is done
  when the residual has none of it. Compiler refusals go under "Needs from track A" with the
  smallest input.
- **Properties (D4)**, each a checked statement about the read term in
  `experiments/cxx/sem/props.cub`: `on_message` never has undefined behaviour on any
  well-sized input (state the precondition; prove by the structure of the code, bounded over
  message kinds if the book's state space forces a bound, and say the bound); encoding then
  decoding a message is the identity; after any sequence of add/cancel/execute the book's
  best bid is the maximum of live bids.

## Needs from track A
## Done
## Found
## Limits

## Next step

Read everything listed at the top. Write `lib/cxx/machine.cub` (environment, `CType → MTy`,
lvalues) and read `sum`-like C first: a scratch unit `experiments/cxx/sem/sum.cpp` imported with
`tools/clang2cub.py`, its meaning by `read` equal (a checked path or equal readings on 10
inputs) to `experiments/mem/sum.cub`'s hand-written one. Commit. Then expressions by cast
kind and operator, in the order `book.cpp` needs them.
