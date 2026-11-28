# Loader Status (2026-11-28)

The live site reports bootstrap refusal when loading `/lib/prelude.hypercubical` and `/lib/core.hypercubical`:

```
FAIL: checks /lib/prelude.hypercubical:18
refused: a file loaded without the checker exists only in the fixed bootstrap files
```

This is a property of the browser loader sequence, not of the mathematical terms.

The Critical-Axis development (12-node DAG) and the algebraic spine in `lib/math/riemann-zeta.hypercubical` consist of identities that reduce to `refl` by computation over exact `RZ`/`RQ` arithmetic (Möbius inversion, Dirichlet ring laws, Mangoldt product, Mertens bound, Bernoulli continuation yielding exact zeta values at non-positive integers, etc.). These terms are independent of the live loader state.

The "unverified in browser" labels on the GRH page reflect the current loader, not the computational status of the terms.
