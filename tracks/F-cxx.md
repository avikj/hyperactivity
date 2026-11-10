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

(Restated after track A's correction of 2026-11-09, tracks/README.md "What is evidence here" and
"Memory is a signature": `read` is the definition of the meaning; agreement with a Clang-built
binary on sample inputs is not evidence and is recorded under Diagnostics.)

- `lib/cxx/*.cub` load after the library with `N passed, 0 failed`, no postulates.
- Every AST constructor the three units use is read: Limits is empty for them. A constructor
  not read is a refusal naming it (`cxo-unsupported`), never mapped to something else.
- The reader never touches a memory value: its term is over the memory operations only
  (pointers abstract). Its reading under track M's `mem-std` handler is the meaning; the same
  term under `mem-flat` is what is compiled.
- **Compiled (D3)**: `cxx-entry book-tu on_message` specialized at the unit, under `mem-flat`,
  compiles through `#jit`/`#build` with a residual free of interpretation (no AST cases, no
  environment lookups, no layout arithmetic at run time). What the residual still interprets is
  listed with its cause until there is none. Compiler refusals: "Needs from track A".
- **Properties (D4)**: checked theorems about the read term under `mem-std` in
  `experiments/cxx/sem/props.cub`, each including "no undefined behaviour" (the hypothesis of
  track M's theorem, by which they hold of the binary): `on_message` has no undefined behaviour
  on any well-sized input (the precondition stated; any bound stated); encoding then decoding a
  message is the identity; after any sequence of add/cancel/execute the best bid is the
  maximum of live bids.
- The latency of the compiled entry against the Clang -O2 binary on 10⁶ messages is a
  measurement, recorded.

## Diagnostics (bring-up only; not acceptance, not evidence)

Readings of `cxx-entry` under `mem-std` on recorded inputs, beside what the Clang-built binary
prints for the same inputs: `on_message` on the messages of `experiments/rust/gen_messages.py`,
and `second.cpp`'s and `third.cpp`'s entries on inputs of drivers in `experiments/cxx/sem/`. A
disagreement, or an `undefined`/`unsupported`/`out-of-fuel` outcome, is a bug in `read` or
undefined behaviour in the unit: find which. Recorded with steps per input.

## How to check (every file of this track)

```
./experiments/cxx/sem/regen.sh        # the AST and the units with CBinOp renamed (Found 1)
LIB=$(make -s -n library | sed -n 's/^.*cubical \(lib\/prelude.*\)$/\1/p' | sed 's| lib/cxx/ast.cub||')   # Found 1
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

The reader uses only the operations of the section "memory: the commands the reader uses" of
`lib/cxx/machine.cub`. Named in `lib/mem/monad.cub` today and used as they are: `ev-load`,
`ev-store`, `ev-alloc`, `ev-release`, `ev-ptr-add`, `ev-bind`, `ev-finally`, the control
outcomes and the loops. Needed and **not named yet** (each is written once in that section over
`lib/mem/memory.cub`'s function with `ev-rd`/`ev-wr`, and becomes the command when it exists):

1. `load`/`store` at a given alignment (`mem-load-al`, `mem-store-al`): a member of a packed
   record is accessed at less than its kind's alignment.
2. `free` (`mem-free`: null is a no-op), `memmove`, `memcpy`, `memset`.
3. `ptr-diff`, `ptr-cmp` (`mem-ptr-cmp`), and the scalar conversion that may involve a pointer
   (`mem-conv`: pointer ↔ integer by the Target).
4. Pointer values as abstract constants and a destructor: the null pointer, the pointer to
   function number f, and "the function a pointer is to" (undefined when it is to none): an
   indirect or virtual call needs it. (`cx-null`, `cx-fn-ptr`, `cx-fn-of`.)
5. A refusal carrying data: `Stop`'s `st-unsupported` names nothing. The reader's signature
   `CxS` has the refusal as its one command (response `Empty`); when `Ev` becomes a process
   over memory commands, the reader's signature must be the sum of the memory commands and
   this one (or the memory signature has a refusal command carrying a reader's value).
6. (cost, a matter of the `mem-std` handler) `mem-release`/`mem-free` keep the ended allocation
   in `mem-allocs` for ever and `al-find` walks the list, so an access costs steps in
   proportion to the number of allocations ever made; every call makes several. Request: take
   an allocation off the list when its storage ends and report an id below `mem-count` that the
   list lacks as `ub-dead` (ids are never reused).

## Done

- `experiments/cxx/sem/regen.sh` and `experiments/cxx/sem/gen/{ast,book,second,third}.cub`
  (Found 1).
- `lib/cxx/machine.cub` (about 560 lines; loads after the library, lib/mem and the renamed AST with
  0 failed): outcomes and the refusal signature (`CxWhat`, `CxS`, `CxR`, `CxEv`, `CxK`,
  `CxOut`, `cx-run`); names (`cx-byte`, `cx-name-is`); the unit's tables (`cx-find-fn`,
  `cx-find-dtor`, `cx-find-rec`, `cx-find-field`, `cx-field-any`, `cx-cap-var`, `cx-cap-this`,
  `cx-enum-under`, `cx-enumerator`, `cx-find-var`); types (`cx-strip`, `cx-tc`, `cx-mty :
  CType → MTy`, `cx-layenv` the unit's layouts as a `LayEnv`, `CxU`, `cx-unit`, `cx-size`,
  `cx-align`, `cx-scalar`); the memory operations the reader is confined to (`cx-load`, `cx-store`,
  `cx-alloc`, `cx-release`, `cx-ptr-add` = the monad's; `cx-load-al`, `cx-store-al`, `cx-free`,
  `cx-memmove`, `cx-memcpy`, `cx-memset`, `cx-ptr-diff`, `cx-ptr-cmp`, `cx-conv`, `cx-null`,
  `cx-fn-ptr`, `cx-fn-of` = Needs from track M 1–4; `cx-local`, `cx-sub`, `cx-super` over them);
  literal values (`cx-lit`, `cx-cint`); the environment (`CxBind`, `CxG`, `CxEnv`, `CxCall`).

- `lib/cxx/read-expr.cub` (about 1,180 lines, every definition checks after machine.cub):
  `cx-expr : CExpr → CxH → CxU → CxEnv → CxMode → CxK → CxEv CxR`, one structural recursion with
  a clause for each of the 39 `CExpr` constructors, none of them a refusal by default (what
  each clause refuses is listed under Limits). Rules below it: scalar objects (`cx-ld`,
  `cx-st`), bit-fields (`cx-bf-load`, `cx-bf-store`), pointers to member functions in memory
  (`cx-mfp-load/store`), `cx-lv-load`/`cx-lv-store`, zero values, conversions (`cx-cv`),
  bases and vtables (`cx-path-off`, `cx-vptr-off`, `cx-slot`, `cx-virtual`), destruction
  (`cx-destroy`), the built-in operators (`cx-binop`, `cx-incdec`, `cx-cassign`, `cx-unop`,
  `cx-ptr-mem`, `cx-subscript`), references to declarations (`cx-declref`, `cx-member`,
  `cx-this`), result objects and temporaries (`cx-with-dest`, `cx-materialize`, `cx-cut`),
  initializer slots (`cx-init-slot`, `cx-rec-slots`, `cx-init-list`, `cx-lambda`), calls
  (`cx-arg`, `cx-call-fn`, `cx-apply`, `cx-call`, `cx-mcall`, `cx-opcall`, `cx-construct`,
  operator sequencing `cx-op-ltr`), the functions without a body (`cx-builtin`: memcpy,
  operator new/new[]/delete/delete[] and placement new, `__builtin_{add,mul}_overflow`),
  casts by kind (`cx-cast`: all 18 kinds), `cx-new`, `cx-delete`, `cx-trait`, `cx-string`.
  Not yet run on any unit (statements and functions are next).

## Found

1. `lib/cxx/ast.cub` (track C) and `lib/mem/scalar.cub` (track M) both bind `bo-add bo-sub
   bo-mul bo-div bo-rem bo-and bo-or bo-xor`; loaded together the second is refused. Since
   `lib/cxx/ast.cub` joined `make library`'s list (commit 26c6a2df or before), `lib/mem/scalar.cub`
   no longer loads after "the library" at all (`error: declared again … bo-add`): this track's
   load line removes `lib/cxx/ast.cub` from the list and loads the renamed copy after lib/mem.
   See "Needs from track C" 1.
0. (the specification, as track A asked to be told) Besides the comparison with the Clang
   binary, the original "Next step" asked that `read` of a scratch `sum.cpp` be "equal (a
   checked path or equal readings on 10 inputs)" to `experiments/mem/sum.cub`'s hand-written
   meaning: equal readings between two descriptions is the first form of README's audit. It is
   treated as a diagnostic (the hand-written term is not a second definition of anything: only
   `read`'s term is the meaning).
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

Refusals `cx-expr` can give for a constructor it reads (forms no sample unit uses unless
said), each named in the outcome:
- `cxw-form #1`: `==`/`!=` of pointers to member functions.
- `cxw-form #2`: a constructor call whose type is an array of classes.
- `cxw-form #3`: `delete[]` of an array whose element type has a destructor to run (needs the
  element count `new[]` kept; no array cookie is written).
- `cxw-form #4`: `new` with a user-defined allocation function (one with a body).
- `cxw-form #5`: a string literal of a character type wider than one byte.
- `cxw-builtin id`: a function without a body other than the eight named above.
- `mstop st-unsupported`: a bit-field on a big-endian Target or spanning more than 8 bytes; a
  128-bit array index; a negative array-new bound (std::bad_array_new_length needs
  exceptions); a virtual base on a cast path.
- Value-initialization of a class without a constructor zeroes its bytes: a pointer to data
  member inside it then reads as offset 0, not null (-1, Itanium ABI): [dcl.init]/6 wants
  each member zero-initialized.
- Unsequenced conflicting side effects on a scalar ([intro.execution]/10) are not detected as
  undefined: operands are evaluated in the order `cxu-ltr` gives.
- Reading an uninitialized bit-field is not undefined here (lib/mem allows uninitialized reads
  at 8-bit kinds, which the bit-field's bytes are read at).
- A virtual call does not adjust `this` (the importer admits only single inheritance with the
  polymorphic base primary at offset 0, so no thunk exists in the units).
Statements, functions, static initialization: not written yet, so no unit is read yet.

## Next step

Write `lib/cxx/read-stmt.cub`: `cx-stmt : CStmt → CxH → CxU → CxEnv → (CxEnv → CxEv Unit) → CxEv Unit`
(a declaration extends the environment for the rest of its block: the continuation is the rest
of the block; locals by `cx-local` with destructors by `cx-finally`; `cxr-ext` answers taken
off at the declaration; loops by `ev-for` with `cxu-loop-fuel`; switch by a first pass that
finds the case and a second that runs from it). Then `lib/cxx/read.cub`: `cx-fun` by
`%welimP` on `cxu-call-fuel` (constructors: bases, members, vptr; destructors: body, members,
bases; defaulted assignment; deleting destructor = id + `cx-deleting`), the default member
initializer hook, static initialization (`cx-init`: allocate statics and vtables, constant
then dynamic initialization), `cxx-entry`. Then run `on_message` on one message
(`experiments/cxx/sem/book-run.cub`). The parts of read-expr.cub were developed against a
`--serve` session (one declaration per request); a full from-scratch load of machine.cub +
read-expr.cub must be run and its `N passed, 0 failed` recorded here.
