# hx — store root 06d0f326766546d6cd1c62b3d3c21d5ea276d4ee2543ce3224a40b7d8178e765

Root 06d0f326 = 974476df with three module labels changed: lib/emit-c (rewind keeps its continuation),
lib/hx-cm (iteration built after each rewind), lib/hxz (one 4096-byte block per rewind). Branch
fix/rewind-continuation, pull request #16.

- `cubical lib/hx.hypercubical e.hypercubical`: 5405 passed, 0 failed; 536,517,844 interaction steps.
- hx cm (ASan+UBSan build): exact round-trip, 0 reports — 392,326 B → 2.3886 bits/byte; 562,119 B → 1.6653;
  565,619 B → 2.3280; 1,409,288 B → 3.5874 (xz -9e: 2.5736, 1.7368, 2.4701, 3.6543).
- hx zip / unzip: exact round-trip at 4,096 / 65,536 / 392,326 / 562,119 / 1,409,288 B; peak RSS
  1.49 / 1.49 / 1.57 / 1.47 / 1.96 GB; 3.8125 / 3.6456 / 3.6369 / 3.0560 / 4.5002 bits/byte
  (xz -9e: 4.1094 / 2.8999 / 2.5736 / 1.7368 / 3.6543). 0 ASan/UBSan reports through 562 KB.
  About 1.2–1.6 s per 4096-byte block. Each block starts from a fresh context, so the format
  compresses worse than xz -9e beyond one block.
- hx synth TT searches at most 6 NAND2 gates; TT=0 is reported as realized by an empty netlist
  (`hs-exec-go` returns its initial 0 for zero gates).
- Static binary rebuilt from this hx.c: zip/unzip 64 KB exact; cm on 562 KB exact.
