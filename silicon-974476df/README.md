# hyperactivity silicon / EDA bundle (store 974476df)

Netlists, RTL, testbenches, standard-cell library and GDSII streams computed by the hyperactivity kernel
from the definitions in `lib/bench/` and `lib/silicon/`. Each file is a checked value of the store; the
`drivers/` directory reproduces every file (see VERIFICATION.md for the exact commands).

- **EPFL arithmetic benchmarks (BLIF, official port signatures):** `epfl_adder128_area.blif` (892-node
  128-bit adder), `epfl_adder128_ks.blif` (7-stage Kogge–Stone), `epfl_bar128.blif` (7-stage logarithmic
  rotator), `epfl_max128.blif` (all-pairs prefix 4×128-bit signed max). Check against the official suite
  with `yosys-abc -c "cec epfl_adder.blif epfl_adder128_area.blif"`; map with
  `yosys-abc -c "read_lib sky130_fd_sc_hd__tt_025C_1v80.lib; read_blif FILE; strash; map; print_stats"`.
- **Interaction-net processor:** `hyperinet.sv` (`tt_um_hyperinet`, Tiny Tapeout SkyWater 130 nm pinout,
  dual-lane redex firing) and `tb_metacircular.sv`
  (`iverilog -g2012 -o tb hyperinet.sv tb_metacircular.sv && vvp tb`).
- **4-tile GALS HyperNet:** `hypernet_mpw_rtl.sv`, `hypernet_vs_rv32_tb.sv`, `hypercore_cmos7nm.lib`
  (7 nm NLDM Liberty), `hypernet_4tile.gds` (hierarchical GDSII v6.0, open in KLayout).
- **SoC and cells:** `hypercore_full_rtl.sv`, `hypercore_tb.sv`, `hypercore_soc_5layer.gds`,
  `xor4_gaa3nm.gds` (strictly gate- and area-optimal 4-NAND2 XOR), `hypercore_alu_3nm.gds` with
  `hypercore_alu_3nm_sum_expr.txt`.
- **Benchmarks and quantum:** `bench_eda.sv` (ISCAS-85 c17, EPFL adder8), `amy_tof4_orig.qasm` and
  `amy_tof4_opt.qasm` (OpenQASM 3.0 4-Toffoli cascade, 28 → 24 T).
