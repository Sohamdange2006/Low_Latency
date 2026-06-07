# CSoT Low Latency Track - Week 1

## Headline Numbers (Judge EC2)
* **p50 Latency:** 34 ns
* **p99 Latency:** 58 ns
* **Throughput:** 15.47 M ticks/s
* **Optimization Notes:** Achieved by flattening state into a `std::array`, eradicating `std::sqrt` and division from the hot path, and inlining symbol state lookups.

## Hardware Notes
* **OS:** WSL2 Ubuntu 22.04 on Windows 11
* **CPU:** [Insert your laptop CPU here]
* **RAM:** [Insert your RAM here]

## Build Steps
To build the baseline runner:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

To build the highly optimized strategy for the x86-64-v2 judge (AVX-safe):

```bash
g++ -std=c++20 -O2 -march=x86-64-v2 -fno-omit-frame-pointer -Iinclude -fPIC -shared -Wl,-soname,spec_strategy.so -o spec_strategy.so strategies/spec_strategy.cpp
```
## Surprises This Week
I was incredibly surprised by how much latency is introduced by simply using standard library containers like std::unordered_map or doing standard floating-point division, and how taking manual control of memory layout and branch prediction can drop execution times down to the 30ns range.