# richards

The classic Richards operating-system task-scheduler benchmark. A scheduler runs
a chain of tasks — an idle task, a worker, two packet handlers, and two devices —
passing work and device packets between them until the idle task's countdown
expires. Each task is a different type, dispatched polymorphically; the run
reproduces the standard verification counts.

## Pinned input

`DEFAULT_N = 10000` — the number of independent scheduler simulations to run (for
timing; the result is identical each time). Reference output (`expected.txt`):

```
queue 2322 hold 928
```

These are the canonical Richards verification counts (queued-packet count and
task-hold count) for the standard `COUNT = 1000` scheduling run; the C reference
was checked against them before porting.

## What it stresses

**Dynamic dispatch.** This is the suite's dispatch probe — the only benchmark
built around polymorphic method calls rather than numeric loops. Each language
uses its natural mechanism: a Binate `interface` with a separate `impl` per task
type, Go interfaces, C++/Java virtual methods, Rust `dyn` trait objects, Python
duck-typed classes, C function pointers. It also churns a small object graph
(scheduler / task-control-blocks / packets), so for Binate it exercises the
reference-counting path as links are reassigned.

## Implementation notes

Output is deterministic, compared byte-for-byte (`COMPARE=exact`). The scheduler
is passed to each task's `run()` rather than stored on the task, so the object
graph has no task↔scheduler cycle — which matters for Binate (reference-counted,
no cycle collector) and keeps the design identical across languages. Richards'
node aliasing cannot be expressed with plain safe references, so the C and Rust
entries use raw pointers with arena ownership; the Binate entry uses managed
pointers throughout, and the graph (being acyclic by construction) is freed by
reference counting when each simulation's scheduler goes out of scope.
