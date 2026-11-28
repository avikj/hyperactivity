ifeq ($(shell uname -s),Darwin)
CC      ?= clang
else
CC      ?= gcc
endif
ARCH    := $(shell uname -m)
ifeq ($(filter arm64 aarch64,$(ARCH)),)
NATIVE  ?= $(shell $(CC) -march=native -x c -c /dev/null -o /dev/null 2>/dev/null && echo -march=native)
else
NATIVE  ?= $(shell $(CC) -mcpu=native -x c -c /dev/null -o /dev/null 2>/dev/null && echo -mcpu=native)
endif
CFLAGS  ?= -std=gnu11 -O3 $(NATIVE) -g -Wall -Wextra -Wno-unused-parameter
ifeq ($(shell uname -s),Darwin)
LDFLAGS ?= -Wl,-stack_size,0x10000000
endif
SAN     := -fsanitize=address,undefined -fno-omit-frame-pointer

SRC := src/dnf.c src/big.c src/net.c src/lang.c
HDR := src/core.h src/syntax.c src/store.c src/sha256.c src/drivers.c

all: build/cubical

build/cubical: $(SRC) $(HDR)
	@mkdir -p build
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) -lpthread -lm -ldl

hx: build/hx

build/hx: build/cubical
	@mkdir -p build
	@printf '#run (scmd (w-file-write-named "build/hx.c" hx-c) (λ _ → sdone unit))\n' > build/.emit_hx.hypercubical
	./build/cubical lib/hx.hypercubical build/.emit_hx.hypercubical
	@rm -f build/.emit_hx.hypercubical
	$(CC) -std=c11 -O3 $(NATIVE) $(LDFLAGS) -o $@ build/hx.c -lpthread -lm
	@rm -f build/hx.c

build/cubical-asan: $(SRC) $(HDR)
	@mkdir -p build
	$(CC) $(CFLAGS) $(SAN) $(LDFLAGS) -o $@ $(SRC) -lpthread -lm -ldl

build/cubical-tsan: $(SRC) $(HDR)
	@mkdir -p build
	$(CC) -std=gnu11 -O1 -g -fsanitize=thread -o $@ $(SRC) -lpthread

# the machine as a library: a C program calls checked definitions (src/libcubical.h)
build/libcubical.a: $(SRC) $(HDR) src/libcubical.h
	@mkdir -p build/lib
	for f in $(SRC); do $(CC) $(CFLAGS) -DCUBICAL_LIB -c -o build/lib/$$(basename $$f .c).o $$f || exit 1; done
	ar rcs $@ build/lib/*.o

# Every file is checked by `make every` (2026-11-12): each file outside the library imports what
# it needs (the core and the library are lib/core.hypercubical and lib/library.hypercubical, files of imports in order; the x86 model's order is
# lib/isa/x86/all.hypercubical, written by tools/sail2cub.py), so each file no file imports is checked alone
# with its imports, without performing what acts on the world (--read). Measured 2026-11-12: 80 such
# files; 35 with failures of their own, each from an open node: the untyped definitions refused in
# the library (autodiff, eqproof, ring, intlin, derive, cxx/ast) and the C++ units over
# cxx/ast; bitlaw and flags; build-cost and its runs; the C++ reading (cxx/machine,
# cxx/read-expr, mem/sum); the Rust reading (deriving needs lib/derive.hypercubical); lib/nbe.hypercubical, which
# imports the core when the working copy is swapped in; and the surface items fail-local, fail-one,
# prelude-names and type-data, whose failures are what they show.
#
#
# the core and the library, in dependency order (lib/core.hypercubical, lib/library.hypercubical: each a file of
# imports): every definition is checked by the checker as it loads. There are no tests: the checker
# is the evidence. The one measurement kept is Brunerie's number.
CORE := lib/core.hypercubical
LIB  := lib/library.hypercubical
library: build/cubical
	CUBICAL_NOCACHE=1 ./build/cubical $(LIB)

# Brunerie's number (Ljungström's brunerie', the typed port experiments/brunerie/typed.hypercubical): -2, with one
# and with four workers. (2026-11-12: switched from the untyped port, which the checker has refused
# since 2026-11-09; the typed port computes -2 in 358,603 steps of the reading, 6.6 s end to end.)
brunerie: build/cubical
	for w in 0 4; do CUBICAL_NOCACHE=1 CUBICAL_WORKERS=$$w ./build/cubical $(CORE) \
	  experiments/brunerie/typed.hypercubical experiments/brunerie/run-typed.hypercubical | grep -A1 -x brunerie | grep -q "= negsuc (suc zero)" \
	  || { echo "brunerie: not -2 with $$w workers"; exit 1; }; done; echo "brunerie = negsuc (suc zero)"

# the workflow (lib/workflow.hypercubical, workflow/hyperactivity.hypercubical) checks; `hyperactive check` also
# confirms every node's name is a sentence and every node has a statement
workflow: build/cubical
	CUBICAL_NOCACHE=1 ./build/cubical workflow/hyperactivity.hypercubical
# every file checked is `hyperactive every` (lib/hyperactive.hypercubical)

.PHONY: all hx library brunerie workflow
