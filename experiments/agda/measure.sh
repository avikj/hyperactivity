#!/usr/bin/env bash
# PLAN item 10: Brunerie's number (Ljungström's brunerie') in Agda and in this machine, on the
# same machine, with wall time, CPU time (user+system, all threads) and peak RSS, both polled
# from /proc every 50 ms (VmHWM; utime+stime from stat). Other agents share the 2 cores, so wall
# times vary run to run; CPU time is the steadier figure, and the load average is logged per run.
#
#   experiments/agda/measure.sh setup      # Agda 2.8.0 binary and agda/cubical into $A (default /tmp/agda)
#   experiments/agda/measure.sh agda       # cold library check, BrunerieExperiments, warm load and refl
#   experiments/agda/measure.sh machine    # untyped and typed ports at CUBICAL_WORKERS=0,1,2
#   experiments/agda/measure.sh run LABEL CMD...   # one measured run (used by the two above)
#
# Run from the repository root. Every run goes through the shared memory gate (heavy.sh; --big for Agda).
# Caps: 3 hours wall, 7 GB RSS; a run that hits one is killed and reported as such.
set -u
A=${A:-/tmp/agda}
HEAVY=${HEAVY:-/home/claude/avikj/heavy.sh}
AGDA_VER=v2.8.0                                   # agda/cubical's CI (AGDA_BRANCH) at the commit below
CUBICAL_REV=${CUBICAL_REV:-c37782bb4e382dc41035e0970409a0c17b55e6ce}
LOG=${LOG:-$A/logs/measure.tsv}
SELF=$(readlink -f "$0")
CAP_S=10800; CAP_KB=$((7*1024*1024))

case "${1:-}" in
run)
  label=$2; shift 2
  start=$(date +%s.%N)
  "$@" > "$A/logs/$label.out" 2>&1 & pid=$!
  hwm=0; cpu=0; capped=""; tck=$(getconf CLK_TCK)
  while kill -0 $pid 2>/dev/null; do
    h=$(awk '/^VmHWM/{print $2}' /proc/$pid/status 2>/dev/null); [ -n "$h" ] && hwm=$h
    r=$(awk '/^VmRSS/{print $2}' /proc/$pid/status 2>/dev/null)
    c=$(sed 's/.*) //' /proc/$pid/stat 2>/dev/null | awk '{print $12+$13+$14+$15}'); [ -n "$c" ] && cpu=$c
    now=$(date +%s.%N)
    if [ -n "$r" ] && [ "$r" -gt $CAP_KB ]; then capped="rss-cap"; kill -9 $pid; fi
    if [ "$(echo "$now - $start > $CAP_S" | bc)" = 1 ]; then capped="time-cap"; kill -9 $pid; fi
    sleep 0.05
  done
  wait $pid; rc=$?
  end=$(date +%s.%N)
  wall=$(echo "$end - $start" | bc)
  printf '%s\t%.2f s wall\t%.2f s cpu\t%d MB\trc=%s\t%s\tload %s\n' "$label" "$wall" "$(echo "$cpu / $tck" | bc -l)" \
    $((hwm/1024)) $rc "${capped:-ok}" \
    "$(cut -d' ' -f1-3 /proc/loadavg)" | tee -a "$LOG"
  ;;
setup)
  mkdir -p "$A/dl" "$A/bin" "$A/agdadir" "$A/xdg" "$A/work" "$A/logs"
  if [ ! -x "$A/bin/agda" ]; then
    curl -sSL -o "$A/dl/Agda-$AGDA_VER-linux.tar.xz" \
      "https://github.com/agda/agda/releases/download/$AGDA_VER/Agda-$AGDA_VER-linux.tar.xz"
    tar -xJf "$A/dl/Agda-$AGDA_VER-linux.tar.xz" -C "$A/bin" agda
  fi
  [ -d "$A/cubical/.git" ] || git clone -q https://github.com/agda/cubical "$A/cubical"
  git -C "$A/cubical" checkout -q "$CUBICAL_REV"
  echo "$A/cubical/cubical.agda-lib" > "$A/agdadir/libraries"
  echo "cubical-0.9" > "$A/agdadir/defaults"
  printf 'name: work\ninclude: .\ndepend: cubical-0.9\nflags: --safe --cubical --no-import-sorts -WnoUnsupportedIndexedMatch --guardedness\n' \
    > "$A/work/work.agda-lib"
  # every module BrunerieExperiments imports: checking this is the library part alone
  { echo "module Deps where"
    grep '^open import' "$A/cubical/Cubical/Homotopy/Group/Pi4S3/BrunerieExperiments.agda" \
      | awk '{print "import " $3}'; } > "$A/work/Deps.agda"
  printf 'module Load where\nopen import Cubical.Homotopy.Group.Pi4S3.BrunerieExperiments\n' > "$A/work/Load.agda"
  printf 'module Refl where\nopen import Cubical.Foundations.Prelude\nopen import Cubical.Data.Int\nopen import Cubical.Homotopy.Group.Pi4S3.BrunerieExperiments\nagain : brunerie'"'"' ≡ -2\nagain = refl\n' > "$A/work/Refl.agda"
  "$A/bin/agda" --version
  ;;
agda)
  export AGDA_DIR=$A/agdadir XDG_DATA_HOME=$A/xdg
  BE=$A/cubical/Cubical/Homotopy/Group/Pi4S3/BrunerieExperiments.agda
  cd "$A/work" || exit 1
  ag() { "$HEAVY" --big "$SELF" run "$@"; }
  # end to end from nothing: the library BrunerieExperiments needs, the module, and its
  # brunerie'≡-2 = refl (which normalizes brunerie')
  for i in 1 2; do
    rm -rf "$A/cubical/_build" "$A/work/_build" "$A"/xdg/agda/*/lib/prim/_build
    ag agda-cold-all-$i "$A/bin/agda" "$BE"
  done
  # the same split in two: the imports alone, then the module on top of their interfaces
  rm -rf "$A/cubical/_build" "$A/work/_build" "$A"/xdg/agda/*/lib/prim/_build
  ag agda-cold-deps "$A/bin/agda" "$A/work/Deps.agda"
  ag agda-module "$A/bin/agda" --profile=definitions "$BE"
  # interfaces present: loading only, then loading and brunerie' ≡ -2 by refl once more
  for i in 1 2 3; do
    rm -rf "$A/work/_build"; ag agda-warm-load-$i "$A/bin/agda" "$A/work/Load.agda"
    rm -rf "$A/work/_build"; ag agda-warm-refl-$i "$A/bin/agda" "$A/work/Refl.agda"
  done
  # Agda's own account of where the time goes in the refl module (the definition `again` is the number)
  rm -rf "$A/work/_build"; ag agda-warm-refl-profile "$A/bin/agda" --profile=definitions "$A/work/Refl.agda"
  ;;
machine)
  L="lib/prelude.cub lib/code.cub lib/check.cub lib/std.cub"
  B=experiments/brunerie
  for w in 0 1 2; do for i in 1 2 3; do
    CUBICAL_NOCACHE=1 CUBICAL_WORKERS=$w "$HEAVY" "$SELF" run machine-untyped-w$w-$i \
      ./build/cubical $L $B/typed.cub $B/run-brunerie.cub
    CUBICAL_NOCACHE=1 CUBICAL_WORKERS=$w "$HEAVY" "$SELF" run machine-typed-w$w-$i \
      ./build/cubical $L $B/typed.cub $B/run-typed.cub
  done; done
  # the analogue of Agda with interfaces present: the library from the module cache and the store
  # (filled by one run), the number read afresh (the driver copied with a nonce, so it misses the cache)
  rm -rf "$A/cubical-cache" "$A/cubical-store"
  C="env -u CUBICAL_NOCACHE CUBICAL_CACHE=$A/cubical-cache CUBICAL_STORE=$A/cubical-store"
  $C CUBICAL_WORKERS=0 "$HEAVY" "$SELF" run machine-untyped-cache-fill \
    ./build/cubical $L $B/typed.cub $B/run-brunerie.cub
  for w in 0 1 2; do for i in 1 2 3; do
    d=$A/work/run-brunerie-$w-$i.cub; { cat $B/run-brunerie.cub; echo "-- $(date +%s%N)"; } > "$d"
    $C CUBICAL_WORKERS=$w "$HEAVY" "$SELF" run machine-untyped-warm-w$w-$i ./build/cubical $L $B/typed.cub "$d"
  done; done
  # the cached library alone (no driver): warm minus this is the reading of the number
  for i in 1 2 3; do
    $C CUBICAL_WORKERS=0 "$HEAVY" "$SELF" run machine-untyped-cachedonly-$i ./build/cubical $L $B/typed.cub
  done
  ;;
*) sed -n 2,13p "$SELF"; exit 2 ;;
esac
