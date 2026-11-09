# scoped-machine

A single normalizer for cubical type theory. Every term is compiled to a graph of cells and reduced
by local rewrites. Programs, proofs, the type checker, data structures and the rules for transport
all run on that one mechanism.

```
make test       # all suites; the same output with 0, 1, 2, 4, 8, 16 workers; again under ASan/UBSan
make library    # tests/boxes.cub, tests/sort.cub, tests/memo.cub
./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub FILE.cub
```

Code: `src/net.c` (cells, rewrite rules, parallel scheduler), `src/dnf.c` (intervals, faces,
substitutions), `src/big.c` (exact naturals), `src/lang.c` (core forms, compiler to cells,
readback, top-level forms, modules), `src/syntax.c` (the surface language: layout, parsing,
elaboration, clause compilation, printing). Language files: `lib/prelude.cub` (core data types,
composition clauses), `lib/code.cub` (codes, `View`), `lib/check.cub` (the type checker),
`lib/std.cub` (checked library), `lib/sort.cub` (9.15 over any ordered type; `tests/sort.cub`
instantiates and runs it). Coinduction: §7, `tests/codata.cub`; guarded recursion: §8,
`tests/guarded.cub`. Earlier plans and specs: `archive/`.

## 0. The language

```
data List (A : Type) = nil | cons (x : A) (xs : List A)
data S¹ = base | loop : base ≡ base

length : {A : Type} → List A → Nat
length nil = 0
length (cons x xs) = suc (length xs)

twist : S¹ → S¹
twist base = base
twist (loop i) = loop (~ i)

plus-zero : (n : Nat) → plus n zero ≡ n
plus-zero zero = λ i → zero
plus-zero (suc k) = λ i → suc (plus-zero k i)

#same length (cons 1 (cons 2 nil)) = 2
```

Application is juxtaposition; structure is indentation (a block opens after `where`, `of`, `let`,
`mutual`, `postulate`; `;` separates items on one line). `(x : A) → B`, `{x : A} → B` (implicit),
`(x : A) × B`, `A → B`, `A × B`, `λ x y → e`, `(a , b)`, `let x = e in b`, `if b then t else e`,
`case e of c x → t; d → u`, `case e return P of ...`, `e where` a block of definitions, `Type`,
`Type l` (`Level`, `lzero`, `lsuc`, `lmax`, or a numeral). Numerals are natural numbers. Every
Unicode symbol has an ASCII spelling: `->` `\` `|->` `==` `/\` `\/` `**` `forall`, `Nat` for `ℕ`,
`S1` for `S¹`, `Empty` for `⊥`, `Unit` for `⊤`. Operators are declared: `infix 4 ≡`, `infixr 5 ∙`,
and defined as `_≡_` or by an equation `x ≡ y = ...`. A name used in a signature and bound nowhere
is an implicit parameter (a type, or a level when it stands as a universe's level).

**Paths are equality.** `a ≡ b` is the type of paths between `a` and `b` (in a type found from
them; `Path A a b` names the type), `a ≡[ i ↦ A ] b` a path over a line of types. A path is
`λ i → e`, applied as `p i`; the interval has `0`, `1`, `~ r`, `r ∧ s`, `r ∨ s`. A partial value is
`[ i = 0 ↦ a , i = 1 ↦ b ]`, composition `comp (λ i → A) (λ j → [ ... ]) a0`, transport
`transp (λ i → A) a`. Whether a λ or an application is a path's or a function's is decided by the
checker (below), so it is written the same way. `lib/std.cub` has `refl`, `sym`, `_∙_`, `cong`,
`subst`, `transport`, `funExt`, equivalences and `ua`, with `calc` chains:
`calc a ≡ b by p ≡ c by q` (each step on its own indented line if long), composed by `_∙_`.

**Declarations.** A data type lists its constructors by fields (`cons (x : A) (xs : List A)`) or by
type (`cons : A → List A → List A`); a constructor whose type ends in `a ≡ b` is a path
constructor of a higher inductive type. A definition is a signature and equations; its equations
may match constructors at any depth, path constructors with their dimensions (`twist (loop i)`),
and implicit arguments as `{p}`. A codata type lists its fields (`codata Stream A where head : A;
tail : Stream A`), and a value of it is defined by copatterns: `(smap f s).head = f s.head`.
`record R A = { f : A ; g : B }` is a Σ with named projections, and `R { f = a ; g = b }` (the
fields by name, in any order), `record { f = a ; g = b }` or `(a , b)` a value of it; `R { r | g = c }`
is `r` with the fields given replaced; `x.f` projects. `mutual` groups definitions that call each other;
`postulate x y` declares names without values.

**Recursion.** A call on a field of the matched argument is structural; a definition at a type with
a clock recurses through any functions where a later value is expected (§8). A call on a field's field,
at any depth (`fib (suc (suc n)) = plus (fib n) (fib (suc n))`), is course-of-values: the split
computes each value together with the values below it (a Σ of the value and `Below v`, `Below`
itself an elimination into types), and a split of a variable below reverts that pair, so that in
its branches `Below` unfolds to the pairs of its own recursive fields. Any other terminating
recursion is written with a measure into `Nat` and, where the obligation does not compute to
`Unit`, a proof of it:

```
zig : (a b : Nat) → Nat
zig zero b = b
zig (suc a) b = suc (zig b a)
decreasing by plus a b using <-suc-eq (plus b a) (plus a b) (plus-comm b a)
```

Each call `f as` must satisfy `m as < m ps` (`_<_` of `lib/std.cub`, which computes to `Unit` or
`Empty` on numerals); the `using` term is checked at that type in the scope of the call's clause.
The definition becomes recursion on a bound of the measure, whose hypothesis is reverted through
the clauses' matches. A proof about such a definition does not unfold it by its equations
definitionally (the recursion's bound is part of the term), only by computation on values.

**Elaboration.** An implicit argument left out is a hole. Holes are solved first syntactically:
from the arguments' types as their heads' signatures give them, from the type an application is
expected to have (a λ's binder from the Π expected, a branch's from the motive at its constructor,
a definition's body from its signature), and from a solved argument's own type. What remains is
solved by the checker, which is itself a term (`lib/check.cub`): it checks the elaborated term
with its holes and its guesses (a λ or an application of unknown kind, function or path) and
answers with solutions or a request to redo; an undetermined level is `lzero`. A case written
where a type is expected takes that type as its motive, so its stuck values read back with their
type. `{_}` asks for a hole where an argument must be given positionally. An implicit is found
through a hypothesis whose type only reduces to the needed form (`h : P n` inside a `%welimP`
step, `h : motw b n` for a defined `motw`, a λ applied in a signature). `comp _ u a` takes the
constant line at the type expected (or at `a`'s type). `variable n : Nat` declares a generalized
variable: a signature mentioning `n` free gets `{n : Nat}` (and the variables its type mentions)
as leading implicit parameters, in declaration order.

**Multiplicities.** A Π binder or a constructor field may carry a multiplicity: `(x :0 A) → B`
(erased: `x` is used only in positions the back end erases, which is every type), `(x :1 A) → B`
(linear: consumed exactly once), and `(x : A) → B` (no constraint). The checker computes every
binder's usage (0, 1 or ω) after a definition checks, with the rules of plan §18.1 — an argument
counts as many times as the function's binder says, a type counts nothing, a line of types a
transport runs along and a Glue system count ω — and reports a binder annotated `:0` that is used
or `:1` that is dropped or duplicated. A data type with a field at `:1` is linear: its values are
linear wherever they are bound, and its constructors are applied only in a `primitive`. `#usage f`
prints a checked definition's binder usages (the back end erases the binders at 0, annotated or
not).

**Representations.** `lib/prim.cub` declares the machine's fixed-width numbers and byte strings as
primitives (`Int64`, `UInt32`, `Float64`, `Float32`, `Bytes n`, their operations, `load32`/`store32`
with the proof that the access fits) and `represent Nat by Word`. A representation of a data type
`T` by a type `R` (plan B12) is declared as
`represent T by R where to : T → R = …; from : R → T = …; to-from : … = …; from-to : … = …;
op f : A = native by law = proof`: each line is a checked definition (`T·R·to`, …, `T·R·op·f`,
`T·R·op·f·by`), and the first declared is the type's entry in the table the back end reads
(`%rtab d`). `#layout T` prints a type's layout (`lib/code.cub`'s `Layout`): by its
representation, else the automatic one, a union of its constructors' structs of the fields not at
`:0`, a recursive field a pointer; `#layout T as R` the layout of `R`. `#trusted` lists every
postulate and primitive in scope.

**The first-order criterion.** `lib/stage.cub` (plan B13) decides, for a code at a type, whether
the region may be printed as native code: `first-order T c` is `fo` or `not-fo reason at`, the
reasons `dim` (an interval binder or variable, a path abstraction), `dep-type` (a type at the
boundary depending on an input other than through an erased `:0` binder), `higher` (a type with
path constructors and no representation, a Glue or `%hcompU` type, a universe at the boundary),
`kan` (a composition or transport in the region), `reflect`, `mult`. `#fo f` and `#fo (the T t)`
run it on the normal form at the type (the specializer's normalization), so a closed transport
has reduced. The parts of a schedule (`Stage`, `Strictness`, …) are declared in `lib/code.cub` (`Sched` itself in `lib/specialize.cub`)
for the specializer to read.

**Instance arguments.** A binder `{{e : C A}}` is an implicit argument found by instance search
(plan E28): among the `instance` declarations in scope (`instance eqNat : Eq Nat` before a
definition), the latest whose result type matches the argument's type, as far as the signature
and the solutions so far tell it, is tried; the match binds the candidate's own implicit
arguments and its instance arguments are searched in turn; a candidate the checker rejects
is given up for the next; a variable in scope of the type (a function's own `{{e}}`) comes
first. `{{e}}` gives one by hand, and names it in a clause.

**Parameterized modules.** `module M (A : Type) (e : Eq A) where` a block of definitions (each
with a signature): each is a definition `M·f` abstracted over the parameters (an implicit
parameter `{A : Type}` an implicit argument), its references to the module's other
definitions applied to them; `open M Nat eqNat` defines each `f` as `M·f Nat eqNat` at the
instantiated signature (an implicit parameter not given is inferred from an explicit
argument's type, or given as `{Nat}`).

**Deriving.** `deriving eq D [as name]` defines a decidable equality on the data type `D` (plan
F31): its code is computed by `lib/derive.cub` (`derive-eq`, a term over the data table), read
back and checked at `D → D → Bool` as `D·eq` (or the name given).

**Named worlds.** `world "kv" = kv-world` (plan D27) checks `world-kv : Named S St` (lib/proc.cub:
the handler under an id, the name's hash, also `world-kv-id : Word`), its type read from the
handler's signature or given (`world "n" : World S St = term`); a registry is a list of them and
`find-world` picks one by id.

**Foreign functions.** `foreign name "symbol" : T` (plan D27) makes a C function of the running
program (or of a shared library, `"lib.so:symbol"`) a primitive at `T`, a Π over arguments of
primitive types (`Int64`, `UInt32`, `Float64`, `Word`, `Bytes n`, passed as an address; an
argument at multiplicity 0 is not passed) to a primitive type or `Word`; the arguments are read
in turn and the call made when the last is (`%ffi`). Trusted: `#trusted` and `#safe` list it as
`foreign`.

**Arrays, and stuck primitive operations.** `Array n` is `Bytes (times n 8)` (plan C22,
`lib/prim.cub`): `index n a i h` and `update n a i h v` with `h : le (suc i) n` are a load and a
store at offset `times i 8` with the proof that the access fits, `tabulate n f` writes `f` at
every index, and `index-update` (a read after a write at the same index is the value written)
is the byte layer's law `load64-store64` (trusted) unfolded. A primitive operation on a
variable is a neutral the checker views (`wprim k args`) and compares argument by argument, so
such laws follow by unfolding (an operation stuck on a variable used to be an unknown value).
`Fin→Array n : Equiv (Fin n → Int64) (Array n)` is the interface (item 22): a program written
against functions uses application and `fin-set n f p v`, and under `atab` they are `index` and
`update` (`apply-by`, `fin-set-by`). Byte strings convert by their contents (`%beq`). On the
first-order side a store into a buffer nobody else holds — fresh, or let-bound or loop state
whose other uses are reads before it — is stored in place (`P-store64-ip`, printed `st64i`, no
copy); into a parameter or beside an unsequenced read it copies (`lib/fo.cub`'s `mark-uniq`).
Other element types: `UInt32` on the 4-byte layer (`Array32`, `index32`/`update32`, `Fin→Array32`,
its two laws trusted like the 8-byte ones), and any `A` with an equivalence to `Int64` on the 8-byte
layer (`indexV`/`updateV`/`Fin→ArrayV A to from ft tf n`): `Float64` by its bits (`f64bits`,
`f64ofbits`, trusted inverse), a type represented by `Int64` by its representation's `to`/`from`;
the same on the 4-byte layer for any `A ≃ UInt32` (`Fin→ArrayV32`): `Float32` by its bits
(`f32bits`, `f32ofbits`; `Fin→ArrayF32`).

**Regions as state.** `lib/region.cub` threads a region, `Bytes n`, through a program as
state: `RM n A = Region n → Region n × A`, pointers as offsets with their proof (`Ptr n`),
`rd`/`wr`/`rmodify` the byte layer's accesses, `run-fresh n m`, `enter-at a n m` for memory the
program does not own; `rd-wr` and `rd-wr-disj` are its laws. The region is unique along the
thread, so a specialized region program stores in place. A byte string quotes as a code (the
fresh string, then a store per nonzero word).

**Memory the program does not own.** `ptr-of n b` is the address of a byte string and
`region-at a n` the `n` bytes at an address, copied in (plan C20; `%ptr-of`, `%region-at`,
trusted). `#safe f` lists every trusted definition (postulate, primitive) that `f` reaches
through what its type and term mention, or `nothing assumed`.

**Codebase keys and the store.** A checked definition is named by its content (plan F34): its
key is the SHA-256 of its elaborated type and term with binders nameless, holes solved, every
checked definition it mentions replaced by that definition's key and every data type by its
declaration's key, multiplicities kept — not its name, file, position or the machine's build.
Renaming anything changes no key; a changed proof changes its key and its dependents' only
(`src/store.c`). The store, `CUBICAL_STORE` (default `.cubical-store`; off under
`CUBICAL_NOCACHE` unless set), keeps under each key `code`, `type`, `usage` and `check`, a line
per checker that accepted it (the checker named by the hash of the library texts up to
`chk-def`); a definition whose key the current checker has accepted is not checked again
(`ok: checks f   [store]`), whatever file or name it comes from, however the machine was rebuilt.
`names` logs each binding of a name to a key. `#key d`, `#names K` (a prefix will do),
`#history name`, `#unbind name`, `#gc` (entries unreachable from the bound names, through the
keys their code mentions, are removed).

**Indexed families.** `data Vec (A : Type) : (n : Nat) → Type where vnil : Vec A zero; vcons :
(n : Nat) → A → Vec A n → Vec A (suc n)` declares the family by fording (plan E28): the indices
are the type's last parameters, each constructor carries an equality field per index at
multiplicity 0 (`PathP (λ m → Nat) %ix0 (suc n)`, erased), an application without the equality
fields gets `refl`, a pattern without them is padded, and a branch that writes them (`vcons n x
xs e`) transports along `e` or refutes it (`vtail`'s `vnil` case by `snotz`). The recursion rule
wants the index implicit (`{n : Nat} → Vec A n → …`). A recursion whose index argument is a
variable has its induction hypothesis at the field's own index (`vmap f xs : Vec B m` for `xs :
Vec A m`): the motive is written `(λ ix v → Vec B ix) n` and the checker reads it so (the index
unified at the split, as Agda does). Leaving the equality field out of the pattern rewrites the
goal by it: `vmap f (vcons m x xs) = vcons m (f x) (vmap f xs)` is checked at `Vec B (suc m)`
and transported along the equation; naming it (`vcons m x xs e`) leaves it to the branch.
Indices are unified at a split as Agda does: a case whose indices clash may be left out
(`vhead (vcons m x xs) = x` on `Vec A (suc n)`), a field determined by the indices is solved
(`vzip (vcons m x xs) (vcons m2 y ys) = vcons m (x , y) (vzip xs ys)` with `ys : Vec B m`), and
a missing clause is covered when a later argument's type is empty (`vlookup` needs no `vnil`
clause: `Fin zero` has no constructor). `tests/elab.cub`.

**Running processes and calling native code.** `lib/world.cub` is the drivers' signature `W`
(`time-now`, `write fd n b`, `read fd n`, `sleep ns`, `exit c` as processes of `lib/proc.cub`)
and `to-step p` a process as a tree of steps; `#run (to-step p)` performs it against the
operating system (plan D26, `src/drivers.c`), ending with `run: done a`. `#native f n = prog`
compiles the bytes of `emit-lib-u` (the unboxed C of `lib/emitu.cub`) into a shared library and
redefines `f` over it: on word arguments the compiled code answers, on any other `f` computes
on the net (plan B18, `%native`; up to 32 parameters and argument words, above six through the entry's trampoline; over represented values an argument with a neutral part takes `f`'s own definition, so an open term reads as it does). `#native f n keep = prog` keeps a boxed result native (the
emitted runtime's `V`, passed to the next compiled call as it is; unboxed into cells when the net
reads it). `#native f n from "lib.c" "sym"` enters a hand-written C file's function: a `void *`
result is a handle of type `Native` (`lib/prim.cub`), held by the net without being read, passed
to that library's `void *` parameters, printed `(%handle #k)`, compared by identity through
`%view` (`wnative id k`), and finalized by the library's `cubical_release` when its last copy is
erased (`tests/native/handle.c`). The entries are trusted to treat a handle as read-only: the
net shares a handle among its readers. A parameter or result whose type has a declared
representation crosses as the representation (`Nat` as a `uint64_t`: converted by `to` before the
call and `from` after it, the fallback the definition over the representations); a record of such
fields crosses as its C struct by value (through a trampoline the machine appends), and a `#native`
specialization is cached by key: its expression with each checked definition by its codebase key,
the values written in it, the checker's version and the libraries before the file name it before
it is computed, and a hit (`specialization cached by key, 0 steps`) runs neither the specializer
nor the C compiler. `#native f n at k = P` specializes while the program runs: on the first call
with each value of f's first k word arguments the net computes `P v…` (the residual's C), which is
compiled, entered and kept under the values (and in the cache by key across runs); at most
`CUBICAL_NATIVE_AT_MAX` (default 16) are kept per entry, the least recently called evicted, and the
table is safe with workers. The other direction is `build/libcubical.a` with
`src/libcubical.h`: a C program loads libraries with `cubical_init` and calls checked
definitions on words with `cubical_call_w` (and the emitted C's boxed form, `cubical_call`), on
boxed data with `cubical_call_data`, and on records as C structs (`"struct D"`, `cubical_call_data_r`);
`tests/native/main.c`. A record crosses `#native` and the C calls as its C struct by value: one
constructor its fields; several a `uint64_t` tag and a union of one struct per constructor; a field
that is itself a record inline; a word (or a type represented by `Word`) `uint64_t`, `UInt32`
`uint32_t`, `Int64` `int64_t`, `Float32` `float`, `Float64` `double` (the emitted program's `T<di>`, or
a hand-written file's `struct D`, `tests/native/rec.c`); in the emitted program each such field is held
at its width and read into the program's word (a float by its bits). `%gsteps n` is the steps checked definition `n` took to check, from the
run or the store's `ok` line.

**Commands.** `#check e : T`, `#same a = b` (equal readings, at a type when either side has one),
`#show e`, `#observe n e` (a codata value unfolded n levels), `#usage f`, `#layout T [as R]`,
`#trusted`, `#safe f`, `#fo e`, `#key d`, `#names K`, `#history d`, `#unbind d`, `#gc`, `#run s`, `#native f n [keep] = prog`, `#native f n from "file.c" "sym"`, `#input x` and `#supply x = e` (§6), `#fail` before a declaration or command
(it must be rejected). `import "file.cub"` loads a module
once. Results and failures print in the surface syntax, a failed check with its declaration's
`file:line` and the term at fault. `--serve` after the files reads blocks of declarations from
the standard input, each ended by a line `.`, and answers each as a file would be answered,
ending the reply with `.` (a fatal error ends the request, not the machine). `#retract f` hides
the checked definition `f` (or a data, codata or record type with its constructors, fields and
projections) so the name can be declared again; what was checked against the old one keeps it (a
global is referenced by its index), later declarations see the new one. `#type e` prints the type
the checker infers for `e`, `#scope` the names in scope, `#where f` where `f` was declared.
`tools/lsp.py` is a language server over `--serve`: diagnostics, re-checking by retraction, hover
by `#type`, completion, go-to-definition, incremental sync.
`tools/lsp.py` is a language server over `--serve` (diagnostics, re-checking what follows a
changed declaration, hover with the written signature); an editor runs `python3 tools/lsp.py
--root scoped-machine` as its server for `.cub` files. `CUBICAL_PROFILE=1` prints the run's profile, `=show` the profile from the last `#show`. The core forms the language compiles to
(`(lam x b)`, `(elim P e ((c x) b))`, …) are still read from files of other extensions, and
`--print FILE` prints such a file in the surface syntax.

## 1. Rewrites see only the two cells involved

Each cell has one principal port; a step rewrites two cells whose principal ports meet. No rule
consults an environment or context. Being under a face such as `i = 0` is a substitution of the
dimension by the endpoint. Applying a dimension binder puts a substitution cell on its body, and
that substitution re-activates exactly the stuck values it unblocks: `p @ i` under `i := 0` becomes
the left endpoint of `p`. The step count is fixed by the term: identical with any number of worker
threads (`make test` compares every suite's output at 0, 1, 2, 4, 8 and 16). Reading a stored value
costs the same whether or not the value has arrived: a read that arrives first waits on the wire
and is served the same one-level copy, and waiting is not a step. Every reading is taken over the
complete normalization of what was posed. A worker locks the active pair and the cells it rewires,
in address order, and revalidates before rewriting.

## 2. Values include equality

Besides functions, pairs and data, values are: the interval (`0`, `1`, dimensions, `~`, `/\`,
`\/`, kept in normal form), paths (a value with a dimension abstracted), faces (conditions on
dimensions), partial values (systems), composition, Glue types and their elements, compositions
of types and their elements, and higher inductive types (data types with path constructors).
A constructor's field may be a path in the type itself, of any dimension (the truncations'
arguments: `squash (x y : T) (p q : x ≡ y) (i j : I)`); such a field is a map from the interval into
the type, so it is strictly positive, and the checker accepts it.

**Partial values.** `[ i = 0 ↦ a , i = 1 ↦ b ]` is a value defined only where its faces hold.
Each branch is kept unbuilt, as syntax with its environment, and is built with its face's
substitution applied (`a` with `i := 0`) only when an operation reads it. If a substitution makes a
face true (here `i := 0`), the whole partial value becomes that branch.

**Composition.** `comp A u a0` takes a line of types `A(i)`, a starting element `a0 : A(0)` and a
partial value `u` (where it is given, what the result must be along the line), and produces an
element of `A(1)` matching `u`. Transport is composition with nothing given. How it is computed
depends on what kind of type `A(i)` is; each case is a term in `lib/prelude.cub`, not C:
- functions (`%comp-pi`): to apply the result to `x : A(1)`, move `x` back to each `A(i)`, apply,
  and compose the results in the codomain;
- pairs (`%comp-sigma`): compose the first components, then the second components along the line
  the first one traced;
- paths (`%comp-path`): compose pointwise, adding the path's two endpoints to what is given;
- Glue (`%comp-glue`): this is where univalence computes, so transport along `ua e` applies `e`;
  on the end's faces the fibre over the composed base comes from the equivalence's contractibility;
- universes (`%comp-u`): a composition of types is a type of its own, `%hcompU A S` (Cubical Agda's
  `hcomp` in `Type`). Its faces `S` are the tube's lines of types from `A`, and the complement of
  the tube's face carries `A` itself, so where the tube is empty the composition is `A` and a
  closed composition of types is a type. Its elements are `glue` values; on a face, `%unglueU`
  transports back along that face's line. Composition through it (`%comp-gluel`) composes the
  unglued base and then, on the end's faces, takes the fibre of that transport over it from
  `%tproof`: the transport fillers along the line and one composition back along it. No
  equivalence and no contractibility proof is involved, so eliminating a higher inductive type
  into the universe and transporting through the result costs transports along the face lines,
  at every nesting depth.

A `glue` value's base is held unbuilt. Where the system is total (on the end's faces of a
composition through a Glue type or a composition of types, the usual case) the value is the face's
element and the base is never read; built, it is a further composition whose tube carries the
fibre paths, and those compose generically one level down, so each level of nesting would pay for
every level below it. Transport along the double cover of the circle over a composite of `n`
loops (`tests/codata.cub`, the composites defined one on another):

| loops | composed on the left | composed on the right |
|---|---|---|
| 4 | 4.2k steps | 5.1k |
| 8 | 9.1k | 12.0k |
| 16 | 19.0k | 27.0k |
| 64 | 78.4k | 146k |

With compositions of types encoded as Glue types over transported identity equivalences and the
base built (commit 04f761c and before) the same transports took 20.5k, 46.9k and 99.8k steps on
the left and 51.9k, 340k and 2.49M on the right at 4, 8 and 16 loops.

**Data types.** One mechanism serves every declared data type, with nothing written per type:
- moving a constructor value along a line of data types (e.g. `List A(i)`) moves each of its
  fields along its field type; this is derived from the declaration;
- a path constructor such as `loop i` must still meet its declared endpoints after moving, so the
  result is corrected by a composition that glues those endpoints back on;
- composition is generalized (Cubical Agda's `gcomp`): the tube gets the complement of its face,
  carrying the transported starting value. At every closed point the tube is then total, so a
  composition never leaves a formal composition behind where nothing was to be composed: a
  composition with an empty tube is transport, transport along a constant line of a closed type
  is the identity (`transp (λ i → S1) base` is `base`), and transport along `ua not` of `true` is
  `false`;
- in a data type without path constructors, a composition whose starting value and sides are
  all one constructor goes into the fields, each composed along its type at the earlier fields'
  fillers (CCHM's rule), so `λ j → comp Nat [ j = 0 ↦ 1 , j = 1 ↦ 1 ] 1` is `λ j → 1` and a path
  field transported along `ua` is a constant path again. The machine looks at the starting value
  and then at each side at a fresh dimension; a side that is not that constructor (a variable)
  keeps the composition formal, and a substitution decides again. The checker accepts a formal
  composition in such a type only when its tube covers every closed point, as the machine's own
  do, so a closed value of such a type is a constructor;
- a composition in a data type that cannot be computed further is kept as a value, `hcompD`; a case
  analysis meeting such a value applies itself to the pieces and composes the results in the
  result type.

## 3. Work is done once, and only where it is read

A shared subterm is held by a duplicating cell that copies lazily, one level at a time as it is
read. A stored value lives once, and readers copy its top cell. A substitution of dimensions is
recorded on the reference, not applied by copying. A subterm in a position that might not be read
is held unbuilt until an operation meets it: `fst (a , b)` never computes `b`, even if `b`
diverges. `delay e` holds `e` so wherever it stands: it has `e`'s type, an operation meeting it
builds it, `force` of it is it, and its code is `cthunk` (the first-order and C++ back ends,
being strict, compute it where it stands). A λ's subterms that do not mention its variable are computed once, however often the λ is
applied. A case is one cell, and selecting a branch is one step.

Equal terms reached by different routes are also computed once. Every cell that will produce a
value (a thunk, a saturated call, an application of a known λ, a case) carries a key: its compiled
body over the keys of the values bound in it, interned, so one term is one pointer. A value's key
is its constructor over its parts' keys; a held value (a box) is named by what was put in it, and a
copy out of a box, a definition unfolded, and the scrutinee a case branch rebuilds from the fields
it took all carry that name, so one value has one name by whatever route it is reached. A producer
is pending on its key from its creation; when one fires while another of the same key is pending,
its value goes into a box held by the entry, and the other reads the box instead of computing
(waiting on the wire, uncounted, if the value has not arrived). Retention is reachability: an
entry lives exactly while a producer is pending or a reader holds the box; the table holds nothing
on its own, and two boxes with equal values but different origins are not identified (a numeral
computed by `add` and the same numeral written out are two values). Naive Fibonacci is the test
(`tests/memo.cub`): `fib (n+2)` demands `fib (n+1)`, which demands `fib n` while the outer `fib n`
is pending, so every `fib k` is computed once and the work is linear in the calls (unary addition
dominates the count), whether the argument is a numeral the machine computed or one written out.

Compiled code follows the same rule. Records (a λ's or thunk's body) and case tables are interned,
so equal code compiled twice is one pointer, and the checker compiles every definition's quoted
code again at each reading. Between top-level forms the code is sorted by what still refers to it:
what a definition, a cell or kept code can build from is kept whole; what only keys name (a λ's
key is its record over its captures' keys) loses its body and keeps the hash and SHA-256 interning
compares, so equal code compiled later gets the same pointer and the body back, and the memo finds
what it found before; the rest is freed. Output and step counts are those of keeping everything
(`CUBICAL_NOSWEEP=1`); `CUBICAL_SWEEPALL=1` sorts after every form, `CUBICAL_MEMEND=1` reports
the split at exit.

A reading reports its steps, and with them the memo hits (values found instead of computed) and
the values shared. The steps are the work this reading did, given what earlier readings left held
and the order the rewrites ran in: a term's second reading can cost less than its first, and with
workers a reading can find a value shared that it would have computed alone. Without the memo
(`CUBICAL_NOMEMO=1`) the count is a property of the term; `make test` checks that it does not
depend on the number of workers, and that with the memo the answers do not.

## 4. Open terms normalize, with types

A stuck computation is one with no rule to fire because the thing it inspects is unknown: `f x`
with `f` a variable, `case n ...` with `n` a variable, `p @ i` with `p` a path variable and `i` a
dimension variable. It is kept as a value (a neutral) holding its parts; reduction continues
everywhere else, including inside the branches of a stuck case. When a substitution later
supplies the unknown, it fires. A neutral's type is computed from the variable at its head along
the operations applied to it, and a stuck path's endpoints are read from that type.

A reading of a checked definition, or of `the T t`, is taken at its type: `quote` in
`lib/check.cub` reads the value as a code (`lib/code.cub`), applying a function to a fresh
variable of its domain, reading a pair by its projections and a path at a fresh dimension, and a
stuck elimination's branches at fresh fields typed by the constructor's telescope, at the motive.
So `λ p → p 0` at `Path Nat zero zero → Nat` reads `λ x0 → zero`, and a variable of a Σ type
reads `(fst x0 , snd x0)`. Typed readings give full normal forms, except inside an
elimination written without a motive that is the head of a stuck term (applied, projected, or
eliminated), whose type is not known there; reached at a known type, its branches are read at that
type. A stuck word elimination carries no motive, so its branches are read without types: the
type `T = P w` it is quoted at does not determine an arbitrary motive `P`, so reading them at `T`
would be unsound (a motiveless case can be read at `T` because its motive is constant). The
code satisfies two laws, checked in `tests/quote.cub`: evaluating it (`eval-g`) gives a value
convertible with the original, and quoting that value gives the same code. A failure report
prints its types (expected and actual, a type that was eliminated) the same way.

`#same` of two terms, either of them a checked definition or `the T t`, reads both at that
one type, so equal values compare equal however they are written. Other readings (`#show` and
`#same` of any other terms) are taken without a type: under a binder,
a variable's type is not known, so a path variable stays stuck at an endpoint and nothing is
η-expanded. `#same` compares two readings as text: on well-typed terms equal texts are equal
values; two readings can differ while their values are equal.

## 5. Reflection: `%view`

`(%view v)` turns the outermost form of a computed value into an ordinary data constructor in one
step, whose fields are the value's own parts: a Π type gives `(wpi A B)` with `A` the domain and
`B` the codomain as a function, a stuck application gives `(wapp f a T)`. A program cases on this
like any data, so a program in the language can examine a value's structure. What it sees is the
computed value, not the source text that produced it; a function shows only as `(wlam f)`.
`(%var T)` and `(%dim)` make fresh variables, `(%dnf r)` gives an interval's or a face's clauses
as lists of literals, and `(%motive f)` an elimination's motive. The type checker (`lib/check.cub`) is written with
these and runs on this runtime, over the values the runtime already computed for each subterm:
there is no second evaluator, and checking a definition is running that program on it.

## 6. Inputs

`#input x` declares a definition without a value. An operation that needs `x`'s value (applying
it, a case on it, a projection) is held at exactly that point. Everything that does not depend on
`x` reduces fully, and the parts of a result that do not need `x` are produced. `#supply x = t`
connects the value, and the held operations proceed. Unlike a variable, an input is not computed
with symbolically: what depends on it waits for it.

## 7. Coinduction

`codata Orbit (A : Type) where here : A; next : Orbit A` declares a coinductive record type by
its fields. A field's type may mention the parameters, the earlier fields, and the type itself
strictly positively (at any parameters not mentioning it, under Π codomains and Σ components);
the checker checks this. A field applied to a value is its projection: `here o`, or `o.here`.

A value of a codata type is made by a copattern definition, a definition whose body is λs and
lines over a record:

```
unfold : {A : Type} → (A → A) → A → Orbit A
(unfold f a).here = a
(unfold f a).next = unfold f (f a)
```

Such a definition may call itself, and the definitions of a `mutual` group may call each
other. Its value is its stuck call: a neutral of its declared type, holding the definition and its
arguments. A projection unfolds the call once (the body applied to the same arguments, at the same
dimensions) and takes the field. Nothing else unfolds it: an application, a comparison, a reading,
and a path application at 0 or 1, which reads the endpoint from the declared type as for any
neutral path. So a process that never ends is a finite value, an observation of depth n costs n
unfoldings, and a definition that is its own tail (`zeros`, whose `next` is `zeros`) is held as
its call. A record stands only as the body of a copattern definition.

The checker (`chk-codef` in `lib/check.cub`) requires every call of the group in a field's term to
be guarded: it is the head of the field's term (applied to arguments not mentioning the group, at
any dimensions), or stands there under a λ, a line, a pair, a constructor, a case or system
branch, or a let's body; never as an argument or eliminated. Every body of a group is found guarded
before any is checked, since checking one can unfold another. Each body is then checked at its
type, the group's calls at their declared types. A guarded call is productive, so the value exists at
every depth. At a path type the boundary is checked field by field (a record is compared with a
value by its fields), so it holds at every depth. Two stuck calls are convertible when their
definitions and arguments are; there is no η for codata, so two definitions whose unfoldings are
bisimilar are not convertible. Their equality is a path, and a path between orbits is a
bisimulation: `tests/codata.cub` proves `(x ≡ y) ≃ (x ≈ y)` by four corecursive definitions
(`punaragamana/Orbit.agda`), and proves `mapO id o ≡ o` by a corecursive bisimulation.

Composition in a codata type is a copattern definition generated from the declaration (`%comp-D`):
each field composed along its type, at the parameters' lines and each earlier field's filler. A
transported process is again a process, unfolded field by field when it is observed.

`#show t` reads a stuck call as the call. `#observe n t` reads it unfolded n levels deep, as
records. An unfolding is named by the call it unfolds, so two unfoldings of one call pending at
once share the body (§3).

## 8. Guarded recursion

```
nats : Stream κ Nat
nats = 0 ∷ smap suc nats

fibs : Stream κ Nat
fibs = 0 ∷ 1 ∷ zipWith plus fibs (stail fibs)

#same nth 10 (λ κ → fibs {κ}) = 55
```

`▸ κ A` is an `A` one tick of the clock `κ` later: the Π type `(α : Tick κ) → A`. A tick is bound
by λ and given by application, and nothing else: `Clock` and `Tick κ` are the domains of Π types
only, in no universe, so a tick cannot be stored, returned, or let-bound. The checker's
timelessness rule makes "later" mean later: in the body of a tick λ, a function applied to the
tick α may use no variable bound at or after α. So `λ α → x α α` is rejected (one tick spent
twice) and `λ α β → x α β` accepted. Interval variables are exempt: `λ α (i : I) → p i α` turns a
path of later values into a later path, and `λ (i : I) α → p α i` turns it back. This is sound
because the interval does not vary in time: the "earlier" of a context extended by an interval
is the earlier context extended by the interval, ◁(Γ × I) ≅ ◁Γ × I, as in the cubical models of
guarded type theory.

`dfix : (▸ κ A → A) → ▸ κ A` (at every level: `next`, `⊛`, `dfix`, `fix`, `pfix` and `forceκ`
take `{l : Level}` first) is a stuck call of the kind copattern definitions make (§7), with one
difference: it unfolds, `dfix f ⋄ = f (dfix f)`, only when applied to the tick constant `⋄`, and at
a tick variable it stays stuck. `fix f = f (dfix f)` is then the guarded fixed point, and every
recursion whose recursive use is under `▸` is well typed, whatever functions it passes through.
`⋄` is not written in a checked term; it is the computation of `forceκ : ((κ : Clock) → ▸ κ (A κ))
→ (κ : Clock) → A κ`, a later value at every clock being a value now at every clock (the clock is
fresh, so its tick constant may be given). A total stream is a stream at every clock,
`Str A = (κ : Clock) → Stream κ A`, observed through `forceκ` (`tl`, `nth`); `κ0` is the closed
clock. `pfix : dfix f ≡ next (fix f)` is the unfolding at a tick variable. It is a stuck call
too: at `⋄` both its ends are `fix f`, and it computes there, `pfix f i ⋄ = fix f` for every `i`
(the tick-unfolding definitions count path parameters among theirs). So transport along `pfix`
computes once `forceκ` gives the tick, and a guarded recursive type, a fixed point in the universe
such as `G κ = fix (λ X → Nat × ((α : Tick κ) → X α))`, can be unfolded and folded by transport
along `pfix` and still read canonical numerals (`tests/guarded.cub`). `pfix` being a path rather
than a judgmental equality is a property of the theory (`dfix f` at a tick variable must not
unfold, or normalization fails); the path computes wherever the equality would.
Normalization terminates because the only unfolding fires on `⋄`, which only `forceκ` gives.

Guarded types are ordinary data whose recursive fields are later:
`data Stream (κ : Clock) A = _∷_ (head : A) (tail : ▸ κ (Stream κ A))`. Such a field is not a
recursive field (an elimination takes no hypothesis for it). Composition in `▸ κ A` is pointwise
(the composition of ticks and clocks is the identity), so transport along a line of stream types
moves each tail at its own tick, and the heads read are numerals again (composition with an empty
tube in a data type without path constructors goes into the fields, §2).

The elaborator writes the ticks. A clock named in a signature (an argument of `Tick` or of a
data type's `Clock` parameter) is an implicit parameter. A definition with a clock whose
recursion is not structural is the guarded fixed point over its explicit parameters after the
clock. A term given where `▸ κ X` is expected and not itself later is a λ over a fresh tick, inside
which a later subterm standing where a present value is due is applied to that tick, and a
recursive call is the fixed point's self at the outermost such tick. A tick λ written by hand
(`1 ∷ λ α → ones`) is treated the same way, and so is the later component of a pair. A clock
given to a defined family (`G κ` with `G : Clock → Type`) is bound as well. A clock argument nothing
determines is the one clock in scope. A recursive call needed now is reported where it stands:
`bad = smap suc bad` is rejected with "a recursive call needed now, not under ▸".

The plan and status document is `PLAN.md` (the single one; a fresh agent starts there).

## 9. Programs about programs: the first-order form, matching, navigation, certificates

Six libraries over codes, each untyped data as `lib/code.cub` is and loaded before the checker (two after it),
each with its readings in `make test`:

- **`lib/fo.cub`**, the first-order form. A program with no types, paths, dimensions or universes in
  it: variables, words, constructors with their fields, switches on tags, projections, calls by key,
  native primitives, let, if, loops, closures, thunks, calls into the net. `eval-fo` runs it; `lower`
  maps the first-order fragment of `Code` to it (variables, words and their operations, constructors
  and case, pairs, λ and application, let, word elimination as a loop; a written step λ is inlined
  into the loop). **`lib/emitc.cub`** prints a first-order program as C source (a list of bytes: a
  small runtime of boxed values, one function per definition, a main); `experiments/fo/run.py` decodes
  a program, compiles it with gcc and runs it: factorial as a loop, a switch, let and if, and a code
  that the machine specialized.
- **`lib/match.cub`**, matching and rewriting. A pattern is a code whose variables above a bound are
  metavariables; matching binds them, consistently when one is met twice and never capturing a
  variable bound inside the pattern; instantiation shifts under binders; a rule is (depth, left,
  right); rewriting at the root, at the first position, and the count of applicable positions.
- **`lib/nav.cub`**, navigation. A view is a list of rules; the available steps from a term are the
  rules that apply; a step rewrites; the choice is by the size of the result, with a goal pattern
  taken first; navigation runs to a bound, to the goal, or until nothing applies. Validity never
  enters the choice: every step is a transport along an identity in view.
- **`lib/specialize.cub`** (after the checker), the pipeline: a closed code is normalized by the
  machine (`eval`, then `quote` at its type), rewritten along rules to a bound, lowered. A known
  computation becomes a literal, an open function a λ over a primitive, word elimination with an
  unknown bound a loop; the native evaluation agrees with the normal form, and `run.py` compiles one.
  `residual sch T rules c env` substitutes the schedule's leading `st-now` binders from `env` first:
  a choreography's projection to a role is the residual of `λ m → project-raw m g` with the role
  now (`tests/choreo-spec.cub`).
- **`lib/lin.cub`**, linear arithmetic. Normal forms for words built from literals, variables,
  addition and successor; equality by normal form; order over naturals by pointwise domination; a
  Farkas certificate (nonnegative multipliers of hypotheses `l ≤ r`) verified by `check-cert`, and a
  bounded search whose answer is checked before use. The reading is a decoder's obligation
  `off + 4 ≤ n` from `off ≤ i` and `i + 8 ≤ n`.
- **`lib/rules.cub`** (after the checker and `lib/std.cub`), the store's identities as rules. A
  checked definition whose type is `Π x1 … xk. l ≡ r` is read through `quote-ty`; `rule-of` makes
  `l → r` with the binders as metavariables, `view-of` a navigation view from named theorems. The
  endpoints come from the type's value, so they are in normal form, as the specializer's terms are:
  `plus-assoc` rewrites the normal form of `(a+b)+c` to that of `a+(b+c)`, navigation reaches it in
  one step, and `plus-zero` is consumed by the specializer so that what is lowered is the identity;
  `run.py`'s `law` case takes `plusw-zero-l` through the specializer to a compiled identity, and its
  `assume` case prints an assumed fact as `ASSUME(expr)` for the C compiler.

Four more are typed, loaded after `lib/std.cub`, and are theorems the machine checks: the method of
reflection, where a decision procedure is a term, its soundness is checked once, and each use is an
evaluation.

- **`lib/linproof.cub`**: the linear normalizer over words with `same-norm-eq` (equal normal forms,
  equal values), order as a witnessed sum, and `cert-sound`: a Farkas certificate that checks
  proves `a ≤ b` wherever the hypotheses hold. Multiples are repeated addition, so no multiplication
  law enters; the order check is an equality test on `f + (g − f)`, so the subtraction need not be
  verified.
- **`by-lin`** fills a hole whose type is `Lew a b` (words) or `leZ a b` (integers): the goal and the
  hypotheses of those types in scope are read as linear codes over their atoms, `lib/lin.cub`'s
  search finds a certificate while elaborating (untrusted), and the hole becomes `cert-sound` at
  the atoms' environment, which the checker verifies by normalization. Needs `lib/lin.cub` and
  `lib/linproof.cub` (and `lib/intproof.cub` for integers). `leZ-add` is
  `leZ-add a b c d h k = by-lin`.
- **`lib/eqproof.cub`**: equality certificates over uninterpreted functions. A certificate is a
  trace of steps (position, hypothesis, direction, substitution); `eq-cert-sound`: a trace that
  checks proves `s = t` in every model where the schematic hypotheses hold at every valuation.
  `find-trace` searches (unverified) and what it finds is checked.
- **`lib/ring.cub`**: the commutative semiring of `Nat` decided by reflection. Expressions
  normalize to sorted sums of monomials; `ring-eq : norm a ≡ norm b → xeval e a ≡ xeval e b` from
  the semiring laws of `lib/std.cub`. `(x+1)² = x²+2x+1` is a theorem at every valuation with the
  proof term a constant path, one evaluation of the normalizer; a non-identity is rejected.
- **`lib/autodiff.cub`**: `D` the symbolic derivative over polynomial codes, `fwd` forward-mode
  differentiation by dual numbers, `fwd-der` (they agree), and `taylor`: `p(x+h) = p(x) + h·p'(x) +
  h²·q` with `q` computed, each inductive step a ring identity discharged by the normalizer.

Every top-level name in `lib/` is unique (`tools/names.py`, first in `make test`): a later module's
name rebinds the references of every module parsed after it.

The back end, as it stands (all libraries, each with readings; `experiments/fo/run.py` compiles
seventeen programs with gcc and runs them, with both printers):

- **`lib/lower.cub`** (after the checker): codes to the first-order form, with the data table's
  shapes: a `case`/`rec` becomes a recursion form (`frec`: the fields pushed, then a suspended
  recursion per recursive field), which `lift` turns into a definition that calls itself on the
  field. The length of a list written with `rec` is a C function calling itself on the tail.
- **`lib/specialize.cub`** also takes the schedule as a value (`specialize-s`); `program-of` lowers a code and, transitively, the definitions it calls;
  `fo-report` lists the forms of a code outside the fragment.
- **`lib/repr.cub`** consumes the representations: calls of `load64`, `store64`, … become native
  primitives on the arguments that carry information (the length and the bound proof dropped, the
  proof becoming an `ASSUME` around the access), the conversions vanish, the word operations and
  `plus` on represented `Nat` go native, closed `Nat` trees become words by `%rtab`. Two loads from
  an unknown buffer, written with their proofs, compile and run. `program-rep reps fuel c` is the
  whole program with every declared representation consumed (B12 §6): definitions lowered from
  their code as written (`%gcode n`), operations with an `op` line replaced by their natives,
  constructors of a represented type through `to`, recursion on one through `from`, by the
  representation's table `%rops d`; the printers lay a field of a type represented by `Word` out as
  a word.
- **`lib/foproof.cub`**, **`lib/foproofd.cub`**: the justifying path of compilation for the word fragment (18.5 step 7):
  the fragment's codes and first-order forms mirrored as checked data with checked evaluators and
  lowering, one lemma per form (`lower-sound`), verified rewrites (`VRule`, from the store's word
  theorems) applied with soundness by congruence, and `spec-path : fo-ev (spec-w r c) ρ ≡ wc-ev c ρ`
  for every code; `ex-path` is that path for a term. Bridges read real `Code` and `FO` into the
  mirror (the untyped `lower` agrees with the checked one on the readings) and the checked
  pipeline's output back for the printers. Past the word fragment (`lib/foproofd.cub`), a mirror of
  the whole first-order fragment: every primitive `lower` lowers (Int64's, UInt32's as words,
  Float64's, by their bits, the machine's operation as the evaluator clause), comparisons, Bool as
  1 and 0 with if, max, pairs, unit and projections, constructors of any data type with any number
  of fields (a tuple) and cases of any number of alternatives, closures, calls of any arity (a
  tuple), the byte layer at 4 and 8 bytes and any literal offset (a string as its stores over a
  base, a partial overlap read byte by byte), and checked closure conversion `dconv` with
  `dc-sound` at every fuel. The bridge lifts a structural recursion on a recursive type, and a word
  recursion, to definitions of their own called through the table.
  `specialize-proved` takes the word route (`spec-path`), else the data route: the normal form
  read into the mirror (`dc-prog`, globals quoted and read), its tables decided by `dtok-chk` (so
  `dc-sound` holds through `dtok-auto`), converted and taken to first-order definitions (`run.py`
  `proved-spec`, `proved-region`), else the untyped specializer, tagging which. The data route's
  rewrites are verified too: `FRule`s on the converted form (folding every primitive, `0 + x`, a
  load forwarded through stores at its width, a projection of a pair and an if on a literal) sound
  as refinement at every fuel, applied bottom up under every construct and through both tables
  (`fr-bu-ok`), composed with `dc-sound` in `fr-dc-path`; `run.py`'s `proved-cmp`, `proved-tags`,
  `proved-list`, `proved-loop`, `proved-num`, `proved-float`, `proved-calls`, `proved-bytes`,
  `proved-bytes32` and `proved-pair` compile its output boxed and unboxed. Offsets that are variables (`fromNat` of the `Nat`), `%blen` and `max` are read too, on both routes (the untyped `lower` had loaded at 0 for any `Nat` offset and run `max` as addition); `proved-fib` runs the word route's fib as x86.
- **`lib/foproofq.cub`**: the last link of the path, from the mirrors to the first-order form the
  back ends print. Both printers are checked functions into `FQ` (lib/fo.cub's `FO` as checked
  data, printed constructor for constructor by `fq-to-fo`); `evq`, a checked evaluator by fuel, is
  the first-order form's meaning (read against `eval-fo` on every form the printers emit,
  `tests/foproofq.cub`); `q-sound` and `fw-sound` prove the printers right, and `q-path-n` /
  `fw-path` compose them with the mirrors' paths: what a source program computes at every fuel and
  argument, the form handed to the back ends computes. Every `run.py` proved program's instance is
  checked (`tests/foproofq-progs.cub`, generated by `tools/gen_foproofq.py`, its tables tied by
  readings to what `specialize-proved` hands over).
- **`lib/foproofe.cub`**: the first-order form's checked meaning on its own (`KQ`, `q-prim`, `evq`),
  so the back-end links load it without the printers' proofs.
- **`lib/foproofc.cub`**: the C link. `CA`, the C subset `lib/emitc.cub` prints, as checked syntax,
  and `evu` its meaning by fuel on C's cells and named locals over unbounded words (stricter than C at the points the
  file's header lists); `fq-to-ca` the checked translation and `ca-sound` its proof (the C's value
  refines `evq`'s at every fuel), composed into `ca-path-n` (data route) and `ca-fw-path` (word
  route); `ca-to-c` prints `CA` construct for construct. `run.py`'s proved programs print their
  boxed C through it, and each one's path to the C is checked (`tests/foproofc-progs.cub`,
  generated by `tools/gen_foproofc.py`). `lib/foproofm.cub` gives `evc`, the same meaning on C's
  `uint64_t` words (a limit hit, no claim, wherever an addition, multiplication or literal leaves 64
  bits), and `m-sound`: whenever the C runs without a limit hit its value refines `evq`'s
  (`cm-path-n`, `cm-fw-path` in `lib/foproofcp.cub`; the per-program paths end there). Outside the
  path on the boxed route: `ca-to-c`'s text and gcc; the unboxed and x86 printers run from the
  first-order form, outside it.
- **`lib/foproofu.cub`**: the unboxed C link's first piece (U1), kinds for the first-order form: a
  word (`kw`: a word, an Int64 or a Float64, a C `uint64_t`), a byte string (`kb`), a constructor of
  data type `d` (`kdc d`, its fields' kinds from a per-program table), anything (`ka`, boxed), no value
  (`kdn`). `ksyn` gives a term's kind, `kok` decides the conditions (a constructor's fields of its
  table's kinds, a call's arguments of its callee's parameters' kinds, a loop's state of its kinds),
  and `kind-sound` proves them: at every fuel, a term meeting them in an environment of its kinds has
  its kind under `evq`. `kinfer` finds a program's tables (outside the path: any tables `kok` accepts
  are sound), and every `run.py` proved program's tables are inferred, decided and `kind-sound`
  instantiated (`tests/foproofu-progs.cub`, generated by `tools/gen_foproofu.py`). The unboxed printer
  guesses by first use (`lib/emitu.cub`'s `infer-u`), which is not sound by construction; these kinds
  are what the unboxed route will print by.
- **`lib/foproofx.cub`**: the x86 link's syntax and meaning. `XA`, the x86-64 `lib/emitx86.cub` emits,
  as checked syntax; `evx` the machine's meaning by fuel on registers and one memory (a limit hit
  wherever a word leaves 64 bits or an access leaves the stack or arena); `fq-to-xa` the checked
  translation; `xa-prog` the encoder, equal to `emit-x86` byte for byte (`tests/foproofx.cub`, where
  `evx` agrees with `evq` on every construct and on the proved programs). `run.py`'s X86 route prints
  the proved programs through it. Its soundness proof (`xa-sound`) is not built yet.
- **`lib/derive.cub`**: `derive-eq d`, a decidable equality as a code read off the data table;
  evaluated, quoted back at its type, lowered and compiled (the derived equality on `Nat` prints
  10 for eq(2,2), eq(2,1)).
- **`lib/decode.cub`**: a byte decoder whose access bounds are certificates checked by
  evaluation (`cert-sound` of `lib/linproof.cub` across `Word ≃ Nat`), including a loop over
  messages with one certificate serving every offset.
- **`lib/proc.cub`**: processes over a signature in Church form, worlds as state handlers that may
  refuse, outcomes, handlers between signatures, supervision with restart strategies, and replay
  (a recording world and a replaying world). A process run against its world with the store
  unknown specializes to a function of the store and compiles.
- **`lib/emitt.cub`**: C type declarations from the layouts.
- **`lib/emitu.cub`**: the C printer with words, pairs and flat data unboxed. Every local,
  parameter and return is typed by inference (a parameter by its uses, a return by its body, calls
  by their callees); a loop of `lower-welim`'s shape is a `for` over typed state; pairs are structs
  by value; a data type whose layout is a union of structs of words (a field of `UInt32`, `Int64`,
  `Float32` or `Float64` at its C width) is its typedef by value, with
  `box_d`/`unbox_d` generated where a boxed value meets an unboxed one. Two loads from a buffer
  print as `uint64_t f(V *b) { return ld64(b, 16) + ld64(b, 0); }` under their `ASSUME`s; `fib` by
  word elimination over a pair is a `for` over a `uint64_t` and a `Pr0` with no allocation.
  `run.py` runs every program with both printers (`UNBOXED=1`).
- **`lib/image.cub`**, **`lib/reimport.cub`**: the C++ image (18.9) and its re-import for
  translation validation. A checked definition becomes a C++20 function (a type parameter a
  template parameter, a data type structs and a `std::variant`, a case a `std::visit`), read back
  equal to its source; records of operations become concepts and instances, codata types classes
  with lazily computed observations, represented types classes wrapping their representation, and
  `cxx-module` writes the image as a module interface unit. `cxx-image-cfg` specializes the image
  against a configuration (18.15): the platform's word width, byte order and alignment, and the
  instantiations consumers recorded (`$CUB_CONFIG_OUT`) as explicit specializations.
  `IMAGE=1 run.py` compiles twelve programs with g++ and runs them.

## Measurements

Every reading reports the steps taken. The sorted list of 9.15 reads in 1.6k steps (eight words).
The library (`lib/std.cub`, elaborated and checked) loads in 1.4M steps, and with the paper's terms
(`tests/boxes.cub`) in 3.1M; elaboration runs the checker again when it has solved holes, and a
case given a motive from its expected type carries it at run time. Naive `fib` of a computed 20
reads in 36k steps, of 25 in 278k, and of 20 written out in 36k; the iterative `fib` reads 21k and
226k (`tests/memo.cub`; without the memo, naive `fib` takes 294k and 3.8M). `CUBICAL_KEYS=1` traces
the keys registered, fired and hit on stderr, and the entries left at the end. The development of
`Orbit.agda` (the codata types, `unfold`, `mapO`, the four corecursive maps between paths and
bisimulations, and `(x ≡ y) ≃ (x ≈ y)`) checks in 60k steps, `iso1` (a corecursive square) in 13k.

Brunerie's number, by an untyped port of Ljungström's `brunerie'` from agda/cubical
(`experiments/brunerie/`, about 1 s in Agda), reads `negsuc (suc zero)`, that is -2, in 1.26M
steps: 0.4 s for the reading, 1.1 s to load the cached library (one thread; the library
elaborated, compiled and its checks skipped); `make test` checks it with one and four workers. The port has no
placeholders: isSetℤ is Hedberg's theorem in the library, and the 4-truncation case of the Hopf
H-space structure is a checked filler over families of sets. The pipeline
eliminates the circle and S² into truncations, the Hopf fibration and `Code` into the universe,
and transports through the proofs that the types of sets and of groupoids have the next h-level.
Before compositions of types were a type former and the glue base was held unbuilt it ran out of
7 GB of memory.
