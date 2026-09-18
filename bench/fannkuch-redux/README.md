# fannkuch-redux

For every permutation of `1..N`, count the "pancake flips" (prefix reversals)
needed to bring the first element to 1. Report the maximum flip count over all
permutations and an alternating-sign checksum.

## Pinned input

`DEFAULT_N = 11`. Reference output (`expected.txt`):

```
556355
Pfannkuchen(11) = 51
```

The canonical CLBG size is 12, but the straightforward full-prefix-reversal flip
count — kept identical across all languages for a fair comparison — makes 12
about ten times slower, too heavy for the pin.

## What it stresses

Integer work: permutation generation and tight array indexing/swapping, with a
data-dependent inner loop. The C reference was validated against the published
checksum/maxflips for N=7/8/10/11 before porting.

## Implementation notes

Output is deterministic, compared byte-for-byte (`COMPARE=exact`).
