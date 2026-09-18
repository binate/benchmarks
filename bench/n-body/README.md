# n-body

Simulate the Sun and the four gas giants under Newtonian gravity with a
symplectic leapfrog integrator at fixed `dt = 0.01`, and report the total system
energy before and after `N` steps to nine decimal places.

## Pinned input

`DEFAULT_N = 5000000` (Computer Language Benchmarks Game input size). Reference
output (`expected.txt`):

```
-0.169075164
-0.169083134
```

The first line (initial energy) is independent of `N`; the canonical value is
`-0.169075164`.

## What it stresses

Scalar floating-point with loop-carried dependencies over a tiny fixed body
count — pairwise force accumulation and a reciprocal square root per pair.

## Implementation notes

All implementations are single-threaded and evaluate the pairwise interactions in
the same order, so outputs agree to a permitted 1e-8 tolerance. The Binate entry
holds the bodies in a `@[]Body` struct-of-bodies and updates fields in place
through the slice index.
