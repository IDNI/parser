#include "utility/tree.h"

#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include "testing.h"

using namespace idni;

int test(bool different_key) {
    using T = int;

    // Optional: keep GC enabled; for minimal correctness it should be safe.
    // If you see long pauses, you can disable it:
    // bintree<T>::gc_enabled = false;

    int kthreads = testing::thread_count;
    int kiters = 20000;
    std::atomic<int> arrived{0};
    std::vector<tref> results(kthreads, nullptr);

    tref leafL = bintree<T>::get(1, nullptr, nullptr);
    tref leafR = bintree<T>::get(2, nullptr, nullptr);

    auto tstart = std::chrono::steady_clock::now();

    auto worker = [&](int tid) {
        arrived.fetch_add(1, std::memory_order_acq_rel);
        while (arrived.load(std::memory_order_acquire) < kthreads) {
            std::this_thread::yield();
        }

        tref last = nullptr;

        for (int i = 0; i < kiters; ++i) {
            tref n = bintree<T>::get(different_key ? 42 + tid : 42, leafL, leafR);

            // Sample geth to avoid allocation dominating the test
            if ((i & 15) == 0) {
                auto h = bintree<T>::geth(n);
                assert(h && "geth() must not return null for non-null tref");
                assert(h->get() == n && "handle must point to same tref");
            }

            if ((i % 97) == 0) {
                (void)bintree<T>::get(100000 + tid * 1000 + (i % 1000), n, nullptr);
            }

            last = n;
        }

        results[tid] = last;
    };

    std::vector<std::thread> threads;
    threads.reserve(kthreads);
    for (int t = 0; t < kthreads; ++t) threads.emplace_back(worker, t);
    for (auto& th : threads) th.join();

    auto tend = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tend - tstart).count();

    if (!different_key) {
        for (int t = 1; t < kthreads; ++t) {
            if (results[t] != results[0]) {
                std::cerr << "FAILED: interning mismatch between thread 0 and thread "
                          << t << "\n";
                std::cerr << "t0=" << results[0] << " t" << t << "=" << results[t] << "\n";
                return 1;
            }
        }
    } else {
        for (int t = 0; t < kthreads; ++t) {
            tref expected = bintree<T>::get(42 + t, leafL, leafR);
            if (results[t] != expected) {
                std::cerr << "FAILED: different_key interning mismatch for thread "
                          << t << "\n";
                std::cerr << "got=" << results[t] << " expected=" << expected << "\n";
                return 1;
            }
        }
    }

    std::cout << "PASSED: "
              << (different_key ? "different_key" : "same_key")
              << " threads=" << kthreads
              << " iters=" << kiters
              << " elapsed_ms=" << ms
              << "\n";

    idni::bintree<int>::print_get_lock_profile(std::cerr);
    return 0;
}

int main(int argc, char** argv)
{
    testing::process_args(argc, argv);
    test(false);
    idni::bintree<int>::reset_get_lock_profile();
    test(true);
}