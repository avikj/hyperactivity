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

## How to check (every file of this track)

```
./experiments/cxx/sem/regen.sh        # the AST and the units with CBinOp renamed (Found 1)
LIB=$(make -s -n library | sed -n 's/^.*cubical \(lib\/prelude.*\)$/\1/p')
MEM="lib/mem/target.cub lib/mem/scalar.cub lib/mem/memory.cub lib/mem/layout.cub lib/mem/monad.cub"
(ulimit -v 3000000; timeout 900 ./build/cubical $LIB $MEM experiments/cxx/sem/gen/ast.cub lib/cxx/machine.cub | tail -3)
```
Files load in this order: the library, `$MEM`, `experiments/cxx/sem/gen/ast.cub`,
`lib/cxx/machine.cub`, then (as they are written) `lib/cxx/read-expr.cub`,
`lib/cxx/read-stmt.cub`, `lib/cxx/read.cub`, a unit from `experiments/cxx/sem/gen/`, and a file
of readings from `experiments/cxx/sem/`. Names of this track are prefixed `cx-`/`Cx`.

## Design (what a fresh reader of the code needs first)

- A reading is a computation in `lib/mem/monad.cub`'s `Ev` at the signature `CxS`, whose one
  command is a refusal carrying the AST node refused (`cx-unsupported (cxw-expr e)`); the
  command has no response (`Empty`), so the outcome of a run (`cx-run : … → CxOut`) is
  `cxo-ok | cxo-ub why | cxo-stop why | cxo-unsupported what`, and "unsupported" names its node.
- Expressions are read in continuation-passing style over `Ev` (`CxK = CxR → CxEv CxR`): a
  temporary materialized in a full-expression lives for the extent of the continuation it is
  handed to, so it is destroyed, in reverse order of construction, when the full-expression's
  continuation returns ([class.temporary]/4) with no dynamic list of temporaries. A
  lifetime-extended temporary leaves the full-expression as a wrapper on its answer
  (`cxr-ext`), which the declaration it initializes takes off and ends at the end of the block.
- The result of an expression is a `CxR`: a scalar value, a glvalue (address and the alignment
  its accesses may assume), a bit-field glvalue, a bound member function, a pointer to member
  function, nothing (void; a class prvalue has initialized the object it was given).
- What the standard leaves open is a field of `CxU`: the Target, the layouts (the unit's),
  `cxu-ltr` (order of unsequenced operands and of arguments), `cxu-parm-full` (when a
  parameter's lifetime ends, [expr.call]/6), the loop fuel and the call fuel.

## Needs from track A

(none yet)

## Needs from track C

1. `tools/clang2cub.py`'s `SCHEMA`: rename `CBinOp`'s constructors (`bo-mul` … `bo-ptr-mem-i`)
   so that they do not collide with `lib/mem/scalar.cub`'s `BinOp` (`bo-add bo-sub bo-mul bo-div
   bo-rem bo-and bo-or bo-xor`): `lib/cxx/ast.cub` and `lib/mem/scalar.cub` cannot be loaded
   together (`error: declared again (a name is bound once): bo-mul`). This track uses `cbo-…`
   (`experiments/cxx/sem/regen.sh` writes renamed copies of `lib/cxx/ast.cub` and the three
   units into `experiments/cxx/sem/gen/`); with the rename in the importer the copies and the
   script are deleted and the load line names track C's files.
2. A reader for `CStr` beside `cstr-put` (`lib/cxx/machine.cub` declares its own
   `primitive cx-byte` = `%load8`, one more trusted line): the reader has to recognise by name
   the functions the unit declares without a body (`memcpy`, `operator new`, …) and the
   operator a `CXXOperatorCallExpr` calls (its sequencing rule depends on it).

## Needs from track M

1. `mem-release`/`mem-free` keep the dead allocation in `mem-allocs` for ever and `al-find`
   walks the list, so every access costs steps in proportion to the number of allocations ever
   made (a function call makes several). Request: take an allocation off the list when its
   storage ends and report an id below `mem-count` that the list lacks as `ub-dead` in
   `mem-at` (ids are never reused, so nothing is lost; `mem-addr`/`al-of-addr` of a pointer
   into an ended allocation then need a rule: [basic.stc.general]/4 makes any other use of an
   invalid pointer value implementation-defined). Worked around in `lib/cxx/machine.cub`
   (`cx-release`, `cx-free` drop the allocation; `cx-dead` turns lib/mem's "no such
   allocation" stop into `ub-dead` for an id below the count), so in this track's readings a
   pointer into an ended allocation that was stored in memory and loaded again decodes as
   `mp-addr` and an access through it is `ub-out-of-bounds` rather than `ub-dead`.

## Done

- `experiments/cxx/sem/regen.sh` and `experiments/cxx/sem/gen/{ast,book,second,third}.cub`
  (Found 1).
- `lib/cxx/machine.cub` (572 lines; loads after the library, lib/mem and the renamed AST with
  0 failed): outcomes and the refusal signature (`CxWhat`, `CxS`, `CxR`, `CxEv`, `CxK`,
  `CxOut`, `cx-run`); names (`cx-byte`, `cx-name-is`); the unit's tables (`cx-find-fn`,
  `cx-find-dtor`, `cx-find-rec`, `cx-find-field`, `cx-field-any`, `cx-cap-var`, `cx-cap-this`,
  `cx-enum-under`, `cx-enumerator`, `cx-find-var`); types (`cx-strip`, `cx-tc`, `cx-mty :
  CType → MTy`, `cx-layenv` the unit's layouts as a `LayEnv`, `CxU`, `cx-unit`, `cx-size`,
  `cx-align`, `cx-scalar`); memory accesses with ended allocations dropped (`cx-load`,
  `cx-store`, `cx-alloc`, `cx-release`, `cx-free`, `cx-memmove`, `cx-memcpy`, `cx-memset`,
  `cx-ptr-add`, `cx-ptr-diff`, `cx-ptr-cmp`, `cx-conv`, `cx-local`, `cx-sub`, `cx-super`);
  literal values (`cx-lit`, `cx-cint`); the environment (`CxBind`, `CxG`, `CxEnv`, `CxCall`).

## Found

1. `lib/cxx/ast.cub` (track C) and `lib/mem/scalar.cub` (track M) both bind `bo-add bo-sub
   bo-mul bo-div bo-rem bo-and bo-or bo-xor`; loaded together the second is refused. See
   "Needs from track C" 1.
2. `data List (A : Type)` is at level 0 only and a `data` declaration with a field of a
   `Type 1` type (`data T = c (f : Ev {lzero} S V A)`) fails with `error: unbound name %imp`:
   computations cannot be stored in data. The reader therefore never builds a list of
   meanings; an argument list is evaluated by the one recursive function in a mode that
   carries the parameter types.
3. A definition recurses structurally only through its own clauses' patterns: `f t = case t
   of c u → f u` is refused ("a recursive call is not on a constructor field …"); written as
   clauses `f (c u) = f u` it checks.
4. `lam` and `code` cannot be used as pattern variable or field names ("a keyword cannot be
   bound").
5. Under `--serve`, a `data` declaration that fails part-way (the keyword error above) leaves
   its name bound: the corrected declaration is refused as "declared again", and `#retract` of
   it reports `retracted data …` yet the redeclaration is still refused. A `primitive` cannot
   be retracted either ("declared again" after `#retract`).

## Limits

(Constructors of the AST not yet read are every one: the expression, statement and unit
readers are not written yet.)

## Next step

Write `lib/cxx/read-expr.cub`: `cx-expr : CExpr → CxCall → CxU → CxEnv → CxMode → CxK → CxEv CxR`,
one structural recursion over `CExpr` (nodes and `ce-nil`/`ce-cons` lists), each case a call
of a non-recursive helper that takes the sub-expressions' meanings; modes `cxm-get` (value or
glvalue by category), `cxm-init p` (initialize the object at p: class and array prvalues),
`cxm-args` (an argument list against parameter types). Then `lib/cxx/read-stmt.cub`
(`cx-stmt`), `lib/cxx/read.cub` (`cx-fun` by call fuel, static initialization, `cxx-entry`),
then `experiments/cxx/sem/sum.cpp`.
