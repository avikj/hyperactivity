# Silicon / EDA bundle — verification record (store root 974476df)

Source: avikj/hyperactivity commit 92ee19d26, store root
974476df85cd5ed2fe5aefde5432b81d24156c05f170b67ddaf5b5e71763a8af. Kernel: `build/cubical` built by `make`
from that commit. Platform: Linux x86-64. Every artifact below is a value of the named module, written
to disk by the driver in `drivers/` (each driver is one `#run` of `w-file-write-named` calls; `OUT/` is
the output directory). The modules carry no `#run` of their own.

World signature loaded for the drivers: `W = lib/prim.hypercubical lib/proc.hypercubical lib/world.hypercubical`.

| command (from the repository root) | result | time |
|---|---|---|
| `build/cubical lib/bench/epfl-128.hypercubical W drivers/epfl128.hypercubical` | 1467 passed, 0 failed (143,599,198 steps) | 24.9 s |
| `build/cubical lib/bench/epfl-mult64.hypercubical` | 1250 passed, 0 failed | 1.4 s |
| `build/cubical lib/bench/eda-quantum.hypercubical W drivers/edaq.hypercubical` | 1449 passed, 0 failed | 2.3 s |
| `build/cubical lib/silicon/inet-fabric.hypercubical W drivers/fabric.hypercubical` | 1434 passed, 0 failed | 1.5 s |
| `build/cubical lib/silicon/inet-compiler.hypercubical W drivers/compiler.hypercubical` | 1477 passed, 0 failed | 1.8 s |
| `build/cubical lib/silicon/hypernet.hypercubical lib/world.hypercubical drivers/hypernet.hypercubical` | 1660 passed, 0 failed | 2.3 s |
| `build/cubical lib/silicon/tapeout.hypercubical lib/silicon/emit.hypercubical W drivers/tapeout.hypercubical` | 1416 passed, 0 failed | 1.6 s |
| `build/cubical lib/optimality.hypercubical` | 1825 passed, 0 failed | 3.5 s |

## Artifacts
| file | bytes | value | structural check here |
|---|---:|---|---|
| epfl_adder128_area.blif | 26,111 | `epfl-adder128-area-blif` | 892 `.names` nodes = 3 + 127×7 (`epfl-adder128-area-aig`); 256 in / 129 out (EPFL `adder` ports) |
| epfl_adder128_ks.blif | 112,675 | `epfl-adder128-ks-blif` | 3,456 `.names`; 256 in / 129 out |
| epfl_bar128.blif | 44,542 | `epfl-bar128-blif` | 896 `.names` = 128×7 three-input muxes (×3 AIG = 2,688, `epfl-bar128-aig`); 135 in / 128 out |
| epfl_max128.blif | 198,595 | `epfl-max128-blif` | 4,730 `.names`; 512 in / 130 out |
| bench_eda.sv | 1,669 | `bench-eda-sv` | ISCAS-85 c17 + EPFL adder8 NAND netlists |
| amy_tof4_orig.qasm / amy_tof4_opt.qasm | 722 / 698 | `amy-tof4-orig-qasm` / `amy-tof4-opt-qasm` | OpenQASM 3.0, 4-Toffoli cascade before/after T reduction |
| hyperinet.sv | 7,953 | `hyperinet-sv` | `tt_um_hyperinet` dual-lane interaction-net core (Tiny Tapeout pinout) |
| tb_metacircular.sv | 4,058 | `metacircular-tb-sv` | testbench for the three compiled programs |
| hypernet_mpw_rtl.sv / hypernet_vs_rv32_tb.sv | 14,237 / 2,044 | `hypernet-mpw-rtl-sv` / `sv-hypernet-vs-rv32-tb` | 4-tile GALS HyperNet RTL and its RV32 comparison testbench |
| hypercore_cmos7nm.lib | 5,165 | `hypercore-cmos7nm-lib` | 7 nm NLDM Liberty |
| hypercore_full_rtl.sv / hypercore_tb.sv | 7,175 / 4,461 | `hypercore-full-rtl-sv` / `hypercore-sv-testbench` | SoC RTL suite and testbench |
| hypernet_4tile.gds | 1,328 | `hypernet-4tile-gds-bytes` | GDSII v6.0 parses: 101 records, HEADER…ENDLIB, 2 structures, 17 BOUNDARY |
| hypercore_soc_5layer.gds | 890 | `hypercore-soc-gds6-bytes` | parses: 68 records, 1 structure, 12 BOUNDARY |
| xor4_gaa3nm.gds | 376 | `hypercore-gds-bytes` | parses: 28 records, 4 BOUNDARY (the 376-byte length is certified in emit.hypercubical) |
| hypercore_alu_3nm.gds | 440 | `gds-v6-bytes #10 #0` of the placed cells of `hypercore-alu-build` | parses: 33 records, 5 BOUNDARY |
| hypercore_alu_3nm_sum_expr.txt | 7 | SV expression of `hypercore-alu-build` (`(a ^ b)`) | the optimized sum expression, not a module |

External tools: yosys, yosys-abc, iverilog, vvp, verilator, klayout are not installed in this environment,
so the `abc cec` equivalence, Sky130 mapping and simulation transcripts in `docs/chip-design.md` were not
re-run here. The kernel checks above are the evidence produced in this run.

The 64×64 Dadda multiplier netlist (`hy_mult64_dadda.blif` in `docs/chip-design.md`) is not a value in
the store: `lib/bench/epfl-mult64.hypercubical` holds the cell generators (`dadda-fa-blif`,
`dadda-ha-blif`) and the conservation proofs, not the assembled netlist, so it is not in this bundle.
