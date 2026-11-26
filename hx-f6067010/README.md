`hx` — the 45-engine scientific computing toolchain, emitted from store root `f6067010` (branch `fix/rewind-continuation`, pull request #16): store root 974476df with the rewind continuation fix in `lib/emit-c` and `lib/hx-cm`. The 118-module closure checked cold: **5394 passed, 0 failed** (530,462,526 interaction steps).

- `hx-linux-x86_64` — static Linux x86-64 binary. `hx cm c|d IN OUT` is the lossless context-mixing coder: exact round-trip with zero AddressSanitizer/UBSan reports on every input tested (392 KB to 1.4 MB), 4–8% smaller than `xz -9e` on the same files.
- `cubical-linux-x86_64` — the kernel (unchanged C sources).
- `hx.c` — the ISO C11 source; `cc -std=c11 -O2 -o hx hx.c -lm`.
- `VERIFICATION.md`, `SHA256SUMS`, `LICENSE`.
