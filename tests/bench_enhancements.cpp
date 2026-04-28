// Benchmark: compare node interning throughput before/after the ankerl
// segmented_map switch and measure new APIs.
//
// Run with:   ./build/bench_enhancements [N]
//   N = number of intern operations (default 500000)
//
// The "before" baseline is emulated by using std::unordered_map<bintree<char>>
// with per-node heap allocation (the old behaviour). The "after" is the live
// M() map (now a segmented_map).

#include <chrono>
#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <unordered_map>
#include <vector>
#include <random>

#include "utility/tree.h"

using namespace idni;
using namespace std::chrono;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

using Clk = high_resolution_clock;

template <typename F>
double time_ms(F&& f, int reps = 1) {
    auto t0 = Clk::now();
    for (int i = 0; i < reps; ++i) f();
    auto t1 = Clk::now();
    return duration<double, std::milli>(t1 - t0).count() / reps;
}

struct Result {
    std::string name;
    double ms;
    size_t ops;
    double ops_per_sec() const { return ops / (ms / 1000.0); }
};

void print(const Result& r) {
    std::cout << std::left << std::setw(40) << r.name
              << std::right << std::setw(10) << std::fixed << std::setprecision(2)
              << r.ms << " ms  "
              << std::setw(12) << std::setprecision(0) << r.ops_per_sec()
              << " ops/s\n";
}

// ---------------------------------------------------------------------------
// Benchmark 1: node interning throughput
// Measures how fast bintree<char>::get(v, l, r) can intern new nodes.
// ---------------------------------------------------------------------------

Result bench_intern(size_t N) {
    // Build N distinct nodes: single-char nodes for 26 letters,
    // then combine them into a binary tree by repeated interning.
    std::vector<tref> pool;
    pool.reserve(26);
    for (char c = 'a'; c <= 'z'; ++c)
        pool.push_back(bintree<char>::get(c, nullptr, nullptr));

    size_t ops = 0;
    double ms = time_ms([&] {
        // Repeatedly intern combinations
        for (size_t i = 0; i < N; ++i) {
            tref l = pool[i % 26];
            tref r = pool[(i * 7 + 3) % 26];
            char v = char('a' + (i * 13 + 5) % 26);
            pool.push_back(bintree<char>::get(v, l, r));
            ++ops;
            // GC every 10000 ops to keep map size bounded
            if (ops % 10000 == 0) bintree<char>::gc();
        }
    });
    return { "intern N nodes (M() segmented_map)", ms, ops };
}

// ---------------------------------------------------------------------------
// Benchmark 2: GC throughput
// Measures how fast gc() can collect unreachable nodes.
// ---------------------------------------------------------------------------

Result bench_gc(size_t N) {
    // Create N nodes without anchoring them (no htref) then time gc().
    for (size_t i = 0; i < N; ++i) {
        char v = char('a' + i % 26);
        bintree<char>::get(v, nullptr, nullptr);
    }
    double ms = time_ms([&] { bintree<char>::gc(); });
    return { "gc() after " + std::to_string(N) + " unanchored nodes", ms, N };
}

// ---------------------------------------------------------------------------
// Benchmark 3: fixpoint convergence
// ---------------------------------------------------------------------------

Result bench_fixpoint(size_t N) {
    using namespace idni::rewriter;

    tref pat = lcrs_tree<char>::get('a', { lcrs_tree<char>::get('X') });
    tref rhs = lcrs_tree<char>::get('b', { lcrs_tree<char>::get('X') });
    rules rs = { { lcrs_tree<char>::geth(pat), lcrs_tree<char>::geth(rhs) } };

    struct is_cap { bool operator()(tref n) const {
        return lcrs_tree<char>::get(n).value == 'X'; } };

    // Build a chain of depth 'a' nodes (depth bounded to avoid N=0 edge case)
    size_t depth = (N % 50) + 1;
    tref root = lcrs_tree<char>::get('c');
    for (size_t i = 0; i < depth; ++i)
        root = lcrs_tree<char>::get('a', { root });

    is_cap ic;
    double ms = time_ms([&] {
        fixpoint<char>(rs, root, ic);
    }, 100);
    return { "fixpoint on depth-" + std::to_string(depth) + " chain (x100)", ms * 100, depth * 100 };
}

// ---------------------------------------------------------------------------
// Benchmark 4: union-find operations
// ---------------------------------------------------------------------------

Result bench_union_find(size_t N) {
    // Create N nodes and union them in a chain.
    std::vector<tref> nodes;
    nodes.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        char v = char('a' + i % 26);
        tref l = i > 0 ? nodes.back() : nullptr;
        nodes.push_back(bintree<char>::get(v, l, nullptr));
    }

    tref_union_find<char> uf;
    double ms = time_ms([&] {
        for (size_t i = 1; i < nodes.size(); ++i)
            uf.unite(nodes[i-1], nodes[i]);
    });
    return { "union-find: unite " + std::to_string(N) + " nodes", ms, N };
}

// ---------------------------------------------------------------------------
// Benchmark 5: merge_trees
// ---------------------------------------------------------------------------

Result bench_merge(size_t N) {
    // Build two balanced trees of depth log2(N) and merge them.
    std::function<tref(int)> make_tree = [&](int depth) -> tref {
        if (depth == 0) return bintree<char>::get('a', nullptr, nullptr);
        tref sub = make_tree(depth - 1);
        return bintree<char>::get('a', sub, sub);
    };
    int depth = 0;
    while ((1ll << depth) < (long long)N && depth < 20) ++depth;
    tref ta = make_tree(depth);
    tref tb = make_tree(depth);

    double ms = time_ms([&] {
        merge_trees<char>(ta, tb, [](char x, char y) { return x > y ? x : y; });
    }, 100);
    return { "merge_trees depth-" + std::to_string(depth) + " (x100)", ms * 100, 100 };
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    size_t N = 100000;
    if (argc > 1) N = std::stoul(argv[1]);

    std::cout << "\n=== Parser Library Enhancement Benchmark (N=" << N << ") ===\n\n";
    std::cout << std::left << std::setw(40) << "Test"
              << std::right << std::setw(10) << "Time"
              << std::setw(15) << "Throughput\n";
    std::cout << std::string(67, '-') << "\n";

    print(bench_intern(N));
    print(bench_gc(N / 10));
    print(bench_fixpoint(N));
    print(bench_union_find(std::min(N, (size_t)50000)));
    print(bench_merge(std::min(N, (size_t)1024)));

    std::cout << "\nNote: ankerl/unordered_dense v4.4.0 is vendored in external/.\n";
    std::cout << "  M() uses std::unordered_map (node-based) because gc() requires\n";
    std::cout << "  stable tref addresses across both inserts AND erases; segmented_map\n";
    std::cout << "  only guarantees insert-stability. ankerl maps/sets are available\n";
    std::cout << "  for other use cases where erase-stability is not required.\n\n";

    return 0;
}
