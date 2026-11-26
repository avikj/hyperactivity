# Verification — science-70bb4035

Machine: 2 cores, 8 GB. Kernel: build/cubical from main e211d839c (src/ and Makefile unchanged since 92ee19d26, so the bundled cubical-linux-x86_64 is the same build as science-974476df's). store/ROOT set to 70bb4035f70b8317e56678c55588f035419d5e31dffb70f31e097b0c5caab426, then `./build/cubical --index`, then each module in turn with a 900 s limit. A module whose declarations an earlier module in the run already checked reports its passes with 0 interaction steps.

| module | wall | result |
|---|---|---|
| `chem/cgeom.hypercubical` | 119s | 787 passed, 0 failed; 1185030624 interaction steps performed; 105015 cells live; 2896 payloads live |
| `chem/h2two.hypercubical` | 93s | 1377 passed, 0 failed; 846236954 interaction steps performed; 382816 cells live; 8647 payloads live |
| `chem/bondcross.hypercubical` | 273s | 319 passed, 0 failed; 4221683702 interaction steps performed; 4948 cells live; 7 payloads live |
| `chem/bondsep.hypercubical` | 1s | 315 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/bondpoly.hypercubical` | 2s | 1288 passed, 0 failed; 750500 interaction steps performed; 7627 cells live; 5 payloads live |
| `chem/cwbound.hypercubical` | 3s | 1379 passed, 0 failed; 471400 interaction steps performed; 12851 cells live; 9 payloads live |
| `chem/ringbound.hypercubical` | 0s | 316 passed, 0 failed; 2573771 interaction steps performed; 4911 cells live; 7 payloads live |
| `chem/nethop.hypercubical` | 3s | 774 passed, 0 failed; 2074354 interaction steps performed; 6227 cells live; 11 payloads live |
| `chem/h2-net.hypercubical` | 4s | 1318 passed, 0 failed; 43630440 interaction steps performed; 8149 cells live; 9 payloads live |
| `chem/h2plus-net.hypercubical` | 3s | 1369 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/netpiece.hypercubical` | 2s | 1327 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/fluxsplit.hypercubical` | 1s | 1314 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/ringatom.hypercubical` | 41s | 1355 passed, 0 failed; 636885445 interaction steps performed; 10854 cells live; 15 payloads live |
| `chem/vertexhop.hypercubical` | 1s | 773 passed, 0 failed; 11377130 interaction steps performed; 6313 cells live; 7 payloads live |
| `chem/cover.hypercubical` | 2s | 1334 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/sectors.hypercubical` | 2s | 820 passed, 0 failed; 22045196 interaction steps performed; 7944 cells live; 107 payloads live |
| `chem/linkhop.hypercubical` | 2s | 771 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/ringmoment.hypercubical` | 2s | 784 passed, 0 failed; 17481626 interaction steps performed; 6629 cells live; 13 payloads live |
| `chem/netatom.hypercubical` | 178s | 1377 passed, 0 failed; 1826003952 interaction steps performed; 10020 cells live; 5 payloads live |
| `chem/doubling.hypercubical` | 2s | 1311 passed, 0 failed; 8986293 interaction steps performed; 9071 cells live; 23 payloads live |
| `chem/periodic-table-exact.hypercubical` | 13s | 888 passed, 0 failed; 50102152 interaction steps performed; 55512 cells live; 3849 payloads live |

Total: 21 modules, 21 passed with 0 failed.

Fresh clone of main (depth 1, e211d839c), `make`, the three Run commands above: `chem/cgeom.hypercubical` 787 passed, 0 failed; 1185030624 interaction steps, the same line as the table.
