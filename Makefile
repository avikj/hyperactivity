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

# t30 (2026-11-12): needs-work. Many files are checked by no target here: lib/mem/{target,scalar,
# sig,flat,memory,layout,monad} (measured 2026-11-12: loaded after LIB they check, 385 definitions,
# 0 failures), lib/bits.cub, lib/net.cub, lib/fleet.cub, lib/deploy.cub, lib/compile/*,
# lib/isa/x86/*, lib/cxx/machine.cub and read-expr.cub, lib/lang/rust/* other than ast. Their state
# is known only when someone loads them. One command should check every file meant to check, in
# dependency order, so the state of every file is measured.
# Measured 2026-11-12: the machine finding each file's prerequisites (load a base, the prerequisites
# found so far and the file; each unbound name looked up in an index of the repository's declarations
# and the file declaring the most of them added, until none is left). Run first with CORE as the
# base, it reported 63 of 217 files loading with 0 failures; that was wrong as a statement of the
# files, because most are written to load after the whole library: lib/isa/x86, for one, loaded
# after LIB, lib/bits.cub, the files of lib/isa/x86/ORDER and lib/isa/x86/run.cub, checks with 4,699
# passed and only LIB's own 52 failed (t16-t21). The order exists for that tree (ORDER, written by
# tools/sail2cub.py) and, for 122 files, in prose ("Load after ..."), which no machine reads. One
# combination ends the machine (t67). The language has `import "file"` (loaded once, by real path, the command
# line's files included): each file importing what it needs, written by the generator for generated
# files, makes every file loadable alone, the check of every file one run per file, and the reading
# of every declaration's uses (#uses, tools/work.py) cover the repository.
#
# t44 (2026-11-12): every target sets CUBICAL_NOCACHE=1, so the standard commands never use the
# store or the module cache and recheck everything from nothing (64 s for the library). With the
# store warm, elaboration and compilation still run for every definition (29 s), so incremental
# checking reaches only the checker's run, not the rest of the work done per definition.
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

# the record of the work (lib/work.cub, work/): the goals and tickets check; `python3 tools/work.py
# check` also confirms every ticket record has its one text comment in the code
work: build/cubical
	CUBICAL_NOCACHE=1 ./build/cubical work/graph.cub

.PHONY: all library brunerie work
