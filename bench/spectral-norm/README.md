# spectral-norm

Approximate the largest singular value (the spectral norm) of the infinite
matrix

```
A(i,j) = 1 / ((i+j)(i+j+1)/2 + i + 1)          (i, j ≥ 0)
```

by the power method: starting from the all-ones vector, apply Aᵀ·A ten times
(twenty matrix–vector products), then report `sqrt((uᵀ·v)/(vᵀ·v))` formatted to
nine decimal places.

## Pinned input

`DEFAULT_N = 5500` (the Computer Language Benchmarks Game's canonical size). The
reference output is in `expected.txt`:

```
1.274224153
```

Run a different size with `N=<int> scripts/run.sh spectral-norm`.

## What it stresses

Tight nested floating-point loops with a per-element reciprocal — the inner term
is a `double` division. It is one of the sharpest probes of the native↔LLVM code-
quality gap: LLVM's `-O2` vectorizes and schedules this loop aggressively, so the
`binate-native` vs `binate-llvm` rows show how much of that the native backend
still leaves on the table.

## Implementation notes

Every implementation is single-threaded and scalar, evaluates `A(i,j)` the same
way, and accumulates in the same order, so all outputs agree to the last figure
(modulo a permitted last-ULP floating-point tolerance). `A(i,j)`'s denominator
is computed in integer arithmetic — `(i+j)(i+j+1)/2` is always exact — before the
single `double` reciprocal.
