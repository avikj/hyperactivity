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

5. **`lib/x86.cub`, the erasure's fixed point (item 7, cause 2; measurements under Found).**
   Exact change, in `x-erase-roots`:
   ```
   x-rev l acc = case l of lnil → acc; lcons x r → x-rev r (lcons x acc)
   x-param-count ars = case ars of lnil → #0; lcons e r → %wadd (snd e) (x-param-count r)
   x-erase-roots roots defs0 =
     let ars = x-arities defs0
         R = x-rel-fix (%wsucc (x-param-count ars)) (x-root-params roots ars lnil) ars (x-rev defs0 lnil)
         defs1 = x-er-defs R ars defs0
     in x-keep (x-reach defs1 roots lnil) defs1
   ```
   (a) the definitions reversed for the fixed point only (callees before callers: 2 passes
   instead of 6–41 on every program measured, same set); (b) the fuel the number of parameters
   plus one, which no run can exhaust (every pass but the last adds a parameter), instead of
   `#64`, which the tour reaches 41 of — and when exhausted erases needed arguments silently.
   If a bound is kept for another reason, exhausting it must be a `not-fo` verdict, not a
   result. Expected on `os-tour-step`: `#build` from 555 M steps to about 35 M. A further
   factor, not needed for that: the needed set as one bit mask per definition kept with its
   arity (one lookup per call instead of `x-look` + one `x-pmem` scan per argument).
6. **`lib/elf.cub` / `lib/x86.cub`, a verdict for cause 1.** A process written with a `case`
   on a response at a process type compiles to a correct but ~10× larger and slower binary,
   silently. `experiments/os/build-cost.cub`'s `bc-stuck` counts the applications whose head
   is a stuck `ccase`/`celim` (12 lines); `#build` and `#jit` printing that count beside the
   bytes (`…, 3 stuck applications`) makes it visible. The general remedy inside
   `lib/specialize.cub` would be the commuting conversion `(case x of cᵢ ys → tᵢ) a ⟶ case x
   of cᵢ ys → tᵢ a`; it is not a definitional equality, so `build-plan`'s `conv` would refuse
   the result unless the specializer also returned its path (by case on `x`, `refl` in each
   branch, under the binders by `funExt`): the rule mechanism `specialize` already has
   (`rules`, the path it returns). I did not write it: it is in files this track may not edit.
7. **`lib/proc.cub`**: `lx-sum`/`lx-if` (in `lib/os/linux.cub`) are not about Linux: they are
   the case and the conditional on processes of any signature. They belong beside `bind` as
   `proc-sum`/`proc-if`, and `lib/world.cub` wants the analogue of `linux-step` for any
   handler (`handle-step : Handler S W → Proc {lzero} S A → Step W A`,
   `handle-step hd p = p (Step W A) sdone (λ c k → hd c (Step W A) k scmd)`), which is
   `to-step (handle hd p)` with no function-typed case left.

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
- **Item 7** (diagnosis; details and tables under Found, the compiler-side changes under
  Needs 5–7). In this track's own files: `lib/os/linux.cub` gains `lx-sum`, `lx-if`,
  `linux-step`; `lx-then` and four handler cases rewritten; `lib/os/replay.cub` gains
  `linux-step-rec`; the experiments' steps are built by `linux-step`. Re-measured after the
  change (all on the kernel again): echo on the net `run: done (inr #12)   [1743 steps]`
  (was 1,871); `build/udp-echo` 15,178 bytes, 3,216,994 steps to build, its calls under
  `strace` unchanged (socket, bind, recvfrom, sendto, close) and python gets `hello, world`
  back; `build/udp-echo-rec` 53,065 bytes, its log replayed: `run: done (pair (done (inr #12))
  #0)   [3405 steps]`; `build/os-tour` 47,205 bytes, 555,372,003 steps, the same 20 calls and
  status 0; tour on the net `[5243 steps]` with the same 12 observations; tcp prints `ping`,
  status 7; the model runs unchanged (`done (inr #12)`, the tour's record). `make library`:
  1135 passed, 0 failed (24,825,019 steps).

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

- **Item 7: where `#build`'s steps go, and why** (complete; for track N as well). Files:
  `experiments/os/build-cost.cub` (the stages of `lib/elf.cub`'s `build-plan` called one at a
  time; `bc-chain n` = n `close`s over `Linux`), `build-cost-{3,5,7,10}.cub`,
  `build-cost-direct.cub`, `build-cost-passes.cub`, `build-cost-stuck.cub`. Every number is the
  steps of one `#show`, cumulative from `unfold` (a `#show` starts from nothing).

  **Table 1 — the stages, `bc-chain n`, with `lib/os/linux.cub` as it was at `38428f57`:**

  | stage (cumulative steps; size) | n = 3 | n = 5 | n = 7 | n = 10 |
  |---|---|---|---|---|
  | `unfold` (code nodes) | 320,118 (4,101) | 361,654 (4,695) | 402,885 (5,289) | 464,079 (6,180) |
  | `specialize` (code nodes) | 770,870 (4,101) | 867,742 (4,695) | 964,015 (5,289) | — |
  | `conv` | 898,571 | 992,495 | 1,085,801 | — |
  | `program-of` (definitions) | 839,861 (1) | — (1) | 1,051,830 (1) | — |
  | `x-forms-all` (nodes; definitions) | 2,153,090 (2,404; 129) | 2,552,960 (2,462; 133) | 3,079,944 (2,520; 137) | 6,571,145 (2,607; 143) |
  | `x-erase` | 31,836,231 | 34,353,194 | 37,070,974 | 41,516,832 |
  | `x-verdict` | 32,970,432 | 35,605,009 | 38,440,109 | — |
  | `encode-proc` (blob bytes) | 33,870,456 (50,439) | 36,433,769 (51,523) | 39,198,235 (52,607) | 43,737,966 (54,233) |

  (`conv` and `program-of` are separate `#show`s over `specialize`, not over each other; "—" not
  run.) Reading: `x-erase` is 85–93% of the steps at every n; and the cost is not in the number
  of commands: three closes are already 129 definitions and 50 KB, each further command adds 2
  definitions, 542 bytes and ~1.4 M steps.

  **Cause 1 (size; in the process's text, fixed here): the handler stayed in the code.**
  `lx-then` was `bind p (λ r → case r of inl e → ret (inl e); inr a → f a)`: a `case` on a
  response whose branches are processes, i.e. functions of the interpretation, applied to the
  interpretation from outside. It is stuck until the response is known, so what it is applied
  to — `handle`'s `λ c k → bind (linux-on-w pf le sc c) k`, all 29 cases — stays in the
  specialized code as a value and every later command is dispatched through it at run time
  (in the code of three closes the errno threshold literal occurs 23 times). Fixed in
  `lib/os/linux.cub` without touching the compiler, in two steps, both measured:
  (i) `lx-sum`/`lx-if`: the case written with the interpretation inside its branches
  (`λ R k h → case s of inl a → f a R k h; inr b → g b R k h`), `lx-then` by it, and the
  handler's own cases on a response (clock_gettime, accept4, recvfrom, epoll_wait) made cases on
  values; (ii) `linux-step : … → Proc {lzero} Linux A → Step W A`, the process interpreted at
  the type of steps directly (the drivers' interpretation is then inside every branch too: a
  step is data, no case in the code returns a function).

  **Table 2 — the same processes after each step (`encode-proc` cumulative steps; blob bytes;
  definitions after lifting; code nodes; applications whose head is a stuck case):**

  | | n = 3 | n = 5 | n = 7 | n = 10 |
  |---|---|---|---|---|
  | before | 33,870,456; 50,439 B; 129; 4,101 | 36,433,769; 51,523 B; 133; 4,695 | 39,198,235; 52,607 B; 137; 5,289 | 43,737,966; 54,233 B; 143; 6,180 |
  | (i) `to-step (linux-run …)` with `lx-sum` | 372,627; 5,753 B; 15; 239; 3 stuck | 648,281; 8,263 B; 21; 353 | 979,307; 10,773 B; 27; 467 | 1,580,576; 14,538 B; 36; 638 |
  | (ii) `linux-step` | 255,216; 4,861 B; 11; 133; 0 stuck | 427,427; 7,187 B; 17; 217; 0 | 635,297; 9,513 B; 23; 301; 0 | 997,532; 13,002 B; 32; 427; 0 |

  After: 3 definitions and 1,163 bytes per `close`, steps growing a little faster than n
  (255 k, 427 k, 635 k, 998 k: `x-erase` again, cause 2). The real programs, rebuilt through
  `linux-step`: `build/udp-echo` 52,133 B / 42,415,757 steps → **15,178 B / 3,216,994 steps**
  (code 4,709 → 581 nodes, 0 stuck); `build/udp-echo-rec` 97,011 B / 122,343,903 →
  53,065 B / 63,683,157 (3,052 nodes; 5 stuck applications left, all loops over a list or a
  count known only at run time: `lx-store-words`); `build/os-tour` 66,049 B / 1,209,198,335 →
  47,205 B / 555,372,003 (code 9,131 → 1,876 nodes, 166 → 114 definitions, 0 stuck).

  **Cause 2 (steps; in `lib/x86.cub`, not fixed here: Needs 5): the erasure's fixed point visits
  the definitions in the order that needs the most passes.** `x-rel-fix` iterates `x-rel-step`
  (one pass over all definitions, the needed set threaded through) until the set stops growing.
  A parameter is needed because of its uses in its own definition's body, and that makes
  arguments at the call sites in *callers* needed: need flows from callee to caller. Lifted
  definitions are listed callers first, so one pass moves need one call-link up, and the
  number of passes is the depth of the chain of continuations. Table 3
  (`build-cost-passes.cub`: `bc-passes` is `x-rel-fix` with the passes counted; "reversed" is
  the same `x-rel-step` over the reversed list):

  | program (library version) | definitions | parameters | passes, order as is | steps | passes, reversed | steps | needed set |
  |---|---|---|---|---|---|---|---|
  | `bc-s3` (before) | 129 | 412 | 6 | 28,471,060 | 2 | 9,681,013 | 367 either way |
  | `udp-echo-step` (before) | 133 | 442 | 6 | 32,511,652 | 2 | 11,096,076 | 392 |
  | `os-tour-step` (before) | 166 | — | 38 | 1,175,304,827 | 2 | 37,892,946 | 750 |
  | `udp-echo-step` (i) | 38 | 136 | 7 | 4,173,741 | 2 | 1,472,752 | 127 |
  | `os-tour-step` (i) | 114 | 655 | 41 | 861,561,178 | 2 | 22,536,442 | 636 |

  (steps include `x-forms-all`: 12.2 M for the tour (i), 4–5 M for the others before.) So the
  tour's 1.2 × 10⁹ steps are 38 passes at ~30 M each; in the reversed order the same set is
  reached in 2 passes: 31× fewer steps for the tour before, 38× after (i). A pass costs
  (code nodes) × (a list lookup of the arity, `x-look`, plus a list scan of the needed set,
  `x-pmem`, per argument of every call), which is why steps per pass grow with the product of
  code size and parameters.

  **Cause 2 has a correctness side.** `x-erase-roots` calls `x-rel-fix #64`: when the fuel runs
  out the set reached so far is returned (`if %weq fuel #0 then R`), and `x-er-args` then
  replaces every argument not in it by `cwlit #0`. A set cut short is a subset of the fixed
  point, so a needed argument is erased and the binary computes with 0 in its place, with no
  verdict. The tour needs 38 passes (41 after (i)) of the 64; a process whose continuation
  chain is about 1.6× the tour's depth passes the bound. Not demonstrated by a failing binary
  (a 64-pass build is ~2 × 10⁹ steps here); read from the code and the measured pass counts.

- **A loop with a literal bound is unrolled by specialization once its body's case can be
  entered.** `lx-read-all` (`lib/os/replay.cub`) is `%welimP … #4096 …` with a case on each
  read's result; written with `lx-sum` the specializer would enter the case and unroll 4096
  levels at `#build`. It is left with the case applied from outside (stuck, so the loop stays a
  loop), with a comment. There is no way in the source to say "this recursion is a loop, keep
  it" short of a bound known only at run time; `sched-holds` exists in `lib/specialize.cub`
  but `build-plan` passes `sched-default` (no holds).

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
