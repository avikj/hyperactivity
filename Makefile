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

# the library, in dependency order: every definition is checked by the checker as it loads.
# There are no tests: the checker is the evidence (PLAN.md section 1, "Every piece's output is
# checkable"). The one measurement kept is Brunerie's number (PLAN item 8, 18.16).
CORE := lib/prelude.cub lib/code.cub lib/match.cub lib/nav.cub lib/cost.cub lib/lin.cub lib/check.cub lib/std.cub
LIB  := $(CORE) lib/sort.cub lib/linproof.cub lib/eqproof.cub lib/ring.cub lib/autodiff.cub lib/intproof.cub lib/intlin.cub \
        lib/prim.cub lib/word.cub lib/derive.cub lib/decode.cub lib/fiber.cub lib/proc.cub lib/world.cub lib/session.cub \
        lib/choreo.cub lib/obs.cub lib/region.cub lib/specialize.cub lib/rules.cub lib/machine.cub lib/x86.cub lib/elf.cub \
        lib/os/platform.cub lib/os/linux.cub lib/os/contract.cub lib/os/replay.cub \
        lib/isa/tso.cub lib/isa/tso-acyclic.cub lib/mem/atomics.cub lib/mem/rc11.cub lib/cxx/ast.cub lib/lang/rust/ast.cub \
        lib/compile/tm.cub lib/compile/stack.cub

library: build/cubical
	CUBICAL_NOCACHE=1 ./build/cubical $(LIB)

# Brunerie's number (Ljungström's brunerie', an untyped port): -2, with one and with four workers
brunerie: build/cubical
	for w in 0 4; do CUBICAL_NOCACHE=1 CUBICAL_WORKERS=$$w ./build/cubical $(CORE) \
	  experiments/brunerie/brunerie.cub experiments/brunerie/run-brunerie.cub | grep -A1 -x brunerie | grep -q "= negsuc (suc zero)" \
	  || { echo "brunerie: not -2 with $$w workers"; exit 1; }; done; echo "brunerie = negsuc (suc zero)"

.PHONY: all library brunerie
