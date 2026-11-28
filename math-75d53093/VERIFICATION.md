# Verification — math-75d53093

Machine: 2 cores, 8 GB. Fresh depth-1 clone of main at c2429aa38 (store/ROOT 75d53093adbcc7cecb71202076dbd47a2ca5d8a08550230a1af65542ce3fde49); `make`; `./build/cubical --index`; each module with a 900 s limit. A module whose declarations an earlier module in the same run already checked reports its passes with 0 interaction steps. src/ and the Makefile are byte-identical to e211d839c, so the bundled static cubical-linux-x86_64 is the same build as science-974476df's.

| module | wall | result |
|---|---|---|
| `lib/hx-genesis.hypercubical` | 100s | 2767 passed, 0 failed; 994503737 interaction steps performed; 1260555 cells live; 36979 payloads live |
| `lib/ir/beast-omni-cryptic.hypercubical` | 5s | 2483 passed, 0 failed; 10593551 interaction steps performed; 39286 cells live; 465 payloads live |
| `lib/ir/hx-collablink.hypercubical` | 5s | #run program: writes collab.html and compiles the UBSan-clean bridge binary; exit 0 |
| `lib/ir/hx-grh-surface.hypercubical` | 11s | 725 passed, 0 failed; 81822904 interaction steps performed; 776161 cells live; 151 payloads live |
| `lib/ir/hx-univalent-surface.hypercubical` | 6s | 733 passed, 0 failed; 52608897 interaction steps performed; 709826 cells live; 255 payloads live |
| `lib/ir/hx-universal-eval.hypercubical` | 4s | 761 passed, 0 failed; 18071640 interaction steps performed; 121494 cells live; 955 payloads live |
| `lib/ir/hx-x402-cashier.hypercubical` | 2s | 787 passed, 0 failed; 4627155 interaction steps performed; 8629 cells live; 15 payloads live |
| `lib/ir/poly-opt.hypercubical` | 2s | 2603 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `lib/math/adelic-theta-poisson.hypercubical` | 2s | 1094 passed, 0 failed; 10028533 interaction steps performed; 25454 cells live; 848 payloads live |
| `lib/math/cm-formal-group-divisibility.hypercubical` | 1s | 1079 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `lib/math/grh-atlas.hypercubical` | 104s | 3101 passed, 0 failed; 1060335272 interaction steps performed; 1373882 cells live; 46271 payloads live |
| `lib/math/rh-alpo-masullo.hypercubical` | 1s | 965 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `lib/math/rh-masullo-spine.hypercubical` | 1s | 1033 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `lib/math/rh-unified-cores.hypercubical` | 2s | 1064 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `lib/math/riemann-zeta.hypercubical` | 1s | 926 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `lib/math/synesthetic-flow.hypercubical` | 1s | 1003 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `lib/os-flux.hypercubical` | 0s | 714 passed, 0 failed; 410794 interaction steps performed; 9176 cells live; 165 payloads live |
| `lib/os-supercomputer.hypercubical` | 14s | 2762 passed, 0 failed; 52523066 interaction steps performed; 152672 cells live; 12191 payloads live |
| `lib/os-this.hypercubical` | 1s | 737 passed, 0 failed; 3385164 interaction steps performed; 10677 cells live; 51 payloads live |
| `lib/py-syntax.hypercubical` | 1s | 281 passed, 0 failed; 54461 interaction steps performed; 8794 cells live; 0 payloads live |
| `lib/semantic-factor.hypercubical` | 61s | #run program: prints its subterm factoring and ends "ZERO POSTULATES · ZERO C CHANGES · ZERO AGENTS IN LOOP"; exit 0 |
| `lib/treatise-synesthesia.hypercubical` | 1s | 743 passed, 0 failed; 1734516 interaction steps performed; 21357 cells live; 291 payloads live |

Not run here: `lib/server` (CI job 3 checks the full library on every main commit). Not published: `lib/ir/hx-total-reflection`, which is marked confidential.
