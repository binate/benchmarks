# Pinned parameters for the binary-trees benchmark.
DEFAULT_N=16           # tree depth (canonical CLBG uses 21; 16 keeps the pin fast)
SMALL_N=10             # depth for the CI correctness cross-check
JAVA_MAIN=BinaryTrees
COMPARE=exact          # deterministic integer text output, matched byte-for-byte
