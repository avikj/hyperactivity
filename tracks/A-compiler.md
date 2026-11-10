# Track A — the compiler and integration

Files: `lib/specialize.cub`, `lib/machine.cub`, `lib/x86.cub`, `lib/elf.cub`, `src/*.c`,
`lib/check.cub`, `lib/code.cub`, `Makefile`, `PLAN.md`, `experiments/jit/`, `experiments/native/`.

## How to verify the current state (run after any change here)

```
make build/cubical && make library                     # 830 passed, 0 failed
L=$(make -s -n library | sed -n 's/^.*cubical \(lib\/prelude.*\)$/\1/p')
CUBICAL_NOCACHE=1 ./build/cubical $L experiments/jit/workloads.cub | grep -c '^ok: jit'   # 33
for f in feed feedloop sys; do CUBICAL_NOCACHE=1 ./build/cubical $L experiments/native/$f.cub | grep build; done   # three ok lines
./build/sys; echo $?                                    # A, then 65
printf '\x02\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x05\0\0\0\0\0\0\0\x01\0\0\0\0\0\0\0\x03\0\0\0\0\0\0\0' | ./build/feed | od -An -td8   # 5 then 2
```
Always run with `stdbuf -oL` when hunting a crash: output is otherwise lost and the last line
printed is not where it died.

## Done

- `#jit f [using t…]`: specialize, check (conversion, or the path along theorems as `f·jit`),
  encode, bind. 33 workloads.
- Closures and the apply routine; codata records; copattern definitions (lazy for recursive
  codata); recursion by a measure; erasure of arguments no result depends on; η-contraction;
  types as values are 0; stack limit and arena overflow leave by the overflow exit.
- `#build s "file"` (`lib/elf.cub`): `s : Step W A` as a static ELF. The feed demo's output
  equals the expected bytes on 1M messages, 1.07 s against 0.91 s for the same loop in C.
- The elaborator binds each name once; an unreachable clause is refused.
- `#build-loop init step copy copy-id "file"`: a process that runs forever in bounded memory.
  The arena is two halves; each round the state is copied into the other half by the program's
  own `copy` (checked equal to the identity by `copy-id`), so the last round's memory is free.
  `experiments/native/feedloop.cub`: 1M messages byte-equal to the expected output in 0.82 s;
  20M messages, final value −2,364,566 (equal to an independent computation), resident memory
  52 kB throughout. `w-alloc` is its own `mmap` (permanent); reused arena memory is zeroed where
  a command promises zero bytes.
- Word operations `wmul wquot wrem wand wor wxor wshl wshr` on numerals of any size.

## Next step

1. **Derive `copy` and `copy-id`** for `#build-loop` from the state's data table (`lib/derive.cub`
   derives equality the same way), so the directive takes `init step "file"`; and a copying
   identity for `Bytes n` fields (today a state holding byte strings is refused).
2. **Registers.** Every value lives in the frame. `Sched.regs` is unused. Assign the first six
   let-bound values of a definition without calls in between to rbx/r8–r11 (caller-saved
   around calls); measure feed and `sumL` before and after.
3. **Specialization on arrival** (PLAN 4.4 item 6): `#jit f at k` rebuilt on the new compiler:
   when argument k is a closed value, specialize `f` with it substituted, check, encode, cache
   under the content key of the specialized code.
4. **Apply the "Needs from track A" sections** of the other tracks as they appear (check each
   track file after every `git pull`).
5. **Object-model interface for C++** (unblocks track F): fix in `tracks/F-cxx.md` the types
   `Mem`, `Ptr`, `Val`, the monad of C++ evaluation, and the signature of `read-expr`,
   `read-stmt`, `read-decl`, before F starts.

## Found (defects seen, with their status)

- The net segfaults normalizing a unary `Nat` numeral of 10⁶ (`to-step (serve 1000000 …)`):
  C recursion depth in `src/net.c`. Open. It should be an error, not a crash.
- `%gsteps`-scale compile cost: `zig` 1.3M steps, `feed-step` 1.2M steps to compile. Open
  (PLAN 4.4 item 8).

## Replies to other tracks

- **D, request 1 (ELF driver cases for `w-sys`, `w-alloc`, `w-load8`, `w-store8`)**: done.
  The driver loop's source is `tools/asm/driver.s` (do not write another); `python3
  tools/asm/gen.py --patch` assembles it into `lib/elf.cub`. `experiments/native/sys.cub`
  builds a binary using every memory and raw-syscall command; it writes `A` and exits 65.
- **D, request 2 (`.gitignore` hid `experiments/build/`)**: the demos are now in
  `experiments/native/` and tracked.
- **D, request 3**: say when `lib/os/*.cub` load and I append them to `LIB`.
