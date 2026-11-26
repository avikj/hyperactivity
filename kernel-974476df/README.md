# hyperactivity kernel — Linux x86-64

`cubical-linux-x86_64` is the hyperactivity machine: a De Morgan cubical type theory checked and run on a
lock-free interaction-net reducer. It needs no libraries.

Run it from a checkout of the repository (it reads code from `store/`):

    git clone https://github.com/avikj/hyperactivity && cd hyperactivity
    ./cubical-linux-x86_64 FILE.hypercubical [MORE.hypercubical ...]

Each file is checked as it loads, after the files before it; the last line reports
`N passed, M failed` and the interaction steps performed. Useful settings:

- `CUBICAL_WORKERS=N` (or `auto`): reduce with N worker threads.
- `CUBICAL_NOCACHE=1`: check everything again instead of reusing `.cubical-store`.
- `--read`: check without performing what acts on the world.

Brunerie's number, π₄(S³) = −2:

    ./cubical-linux-x86_64 lib/core.hypercubical experiments/brunerie/typed.hypercubical experiments/brunerie/run-typed.hypercubical

The sanitizer builds (`-asan`, `-tsan`) take the same arguments. `VERIFICATION.md` records every check run on
this build; `SHA256SUMS` lists the digests.
