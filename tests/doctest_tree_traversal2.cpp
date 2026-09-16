// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <functional>
#include <tuple>

#include "init_test.h"

auto id = [] (tref n) { return n; };


/*
auto is_e = [] (tref n) {
        return chtree::get(n).value == 'e';
};

auto is_not_e = [] (tref n) {
        return chtree::get(n).value != 'e';
};


auto is_b = [] (tref n) {
        return chtree::get(n).value == 'b';
};
*/

auto is_not_b = [] (tref n) {
        return chtree::get(n).value != 'b';
};

auto tr_e_E = [] (tref n) {
        auto x = chtree::get(n);
        if (x.value == 'e') return bintree<char>::get('E', x.l, x.r);
        return n;
};


TEST_SUITE("post_order") {

        TEST_CASE_FIXTURE(test_tree_fixture, "apply_unique") {
                auto result = post_order<char>(in).apply_unique(id);
                CHECK(result == in);

                auto result2 = post_order<char>(in).apply_unique(tr_e_E);
                auto x = chtree::get(result2);
                CHECK(x.value == 'E');
                CHECK(x.first_tree().value == 'b');
                CHECK(x.second_tree().value == 'E');
                CHECK(x.first_tree().second_tree().value == 'E');

                auto result3 = post_order<char>(in)
                                        .apply_unique(tr_e_E, is_not_b);
                auto y = chtree::get(result3);
                CHECK(y.value == 'E');
                CHECK(y.first_tree().value == 'b');
                CHECK(y.second_tree().value == 'E');
                CHECK(y.first_tree().second_tree().value == 'e');

                // bintree<char>::gc();
       }

        TEST_CASE_FIXTURE(test_tree_fixture, "search(visit, visit_subtree)") {
                trefs found;
                auto search_f = [&found] (tref n) {
                        if (chtree::get(n).value == 'f') found.push_back(n);
                        return true;
                };
                post_order<char>(in).search(search_f, all);
                CHECK( found.size() == 2);
                CHECK( found[0] == chtree::get(in)[1][0].get() );
                CHECK( found[1] == chtree::get(in)[1][1][0][0].get() );

                found.clear();
                post_order<char>(in).search(search_f, is_not_b);
                CHECK( found.size() == 1);
                CHECK( found[0] == chtree::get(in)[1][0].get() );

                // bintree<char>::gc();
        }

        TEST_CASE_FIXTURE(test_tree_fixture, "search_unique(visit, visit_subtree)") {
                trefs found;
                auto search_f = [&found] (tref n) {
                        if (chtree::get(n).value == 'f') found.push_back(n);
                        return true;
                };
                post_order<char>(in).search_unique(search_f);
                CHECK( found.size() == 1);
                CHECK( found[0] == chtree::get(in)[1][0].get() );

                found.clear();
                auto search_e = [&found] (tref n) {
                        if (chtree::get(n).value == 'e') found.push_back(n);
                        return true;
                };
                post_order<char>(in).search_unique(search_e, is_not_b);
                CHECK( found.size() == 2);
                CHECK( found[0] == chtree::get(in)[1].get() );
                CHECK( found[1] == in );

                // bintree<char>::gc();
        }

}

TEST_SUITE("pre_order") {

        TEST_CASE_FIXTURE(test_tree_fixture, "apply(f, visit_subtree, up)") {
                auto result = pre_order<char>(in).apply_unique(id);
                CHECK(result == in);

                auto result2 = pre_order<char>(in).apply_unique(tr_e_E);
                auto x = chtree::get(result2);
                CHECK(x.value == 'E');
                CHECK(x.first_tree().value == 'b');
                CHECK(x.second_tree().value == 'E');
                CHECK(x.first_tree().second_tree().value == 'E');

                auto result3 = pre_order<char>(in)
                                        .apply_unique(tr_e_E, is_not_b, id);
                auto y = chtree::get(result3);
                CHECK(y.value == 'E');
                CHECK(y.first_tree().value == 'b');
                CHECK(y.second_tree().value == 'E');
                CHECK(y.first_tree().second_tree().value == 'e');

                // bintree<char>::gc();
        }

        TEST_CASE_FIXTURE(test_tree_fixture, "apply_until_change(f, visit_subtree, up)") {

        }

        TEST_CASE_FIXTURE(test_tree_fixture, "apply_unique(f, visit_subtree)") {

        }

        TEST_CASE_FIXTURE(test_tree_fixture, "apply_unique_until_change(f, visit_subtree, up)") {

        }

        TEST_CASE_FIXTURE(test_tree_fixture, "search(visit, visit_subtree, up, between)") {
                trefs found;
                auto search_f = [&found] (tref n) {
                        if (chtree::get(n).value == 'f') found.push_back(n);
                        return true;
                };
                pre_order<char>(in).search(search_f);
                CHECK( found.size() == 2);
                CHECK( found[0] == chtree::get(in)[1][0].get() );
                CHECK( found[1] == chtree::get(in)[1][1][0][0].get() );

                found.clear();
                pre_order<char>(in).search(search_f, is_not_b, id);
                CHECK( found.size() == 1);
                CHECK( found[0] == chtree::get(in)[1][0].get() );

                // bintree<char>::gc();
        }

        TEST_CASE_FIXTURE(test_tree_fixture, "search_unique(visit, visit_subtree, up)") {
                trefs found;
                auto search_f = [&found] (tref n) {
                        if (chtree::get(n).value == 'f') found.push_back(n);
                        return true;
                };
                pre_order<char>(in).search_unique(search_f);
                CHECK( found.size() == 1);
                CHECK( found[0] == chtree::get(in)[1][0].get() );

                found.clear();
                auto search_e = [&found] (tref n) {
                        if (chtree::get(n).value == 'e') found.push_back(n);
                        return true;
                };
                pre_order<char>(in).search_unique(search_e, is_not_b, id);
                CHECK( found.size() == 2);
                CHECK( found[0] == in );
                CHECK( found[1] == chtree::get(in)[1].get() );

                // bintree<char>::gc();
        }

        TEST_CASE_FIXTURE(test_tree_fixture, "visit(visit, visit_subtree, up, between)") {

        }

        TEST_CASE_FIXTURE(test_tree_fixture, "visit_unique(visit, visit_subtree)") {

        }

}

// ---------------------------------------------------------------------------
// Morris post-order traversal tests (migrated from test_tree.cpp)
// ---------------------------------------------------------------------------

static std::vector<int> post_order_recursive(tref root) {
	std::vector<int> res;
	std::function<void(tref)> rec = [&](tref r) {
		if (!r) return;
		const auto& node = bintree<int>::get(r);
		rec(node.l);
		rec(node.r);
		res.push_back(node.value);
	};
	rec(root);
	return res;
}

static std::vector<int> morris_po_traverse(tref root) {
	std::vector<int> res;
	if (!root) return res;
	morris_post_order<int> traverser(root);
	auto visitor = [&res](tref node) -> bool {
		if (node) res.push_back(bintree<int>::get(node).value);
		return true;
	};
	traverser.search(visitor);
	return res;
}

// Snapshot (value, l, r) of every reachable node via recursive pre-order.
// Used to verify Morris traversal restores the tree's link structure.
static std::vector<std::tuple<int, tref, tref>> snapshot_tree(tref root) {
	std::vector<std::tuple<int, tref, tref>> snap;
	std::function<void(tref)> rec = [&](tref r) {
		if (!r) return;
		const auto& node = bintree<int>::get(r);
		snap.emplace_back(node.value, node.l, node.r);
		rec(node.l);
		rec(node.r);
	};
	rec(root);
	return snap;
}

TEST_SUITE("morris_post_order") {

	TEST_CASE("empty") {
		CHECK(post_order_recursive(nullptr)
			== morris_po_traverse(nullptr));
	}

	TEST_CASE("single node") {
		tref t = bintree<int>::get(1, nullptr, nullptr);
		CHECK(post_order_recursive(t) == morris_po_traverse(t));
	}

	TEST_CASE("simple: 1->{2,3}") {
		tref t = bintree<int>::get(1,
			bintree<int>::get(2, nullptr, nullptr),
			bintree<int>::get(3, nullptr, nullptr));
		CHECK(post_order_recursive(t) == morris_po_traverse(t));
	}

	TEST_CASE("left-skewed: 1->2->3->4") {
		tref l4 = bintree<int>::get(4, nullptr, nullptr);
		tref n3 = bintree<int>::get(3, l4, nullptr);
		tref n2 = bintree<int>::get(2, n3, nullptr);
		tref t   = bintree<int>::get(1, n2, nullptr);
		CHECK(post_order_recursive(t) == morris_po_traverse(t));
	}

	TEST_CASE("right-skewed: 1->2->3->4") {
		tref r4 = bintree<int>::get(4, nullptr, nullptr);
		tref r3 = bintree<int>::get(3, nullptr, r4);
		tref r2 = bintree<int>::get(2, nullptr, r3);
		tref t   = bintree<int>::get(1, nullptr, r2);
		CHECK(post_order_recursive(t) == morris_po_traverse(t));
	}

	TEST_CASE("complex tree") {
		/*
		 *         1
		 *       /   \
		 *      2     3
		 *     / \   / \
		 *    4   5 6   7
		 *   /   / \     \
		 *  8   9  10     11
		 *     / \       /
		 *    12 13     14
		 *        \    / \
		 *        15  16 17
		 */
		tref n8  = bintree<int>::get(8, nullptr, nullptr);
		tref n12 = bintree<int>::get(12, nullptr, nullptr);
		tref n13 = bintree<int>::get(13, nullptr, nullptr);
		tref n16 = bintree<int>::get(16, nullptr, nullptr);
		tref n17 = bintree<int>::get(17, nullptr, nullptr);
		tref n9  = bintree<int>::get(9, n12, n13);
		tref n10 = bintree<int>::get(10, nullptr, nullptr);
		tref n14 = bintree<int>::get(14, n16, n17);
		tref n4  = bintree<int>::get(4, n8, nullptr);
		tref n5  = bintree<int>::get(5, n9, n10);
		tref n6  = bintree<int>::get(6, nullptr, nullptr);
		tref n11 = bintree<int>::get(11, n14, nullptr);
		tref n7  = bintree<int>::get(7, nullptr, n11);
		tref n2  = bintree<int>::get(2, n4, n5);
		tref n3  = bintree<int>::get(3, n6, n7);
		tref t   = bintree<int>::get(1, n2, n3);
		// Morris threads the tree during traversal; verify the link
		// structure is restored to its original state on exit.
		auto before = snapshot_tree(t);
		auto result = morris_po_traverse(t);
		auto after  = snapshot_tree(t);
		CHECK(result == post_order_recursive(t));
		CHECK(before == after);
	}

	TEST_CASE("deep left-skewed (1024 nodes)") {
		// Stresses the O(1)-space property: a depth that would be
		// uncomfortable for recursion is the normal operating range
		// for Morris. Recursive comparator stays within stack at this
		// depth on platforms with default thread stacks.
		constexpr int N = 1024;
		tref t = bintree<int>::get(N, nullptr, nullptr);
		for (int i = N - 1; i >= 1; --i)
			t = bintree<int>::get(i, t, nullptr);
		auto before = snapshot_tree(t);
		auto result = morris_po_traverse(t);
		auto after  = snapshot_tree(t);
		CHECK(result == post_order_recursive(t));
		CHECK(before == after);
	}
}
