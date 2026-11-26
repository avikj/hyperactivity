`hx` — the 45-engine scientific computing toolchain, emitted from store root `06d0f326` (branch `fix/rewind-continuation`, pull request #16). The 118-module closure checks **5405 passed, 0 failed** (536,517,844 interaction steps).

- `hx-linux-x86_64` — static Linux x86-64 binary.
  - `hx cm c|d IN OUT`: lossless context-mixing coder; exact round-trip, 0 sanitizer reports, below `xz -9e` on every input tested.
  - `hx zip|unzip IN OUT`: rANS block coder; memory now independent of input length (1.5–2 GB peak at every size from 4 KB to 1.4 MB).
  - `hx synth TT`: provably minimal NAND2 netlist (≤ 6 gates) of a 3-input truth table.
  - Plus the verification, silicon, physics, chemistry, GPU and transport engines; run `hx` for the list.
- `cubical-linux-x86_64` — the kernel.
- `hx.c` — ISO C11 source; `cc -std=c11 -O2 -o hx hx.c -lm`.
- `VERIFICATION.md`, `SHA256SUMS`, `LICENSE`.
