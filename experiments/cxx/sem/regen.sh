#!/bin/sh
# experiments/cxx/sem/regen.sh — run from scoped-machine/. Writes experiments/cxx/sem/gen/*.cub:
# track C's lib/cxx/ast.cub and the imported units with the binary-operator constructors
# renamed bo-X → cbo-X, because lib/cxx/ast.cub's CBinOp (bo-add …) and lib/mem/scalar.cub's
# BinOp (bo-add …) bind the same eight names and a name is bound once (tracks/F-cxx.md, Found 1).
# Nothing else differs from track C's files; when the rename lands in tools/clang2cub.py these
# copies go away. Units given as extra arguments (a .cub written by tools/clang2cub.py) are
# renamed the same way.
set -e
out=experiments/cxx/sem/gen
mkdir -p $out
for f in lib/cxx/ast.cub experiments/cxx/book.cub experiments/cxx/second.cub experiments/cxx/third.cub "$@"; do
  perl -pe 's/(?<![A-Za-z0-9-])bo-/cbo-/g' "$f" > $out/$(basename "$f")
done
