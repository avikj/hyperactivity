# Track A — the compiler and integration

Files: `lib/specialize.cub`, `lib/machine.cub`, `lib/x86.cub`, `lib/elf.cub`, `src/*.c`,
`lib/check.cub`, `lib/code.cub`, `Makefile`, `PLAN.md`, `experiments/jit/`, `experiments/build/`.

## How to verify the current state (run after any change here)

```
make build/cubical && make library                     # 830 passed, 0 failed
L=$(make -s -n library | sed -n 's/^.*cubical \(lib\/prelude.*\)$/\1/p')
CUBICAL_NOCACHE=1 ./build/cubical $L experiments/jit/workloads.cub | grep -c '^ok: jit'   # 33
CUBICAL_NOCACHE=1 ./build/cubical $L experiments/build/feed.cub | grep build              # ok: build feed-step
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

## Next step

1. **Arena reset for unbounded processes.** Today a `#build` binary never frees: 1 GB is enough
   for ~10⁷ commands and then it exits 3. Design: `Step` gains nothing; the driver loop, after
   applying the continuation, holds the only live value (the next step). Copy that value to the
   other half of the arena (a two-space copy over cells: tag word, fields; closures
   `[off][arity][given][args]`; bytes `[len][data]`; immediates < 4096 and words are
   indistinguishable from pointers, so the copy must be **typed**: generate, per program, a
   copy routine from the Step's continuation closure layouts) — or simpler and exact: make the
   loop state explicit. Preferred: a second entry form `#build-loop init step`, with
   `init : St` and `step : St → Step W St` for a first-order `St`; the driver copies `St` by
   its type (the JT-style walk `jit_in` does, emitted as code), resets the arena, and loops.
   Acceptance: feed over 10⁸ messages with constant memory.
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
- The driver's `w-load`/`w-store`/`w-cas`/`w-fence` templates in `lib/elf.cub` have never run
  (no process in the repo obtains an address). Open until track D gives one.
