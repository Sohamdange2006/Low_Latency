# Cache profile — matching engine

Workload: 1,000,000 orders (80% limit / 10% market / 10% cancel), seed=42.
Deterministic result: 881,935 fills, 22,485,200 volume.
Throughput: ~13.7M orders/sec, single-threaded (-O2, no instrumentation).

Profiled with: valgrind --tool=cachegrind --cache-sim=yes

## Where the L1 data-read misses go (2.10M total)

| Source                         | Instr share | L1 read-miss share |
|--------------------------------|-------------|--------------------|
| CSV parsing (sscanf/strtoul)   | ~63%        | ~0%                |
| Cancel hash-index (idx_)       | ~2%         | ~47%               |
| All order-book structures      | ~12%        | ~62%               |
| Order-feed streaming (rows[])  | small       | ~24%               |

## Takeaways
- The O(1)-cancel `unordered_map` is the single largest miss source: ~5% L1
  miss rate, because hash lookups have no spatial locality.
- Parsing is instruction-heavy but cache-friendly (sequential) -> ~0% misses.
- Last-level (DRAM) misses are ~98% the sequential rows[] feed, not the book.
  The live book fits in L2/L3 but not L1: an L1-locality cost, not DRAM-bound.
- Bottleneck is memory locality, not algorithmic complexity.

## What I'd do next
Arena/pool-allocate the list and map nodes (or go intrusive) to pack the hot
path into contiguous memory and cut the scattered hash/node misses.
