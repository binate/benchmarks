# binary-trees

Allocate, walk, and free many binary trees: build one "stretch" tree, hold one
long-lived tree, then for each depth from 4 to `maxDepth` build and check a large
number of short-lived trees. Report the node checks.

## Pinned input

`DEFAULT_N = 16` (tree depth; the canonical CLBG size is 21, but 16 keeps the pin
fast). Reference checks are in `expected.txt`.

## What it stresses

Memory management — allocation and teardown churn. Each language uses its natural
strategy: `malloc`/`free` (C), `new`/`delete` (C++), `Box` (Rust), garbage
collection (Go/Java), reference counting (Binate/Python). This is the suite's
probe of Binate's refcount allocation path, and of how much a codegen backend
matters when time is dominated by the shared allocator/runtime.

## Implementation notes

The check values are deterministic integers, compared byte-for-byte
(`COMPARE=exact`). The Binate entry uses managed `@TreeNode` nodes with `nil`
children for leaves; each temporary tree is scoped to one loop iteration and
reclaimed by reference counting when it goes out of scope.
