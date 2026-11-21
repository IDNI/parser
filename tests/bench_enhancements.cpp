// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Benchmark: node interning (new nodes and repeated lookups), gc, and the
// April 2026 APIs. To compare two storage layouts, build and run it on each.
//
// Run with:   ./build/bench_enhancements [N]
//   N = number of intern operations (default 100000)

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
    double ops_per_sec() const { return static_cast<double>(ops) / (ms / 1000.0); }
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

// Node i takes the node built just before it as its left child, so all N
// nodes are distinct and every call of the first pass creates a node (a
// miss); its right child is a pseudo-random earlier node. The second pass
// repeats the same N calls, and each finds its node (a hit). gc is paused
// while the pool holds plain trefs (no handles), so nothing is swept under it.
std::pair<Result, Result> bench_intern(size_t N) {
    bintree<char>::gc_pause pause;
    std::vector<tref> pool;
    pool.reserve(N + 26);
    for (char c = 'a'; c <= 'z'; ++c)
        pool.push_back(bintree<char>::get(c, nullptr, nullptr));
    std::vector<size_t> right(N);
    std::mt19937_64 rng(1);
    for (size_t i = 0; i < N; ++i) right[i] = rng() % (26 + i);

    double ms_miss = time_ms([&] {
        for (size_t i = 0; i < N; ++i)
            pool.push_back(bintree<char>::get(char('a' + i % 26),
                pool[25 + i], pool[right[i]]));
    });
    size_t same = 0;
    double ms_hit = time_ms([&] {
        for (size_t i = 0; i < N; ++i)
            same += bintree<char>::get(char('a' + i % 26),
                pool[25 + i], pool[right[i]]) == pool[26 + i];
    });
    if (same != N)
        std::cerr << "bench_intern: " << N - same
                  << " lookups did not return their node\n";
    return { { "intern N new nodes (misses)", ms_miss, N },
             { "intern the same N nodes again (hits)", ms_hit, N } };
}

// ---------------------------------------------------------------------------
// Benchmark 2: GC throughput
// Builds N distinct nodes that no handle holds (a chain), then times one
// gc() that sweeps them. A first, untimed gc() clears the earlier benchmark.
// ---------------------------------------------------------------------------

Result bench_gc(size_t N) {
    bintree<char>::gc();
    {
        bintree<char>::gc_pause pause;
        tref prev = nullptr;
        for (size_t i = 0; i < N; ++i)
            prev = bintree<char>::get(char('a' + i % 26), prev, nullptr);
    }
    size_t before = bintree<char>::node_count();
    double ms = time_ms([&] { bintree<char>::gc(); });
    size_t swept = before - bintree<char>::node_count();
    return { "gc() sweeping " + std::to_string(swept) + " unanchored nodes",
             ms, swept };
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

    auto [miss, hit] = bench_intern(N);
    print(miss);
    print(hit);
    print(bench_gc(N));
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
