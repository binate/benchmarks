# Binate benchmarks

Cross-language performance benchmarks for the [Binate](https://github.com/binate)
programming language.

Each benchmark is one well-specified problem, implemented in Binate and in a set
of peer languages (C, C++, Rust, Go, Java, Python). The harness builds, runs,
times, and cross-checks every implementation.

The point is the two Binate rows: the same Binate source compiled by the
**native** backend and by the **LLVM** backend. Binate's native backend is the
real code generator; the LLVM backend is a stopgap. Where native-generated code
runs slower than LLVM-generated code for the same program, that gap is a
native-backend defect to close, and these benchmarks measure it directly. The
peer languages are the external yardstick.

## Layout

```
bench/<name>/
  README.md        the problem spec: what it computes, the pinned input, what it stresses
  config.sh        pinned parameters (DEFAULT_N, the Java main class, …)
  expected.txt     reference output at the pinned input
  binate/          Binate entry (cmd/<name>/main.bn) — built by both backends
  c/  cpp/  rust/  go/  java/  python/   one peer implementation each
scripts/
  run.sh           the harness
  fetch-builder.sh resolves the pinned Binate toolchain
  lib.sh           timing + Binate dual-backend build helpers
```

## Running

```sh
scripts/run.sh spectral-norm                 # every language
scripts/run.sh spectral-norm binate-native binate-llvm c   # a subset
```

Language names: `binate-native`, `binate-llvm`, `c`, `cpp`, `rust`, `go`,
`java`, `python`. A peer whose toolchain is not installed is reported and
skipped, not an error.

Environment knobs:

- `N=<int>` — problem size (default: the benchmark's `DEFAULT_N`).
- `ROUNDS=<int>` — timed rounds; best and median are reported (default 5, after
  one warmup).
- `BUILDER_VERSION` / `BINATE_BUNDLE` — select the Binate toolchain (see below).

## Toolchain

The Binate entries build against a released Binate bundle, pinned in
`BUILDER_VERSION` and fetched/cached on demand by `scripts/fetch-builder.sh`
(the same mechanism the examples repo uses). The native and LLVM backends are
selected with `bnc --backend native -O2` and `bnc --backend llvm -O2 --cflag
-O2`. Set `BINATE_BUNDLE=<dir>` to run against a toolchain built from a `main`
checkout instead of a release. The LLVM backend needs `clang` on `PATH`.

Peer toolchains (`cc`, `c++`, `rustc`, `go`, `javac`/`java`, `python3`) are used
if present.

## Methodology and fairness

- **Same algorithm, same order.** Every implementation is single-threaded,
  scalar, and uses the same operation order, so the outputs agree and the
  numbers reflect code quality rather than a cleverer algorithm in one language.
  (The Computer Language Benchmarks Game's fastest entries parallelize and hand-
  vectorize; that is a different contest.)
- **Outputs are cross-checked.** The harness fails if any implementation's
  result disagrees with the reference (C) beyond a 1e-8 floating-point
  tolerance — last-ULP differences (e.g. fused multiply-add on one target only)
  are allowed; real divergence is not.
- **Python is the floor**, present for scale, not to be competitive.

## Provenance and licence

The implementations are written from each benchmark's public problem
specification (the Computer Language Benchmarks Game defines the problems); they
are not copied from that project's licensed source. Licensed under MIT (see
`LICENSE`).
