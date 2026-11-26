# Exact chemistry, physics, quantum gravity, folding, neuroscience and sound — verification record

Source: avikj/hyperactivity commit 92ee19d26, store root 974476df85cd5ed2fe5aefde5432b81d24156c05f170b67ddaf5b5e71763a8af.
Kernel: `build/cubical` built by `make` from that commit (gcc, Linux x86-64, 2 vCPU). Each module was checked alone from the repository root with
`timeout 600 ./build/cubical MODULE`; the line shown is the kernel's own summary. A step count of 0 means every definition was answered from the
kernel's content-addressed check cache (`.cubical-store`), filled by earlier checks with the same kernel build.

Result: **123 of 126 modules checked, 0 failed**; 3 modules exceed the 600 s bound (below). Total kernel time 4,385 s.

| module | exit | seconds | kernel summary |
|---|---|---|---|
| `chem/algebra.hypercubical` | 0 | 67.4 | 1286 passed, 0 failed; 344649880 interaction steps performed; 390899 cells live; 8763 payloads live |
| `chem/algebraclaim.hypercubical` | 0 | 3.1 | 1294 passed, 0 failed; 611697 interaction steps performed; 7976 cells live; 11 payloads live |
| `chem/atom.hypercubical` | 0 | 1.4 | 269 passed, 0 failed; 445752 interaction steps performed; 5408 cells live; 159 payloads live |
| `chem/bareisswit.hypercubical` | 0 | 110.4 | 1568 passed, 0 failed; 562663369 interaction steps performed; 56297 cells live; 5871 payloads live |
| `chem/barta.hypercubical` | 0 | 5.7 | 1273 passed, 0 failed; 3622984 interaction steps performed; 7823 cells live; 7 payloads live |
| `chem/basin-compose.hypercubical` | 0 | 1.4 | 253 passed, 0 failed; 439523 interaction steps performed; 8876 cells live; 104 payloads live |
| `chem/bholonomy.hypercubical` | 0 | 1.8 | 767 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/binary.hypercubical` | 0 | 1.1 | 635 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/born-solvation.hypercubical` | 0 | 2.6 | 1294 passed, 0 failed; 306517 interaction steps performed; 7960 cells live; 11 payloads live |
| `chem/bsound.hypercubical` | 0 | 1.4 | 871 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/bvolume.hypercubical` | 0 | 1.4 | 734 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/charge.hypercubical` | 0 | 0.8 | 307 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/collapse.hypercubical` | 0 | 2.9 | 1411 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/crystal-materials-exact.hypercubical` | 0 | 2.0 | 920 passed, 0 failed; 1635145 interaction steps performed; 10523 cells live; 123 payloads live |
| `chem/cut.hypercubical` | 0 | 6.1 | 1390 passed, 0 failed; 50819961 interaction steps performed; 9436 cells live; 33 payloads live |
| `chem/dirac.hypercubical` | 0 | 2.4 | 1182 passed, 0 failed; 5681489 interaction steps performed; 8889 cells live; 31 payloads live |
| `chem/dyadic.hypercubical` | 0 | 29.7 | 1974 passed, 0 failed; 246547390 interaction steps performed; 39062 cells live; 2885 payloads live |
| `chem/dyadicclaim.hypercubical` | 0 | 7.5 | 1980 passed, 0 failed; 2879908 interaction steps performed; 10679 cells live; 11 payloads live |
| `chem/dyadicfock.hypercubical` | 0 | 57.0 | 2058 passed, 0 failed; 639400554 interaction steps performed; 13883 cells live; 19 payloads live |
| `chem/electric.hypercubical` | 0 | 0.8 | 611 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/facts.hypercubical` | 0 | 0.5 | 236 passed, 0 failed; 137138 interaction steps performed; 6085 cells live; 65 payloads live |
| `chem/fermi.hypercubical` | 0 | 1.7 | 1314 passed, 0 failed; 350536 interaction steps performed; 9162 cells live; 0 payloads live |
| `chem/fermiclaim.hypercubical` | 0 | 1.7 | 1336 passed, 0 failed; 573575 interaction steps performed; 9948 cells live; 17 payloads live |
| `chem/fine.hypercubical` | 0 | 1.7 | 1266 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/fineclaim.hypercubical` | 0 | 4.0 | 1267 passed, 0 failed; 30119966 interaction steps performed; 7493 cells live; 5 payloads live |
| `chem/fock.hypercubical` | 0 | 7.1 | 2005 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/fockfermi.hypercubical` | 0 | 9.9 | 2068 passed, 0 failed; 25332557 interaction steps performed; 16276 cells live; 7 payloads live |
| `chem/fold.hypercubical` | 0 | 0.6 | 229 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/freedual.hypercubical` | 0 | 32.9 | 1341 passed, 0 failed; 346738911 interaction steps performed; 38574 cells live; 11 payloads live |
| `chem/freespace.hypercubical` | 0 | 28.1 | 1378 passed, 0 failed; 298429781 interaction steps performed; 9781 cells live; 7 payloads live |
| `chem/frustration.hypercubical` | 0 | 5.4 | 1349 passed, 0 failed; 44567736 interaction steps performed; 8124 cells live; 5 payloads live |
| `chem/gaugetest.hypercubical` | 0 | 60.9 | 2065 passed, 0 failed; 225595258 interaction steps performed; 14870 cells live; 27 payloads live |
| `chem/h2cert.hypercubical` | 0 | 51.8 | 1257 passed, 0 failed; 431668509 interaction steps performed; 1105180 cells live; 2445 payloads live |
| `chem/h2claim.hypercubical` | 124 | 600.0 | not finished within 600 s |
| `chem/h2ground.hypercubical` | 0 | 3.2 | 1284 passed, 0 failed; 3340824 interaction steps performed; 13750 cells live; 533 payloads live |
| `chem/h2plus.hypercubical` | 0 | 1.1 | 830 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/h2r2.hypercubical` | 124 | 600.1 | not finished within 600 s |
| `chem/h2sound.hypercubical` | 0 | 5.3 | 1133 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/hbond-orbital.hypercubical` | 0 | 3.9 | 1290 passed, 0 failed; 441287 interaction steps performed; 7744 cells live; 7 payloads live |
| `chem/he.hypercubical` | 0 | 3.6 | 1223 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/he2.hypercubical` | 0 | 14.6 | 1369 passed, 0 failed; 54883685 interaction steps performed; 9706 cells live; 7 payloads live |
| `chem/he2claim.hypercubical` | 0 | 245.1 | 1371 passed, 0 failed; 2069620638 interaction steps performed; 7972 cells live; 5 payloads live |
| `chem/hecert.hypercubical` | 0 | 12.6 | 1909 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/heclaim.hypercubical` | 0 | 394.6 | 1231 passed, 0 failed; 4089322768 interaction steps performed; 7990 cells live; 11 payloads live |
| `chem/hetemple.hypercubical` | 124 | 600.1 | not finished within 600 s |
| `chem/holonomy.hypercubical` | 0 | 1.7 | 441 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/hubbard.hypercubical` | 0 | 3.6 | 1244 passed, 0 failed; 2163113 interaction steps performed; 10553 cells live; 9 payloads live |
| `chem/inertia.hypercubical` | 0 | 1.7 | 1154 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/inertiaq.hypercubical` | 0 | 1.4 | 1180 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/inertiasound.hypercubical` | 0 | 2.8 | 1322 passed, 0 failed; 1528774 interaction steps performed; 10398 cells live; 495 payloads live |
| `chem/inertiathm.hypercubical` | 0 | 2.6 | 1533 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/laplace.hypercubical` | 0 | 1.7 | 1325 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/legendre.hypercubical` | 0 | 3.2 | 1305 passed, 0 failed; 16554798 interaction steps performed; 7931 cells live; 7 payloads live |
| `chem/linalg.hypercubical` | 0 | 2.5 | 1353 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/lithium-exact.hypercubical` | 0 | 2.0 | 1291 passed, 0 failed; 1301543 interaction steps performed; 7786 cells live; 9 payloads live |
| `chem/local.hypercubical` | 0 | 1.5 | 1217 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/magloop.hypercubical` | 0 | 108.8 | 804 passed, 0 failed; 798339902 interaction steps performed; 18178 cells live; 117 payloads live |
| `chem/magnetic.hypercubical` | 0 | 2.3 | 783 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/multiatom-exact.hypercubical` | 0 | 4.0 | 1292 passed, 0 failed; 1009019 interaction steps performed; 7855 cells live; 9 payloads live |
| `chem/new-physics-nuclei.hypercubical` | 0 | 15.3 | 2825 passed, 0 failed; 5899701 interaction steps performed; 29768 cells live; 4969 payloads live |
| `chem/onecentre.hypercubical` | 0 | 3.6 | 1357 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/order.hypercubical` | 0 | 1.3 | 692 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/organic-oncology-exact.hypercubical` | 0 | 5.4 | 2693 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/pair-base-n.hypercubical` | 0 | 5.5 | 1291 passed, 0 failed; 2934231 interaction steps performed; 7786 cells live; 9 payloads live |
| `chem/pairbase.hypercubical` | 0 | 252.0 | 1957 passed, 0 failed; 1759493708 interaction steps performed; 11391 cells live; 13 payloads live |
| `chem/pairgauge.hypercubical` | 0 | 3.0 | 1396 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/pairlevel.hypercubical` | 0 | 378.0 | 2011 passed, 0 failed; 3476911734 interaction steps performed; 13745 cells live; 17 payloads live |
| `chem/pairone.hypercubical` | 0 | 3.8 | 1279 passed, 0 failed; 5775196 interaction steps performed; 8030 cells live; 17 payloads live |
| `chem/pairthm.hypercubical` | 0 | 2.3 | 1281 passed, 0 failed; 405070 interaction steps performed; 7705 cells live; 7 payloads live |
| `chem/pauli.hypercubical` | 0 | 6.8 | 1347 passed, 0 failed; 48225302 interaction steps performed; 8711 cells live; 7 payloads live |
| `chem/periodic-table-exact.hypercubical` | 0 | 2.3 | 888 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/pocket-binder.hypercubical` | 0 | 2.7 | 1317 passed, 0 failed; 1694332 interaction steps performed; 10377 cells live; 959 payloads live |
| `chem/quantum-chem-space.hypercubical` | 0 | 5.7 | 2773 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/quantum-forces.hypercubical` | 0 | 2.0 | 1294 passed, 0 failed; 576585 interaction steps performed; 7966 cells live; 11 payloads live |
| `chem/rdm2-sos.hypercubical` | 0 | 2.1 | 1292 passed, 0 failed; 506281 interaction steps performed; 7855 cells live; 9 payloads live |
| `chem/recoupling.hypercubical` | 0 | 0.8 | 345 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/ring.hypercubical` | 0 | 2.5 | 777 passed, 0 failed; 14416628 interaction steps performed; 6347 cells live; 7 payloads live |
| `chem/rootsum.hypercubical` | 0 | 1.0 | 483 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/selection.hypercubical` | 0 | 2.1 | 1315 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/separate.hypercubical` | 0 | 2.1 | 1295 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/sos.hypercubical` | 0 | 2.0 | 1318 passed, 0 failed; 79522 interaction steps performed; 7914 cells live; 0 payloads live |
| `chem/sosclaim.hypercubical` | 0 | 2.1 | 1324 passed, 0 failed; 192661 interaction steps performed; 8124 cells live; 7 payloads live |
| `chem/spin.hypercubical` | 0 | 0.7 | 259 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/sylvester.hypercubical` | 0 | 2.2 | 1493 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/tailblock.hypercubical` | 0 | 2.0 | 1382 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/vertex.hypercubical` | 0 | 0.7 | 419 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/volume.hypercubical` | 0 | 0.8 | 335 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/wetlab-laws.hypercubical` | 0 | 2.4 | 1325 passed, 0 failed; 2025395 interaction steps performed; 9289 cells live; 33 payloads live |
| `chem/witness.hypercubical` | 0 | 8.9 | 1925 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `chem/witnessclaim.hypercubical` | 0 | 378.7 | 1926 passed, 0 failed; 4032427646 interaction steps performed; 10185 cells live; 5 payloads live |
| `chem/zring.hypercubical` | 0 | 2.6 | 1896 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `phys/photonic-geodesic.hypercubical` | 0 | 1.5 | 883 passed, 0 failed; 123146 interaction steps performed; 8513 cells live; 87 payloads live |
| `phys/relativistic-atom.hypercubical` | 0 | 2.3 | 1296 passed, 0 failed; 2196952 interaction steps performed; 8045 cells live; 15 payloads live |
| `phys/superconductivity.hypercubical` | 0 | 1.9 | 1294 passed, 0 failed; 573340 interaction steps performed; 7966 cells live; 11 payloads live |
| `lqg/contextuality.hypercubical` | 0 | 2.3 | 1302 passed, 0 failed; 1490998 interaction steps performed; 24792 cells live; 302 payloads live |
| `lqg/cosmology-bounce.hypercubical` | 0 | 2.1 | 1294 passed, 0 failed; 389642 interaction steps performed; 7966 cells live; 11 payloads live |
| `lqg/graph-dynamics.hypercubical` | 0 | 1.9 | 1291 passed, 0 failed; 711501 interaction steps performed; 8137 cells live; 175 payloads live |
| `lqg/hamiltonian-vertex.hypercubical` | 0 | 2.2 | 1298 passed, 0 failed; 1148751 interaction steps performed; 8618 cells live; 11 payloads live |
| `lqg/recoupling-15j.hypercubical` | 0 | 19.0 | 357 passed, 0 failed; 191041164 interaction steps performed; 5571 cells live; 11 payloads live |
| `lqg/renormalization-fiber.hypercubical` | 0 | 0.7 | 243 passed, 0 failed; 249572 interaction steps performed; 6533 cells live; 145 payloads live |
| `lqg/rqm.hypercubical` | 0 | 0.9 | 248 passed, 0 failed; 760034 interaction steps performed; 8759 cells live; 437 payloads live |
| `lqg/semiclassical-bridge.hypercubical` | 0 | 2.2 | 1294 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `fold/amino-ramachandran.hypercubical` | 0 | 2.1 | 1301 passed, 0 failed; 129819 interaction steps performed; 8677 cells live; 9 payloads live |
| `fold/backbone-kinematics.hypercubical` | 0 | 2.2 | 1292 passed, 0 failed; 1987854 interaction steps performed; 7855 cells live; 9 payloads live |
| `fold/backbone-solver.hypercubical` | 0 | 5.0 | 1300 passed, 0 failed; 3313347 interaction steps performed; 10757 cells live; 7 payloads live |
| `fold/branch-and-bound.hypercubical` | 0 | 2.2 | 1295 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `fold/horn-kabsch.hypercubical` | 0 | 2.3 | 1288 passed, 0 failed; 1268583 interaction steps performed; 7633 cells live; 5 payloads live |
| `fold/hx-fold.hypercubical` | 0 | 3.4 | 1959 passed, 0 failed; 391589 interaction steps performed; 12360 cells live; 13 payloads live |
| `fold/lie-torque.hypercubical` | 0 | 2.1 | 1288 passed, 0 failed; 556721 interaction steps performed; 7627 cells live; 5 payloads live |
| `fold/se3-monoid.hypercubical` | 0 | 3.2 | 1290 passed, 0 failed; 8791077 interaction steps performed; 7744 cells live; 7 payloads live |
| `fold/sequence-folder.hypercubical` | 0 | 1.7 | 1316 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `fold/spectral-gram.hypercubical` | 0 | 1.8 | 1288 passed, 0 failed; 261287 interaction steps performed; 7633 cells live; 5 payloads live |
| `neuro/barrier.hypercubical` | 0 | 2.1 | 1292 passed, 0 failed; 1270380 interaction steps performed; 7855 cells live; 9 payloads live |
| `neuro/coproc.hypercubical` | 0 | 2.2 | 1314 passed, 0 failed; 689893 interaction steps performed; 9907 cells live; 67 payloads live |
| `neuro/descent.hypercubical` | 0 | 0.8 | 235 passed, 0 failed; 367074 interaction steps performed; 9396 cells live; 221 payloads live |
| `neuro/eye-contact.hypercubical` | 0 | 53.9 | 1293 passed, 0 failed; 11821051 interaction steps performed; 9104 cells live; 87 payloads live |
| `neuro/inhibition.hypercubical` | 0 | 0.9 | 274 passed, 0 failed; 159203 interaction steps performed; 7405 cells live; 111 payloads live |
| `neuro/lyapunov.hypercubical` | 0 | 1.8 | 1292 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `neuro/opioid-addiction.hypercubical` | 0 | 2.1 | 1292 passed, 0 failed; 399503 interaction steps performed; 7855 cells live; 9 payloads live |
| `neuro/reshaping-coproc.hypercubical` | 0 | 3.1 | 1519 passed, 0 failed; 2365091 interaction steps performed; 22615 cells live; 805 payloads live |
| `neuro/synesthesia.hypercubical` | 0 | 0.7 | 215 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `sound/daw.hypercubical` | 0 | 2.5 | 1414 passed, 0 failed; 2101742 interaction steps performed; 17058 cells live; 127 payloads live |
| `sound/graph.hypercubical` | 0 | 4.6 | 2205 passed, 0 failed; 4944289 interaction steps performed; 39189 cells live; 821 payloads live |
| `sound/hx-sound.hypercubical` | 0 | 4.1 | 2536 passed, 0 failed; 2519790 interaction steps performed; 14945 cells live; 0 payloads live |
| `sound/time.hypercubical` | 0 | 0.6 | 292 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |
| `sound/wave.hypercubical` | 0 | 1.8 | 1397 passed, 0 failed; 0 interaction steps performed; 0 cells live; 0 payloads live |

## Modules not finished within 600 s
- `chem/h2claim.hypercubical`: keeps the certified H₂⁺ energy at R = 2 as a verdict; its own header records the computation as 4·10¹⁰ interaction steps, run once.
- `chem/h2r2.hypercubical`: the two 48-bit certificates bracketing p for H₂⁺ at R = 2 bohr (radial chains past depth 60, angular past 6), computed once and kept as verdicts.
- `chem/hetemple.hypercubical`: helium's ground energy bracketed from both sides (Temple and Ritz) on the Hylleraas functions 1, s, u at α = 7/4.
All three are leaves that no module imports; on this 2-vCPU machine the certified arithmetic runs past the bound. Run them without `timeout` to obtain the verdict.

## Modules run past the 600 s limit (no timeout)
| module | exit | seconds | kernel summary |
|---|---|---|---|
| chem/h2claim.hypercubical | 0 | 3747 | 832 passed, 0 failed; 41,548,845,923 interaction steps |
