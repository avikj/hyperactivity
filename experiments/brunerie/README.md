# Brunerie's number

`typed.cub` ports Ljungström's `brunerie'` from agda/cubical
(`Cubical/Homotopy/Group/Pi4S3/BrunerieExperiments.agda`), every definition typed. `make brunerie`
(`hyperactive brunerie`) loads it with `run-typed.cub` and computes −2, with and without workers.

`run-brunerie.cub`, `run-circle.cub` and `run-square.cub` load after `typed.cub`.

(2026-11-13: the earlier untyped port, refused by the checker since 2026-11-09, and the two runs that
needed it, `run-cube4.cub` and `run-squash.cub`, were removed; the history keeps them.)
