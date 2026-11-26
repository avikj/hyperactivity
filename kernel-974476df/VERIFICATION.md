# hyperactivity kernel — verification record (store root 974476df)

Source: avikj/hyperactivity commit 92ee19d26, store root
974476df85cd5ed2fe5aefde5432b81d24156c05f170b67ddaf5b5e71763a8af.
Platform: Linux x86-64, 2 vCPU, 8 GB memory cgroup, gcc, glibc static. Every run with `CUBICAL_NOCACHE=1`.

## Binaries
- `cubical-linux-x86_64`: `cc -std=gnu11 -O3 -static src/dnf.c src/big.c src/net.c src/lang.c -lpthread -lm -ldl`
- `cubical-asan-linux-x86_64`: same sources, `-O3 -g -fsanitize=address,undefined -fno-omit-frame-pointer`
- `cubical-tsan-linux-x86_64`: same sources, `-O1 -g -fsanitize=thread`

## Checks
| file(s) | result | steps | wall |
|---|---|---|---|
| lib/library.hypercubical (the core and the library, `make library`) | 3168 passed, 0 failed | 342,186,660 | 69.0 s |
| Brunerie (lib/core + experiments/brunerie/typed + run-typed), 1 worker | `negsuc (suc zero)` = −2 (395,608 steps of the reading); 263 passed, 0 failed | 8,877,242 | 2.4 s |
| Brunerie, 4 workers | −2 (405,568 steps); 263 passed, 0 failed | 9,121,572 | 3.4 s |
| Brunerie under ASan+UBSan, 1 and 4 workers | −2, 263 passed, 0 failed, no sanitizer report | — | 39 s / 42 s |
| Brunerie under TSan, 4 workers | −2, 263 passed, 0 failed, no data race reported | — | 223 s |
| workflow/hyperactivity.hypercubical | 2716 passed, 0 failed | 210,203,247 | 59.1 s |
| lib/self-host-tcb.hypercubical | 1170 passed, 0 failed | 61,582,883 | 17.9 s |
| lib/kernel.hypercubical | 4061 passed, 0 failed | 5,570,960,932 | 839.2 s |
| lib/core + lib/check.hypercubical | 189 passed, 0 failed | 2,870,001 | 1 s |
| lib/guard.hypercubical | exit 0 (silent by design) | — | 68.4 s |
| lib/nbe-model.hypercubical | 775 passed, 0 failed | 287,365,487 | 108.2 s |
| lib/nbe-det, nbe-interval, nbe-cmap, nbe-subst, nbe | not completed here: each process was killed by the 8 GB memory cgroup at 4.0–5.9 GB resident, while other checks ran beside it | — | 88–230 s |

`lib/check.hypercubical` carries no imports: it is written to load inside `lib/core.hypercubical`'s order
(after the bootstrap files), so checking it alone reports unbound names (`Level`, `notb`); checked in
that order it passes, as above.
