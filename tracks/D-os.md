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

5. **The rest of Phase 3's calls** (added 2026-11-10 by the coordinator). Typed commands at
   the end of `LinuxCmd` with contracts, the handler's case, the model's answer or explicit
   refusal, the replay encoding: `sendmsg`/`recvmsg` (the `iovec` and `msghdr` layouts stated
   once), `io_uring` (`io_uring_setup`, the mmap'd rings, submit and complete for read, write,
   recv, send through `io_uring_enter`), `eventfd2`, `timerfd_create`/`timerfd_settime`, and
   thread start by `clone` with futex wake if expressible through `w-sys`.
6. **The kernel-bypass shape** (`lib/os/packet.cub`): a signature `Pkt` for a polled packet
   interface (poll the rx ring → a packet or nothing, release it, submit on the tx ring) whose
   handler is memory commands over mmap'd rings, with AF_PACKET + PACKET_MMAP (TPACKET_V2) on
   `lo` as the instance that runs here; ef_vi and DPDK are other instances of the same
   signature (hardware absent: stated under Limits).
7. **Diagnosis of `#build`'s cost** (no edits to `lib/x86.cub`, `lib/elf.cub`,
   `lib/specialize.cub`): the stages of `build-plan` measured separately at 3, 5, 7, 10
   commands, the table and the cause under Found, the exact fix under Needs.

## Acceptance

- `make library` passes with `lib/os/*.cub` appended to `LIB` (ask A to append; until then load
  them after the library by hand).
- `experiments/os/udp-echo.cub`: a process over `Linux` that binds a UDP socket on
  127.0.0.1:port, receives one datagram, sends it back, exits. `#run (to-step (handle
  linux-on-w …))` performs it on the net (send it a datagram with python from another
  shell); record the steps. The same process run by `runp` against `linux-model` with a
  queued datagram ends `done` with the datagram in the model's outgoing queue (a `#show`).
- After A adds the ELF side: `#build` of the same process; the binary echoes a datagram.

- Item 5: `experiments/os/msg.cub`, `uring.cub`, `evtimer.cub`, `thread.cub`, each with a
  `-run.cub` whose `#run` on the kernel is recorded here with its `strace` lines; the model's
  answer for each (a `#show` of `runp` against `linux-model`: `done …` or `stopped` where the
  model refuses, as the header of `lib/os/contract.cub` lists).
- Item 6: `experiments/os/packet.cub` + `packet-run.cub`: a frame submitted on the tx ring of
  an AF_PACKET socket bound to `lo` is polled from the rx ring by the same process; the
  result recorded here.
- Item 7: the table (stage × number of commands → steps, sizes) under Found.

## Limits

(filled by item 6)

## Needs from track A

1. ~~ELF driver cases for `w-sys` (11), `w-alloc` (12), `w-load8` (13), `w-store8` (14)~~ —
   **done by A** in `6533fae9` (`tools/asm/driver.s`, `python3 tools/asm/gen.py --patch`;
   `experiments/native/sys.cub`). Any further command is requested here; no assembly source of
   the loop is kept in `experiments/os/`.
2. ~~`.gitignore` hid `experiments/build/`~~ — **done by A**: the demos are `experiments/native/`.
3. ~~`lib/os` in `LIB`~~ — **done by A** in `5835b6db` (`make library` 993 passed).
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

- **Item 7, where `#build`'s steps go (interim; the table is being completed).**
  `experiments/os/build-cost.cub` + `build-cost-{3,5,7,10}.cub`: the stages of `build-plan`
  on `bc-chain n` (n closes over `Linux` through `linux-run`), cumulative steps per `#show`:

  | stage (cumulative) | n = 3 | n = 5 | n = 7 |
  |---|---|---|---|
  | `unfold` (code nodes) | 320,118 (4,101) | 361,654 (4,695) | 402,885 (5,289) |
  | `specialize` (code nodes) | 770,870 (4,101) | 867,742 (4,695) | 964,015 (5,289) |
  | `conv` | 898,571 | 992,495 | 1,085,801 |
  | `program-of` (definitions) | 839,861 (1) | — (1) | 1,051,830 (1) |
  | `x-forms-all` (nodes, definitions) | 2,153,090 (2,404; 129) | 2,552,960 (2,462; 133) | 3,079,944 (2,520; 137) |
  | `x-erase` | 31,836,231 | 34,353,194 | 37,070,974 |
  | `x-verdict` | 32,970,432 | 35,605,009 | 38,440,109 |
  | `encode-proc` (bytes) | 33,870,456 (50,439) | 36,433,769 (51,523) | 39,198,235 (52,607) |

  Two facts so far. (a) `x-erase` is ~93% of the steps at every n. (b) The cost is not in the
  number of commands: a process of 3 closes is already 129 definitions and 50 KB, and each
  further command adds 2 definitions, 542 bytes and 1.4 M steps. The fixed part is the whole
  handler: the unfolded code's first command is reduced (`w-sys #3 #3 …`), but its
  continuation is `(case r of inl e → …; inr a → …) R k h` — a `case` on the response, at a
  function type (`Proc`), applied to the interpretation — which is stuck, so `handle`'s
  `λ c k → bind (linux-on-w … c) k` stays in the code as a value with all 29 cases (the errno
  threshold literal occurs 23 times in the code of three closes) and commands are dispatched
  through it at run time. Cause and fix are being confirmed (passes of the fixed point, the
  tour's numbers, and the same processes with the case commuted by hand in `lib/os/linux.cub`).

## Next step

Item 7 in progress. Running: `build-cost-passes.cub` (the erasure's fixed-point passes for
`bc-s3`, `udp-echo-step`, `os-tour-step`, in both definition orders) and `build-cost-10.cub`;
put their numbers in the table under Found. Then in `lib/os/linux.cub`: `lx-then` and every
`case`/`if` whose result is a process written with the interpretation's arguments inside the
branches (`λ R k h → case r of inl e → k (inl e); inr a → f a R k h`), the handler's cases on a
response made cases on values; re-run the table; write the compiler-side fix (the commuting
conversion in `normalize-held`) under Needs. Then item 5 (eventfd/timerfd, sendmsg/recvmsg,
io_uring, clone), then item 6 (`lib/os/packet.cub`).
