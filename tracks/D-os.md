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

1. ~~ELF driver cases for `w-sys` (11), `w-alloc` (12), `w-load8` (13), `w-store8` (14)~~ —
   **done by A** in `6533fae9` (`tools/asm/driver.s`, `python3 tools/asm/gen.py --patch`;
   `experiments/native/sys.cub`). Any further command is requested here; no assembly source of
   the loop is kept in `experiments/os/`.
2. ~~`.gitignore` hid `experiments/build/`~~ — **done by A**: the demos are `experiments/native/`.
3. Append `lib/os/platform.cub lib/os/linux.cub lib/os/contract.cub` (this order) to `LIB` in
   the `Makefile`. **Not yet**: only `platform.cub` exists; this line is rewritten to "ready"
   when all three load.

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
- **Item 4, platform parameters**: `lib/os/platform.cub`. `Sys` (the 23 calls), `Platform`
  (page, cache line, cores, clock id, byte order, `pf-sysnr : Sys → Word`, the size of
  `struct epoll_event` and its data offset), `LittleEndian pf` (the fact word layouts rely on),
  `x86-64-linux cores` with `x86-64-linux-le`. Loaded after the library: 857 passed, 0 failed
  (23 new definitions; `x86-64-sysnr` 2,942 steps).
- **Item 2, the typed world**: `lib/os/linux.cub`. `Linux : Sig` over `LinuxCmd` (29 commands:
  the 23 calls with futex as wait and wake, and `lx-alloc`, `lx-peek8/poke8`, `lx-peek64/poke64`),
  `lx-resp` (each `Sum Errno R`; `Upto len` = a count with the fact `n ≤ len` for read, write,
  recvfrom, sendto, epoll_wait, futex-wake; `getpid : Word`), `lx-decode` (rax ≥ 2^64 − 4095 is
  errno 2^64 − rax), the layouts (`sockaddr-word`/`word-sockaddr`, timespec, epoll_event at the
  platform's size and offset), `linux-on-w : (pf : Platform) → LittleEndian pf → Word → Handler
  Linux W` (the `Word` a 4096-byte scratch area for the structures), `linux-run` (takes the
  scratch once, then `handle`), `lx` and `lx-then` for writing processes. A kernel answer
  outside a stated fact is returned as the error `os-breach` (0). Measured (NOCACHE, after the
  library and platform.cub): 896 passed, 0 failed; `linux-on-w` checks in 144,867 steps, `lx-events` 11,073.
- **Acceptance, the echo on the net**: `experiments/os/udp-echo.cub` (`udp-echo`, a process over
  `Linux` at every level; `udp-echo-step` at `x86-64-linux #2`) and `udp-echo-run.cub` (`#run`).
  With `python3 -c "import socket;s=socket.socket(2,2);s.sendto(b'hello, world',('127.0.0.1',47474));print(s.recvfrom(99))"`
  from another shell: python prints `(b'hello, world', ('127.0.0.1', 47474))`, the machine
  `run: done (inr #12)   [1871 steps]`.
- **Acceptance, the echo as a binary** (A's ELF driver, `6533fae9`): `udp-echo-build.cub` prints
  `ok: build udp-echo-step   [build/udp-echo, 52133 bytes, the checker's conversion, 42415757
  steps]` (12.7 s with the library cached). `build/udp-echo` under `strace`, sent
  `hello, world` by the python line: its system calls are exactly `mmap` (the arena),
  `socket(AF_INET, SOCK_DGRAM, 0) = 3`, `bind(3, 127.0.0.1:47474, 16) = 0`,
  `recvfrom(3, "hello, world", 2048, 0, {127.0.0.1:58299}, [16]) = 12`,
  `sendto(3, "hello, world", 12, 0, {127.0.0.1:58299}, 16) = 12`, `close(3) = 0`,
  `exit_group(0)`; python prints `(b'hello, world', ('127.0.0.1', 47474))`.

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

Item 3, `lib/os/contract.cub` (loaded after `lib/os/linux.cub`): `KState` (byte memory, next
address, descriptor table, pipes, UDP sockets with their queues, outgoing datagrams, clock,
pid) and `linux-model : World Linux KState`; then `experiments/os/udp-echo-model.cub` with
`runp` of `udp-echo {lzero}` against the model with one queued datagram and a `#show` of the
outcome and the outgoing queue. Then tell A (Needs 3) that the three files load.
