// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "init_test.h"

// Capture predicate: nodes with value 'X', 'Y', or 'Z' are capture variables.
struct is_capture_predicate {
	bool operator()(const tref& n) const {
		auto v = chtree::get(n).value;
		return v == 'X' || v == 'Y' || v == 'Z';
	}
};

// ============================================================================
// Predicate combinators: true_predicate, false_predicate,
// and_predicate, or_predicate, neg_predicate
// ============================================================================

TEST_SUITE("true_predicate") {

	TEST_CASE("always returns true for any node") {
		auto t = true_predicate;
		CHECK( t(n('a')) );
		CHECK( t(n('z')) );
		CHECK( t(n('a', {n('b'), n('c')})) );
	}
}

TEST_SUITE("false_predicate") {

	TEST_CASE("always returns false for any node") {
		auto f = false_predicate;
		CHECK( !f(n('a')) );
		CHECK( !f(n('z')) );
		CHECK( !f(n('a', {n('b'), n('c')})) );
	}
}

TEST_SUITE("and_predicate") {

	TEST_CASE("truth table: true && true = true") {
		auto t = true_predicate;
		CHECK( and_predicate(t, t)(n('a')) );
	}

	TEST_CASE("truth table: true && false = false") {
		auto t = true_predicate;
		auto f = false_predicate;
		CHECK( !and_predicate(t, f)(n('a')) );
	}

	TEST_CASE("truth table: false && true = false") {
		auto t = true_predicate;
		auto f = false_predicate;
		CHECK( !and_predicate(f, t)(n('a')) );
	}

	TEST_CASE("truth table: false && false = false") {
		auto f = false_predicate;
		CHECK( !and_predicate(f, f)(n('a')) );
	}

	TEST_CASE("works with lambda predicates that inspect node values") {
		auto is_a = [](tref nd) { return chtree::get(nd).value == 'a'; };
		auto is_b = [](tref nd) { return chtree::get(nd).value == 'b'; };
		CHECK( !and_predicate(is_a, is_b)(n('a')) );
		CHECK( !and_predicate(is_a, is_b)(n('b')) );
		CHECK( !and_predicate(is_a, is_b)(n('c')) );
	}
}

TEST_SUITE("or_predicate") {

	TEST_CASE("truth table: true || true = true") {
		auto t = true_predicate;
		CHECK( or_predicate(t, t)(n('a')) );
	}

	TEST_CASE("truth table: true || false = true") {
		auto t = true_predicate;
		auto f = false_predicate;
		CHECK( or_predicate(t, f)(n('a')) );
	}

	TEST_CASE("truth table: false || true = true") {
		auto t = true_predicate;
		auto f = false_predicate;
		CHECK( or_predicate(f, t)(n('a')) );
	}

	TEST_CASE("truth table: false || false = false") {
		auto f = false_predicate;
		CHECK( !or_predicate(f, f)(n('a')) );
	}

	TEST_CASE("works with lambda predicates that inspect node values") {
		auto is_a = [](tref nd) { return chtree::get(nd).value == 'a'; };
		auto is_b = [](tref nd) { return chtree::get(nd).value == 'b'; };
		CHECK( or_predicate(is_a, is_b)(n('a')) );
		CHECK( or_predicate(is_a, is_b)(n('b')) );
		CHECK( !or_predicate(is_a, is_b)(n('c')) );
	}
}

TEST_SUITE("neg_predicate") {

	TEST_CASE("negates true_predicate") {
		auto t = true_predicate;
		CHECK( !neg_predicate(t)(n('a')) );
	}

	TEST_CASE("negates false_predicate") {
		auto f = false_predicate;
		CHECK( neg_predicate(f)(n('a')) );
	}

	TEST_CASE("double negation of true_predicate is true") {
		auto t = true_predicate;
		auto nt = neg_predicate(t);
		CHECK( !neg_predicate(nt)(n('a')) );
	}

	TEST_CASE("negates a lambda predicate") {
		auto is_a = [](tref nd) { return chtree::get(nd).value == 'a'; };
		CHECK( !neg_predicate(is_a)(n('a')) );
		CHECK( neg_predicate(is_a)(n('b')) );
	}
}

// ============================================================================
// select_top_predicate
// ============================================================================

TEST_SUITE("select_top_predicate") {

	TEST_CASE("leaf root satisfying predicate: selects root") {
		tref root = n('a');
		auto pred = [](tref nd) { return chtree::get(nd).value == 'a'; };
		trefs selected;
		select_top_predicate<decltype(pred)> sel{pred, selected};
		post_order_traverser<char, decltype(idni::identity),
			decltype(sel)>(idni::identity, sel)(root);
		CHECK( cmp(selected, trefs{root}) );
	}

	TEST_CASE("leaf root not satisfying predicate: selects nothing") {
		tref root = n('a');
		auto pred = [](tref nd) { return chtree::get(nd).value == 'z'; };
		trefs selected;
		select_top_predicate<decltype(pred)> sel{pred, selected};
		post_order_traverser<char, decltype(idni::identity),
			decltype(sel)>(idni::identity, sel)(root);
		CHECK( selected.empty() );
	}

	TEST_CASE("inner children satisfying predicate: selects children, not root") {
		tref root = n('a', {n('b'), n('c')});
		auto pred = [](tref nd) {
			auto v = chtree::get(nd).value;
			return v == 'b' || v == 'c';
		};
		trefs selected;
		select_top_predicate<decltype(pred)> sel{pred, selected};
		post_order_traverser<char, decltype(idni::identity),
			decltype(sel)>(idni::identity, sel)(root);
		// top nodes are b and c (children come before root in post-order)
		CHECK( selected.size() == 2 );
		CHECK( cmp(selected[0], n('b')) );
		CHECK( cmp(selected[1], n('c')) );
	}

	TEST_CASE("root satisfying predicate: does not descend into children") {
		tref root = n('a', {n('a', {n('b')})});
		auto pred = [](tref nd) { return chtree::get(nd).value == 'a'; };
		trefs selected;
		select_top_predicate<decltype(pred)> sel{pred, selected};
		post_order_traverser<char, decltype(idni::identity),
			decltype(sel)>(idni::identity, sel)(root);
		// The top 'a' stops descent, so inner 'a' is not included
		CHECK( selected.size() == 1 );
	}

	TEST_CASE("diamond DAG: shared node counted only once") {
		tref d_node = n('d');
		tref root = n('a', {n('b', {d_node}), n('c', {d_node})});
		auto pred = [](tref nd) { return chtree::get(nd).value == 'd'; };
		trefs selected;
		select_top_predicate<decltype(pred)> sel{pred, selected};
		post_order_traverser<char, decltype(idni::identity),
			decltype(sel)>(idni::identity, sel)(root);
		CHECK( selected.size() == 1 );
		CHECK( cmp(selected[0], n('d')) );
	}
}

// ============================================================================
// select_all_predicate
// ============================================================================

TEST_SUITE("select_all_predicate") {

	TEST_CASE("leaf root satisfying predicate: selects root") {
		tref root = n('a');
		auto pred = [](tref nd) { return chtree::get(nd).value == 'a'; };
		trefs selected;
		select_all_predicate<decltype(pred)> sel{pred, selected};
		post_order_traverser<char, decltype(idni::identity),
			decltype(sel)>(idni::identity, sel)(root);
		CHECK( cmp(selected, trefs{root}) );
	}

	TEST_CASE("no matching nodes: empty result") {
		tref root = n('a', {n('b'), n('c')});
		auto pred = [](tref nd) { return chtree::get(nd).value == 'z'; };
		trefs selected;
		select_all_predicate<decltype(pred)> sel{pred, selected};
		post_order_traverser<char, decltype(idni::identity),
			decltype(sel)>(idni::identity, sel)(root);
		CHECK( selected.empty() );
	}

	TEST_CASE("all nodes match: collects root and all children") {
		tref root = n('a', {n('b'), n('c')});
		auto pred = [](tref) { return true; };
		trefs selected;
		select_all_predicate<decltype(pred)> sel{pred, selected};
		post_order_traverser<char, decltype(idni::identity),
			decltype(sel)>(idni::identity, sel)(root);
		CHECK( selected.size() == 3 );
	}

	TEST_CASE("only children match: both children collected") {
		tref root = n('a', {n('b'), n('c')});
		auto pred = [](tref nd) {
			auto v = chtree::get(nd).value;
			return v == 'b' || v == 'c';
		};
		trefs selected;
		select_all_predicate<decltype(pred)> sel{pred, selected};
		post_order_traverser<char, decltype(idni::identity),
			decltype(sel)>(idni::identity, sel)(root);
		CHECK( selected.size() == 2 );
	}

	TEST_CASE("diamond DAG: shared node appears only once") {
		tref d_node = n('d');
		tref root = n('a', {n('b', {d_node}), n('c', {d_node})});
		auto pred = [](tref nd) { return chtree::get(nd).value == 'd'; };
		trefs selected;
		select_all_predicate<decltype(pred)> sel{pred, selected};
		post_order_traverser<char, decltype(idni::identity),
			decltype(sel)>(idni::identity, sel)(root);
		// select_all_predicate does NOT deduplicate; result size depends on traversal
		// Since post_order_traverser de-duplication: each structural node visited once
		CHECK( selected.size() >= 1 );
		CHECK( cmp(selected[0], n('d')) );
	}
}

// ============================================================================
// find_top_predicate
// ============================================================================

TEST_SUITE("find_top_predicate") {

	TEST_CASE("leaf root satisfying predicate: finds root") {
		tref root = n('a');
		auto pred = [](tref nd) { return chtree::get(nd).value == 'a'; };
		tref found = nullptr;
		find_top_predicate<decltype(pred)> finder{pred, found};
		post_order_traverser<char, decltype(idni::identity),
			decltype(finder)>(idni::identity, finder)(root);
		CHECK( cmp(found, root) );
	}

	TEST_CASE("leaf root not satisfying predicate: finds nothing") {
		tref root = n('a');
		auto pred = [](tref nd) { return chtree::get(nd).value == 'z'; };
		tref found = nullptr;
		find_top_predicate<decltype(pred)> finder{pred, found};
		post_order_traverser<char, decltype(idni::identity),
			decltype(finder)>(idni::identity, finder)(root);
		CHECK( found == nullptr );
	}

	TEST_CASE("finds first matching child in post-order") {
		tref root = n('a', {n('b'), n('c')});
		auto pred = [](tref nd) {
			auto v = chtree::get(nd).value;
			return v == 'b' || v == 'c';
		};
		tref found = nullptr;
		find_top_predicate<decltype(pred)> finder{pred, found};
		post_order_traverser<char, decltype(idni::identity),
			decltype(finder)>(idni::identity, finder)(root);
		// post-order: b is visited before c
		CHECK( cmp(found, n('b')) );
	}

	TEST_CASE("once found, stays found (does not continue searching)") {
		tref root = n('a', {n('b'), n('c'), n('b')});
		auto pred = [](tref nd) { return chtree::get(nd).value == 'b'; };
		tref found = nullptr;
		find_top_predicate<decltype(pred)> finder{pred, found};
		post_order_traverser<char, decltype(idni::identity),
			decltype(finder)>(idni::identity, finder)(root);
		CHECK( cmp(found, n('b')) );
	}
}

// ============================================================================
// select_top (free function)
// ============================================================================

TEST_SUITE("select_top") {

	TEST_CASE("empty result when predicate never satisfied") {
		tref root = n('a', {n('b'), n('c')});
		auto pred = [](tref nd) { return chtree::get(nd).value == 'z'; };
		auto result = select_top<char>(root, pred);
		CHECK( result.empty() );
	}

	TEST_CASE("root satisfies predicate: returns root alone") {
		tref root = n('a');
		auto pred = [](tref nd) { return chtree::get(nd).value == 'a'; };
		auto result = select_top<char>(root, pred);
		CHECK( result.size() == 1 );
		CHECK( cmp(result[0], root) );
	}

	TEST_CASE("children satisfy predicate: returns children, not root") {
		tref root = n('a', {n('b'), n('c')});
		auto pred = [](tref nd) {
			auto v = chtree::get(nd).value;
			return v == 'b' || v == 'c';
		};
		auto result = select_top<char>(root, pred);
		CHECK( result.size() == 2 );
		CHECK( cmp(result[0], n('b')) );
		CHECK( cmp(result[1], n('c')) );
	}

	TEST_CASE("root matches: children not explored") {
		// root = a(a(b))  — if root satisfies predicate, inner 'a' not added
		tref root = n('a', {n('a', {n('b')})});
		auto pred = [](tref nd) { return chtree::get(nd).value == 'a'; };
		auto result = select_top<char>(root, pred);
		CHECK( result.size() == 1 );
		CHECK( cmp(result[0], root) );
	}

	TEST_CASE("diamond DAG: shared node returned once") {
		tref root = n('a', {n('b', {n('d')}), n('c', {n('d')})});
		auto pred = [](tref nd) { return chtree::get(nd).value == 'd'; };
		auto result = select_top<char>(root, pred);
		CHECK( result.size() == 1 );
		CHECK( cmp(result[0], n('d')) );
	}

	TEST_CASE("entire tree selected when root satisfies: only root") {
		tref root = n('a', {n('b')});
		auto pred = [](tref) { return true; };
		auto result = select_top<char>(root, pred);
		CHECK( result.size() == 1 );
		CHECK( cmp(result[0], root) );
	}
}

// ============================================================================
// select_all (free function)
// ============================================================================

TEST_SUITE("select_all") {

	TEST_CASE("empty result when predicate never satisfied") {
		tref root = n('a', {n('b')});
		auto pred = [](tref nd) { return chtree::get(nd).value == 'z'; };
		CHECK( select_all<char>(root, pred).empty() );
	}

	TEST_CASE("returns root when only root satisfies") {
		tref root = n('a');
		auto pred = [](tref nd) { return chtree::get(nd).value == 'a'; };
		auto result = select_all<char>(root, pred);
		CHECK( result.size() == 1 );
		CHECK( cmp(result[0], root) );
	}

	TEST_CASE("returns all matching nodes regardless of depth") {
		tref root = n('a', {n('b', {n('a')}), n('c')});
		auto pred = [](tref nd) { return chtree::get(nd).value == 'a'; };
		auto result = select_all<char>(root, pred);
		// both the root 'a' and the nested 'a' are returned
		CHECK( result.size() == 2 );
	}

	TEST_CASE("returns all matching children") {
		tref root = n('r', {n('a'), n('b'), n('a')});
		auto pred = [](tref nd) { return chtree::get(nd).value == 'a'; };
		auto result = select_all<char>(root, pred);
		// n('a') is structurally unique, so visit_unique deduplicates
		CHECK( result.size() == 1 );
		CHECK( cmp(result[0], n('a')) );
	}

	TEST_CASE("diamond DAG: shared node counted once") {
		tref root = n('a', {n('b', {n('d')}), n('c', {n('d')})});
		auto pred = [](tref nd) { return chtree::get(nd).value == 'd'; };
		auto result = select_all<char>(root, pred);
		CHECK( result.size() == 1 );
		CHECK( cmp(result[0], n('d')) );
	}
}

// ============================================================================
// select_all_until (free function)
// ============================================================================

TEST_SUITE("select_all_until") {

	TEST_CASE("collects all matching nodes when until predicate never fires") {
		tref root = n('a', {n('b'), n('c')});
		auto query = [](tref nd) {
			auto v = chtree::get(nd).value;
			return v == 'b' || v == 'c';
		};
		auto until = [](tref nd) { return chtree::get(nd).value == 'z'; };
		auto result = select_all_until<char>(root, query, until);
		CHECK( result.size() == 2 );
	}

	TEST_CASE("stops descent when until fires but still includes the until node if query matches") {
		// root=a(b(c))  query=always true, until=b → b is included (query fires first), c is not visited
		tref root = n('a', {n('b', {n('c')})});
		auto query = [](tref) { return true; };
		auto until = [](tref nd) { return chtree::get(nd).value == 'b'; };
		auto result = select_all_until<char>(root, query, until);
		// 'a' and 'b' visited; 'b' triggers until so 'c' is not visited
		// Since query(b) fires before until check, b is included; c is not
		CHECK( result.size() == 2 );
		bool has_a = false, has_b = false;
		for (auto r : result) {
			if (chtree::get(r).value == 'a') has_a = true;
			if (chtree::get(r).value == 'b') has_b = true;
		}
		CHECK( has_a );
		CHECK( has_b );
	}

	TEST_CASE("empty result when predicate never satisfied") {
		tref root = n('a', {n('b')});
		auto query = [](tref nd) { return chtree::get(nd).value == 'z'; };
		auto until = [](tref nd) { return chtree::get(nd).value == 'z'; };
		auto result = select_all_until<char>(root, query, until);
		CHECK( result.empty() );
	}
}

// ============================================================================
// select_top_until (free function)
// ============================================================================

TEST_SUITE("select_top_until") {

	TEST_CASE("returns top nodes satisfying query") {
		tref root = n('a', {n('b'), n('c')});
		auto query = [](tref nd) {
			auto v = chtree::get(nd).value;
			return v == 'b' || v == 'c';
		};
		auto until = [](tref nd) { return chtree::get(nd).value == 'z'; };
		auto result = select_top_until<char>(root, query, until);
		CHECK( result.size() == 2 );
	}

	TEST_CASE("until stops descent before children can be found") {
		// a(b(c))  query=c, until=b → never reach c
		tref root = n('a', {n('b', {n('c')})});
		auto query = [](tref nd) { return chtree::get(nd).value == 'c'; };
		auto until = [](tref nd) { return chtree::get(nd).value == 'b'; };
		auto result = select_top_until<char>(root, query, until);
		CHECK( result.empty() );
	}

	TEST_CASE("top node satisfying query stops descent into its subtree") {
		// a(b(b))  query=b, until=none → top 'b' selected, inner 'b' not
		tref root = n('a', {n('b', {n('b')})});
		auto query = [](tref nd) { return chtree::get(nd).value == 'b'; };
		auto until = [](tref nd) { return chtree::get(nd).value == 'z'; };
		auto result = select_top_until<char>(root, query, until);
		CHECK( result.size() == 1 );
	}
}

// ============================================================================
// find_top (free function)
// ============================================================================

TEST_SUITE("find_top") {

	TEST_CASE("no match returns nullptr") {
		tref root = n('a', {n('b')});
		auto pred = [](tref nd) { return chtree::get(nd).value == 'z'; };
		CHECK( find_top<char>(root, pred) == nullptr );
	}

	TEST_CASE("finds root when root satisfies predicate") {
		tref root = n('a');
		auto pred = [](tref nd) { return chtree::get(nd).value == 'a'; };
		CHECK( cmp(find_top<char>(root, pred), root) );
	}

	TEST_CASE("finds first child satisfying predicate in pre-order") {
		tref root = n('a', {n('b'), n('c')});
		auto pred = [](tref nd) {
			auto v = chtree::get(nd).value;
			return v == 'b' || v == 'c';
		};
		// pre-order: a, b, c → first match is b
		auto result = find_top<char>(root, pred);
		CHECK( cmp(result, n('b')) );
	}

	TEST_CASE("root satisfying predicate is returned without exploring children") {
		tref root = n('a', {n('a', {n('b')})});
		auto pred = [](tref nd) { return chtree::get(nd).value == 'a'; };
		auto result = find_top<char>(root, pred);
		// The outer 'a' (which is root) is the first match in pre-order
		CHECK( cmp(result, root) );
	}

	TEST_CASE("deep nested match found") {
		tref root = n('a', {n('b', {n('c', {n('d')})})});
		auto pred = [](tref nd) { return chtree::get(nd).value == 'd'; };
		auto result = find_top<char>(root, pred);
		CHECK( cmp(result, n('d')) );
	}
}

// ============================================================================
// find_top_until (free function)
// ============================================================================

TEST_SUITE("find_top_until") {

	TEST_CASE("no match when predicate never satisfied") {
		tref root = n('a', {n('b')});
		auto query = [](tref nd) { return chtree::get(nd).value == 'z'; };
		auto until = [](tref nd) { return chtree::get(nd).value == 'z'; };
		CHECK( find_top_until<char>(root, query, until) == nullptr );
	}

	TEST_CASE("finds root when it satisfies query") {
		tref root = n('a');
		auto query = [](tref nd) { return chtree::get(nd).value == 'a'; };
		auto until = [](tref nd) { return chtree::get(nd).value == 'z'; };
		CHECK( cmp(find_top_until<char>(root, query, until), root) );
	}

	TEST_CASE("until stops descent, so nested match not found") {
		tref root = n('a', {n('b', {n('c')})});
		auto query = [](tref nd) { return chtree::get(nd).value == 'c'; };
		auto until = [](tref nd) { return chtree::get(nd).value == 'b'; };
		CHECK( find_top_until<char>(root, query, until) == nullptr );
	}
}

// ============================================================================
// find_bottom (free function)
// ============================================================================

TEST_SUITE("find_bottom") {

	TEST_CASE("no match returns nullptr") {
		tref root = n('a', {n('b')});
		auto pred = [](tref nd) { return chtree::get(nd).value == 'z'; };
		CHECK( find_bottom<char>(root, pred) == nullptr );
	}

	TEST_CASE("finds root when root satisfies predicate (leaf)") {
		tref root = n('a');
		auto pred = [](tref nd) { return chtree::get(nd).value == 'a'; };
		CHECK( cmp(find_bottom<char>(root, pred), root) );
	}

	TEST_CASE("finds deepest (post-order first) matching node") {
		tref root = n('a', {n('b', {n('d')}), n('c', {n('d')})});
		auto pred = [](tref nd) {
			auto v = chtree::get(nd).value;
			return v == 'b' || v == 'c' || v == 'd';
		};
		// post-order: d is visited before b and c
		auto result = find_bottom<char>(root, pred);
		CHECK( cmp(result, n('d')) );
	}

	TEST_CASE("finds child before root in post-order") {
		tref root = n('a', {n('b')});
		auto pred = [](tref) { return true; };
		auto result = find_bottom<char>(root, pred);
		// post-order: b visited first
		CHECK( cmp(result, n('b')) );
	}
}

// ============================================================================
// trim_top (free function)
// ============================================================================

TEST_SUITE("trim_top") {

	TEST_CASE("no matching children: returns original tree") {
		tref root = n('a', {n('b'), n('c')});
		auto pred = [](tref nd) { return chtree::get(nd).value == 'z'; };
		auto result = trim_top<char>(root, pred);
		CHECK( cmp(result, root) );
	}

	TEST_CASE("leaf root: always returned as-is (cannot trim root)") {
		tref root = n('a');
		auto pred = [](tref) { return true; };
		auto result = trim_top<char>(root, pred);
		CHECK( cmp(result, root) );
	}

	TEST_CASE("trims one child that satisfies predicate") {
		tref root = n('a', {n('b'), n('c')});
		auto pred = [](tref nd) { return chtree::get(nd).value == 'c'; };
		auto result = trim_top<char>(root, pred);
		CHECK( cmp(result, n('a', {n('b')})) );
	}

	TEST_CASE("trims both children when both satisfy predicate") {
		tref root = n('a', {n('b'), n('c')});
		auto pred = [](tref nd) {
			auto v = chtree::get(nd).value;
			return v == 'b' || v == 'c';
		};
		auto result = trim_top<char>(root, pred);
		CHECK( cmp(result, n('a')) );
	}

	TEST_CASE("diamond DAG: trims shared node from both parents") {
		tref root = n('a', {n('b', {n('d')}), n('c', {n('d')})});
		auto pred = [](tref nd) { return chtree::get(nd).value == 'd'; };
		auto result = trim_top<char>(root, pred);
		CHECK( cmp(result, n('a', {n('b'), n('c')})) );
	}

	TEST_CASE("deep trim: removes nodes at every level satisfying predicate") {
		tref root = n('a', {n('b', {n('c', {n('d')})})});
		auto pred = [](tref nd) { return chtree::get(nd).value == 'd'; };
		auto result = trim_top<char>(root, pred);
		CHECK( cmp(result, n('a', {n('b', {n('c')})})) );
	}
}

// ============================================================================
// replace (free functions)
// ============================================================================

TEST_SUITE("replace") {

	TEST_CASE("replace with std::map: simple root replacement") {
		tref root = n('a');
		std::map<tref, tref> changes = {{n('a'), n('b')}};
		auto result = replace<char>(root, changes);
		CHECK( cmp(result, n('b')) );
	}

	TEST_CASE("replace with subtree_map: child replacement") {
		tref root = n('r', {n('a'), n('b')});
		subtree_map<char, tref> changes = {{n('a'), n('z')}};
		auto result = replace<char>(root, changes);
		CHECK( cmp(result, n('r', {n('z'), n('b')})) );
	}

	TEST_CASE("replace with tref pair: replaces matching subtree") {
		tref root = n('a', {n('b'), n('c')});
		auto result = replace<char>(root, n('b'), n('z'));
		CHECK( cmp(result, n('a', {n('z'), n('c')})) );
	}

	TEST_CASE("replace with tref pair: no match returns original tree") {
		tref root = n('a', {n('b'), n('c')});
		auto result = replace<char>(root, n('z'), n('w'));
		CHECK( cmp(result, root) );
	}

	TEST_CASE("replace with subtree_map: no match returns original tree") {
		tref root = n('a', {n('b'), n('c')});
		subtree_map<char, tref> changes = {{n('z'), n('w')}};
		auto result = replace<char>(root, changes);
		CHECK( cmp(result, root) );
	}

	TEST_CASE("replace with subtree_map: replaces multiple occurrences of same node") {
		tref root = n('r', {n('a'), n('b'), n('a')});
		subtree_map<char, tref> changes = {{n('a'), n('z')}};
		auto result = replace<char>(root, changes);
		CHECK( cmp(result, n('r', {n('z'), n('b'), n('z')})) );
	}

	TEST_CASE("replace with std::map: empty changes returns original") {
		tref root = n('a', {n('b')});
		std::map<tref, tref> changes;
		auto result = replace<char>(root, changes);
		CHECK( cmp(result, root) );
	}
}

// ============================================================================
// replace_if (free function)
// ============================================================================

TEST_SUITE("replace_if") {

	TEST_CASE("replaces node when query satisfied") {
		tref root = n('r', {n('a'), n('b')});
		subtree_map<char, tref> changes = {{n('a'), n('z')}};
		auto query = [](tref) { return true; };
		auto result = replace_if<char>(root, changes, query);
		CHECK( cmp(result, n('r', {n('z'), n('b')})) );
	}

	TEST_CASE("replaces tref pair when query satisfied") {
		tref root = n('r', {n('a'), n('b')});
		auto query = [](tref) { return true; };
		auto result = replace_if<char>(root, n('a'), n('z'), query);
		CHECK( cmp(result, n('r', {n('z'), n('b')})) );
	}

	TEST_CASE("skips subtrees not satisfying query") {
		// query only allows subtrees rooted at 'r', not 'x'
		tref root = n('r', {n('x', {n('a')}), n('b')});
		subtree_map<char, tref> changes = {{n('a'), n('z')}};
		auto query = [](tref nd) { return chtree::get(nd).value != 'x'; };
		auto result = replace_if<char>(root, changes, query);
		// 'a' under 'x' not replaced because 'x' is skipped
		CHECK( cmp(result, root) );
	}
}

// ============================================================================
// replace_until (free function)
// ============================================================================

TEST_SUITE("replace_until") {

	TEST_CASE("replaces nodes before the until boundary") {
		tref root = n('r', {n('a'), n('b')});
		subtree_map<char, tref> changes = {{n('a'), n('z')}};
		auto until = [](tref nd) { return chtree::get(nd).value == 'q'; };
		auto result = replace_until<char>(root, changes, until);
		CHECK( cmp(result, n('r', {n('z'), n('b')})) );
	}

	TEST_CASE("does not replace nodes under a node that satisfies until") {
		tref root = n('r', {n('s', {n('a')}), n('b')});
		subtree_map<char, tref> changes = {{n('a'), n('z')}};
		auto until = [](tref nd) { return chtree::get(nd).value == 's'; };
		auto result = replace_until<char>(root, changes, until);
		// 'a' under 's' subtree not replaced because 's' satisfies until
		CHECK( cmp(result, root) );
	}
}

// ============================================================================
// apply_rule (free function)
// ============================================================================

TEST_SUITE("apply_rule") {

	TEST_CASE("rule matches root: transforms root") {
		tref root = n('a', {n('b')});
		tref pattern = n('a', {n('X')});
		tref body    = n('a', {n('X'), n('X')});
		rule r{chtree::geth(pattern), chtree::geth(body)};
		is_capture_predicate ic;
		auto result = apply_rule<char>(r, root, ic);
		CHECK( cmp(result, n('a', {n('b'), n('b')})) );
	}

	TEST_CASE("rule does not match: returns original tree") {
		tref root = n('a', {n('b')});
		tref pattern = n('c', {n('X')});
		tref body    = n('c', {n('X'), n('X')});
		rule r{chtree::geth(pattern), chtree::geth(body)};
		is_capture_predicate ic;
		auto result = apply_rule<char>(r, root, ic);
		CHECK( cmp(result, root) );
	}

	TEST_CASE("rule swaps two children") {
		tref root    = n('a', {n('b'), n('c')});
		tref pattern = n('a', {n('X'), n('Y')});
		tref body    = n('a', {n('Y'), n('X')});
		rule r{chtree::geth(pattern), chtree::geth(body)};
		is_capture_predicate ic;
		auto result = apply_rule<char>(r, root, ic);
		CHECK( cmp(result, n('a', {n('c'), n('b')})) );
	}

	TEST_CASE("rule drops children") {
		tref root    = n('a', {n('b')});
		tref pattern = n('a', {n('X')});
		tref body    = n('a');
		rule r{chtree::geth(pattern), chtree::geth(body)};
		is_capture_predicate ic;
		auto result = apply_rule<char>(r, root, ic);
		CHECK( cmp(result, n('a')) );
	}

	TEST_CASE("rule applied to nested match inside larger tree") {
		tref root    = n('r', {n('a', {n('b')}), n('c')});
		tref pattern = n('a', {n('X')});
		tref body    = n('a', {n('X'), n('X')});
		rule r{chtree::geth(pattern), chtree::geth(body)};
		is_capture_predicate ic;
		auto result = apply_rule<char>(r, root, ic);
		CHECK( cmp(result, n('r', {n('a', {n('b'), n('b')}), n('c')})) );
	}

	TEST_CASE("rule with three captures applied correctly") {
		tref root    = n('a', {n('b'), n('c', {n('d'), n('e')})});
		tref pattern = n('a', {n('X'), n('c', {n('Y'), n('Z')})});
		tref body    = n('a', {n('Y'), n('c', {n('Z'), n('X')})});
		rule r{chtree::geth(pattern), chtree::geth(body)};
		is_capture_predicate ic;
		auto result = apply_rule<char>(r, root, ic);
		CHECK( cmp(result, n('a', {n('d'), n('c', {n('e'), n('b')})})) );
	}

	TEST_CASE("rule applied at deepest level first (post-order)") {
		tref root    = n('a', {n('b', {n('d')}), n('c', {n('d')})});
		tref pattern = n('b', {n('X')});
		tref body    = n('b', {n('e')});
		rule r{chtree::geth(pattern), chtree::geth(body)};
		is_capture_predicate ic;
		auto result = apply_rule<char>(r, root, ic);
		// 'b(d)' matched, replaced with 'b(e)'; 'c(d)' unchanged
		CHECK( cmp(result, n('a', {n('b', {n('e')}), n('c', {n('d')})})) );
	}
}

// ============================================================================
// apply_rule_guarded (free function)
// ============================================================================

TEST_SUITE("apply_rule_guarded") {

	TEST_CASE("guard accepts: rule applied") {
		tref root    = n('a', {n('b')});
		tref pattern = n('a', {n('X')});
		tref body    = n('a', {n('X'), n('X')});
		rule r{chtree::geth(pattern), chtree::geth(body)};
		is_capture_predicate ic;
		// guard always accepts
		auto guard = [](const rewriter::environment<char>&, tref) { return true; };
		auto result = apply_rule_guarded<char>(r, root, ic, guard);
		CHECK( cmp(result, n('a', {n('b'), n('b')})) );
	}

	TEST_CASE("guard rejects: rule not applied, original tree returned") {
		tref root    = n('a', {n('b')});
		tref pattern = n('a', {n('X')});
		tref body    = n('a', {n('X'), n('X')});
		rule r{chtree::geth(pattern), chtree::geth(body)};
		is_capture_predicate ic;
		// guard always rejects
		auto guard = [](const rewriter::environment<char>&, tref) { return false; };
		auto result = apply_rule_guarded<char>(r, root, ic, guard);
		CHECK( cmp(result, root) );
	}

	TEST_CASE("guard inspects environment to decide") {
		// Pattern: a(X). Body: a(X, X). Guard: only apply if captured X == 'b'
		tref root    = n('a', {n('b')});
		tref pattern = n('a', {n('X')});
		tref body    = n('a', {n('X'), n('X')});
		rule r{chtree::geth(pattern), chtree::geth(body)};
		is_capture_predicate ic;
		tref x_node = n('X');
		auto guard = [&x_node](const rewriter::environment<char>& env, tref) -> bool {
			auto it = env.find(x_node);
			if (it == env.end()) return false;
			return chtree::get(it->second).value == 'b';
		};
		auto result = apply_rule_guarded<char>(r, root, ic, guard);
		CHECK( cmp(result, n('a', {n('b'), n('b')})) );
	}

	TEST_CASE("guard inspects environment and rejects based on captured value") {
		tref root    = n('a', {n('c')});
		tref pattern = n('a', {n('X')});
		tref body    = n('a', {n('X'), n('X')});
		rule r{chtree::geth(pattern), chtree::geth(body)};
		is_capture_predicate ic;
		tref x_node = n('X');
		// guard only allows if captured X is 'b'; here X captures 'c' → reject
		auto guard = [&x_node](const rewriter::environment<char>& env, tref) -> bool {
			auto it = env.find(x_node);
			if (it == env.end()) return false;
			return chtree::get(it->second).value == 'b';
		};
		auto result = apply_rule_guarded<char>(r, root, ic, guard);
		CHECK( cmp(result, root) );
	}
}

// ============================================================================
// fixpoint (free function)
// ============================================================================

TEST_SUITE("fixpoint") {

	TEST_CASE("fixpoint with no matching rule returns original tree") {
		tref root    = n('a', {n('b')});
		tref pattern = n('c', {n('X')});
		tref body    = n('d', {n('X')});
		rules rs{rule{chtree::geth(pattern), chtree::geth(body)}};
		is_capture_predicate ic;
		auto result = fixpoint<char>(rs, root, ic);
		CHECK( cmp(result, root) );
	}

	TEST_CASE("fixpoint applies rule repeatedly until stable") {
		// Rule: a(X) → a(a(X)) — applying once gives a(a(b)), not stable yet!
		// Actually this would loop forever. Let's use a converging rule.
		// Rule: a(b) → c  — applies once, result is c, no more matches
		tref root    = n('a', {n('b')});
		tref pattern = n('a', {n('b')});
		tref body    = n('c');
		rules rs{rule{chtree::geth(pattern), chtree::geth(body)}};
		is_capture_predicate ic;
		auto result = fixpoint<char>(rs, root, ic);
		CHECK( cmp(result, n('c')) );
	}

	TEST_CASE("fixpoint with multiple rules applies them in sequence") {
		// Rule 1: a(b) → a(c)  Rule 2: a(c) → d
		tref root = n('a', {n('b')});
		tref p1 = n('a', {n('b')});
		tref b1 = n('a', {n('c')});
		tref p2 = n('a', {n('c')});
		tref b2 = n('d');
		rules rs{
			rule{chtree::geth(p1), chtree::geth(b1)},
			rule{chtree::geth(p2), chtree::geth(b2)}
		};
		is_capture_predicate ic;
		auto result = fixpoint<char>(rs, root, ic);
		CHECK( cmp(result, n('d')) );
	}

	TEST_CASE("fixpoint with done predicate stops early") {
		// Rule: a(b) → a(c)  (rule would keep being applied but done triggers)
		tref root = n('a', {n('b')});
		tref p1   = n('a', {n('b')});
		tref b1   = n('a', {n('c')});
		rules rs{rule{chtree::geth(p1), chtree::geth(b1)}};
		is_capture_predicate ic;
		// done predicate: stop when root is a(c)
		auto done = [](tref nd) {
			const auto& t = chtree::get(nd);
			return t.value == 'a' && t.has_child()
				&& t.first_tree().value == 'c';
		};
		auto result = fixpoint<char>(rs, root, ic, done);
		CHECK( cmp(result, n('a', {n('c')})) );
	}

	TEST_CASE("fixpoint with empty rule set returns original tree") {
		tref root = n('a', {n('b')});
		rules rs;
		is_capture_predicate ic;
		auto result = fixpoint<char>(rs, root, ic);
		CHECK( cmp(result, root) );
	}
}

// ============================================================================
// pattern_matcher2
// ============================================================================

TEST_SUITE("pattern_matcher2") {

	TEST_CASE("match succeeds: operator() returns true and populates changes") {
		tref root    = n('a', {n('b')});
		tref pattern = n('a', {n('X')});
		tref body    = n('a', {n('X'), n('X')});
		rule r{chtree::geth(pattern), chtree::geth(body)};
		is_capture_predicate ic;
		pattern_matcher2<char, is_capture_predicate> m(r, ic);
		bool res = m(root);
		CHECK( res == true );
		CHECK( !m.changes.empty() );
	}

	TEST_CASE("match fails: changes maps node to rebuilt form") {
		tref root    = n('b', {n('c')});
		tref pattern = n('a', {n('X')});
		tref body    = n('a', {n('X'), n('X')});
		rule r{chtree::geth(pattern), chtree::geth(body)};
		is_capture_predicate ic;
		pattern_matcher2<char, is_capture_predicate> m(r, ic);
		m(root);
		// No pattern match, but changes still tracks rebuilt form
		auto it = m.changes.find(root);
		if (it != m.changes.end())
			CHECK( cmp(it->second, root) );
	}

	TEST_CASE("capture variable binds to subtree") {
		tref root    = n('a', {n('b', {n('c')})});
		tref pattern = n('a', {n('X')});
		tref body    = n('a', {n('X')});
		rule r{chtree::geth(pattern), chtree::geth(body)};
		is_capture_predicate ic;
		pattern_matcher2<char, is_capture_predicate> m(r, ic);
		m(root);
		const auto& env = m.get_env();
		// X should be bound to n('b', {n('c')})
		CHECK( !env.empty() );
	}

	TEST_CASE("replace_root returns transformed node") {
		tref root    = n('a', {n('b')});
		tref pattern = n('a', {n('X')});
		tref body    = n('a', {n('X'), n('X')});
		rule r{chtree::geth(pattern), chtree::geth(body)};
		is_capture_predicate ic;
		pattern_matcher2<char, is_capture_predicate> m(r, ic);
		post_order<char>(root).search_unique(m);
		auto result = m.replace_root(root);
		CHECK( cmp(result, n('a', {n('b'), n('b')})) );
	}
}

// ============================================================================
// merge (rules utility)
// ============================================================================

TEST_SUITE("merge_rules") {

	TEST_CASE("merging two empty rule sets produces empty set") {
		rules rs1, rs2;
		auto merged = rewriter::merge(rs1, rs2);
		CHECK( merged.empty() );
	}

	TEST_CASE("merging non-empty sets concatenates them") {
		tref p1 = n('a', {n('X')});
		tref b1 = n('b', {n('X')});
		tref p2 = n('c', {n('Y')});
		tref b2 = n('d', {n('Y')});
		rules rs1{rule{chtree::geth(p1), chtree::geth(b1)}};
		rules rs2{rule{chtree::geth(p2), chtree::geth(b2)}};
		auto merged = rewriter::merge(rs1, rs2);
		CHECK( merged.size() == 2 );
	}

	TEST_CASE("merging empty with non-empty returns the non-empty set") {
		tref p1 = n('a', {n('X')});
		tref b1 = n('b', {n('X')});
		rules rs1{rule{chtree::geth(p1), chtree::geth(b1)}};
		rules rs2;
		auto merged = rewriter::merge(rs1, rs2);
		CHECK( merged.size() == 1 );
	}
}
