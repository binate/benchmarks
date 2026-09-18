#!/bin/sh
# run.sh <bench> [lang ...]
#
# Build, run, time, and cross-check every language implementation of a
# benchmark under bench/<bench>/. Each implementation is a single-threaded,
# identical-order port of the same problem spec, so all outputs must agree; the
# harness fails if any language's output diverges from the reference (C).
#
# The point is the two Binate rows — `binate-native` vs `binate-llvm` — the same
# Binate source compiled by both backends. The peer languages (c, cpp, rust, go,
# java, python) are the yardstick.
#
# Options come from the environment:
#   N=<int>        problem size (default: the benchmark's DEFAULT_N)
#   ROUNDS=<int>   timed rounds; best + median reported (default 5, after 1 warmup)
#   BUILDER_VERSION / BINATE_BUNDLE  passed through to fetch-builder.sh
#
# A missing peer toolchain is reported and skipped, not an error.
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
. "$SCRIPT_DIR/lib.sh"

[ $# -ge 1 ] || { echo "usage: $0 <bench> [lang ...]" >&2
    echo "benchmarks:" >&2; ls "$REPO_DIR/bench" >&2; exit 2; }
BENCH="$1"; shift
BENCHDIR="$REPO_DIR/bench/$BENCH"
[ -d "$BENCHDIR" ] || { echo "no such benchmark: $BENCH" >&2; exit 2; }

ALL_LANGS="binate-native binate-llvm c cpp rust go java python"
LANGS="${*:-$ALL_LANGS}"

# Pinned problem parameters live beside the sources.
DEFAULT_N=""
. "$BENCHDIR/config.sh"
: "${N:=$DEFAULT_N}"
: "${ROUNDS:=5}"

WORK="$(mktemp -d "${TMPDIR:-/tmp}/bench_XXXXXX")"
trap 'rm -rf "$WORK"' EXIT INT TERM
binate_resolve

# --- per-language build: sets CMD_PRE (prefix words) + CMD_BIN (program/arg).
# Returns non-zero (and prints a reason) when the toolchain is unavailable.
have() { command -v "$1" >/dev/null 2>&1; }
build_lang() {
    CMD_PRE=""; CMD_BIN=""
    case "$1" in
        binate-native) CMD_BIN="$WORK/bn_native"
            binate_build native "$BENCHDIR/binate" "$BENCHDIR/binate/cmd/$BENCH" "$CMD_BIN" ;;
        binate-llvm)   CMD_BIN="$WORK/bn_llvm"
            binate_build llvm   "$BENCHDIR/binate" "$BENCHDIR/binate/cmd/$BENCH" "$CMD_BIN" ;;
        c)    have cc    || return 3; CMD_BIN="$WORK/c_bin";   cc  -O2 -ffp-contract=off -o "$CMD_BIN" "$BENCHDIR/c/$BENCH.c" -lm ;;
        cpp)  have c++   || return 3; CMD_BIN="$WORK/cpp_bin"; c++ -O2 -ffp-contract=off -std=c++17 -o "$CMD_BIN" "$BENCHDIR/cpp/$BENCH.cpp" ;;
        rust) have rustc || return 3; CMD_BIN="$WORK/rust_bin"; rustc -O -o "$CMD_BIN" "$BENCHDIR/rust/$BENCH.rs" ;;
        go)   have go    || return 3; CMD_BIN="$WORK/go_bin"
              ( cd "$BENCHDIR/go" && go build -o "$CMD_BIN" . ) ;;
        java) javac -version >/dev/null 2>&1 || return 3
              javac -d "$WORK/java" "$BENCHDIR/java/"*.java
              CMD_PRE="java -cp $WORK/java"; CMD_BIN="$JAVA_MAIN" ;;
        python) have python3 || return 3; CMD_PRE="python3"; CMD_BIN="$BENCHDIR/python/$BENCH.py" ;;
        *) echo "unknown language: $1" >&2; return 2 ;;
    esac
}

# Time one run of the built command on problem size N; stdout -> $1.
run_once() { $CMD_PRE "$CMD_BIN" "$N" > "$1" 2>/dev/null; }

# Numeric agreement: true when |a-b| <= 1e-8. Cross-language floating-point
# output can differ in the last ULP (e.g. FMA contraction on one target but not
# another), so exact string match would be too strict; a real algorithm error
# moves the value far more than 1e-8.
fpeq() { awk -v a="$1" -v b="$2" 'BEGIN{d=a-b; if(d<0)d=-d; exit !(d<=1e-8)}'; }

echo "=== $BENCH  (N=$N, rounds=$ROUNDS) ==="
GOLD=""
printf "%-14s %10s %10s   %s\n" "language" "best(s)" "median(s)" "output"
for lang in $LANGS; do
    if build_lang "$lang" 2>"$WORK/err"; then rc=0; else rc=$?; fi
    if [ "$rc" -ne 0 ]; then
        if [ "$rc" -eq 3 ]; then printf "%-14s %10s %10s   %s\n" "$lang" "-" "-" "(toolchain missing — skipped)"
        else printf "%-14s %10s %10s   BUILD FAILED\n" "$lang" "-" "-"; sed 's/^/    /' "$WORK/err" >&2; fi
        continue
    fi
    run_once "$WORK/out.$lang" || { printf "%-14s %10s %10s   RUN FAILED\n" "$lang" "-" "-"; continue; }
    out="$(cat "$WORK/out.$lang")"
    [ -z "$GOLD" ] && GOLD="$out" && GOLD_LANG="$lang"
    ok="ok"; fpeq "$out" "$GOLD" || ok="MISMATCH (want '$GOLD')"
    # Warmup + timed rounds.
    run_once "$WORK/out.$lang" || true
    TS=""; r=1
    while [ "$r" -le "$ROUNDS" ]; do
        t0="$(now)"; run_once "$WORK/out.$lang"; t1="$(now)"
        TS="$TS $(delta "$t0" "$t1")"; r=$((r + 1))
    done
    printf "%-14s %10s %10s   %s  %s\n" "$lang" "$(minof "$TS")" "$(medianof "$TS")" "$out" "$ok"
done

# Pin check: at the canonical size the reference must match the recorded value.
if [ -n "$GOLD" ] && [ "$N" = "$DEFAULT_N" ] && [ -f "$BENCHDIR/expected.txt" ]; then
    want="$(cat "$BENCHDIR/expected.txt")"
    if ! fpeq "$GOLD" "$want"; then
        echo "PIN MISMATCH: reference ($GOLD_LANG) produced '$GOLD', expected.txt says '$want'" >&2
        exit 1
    fi
    echo "pin: reference output matches expected.txt ($want)"
fi
