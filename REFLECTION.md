# A1 REFLECTION

> Complete every section. CI will:
>
> 1. Verify all `## Section` headers below are present.
> 2. Verify each section has **at least 50 words**.
>
> No automatic content grading: the prose is read by a human, and the short
> prompt at the end is marked on a 0 / 0.5 / 1 scale. The numbers you quote
> in your reflection do **not** have to match canonical times exactly — HPC
> queue variance is real. Be concise, ground claims in your measurements, show your working.

## Section 1 — Schedule choice and why

Which schedule (`static` / `dynamic` / `guided` / chunk size) did you end up with, and why? Reference the cost structure of `f(x)` and what the measured timings told you. Mention at least one schedule you tried and discarded, and what the measured evidence was. Minimum 50 words.

I chose schedule(guided) because it provided the best scaling with thread count, achieving a final measured time of ~0.045 seconds at 128 threads. This was better than its static (~0.59) and dynamic (0.1) alternatives by a significant margin. The function f(x) has a highly uneven cost structure with a severe 10× compute spike between x∈[0.3,0.4]. This makes load balancing critical.

Interestingly dynamic schedule measured timings revealed a regression in runtime when scaling from 64 to 128 threads and the execution time actually increased. This points to an overhead cost being present in how the dynamic scheduler was functioning. As workload was split smaller between more threads, it would be querying the central task queue more, and eventually this overhead cost would outweigh the actual execution time. I switched to guided, which likely solved this bottleneck by starting with large chunk sizes to solve cheap regions first, and progressively shrinking them down to focus on the remaining iterations in the f(x) spike region, leading to better cpu utilisation.

## Section 2 — Scaling behaviour

Looking at your `tables.csv`, where does your speedup curve depart from ideal (linear)? What does that tell you about overhead, memory bandwidth, or load balance for this kernel? Minimum 50 words.

The speedup curve departs from the ideal linear trajectory immediately, achieving only an 8.19× speedup at 16 threads (51% efficiency). This sub-linear scaling continues further at 64 and 128 threads, where efficiency drops to 41% and 33%, respectively. Because we largely mitigated the load imbalance of the f(x) spike region by using the guided schedule, this mismatch is likely due to increasing overhead. Specifically, the synchronization overhead of OpenMP managing the dynamic chunk sizes and the reduction(+:sum) lock becomes more dominant as the thread count rises. Furthermore, activating 128 threads on an HPC node inevitably introduces NUMA (Non-Uniform Memory Access) latency and shared cache contention, preventing perfect 1:1 linear scaling even in compute-bound kernels.

## Section 3 — Roofline position

Pick your best thread count. Using the Rome roofline constants from the day-2 slides (theoretical peak 4608 GFLOPs, HPL-achievable 2896 GFLOPs, STREAM triad 246 GB/s), what roofline fraction did you achieve against the *theoretical* and the *HPL-achievable* compute ceilings? Most non-DGEMM code (including A1) lands well below both — explain why your kernel doesn't approach DGEMM-class efficiency. If you want to argue your kernel is bandwidth-bound rather than compute-bound, justify it. Minimum 50 words.

At my best thread count (128 threads using the guided schedule), my kernel achieved approximately 24.1 GFLOP/s. This yielded a roofline fraction of <1% against both the theoretical peak and the HPL-achievable peak.

My code does not approach DGEMM-class efficiency, but this is entirely expected based on the workload. The scalar function f(x) contains a heavily nested control-flow branch (the 10× spike region). This branch divergence prevents the compiler from vectorizing the loop, reducing the execution to scalar instructions. This kernel is also compute bound. The integration has an high operational cost. Because no large arrays are being streamed to or from main memory, the 246 GB/s STREAM bandwidth ceiling is irrelevant (so not memory bound). The kernel is throttled entirely by the CPU instruction latency of the 10 million nested std::sqrt operations in the spike region.

## Section 4 — What you'd try next

You have two more days. What would you change about `integrate.cpp`? Pick one concrete change and predict its effect. Minimum 50 words.

Given more time I would fine tune the minimum chunk size of the scheduling clause via trial and error. Currently, the default guided schedule progressively shrinks the chunk sizes down to a single iteration at the very end of the loop. At 128 threads, querying the OpenMP task queue for 1-iteration chunks introduces unnecessary overhead. By enforcing a more suitable minimum chunk size, I predict this change could help eliminate that overhead. It would yield a small but measurable reduction in execution time by keeping the threads computing rather than queuing.

## Reasoning question (instructor-marked, ≤100 words)

**In at most 100 words, explain why your chosen schedule is appropriate for the cost structure of this particular `f(x)`.**

The guided schedule addresses the 10x cost spike region in f(x). A static schedule suffers from severe load imbalance, while a dynamic schedule causes massive lock contention at 128 threads due to constant central queue querying. guided offers the best of both worlds. It minimizes queue overhead by assigning large chunks for the cheap regions of the integral, then progressively shrinks the chunk sizes. This dynamically load-balances the expensive "spike" iterations, ensuring all threads remain utilized without overwhelming the task queue.