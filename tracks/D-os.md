# Track D — the operating system as a typed world (Phase 3; needed by D5)

Read `README.md` here first (rules), then `../PLAN.md` §6 Phase 3, and `lib/proc.cub`,
`lib/world.cub`, `src/drivers.c`, `lib/elf.cub` (the driver loop as machine code).

Files you own: `lib/os/**`, `experiments/os/**`, this file. You may also edit
`lib/world.cub` and `src/drivers.c` **for item 1 only**, in one commit, announced under Done.

## What is built

1. **One raw command.** Add to `WCmd` (at the end, so existing constructor indices do not
   move; `lib/elf.cub` dispatches on them) `w-sys (nr a1 a2 a3 a4 a5 a6 : Word)` with response
   `Word` (the kernel's return value as a 64-bit word, errors as the two's complement of
   errno, exactly what the `syscall` instruction leaves in `rax`), and its case in
   `src/drivers.c` (`syscall(2)`). Buffers are addresses: a process needs memory it can name.
   Add `w-alloc (n : Word)` with response `Word` (the address of n fresh zero bytes that stay
   valid for the process's life: `calloc` on the net side), next to the existing
   `w-load`/`w-store` (64-bit) and add `w-load8`/`w-store8` if byte access is needed. Write
   under "Needs from track A" the exact machine code the ELF driver needs for each new
   command (index, fields, what it leaves as the response); A adds it to `lib/elf.cub`.
2. **`lib/os/linux.cub`**: the signature `Linux` of typed commands and the handler
   `linux-on-w : Handler Linux W` that performs each by `w-sys` (x86-64 numbers) and the memory
   commands. Commands: `clock_gettime`, `nanosleep`, `read`, `write`, `open`, `close`, `mmap`,
   `munmap`, `mprotect`, `socket`, `bind`, `listen`, `accept4`, `connect`, `recvfrom`,
   `sendto`, `setsockopt`, `epoll_create1`, `epoll_ctl`, `epoll_wait`, `futex` (wait, wake),
   `getpid`, `exit_group`. Each response is `Sum Errno R` (decode the negative range), with
   `R` precise: a `read` returns `(n : Word) × (n ≤ len)`-style facts only where the kernel
   guarantees them (state them as fields of the response type; this is the contract).
   Structures passed by address (`sockaddr_in`, `timespec`, `epoll_event`) are written into
   `w-alloc`ed memory by byte-layer functions with their layouts stated once.
3. **`lib/os/contract.cub`**: the contract as a world model: `linux-model : World Linux KState`
   (`lib/proc.cub`'s `World`), a deterministic model of the subset (descriptor table, pipes
   and UDP sockets as queues, a clock) against which a process is run by `runp` on the net.
   It is the statement of what is assumed of the kernel (PLAN §7: trusted until the kernel is
   lifted); properties of processes are proved against it.
4. **Platform parameters** (`lib/os/platform.cub`): a record `Platform` (page size, cache line,
   cores, clock source, endianness, syscall numbers table) and the x86-64 Linux value; the
   handler takes it as an argument, so a program's type names what it relies on.

## Acceptance

- `make library` passes with `lib/os/*.cub` appended to `LIB` (ask A to append; until then load
  them after the library by hand).
- `experiments/os/udp-echo.cub`: a process over `Linux` that binds a UDP socket on
  127.0.0.1:port, receives one datagram, sends it back, exits. `#run (to-step (handle
  linux-on-w …))` performs it on the net (send it a datagram with python from another
  shell); record the steps. The same process run by `runp` against `linux-model` with a
  queued datagram ends `done` with the datagram in the model's outgoing queue (a `#show`).
- After A adds the ELF side: `#build` of the same process; the binary echoes a datagram.

## Needs from track A

1. **ELF driver cases for the four new commands** (`lib/elf.cub`, `i-driver-loop`). The exact
   bytes are being produced as an assembly source of the whole loop in `experiments/os/`
   (next step below); until that lands, the statement of each case, in the loop's own
   conventions (`rsi` the command's cell `[index][fields]`, every `Word` field a raw 64-bit
   value, the response left in `rax` before the jump to `resp`, `r15`/`r12` the arena's next
   and end, the overflow exit the first byte after the loop):
   - index 11, `w-sys nr a1 a2 a3 a4 a5 a6` (fields at `[rsi+8]`…`[rsi+56]`): `rax=nr, rdi=a1,
     rsi=a2, rdx=a3, r10=a4, r8=a5, r9=a6; syscall`; response `rax` unchanged (no sign
     extension, no clamping to 0).
   - index 12, `w-alloc n` (`[rsi+8]`): `rax = r15; r15 += (n+7) & ~7`; overflow exit when the
     sum carries or passes `r12`; response the old `r15` (arena memory is fresh, zero, and
     never reused).
   - index 13, `w-load8 a`: response `movzx eax, byte [a]`.
   - index 14, `w-store8 a v`: `mov [a], v's low byte`; response unit (`xor eax, eax`).
   - the dispatch chain today sends every index above 8 to `w-cas` (index 9): it needs
     `cmp rcx, 9 / je cas` and the four new comparisons.
2. **`.gitignore` hides `experiments/build/`** (see Found): `feed.cub` is not in the repository.
3. Append `lib/os/platform.cub lib/os/linux.cub lib/os/contract.cub` to `LIB` in the `Makefile`
   when they land (not yet written).

## Done

- **Item 1, the raw command** (this commit; the one commit that touches `lib/world.cub` and
  `src/drivers.c`). `WCmd` gains, at the end (indices 11–14, the first eleven unmoved):
  `w-sys nr a1..a6 → Word`, `w-alloc n → Word`, `w-load8 a → Word`, `w-store8 a v → Unit`, with
  the processes `sys-call`, `alloc-bytes`, `load8-at`, `store8-at`. `src/drivers.c`: `w-sys` is
  `syscall(2)` with libc's `-1`/`errno` put back as the two's complement of the errno (what
  the instruction leaves in `rax`); `w-alloc` is `calloc` (never freed); byte load and store.
  Measured: `make library` 834 passed, 0 failed, 21,734,081 steps (22 s);
  `experiments/os/getpid.cub` (`#run` of `sys-call #39 …`) prints `run: done #4821   [20 steps]`
  (the process id); `experiments/build/feed.cub` (from track A's checkout, see Found) still
  builds: `ok: build feed-step [build/feed, 8064 bytes, the checker's conversion, 1216718
  steps]` and the binary answers 5, 3, 13 on three messages (bid 5, ask 2, bid 10).

## Found

- `experiments/build/feed.cub` is named by PLAN, the tracks table and commit `856ab5a8`, and is
  not in the repository: the root `.gitignore` has the pattern `build/`, which matches
  `scoped-machine/experiments/build/`. The file exists only in `/home/claude/avikj/math`
  (untracked). Fix (track A / owner): `git add -f scoped-machine/experiments/build/feed.cub`
  or add `!scoped-machine/experiments/build/` after the `build/` line.
- The ELF driver loop's dispatch sends every command index above 8 to the `w-cas` code and
  every field-less command other than index 0 to `mfence`: a new command reaching an old
  binary's loop is performed as a compare-and-swap on its first field. Closed by the dispatch
  in Needs 1.

## Next step

Write `experiments/os/driver-loop.s`: the whole ELF driver loop as assembly (the existing 523
bytes reproduced exactly when the new cases are left out, checked with `as` + `objcopy` +
`cmp` against the bytes in `lib/elf.cub`), with the four new cases; put the resulting bytes
and the replacement `i-driver-loop` line under Needs 1. Then item 4 (`lib/os/platform.cub`),
item 2 (`lib/os/linux.cub`), `experiments/os/udp-echo.cub`, item 3 (`lib/os/contract.cub`).
