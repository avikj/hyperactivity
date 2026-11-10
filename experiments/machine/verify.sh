#!/bin/bash
# full verification for track N; run from scoped-machine
cd /home/claude/work-N/scoped-machine || exit 1
ulimit -v 3000000
make build/cubical 2>&1 | grep -E 'error' 
echo "library: $( { timeout 900 make library 2>&1; } | tail -2 | tr '\n' ' ')"
L=$(make -s -n library | sed -n 's/^.*cubical \(lib\/prelude.*\)$/\1/p')
echo "jit ok count: $(CUBICAL_NOCACHE=1 timeout 900 ./build/cubical $L experiments/jit/workloads.cub | grep -c '^ok: jit')"
CUBICAL_NOCACHE=1 timeout 900 ./build/cubical $L experiments/jit/workloads.cub | tail -1
CUBICAL_NOCACHE=1 timeout 900 ./build/cubical $L experiments/native/feed.cub | grep -E 'build|passed'
printf '\x02\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x05\0\0\0\0\0\0\0\x01\0\0\0\0\0\0\0\x03\0\0\0\0\0\0\0' | ./build/feed | od -An -td8
timeout 900 make brunerie 2>&1 | tail -1
