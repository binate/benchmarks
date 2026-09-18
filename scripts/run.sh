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

# Pinned problem parameters live beside the sources. COMPARE selects how outputs
# are cross-checked:
#   float  (default) — line-by-line with a 1e-8 tolerance (floating-point output)
#   exact            — byte-identical (deterministic integer/text output)
#   bytes            — byte-identical, shown as a byte count (binary output)
# DEFAULT_N is the canonical (pin/timing) size; SMALL_N is a safe size for the
# correctness cross-check — for benchmarks where N is a tree depth or permutation
# size, the canonical N is far too large to run every language on.
DEFAULT_N=""
SMALL_N=""
COMPARE=float
. "$BENCHDIR/config.sh"
# CHECK=1 selects the small correctness size; otherwise the canonical size. An
# explicit N in the environment overrides either.
if [ -z "$N" ]; then
    if [ -n "$CHECK" ]; then N="${SMALL_N:-$DEFAULT_N}"; else N="$DEFAULT_N"; fi
fi
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
        rust) if have rustc; then RUSTC="rustc"           # rustc on PATH (e.g. CI)
              elif have rustup; then RUSTC="rustup run stable rustc"  # rustup without cargo/bin on PATH
              else return 3; fi
              CMD_BIN="$WORK/rust_bin"; $RUSTC -O -o "$CMD_BIN" "$BENCHDIR/rust/$BENCH.rs" ;;
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

# outputs_agree <file-a> <file-b>: do two runs' outputs match under COMPARE?
#   bytes/exact — byte-identical.
#   float  — same number of lines, each pair of values within 1e-8. Cross-
#            language floating-point output can differ in the last ULP (e.g. FMA
#            contraction on one target but not another), so exact match would be
#            too strict; a real algorithm error moves a value far more than that.
outputs_agree() {
    case "$COMPARE" in
        bytes|exact) cmp -s "$1" "$2" ;;
        *) awk 'NR==FNR{a[FNR]=$0; na=FNR; next} {b[FNR]=$0; nb=FNR}
                END{ if (na != nb) exit 1
                     for (i = 1; i <= na; i++) { d = a[i]-b[i]; if (d<0) d=-d
                         if (!(d <= 1e-8)) exit 1 }
                     exit 0 }' "$1" "$2" ;;
    esac
}

# disp_of <file>: a compact one-line summary of a run's output for the table.
disp_of() {
    if [ "$COMPARE" = bytes ]; then echo "$(wc -c < "$1" | tr -d ' ') bytes"; return; fi
    _n="$(wc -l < "$1" | tr -d ' ')"
    if [ "$_n" -le 1 ]; then head -1 "$1"; else echo "$(head -1 "$1") … ($_n lines)"; fi
}

echo "=== $BENCH  (N=$N, rounds=$ROUNDS) ==="
GOLD_FILE=""   # the reference (first successful) run's output; others must agree
FAIL=0         # real failures (build/run error or output mismatch); skips don't count
printf "%-14s %10s %10s   %s\n" "language" "best(s)" "median(s)" "output"
for lang in $LANGS; do
    if build_lang "$lang" 2>"$WORK/err"; then rc=0; else rc=$?; fi
    if [ "$rc" -ne 0 ]; then
        if [ "$rc" -eq 3 ]; then printf "%-14s %10s %10s   %s\n" "$lang" "-" "-" "(toolchain missing — skipped)"
        else printf "%-14s %10s %10s   BUILD FAILED\n" "$lang" "-" "-"; sed 's/^/    /' "$WORK/err" >&2; FAIL=$((FAIL + 1)); fi
        continue
    fi
    run_once "$WORK/out.$lang" || { printf "%-14s %10s %10s   RUN FAILED\n" "$lang" "-" "-"; FAIL=$((FAIL + 1)); continue; }
    if [ -z "$GOLD_FILE" ]; then cp "$WORK/out.$lang" "$WORK/gold"; GOLD_FILE="$WORK/gold"; GOLD_LANG="$lang"; fi
    ok="ok"; outputs_agree "$WORK/out.$lang" "$GOLD_FILE" || { ok="MISMATCH (vs $GOLD_LANG)"; FAIL=$((FAIL + 1)); }
    # Warmup + timed rounds.
    run_once "$WORK/out.$lang" || true
    TS=""; r=1
    while [ "$r" -le "$ROUNDS" ]; do
        t0="$(now)"; run_once "$WORK/out.$lang"; t1="$(now)"
        TS="$TS $(delta "$t0" "$t1")"; r=$((r + 1))
    done
    printf "%-14s %10s %10s   %s  %s\n" "$lang" "$(minof "$TS")" "$(medianof "$TS")" "$(disp_of "$WORK/out.$lang")" "$ok"
done

# Pin check: at the canonical size the reference must match the recorded output.
if [ -n "$GOLD_FILE" ] && [ "$N" = "$DEFAULT_N" ] && [ -f "$BENCHDIR/expected.txt" ]; then
    if ! outputs_agree "$GOLD_FILE" "$BENCHDIR/expected.txt"; then
        echo "PIN MISMATCH: reference ($GOLD_LANG) output does not match expected.txt" >&2
        exit 1
    fi
    echo "pin: reference ($GOLD_LANG) output matches expected.txt"
fi

# Non-zero exit on any real failure (build error, run error, output mismatch) so
# the harness is usable directly in CI. A skipped-because-missing toolchain is
# not a failure.
[ "$FAIL" -eq 0 ] || { echo "$FAIL implementation(s) failed" >&2; exit 1; }
