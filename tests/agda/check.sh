#!/bin/sh
# tests/agda/check.sh — translate every Cubical Agda example here with tools/agda2cub.py and
# check the result with the machine (plan 12 item 29). Run from scoped-machine/; the Makefile's
# test target calls it. Each example carries its readings as `-- cub:` lines.
set -e
cd "$(dirname "$0")/../.."
mkdir -p build/agda
for f in tests/agda/*.agda; do
  b=$(basename "$f" .agda)
  python3 tools/agda2cub.py "$f" -o "build/agda/$b.cub"
  CUBICAL_NOCACHE=1 ./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub "build/agda/$b.cub" > "build/agda/$b.out" 2>&1 || { echo "agda: $b: the machine rejected the translation"; tail -5 "build/agda/$b.out"; exit 1; }
  if grep -q '^FAIL' "build/agda/$b.out"; then echo "agda: $b failed"; grep -A4 '^FAIL' "build/agda/$b.out"; exit 1; fi
  tail -1 "build/agda/$b.out" | sed "s/^/agda: $b: /"
done
