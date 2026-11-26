`hx` — the 45-engine scientific computing toolchain, emitted by the hyperactivity kernel from store root `974476df` (commit 92ee19d26) after checking its 118-module closure: **5394 passed, 0 failed**.

### Downloads (Linux x86-64, static, no dependencies)
- `hx-linux-x86_64` — logic synthesis and superoptimization (`synth`, `endgame`, `beast`, `discover`, `poly`, `tower`), formal verification (`verify`, `audit`, `loop`), silicon (`inet`, `meta`, `tapeout`, `alu`, `psc`, `lux`, `cosmos`), exact physics and chemistry (`matter`, `wetlab`, `omni`, `genesis`), GPU/tensor lowering (`gpu`, `tensor`, `llmzip`, `ir`, `kexpr`), transport (`hxp`, `node`, `sovereign`), and coordination (`swarm`, `agent`, `tui`). Run `hx` for the full list.
- `cubical-linux-x86_64` — the kernel itself. From a checkout of the repository: `cubical-linux-x86_64 lib/core.hypercubical experiments/brunerie/typed.hypercubical experiments/brunerie/run-typed.hypercubical` computes Brunerie's number, −2.
- `hx.c` — the single ISO C11 source the kernel emitted; build anywhere with `cc -std=c11 -O2 -o hx hx.c -lm`.
- `hx-engine-artifacts.tar.gz` — every artifact the engines emitted in this run: C11, SystemVerilog RTL, GDSII v6.0, Metal, CUDA, WebAssembly, OpenQASM 3.0, Gerber, WAV.
- `VERIFICATION.md` — every engine run, the rebuilt artifacts' self-checks, and one defect found (`hx cm`/`hx zip`).
- `SHA256SUMS`
