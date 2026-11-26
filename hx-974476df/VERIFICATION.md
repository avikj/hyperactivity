# hx 45-engine toolchain — verification record (store root 974476df)

Source: avikj/hyperactivity commit 92ee19d26, store root
974476df85cd5ed2fe5aefde5432b81d24156c05f170b67ddaf5b5e71763a8af.
Platform of every run below: Linux x86-64 (2 vCPU container), gcc, glibc static.

## Provenance
- `./build/cubical lib/hx.hypercubical e_hx.hypercubical` — checked the 118-module hx closure cold:
  **5394 passed, 0 failed**, 2,408,350,028 interaction steps, emitted `hx.c` (2,828,845 B).
- `cc -std=c11 -O2 -static -o hx-linux-x86_64 hx.c -lm`
- `cc -std=gnu11 -O3 -static -o cubical-linux-x86_64 src/{dnf,big,net,lang}.c -lpthread -lm -ldl`
- Kernel check: Brunerie's number with `cubical-linux-x86_64` = `negsuc (suc zero)` (−2) with 1 worker
  (395,608 steps, 3.3 s) and 4 workers (405,568 steps, 5.5 s).
- Closed discovery loop: `hx endgame` emitted `hx_discovered_6in.hypercubical`; the kernel checked it
  **840 passed, 0 failed** (parity5 26, parity6 33, maj5 23, ge3 16, add3c 16, shacm 27 NAND2).

## Engines run (all exit 0)
hxp, treasury, lux, verify, geodesic, genesis, poly, endgame, beast, kexpr, strike, tower, cosmos,
omni, wetlab, supercomp, meta, inet, psc, ir, matter, solve, web, space, loop, discover oe, synth,
tensor, book, alu, tapeout, llmzip, gpu, node, swarm, agent, tui, hf, audit.
Emitted artifacts (C11, SystemVerilog, GDSII v6.0, Metal, CUDA, WASM, OpenQASM 3.0, Gerber, WAV) are in
`hx-engine-artifacts.tar.gz`.

## Emitted C11 rebuilt with -fsanitize=undefined and run
| artifact | result |
|---|---|
| genesis.c | 8/8 superopts × 200,000 vectors = 1,600,000 checks, errors=0 |
| poly.c | 8/8 × 100,000 = 800,000 checks, errors=0 |
| verify.c | 4 regimes, total_err=0 (65,536/65,536 u4 atlas) |
| endgame_endgame.c | 65,536/65,536 4-var atlas; 6/6 six-input targets exact, 141 NAND2 |
| tower_tower.c | fold exact; 337 → 1 gates in 14.0 µs |
| omni_omni.c | BF16 escape-safe 65,536/65,536 exact; 64-site DMRG χ=8 E0=−45.008351 |
| strike_cbf_actuator_rt.c | 100,000 frames, viol=0, 86.8 ns/frame |
| cosmos_cosmos.c | certified CBF viol=0 over 10K steps |
| ir.c, kexpr.c | exact=1 |
| treasury.c | runs; with no files under /tmp/hy_out/live_world_feeds/ it uses its compiled-in reserve snapshot |
| beast_daemon.c, geodesic_beast.c | unbounded daemons (run until stopped) |
| hxp.c | targets macOS (`<sys/sysctl.h>`, Unified Memory Metal); build on Apple silicon |

## Subcommands with a defect found in this run
- `hx cm c|d`, `hx zip|unzip`: heap-use-after-free under AddressSanitizer in both directions.
  `hcm-maybe-rewind` (lib/hx-cm.hypercubical) issues `w-sys #901` every 64 bytes; the emitted
  `hy_rewind` frees every slab allocated after `hy_mark`, and the pending continuation of that very
  `w-sys` step was allocated after the mark, so `hy_run_step` then applies a freed closure (`hy_app`,
  hx.c line 33). Without ASan, `hx cm` round-trips some inputs (392,326 B exact: 2.3886 bpb vs
  `xz -9e` 2.5736) and segfaults on others (562,119 B and 565,619 B decode). Sound form: rewind only
  at a point whose continuation was allocated before the mark (or copy the live continuation below
  the mark before rewinding).
