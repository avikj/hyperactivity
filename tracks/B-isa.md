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

## Decisions (what the generated code is, so far)

- Names. `Res`, `ok`, `fault` are taken by `lib/check.cub`; the outcome type is named in
  `lib/bits.cub` when it is written (see Next step).
- Cub facts the emitter relies on (probed): a `let` binding needs a signature line when its value
  has no inferable type (`let x : T` newline `x = e`); a `let` binds a name, not a pattern (tuples
  are taken apart with `fst`/`snd`); a later `let` of the same name shadows; identifiers may
  contain `_`, `?`, `'`; `%wsub` is truncated subtraction (3 − 5 = 0).
- Run a file against the cached library:
  `CUBICAL_STORE=/tmp/trackB/store ./build/cubical $(make -s -n library | sed -n 's/^.*cubical \(lib\/prelude.*\)$/\1/p') FILE.cub`
  (first run 20 s, later runs under 1 s).

## Found

(defects in the machine or the model you hit, with the input that shows them)

## Next step

1. Write `lib/bits.cub` (`Bits`, `ZI`, the outcome type, the state monad's bind) against
   `wand wor wxor wshl wshr wquot wrem` of `lib/prim.cub`.
2. In `tools/sail2cub.py` add, after `Model`: the type pass (every expression gets its Sail type;
   bit widths are either a Python int or a cub term of type `Word` built from the function's
   `int('n)` arguments), overload resolution by argument types, the effect fixed point over the
   call graph, and the emitter (`--out lib/isa/x86`). First target: `lib/isa/x86/add_spec.cub`
   and what it includes (`rflags_spec`, `structures`, `prelude`) checking.
