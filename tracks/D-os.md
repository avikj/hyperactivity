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
3. **Ready (track A, please do this):** append `lib/os/platform.cub lib/os/linux.cub
   lib/os/contract.cub lib/os/replay.cub` (this order, after `lib/elf.cub`) to `LIB` in the `Makefile`. All four
   load after the library with 0 failures (measurements under Done).
4. Checker (not blocking; worked around in `lib/os/contract.cub`): see Found, "a matched
   variable in a branch without type information".

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
- **Item 3, the contract**: `lib/os/contract.cub`. `KState` (byte memory with the mapped
  regions, the descriptor table with lowest-free allocation, pipe ends, UDP sockets, epoll
  interest lists, `k-net` the datagrams that will arrive, `k-sent` those that left the host, a
  clock, the next ephemeral port, the page size, the pid, the exit status), `k-boot`, and
  `linux-model : World Linux KState`. Answered and refused commands are listed in the file's
  header (refused: open, connect, and every wait nothing in the state ends). Loaded after
  linux.cub: `linux-model` checks in 319,269 steps, `k-recv` 15,390; the library with the three
  `lib/os` files and the echo files (NOCACHE): 967 passed, 0 failed, 23,188,122 steps.
- **Acceptance, the echo against the model**: `experiments/os/udp-echo-model.cub`: `runp` of the
  same `udp-echo` term in `linux-model` from `k-boot` with `hello, world` from 10.0.0.7:5555
  arriving. `#show fst udp-echo-model-run = done (inr #12)` (2,016 steps); `#show k-sent … =
  cons ((#47474 , #2130706433) , (#5555 , #167772167) , cons #104 (cons #101 … (cons #100 nil)))
  nil` (1,651 steps); `k-net` is `nil`; the descriptors left are 0, 1, 2. Stated and checked:
  `udp-echo-echoes` (the outgoing queue is exactly the datagram turned round, 9,078 steps) and
  `udp-echo-done : IsDone …`. (The statement `k-sent … ≡ nil` in its place is rejected:
  `FAIL: checks udp-echo-wrong`.)
- **The calls the echo does not make, on the kernel, in a binary, and in the model**:
  `experiments/os/tour.cub` (`os-tour`: clock_gettime, mmap, poke64/peek8/peek64, socket,
  setsockopt, bind, epoll_create1, epoll_ctl, epoll_wait ×2, sendto to itself, recvfrom,
  futex wake and wait, nanosleep, getpid, munmap, close ×2; its answer the record `Tour` of what
  was seen). `tour-run.cub` on the net: `run: done (inr (#0, #1, [(#1, #1003)], #8, #8,
  (#47475, #2130706433), #1234605616436508552, #119, #0, inl #11, true, true))   [5512 steps]`
  (no event ready, then one `EPOLLIN` with the data given, 8 sent, 8 received from itself, the
  word and the byte read back, 0 woken, `EAGAIN`, the clock at least a millisecond later, a
  positive pid). `tour-model.cub`, `runp` in `linux-model`: `done (inr (#0 , #1 , cons (#1 ,
  #1003) nil , #8 , #8 , (#47475 , #2130706433) , #1234605616436508552 , #119 , #0 , inl #11 ,
  true , true))` (10,146 steps), no region left mapped, descriptors 0–2 left, the clock at
  1,001,000,002 ns (two readings and the sleep). `tour-build.cub`: `ok: build os-tour-step
  [build/os-tour, 66049 bytes, the checker's conversion, 1209198335 steps]`; under `strace` the
  binary makes exactly those 20 calls with those results (`epoll_ctl(4, EPOLL_CTL_ADD, 3,
  {events=EPOLLIN, data={u64=1003}}) = 0`, `epoll_wait(4, [{events=EPOLLIN, data=1003}], 8,
  1000) = 1`, `futex(…, FUTEX_WAIT, 5, NULL) = -1 EAGAIN`, …) and exits 0.
- **The remaining calls on the kernel**: `experiments/os/tcp.cub` + `tcp-run.cub` (`os-tcp`:
  open of `/dev/null` by a poked name, a TCP pair over loopback by listen / connect / accept4,
  write, read, write to descriptor 1, mprotect ×2, poke8, exit_group). On the net it prints
  `ping` and the machine leaves with status 7; under `strace`: `open("/dev/null", O_WRONLY) =
  3`, `bind(3, 127.0.0.1:47476)`, `connect(4, …) = 0`, `write(4, "ping\n", 5) = 5`, `read(5,
  "ping\n", 32) = 5`, `write(1, "ping\n", 5) = 5`, `mprotect(…, 4096, PROT_READ) = 0`,
  `exit_group(7)`. With this every one of the 23 calls and 5 memory commands of `Linux` has
  been performed on the kernel through `linux-on-w`. Against `linux-model` the same term ends
  `stopped` (at `open`, which the model refuses: files and TCP are outside its subset).
- **Replay (PLAN Phase 3's acceptance)**: `lib/os/replay.cub`. `lx-enc`/`lx-dec` (each response
  of `Linux` as a list of words and back, the facts decided again on reading; 31,863 and 41,789
  steps to check), `lx-recording`, `lx-replaying : World Linux LxLog`, `linux-on-w-rec` /
  `linux-run-rec` (the kernel's responses also written to a descriptor), `lx-read-log` /
  `lx-replay-run` (a log read from a descriptor, the process run by `runp` in `lx-replaying`).
  The library with the four `lib/os` files (NOCACHE): 993 passed, 0 failed, 23,474,512 steps.
  `experiments/os/udp-echo-replay.cub`: in the model, the recorded log is `[[1,3],[1],
  [4294967296],[1,12,5555,167772167],[1,12],[1]]`, the replay from it `(done (inr #12) , nil)`,
  and `udp-echo-replay-same : fst udp-echo-replayed ≡ fst udp-echo-model-run` checks (3,095
  steps). On the kernel (`udp-echo-rec-run.cub` with `3>build/udp-echo.log`, the python
  datagram): `run: done (inr #12)   [4283 steps]`, the log 15 words (`2 1 4 | 1 1 | 1 <addr> |
  4 1 12 46303 2130706433 | 2 1 12 | 1 1`); then `udp-echo-replay-run.cub` with
  `<build/udp-echo.log`: `run: done (pair (done (inr #12)) #0)   [3172 steps]` — the same
  outcome from the responses alone, none left over.
- **Replay, compiled**: `udp-echo-rec-build.cub`: `ok: build udp-echo-rec-step
  [build/udp-echo-rec, 97011 bytes, the checker's conversion, 122343903 steps]` (16 s).
  `./build/udp-echo-rec 3>build/udp-echo.log` with the python datagram: python prints
  `(b'hello, world', ('127.0.0.1', 47474))`, status 0, the log is the 15 words `2 1 4 | 1 1 |
  1 140443966550016 | 4 1 12 40094 2130706433 | 2 1 12 | 1 1`; `udp-echo-replay-run.cub
  <build/udp-echo.log` prints `run: done (pair (done (inr #12)) #0)   [3172 steps]`: the
  compiled process's run on the kernel, reproduced on the net from its recorded responses.
- **State at the last commit of this session** (head includes tracks C, G, H, I): `make library`
  845 passed, 0 failed (21,745,292 steps; `LIB` does not yet name `lib/os`). The library, then
  the four `lib/os` files, then `experiments/os/{udp-echo,udp-echo-model,udp-echo-replay,tour,
  tour-model,tcp}.cub` (NOCACHE): 1028 passed, 0 failed, 23,821,936 steps. Loaded after
  `lib/bits.cub lib/obs.cub lib/net.cub lib/fleet.cub` as well: 1179 passed, 0 failed (no name
  of `lib/os` clashes with the other tracks' files).

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

- **A matched variable in a branch without type information.** `case b of inl x → …; inr u → f b`
  with `b : Sum SockAddr Unit`: in the `inr` branch the use of `b` is checked as `inr u` and
  fails with `no inferable type here: annotate it with (the T t)` (`at: cfree (inr u)`); the
  same for `case evs of cons x xs → …; nil → (k , evs)` (`at: nil`), and for a `let` whose
  value is a record or a call with an implicit type argument, used in a branch. The variable has
  a declared type; the refinement loses it. The failing forms are the ones quoted here (they
  were never committed); `k-autobind`, `k-wait` and `k-deliver` in `lib/os/contract.cub` are
  the forms that check: the value passed as an argument, the split decided by a `Bool`.
- **Structural recursion wants the matched argument first when another argument varies.**
  `k-take n (cons x xs) = … k-take (%wsub n #1) xs` is refused (`a recursive call is not on a
  constructor field …`) although `xs` is a field of the match; `k-take-go (cons x xs) n = …
  k-take-go xs (%wsub n #1)` is accepted. With the other argument unchanged (`k-find fd (cons e
  r) = … k-find fd r`) either order is accepted.
- **`#build` cost grows much faster than the process** (track A's; measured here, cause not
  investigated). `udp-echo` (7 commands of `Linux`): 52,133 bytes, 42,415,757 steps, 12.7 s.
  `os-tour` (22 commands): 66,049 bytes, 1,209,198,335 steps, 3 min 39 s. `build/feed` (track
  A's, straight over `W`): 8,064 bytes, 1.2 M steps. Both binaries are correct (their system
  calls under `strace` are listed under Done). Reproduce:
  `./build/cubical <library> lib/os/platform.cub lib/os/linux.cub lib/os/contract.cub experiments/os/tour.cub experiments/os/tour-build.cub`.

- **A clause definition at a type that only unfolds to a function type.** `lx-replaying : World
  Linux LxLog` written by clauses `lx-replaying c nil = …; lx-replaying c (cons x rest) = …` is
  refused with `error: a dimension is matched only by a variable in lx-replaying` (a fatal
  error, not a FAIL: the load stops). Written `lx-replaying c log = case log of …` it checks.
- **A nested `case` is not one of "the definition's own matches".** `f (cons e r) = case r of
  nil → …; cons d r2 → … f r2` is refused as non-structural; the nested pattern `f (cons e
  (cons d r2)) = … f r2` is accepted.

## Next step

Everything under "What is built" and every acceptance line is met, and PLAN Phase 3's replay
acceptance with it. What is left is not this track's to do:
1. Track A appends `lib/os/platform.cub lib/os/linux.cub lib/os/contract.cub lib/os/replay.cub`
   to `LIB` (Needs 3). Then run `make library` and replace the 845 above with the new count
   (expected: 845 + 148 from the four files).
2. Not built here, and in PLAN Phase 3: kernel-bypass interfaces (ef_vi, DPDK) as worlds, and
   `sendmsg`/`recvmsg`/`io_uring` in `Linux` (this track's command list did not have them). To
   add a call: a constructor at the end of `LinuxCmd`, its line in `lx-resp`, `linux-on-w`,
   `linux-model`, `lx-enc`, `lx-dec`, and its number in `Sys` / `x86-64-sysnr`
   (`lib/os/platform.cub`); no change to `W`, the drivers or the ELF loop is needed (`w-sys`
   carries any call of up to six arguments).
3. The model's subset (see `lib/os/contract.cub`'s header) refuses files, TCP, and waits that
   nothing in the state ends; a process using them ends `stopped` against it (`os-tcp` does).
   Extending it is the same file, `linux-model`'s cases.
