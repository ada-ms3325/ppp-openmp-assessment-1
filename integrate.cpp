// Assignment 1 — STUDENT IMPLEMENTATION.
//
// Parallelise `integrate_parallel` using OpenMP. The performance score
// is a reference-parallel-time ratio: min(1.0, T_ref(128) / T_yours(128))
// against a published reference solution measured on Rome. Aim to match
// or beat the reference at 128 threads. Roofline fraction is not the
// scoring metric for A1 — non-DGEMM kernels reach a few percent of peak
// in practice, while HPL (vendor-tuned DGEMM) tops out at ~63 %.
//
// Requirements:
//   - Return the same numerical result as integrate_serial to within
//     smart_diff tolerance (float comparison with small rtol/atol).
//   - Build cleanly under Clang-18 + TSan + Archer (the CI gate).
//   - Choose a schedule and justify it in REFLECTION.md.
//
// You may NOT:
//   - Modify f(x) or integrate_serial() — they live in integrate_serial.cpp.
//   - Add third-party libraries or change the CMake build.
//   - Change `main()` in this file (it drives the grader's expected output).

#include "integrate.h"

#include <iomanip>
#include <iostream>

// TODO(): parallelise the body of this function.
// Starter implementation is a serial fallback so the file builds on day 2.
double integrate_parallel(double a, double b, long n)
{
    const double h = (b - a) / static_cast<double>(n);
    double sum = 0.5 * (f(a) + f(b));

#pragma omp parallel for default(none) shared(a, h, n) reduction(+ : sum) schedule(guided)
    for (long i = 1; i < n; ++i) {
        const double x = a + (static_cast<double>(i) * h);
        sum += f(x);
    }
    return sum * h;
}

int main()
{
    // Fixed problem parameters — do NOT edit. Matches expected_output.txt.
    constexpr double A = 0.0;
    constexpr double B = 1.0;
    constexpr long N = 100'000'000;

    const double result = integrate_parallel(A, B, N);

    // Deterministic output — stdout is the correctness channel only. Timing
    // is measured by the hyperfine harness into perf-results-a1.json.
    std::cout << "integral = " << std::fixed << std::setprecision(12)
              << result << "\n";
    return 0;
}
