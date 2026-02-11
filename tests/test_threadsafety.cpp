#include "utility/tree.h"

#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>

using namespace idni;

static void barrier_spin(std::atomic<int>& arrived, int total) {
    arrived.fetch_add(1, std::memory_order_acq_rel);
    while (arrived.load(std::memory_order_acquire) < total) {
        // spin
    }
}

int main(int argc, char** argv) {
    using T = int;

    // Optional: keep GC enabled; for minimal correctness it should be safe.
    // If you see long pauses, you can disable it:
    // bintree<T>::gc_enabled = false;

    constexpr int kthreads = 16;
    constexpr int kiters   = 20000;

    std::atomic<int> arrived{0};
    std::vector<tref> results(kthreads, nullptr);

    // Pre-create some canonical nodes (single-thread) to make shared references.
    tref leafL = bintree<T>::get(1, nullptr, nullptr);
    tref leafR = bintree<T>::get(2, nullptr, nullptr);

    

    auto worker = [&](int tid) {
        barrier_spin(arrived, kthreads);

        // All threads repeatedly request the SAME node.
        // it should always return the same tref.
        tref last = nullptr;

        for (int i = 0; i < kiters; ++i) {
            // Same key each time:
            tref n = bintree<T>::get(42, leafL, leafR);

            // Touch handle cache path too:
            auto h = bintree<T>::geth(n);
            assert(h && "geth() must not return null for non-null tref");
            assert(h->get() == n && "handle must point to same tref");

            // Occasionally create some additional load
            if ((i % 97) == 0) {
                // Different nodes to force inserts/rehashes
                (void)bintree<T>::get(100 + (tid % 7), n, nullptr);
            }

            last = n;

            // Optional: occasionally invoke GC (now serialized by mutex)
            if ((i % 5000) == 0 ) 
                bintree<T>::gc();
        }

        results[tid] = last;
    };

    std::vector<std::thread> threads;
    threads.reserve(kthreads);
    for (int t = 0; t < kthreads; ++t) threads.emplace_back(worker, t);
    for (auto& th : threads) th.join();

    // All threads should have obtained the SAME canonical node pointer.
    for (int t = 1; t < kthreads; ++t) {
        if (results[t] != results[0]) {
            std::cerr << "FAILED: interning mismatch between thread 0 and thread "
                      << t << "\n";
            std::cerr << "t0=" << results[0] << " t" << t << "=" << results[t] << "\n";
            return 1;
        }
    }

    std::cout << "PASSED: concurrent bintree<int>::get/geth interning stable across "
              << kthreads << " threads\n";

    return 0;
}
