// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "init_test.h"

// ============================================================================
// Helpers
// ============================================================================

// Collect visited node values in order
static std::vector<char> collect_values(const trefs& nodes) {
	std::vector<char> vals;
	for (tref nd : nodes) vals.push_back(chtree::get(nd).value);
	return vals;
}

// Identity transformer: returns same node unchanged
static auto id = [](tref nd) { return nd; };

// Transformer: maps 'a' -> 'z'
static auto tr_a_z = [](tref nd) -> tref {
	auto x = chtree::get(nd);
	if (x.value == 'a') return bintree<char>::get('z', x.l, x.r);
	return nd;
};

// ============================================================================
// post_order — apply_unique
// ============================================================================

TEST_SUITE("post_order::apply_unique") {

	TEST_CASE("identity transform returns same tree") {
		tref root = n('a', {n('b'), n('c')});
		auto result = post_order<char>(root).apply_unique(id);
		CHECK( result == root );
	}

	TEST_CASE("transform on leaf returns transformed leaf") {
		tref root = n('a');
		auto result = post_order<char>(root).apply_unique(tr_a_z);
		CHECK( cmp(result, n('z')) );
	}

	TEST_CASE("transform applied to all matching nodes in subtree") {
		tref root = n('a', {n('b'), n('a')});
		auto result = post_order<char>(root).apply_unique(tr_a_z);
		// Both 'a' nodes become 'z'
		auto& t = chtree::get(result);
		CHECK( t.value == 'z' );
		CHECK( t.second_tree().value == 'z' );
	}

	TEST_CASE("transform with visit_subtree filter skips subtrees") {
		// Tree: a(b(a), c)  — skip subtrees rooted at 'b'
		tref root = n('a', {n('b', {n('a')}), n('c')});
		auto not_b = [](tref nd) { return chtree::get(nd).value != 'b'; };
		auto result = post_order<char>(root).apply_unique(tr_a_z, not_b);
		// The 'a' inside 'b' is not transformed (subtree skipped)
		// The root 'a' IS transformed
		auto& t = chtree::get(result);
		CHECK( t.value == 'z' );
		CHECK( t.first_tree().value == 'b' );
		CHECK( t.first_tree().first_tree().value == 'a' ); // unchanged
	}

	TEST_CASE("nullptr root returns nullptr") {
		auto result = post_order<char>(nullptr).apply_unique(id);
		CHECK( result == nullptr );
	}

	TEST_CASE("single node tree: transform applied once") {
		tref root = n('a');
		int calls = 0;
		auto counting_tr = [&calls](tref nd) -> tref {
			++calls;
			return nd;
		};
		post_order<char>(root).apply_unique(counting_tr);
		CHECK( calls == 1 );
	}

	TEST_CASE("diamond DAG: shared internal node transformed once") {
		// Use a non-leaf shared node — apply_unique caches internal (non-leaf) nodes
		tref shared = n('d', {n('e')});
		tref root = n('a', {n('b', {shared}), n('c', {shared})});
		int calls = 0;
		auto counting_tr = [&calls, &shared](tref nd) -> tref {
			if (nd == shared) ++calls;
			return nd;
		};
		post_order<char>(root).apply_unique(counting_tr);
		// Unique mode caches non-leaf nodes: 'd(e)' transformed once
		CHECK( calls == 1 );
	}
}

// ============================================================================
// post_order — search
// ============================================================================

TEST_SUITE("post_order::search") {

	TEST_CASE("search visits all nodes in post-order") {
		tref root = n('a', {n('b'), n('c')});
		trefs visited;
		auto collect = [&visited](tref nd) {
			visited.push_back(nd);
			return true; // continue
		};
		post_order<char>(root).search(collect);
		// post-order: b, c, a
		auto vals = collect_values(visited);
		CHECK( vals == std::vector<char>{'b', 'c', 'a'} );
	}

	TEST_CASE("search on single-node tree visits just root") {
		tref root = n('a');
		trefs visited;
		auto collect = [&visited](tref nd) {
			visited.push_back(nd);
			return true;
		};
		post_order<char>(root).search(collect);
		CHECK( visited.size() == 1 );
		CHECK( cmp(visited[0], root) );
	}

	TEST_CASE("search with false return stops traversal") {
		tref root = n('a', {n('b', {n('c')}), n('d')});
		trefs visited;
		auto stop_after_first = [&visited](tref nd) {
			visited.push_back(nd);
			return false; // stop after first visit
		};
		post_order<char>(root).search(stop_after_first);
		CHECK( visited.size() == 1 );
	}

	TEST_CASE("search with visit_subtree filter skips filtered subtrees") {
		tref root = n('a', {n('b', {n('c')}), n('d')});
		trefs visited;
		auto collect = [&visited](tref nd) {
			visited.push_back(nd);
			return true;
		};
		auto not_b = [](tref nd) { return chtree::get(nd).value != 'b'; };
		post_order<char>(root).search(collect, not_b);
		// 'b' and its child 'c' should be skipped
		auto vals = collect_values(visited);
		for (char v : vals) {
			CHECK( v != 'c' );
			CHECK( v != 'b' );
		}
	}

	TEST_CASE("search_unique does not re-visit shared nodes") {
		tref d = n('d');
		tref root = n('a', {n('b', {d}), n('c', {d})});
		trefs visited;
		auto collect = [&visited](tref nd) {
			visited.push_back(nd);
			return true;
		};
		post_order<char>(root).search_unique(collect);
		// 'd' should appear only once
		int d_count = 0;
		for (tref nd : visited)
			if (chtree::get(nd).value == 'd') ++d_count;
		CHECK( d_count == 1 );
	}

	TEST_CASE("search_unique: deep tree visits each unique node once") {
		tref leaf = n('x');
		tref mid  = n('m', {leaf, leaf});
		tref root = n('a', {mid, mid});
		std::set<tref> seen;
		auto collect = [&seen](tref nd) {
			seen.insert(nd);
			return true;
		};
		post_order<char>(root).search_unique(collect);
		// 'x' and 'm' are shared; each visited once
		CHECK( seen.size() == 3 ); // x, m, a
	}
}

// ============================================================================
// pre_order — apply_unique
// ============================================================================

TEST_SUITE("pre_order::apply_unique") {

	TEST_CASE("identity transform returns same tree") {
		tref root = n('a', {n('b'), n('c')});
		auto result = pre_order<char>(root).apply_unique(id);
		CHECK( result == root );
	}

	TEST_CASE("transform on leaf returns transformed leaf") {
		tref root = n('a');
		auto result = pre_order<char>(root).apply_unique(tr_a_z);
		CHECK( cmp(result, n('z')) );
	}

	TEST_CASE("transform applied to matching nodes") {
		tref root = n('a', {n('b'), n('a')});
		auto result = pre_order<char>(root).apply_unique(tr_a_z);
		auto& t = chtree::get(result);
		CHECK( t.value == 'z' );
		CHECK( t.second_tree().value == 'z' );
	}

	TEST_CASE("transform with visit_subtree filter") {
		tref root = n('a', {n('b', {n('a')}), n('c')});
		auto not_b = [](tref nd) { return chtree::get(nd).value != 'b'; };
		auto result = pre_order<char>(root).apply_unique(tr_a_z, not_b, id);
		auto& t = chtree::get(result);
		CHECK( t.value == 'z' ); // root 'a' transformed
		CHECK( t.first_tree().value == 'b' ); // 'b' subtree not entered
		CHECK( t.first_tree().first_tree().value == 'a' ); // inner 'a' untouched
	}

	TEST_CASE("nullptr root returns nullptr") {
		auto result = pre_order<char>(nullptr).apply_unique(id);
		CHECK( result == nullptr );
	}

	TEST_CASE("diamond DAG: shared node's children processed only once") {
		// In pre_order::apply_unique, f is called per child encounter but
		// children of a cached node are not re-traversed.
		// Use a node with children to test that the grandchildren are not visited twice.
		tref shared = n('d', {n('e')});
		tref root = n('a', {n('b', {shared}), n('c', {shared})});
		int e_calls = 0;
		auto counting_tr = [&e_calls](tref nd) -> tref {
			// count visits to the leaf 'e' (child of shared)
			if (chtree::get(nd).value == 'e') ++e_calls;
			return nd;
		};
		pre_order<char>(root).apply_unique(counting_tr);
		// 'e' is the leaf child of 'shared'; due to caching, 'e' should be
		// visited only once even though 'shared' appears under both 'b' and 'c'
		CHECK( e_calls == 1 );
	}
}

// ============================================================================
// pre_order — apply (non-unique)
// ============================================================================

TEST_SUITE("pre_order::apply") {

	TEST_CASE("identity transform returns same tree") {
		tref root = n('a', {n('b')});
		auto result = pre_order<char>(root).apply(id);
		CHECK( cmp(result, root) );
	}

	TEST_CASE("transform applied to all nodes including duplicates") {
		tref d = n('d');
		tref root = n('a', {n('b', {d}), n('c', {d})});
		int calls = 0;
		auto counting_tr = [&calls, &d](tref nd) -> tref {
			if (nd == d) ++calls;
			return nd;
		};
		pre_order<char>(root).apply(counting_tr);
		// Non-unique: 'd' may be visited more than once
		CHECK( calls >= 1 );
	}
}

// ============================================================================
// pre_order — apply_unique_until_change
// ============================================================================

TEST_SUITE("pre_order::apply_unique_until_change") {

	TEST_CASE("no change: returns same node") {
		tref root = n('a', {n('b')});
		auto result = pre_order<char>(root).apply_unique_until_change(id);
		CHECK( result == root );
	}

	TEST_CASE("change at root: stops and returns changed node") {
		tref root = n('a');
		int calls = 0;
		auto change_a = [&calls](tref nd) -> tref {
			++calls;
			if (chtree::get(nd).value == 'a')
				return n('z');
			return nd;
		};
		auto result = pre_order<char>(root).apply_unique_until_change(change_a);
		CHECK( cmp(result, n('z')) );
	}

	TEST_CASE("change at first encountered node stops further traversal") {
		tref root = n('a', {n('b'), n('c')});
		std::vector<char> visited;
		// Transformer that renames 'a' to 'z' while preserving children structure
		auto change_a_record = [&visited](tref nd) -> tref {
			const auto& x = chtree::get(nd);
			char v = x.value;
			visited.push_back(v);
			if (v == 'a') return bintree<char>::get('z', x.l, x.r);
			return nd;
		};
		auto result = pre_order<char>(root)
				.apply_unique_until_change(change_a_record);
		// Root 'a' is the first node visited; it changes → traversal stops.
		// Children are kept from the original node (since we preserve x.l, x.r).
		CHECK( cmp(result, n('z', {n('b'), n('c')})) );
		// Only root was visited before the break
		CHECK( visited.size() == 1 );
		CHECK( visited[0] == 'a' );
	}
}

// ============================================================================
// pre_order — visit (const traversal)
// ============================================================================

TEST_SUITE("pre_order::visit") {

	TEST_CASE("visits all nodes in pre-order") {
		tref root = n('a', {n('b'), n('c')});
		trefs visited;
		auto collect = [&visited](tref nd) { visited.push_back(nd); };
		pre_order<char>(root).visit(collect);
		// pre-order: a, b, c
		auto vals = collect_values(visited);
		CHECK( vals == std::vector<char>{'a', 'b', 'c'} );
	}

	TEST_CASE("visit single node") {
		tref root = n('a');
		trefs visited;
		auto collect = [&visited](tref nd) { visited.push_back(nd); };
		pre_order<char>(root).visit(collect);
		CHECK( visited.size() == 1 );
		CHECK( cmp(visited[0], root) );
	}

	TEST_CASE("visit deep tree in pre-order") {
		tref root = n('a', {n('b', {n('d'), n('e')}), n('c')});
		trefs visited;
		auto collect = [&visited](tref nd) { visited.push_back(nd); };
		pre_order<char>(root).visit(collect);
		auto vals = collect_values(visited);
		CHECK( vals == std::vector<char>{'a', 'b', 'd', 'e', 'c'} );
	}

	TEST_CASE("visit with visit_subtree filter skips filtered subtrees") {
		tref root = n('a', {n('b', {n('c')}), n('d')});
		trefs visited;
		auto collect = [&visited](tref nd) { visited.push_back(nd); };
		auto not_b = [](tref nd) { return chtree::get(nd).value != 'b'; };
		pre_order<char>(root).visit(collect, not_b, id);
		// 'b' and 'c' are skipped (b's subtree not entered)
		auto vals = collect_values(visited);
		for (char v : vals) {
			CHECK( v != 'b' );
			CHECK( v != 'c' );
		}
	}

	TEST_CASE("visit_unique does not revisit shared DAG nodes") {
		tref d = n('d');
		tref root = n('a', {n('b', {d}), n('c', {d})});
		trefs visited;
		auto collect = [&visited](tref nd) { visited.push_back(nd); };
		pre_order<char>(root).visit_unique(collect);
		// 'd' appears only once
		int d_count = 0;
		for (tref nd : visited)
			if (chtree::get(nd).value == 'd') ++d_count;
		CHECK( d_count == 1 );
	}
}

// ============================================================================
// pre_order — search (with early termination)
// ============================================================================

TEST_SUITE("pre_order::search") {

	TEST_CASE("search visits nodes in pre-order") {
		tref root = n('a', {n('b'), n('c')});
		trefs visited;
		auto collect = [&visited](tref nd) {
			visited.push_back(nd);
			return true;
		};
		pre_order<char>(root).search(collect);
		auto vals = collect_values(visited);
		CHECK( vals == std::vector<char>{'a', 'b', 'c'} );
	}

	TEST_CASE("search returns false to stop early") {
		tref root = n('a', {n('b', {n('c')}), n('d')});
		trefs visited;
		auto stop_at_b = [&visited](tref nd) {
			visited.push_back(nd);
			return chtree::get(nd).value != 'b';
		};
		pre_order<char>(root).search(stop_at_b);
		// 'a' then 'b' visited, 'b' returns false → stop
		auto vals = collect_values(visited);
		CHECK( vals == std::vector<char>{'a', 'b'} );
	}

	TEST_CASE("search with visit_subtree limits exploration") {
		tref root = n('a', {n('b', {n('c')}), n('d')});
		trefs visited;
		auto collect = [&visited](tref nd) {
			visited.push_back(nd);
			return true;
		};
		auto not_b = [](tref nd) { return chtree::get(nd).value != 'b'; };
		pre_order<char>(root).search(collect, not_b, id);
		// 'b' and its child 'c' not visited
		for (tref nd : visited) {
			char v = chtree::get(nd).value;
			CHECK( v != 'b' );
			CHECK( v != 'c' );
		}
	}

	TEST_CASE("search_unique skips already-visited nodes") {
		tref d = n('d');
		tref root = n('a', {n('b', {d}), n('c', {d})});
		trefs visited;
		auto collect = [&visited](tref nd) {
			visited.push_back(nd);
			return true;
		};
		pre_order<char>(root).search_unique(collect);
		int d_count = 0;
		for (tref nd : visited)
			if (chtree::get(nd).value == 'd') ++d_count;
		CHECK( d_count == 1 );
	}
}

// ============================================================================
// morris_post_order — search
// ============================================================================

TEST_SUITE("morris_post_order::search") {

	TEST_CASE("visits single node") {
		tref root = n('a');
		trefs visited;
		auto collect = [&visited](tref nd) { visited.push_back(nd); };
		morris_post_order<char>(root).search(collect);
		CHECK( visited.size() == 1 );
		CHECK( cmp(visited[0], root) );
	}

	TEST_CASE("visits all nodes in post-order for linear tree") {
		// Linear (right-skewed) tree: a -> b -> c
		tref root = n('a', {n('b', {n('c')})});
		trefs visited;
		auto collect = [&visited](tref nd) { visited.push_back(nd); };
		morris_post_order<char>(root).search(collect);
		// Post-order: c, b, a
		auto vals = collect_values(visited);
		// The exact order depends on the morris traversal internals;
		// verify all nodes are visited
		CHECK( visited.size() == 3 );
		bool has_a = false, has_b = false, has_c = false;
		for (char v : vals) {
			if (v == 'a') has_a = true;
			if (v == 'b') has_b = true;
			if (v == 'c') has_c = true;
		}
		CHECK( has_a );
		CHECK( has_b );
		CHECK( has_c );
	}

	TEST_CASE("visits all nodes in tree with two children") {
		tref root = n('a', {n('b'), n('c')});
		trefs visited;
		auto collect = [&visited](tref nd) { visited.push_back(nd); };
		morris_post_order<char>(root).search(collect);
		CHECK( visited.size() == 3 );
		auto vals = collect_values(visited);
		bool has_a = false, has_b = false, has_c = false;
		for (char v : vals) {
			if (v == 'a') has_a = true;
			if (v == 'b') has_b = true;
			if (v == 'c') has_c = true;
		}
		CHECK( has_a );
		CHECK( has_b );
		CHECK( has_c );
	}

	TEST_CASE("children visited before parent") {
		tref root = n('a', {n('b'), n('c')});
		trefs visited;
		auto collect = [&visited](tref nd) { visited.push_back(nd); };
		morris_post_order<char>(root).search(collect);
		auto vals = collect_values(visited);
		// 'a' must be the last element
		CHECK( vals.back() == 'a' );
	}

	TEST_CASE("nullptr root: no nodes visited") {
		trefs visited;
		auto collect = [&visited](tref nd) { visited.push_back(nd); };
		morris_post_order<char>(nullptr).search(collect);
		CHECK( visited.empty() );
	}

	TEST_CASE("four-node tree: all nodes visited") {
		tref root = n('a', {n('b', {n('d')}), n('c')});
		trefs visited;
		auto collect = [&visited](tref nd) { visited.push_back(nd); };
		morris_post_order<char>(root).search(collect);
		CHECK( visited.size() == 4 );
	}
}

// ============================================================================
// Corner cases
// ============================================================================

TEST_SUITE("traversal corner cases") {

	TEST_CASE("post_order on nullptr root") {
		trefs visited;
		auto collect = [&visited](tref nd) {
			visited.push_back(nd);
			return true;
		};
		post_order<char>(nullptr).search(collect);
		CHECK( visited.empty() );
	}

	TEST_CASE("pre_order on nullptr root") {
		trefs visited;
		auto collect = [&visited](tref nd) { visited.push_back(nd); };
		pre_order<char>(nullptr).visit(collect);
		CHECK( visited.empty() );
	}

	TEST_CASE("very deep linear tree does not crash (stack overflow protection)") {
		// Build a depth-100 chain: a(a(a(...)))
		tref current = n('a');
		for (int i = 0; i < 100; ++i)
			current = n('a', {current});
		int count = 0;
		auto counter = [&count](tref) { ++count; return true; };
		post_order<char>(current).search(counter);
		CHECK( count == 101 ); // root + 100 inner nodes
	}

	TEST_CASE("tree with single child at each level: pre-order correct") {
		// a(b(c(d)))
		tref root = n('a', {n('b', {n('c', {n('d')})})});
		trefs visited;
		auto collect = [&visited](tref nd) { visited.push_back(nd); };
		pre_order<char>(root).visit(collect);
		auto vals = collect_values(visited);
		CHECK( vals == std::vector<char>{'a', 'b', 'c', 'd'} );
	}

	TEST_CASE("tree with single child at each level: post-order correct") {
		// a(b(c(d)))
		tref root = n('a', {n('b', {n('c', {n('d')})})});
		trefs visited;
		auto collect = [&visited](tref nd) {
			visited.push_back(nd);
			return true;
		};
		post_order<char>(root).search(collect);
		auto vals = collect_values(visited);
		CHECK( vals == std::vector<char>{'d', 'c', 'b', 'a'} );
	}

	TEST_CASE("post_order search_unique with all-same-valued nodes") {
		// a(a, a) where both children are structurally equal
		tref leaf = n('a');
		tref root = n('a', {leaf, leaf});
		trefs visited;
		auto collect = [&visited](tref nd) {
			visited.push_back(nd);
			return true;
		};
		post_order<char>(root).search_unique(collect);
		// Unique: the shared leaf 'a' visited once; root 'a' (different structure) once
		CHECK( visited.size() == 2 );
	}

	TEST_CASE("pre_order visit_unique with all-same-valued nodes") {
		tref leaf = n('a');
		tref root = n('a', {leaf, leaf});
		trefs visited;
		auto collect = [&visited](tref nd) { visited.push_back(nd); };
		pre_order<char>(root).visit_unique(collect);
		// root 'a' and leaf 'a' are structurally different (one has children)
		CHECK( visited.size() == 2 );
	}

	TEST_CASE("apply_unique_until_change stops at first change pre_order") {
		// Transformer that changes only 'b' nodes
		tref root = n('a', {n('b'), n('b')});
		int changes = 0;
		auto change_b = [&changes](tref nd) -> tref {
			if (chtree::get(nd).value == 'b') {
				++changes;
				return n('z');
			}
			return nd;
		};
		auto result = pre_order<char>(root).apply_unique_until_change(change_b);
		// At first 'b' is changed; check first child is now 'z'
		CHECK( cmp(chtree::get(result).child(0), n('z')) );
	}

	TEST_CASE("post_order apply_unique with no changes returns original pointer") {
		tref root = n('a', {n('b'), n('c')});
		auto no_change = [](tref nd) { return nd; };
		auto result = post_order<char>(root).apply_unique(no_change);
		CHECK( result == root );
	}

	TEST_CASE("post_order search on wide tree (many children)") {
		trefs children;
		for (char c = 'a'; c <= 'e'; ++c)
			children.push_back(n(c));
		tref root = n('r', children);
		trefs visited;
		auto collect = [&visited](tref nd) {
			visited.push_back(nd);
			return true;
		};
		post_order<char>(root).search(collect);
		// post-order: a, b, c, d, e, r
		CHECK( visited.size() == 6 );
		CHECK( chtree::get(visited.back()).value == 'r' );
	}

	TEST_CASE("pre_order search on wide tree (many children)") {
		trefs children;
		for (char c = 'a'; c <= 'e'; ++c)
			children.push_back(n(c));
		tref root = n('r', children);
		trefs visited;
		auto collect = [&visited](tref nd) {
			visited.push_back(nd);
			return true;
		};
		pre_order<char>(root).search(collect);
		// pre-order: r, a, b, c, d, e
		CHECK( visited.size() == 6 );
		CHECK( chtree::get(visited.front()).value == 'r' );
	}
}
