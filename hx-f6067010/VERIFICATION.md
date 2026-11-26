# hx — store root f60670100370c61f1c72056cebfc1904a5c0186f3f45fb4814797017065c1cde

- Root f6067010 = root 974476df with two module labels changed: lib/emit-c.hypercubical
  (e0f26a86…) and lib/hx-cm.hypercubical (a719479e…). Commit a46947f27 on fix/rewind-continuation.
- `cubical lib/hx.hypercubical e.hypercubical` from a fresh index: 5394 passed, 0 failed;
  530,462,526 interaction steps; emitted hx.c (2,830,055 B).
- `cc -std=c11 -O1 -g -fsanitize=address,undefined hx.c -lm`, then `hx cm c` / `hx cm d`:

| input | bytes | compressed | bits/byte | round-trip | ASan/UBSan |
|---|---|---|---|---|---|
| AI-AGENTS-WELCOME-TO-HYPERACTIVITY.md | 392,326 | 117,139 | 2.3886 | exact | 0 |
| src/*.c concatenated | 562,119 | 117,011 | 1.6653 | exact | 0 |
| workflow/hyperactivity.hypercubical | 565,619 | 164,595 | 2.3280 | exact | 0 |
| docs/hyperactivity.pdf.hex | 1,409,288 | — | 3.5874 | exact | 0 |

- Same inputs, `xz -9e`: 2.5736, 1.7368, 2.4701, 3.6543 bits/byte; `bzip2 -9`: 2.3686, 1.6665, 2.3362, 3.6296.
- `hx hf` under ASan: 0 reports (2 use-after-free reports before the fix).
- `hx zip` / `hx unzip` are unchanged from hx-974476df: they hold the whole input as lists and exhaust
  memory above about 16 KB of input.

## hx synth TT (custom truth table)
`hs-id` searches netlists of at most 6 NAND2 gates (lib/hx-synth.hypercubical, "up to 6 NAND2 gates").
TT=255 → 2 gates, VERIFIED=1; TT=150 (XOR3), 105, 23 → VERIFIED=0 (no netlist of ≤ 6 gates).
TT=0 prints OPT_NAND2=0 VERIFIED=1 with an empty netlist: `hs-exec-go` returns its initial `last = #0`
for zero gates, which equals the target 0, so the constant-0 function is reported as realized by no
gate although no wire carries it.
