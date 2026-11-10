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

4. **Every construct the importer stopped on by design, closed** (added by the coordinator
   after items 1–3 were accepted; nothing the importer stops on is optional). Each by a
   checked-in unit that uses it (`experiments/cxx/fourth.cpp`, `fifth.cpp`, …), the rule, and
   the constructors: `std::span` and `CXXRewrittenBinaryOperator`; enumerations without a
   fixed underlying type (the underlying type from a dump or from a generated probe unit
   compiled alongside: say which); anonymous struct/union members; init-captures; alias
   templates; `decltype(expr)`/`noexcept(expr)` in type strings; `float`, `long double`,
   wide/UTF literals; `thread_local`; `if constexpr`; `std::initializer_list`; multiple and
   virtual inheritance (base offsets, virtual base offsets, vtable layout including thunks,
   VTTs, from `-fdump-vtable-layouts`); a polymorphic class whose vtable is emitted in another
   unit.
5. **What `-fno-exceptions -fno-rtti` and scope kept out**, as data only (semantics is track
   F's), each in a checked-in unit that imports and checks: a unit compiled with exceptions
   and RTTI: try/catch/throw (`CXXTryStmt`, `CXXCatchStmt`, `CXXThrowExpr`, the implicit
   cleanups: `ExprWithCleanups` objects, and where destructors run on unwinding), `noexcept`,
   `dynamic_cast`, `typeid`; coroutines (`CoroutineBodyStmt` and its sub-statements,
   `co_await`/`co_yield`/`co_return` as Clang desugars them); `std::atomic` operations (the
   builtin/atomic expression nodes Clang produces), `volatile`; `goto`/labels; unions with
   non-trivial members; bit-field layouts; pointer to data member; statics with dynamic
   initialization and their order (the unit's global initialization function).
6. **Multi-unit programs**: two translation units that refer to each other (extern functions
   and variables, an inline function and a template instantiated in both, a class with its
   key function in one), declaration identity across units by mangled name (`mangledName` of
   the JSON), as one program value (`CProg` = units + a symbol table), checked. And the list,
   per unit, of every function it refers to whose body is not in the unit: a header inline
   that can be imported, or an out-of-line libstdc++/libc symbol to be lifted from the library
   binary later (input for the binary-lifting track).

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

- Item 4: each construct of the list appears in a checked-in unit under `experiments/cxx/`
  that imports without a stop and whose `.cub` checks; the "stops by design" list under Next
  step is empty of them; the kind lists below name the new kinds.
- Item 5: likewise for each construct of item 5; the unit with exceptions and RTTI is imported
  with those flags (recorded in its header).
- Item 6: `experiments/cxx/prog/` holds two units and the program value; loading the library
  and it ends `0 failed` with `…-prog : CProg` checked; the list of out-of-unit functions is in
  this file.

## Needs from track A

1. `Makefile`: add `lib/cxx/ast.cub` to `LIB` so `make library` checks it. It needs only
   `lib/prelude.cub` … `lib/std.cub` and `lib/prim.cub` (it uses `List`, `Word`, `wadd`, `Bytes`,
   `bytes`), so it can stand anywhere after `lib/prim.cub`; at the end is simplest:
   `... lib/x86.cub lib/elf.cub lib/cxx/ast.cub`. Cost measured: 37.0M steps, about 30 s (see
   Found 6 — most of it is the checker re-walking the types a `data` mentions).
2. Mutual `data` and/or `List D` as a field of `data D` (Found 1). With either, the in-type
   list constructors (`ct-nil`/`ct-cons`, `ce-nil`/`ce-cons`, `cs-nil`/`cs-cons`) become the
   library's `List`; the change in `tools/clang2cub.py` is `INLIST`, `inlist()` and the field
   types in `SCHEMA`, nothing else.
3. A byte-string literal, or `store8` in `lib/prim.cub` (Found 2). `lib/cxx/ast.cub` declares
   its own `primitive cstr-put` (`%store8`) meanwhile; it is one more line in `#trusted`.

## Done

Everything under "What is built" is built and both acceptance lines are met. Commands, from
`scoped-machine/` (L is the library list, `make -s -n library | sed -n 's/^.*cubical \(lib\/prelude.*\)$/\1/p'`):

```
python3 tools/clang2cub.py --emit-ast > lib/cxx/ast.cub
python3 tools/clang2cub.py experiments/cxx/book.cpp   -o experiments/cxx/book.cub
python3 tools/clang2cub.py experiments/cxx/second.cpp -o experiments/cxx/second.cub
python3 tools/clang2cub.py experiments/cxx/third.cpp  -o experiments/cxx/third.cub
CUBICAL_NOCACHE=1 ./build/cubical $L lib/cxx/ast.cub experiments/cxx/book.cub     # likewise second, third
```

The generated files are committed; regenerating them gives the same bytes (checked).

### Acceptance record (clang 18.1.3, x86_64-pc-linux-gnu; library of 35 files at `c398191b`)

| run | result | steps (this file's share) |
|---|---|---|
| library | 1131 passed, 0 failed | 24,785,474 |
| + `lib/cxx/ast.cub` | 1173 passed, 0 failed | 61,775,019 (36,989,545; 42 definitions) |
| + `book.cub` | 1365 passed, 0 failed, `ok: checks book-tu` | 63,427,867 (1,652,848; 192 definitions) |
| + `second.cub` (instead) | 1334 passed, 0 failed, `ok: checks second-tu` | 63,106,718 (1,331,699; 161 definitions) |
| + `third.cub` (instead) | 1344 passed, 0 failed, `ok: checks third-tu` | 62,786,294 (1,011,275; 171 definitions) |

Wall time, with `CUBICAL_NOCACHE=1`: measured on a quiet machine with the 29-file library of
that hour, library 27 s, + `ast.cub` 57 s, + `book.cub` 69 s; with other tracks' checks running
the same runs took 60–115 s. A unit by itself is 1–2 s of that (about 480 steps per node).

| unit | source lines | nodes written | declarations | shared types | records with layouts | import |
|---|---|---|---|---|---|---|
| `book.cpp` | 207 | 3462 | 62 | 98 | 8 of 8 | 5.4 s |
| `second.cpp` | 195 | 3188 | 43 | 90 | 8 of 8 | 0.7 s |
| `third.cpp` | 158 | 2318 | 45 | 86 | 8 of 8 | 0.8 s |

(`book.cpp` pulls in `<array>`: the JSON dump is 45.6 MB, of which the 62 declarations
reachable from the roots are written.) Reference binary for the latency comparison:
`clang++ -std=c++20 -O2 -fno-exceptions -fno-rtti -c book.cpp; clang -O2 book_main.c book.o`
answers `sum -11432484` on 1,000,000 random 22-byte messages (50% add, 25% cancel, ~25%
execute, 0.1% reset, prices 994..1006, seed 1) at about 71 ns/message here.

### What each piece is

- `experiments/cxx/book.cpp`, `book_main.c`: the D3 acceptance input and its reference driver.
- `experiments/cxx/second.cpp`: the second acceptance unit (every item of the list).
- `experiments/cxx/third.cpp`: a third unit, added so that what probing showed real code
  reaches immediately is met in a checked-in unit rather than left as constructors nobody
  exercised: virtual destructors (complete and deleting vtable slots), placement new, array
  new/delete, an array initializer with a filler, a qualified (non-virtual) member call,
  `double`, a static data member, a static local, a function template instantiated twice, a
  conversion function, a lambda capturing `this`, one name declared in two namespaces, a
  delegating constructor, ref-qualified methods, lifetime-extended temporaries (automatic and
  static), `alignas` on variables, default arguments, C-style and `reinterpret_cast`, the
  remaining integer types, operators and cast kinds.
- `tools/clang2cub.py`: three Clang runs per unit (JSON AST + record layouts in one process;
  vtable layouts from a code-generation run; the text AST for two facts the JSON omits), a
  parser for Clang's type strings with scoped name lookup, a worklist from the unit's roots,
  one rule per node kind, and a writer that shares types (`<unit>-tN`), writes one definition
  per declaration (`<unit>-dN`, N the declaration's id) and hoists any subterm over 400 nodes
  into its own definition (`<unit>-nN`; `--limit` changes the 400). `SCHEMA` in the tool is the
  one description of the AST; `--emit-ast` prints it as `lib/cxx/ast.cub`, and every term is
  validated against it as it is built. The header of each generated file repeats the unit's
  own counts and kind lists.
- `lib/cxx/ast.cub`: 37 data types, 220 constructors, plus `CStr`/`cstr`.

### The AST: what the next track reads (conventions)

- Ids are `Word`s, unique within a unit; every `VarDecl`, `ParmVarDecl`, `FieldDecl`,
  `EnumConstantDecl`, function, class and enum has one. `<unit>-dN` is the declaration with id N.
- `ctu name target roots statics decls`. Roots: the functions defined in the main file that
  another unit can call (external linkage, not inline, not a template) and every variable of
  static storage defined in it. `statics`: all variables of static storage written, in the
  order of their definitions (the order of initialization). Members are flat in `decls`, each
  with the id of its class.
- Every expression starts with its type and value category. A list of expressions (arguments,
  initializers, captures) is a `CExpr` built from `ce-nil`/`ce-cons`; an absent expression
  child is `ce-nil`; an absent statement child is `cs-none`; `cs-expr` is an expression in
  statement position.
- `ct-qual const volatile restrict T` wraps a qualified type; qualifiers on an array are on
  its element type. `ct-function ret params variadic const volatile ref noexcept`.
- `ce-member … arrow qualified kind id`: `qualified` true means the member was named with a
  nested-name-specifier (`Future::margin(qty)`): a call through it is not virtual.
- `ce-construct`/`ce-temporary-object` carry the id of the constructor chosen (found among the
  class's constructors by the type Clang prints; the JSON has no id there).
- `ce-default-init … field`: the field whose default member initializer is meant; the
  initializer itself is the `init` of that `cfield`.
- `ce-default-arg … parm`: the parameter whose default argument is meant; the argument itself
  is the `default` of that `cv-parm` (taken from whichever declaration of the function wrote
  it). The JSON names neither; the callee's declaration and the argument's position do.
- `ce-init-list … union-field filler inits`: for a class, `inits` are bases then fields in
  order; `filler` (or `ce-nil`) initializes the array elements `inits` does not reach.
- `ce-lambda … closure captures`: `closure` is the id of the closure class (a `cd-record`
  with `lambda` true), `captures` initialize its fields in order. Each `cfield` of a closure
  says what it holds (`ccap-var id` / `ccap-this`): inside the body (the class's `operator()`,
  a `cd-method`) a `ce-declref` to that variable, or `ce-this`, means that field. Clang marks
  neither in the JSON.
- `ce-new … global array operator-new init-style array-size placement init`.
- Implicit special members that Sema defined have bodies (`Level::operator=` is its
  memberwise assignments); the ones it left trivial are `cfb-defaulted`. Destructor calls at
  scope exit are not nodes (Clang's AST has none); `ce-with-cleanups`, lifetime-extended
  `ce-materialize-temporary` and the declared destructors (written for every class that has
  one) are what the semantics has to work from.
- Layout (`clayout size align dsize nvsize nvalign vptr`, bytes), `cfield`'s `offset-bits`,
  `cbase … offset primary virtual`, and `cvtable address-point entries` are Clang's for this
  target: the implementation parameters of PLAN §5.1. A method is virtual (`cd-method`'s
  flag) when it is written `virtual` or occupies a vtable slot.
- `sizeof`/`alignof` (`ce-unary-or-type-trait`) carry the type or expression, not the value,
  unless Sema wrapped them in a `ce-constant`.

### Node kinds the three units use (the next track writes the semantics for exactly these)

Counts are book/second/third.

Declarations: CXXRecordDecl 6/6/8, ClassTemplateSpecializationDecl 2/2/0 (TemplateArgument
3/4/0), FieldDecl 19/30/11, EnumDecl 2/0/0, EnumConstantDecl 6/0/0, FunctionDecl 8/8/11,
CXXMethodDecl 27/12/8, CXXConstructorDecl 9/6/6 (CXXCtorInitializer 16/12/10),
CXXDestructorDecl 7/7/7, CXXConversionDecl 0/0/1, VarDecl 24/23/27, ParmVarDecl 54/27/27,
AlignedAttr on a variable 0/0/2.

Statements: CompoundStmt 43/26/26 (and 39/23/25 as function bodies), DeclStmt 23/22/24, IfStmt
11/15/1, ForStmt 3/3/1, CXXForRangeStmt 2/1/0, WhileStmt 1/1/1, DoStmt 0/1/0, SwitchStmt 1/0/1,
CaseStmt 4/0/2, DefaultStmt 1/0/1, BreakStmt 1/1/1, ContinueStmt 1/2/0, ReturnStmt 38/29/16,
NullStmt 2/0/0.

Expressions: IntegerLiteral 38/71/47, CharacterLiteral 0/4/0, CXXBoolLiteralExpr 6/12/0,
FloatingLiteral 0/0/2, StringLiteral 0/1/0, CXXNullPtrLiteralExpr 0/1/2, DeclRefExpr
190/123/108, MemberExpr 99/106/31, CallExpr 4/6/9, CXXMemberCallExpr 23/11/10,
CXXOperatorCallExpr 28/2/1, CXXConstructExpr 19/7/7, CXXTemporaryObjectExpr 0/0/2,
ImplicitCastExpr 186/205/163, CXXStaticCastExpr 9/12/6, CXXFunctionalCastExpr 6/0/1,
CXXReinterpretCastExpr 0/0/1, CStyleCastExpr 0/0/2, UnaryOperator 25/27/15, BinaryOperator
50/82/39, CompoundAssignOperator 4/9/12, ConditionalOperator 8/4/2, ParenExpr 1/11/2,
ArraySubscriptExpr 2/14/10, InitListExpr 3/6/8, ImplicitValueInitExpr 0/8/5,
CXXDefaultInitExpr 11/0/4, CXXDefaultArgExpr 0/0/2, CXXThisExpr 49/50/15, MaterializeTemporaryExpr 3/0/3,
ExprWithCleanups 3/0/3, ConstantExpr 4/0/2, LambdaExpr 1/0/1, UnaryExprOrTypeTraitExpr 2/6/0,
SubstNonTypeTemplateParmExpr 5/6/0, CXXNewExpr 0/1/3, CXXDeleteExpr 0/1/2.

Cast kinds (18): LValueToRValue 93/107/84, NoOp 30/19/16, IntegralCast 39/74/34,
FunctionToPointerDecay 32/6/10, ArrayToPointerDecay 3/8/8, BitCast 2/0/6, DerivedToBase 1/0/3,
UncheckedDerivedToBase 0/0/2, ConstructorConversion 1/0/0, UserDefinedConversion 0/0/1,
BuiltinFnToFnPtr 0/2/0, NullToPointer 0/1/0, NullToMemberPointer 0/0/2, IntegralToBoolean
0/0/1, PointerToBoolean 0/0/1, IntegralToFloating 0/0/1, FloatingToIntegral 0/0/1, ToVoid
0/0/1.

Unary operators: `x++ x-- ++x --x & * + - ~ !`. Binary: `* / % + - << >> < > <= >= == != & ^ |
&& || = , .* ->*`; compound assignment names the operator applied.

Builtin types: void bool char schar uchar short ushort int uint long ulong llong ullong
int128 uint128 double nullptr, and Clang's two placeholders bound-member (the type of
`obj.method` before the call) and builtin-fn (the type of `__builtin_mul_overflow` before its
BuiltinFnToFnPtr cast). Other type forms: pointer, lvalue and rvalue reference, array, record,
enum, function, member pointer, qualifiers.

Functions without a body in the units (the semantics supplies them): `memcpy`, `operator
new`, `operator new[]`, placement `operator new`, `operator delete`, `operator delete[]`,
`__builtin_mul_overflow`, `__builtin_add_overflow`.

Met and deliberately not written as nodes (each is counted in the generated file's header):
attributes whose effect is already in what is written or holds only at compile time (Packed,
Aligned on a field or class: in the layout; Final, Override; NoThrow: in the function type;
AlwaysInline, Const, NonNull, AllocSize, ReturnsNonNull, WarnUnusedResult, Owner, Visibility,
Builtin), local `using`/`static_assert`/typedef declarations, and the template arguments of
function specializations (the body is written substituted). Also not carried: source
locations, `isUsed`/`isReferenced`, `hadMultipleCandidates`, `canOverflow`, `adl`,
`nonOdrUseReason`, a class's `definitionData` flags, `inline` on variables.

### Trusted, exactly

- "The AST dumped is the AST Sema produced" (PLAN Phase 4 item 2).
- Clang prints types as strings; nested names in them are as written. The importer resolves a
  name by C++ lookup from the place of use (function, class and its bases, enclosing
  namespaces with inline/anonymous namespaces and using-directives), and when the name is not
  in scope there, accepts it only if every declaration of that name in the unit (outside
  uninstantiated templates) means the same type. A candidate it cannot read is never skipped.
  Names that had to be chosen by scope and mean something else elsewhere in the unit are
  listed in each generated file's header (book: `iterator pointer reference value_type`, each
  inside `std::array<Level, 8>`; second: `Ring`; third: `Quote`). This lookup following
  Clang's is the one place the importer re-derives something Sema decided.
- Three correspondences the JSON leaves implicit, each checked as far as the dumps allow and
  stopping the import otherwise: constructor by printed signature (exactly one match);
  layout and vtable blocks by printed class name (parsed and resolved; several blocks must be
  identical); the k-th `MemberExpr`/`AlignedAttr` of the text dump with the k-th of the JSON
  (counts, arrows and member names must all agree).
- An enumerator without an initializer is the previous one plus one (the dump has no value
  for it).

## Found

In the machine:

1. No mutual `data` (`mutual` followed by `data`: "unexpected data"), and `List D` as a field
   of `data D` is rejected ("the field type is not strictly positive (the type itself at its
   own parameters, or not mentioning it)"). `mutual` over functions is for codata only ("a
   definition of a corecursive group is λs and lines over a record"). What does check: a type
   carrying its own list constructors, and one function recursing structurally through both
   node and list constructors (tried: `te-size`, `ts-size` on a trial type).
2. No byte-string literal and no `store8` in `lib/prim.cub`. `%store8 b off v` with a `Word`
   offset and a `%u32ofw` value works inside a `primitive` and is bounds-checked by the net.
3. `f : (n : Nat) → Bytes n → Word → List Word → Bytes n` recursing on the list in the fourth
   position: "a recursive call is not on a constructor field or a field's field from one of
   the definition's own matches". With the list second (before the arguments that change) it
   checks. The position of the matched argument should not matter.
4. `length` of `lib/prelude.cub` is "not a bound variable or checked definition" for a
   definition loaded after the library.
5. Field names `then`, `else`, `record` in a `data` declaration are syntax errors ("unexpected
   then"): keywords are not allowed as field names. (Renamed `then-s`, `else-s`, `base`.)
6. Checking a `data` declaration costs steps in proportion to everything the types it mentions
   contain, again for each mention: `CExpr` (44 constructors) 226,961 steps; `CVarInit` (three
   fields of type `CExpr`) 650,153; `CStmt` 2,912,021; `CBody` (one field of type `CStmt`)
   3,082,688; `CFunction` 3,355,614; `CDecl` (9 constructors) 13,017,022; `CTU` (one
   constructor, one field `List CDecl`) 11,715,810. `ast.cub` is 37.0M steps, 67% of them
   those last two. A declaration's check should not re-walk the declarations it names.
7. No hang, kill or segfault met: the largest generated file is 103 KB / 3462 nodes in 192
   definitions (largest single definition about 400 nodes, 81k steps); a unit costs about 480
   steps per node. An earlier binary failed at once with `error: unbound name %wmul` after a
   `git pull` changed `src/`: rebuild (`make build/cubical`) after every pull.

Forwarded (by the coordinator, 2026-11-10): items 1 and 5 to track S (mutual data, `List D`
in `data D`, keywords as field names); item 6 to track N (the cost of checking a data
declaration that mentions large types). Items 2–4 stand as recorded.

In Clang 18.1.3 (the oracle):

8. `-Xclang -fdump-record-layouts-complete` (the flag the track specification names)
   segfaults in `ASTContext::getASTRecordLayout` on libstdc++'s `<type_traits>` (it lays out
   dependent classes). `-fdump-record-layouts` is used; with `-fsyntax-only` it printed the
   layout of every class the three units write, the lambda closures included. A class with no
   block stops the import.
9. Vtable layouts are printed only by a code-generation run (`-emit-llvm`), so there is a
   second process; class names tie the two.
10. The JSON omits: the constructor's id in `CXXConstructExpr` (only `ctorType`); the field of
    a `CXXDefaultInitExpr`; a lambda's captures and which references in its body are to
    captured variables; whether a `MemberExpr` has a qualifier; the type of `alignas(type)`;
    the values of enumerators without initializers; the underlying type of an enumeration
    without a fixed one; the value of `sizeof`. Each has a rule above or stops the import.
11. JSON shapes that are easy to misread: a declaration listed a second time is a stub of
    `id`/`kind`/`name` only (taking the last node seen for an id loses the definition); an
    `InitListExpr` with a filler has no `inner`, its children are all under `array_filler`
    (filler first); `loc.includedFrom.file` is the including file, not the node's; `file` and
    `line` are printed only when they change from the previous location printed; a
    `CXXNewExpr`'s children are array size, initializer, placement arguments in that order;
    `virtual` is true only where written (an overrider without the keyword has no flag).
    Type strings are desugared one level at most (`iterator` → `value_type *`), and a
    signature printed at a use names member typedefs that are not in scope there.

## Next step

Items 1–3 are accepted and `lib/cxx/ast.cub` is in `LIB` (track A, `4d04760c`); track F reads
it (`tracks/F-cxx.md`: a request under "Needs from track C" there takes priority over the
list below). Items 4–6 are open. Do them in this order, one checked-in unit at a time, each
commit regenerating `lib/cxx/ast.cub` and every unit and recording the new kinds here:

1. `experiments/cxx/fourth.cpp`: the first group of item 4 (`std::span`,
   `CXXRewrittenBinaryOperator`, enums without a fixed underlying type, anonymous
   struct/union members, init-captures, alias templates, `decltype`/`noexcept` in type
   strings, `float`/`long double`/wide and UTF literals, `thread_local`, `if constexpr`,
   `std::initializer_list`). Run `python3 tools/clang2cub.py experiments/cxx/fourth.cpp -o
   experiments/cxx/fourth.cub`; each stop names the rule to add.
2. `experiments/cxx/fifth.cpp` (+ a second file for the key function): multiple and virtual
   inheritance, thunks, VTTs, a vtable emitted elsewhere.
3. Item 5 units, then item 6.
