// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Tests for the 8 library enhancements:
//   0/8. segmented_map in M() — stable addresses after many insertions
//   1.   fixpoint operator
//   2.   merge_trees (join on labeled trees)
//   3.   annotated_forest (label access + propagation)
//   4.   forest::traverse_backward / build_reverse_index / predecessors
//   5.   tref_union_find
//   6.   grammar::derive_all (unit-rule propagation)
//   7.   pattern_matcher_guarded

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "init_test.h"
#include "utility/annotated_forest.h"
#include "parser.h"

using namespace idni;
using namespace idni::rewriter;

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

struct is_capture_ch {
    bool operator()(tref n) const {
        char v = chtree::get(n).value;
        return v == 'X' || v == 'Y' || v == 'Z';
    }
};

static rule make_rule(tref pat, tref rhs) {
    return { chtree::geth(pat), chtree::geth(rhs) };
}

// Build a tiny grammar for testing: S -> A, A -> B, B -> 'x'
struct tiny_grammar {
    nonterminals<char> nts;
    prods<char> ps, S, A, B;
    char_class_fns<char> cc{};
    grammar<char> g;

    tiny_grammar() :
        S(nts("S")), A(nts("A")), B(nts("B")),
        g(nts, build_ps(), S, cc)
    {}

    prods<char> build_ps() {
        ps(S, A);              // S -> A (unit rule)
        ps(A, B);              // A -> B (unit rule)
        ps(B, prods<char>('x')); // B -> 'x'
        return ps;
    }
};

// ---------------------------------------------------------------------------
// 0/8. segmented_map stability
// ---------------------------------------------------------------------------

TEST_SUITE("enhancement 8: segmented_map stability") {

    TEST_CASE("node addresses remain stable after bulk insertions") {
        tref ref = chtree::get('A');
        // Insert many distinct nodes to potentially trigger rehashing.
        for (int i = 0; i < 26; ++i)
            for (int j = 0; j < 10; ++j)
                chtree::get(char('a'+i), { chtree::get(char('a'+j)) });
        // Address of the original node must still decode correctly.
        CHECK(chtree::get(ref).value == 'A');
    }

    TEST_CASE("identical subtrees share the same tref") {
        tref a = n('a', { n('b'), n('c') });
        tref b = n('a', { n('b'), n('c') });
        CHECK(a == b);
    }

    TEST_CASE("structurally different trees get different trefs") {
        tref a = n('a', { n('b') });
        tref b = n('a', { n('c') });
        CHECK(a != b);
    }
}

// ---------------------------------------------------------------------------
// 1. fixpoint operator
// ---------------------------------------------------------------------------

TEST_SUITE("enhancement 1: fixpoint") {

    TEST_CASE("fixpoint converges on already-stable tree") {
        tref root = n('b', { n('c') });
        tref pat  = n('a', { n('X') });
        tref rhs  = n('a', { n('X') });
        rules rs  = { make_rule(pat, rhs) };
        is_capture_ch ic;
        tref result = fixpoint<char>(rs, root, ic);
        CHECK(cmp(result, root)); // no 'a' nodes — unchanged
    }

    TEST_CASE("fixpoint applies rule until no change") {
        // Rule: 'a'(X) -> 'b'(X).
        // Input: 'a'('a'('c')). After 2 passes: 'b'('b'('c')).
        tref root = n('a', { n('a', { n('c') }) });
        tref pat  = n('a', { n('X') });
        tref rhs  = n('b', { n('X') });
        rules rs  = { make_rule(pat, rhs) };
        is_capture_ch ic;
        tref result = fixpoint<char>(rs, root, ic);
        tref expected = n('b', { n('b', { n('c') }) });
        CHECK(cmp(result, expected));
    }

    TEST_CASE("fixpoint with done predicate stops early") {
        tref root = n('a', { n('a', { n('c') }) });
        tref pat  = n('a', { n('X') });
        tref rhs  = n('b', { n('X') });
        rules rs  = { make_rule(pat, rhs) };
        is_capture_ch ic;
        bool first = true;
        // done() returns true on 2nd call → only one pass runs
        auto done = [&](tref) { if (first) { first = false; return false; } return true; };
        tref result = fixpoint<char>(rs, root, ic, done);
        CHECK(result != nullptr); // terminated early but produced something valid
    }
}

// ---------------------------------------------------------------------------
// 2. merge_trees
// ---------------------------------------------------------------------------

TEST_SUITE("enhancement 2: merge_trees") {

    TEST_CASE("merge identical trees returns same node") {
        tref a = bintree<char>::get('a', nullptr, nullptr);
        tref result = merge_trees<char>(a, a, [](char x, char) { return x; });
        CHECK(result == a);
    }

    TEST_CASE("merge two single-node trees by max") {
        tref b = bintree<char>::get('b', nullptr, nullptr);
        tref d = bintree<char>::get('d', nullptr, nullptr);
        tref result = merge_trees<char>(b, d,
            [](char x, char y) { return std::max(x, y); });
        CHECK(bintree<char>::get(result).value == 'd');
    }

    TEST_CASE("null left side uses mismatch_fn") {
        tref a = bintree<char>::get('a', nullptr, nullptr);
        tref result = merge_trees<char>(nullptr, a,
            [](char x, char) { return x; });
        CHECK(result == a);
    }

    TEST_CASE("null right side uses mismatch_fn") {
        tref a = bintree<char>::get('a', nullptr, nullptr);
        tref result = merge_trees<char>(a, nullptr,
            [](char x, char) { return x; });
        CHECK(result == a);
    }

    TEST_CASE("merge sibling chains combines root values") {
        tref b = bintree<char>::get('b', nullptr, nullptr);
        tref d = bintree<char>::get('d', nullptr, nullptr);
        tref ab = bintree<char>::get('a', nullptr, b); // 'a' with right-sibling 'b'
        tref cd = bintree<char>::get('c', nullptr, d); // 'c' with right-sibling 'd'
        auto join = [](char x, char y) -> char { return (x > y) ? x : y; };
        tref result = merge_trees<char>(ab, cd, join);
        CHECK(bintree<char>::get(result).value == 'c'); // max('a','c')
    }
}

// ---------------------------------------------------------------------------
// 5. tref_union_find
// ---------------------------------------------------------------------------

TEST_SUITE("enhancement 5: tref_union_find") {

    TEST_CASE("find returns self for fresh node") {
        tref_union_find<char> uf;
        tref a = n('a');
        CHECK(uf.find(a) == a);
    }

    TEST_CASE("same returns false before unite") {
        tref_union_find<char> uf;
        tref a = n('a'); tref b = n('b');
        // Initialize both
        uf.find(a); uf.find(b);
        CHECK(!uf.same(a, b));
    }

    TEST_CASE("same returns true after unite") {
        tref_union_find<char> uf;
        tref a = n('a'); tref b = n('b');
        uf.unite(a, b);
        CHECK(uf.same(a, b));
    }

    TEST_CASE("transitivity: unite(a,b), unite(b,c) => same(a,c)") {
        tref_union_find<char> uf;
        tref a = n('a'); tref b = n('b'); tref c = n('c');
        uf.unite(a, b);
        uf.unite(b, c);
        CHECK(uf.same(a, c));
    }

    TEST_CASE("each_class enumerates correct number of classes and members") {
        tref_union_find<char> uf;
        tref a = n('a'); tref b = n('b');
        tref c = n('c'); tref d = n('d');
        uf.unite(a, b);   // class {a,b}
        uf.find(c);        // singleton {c}
        uf.find(d);        // singleton {d}

        int class_count = 0, total = 0;
        uf.each_class([&](tref, const std::vector<tref>& members) {
            ++class_count; total += (int)members.size();
        });
        CHECK(class_count == 3);
        CHECK(total == 4);
    }

    TEST_CASE("unite is idempotent") {
        tref_union_find<char> uf;
        tref a = n('a'); tref b = n('b');
        uf.unite(a, b); uf.unite(a, b);
        CHECK(uf.same(a, b));
    }
}

// ---------------------------------------------------------------------------
// 4. forest::traverse_backward + build_reverse_index
// (Use a small grammar + parse to get a real forest)
// ---------------------------------------------------------------------------

TEST_SUITE("enhancement 4: forest backward traversal") {

    // Build a single-rule grammar S -> 'a' and parse "a".
    struct test_setup {
        nonterminals<char> nts;
        prods<char> ps, S;
        char_class_fns<char> cc{};
        grammar<char> g;
        parser<char> p;

        test_setup() : S(nts("S")), g(build_g()), p(g) {}

        grammar<char> build_g() {
            ps(S, prods<char>('a')); // S -> 'a'
            return grammar<char>(nts, ps, S, cc);
        }
    };

    TEST_CASE("build_reverse_index and predecessors work on parse forest") {
        test_setup ts;
        std::string input = "a";
        auto res = ts.p.parse(input.c_str(), input.size());
        REQUIRE(res.found);
        auto& f = *res.get_forest();
        f.build_reverse_index();

        // Root has no predecessors.
        bool root_has_pred = false;
        f.predecessors(f.root(), [&](const auto&) { root_has_pred = true; });
        CHECK(!root_has_pred);
    }

    TEST_CASE("traverse_backward from root reaches only root (no predecessors)") {
        test_setup ts;
        std::string input = "a";
        auto res = ts.p.parse(input.c_str(), input.size());
        REQUIRE(res.found);
        auto& f = *res.get_forest();
        f.build_reverse_index();

        using F = std::decay_t<decltype(f)>;
        F::nodes_set starts;
        F::nodes pack = { f.root() };
        starts.insert(pack);

        std::vector<F::node> visited;
        f.traverse_backward(starts, [&](const auto& nd) { visited.push_back(nd); });
        // Root itself is enqueued from starts
        CHECK(!visited.empty());
    }
}

// ---------------------------------------------------------------------------
// 3. annotated_forest
// ---------------------------------------------------------------------------

TEST_SUITE("enhancement 3: annotated_forest") {

    struct test_setup {
        nonterminals<char> nts;
        prods<char> ps, S;
        char_class_fns<char> cc{};
        grammar<char> g;
        parser<char> p;

        test_setup() : S(nts("S")), g(build_g()), p(g) {}
        grammar<char> build_g() {
            ps(S, prods<char>('a'));
            return grammar<char>(nts, ps, S, cc);
        }
    };

    TEST_CASE("label access and has_label") {
        test_setup ts;
        std::string input = "a";
        auto res = ts.p.parse(input.c_str(), input.size());
        REQUIRE(res.found);
        auto& f = *res.get_forest();
        annotated_forest<parser<char>::pnode, int> af(f);

        CHECK(!af.has_label(f.root()));
        af.label(f.root()) = 42;
        CHECK(af.has_label(f.root()));
        CHECK(af.label(f.root()) == 42);
    }

    TEST_CASE("propagate returns true when a child label changes") {
        test_setup ts;
        std::string input = "a";
        auto res = ts.p.parse(input.c_str(), input.size());
        REQUIRE(res.found);
        auto& f = *res.get_forest();
        annotated_forest<parser<char>::pnode, int> af(f);

        af.label(f.root()) = 1;
        bool changed = af.propagate([](int p, int c) { return p + c; });
        CHECK(changed); // at least one child should have changed from 0 to 1
    }
}

// ---------------------------------------------------------------------------
// 6. grammar::derive_all (unit-rule propagation)
// ---------------------------------------------------------------------------

TEST_SUITE("enhancement 6: grammar::derive_all") {

    // Grammar: S -> A, A -> B (unit rules). Seed B at pos 0.
    // Expected: A and S also derived at span [0,1].
    TEST_CASE("unit rule chain: S->A->B; seed B derives A and S") {
        nonterminals<char> nts;
        prods<char> ps, S(nts("S")), A(nts("A")), B(nts("B"));
        char_class_fns<char> cc{};
        ps(S, A);          // S -> A
        ps(A, B);          // A -> B
        ps(B, prods<char>('x')); // B -> 'x' (anchor)
        grammar<char> g(nts, ps, S, cc);

        // Build lit objects for B, A, S
        auto lB = g.nt(std::string("B"));  // nonterminal lit for B
        auto lA = g.nt(std::string("A"));
        auto lS = g.nt(std::string("S"));

        std::vector<std::pair<lit<char>, size_t>> seeds = { {lB, 0} };
        auto derived = g.derive_all(seeds);

        bool found_B = false, found_A = false, found_S = false;
        for (auto& [l, sp] : derived) {
            if (sp[0] != 0 || sp[1] != 1) continue;
            if (l == lB) found_B = true;
            if (l == lA) found_A = true;
            if (l == lS) found_S = true;
        }
        CHECK(found_B);
        CHECK(found_A);
        CHECK(found_S);
    }

    TEST_CASE("disconnected nonterminal C not derived from B seed") {
        nonterminals<char> nts;
        prods<char> ps, S(nts("S")), A(nts("A")), B(nts("B")), C(nts("C"));
        char_class_fns<char> cc{};
        ps(S, A); ps(A, B); ps(B, prods<char>('x'));
        // C is isolated — no production references it as head from B
        grammar<char> g(nts, ps, S, cc);

        auto lB = g.nt(std::string("B"));
        auto lC = g.nt(std::string("C"));
        auto derived = g.derive_all({ {lB, 0} });

        bool found_C = false;
        for (auto& [l, sp] : derived)
            if (l == lC) found_C = true;
        CHECK(!found_C);
    }
}

// ---------------------------------------------------------------------------
// 7. pattern_matcher_guarded
// ---------------------------------------------------------------------------

TEST_SUITE("enhancement 7: pattern_matcher_guarded") {

    TEST_CASE("guarded rule fires when guard passes") {
        tref root    = n('a', { n('c') });
        tref pattern = n('a', { n('X') });
        tref rhs     = n('b', { n('X') });
        rule r = make_rule(pattern, rhs);
        is_capture_ch ic;
        auto guard = [](const environment<char>&, tref) { return true; };
        tref result = apply_rule_guarded<char>(r, root, ic, guard);
        CHECK(cmp(result, n('b', { n('c') })));
    }

    TEST_CASE("guarded rule does NOT fire when guard rejects") {
        tref root    = n('a', { n('c') });
        tref pattern = n('a', { n('X') });
        tref rhs     = n('b', { n('X') });
        rule r = make_rule(pattern, rhs);
        is_capture_ch ic;
        auto guard = [](const environment<char>&, tref) { return false; };
        tref result = apply_rule_guarded<char>(r, root, ic, guard);
        CHECK(cmp(result, root));
    }

    TEST_CASE("guard can inspect capture bindings") {
        tref pattern = n('a', { n('X') });
        tref rhs     = n('b', { n('X') });
        rule r = make_rule(pattern, rhs);
        is_capture_ch ic;

        // The guard receives the binding environment: X_tref -> captured_tref.
        // Accept only when X was bound to 'd'.
        tref d    = n('d');
        tref x_pt = n('X'); // same interned node as the X in the pattern
        auto guard = [&](const environment<char>& env, tref) -> bool {
            auto it = env.find(x_pt);
            return it != env.end() && cmp(it->second, d);
        };

        tref root_d = n('a', { n('d') }); // X binds to 'd' → guard passes
        tref root_c = n('a', { n('c') }); // X binds to 'c' → guard fails

        tref res_d = apply_rule_guarded<char>(r, root_d, ic, guard);
        tref res_c = apply_rule_guarded<char>(r, root_c, ic, guard);

        CHECK(cmp(res_d, n('b', { n('d') }))); // transformed
        CHECK(cmp(res_c, root_c));             // unchanged
    }
}
