# Track B — the x86 model as terms (Phase 2, first half of D2)

Read `README.md` here first (rules), then `../PLAN.md` §0, §5, §6 Phase 2.

Files you own: `lib/bits.cub`, `tools/sail2cub.py`, `lib/isa/**`, `experiments/isa/**`, this file.

## What is built

1. **`tools/sail2cub.py`**: reads the Sail x86 model and writes `.cub`.
   Source: `git clone https://github.com/rems-project/sail-x86-from-acl2` at commit
   `0c6c023dccc7c20d8d9dbfc481f01d9bb49d62bf`, directory `model/` (33k lines, 528 functions,
   generated from ACL2's x86isa, so the Sail subset is small: `val`/`function`, `let`, `if`,
   `match`, tuples, bitfield structs like `rflagsbits` with `[x with f = v]` updates and
   `Mk_…` constructors, `register` declarations, enums, `bits(n)`/`sbits(n)`, `int`, `nat`,
   `bool`, `throw`/model errors, the built-ins in `model/prelude.sail`). Do not vendor the Sail
   files; record the commit here and commit the generated `.cub` under `lib/isa/x86/`,
   one `.cub` per `.sail` file, loaded in `$include` order (write the order to
   `lib/isa/x86/ORDER`).
2. **`lib/bits.cub`**: what the generated code is written against.
   - `bits(n)` is `Word` (the machine's unbounded natural) kept below 2ⁿ by masking; the type
     is `Bits n = Word` (a plain alias: the model is first-order code, not a place for
     proofs). Operations: add/sub/mul modulo 2ⁿ, and, or, xor, not, shifts, slices
     `x[hi..lo]`, concatenation, zero/sign extension, truncate, `unsigned`, `signed`,
     `bits_of_int`, equality, comparisons.
   - Sail `int` is `ZI` (a sign and a `Word` magnitude: `data ZI = zpos (n : Word) | zneg (n : Word)`
     with `zneg 0` never built), with add, sub, mul, compare, div/mod as the prelude needs.
   - `Word` arithmetic (`lib/prim.cub`, on numerals of any size, 3 net steps each): `wmul wand
     wor wxor wshl wshr wquot wrem`, and `%wadd %wsub %weq %wlt %wmax %wsucc`; `x / 0 = 0`,
     `x mod 0 = x`, `wshr` by any count, `wshl` by at most 2³² bits. (`wdiv`/`wmod` are
     `lib/lin.cub`'s names for the same two.)
3. **Machine state and effects.** Registers and memory are a state value threaded explicitly:
   `X86 = ` a record of the registers the model declares, and memory as a function-free map
   (a `List (Word × Word)` of bytes written over an initial image `Bytes n` at a base address
   is enough to start; say what you chose and why). A Sail function with register or memory
   effects becomes `args → X86 → Res A × X86` where `Res A = ok A | fault Word` (model
   errors and `throw` become `fault`); pure functions stay pure. Decide purity per function by
   a fixed point over the call graph.
4. **`lib/isa/x86/run.cub`** (hand-written, small): `x86-step : X86 → Res Unit × X86` (the
   model's `x86_fetch_decode_execute`) and `x86-run : Word → X86 → …` (n steps, by `%welimP`).
5. **Scalar SSE** (`lib/isa/x86/sse.cub`, hand-written): the Sail snapshot has none. Add
   `addsd subsd mulsd divsd ucomisd cvtsi2sd cvttsd2si movq(xmm,r64)` over the machine's
   `Float64` primitives of `lib/prim.cub` (`f64add` …, `f64bits`, `f64ofbits`), dispatched from
   the two-byte opcode map where the model reports them unimplemented.
6. **`lib/isa/loader.cub`**: an ELF64 static executable (`Bytes n`) to the initial `X86`
   (segments mapped, `rip` at the entry, `rsp` at a stack top, SysV). `lib/elf.cub`'s
   `elf-image` is the writer whose output it must load.

## Acceptance (each is a command whose result you record under Done)

- `make library` still passes, and loading `lib/bits.cub` and every file of `lib/isa/x86/ORDER`
  after the library ends `N passed, 0 failed`. Every generated function has a signature and
  checks (no `postulate`, no skipped function: one the importer cannot translate is listed
  here with the construct that stopped it, and then the importer is extended).
- `experiments/isa/add.cub`: a state with bytes `48 01 C8` (`add rax, rcx`) at `rip`, `rax`
  and `rcx` **variables** (a λ over them): `#show` of `x86-step` on it has `rax` equal to the
  64-bit sum of the variables, `rip + 3`. Record the steps. Then the same for one instruction
  of each class the encoder emits (`lib/x86.cub`'s template table lists them: mov forms,
  add/sub/imul/and/or/xor/shl/shr/sar, cmp + jcc/setcc, push/pop, call/ret, lea, div/idiv,
  rep movsb/stosb, syscall).
- `experiments/isa/hello.cub`: `x86-run` on the loader's state of the bytes of
  `build/feed`-style binary `/tmp/hello.elf` (build it with `experiments/native/`; or load
  `lib/elf.cub`'s `elf-image` of a 3-instruction program) up to its first `syscall`: record
  the steps per instruction.

## Needs from track A

(none: the Word operations landed, see item 2)

## Done

- **Importer, lexer and parser** (`tools/sail2cub.py`, `--parse`). Sail model cloned at
  `0c6c023dccc7c20d8d9dbfc481f01d9bb49d62bf` to `/home/claude/ext/sail-x86-from-acl2` (not vendored).
  `python3 tools/sail2cub.py /home/claude/ext/sail-x86-from-acl2/model --parse` reads the 11 files
  of the model's Makefile (`PRELUDE_SAILS`, `step.sail`, `main.sail`) and everything they
  `$include`, each once, in include order: 63 files, 528 functions, 501 `val`, 38 bitfields,
  36 registers, 27 overloads, 7 type synonyms, 4 structs, 1 enum, 1 union, 1 scattered function.
  Constructs the parser had to learn beyond the list in item 1: `struct { f = e }` literals,
  `let x : int : int = …` (a doubled annotation), an annotation binding as an atom
  (`[false, …] : vector(256, dec, bool)[unsigned(opcode)]`), `foreach`, `try`/`catch`.

- **`lib/bits.cub`** (checks after the library: `850 passed, 0 failed` with it loaded; 20 new
  definitions' worth of probes agree with Sail's meaning, e.g. `bv-signed #8 #200 = zneg #56`,
  `bv-of-int #9 (zneg #56) #0 = #456`, and the open term
  `λ a c → bv-trunc #65 #64 (bv-of-int #65 (z-add (bv-unsigned #64 a) (bv-unsigned #64 c)) #0)`
  normalizes to `%wand (%wand (%wshr (%wadd a c) #0) (2⁶⁵−1)) (2⁶⁴−1)` in 43 steps). Contents:
  `Bits n = Word` and `bv-*` (mask, add, sub, mul, and/or/xor/not, eq, bit, slice, update,
  append, zext/trunc/sext, shl/shr/sar, replicate); `ZI` (`zpos`/`zneg`) and `z-*` (add, sub, mul,
  eq, lt, le, abs, tdiv, tmod, pow2, shl, shr, nat, set-slice); `bv-unsigned`, `bv-signed`,
  `bv-of-int` (= `get_slice_int`), `bv-add-int`; `Text = Word` (a string is its bytes base 256)
  with `str-len`, `str-cat`, `str-eq`; `Option` (`None`/`Some`), `is_some`; vectors as `List`
  with `vec-get`/`vec-set`; `ByteMap` (memory: a binary trie of bytes over the 64 address bits,
  lowest bit first, `mem-load`/`mem-store`, unwritten = 0); `Outc E A = out-val a | out-throw e
  | out-abort why`; `fl-bind`/`fl-assert`/`fl-try` (outcome only) and `State S E A = S → Outc E A × S`
  with `st-ret`, `st-lift`, `st-bind`, `st-try`, `st-foreach`.
  Measured: one `mem-store` or `mem-load` is about 1 400 steps (64 levels × ~20); two stores and
  a load, 4 187 steps.

- **Importer, type pass and emitter** (`tools/sail2cub.py MODEL --out lib/isa/x86`): writes
  `state.cub` (the `X86` data type: `mem : ByteMap` and the 36 registers, a projection
  `x86-R` and an update `x86-set-R` each) and one `.cub` per `.sail` (63), and `ORDER` (64 names).
  Its report: `functions: 505 translated (152 pure, 118 outcome, 235 state), 0 not translated`
  (the model's 528 `function` items are 505 functions: `__SetConfig` has 24 clauses);
  `moved to a later file: 56`; `undefined: prelude.sail:82`; `prints dropped: 41`.
  45 188 generated lines, 3.9 MB.
  Check: `./build/cubical <library> lib/bits.cub $(sed 's|^|lib/isa/x86/|' lib/isa/x86/ORDER)` ends
  `3574 passed, 0 failed` (the library's 830, `lib/bits.cub`, and 2 715 definitions of the
  model; no `postulate`, every definition has a signature).

## Decisions (what the generated code is, so far)

- Memory: a trie rather than a write list over `Bytes n`, because `Bytes n`'s loads need a
  `Nat` offset with a proof of fitting (not available for a computed address) and a list read
  costs its length; the trie reads any address in 64 levels whatever was written, and a write
  of a variable byte at a known address is an ordinary constructor.
- Three effect classes, not two: a function that only asserts or throws (no register, no
  memory) is `args → Outc exception A` (so Sail's `assert` keeps its outcome without threading
  the state through the flag specifications); one that reads or writes registers or memory is
  `args → X86 → Outc exception A × X86`; the rest are pure.
- The outcome type carries the model's own `exception` union (`Emsg`, `Syscall`), not a bare
  `Word`: `out-throw e`. `out-abort why` is a failed `assert` (Sail aborts) or a recursion that
  passed its stated bound.

- Names. `Res`, `ok`, `fault` are taken by `lib/check.cub`, so the outcome type is `Outc` with
  `out-val`/`out-throw`/`out-abort` (also taken, so not used: `Str`, `Memory`, `Outcome`, `Opt`,
  `none`/`some`, `St`, `mem-write`). Hand-written names use hyphens and a prefix (`bv-`, `z-`,
  `str-`, `vec-`, `mem-`, `fl-`, `st-`); names from Sail keep their underscores.
- Cub facts the emitter relies on (probed): a `let` binding needs a signature line when its value
  has no inferable type (`let x : T` newline `x = e`); a `let` binds a name, not a pattern (tuples
  are taken apart with `fst`/`snd`); a later `let` of the same name shadows; identifiers may
  contain `_`, `?`, `'`; `%wsub` is truncated subtraction (3 − 5 = 0).
- Run a file against the cached library:
  `CUBICAL_STORE=/tmp/trackB/store ./build/cubical $(make -s -n library | sed -n 's/^.*cubical \(lib\/prelude.*\)$/\1/p') FILE.cub`
  (first run 20 s, later runs under 1 s).

- What each Sail construct becomes (the importer's rules, all in `tools/sail2cub.py`):
  - `bits(n)` → `Bits n`; a width is a numeral or a term over the function's `int('n)`
    parameters (`wmul #8 (z-nat nbytes)`); an integer type variable no `int('n)` parameter
    gives is a leading `Word` parameter `w_n`. `int`, `nat`, `range`, `{|…|}` → `ZI`. A
    bitfield is an alias of its `Bits` with `B-f` / `B-set-f` per field; `Mk_B` and `.bits` are
    the identity. An enum is a data type with `E-ix`; a struct a one-constructor data type;
    `option` → `Option`; `list` and `vector` → `List` (a `dec` vector literal reversed, so
    position is index); a string → `Text` numeral; a tuple → right-nested pairs.
  - An effectful subexpression is bound before the expression that uses it, left to right
    (`bindS {A} {B} m (λ x → …)`, `bindF` for outcome-only, `onS` for an outcome inside state
    code); a register read is `getS x86-R`, a write `putS (x86-set-R v)`; `__read_mem` /
    `__write_mem` are `x86-read-mem` / `x86-write-mem` (`__write_mem_ea` is `unit`).
    `&` / `|` with an effectful right operand read it only when the left does not decide.
  - `match` on literals: an `if` chain in the cases' order; with 8 or more cases each case is
    a definition `F-caseN` over the locals it mentions and the literal is found by halving
    (`wlt`), the default at every leaf. A literal match with no wildcard takes its last case as
    the rest (Sail accepted it as complete). `match` on an enum, option, list or union: `case`.
  - `throw e` → `out-throw e`; `assert(c, msg)` → `fl-assert c msg` (without a message, the
    Sail `file:line`); `try`/`catch` → `tryS`/`tryF`, an exception no handler names goes on;
    `return e` is accepted in tail position only (all 17 are); `foreach` → `foreachS` over the
    variables its body assigns; a block-local assignment is a shadowing `let`.
  - Recursion (7 functions) is on a stated bound, `F-body` (open recursion) under `%welimP` in
    `F-fuel`; the bounds and why they suffice are the table `BOUNDS` in the importer:
    `feature_flags_fn`, `in_list` (list length), `logcount_int`, `logand_int` (magnitude + 2),
    `bitcount8` (9), `get_prefixes` (`cnt`), `x86_run_halt` (`maxSteps`). Past the bound an
    outcome function aborts `recursion-bound`; the three pure ones return their type's default.
  - A function is emitted in the latest of its own file and the files of what it calls (56 move:
    e.g. `rb`, `wb` to `paging.cub`); a comment says where each came from.
  - A Sail global whose name the library mentions gets `_` appended (`rb_`, `abs_`, `mod_`,
    `ms_`, `fault_`, `exception_` when the library has the word); a local with a global's name
    gets `'`.
  - `undefined` (one use, `undef_read_logic : nat`) is the type's default, `zpos #0` (the
    model's comment: Sail's C backend returns zero there). `print_endline`, `print_int`,
    `print_bits` are `unit`: the emulator's console is not in `X86`.
- Found while checking: a definition the size of `two_byte_opcode_execute` as one term (4 000
  lines, a 256-deep `if` chain) exhausts 6 GB in the checker; hence the per-case definitions.

## Found

(defects in the machine or the model you hit, with the input that shows them)

## Next step

1. `lib/isa/x86/run.cub` (hand-written): `x86-step` = `x86_fetch_decode_execute`, `x86-run n`.
   Then `experiments/isa/add.cub`: a state with `48 01 C8` at `rip` in memory (`mem-store-n`),
   64-bit mode set as `main.sail`'s `initialise_64_bit_mode` does (`msrs[0]` bit 10 `lma`,
   `seg_hidden_attrs[1]` bit `l`), `app_view = true`, `rax` and `rcx` λ-bound; `#show` the step.
   Whatever does not reduce (a stuck test on a variable, a wrong built-in) is the next thing to fix.
2. Then one instruction per class of `lib/x86.cub`'s template table, `sse.cub`, `loader.cub`,
   `experiments/isa/hello.cub`.
