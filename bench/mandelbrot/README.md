# mandelbrot

Render the Mandelbrot set over the region [-1.5, 0.5] × [-1, 1] as an `N`×`N`
image, 50 escape iterations per pixel with escape radius 2, and emit it as a P4
(raw 1-bit-per-pixel) PBM bitmap — MSB-first bit packing, rows padded to a byte
boundary.

## Pinned input

`DEFAULT_N = 1000` → a 125 KB bitmap (`expected.txt` is the reference bitmap).

## What it stresses

A tight complex-arithmetic inner loop (`z = z² + c`) — the kind of loop LLVM
vectorizes aggressively, so it is a sharp probe of the native↔LLVM code-quality
gap.

## Implementation notes

Output is compared byte-for-byte (`COMPARE=bytes`). Byte-identity across the
native backend, the LLVM backend, and C was confirmed at N=200/512/1000 before
adopting the exact-bytes check — no floating-point-contraction boundary
divergence. The Binate entry writes the header and packed bitmap via
`os.Stdout.Write`.
