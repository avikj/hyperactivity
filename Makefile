CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O2 -g -Wall -Wextra -Wno-unused-parameter
SAN     := -fsanitize=address,undefined -fno-omit-frame-pointer

SRC := src/dnf.c src/big.c src/net.c src/lang.c

all: build/cubical

build/cubical: $(SRC) src/core.h src/syntax.c src/store.c src/sha256.c src/drivers.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $(SRC) -lpthread -lm -ldl

build/cubical-asan: $(SRC) src/core.h src/syntax.c src/store.c src/sha256.c src/drivers.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(SAN) -o $@ $(SRC) -lpthread -lm -ldl

# the machine as a library (B18): a C program calls checked definitions (src/libcubical.h)
build/libcubical.a: $(SRC) src/core.h src/syntax.c src/store.c src/sha256.c src/drivers.c src/libcubical.h
	@mkdir -p build/lib
	for f in $(SRC); do $(CC) $(CFLAGS) -DCUBICAL_LIB -c -o build/lib/$$(basename $$f .c).o $$f || exit 1; done
	ar rcs $@ build/lib/*.o
build/native-main: tests/native/main.c build/libcubical.a
	$(CC) $(CFLAGS) -o $@ tests/native/main.c build/libcubical.a -lpthread -lm -ldl

# tests check: the module cache is off, except in the cache test (a fresh directory, run twice)
test: export CUBICAL_NOCACHE = 1
test: build/cubical build/cubical-asan
	python3 tools/names.py
	./build/cubical lib/prelude.cub tests/cubical.cub
	./build/cubical lib/prelude.cub lib/code.cub tests/code.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/fo.cub lib/emitc.cub
	python3 experiments/fo/run.py
	UNBOXED=1 python3 experiments/fo/run.py
	IMAGE=1 python3 experiments/fo/run.py
	X86=1 python3 experiments/fo/run.py
	./build/cubical lib/prelude.cub lib/code.cub lib/match.cub lib/nav.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/match.cub lib/lin.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/fo.cub lib/match.cub lib/check.cub lib/std.cub lib/stage.cub lib/lower.cub lib/specialize.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/fo.cub lib/match.cub lib/nav.cub lib/emitc.cub lib/check.cub lib/std.cub lib/stage.cub lib/lower.cub lib/specialize.cub lib/rules.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/fo.cub lib/match.cub lib/nav.cub lib/emitc.cub lib/check.cub lib/std.cub lib/stage.cub lib/lower.cub lib/specialize.cub lib/linproof.cub lib/prim.cub lib/decode.cub lib/rules.cub lib/derive.cub lib/proc.cub lib/emitt.cub lib/emitu.cub lib/repr.cub
	./build/cubical lib/prelude.cub tests/hit.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub tests/check.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub tests/reject.cub tests/quote.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub tests/codata.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub lib/linproof.cub lib/eqproof.cub lib/ring.cub lib/autodiff.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub tests/recursion.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/fo.cub lib/check.cub lib/std.cub lib/lower.cub lib/prim.cub lib/derive.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub tests/instance.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub tests/elab.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub lib/linproof.cub lib/prim.cub lib/decode.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub lib/proc.cub tests/proc.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub tests/guarded.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub lib/prim.cub lib/stage.cub tests/prim.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub lib/prim.cub lib/region.cub tests/region.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub tests/errors/untyped-imp.cub > build/untyped-imp.out 2>&1; grep -q "^ok: fine" build/untyped-imp.out && grep -q "applies idw to its explicit arguments only" build/untyped-imp.out || { echo "untyped implicit misuse not refused"; exit 1; }
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub tests/errors/untyped-imp-code.cub > build/untyped-imp-code.out 2>&1; grep -q "^error: wrong: an untyped definition applies idw to its explicit arguments only" build/untyped-imp-code.out || { echo "untyped implicit misuse under code not refused"; exit 1; }
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub tests/errors/ctor-name.cub > build/ctor-name.out 2>&1; grep -q "^ok: checks fine" build/ctor-name.out && grep -q "reuse a constructor's name.*: mkb" build/ctor-name.out || { echo "a definition named as a constructor not refused"; exit 1; }
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub tests/errors/ctor-name-u.cub > build/ctor-name-u.out 2>&1; grep -q "reuse a constructor's name.*: mky" build/ctor-name-u.out || { echo "an untyped definition named as a constructor not refused"; exit 1; }
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub lib/fiber.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/match.cub lib/lin.cub lib/check.cub lib/std.cub lib/linproof.cub lib/intproof.cub lib/intlin.cub lib/eqproof.cub tests/cert.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub lib/proc.cub lib/session.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub lib/proc.cub lib/choreo.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/fo.cub lib/match.cub lib/check.cub lib/std.cub lib/stage.cub lib/lower.cub lib/specialize.cub lib/proc.cub lib/choreo.cub tests/choreo-spec.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/match.cub lib/nav.cub lib/cost.cub
	sh tests/agda/check.sh
	./build/cubical lib/prelude.cub lib/code.cub lib/fo.cub lib/match.cub lib/nav.cub lib/emitc.cub lib/check.cub lib/std.cub lib/stage.cub lib/lower.cub lib/specialize.cub lib/linproof.cub lib/prim.cub lib/decode.cub lib/rules.cub lib/derive.cub lib/proc.cub lib/emitt.cub lib/emitu.cub lib/repr.cub lib/image.cub lib/reimport.cub lib/emitx86.cub tests/fo.cub
	# the driver loop (D26): a process over W writes hello and ends with the count written
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub lib/linproof.cub lib/prim.cub lib/proc.cub lib/world.cub tests/world.cub > build/world.out && grep -aq "^hello" build/world.out && grep -aq "run: done #8" build/world.out && grep -aq "run: done #0" build/world.out && grep -aq "^hello" /tmp/cubw.txt
	# the call boundary (B18): the net enters a definition's compiled C (#native), and a C program calls the net
	./build/cubical lib/prelude.cub lib/code.cub lib/fo.cub lib/match.cub lib/nav.cub lib/emitc.cub lib/check.cub lib/std.cub lib/stage.cub lib/lower.cub lib/specialize.cub lib/linproof.cub lib/prim.cub lib/decode.cub lib/rules.cub lib/derive.cub lib/proc.cub lib/emitt.cub lib/emitu.cub lib/repr.cub tests/native.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/fo.cub lib/match.cub lib/nav.cub lib/emitc.cub lib/check.cub lib/std.cub lib/stage.cub lib/lower.cub lib/specialize.cub lib/linproof.cub lib/prim.cub lib/decode.cub lib/rules.cub lib/derive.cub lib/proc.cub lib/emitt.cub lib/emitu.cub lib/repr.cub tests/native.cub 2>&1 | grep -q "specialization cached by key" || { echo "native: a second run did not hit the specialization cache"; exit 1; }
	./build/cubical lib/prelude.cub lib/code.cub lib/fo.cub lib/match.cub lib/nav.cub lib/emitc.cub lib/check.cub lib/std.cub lib/stage.cub lib/lower.cub lib/specialize.cub lib/linproof.cub lib/prim.cub lib/decode.cub lib/rules.cub lib/derive.cub lib/proc.cub lib/emitt.cub lib/emitu.cub lib/repr.cub tests/native.cub 2>&1 | grep -q "native at run: scaleat at #3   \[specialization cached by key\]" || { echo "native at: a second run did not hit the cache"; exit 1; }
	CUBICAL_WORKERS=4 CUBICAL_NATIVE_AT_MAX=4 ./build/cubical lib/prelude.cub lib/code.cub lib/fo.cub lib/match.cub lib/nav.cub lib/emitc.cub lib/check.cub lib/std.cub lib/stage.cub lib/lower.cub lib/specialize.cub lib/emitt.cub lib/emitu.cub tests/native-at.cub
	$(MAKE) -s build/native-main && ./build/native-main
	# the store (F34): a fresh one, run twice: the second run checks nothing, the answers agree, renaming changes no key
	rm -rf build/store; for r in 1 2; do CUBICAL_STORE=build/store ./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub tests/store.cub > build/store-$$r.out || exit 1; done; \
	grep -q "ok: checks .*\[[0-9]* steps\]" build/store-2.out && { echo "store: the second run checked a definition"; grep "ok: checks .*steps" build/store-2.out | head; exit 1; }; \
	for r in 1 2; do sed 's/ *\[[0-9]* steps[^]]*\]//; s/ *\[store\]//; /^[0-9]* passed/d' build/store-$$r.out > build/store-$$r.cmp; done; \
	cmp -s build/store-1.cmp build/store-2.cmp || { echo "store: the two runs differ"; diff build/store-1.cmp build/store-2.cmp | head; exit 1; }; \
	[ "$$(grep '^key twice = ' build/store-2.out | cut -d= -f2)" = "$$(grep '^key double = ' build/store-2.out | cut -d= -f2)" ] || { echo "store: renaming changed a key"; exit 1; }; \
	[ "$$(grep '^key twice-zero = ' build/store-2.out | cut -d= -f2)" != "$$(grep "^key twice-zero' = " build/store-2.out | cut -d= -f2)" ] || { echo "store: a different proof kept its key"; exit 1; }; \
	echo "store: the second run checked nothing; keys as expected"
	# --serve: blocks ended by `.`, #retract hides a checked definition so the name is declared again (item 30)
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub --serve < tests/serve/retract.in > build/serve.out 2>&1; \
	grep -q "^retracted twoN" build/serve.out && [ "$$(grep -c '^ok: ' build/serve.out)" -ge 5 ] && ! grep -q "^FAIL\|^error" build/serve.out || { echo "serve: retraction failed"; tail build/serve.out; exit 1; }; echo "serve: retraction ok"
	python3 tests/lsp/test_lsp.py
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub --serve < tests/serve/data.in > build/serve-data.out 2>&1; grep -q "^retracted data Col red green" build/serve-data.out && grep -q "^type: isRed : Col → Bool" build/serve-data.out && grep -q "^where: plus .*lib/std.cub:" build/serve-data.out && ! grep -q "^FAIL" build/serve-data.out || { echo "serve: data retraction failed"; exit 1; }
	# Brunerie's number (Ljungström's brunerie', an untyped port): -2, with one and with four workers
	for w in 0 4; do CUBICAL_WORKERS=$$w ./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub \
	  experiments/brunerie/brunerie.cub experiments/brunerie/run-brunerie.cub | grep -A1 -x brunerie | grep -q "= negsuc (suc zero) " \
	  || { echo "brunerie: not -2 with $$w workers"; exit 1; }; done; echo "brunerie = negsuc (suc zero)"
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub experiments/brunerie/brunerie.cub \
	  experiments/brunerie/run-squash.cub | tail -1 | grep -q " 0 failed"
	# the typed port: every definition checked (the recursors and surfc are primitives), and -2
	for w in 0 4; do CUBICAL_WORKERS=$$w ./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub \
	  experiments/brunerie/typed.cub experiments/brunerie/run-typed.cub > build/typed-$$w.out; \
	  tail -1 build/typed-$$w.out | grep -q " 0 failed" || { echo "typed brunerie: a check failed with $$w workers"; grep -A3 ^FAIL build/typed-$$w.out | head; exit 1; }; \
	  grep -A1 -x brunerie build/typed-$$w.out | grep -q "= negsuc (suc zero) " || { echo "typed brunerie: not -2 with $$w workers"; exit 1; }; done; echo "typed brunerie = negsuc (suc zero)"
	rm -rf build/cache-test
	env -u CUBICAL_NOCACHE CUBICAL_CACHE=build/cache-test ./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub tests/mod/main.cub | tail -1
	env -u CUBICAL_NOCACHE CUBICAL_CACHE=build/cache-test ./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub tests/mod/main.cub | grep -c "^cached: " | grep -qx 6
	# without the memo, the step counts (every reading's, and the totals) and the answers do not depend
	# on the number of workers; with it, the answers do not (what a reading finds already computed
	# depends on the order of the rewrites, so its count can)
	set -e; for s in "tests/cubical.cub" "lib/code.cub tests/code.cub" "tests/hit.cub" "tests/memo.cub" \
	    "lib/code.cub lib/check.cub tests/check.cub" "lib/code.cub lib/check.cub lib/std.cub tests/reject.cub tests/quote.cub" \
	    "lib/code.cub lib/check.cub lib/std.cub tests/codata.cub" "lib/code.cub lib/check.cub lib/std.cub tests/guarded.cub" \
	    "lib/code.cub lib/check.cub lib/std.cub lib/prim.cub lib/stage.cub tests/prim.cub"; do \
	  CUBICAL_NOMEMO=1 CUBICAL_WORKERS=0 ./build/cubical lib/prelude.cub $$s > build/workers-0.out; \
	  for w in 1 2 4 8 16; do CUBICAL_NOMEMO=1 CUBICAL_WORKERS=$$w ./build/cubical lib/prelude.cub $$s > build/workers-$$w.out; \
	    cmp -s build/workers-0.out build/workers-$$w.out || { echo "differs with $$w workers: $$s"; diff build/workers-0.out build/workers-$$w.out | head; exit 1; }; done; \
	  CUBICAL_WORKERS=0 ./build/cubical lib/prelude.cub $$s | sed 's/ *\[[0-9]* steps[^]]*\]//; /^[0-9]* passed/d' > build/workers-0.out; \
	  for w in 1 2 4 8 16; do CUBICAL_WORKERS=$$w ./build/cubical lib/prelude.cub $$s | sed 's/ *\[[0-9]* steps[^]]*\]//; /^[0-9]* passed/d' > build/workers-$$w.out; \
	    cmp -s build/workers-0.out build/workers-$$w.out || { echo "answers differ with $$w workers: $$s"; diff build/workers-0.out build/workers-$$w.out | head; exit 1; }; done; \
	done; echo "every suite: identical output with 0, 1, 2, 4, 8, 16 workers without the memo; identical answers with it"
	ASAN_OPTIONS=detect_leaks=0 ./build/cubical-asan lib/prelude.cub tests/cubical.cub > /dev/null
	ASAN_OPTIONS=detect_leaks=0 ./build/cubical-asan lib/prelude.cub lib/code.cub tests/code.cub > /dev/null
	ASAN_OPTIONS=detect_leaks=0 ./build/cubical-asan lib/prelude.cub tests/hit.cub > /dev/null
	ASAN_OPTIONS=detect_leaks=0 ./build/cubical-asan lib/prelude.cub lib/code.cub lib/check.cub tests/check.cub > /dev/null
	ASAN_OPTIONS=detect_leaks=0 ./build/cubical-asan lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub tests/quote.cub > /dev/null
	ASAN_OPTIONS=detect_leaks=0 CUBICAL_WORKERS=4 ./build/cubical-asan lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub tests/codata.cub > /dev/null
	ASAN_OPTIONS=detect_leaks=0 CUBICAL_WORKERS=4 ./build/cubical-asan lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub tests/guarded.cub > /dev/null
	ASAN_OPTIONS=detect_leaks=0 CUBICAL_NOCACHE=1 ./build/cubical-asan lib/prelude.cub lib/code.cub lib/fo.cub lib/match.cub lib/nav.cub lib/emitc.cub lib/check.cub lib/std.cub lib/stage.cub lib/lower.cub lib/specialize.cub lib/linproof.cub lib/prim.cub lib/decode.cub lib/rules.cub lib/derive.cub lib/proc.cub lib/emitt.cub lib/emitu.cub lib/repr.cub tests/native.cub > build/native-asan.out 2>&1 || { echo "native under ASan failed"; tail -5 build/native-asan.out; exit 1; }

# the checked library and the acceptance tests (the paper's terms, 9.15's sorting): long, run once
library: export CUBICAL_NOCACHE = 1
library: build/cubical
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub tests/boxes.cub
	./build/cubical lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub tests/sort.cub
	./build/cubical lib/prelude.cub tests/memo.cub

build/cubical-tsan: $(SRC) src/core.h src/syntax.c
	@mkdir -p build
	$(CC) -std=gnu11 -O1 -g -fsanitize=thread -o $@ $(SRC) -lpthread

tsan: build/cubical-tsan
	CUBICAL_WORKERS=4 TSAN_OPTIONS=halt_on_error=1 ./build/cubical-tsan lib/prelude.cub tests/cubical.cub > /dev/null
	CUBICAL_WORKERS=4 TSAN_OPTIONS=halt_on_error=1 ./build/cubical-tsan lib/prelude.cub lib/code.cub tests/code.cub > /dev/null
	CUBICAL_WORKERS=4 TSAN_OPTIONS=halt_on_error=1 ./build/cubical-tsan lib/prelude.cub tests/hit.cub > /dev/null
	CUBICAL_WORKERS=4 TSAN_OPTIONS=halt_on_error=1 ./build/cubical-tsan lib/prelude.cub lib/code.cub lib/check.cub tests/check.cub > /dev/null
	CUBICAL_WORKERS=4 TSAN_OPTIONS=halt_on_error=1 ./build/cubical-tsan lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub tests/quote.cub > /dev/null

clean:
	rm -rf build
