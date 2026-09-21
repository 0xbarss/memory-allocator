# memory-allocator

A memory allocator written in C that manages process heap memory through Linux system calls.

## Features implemented

- Heap management: Requests memory segments from the operating system with `sbrk`.
- 16-byte alignment: Aligns all allocated payloads to 16-byte boundaries.
- Block metadata: Uses 32-byte headers and 16-byte footers (boundary tags) for bidirectional traversal.
- Splitting and coalescing: Splits oversized free blocks to reduce internal fragmentation, and merges adjacent free blocks to prevent external fragmentation.
- Standard API: Implements `my_malloc`, `my_free`, `my_calloc` (with integer overflow checks), and `my_realloc` (with in-place expansion).
- Multiple search strategies: Supports First-Fit, Best-Fit, and Segregated Free Lists across 8 power-of-two size classes.
- Benchmarking: Includes an isolated micro-benchmark comparing First-Fit, Best-Fit, Segregated Lists, and glibc across several workloads.

## Building and running

Run commands with make:

- `make`: Compile the main executable.
- `make run`: Run the demo program.
- `make test`: Run the unit test suite.
- `make bench`: Run the benchmark suite against glibc.
- `make clean`: Remove build artifacts.

## Benchmark results

Comparison of search policies against glibc under isolated test runs:

| Workload | glibc | First-Fit | Best-Fit | Segregated List |
| :--- | :--- | :--- | :--- | :--- |
| Small Objects | 0.38 ms | 178.81 ms | 147.08 ms | 1.29 ms |
| Large Buffer Churn | 2.10 ms | 6.46 ms | 7.13 ms | 2.27 ms |
| Random Life-Cycle | 0.31 ms | 8.39 ms | 15.88 ms | 0.98 ms |
| Fragmentation Torture | 0.73 ms | 114.93 ms | 142.40 ms | 8.40 ms |
| Heap Growth (Torture) | 0.36 MB | 0.59 MB | 0.59 MB | 0.59 MB |

Segregated Free Lists performs close to glibc on small object churn, while First-Fit and Best-Fit show higher latency on fragmented heaps due to full-list traversals.
