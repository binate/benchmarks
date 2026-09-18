# fasta

Generate three DNA sequences in FASTA format (60 characters per line): a repeated
ALU fragment of length `2N`, and two random sequences of length `3N` and `5N`
drawn from fixed probability tables via a linear-congruential PRNG.

## Pinned input

`DEFAULT_N = 1000`. The N=1000 output is exactly the Computer Language Benchmarks
Game's published `fasta-1000.txt`, so `expected.txt` is the canonical reference
output.

## What it stresses

The PRNG and cumulative-probability symbol selection, plus formatted byte output.
It is output/IO-bound rather than a codegen probe — bump `N` (e.g. `N=1000000`)
for a throughput measurement.

## Implementation notes

Output is deterministic, compared byte-for-byte (`COMPARE=exact`). The PRNG seed
is shared across the two random sections (the Binate entry threads it through a
one-element slice, matching the C reference's single global seed).
