CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O2 -g -Wall -Wextra -Wno-unused-parameter
SAN     := -fsanitize=address,undefined -fno-omit-frame-pointer

SRC := src/dnf.c src/big.c src/net.c src/lang.c
HDR := src/core.h src/syntax.c src/store.c src/sha256.c src/drivers.c

all: build/cubical

build/cubical: $(SRC) $(HDR)
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $(SRC) -lpthread -lm -ldl

build/cubical-asan: $(SRC) $(HDR)
	@mkdir -p build
	$(CC) $(CFLAGS) $(SAN) -o $@ $(SRC) -lpthread -lm -ldl

build/cubical-tsan: $(SRC) $(HDR)
	@mkdir -p build
	$(CC) -std=gnu11 -O1 -g -fsanitize=thread -o $@ $(SRC) -lpthread

# the machine as a library: a C program calls checked definitions (src/libcubical.h)
build/libcubical.a: $(SRC) $(HDR) src/libcubical.h
	@mkdir -p build/lib
	for f in $(SRC); do $(CC) $(CFLAGS) -DCUBICAL_LIB -c -o build/lib/$$(basename $$f .c).o $$f || exit 1; done
	ar rcs $@ build/lib/*.o

# Every file is checked by `make every` (2026-11-12, t30): each file outside the library imports what
# it needs (the core and the library are lib/core.cub and lib/library.cub, files of imports in order; the x86 model's order is
# lib/isa/x86/all.cub, written by tools/sail2cub.py), so each file no file imports is checked alone
# with its imports, without performing what acts on the world (--read). Measured 2026-11-12: 80 such
# files; 35 with failures of their own, each from an open node: the untyped definitions refused in
# the library (t16-t21: autodiff, eqproof, ring, intlin, derive, cxx/ast) and the C++ units over
# cxx/ast; bitlaw and flags (t68); build-cost and its runs (t69); the C++ reading (t70: cxx/machine,
# cxx/read-expr, mem/sum); the Rust reading (t39: deriving needs lib/derive.cub); lib/nbe.cub, which
# imports the core when the working copy is swapped in; and the surface items fail-local, fail-one,
# prelude-names and type-data, whose failures are what they show.
#
#
# the core and the library, in dependency order (lib/core.cub, lib/library.cub: each a file of
# imports): every definition is checked by the checker as it loads. There are no tests: the checker
# is the evidence. The one measurement kept is Brunerie's number.
CORE := lib/core.cub
LIB  := lib/library.cub
library: build/cubical
	CUBICAL_NOCACHE=1 ./build/cubical $(LIB)

# Brunerie's number (Ljungström's brunerie', the typed port experiments/brunerie/typed.cub): -2, with one
# and with four workers. (2026-11-12: switched from the untyped port, which the checker has refused
# since 2026-11-09; the typed port computes -2 in 358,603 steps of the reading, 6.6 s end to end.)
brunerie: build/cubical
	for w in 0 4; do CUBICAL_NOCACHE=1 CUBICAL_WORKERS=$$w ./build/cubical $(CORE) \
	  experiments/brunerie/typed.cub experiments/brunerie/run-typed.cub | grep -A1 -x brunerie | grep -q "= negsuc (suc zero)" \
	  || { echo "brunerie: not -2 with $$w workers"; exit 1; }; done; echo "brunerie = negsuc (suc zero)"

# the record of the work (lib/task-graph.cub, task-graph/): the goals and tickets check; `python3 tools/task-graph.py
# check` also confirms every ticket record has its one text comment in the code
task-graph: build/cubical
	CUBICAL_NOCACHE=1 ./build/cubical task-graph/hyperactivity.cub
# every file checked (t30): each file no file imports, alone with its imports, without performing
# what acts on the world; per file, its own failures (tools/task-graph.py every)
every: build/cubical
	python3 tools/task-graph.py every

.PHONY: all library brunerie task-graph every
