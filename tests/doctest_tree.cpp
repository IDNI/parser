// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentiTd cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "init_test.h"

struct is_capture_predicate {

	bool operator()(const tref& n) const {
		auto v = chtree::get(n).value;
		return v == 'X' || v == 'Y' || v == 'Z';
	}
};

TEST_SUITE("structural equality") {

	TEST_CASE_FIXTURE(test_tree_fixture, "map") {
		std::map<tref, tref> m;
		m[in] = n('h'); // e
		m[chtree::get(in).first()] = n('j'); // e b
		m[chtree::get(in).second()] = n('c'); // e e
		m[chtree::get(in)[0][0][1].get()] = n('e'); // e b d h
		auto it = m.find(in);
		CHECK(it != m.end());
		CHECK(it->second == n('h'));
		it = m.find(chtree::get(in).first());
		CHECK(it != m.end());
		CHECK(it->second == n('j'));
		it = m.find(chtree::get(in).second());
		CHECK(it != m.end());
		CHECK(it->second == n('c'));
		it = m.find(n('h'));
		CHECK(it != m.end());
		CHECK(it->second == n('e'));
	}

	TEST_CASE_FIXTURE(test_tree_fixture, "set") {
		std::set<tref> s;
		s.insert(in);
		s.insert(chtree::get(in).first());
		s.insert(chtree::get(in).second());
		s.insert(chtree::get(in)[0][0][1].get());
		s.insert(chtree::get(in)[1][1][1].get()); // e e b e
		s.insert(n('h'));
		s.insert(n('e'));
		CHECK(s.size() == 6);
		CHECK(s.contains(n('h')));
		CHECK(s.contains(n('e')));
		CHECK(s.contains(in));
		CHECK(s.contains(chtree::get(in).first()));
		CHECK(s.contains(chtree::get(in).second()));
		CHECK(s.contains(chtree::get(in)[0][0][1].get()));
		CHECK(s.contains(chtree::get(in)[1][1][1].get()));
	}

	TEST_CASE_FIXTURE(test_tree_fixture, "unordered_map") {
	}

	TEST_CASE_FIXTURE(test_tree_fixture, "unordered_set") {
	}

}

TEST_SUITE("subtree equality") {

	TEST_CASE_FIXTURE(test_tree_fixture, "subtree_map") {
		subtree_map<char, tref> m;
		const auto& root = chtree::get(in);
		m[in] = n('h'); // e
		m[root.first()] = n('j'); // e b
		m[root.second()] = n('c'); // e e
		m[root[0][0][1].get()] = n('e'); // e b d h
		m[root[0][2][0].get()] = n('p'); // e b c g
		auto it = m.find(in);
		CHECK(it != m.end());
		CHECK(it->second == n('h'));
		it = m.find(root.first());
		CHECK(it != m.end());
		CHECK(it->second == n('j'));
		it = m.find(root.second());
		CHECK(it != m.end());
		CHECK(it->second == n('c'));
		it = m.find(n('h'));
		CHECK(it != m.end());
		CHECK(it->second == n('e'));
		it = m.find(root[0][2][0].get());
		CHECK(it != m.end());
		CHECK(it->second == n('p'));
		CHECK(m.size() == 5);
		m[root[1][1][1].get()] = n('e'); // e e b g
		CHECK(m.size() == 5);
		it = m.find(root[1][1][1].get());
		CHECK(it != m.end());
		CHECK(it->second == n('e'));
		it = m.find(root[0][2][0].get());
		CHECK(it != m.end());
		CHECK(it->second == n('e'));
	}

	TEST_CASE_FIXTURE(test_tree_fixture, "subtree_set") {
		std::cout << chtree::get(in).print_to_str() << std::endl;
		subtree_set<char> s;
		s.insert(in);
		s.insert(chtree::get(in).first());
		s.insert(chtree::get(in).second());
		s.insert(chtree::get(in)[0][0][1].get());
		s.insert(chtree::get(in)[0][2][0].get());
		s.insert(chtree::get(in)[1][1][1].get());
		CHECK(s.size() == 5);
		CHECK(s.contains(in));
		CHECK(s.contains(chtree::get(in).first()));
		CHECK(s.contains(chtree::get(in).second()));
		CHECK(s.contains(chtree::get(in)[0][0][1].get()));
		CHECK(s.contains(chtree::get(in)[0][2][0].get()));
		CHECK(s.contains(chtree::get(in)[1][1][1].get()));
		CHECK(!s.contains(n('f')));
		CHECK(!s.contains(n('g')));
	}

}

TEST_SUITE("basic test") {

	TEST_CASE("char as node") {
		using nd_t = char;
		using tree = lcrs_tree<nd_t>;
		nd_t n1, n2, n3, n4;
		n1 = '1';
		n2 = '2', n3 ='3', n4= '4';
		{
			nd_t child [] = {n2, n3};
			auto tid = tree::get(n1, child, 2);
			const auto& t = tree::get(tid);
			// t.print(std::cout);
			CHECK( t.value == n1 );
			CHECK( t.children_size() == 2 );
			CHECK( t.first_tree().value == n2 );
			CHECK( t.child_tree(0).value == n2 );
			CHECK( t.child_tree(1).value == n3 );
			CHECK( t.l == t.left_child() );
			CHECK( t.l == t.first() );
			CHECK( t.l == t.child(0) );
			CHECK( tree::get(t.l).r == t.child(1) );
			CHECK( tree::get(tree::get(t.l).r).r == t.child(2) );
			CHECK( t.child_tree(0) == t[0] );
			CHECK( t.child_tree(1) == t[1] );
			CHECK( t.first_tree() == tree::get(t.l) );
			CHECK( t.first_tree() == t.child_tree(0) );
			CHECK( t.first_tree() == tree::get(t.child(0)) );
			CHECK( t.second_tree() == t.child_tree(1) );
			CHECK( t.second_tree() == tree::get(t.child(1)) );
			CHECK( t.second_tree() == tree::get(t.second()) );
		}
		// above handle goes out of scope
		{
			nd_t arr [] = { n1, n2, n3, n4 };
			auto id = tree::get(n2, arr, 4);
			//get shared handle for id to avoid being gced.
			auto sh = tree::geth(id);
			//print though id
			const auto& t = tree::get(id);
			// t.print(std::cout);
			CHECK( t.value == n2 );
			CHECK( t.children_size() == 4 );
			CHECK( t.child_tree(0).value == n1 );
			CHECK( t.child_tree(1).value == n2 );
			CHECK( t.child_tree(2).value == n3 );
			CHECK( t.child_tree(3).value == n4 );
			CHECK( t.l == t.left_child() );
			CHECK( t.l == t.first() );
			CHECK( t.l == t.child(0) );
			CHECK( t.first_tree() == tree::get(t.l) );
			CHECK( t.first_tree() == t.child_tree(0) );
			CHECK( t.first_tree() == tree::get(t.child(0)) );
			CHECK( t.second_tree() == tree::get(t.child(1)) );
			CHECK( t.second_tree() == t.child_tree(1) );
			CHECK( t.second_tree() == tree::get(t.second()) );
			CHECK( t.third_tree() == tree::get(t.child(2)) );
			CHECK( t.third_tree() == t.child_tree(2) );
			CHECK( t.third_tree() == tree::get(t.third()) );
			CHECK( tree::get(t.child(3)) == t.child_tree(3) );
			CHECK( t[3] == tree::get(t.child(3)) );
			// prev tree handle that was out of scope is garbage out
			bintree<nd_t>::gc();
			
			// print through sh handle (different id) but same tree
			const auto& t2 = tree::get(sh);
			// t2.print(std::cout);
			CHECK( t2.get() == id );
			CHECK( t2.value == n2 );
			CHECK( t2.children_size() == 4 );
			CHECK( t2.child_tree(0).value == n1 );
			CHECK( t2.child_tree(1).value == n2 );
			CHECK( t2.child_tree(2).value == n3 );
			CHECK( t2.child_tree(3).value == n4 );
			// create a new tree
			auto h3 = bintree<nd_t>::get('C', sh->get(), sh->get());
			const auto& t3 = tree::get(h3);
			// t3.print(std::cout);
			CHECK( t3.value == 'C' );
			CHECK( t3.has_right_sibling() );
			CHECK( t3.has_child() );
			CHECK( t3.children_size() == 1 );
			CHECK( t3.first_tree().value == n2 );
			CHECK( t3.right_sibling_tree().value == n2 );
			CHECK( t3.only_child() == t3.first() );
			CHECK( t3.only_child() == t3.left_child() );
			CHECK( t3.only_child() == t3.child(0) );
			CHECK( t3.only_child() == t3.l );
			CHECK( tree::get(t3.only_child()) == t3.only_child_tree() );
			CHECK( tree::get(t3.only_child()) == tree::get(t3.l) );
		}
		bintree<nd_t>::gc();
	}

	TEST_CASE("string as node") {
		using nd_t = std::string;
		using tree = lcrs_tree<nd_t>;
		nd_t n1,n2, n3, n4;
		n1 = "1";
		n2 = "2", n3 ="3", n4= "4";
		{
			nd_t child [] = {n2, n3};
			auto tid = tree::get(n1, child,
						sizeof(child) / sizeof(nd_t));
			auto t = tree::get(tid);
			// t.print(std::cout);
			CHECK( t.value == n1 );
			CHECK( t.children_size() == 2 );
			CHECK( t.child_tree(0).value == n2 );
			CHECK( t.child_tree(1).value == n3 );
			CHECK( t.l == t.left_child() );
			CHECK( t.l == t.first() );
			CHECK( t.l == t.child(0) );
			CHECK( tree::get(t.l) == t.first_tree() );
			CHECK( tree::get(t.l) == t.child_tree(0) );
			CHECK( tree::get(t.l) == tree::get(t.child(0)) );
			CHECK( t.second_tree() == tree::get(t.child(1)) );
			CHECK( t.second_tree() == t.child_tree(1) );
			CHECK( t.second_tree() == tree::get(t.second()) );
		}
		// above handle goes out of scope
		{
			nd_t arr [] = { n1, n2, n3, n4 };
			auto id = tree::get(n2, arr, sizeof(arr) / sizeof(nd_t));
			//get shared handle for id to avoid being gced.
			auto sh = bintree<nd_t>::geth(id);
			//print though id
			const auto& t = tree::get(id);
			// t.print(std::cout);
			CHECK( t.value == n2 );
			CHECK( t.children_size() == 4 );
			CHECK( t.child_tree(0).value == n1 );
			CHECK( t.child_tree(1).value == n2 );
			CHECK( t.child_tree(2).value == n3 );
			CHECK( t.child_tree(3).value == n4 );
			CHECK( t.l == t.left_child() );
			CHECK( t.l == t.first() );
			CHECK( t.l == t.child(0) );
			CHECK( tree::get(t.l) == t.first_tree() );
			CHECK( tree::get(t.l) == t.child_tree(0) );
			CHECK( tree::get(t.l) == tree::get(t.child(0)) );
			CHECK( t.second_tree() == tree::get(t.child(1)) );
			CHECK( t.second_tree() == t.child_tree(1) );
			CHECK( t.second_tree() == tree::get(t.second()) );
			CHECK( t.third_tree() == tree::get(t.child(2)) );
			CHECK( t.third_tree() == t.child_tree(2) );
			CHECK( t.third_tree() == tree::get(t.third()) );
			CHECK( tree::get(t.child(3)) == t.child_tree(3) );
			CHECK( t[3] == tree::get(t.child(3)) );
			// prev tree handle that was out of scope is garbage out
			bintree<nd_t>::gc();
			
			// print through sh handle (different id) but same tree
			const auto& t2 = tree::get(sh);
			// t2.print(std::cout);
			CHECK( t2.value == n2 );
			CHECK( t2.children_size() == 4 );
			CHECK( t2.child_tree(0).value == n1 );
			CHECK( t2.child_tree(1).value == n2 );
			CHECK( t2.child_tree(2).value == n3 );
			CHECK( t2.child_tree(3).value == n4 );

			// create a new tree through bintree
			auto h3 = bintree<nd_t>::get("C", sh->get(), sh->get());
			const auto& t3 = tree::get(h3);
			// t3.print(std::cout);
			CHECK( t3.value == "C" );
			CHECK( t3.has_right_sibling() );
			CHECK( t3.has_child() );
			CHECK( t3.children_size() == 1 );
			CHECK( t3.first_tree().value == n2 );
			CHECK( t3.right_sibling_tree().value == n2 );
			// dont save its handle
		}
		bintree<nd_t>::gc();
	}
}

TEST_SUITE("node") {

	TEST_CASE("node constructor: given a value, the node has that value") {
		auto d1 = d('a');
		CHECK( d1.value == 'a' );
		CHECK( d1.children_size() == 0 );
	}

	TEST_CASE("node constructor: given a value and children, the node has that value "
			"and children") {
		auto d1 = d('a', {n('b'), n('c'), n('d')});
		CHECK( d1.value == 'a' );
		CHECK( d1.children_size() == 3 );
		CHECK( d1.child_tree(0).value == 'b' );
		CHECK( d1.child_tree(1).value == 'c' );
		CHECK( d1.child_tree(2).value == 'd' );
	}
	TEST_CASE("node order: given two simple nodes, the order is given by the order "
			"of the syms") {
		CHECK( d('a') < d('b') );
	}

	TEST_CASE("node order: given two nodes with same value and different children, "
			"one is bigger than the other") {
		auto d1 = d('a', {n('b')});
		auto d2 = d('a', {n('b'), n('c')});
		CHECK( ((d1 < d2) || (d2 < d1)) );
	}

	TEST_CASE("node order: given two nodes with the same value and children, neither "
			"of them is bigger than the other") {
		auto d1 = d('a', {n('b'), n('c'), n('d')});
		auto d2 = d('a', {n('b'), n('c'), n('d')});
		CHECK( (!(d1 < d2) && !(d2 < d1)) );
	}

	TEST_CASE("node equality: given two simple nodes with the same value, they "
			"are equal") {
		CHECK( d('a') == d('a') );
	}

	TEST_CASE("node equality: given two nodes with the same value and children, "
			"they are equal") {
		CHECK( d('a', {n('b'), n('c'), n('d')})
				== d('a', {n('b'), n('c'), n('d')}) );
	}

	TEST_CASE("node equality: given two simple nodes with different syms, they "
			"are not equal") {
		CHECK( d('a') != d('b') );
	}

	TEST_CASE("node equality: given two nodes with different syms and same children, "
			"they are not equal") {
		CHECK( d('a', {n('b'), n('c'), n('d')}) != d('b', {n('b'), n('c'), n('d')}) );
	}

	TEST_CASE("node equality: given two nodes with same syms and different children, "
			"they are equal") {
		CHECK( d('a', {n('b'), n('c'), n('d')})
				== d('a', {n('b'), n('c'), n('d')}) );
	}
}

TEST_SUITE("make_node") {

	TEST_CASE("make_node uniqueness: given two simple nodes with the same value, it "
			"returns the same node") {
		auto n1 = n('a');
		auto n2 = n('a');
		CHECK( n1 == n2 );
	}

	TEST_CASE("make_node uniqueness: given two nodes with the same value and children, "
			"it returns the same node") {
		auto n1 = n('a', {n('b'), n('c'), n('d')});
		auto n2 = n('a', {n('b'), n('c'), n('d')});
		CHECK( n1 == n2 );
	}

	TEST_CASE("make_node uniqueness: given two nodes with different value and same "
			"children, it returns the different nodes") {
		auto n1 = n('a', {n('b'), n('c'), n('d')});
		auto n2 = n('b', {n('b'), n('c'), n('d')});
		CHECK( n1 != n2 );
	}

	TEST_CASE("make_node uniqueness: given two nodes with same value and different "
			"children, it returns the different nodes") {
		auto n1 = n('a', {n('b'), n('c'), n('d')});
		auto n2 = n('a', {n('b'), n('c'), n('e')});
		CHECK( n1 != n2 );
	}
}


TEST_SUITE("post_order_traverser") {

	struct collect_predicate {
		bool operator()(tref n) { return nodes.push_back(n), true; }
		trefs nodes;
	};

	template<typename wrapped_t>
	struct collect_visitor {
		collect_visitor(wrapped_t& wrapped) : wrapped(wrapped) { }
		tref operator()(tref n) {
			return nodes.push_back(wrapped(n)), nodes.back();
		}
		trefs nodes;
		wrapped_t& wrapped;
	};

	TEST_CASE("post_order_traverser: given a simple tree, it visits the root node") {
		tref root = n('a');
		collect_visitor<decltype(idni::identity)> visited{idni::identity};
		trefs expected {root};
		post_order_traverser<char, decltype(visited),
			decltype(all)>(visited, all)(root);
		CHECK( cmp(visited.nodes, expected) );
	}

	TEST_CASE("post_order_traverser: given a tree with two children, it visits "
			"the children and then the root") {
		tref root = n('a', {n('b'), n('c')});
		collect_visitor<decltype(idni::identity)> visited{idni::identity};
		trefs expected {n('b'), n('c'), root};
		post_order_traverser<char, decltype(visited),
			decltype(all)>(visited, all)(root);
		CHECK( cmp(visited.nodes, expected) );
	}

	TEST_CASE("post_order_traversal: given tree with underlying diamond like DAG, "
			"it visits the children and then the root") {
		tref root = n('a', {n('b', {n('d')}), n('c', {n('d')})});
		collect_visitor<decltype(idni::identity)> visited{idni::identity};
		trefs expected {n('d'), n('b', {n('d')}), n('c', {n('d')}), root};
		post_order_traverser<char, decltype(visited),
			decltype(all)>(visited, all)(root);
		CHECK( cmp(visited.nodes, expected) );
	}
}

TEST_SUITE("map_transformer") {

	TEST_CASE("map_transformer: given a simple tree and a visitor, it returns a "
			"new tree with the changes applied by the visitor") {
		tref root = n('a');
		auto transform = [](char c) { return c == 'a' ? 'z' : c; };
		map_transformer<char, decltype(transform)> map{transform};
		tref expected { n('z') };
		tref result = post_order_traverser<char, decltype(map),
			decltype(all)>(map, all)(root);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("map_transformer: given a tree with two children and a visitor, "
			"it returns a new tree with the changes applied by the visitor") {
		tref root = n('a', {n('b'), n('c')});
		auto transform = [](char c) { return c == 'b' ? 'z' : c; };
		map_transformer<char, decltype(transform)> map{transform};
		tref expected { n('a', {n('z'), n('c')}) };
		tref result = post_order_traverser<char, decltype(map),
			decltype(all)>(map, all)(root);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("map_transformer: given a tree with underlying diamond like DAG "
			"and a visitor, it returns a new tree with the changes applied by "
			"the visitor") {
		tref root = n('a', {n('b', {n('d')}), n('c', {n('d')})});
		auto transform = [](char c) { return c == 'd' ? 'z' : c; };
		map_transformer<char, decltype(transform)> map{transform};
		tref expected { n('a', {n('b', {n('z')}), n('c', {n('z')})}) };
		tref result = post_order_traverser<char, decltype(map),
			decltype(all)>(map, all)(root);
		CHECK( cmp(result, expected) );
	}
}

TEST_SUITE("replace_transformer") {

	TEST_CASE("replace_transformer: given a simple tree and a visitor, it returns "
			"a new tree with the provided replacements") {
		tref root = n('a');
		subtree_map<char, tref> m;
		m[root] = n('z');
		replace_transformer<char> replace{m};
		tref expected { n('z') };
		auto result = post_order_traverser<char, decltype(replace),
			decltype(all)>(replace, all)(root);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("replace_transform: given a tree with two children and a visitor, "
			"it returns a new tree with the provided replacements") {
		tref root = n('a', {n('b'), n('c')});
		subtree_map<char, tref> m;
		m[n('b')] = n('z');
		replace_transformer<char> replace{m};
		tref expected { n('a', {n('z'), n('c')}) };
		tref result = post_order_traverser<char,
			decltype(replace), decltype(all)>(replace, all)(root);
		// chtree::get(root).print(std::cout << "input: ");
		// chtree::get(expected).print(std::cout << "expected: ");
		// chtree::get(result).print(std::cout << "result: ");
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("replace_transform: given a tree with underlying diamond like DAG "
			"and a visitor, it returns a new tree with the provided replacements") {
		tref root = n('a', {n('b', {n('d')}), n('c', {n('d')})});
		subtree_map<char, tref> m;
		m[n('d')] = n('z');
		replace_transformer<char> replace{m};
		tref expected { n('a', {n('b', {n('z')}), n('c', {n('z')})}) };
		tref result = post_order_traverser<char, decltype(replace), decltype(all)>(replace, all)(root);
		CHECK( cmp(result, expected) );
	}
	// TODO (LOW) write tests corresponding to related functions
}

TEST_SUITE("select_top_predicate") {

	TEST_CASE("select_top_predicate: given a simple tree whose root satisfies the "
			"predicate, it returns the root") {
		tref root = n('a');
		auto predicate = [](tref n) { return chtree::get(n).value == 'a'; };
		trefs selected;
		trefs expected{ root };
		select_top_predicate<decltype(predicate)>
					select{ predicate, selected };
		post_order_traverser<char, decltype(idni::identity),
			decltype(select)>(idni::identity, select)(root);
		CHECK( cmp(selected, expected) );
	}

	TEST_CASE("select_top_predicate: given a simple tree whose root does not "
			"satisfy the predicate, it returns an empty vector") {
		tref root = n('a');
		auto predicate = [](tref n) { return chtree::get(n).value == 'b'; };
		trefs selected;
		trefs expected {};
		select_top_predicate<decltype(predicate)>
					select{ predicate, selected };
		post_order_traverser<char, decltype(idni::identity),
			decltype(select)>(idni::identity, select)(root);
		CHECK( cmp(selected, expected) );
	}


	TEST_CASE("selected_top_predicate: given a tree with two children that satisfy "
			"the predicate, it returns the children satisfying the predicate") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) {
			char v = chtree::get(n).value;
			return v == 'b' || v == 'c';
		};
		trefs selected;
		trefs expected {n('b'), n('c')};
		select_top_predicate<decltype(predicate)>
					select{ predicate, selected };
		post_order_traverser<char, decltype(idni::identity),
			decltype(select)>(idni::identity, select)(root);
		CHECK( cmp(selected, expected) );
	}

	TEST_CASE("select_top_predicate: given a tree with two children that do not "
			"satisfy the predicate, it returns an empty vector satisfying the "
			"predicate") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) {
			char v = chtree::get(n).value;
			return v == 'd' || v == 'e';
		};
		trefs selected;
		trefs expected {};
		select_top_predicate<decltype(predicate)>
					select{ predicate, selected };
		post_order_traverser<char, decltype(idni::identity),
			decltype(select)>(idni::identity, select)(root);
		CHECK( cmp(selected, expected) );
	}

	TEST_CASE("select_top_predicate: given a tree with underlying diamond like "
			"DAG and a visitor, it returns a vector with only one bottom node "
			"satisfying the predicate") {
		tref root = n('a', {n('b', {n('d')}), n('c', {n('d')})});
		auto predicate = [](tref n) {
			return chtree::get(n).value == 'd';
		};
		trefs selected;
		trefs expected {n('d')};
		select_top_predicate<decltype(predicate)>
					select{ predicate, selected };
		post_order_traverser<char, decltype(idni::identity),
			decltype(select)>(idni::identity, select)(root);
		CHECK( cmp(selected, expected) );
	}

	TEST_CASE("select_top_predicate: given a tree with underlying diamond like "
			"DAG and a visitor, it returns a vector the two top nodes satisfying "
			"the predicate") {
		tref root = n('a', {n('b', {n('d')}), n('c', {n('d')})});
		auto predicate = [](tref n) {
			char v = chtree::get(n).value;
			return v == 'b' || v == 'c';
		};
		trefs selected;
		trefs expected {n('b', {n('d')}), n('c', {n('d')})};
		select_top_predicate<decltype(predicate)>
					select{ predicate, selected };
		post_order_traverser<char, decltype(idni::identity),
			decltype(select)>(idni::identity, select)(root);
		CHECK( cmp(selected, expected) );
	}

	// TODO (LOW) write tests corresponding to related functions
}

// TODO (MEDIUM) write tests for select_subnodes predicates
// TODO (MEDIUM) write tests for select_subnodes functions


TEST_SUITE("select_all_predicate") {

	TEST_CASE("select_all_predicate: given a simple tree whose root satisfies "
			"the predicate, it returns the root") {
		tref root = n('a');
		auto predicate = [](tref n) { return chtree::get(n).value == 'a'; };
		trefs selected;
		trefs expected {root};
		select_all_predicate<decltype(predicate)> select{predicate, selected};
		post_order_traverser<char, decltype(idni::identity),
			decltype(select)>(idni::identity, select)(root);
		CHECK( cmp(selected, expected) );
	}

	TEST_CASE("select_all_predicate: given a simple tree whose root does not "
			"satisfy the predicate, it returns an empty vector") {
		tref root = n('a');
		auto predicate = [](tref n) { return chtree::get(n).value == 'b'; };
		trefs selected;
		trefs expected {};
		select_all_predicate<decltype(predicate)> select{predicate, selected};
		post_order_traverser<char, decltype(idni::identity),
			decltype(select)>(idni::identity, select)(root);
		CHECK( cmp(selected, expected) );
	}

	TEST_CASE("select_all_predicate: given a tree with two children that satisfy "
			"the predicate, it returns the children satisfying the predicate") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) { return chtree::get(n).value == 'b'
					|| chtree::get(n).value == 'c'; };
		trefs selected;
		trefs expected {n('b'), n('c')};
		select_all_predicate<decltype(predicate)> select{predicate, selected};
		post_order_traverser<char, decltype(idni::identity),
			decltype(select)>(idni::identity, select)(root);
		CHECK( cmp(selected, expected) );
	}

	TEST_CASE("select_all_predicate: given a tree with two children that do not "
			"satisfy the predicate, it returns an empty vector satisfying the "
			"predicate") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) { return chtree::get(n).value == 'd'
					|| chtree::get(n).value == 'e'; };
		trefs selected;
		trefs expected {};
		select_all_predicate<decltype(predicate)> select{predicate, selected};
		post_order_traverser<char, decltype(idni::identity),
			decltype(select)>(idni::identity, select)(root);
		CHECK( cmp(selected, expected) );
	}

	TEST_CASE("select_all_predicate: given a tree with underlying diamond like "
			"DAG and a visitor, it returns a vector with only one bottom node "
			"satisfying the predicate") {
		tref root = n('a', {n('b', {n('d')}), n('c', {n('d')})});
		auto predicate = [](tref n) { return chtree::get(n).value == 'd'; };
		trefs selected;
		trefs expected {n('d')};
		select_all_predicate<decltype(predicate)> select{predicate, selected};
		post_order_traverser<char, decltype(idni::identity),
			decltype(select)>(idni::identity, select)(root);
		CHECK( cmp(selected, expected) );
	}

	TEST_CASE("select_all_predicate: given a tree with underlying diamond like "
			"DAG and a visitor, it returns a vector the two top nodes satisfying "
			"the predicate") {
		tref root = n('a', {n('b', {n('d')}), n('c', {n('d')})});
		auto predicate = [](tref n) { return chtree::get(n).value == 'b'
					|| chtree::get(n).value == 'c'; };
		trefs selected;
		trefs expected {n('b', {n('d')}), n('c', {n('d')})};
		select_all_predicate<decltype(predicate)> select{predicate, selected};
		post_order_traverser<char, decltype(idni::identity),
			decltype(select)>(idni::identity, select)(root);
		CHECK( cmp(selected, expected) );
	}
}

TEST_SUITE("find_top_predicate") {

	TEST_CASE("find_top_predicate: given a simple tree whose root satisfies the "
			"predicate, it returns the root") {
		tref root = n('a');
		auto predicate = [](tref n) { return chtree::get(n).value == 'a'; };
		tref found = nullptr;
		tref expected{ root };
		find_top_predicate<decltype(predicate)> find{predicate, found};
		post_order_traverser<char, decltype(idni::identity),
			decltype(find)>(idni::identity, find)(root);
		CHECK( cmp(found, expected) );
	}

	TEST_CASE("find_top_predicate: given a simple tree whose root does not "
			"satisfy the predicate, it returns an empty optional") {
		tref root = n('a');
		auto predicate = [](tref n) { return chtree::get(n).value == 'b'; };
		tref found = nullptr;
		find_top_predicate<decltype(predicate)> find{predicate, found};
		post_order_traverser<char, decltype(idni::identity),
			decltype(find)>(idni::identity, find)(root);
		CHECK( cmp(found, nullptr) );
	}

	TEST_CASE("find_top_predicate: given a tree with two children that satisfy "
			"the predicate, it returns the first child satisfying the predicate") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) { return chtree::get(n).value == 'b'
					|| chtree::get(n).value == 'c'; };
 		tref found = nullptr;
		tref expected = n('b');
		find_top_predicate<decltype(predicate)> find{predicate, found};
		post_order_traverser<char, decltype(idni::identity),
			decltype(find)>(idni::identity, find)(root);
		CHECK( cmp(found, expected) );
	}

	TEST_CASE("find_top_predicate: given a tree with two children that do not "
			"satisfy the predicate, it returns an empty optional") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) { return chtree::get(n).value == 'd'
					|| chtree::get(n).value == 'e'; };
		tref found = nullptr;
		find_top_predicate<decltype(predicate)> find{predicate, found};
		post_order_traverser<char, decltype(idni::identity),
			decltype(find)>(idni::identity, find)(root);
		CHECK( cmp(found, nullptr) );
	}

	TEST_CASE("find_top_predicate: given a tree with underlying diamond like "
			"DAG and a visitor, it returns the top node satisfying the predicate") {
		tref root = n('a', {n('b', {n('d')}), n('c', {n('d')})});
		auto predicate = [](tref n) { return chtree::get(n).value == 'c'; };
		tref found = nullptr;
		tref expected = n('c', {n('d')});
		find_top_predicate<decltype(predicate)> find{predicate, found};
		post_order_traverser<char, decltype(idni::identity),
			decltype(find)>(idni::identity, find)(root);
		CHECK( cmp(found, expected) );
	}
}

// TODO (LOW) write tests for replace

TEST_SUITE("logical predicates") {

	TEST_CASE("true_predicate: given a node, it always returns true") {
		auto t = true_predicate;
		CHECK( t(n('a')) );
	}

	TEST_CASE("false_predicate: given a node, it always returns false") {
		auto f = false_predicate;
		CHECK( !f(n('a')) );
	}

	TEST_CASE("and_predicate: given a true and a false predicate, it computes "
			"the true table") {
		auto t = true_predicate;
		auto f = false_predicate;
		CHECK( and_predicate(t, t)(n('a')) );
		CHECK( !and_predicate(t, f)(n('a')) );
		CHECK( !and_predicate(f, t)(n('a')) );
		CHECK( !and_predicate(f, f)(n('a')) );
	}

	TEST_CASE("or_predicate: given a true and a false predicate, it computes "
			"the true table") {
		auto t = true_predicate;
		auto f = false_predicate;
		CHECK( or_predicate(t, t)(n('a')) );
		CHECK( or_predicate(t, f)(n('a')) );
		CHECK( or_predicate(f, t)(n('a')) );
		CHECK( !or_predicate(f, f)(n('a')) );
	}

	TEST_CASE("neg_predicate: given a true and a false predicate, it computes "
			"the true table") {
		auto t = true_predicate;
		auto f = false_predicate;
		CHECK( !neg_predicate(t)(n('a')) );
		CHECK( neg_predicate(f)(n('a')) );
	}
}

TEST_SUITE("trim_top") {

	TEST_CASE("trim_top: given a simple tree and a predicate not satisfied by "
			"the root, it returns the tree itself whatever the predicate") {
		tref root = n('a');
		auto predicate = [](tref n) { return chtree::get(n).value == 'c'; };
		tref expected {root};
		tref result = trim_top<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("trim_top: given a simple tree and a predicate satisfied by the "
			"root, it returns the tree itself whatever the predicate") {
		tref root = n('a');
		auto predicate = [](tref n) { return chtree::get(n).value == 'a'; };
		tref expected {root};
		tref result = trim_top<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("trim_top: given a tree with two children -the right one matching "
			"the predicate-, it returns the tree without the matching child") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) { return chtree::get(n).value == 'c'; };
		tref expected {n('a', {n('b')})};
		tref result = trim_top<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("trim_top: given a tree with two children -the left one matching "
			"the predicate-, it returns the tree without the matching child") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) { return chtree::get(n).value == 'b'; };
		tref expected {n('a', {n('c')})};
		tref result = trim_top<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("trim_top: given a tree with two children -both matching the "
			"predicate-, it returns the root") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) { return chtree::get(n).value == 'b'
					|| chtree::get(n).value == 'c'; };
		tref expected { n('a') };
		tref result = trim_top<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("trim_top: given a tree with two children -none matching the "
			"predicate-, it returns the given tree") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) { return chtree::get(n).value == 'd'
					|| chtree::get(n).value == 'e'; };
		tref expected { root };
		tref result = trim_top<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("trim_top: given a tree with underlying diamond like DAG and a "
			"predicate not satisfied by the nodes, it returns the tree itself") {
		tref root = n('a', {n('b', {n('d')}), n('c', {n('d')})});
		auto predicate = [](tref n) { return chtree::get(n).value == 'e'; };
		tref expected {root};
		tref result = trim_top<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("trim_top: given a tree with underlying diamond like DAG and a "
			"predicate satisfied by the bottom, it returns the tree without it") {
		tref root = n('a', {n('b', {n('d')}), n('c', {n('d')})});
		auto predicate = [](tref n) { return chtree::get(n).value == 'd'; };
		tref expected {n('a', {n('b'), n('c')})};
		tref result = trim_top<char, decltype(predicate)>(root, predicate);
		// if (result == nullptr) std::cout << "result: nullptr\n";
		// else chtree::get(result).print(std::cout << "result:");
		CHECK( cmp(result, expected) );
	}
}


// TEST_SUITE("select_subnodes") { }	


TEST_SUITE("select_top") {

	TEST_CASE("select_top: given a simple tree and a predicate not satisfied "
			"by the root, it returns the an empty collection") {
		tref root = n('a');
		auto predicate = [](tref n) { return chtree::get(n).value == 'c'; };
		CHECK( select_top<char, decltype(predicate)>(root, predicate).empty() );
	}

	TEST_CASE("select_top: given a simple tree and a predicate satisfied by "
			"the root, it returns the root") {
		tref root = n('a');
		auto predicate = [](tref n) { return chtree::get(n).value == 'a'; };
		trefs expected {root};
		trefs result = select_top<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("select_top: given a tree with two children -the right one "
			"matching the predicate-, it returns the matching child") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) { return chtree::get(n).value == 'c'; };
		trefs expected {n('c')};
		trefs result = select_top<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("select_top: given a tree with two children -the left one "
			"matching the predicate-, it returns the matching child") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) { return chtree::get(n).value == 'b'; };
		trefs expected {n('b')};
		trefs result = select_top<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("select_top: given a tree with two children -both matching "
			"the predicate-, it returns the left one") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) { return chtree::get(n).value == 'b'
					|| chtree::get(n).value == 'c'; };
		trefs expected { n('b'), n('c') };
		trefs result = select_top<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("select_top: given a tree with underlying diamond like DAG and "
			"a predicate satisfied by the bottom, it returns the bottom") {
		tref root = n('a', {n('b', {n('d')}), n('c', {n('d')})});
		auto predicate = [](tref n) { return chtree::get(n).value == 'd'; };
		trefs expected {n('d')};
		trefs result = select_top<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}
}

TEST_SUITE("select_all") {

	TEST_CASE("select_all: given a simple tree and a predicate not satisfied "
			"by the root, it returns an empty collection") {
		tref root = n('a');
		auto predicate = [](tref n) { return chtree::get(n).value == 'c'; };
		trefs expected {};
		trefs result = select_all<char, decltype(predicate)>(root, predicate);
		CHECK( cmp<char>(result, expected) );
	}

	TEST_CASE("select_all: given a simple tree and a predicate satisfied by "
			"the root, it returns the root") {
		tref root = n('a');
		auto predicate = [](tref n) { return chtree::get(n).value == 'a'; };
		trefs expected {root};
		trefs result = select_all<char, decltype(predicate)>(root, predicate);
		CHECK( cmp<char>(result, expected) );
	}

	TEST_CASE("select_all: given a tree with two children -the right one "
			"matching the predicate-, it returns the matching child") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) { return chtree::get(n).value == 'c'; };
		trefs expected {n('c')};
		trefs result = select_all<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("select_all: given a tree with two children -the left one "
			"matching the predicate-, it returns the matching child") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) { return chtree::get(n).value == 'b'; };
		trefs expected {n('b')};
		trefs result = select_all<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("select_all: given a tree with two children -both matching the "
			"predicate-, it returns both") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) { return chtree::get(n).value == 'b'
					|| chtree::get(n).value == 'c'; };
		trefs expected { n('b'), n('c') };
		trefs result = select_all<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("select_all: given a tree with two children -both matching the "
			"predicate-, it returns both") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) { return chtree::get(n).value == 'b'
					|| chtree::get(n).value == 'c'; };
		trefs expected { n('b'), n('c') };
		trefs result = select_all<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}
}

// select_all_until
// select_top_until

TEST_SUITE("find_top") {

	TEST_CASE("find_top: given a simple tree and a predicate not satisfied by "
			"the root, it returns an empty optional") {
		tref root = n('a');
		auto predicate = [](tref n) { return chtree::get(n).value == 'c'; };
		CHECK( !find_top<char, decltype(predicate)>(root, predicate) );
	}

	TEST_CASE("find_top: given a simple tree and a predicate satisfied by the "
			"root, it returns the root") {
		tref root = n('a');
		auto predicate = [](tref n) { return chtree::get(n).value == 'a'; };
		tref expected {root};
		tref result = find_top<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("find_top: given a tree with two children -the right one matching "
			"the predicate-, it returns the matching node") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) { return chtree::get(n).value == 'c'; };
		tref expected {n('c')};
		tref result = find_top<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("find_top: given a tree with two children -the left one matching "
			"the predicate-, it returns the matching node") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) { return chtree::get(n).value == 'b'; };
		tref expected {n('b')};
		tref result = find_top<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("find_top: given a tree with two children -both matching the "
			"predicate-, it returns the first matching node") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) { return chtree::get(n).value == 'b'
					|| chtree::get(n).value == 'c'; };
		tref expected { n('b') };
		tref result = find_top<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("find_top: given a tree with an underlying diamond like DAG "
			"and a predicate satisfied by the bottom nodes, it returns the "
			"first matching node") {
		tref root = n('a', {n('b', {n('d')}), n('c', {n('d')})});
		auto predicate = [](tref n) { return chtree::get(n).value == 'b'
						|| chtree::get(n).value == 'c'
						|| chtree::get(n).value == 'd';};
		tref expected { n('b', {n('d')}) };
		tref result = find_top<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}
}

// find_top_until

TEST_SUITE("find_bottom") {

	TEST_CASE("find_bottom: given a simple tree and a predicate not satisfied by "
			"the root, it returns an empty optional") {
		tref root = n('a');
		auto predicate = [](tref n) { return chtree::get(n).value == 'c'; };
		CHECK( !find_bottom<char, decltype(predicate)>(root, predicate) );
	}

	TEST_CASE("find_bottom: given a simple tree and a predicate satisfied by the "
			"root, it returns the root") {
		tref root = n('a');
		auto predicate = [](tref n) { return chtree::get(n).value == 'a'; };
		tref expected {root};
		tref result = find_bottom<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("find_bottom: given a tree with two children -the right one matching "
			"the predicate-, it returns the matching node") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) { return chtree::get(n).value == 'c'; };
		tref expected {n('c')};
		tref result = find_bottom<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("find_bottom: given a tree with two children -the left one matching "
			"the predicate-, it returns the matching node") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) { return chtree::get(n).value == 'b'; };
		tref expected {n('b')};
		tref result = find_bottom<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("find_bottom: given a tree with two children -both matching the "
			"predicate-, it returns the first matching node") {
		tref root = n('a', {n('b'), n('c')});
		auto predicate = [](tref n) { return chtree::get(n).value == 'b'
					|| chtree::get(n).value == 'c'; };
		tref expected = n('b');
		tref result = find_bottom<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("find_bottom: given a tree with an underlying diamond like DAG "
			"and a predicate satisfied by the bottom nodes, it returns the "
			"first matching node") {
		tref root = n('a', {n('b', {n('d')}), n('c', {n('d')})});
		auto predicate = [](tref n) {
			auto v = chtree::get(n).value;
			return v == 'b' || v == 'c' || v == 'd';};
		tref expected { n('d') };
		tref result = find_bottom<char, decltype(predicate)>(root, predicate);
		CHECK( cmp(result, expected) );
	}
}

TEST_SUITE("replace") {

	TEST_CASE("replace: given a tree and a map of replacements, it returns the tree with the replacements applied") {
		tref root = n('a');
		std::map<tref, tref>
			env = {{ n('a'), { n('b', { n('c'), n('d') }) } }};
		tref expected = n('b', { n('c'), n('d') });
		tref result = replace<char>(root, env);
		// chtree::get(root).print(std::cout << "root: ");
		// chtree::get(expected).print(std::cout << "expected: ");
		// chtree::get(result).print(std::cout << "result: ");
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("replace: given a tree and a map of replacements, it returns the tree with the replacements applied") {
		tref root = n('a', { n('a') });
		subtree_map<char, tref>
			env = {{ n('a'), { n('b', { n('c'), n('d') }) } }};
		tref expected = n('a', { n('b', { n('c'), n('d') }) });
		tref result = replace<char>(root, env);
		// chtree::get(root).print(std::cout << "root: ");
		// chtree::get(expected).print(std::cout << "expected: ");
		// chtree::get(result).print(std::cout << "result: ");
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("replace: given a tree and a map of replacements, it returns the tree with the replacements applied") {
		tref root = n('e', { n('f'), n('a'), n('g') });
		subtree_map<char, tref>
			env = {{ n('a'), { n('b', { n('c'), n('d') }) } }};
		tref expected = n('e', { n('f'), n('b', { n('c'), n('d') }), n('g') });
		tref result = replace<char>(root, env);
		// chtree::get(root).print(std::cout << "root: ");
		// chtree::get(expected).print(std::cout << "expected: ");
		// chtree::get(result).print(std::cout << "result: ");
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("replace: given a tree and a map of replacements, it returns the tree with the replacements applied") {
		tref root = n('e', { n('a'), n('a') });
		subtree_map<char, tref>
			env = {{ n('a'), { n('b', { n('c'), n('d') }) } }};
		tref expected = n('e', { n('b', { n('c'), n('d') }), n('b', { n('c'), n('d') }) });
		tref result = replace<char>(root, env);
		// chtree::get(root).print(std::cout << "root: ");
		// chtree::get(expected).print(std::cout << "expected: ");
		// chtree::get(result).print(std::cout << "result: ");
		CHECK( cmp(result, expected) );
	}


	TEST_CASE("replace: given a tree and a map of replacements, it returns the tree with the replacements applied") {
		tref root = n('a', { n('b'), n('c') });
		subtree_map<char, tref>
			env = {{ n('b'), { n('d', { n('e'), n('f') }) } }};
		tref expected = n('a', { n('d', { n('e'), n('f') }), n('c') });
		tref result = replace<char>(root, env);
		// chtree::get(root).print(std::cout << "root: ");
		// chtree::get(expected).print(std::cout << "expected: ");
		// chtree::get(result).print(std::cout << "result: ");
		CHECK( cmp(result, expected) );
	}

	TEST_CASE("replace: given a tree and a map of replacements, it returns the tree with the replacements applied") {
		tref root = n('a', { n('b'), n('c'), n('b') });
		subtree_map<char, tref>
			env = {{ n('b'), { n('d', { n('e') }) } }};
		tref expected = n('a', { n('d', { n('e') }), n('c'), n('d', { n('e') }) });
		tref result = replace<char>(root, env);
		// chtree::get(root).print(std::cout << "root: ");
		// chtree::get(expected).print(std::cout << "expected: ");
		// chtree::get(result).print(std::cout << "result: ");
		CHECK( cmp(result, expected) );
	}

}

TEST_SUITE("pattern_matcher") {

	TEST_CASE("match capture") {
		tref root {n('a', {n('b')})};
		tref pattern {n('a', {n('X')})};
		tref env {n('a', {n('X'), n('X')})};
		rule r{ chtree::geth(pattern), chtree::geth(env) };
		is_capture_predicate is_capture;
		pattern_matcher2<char, is_capture_predicate> matcher(r, is_capture);
		auto res = matcher(root);
		CHECK( res == true );
		CHECK( matcher.changes.size() == 1 );
	}
}


TEST_SUITE("apply_rule") {

	TEST_CASE("apply: given tree with one child and a environment that "
			"transform a node with one children into two, it returns the "
			"tree with the environment applied") {
		tref root {n('a', {n('b')})};
		tref pattern {n('a', {n('X')})};
		tref env {n('a', {n('X'), n('X')})};
		rule r{ chtree::geth(pattern), chtree::geth(env) };
		tref expected = n('a', {n('b'), n('b')});
		is_capture_predicate is_capture;
		auto replaced = apply_rule<char, is_capture_predicate>(r, root, is_capture) ;
		// chtree::get(replaced).print(std::cout << "replaced: ");
		// chtree::get(expected).print(std::cout << "expected: ");
		CHECK( cmp(replaced, expected) );
	}
}

// TEST_SUITE("apply_if") { }

TEST_SUITE("apply") {

	struct is_capture_predicate {

		bool operator()(const tref& n) const {
			auto v = chtree::get(n).value;
			return v == 'X' || v == 'Y' || v == 'Z';
		}
	};

	TEST_CASE("apply: given tree with one child and a environment that "
	 		"transform a node with one children into two, it returns the "
			"tree with the environment applied") {
		tref root {n('a', {n('b')})};
		tref pattern {n('a', {n('X')})};
		tref env {n('a', {n('X'), n('X')})};
		rule r{ chtree::geth(pattern), chtree::geth(env) };
		tref expected = n('a', {n('b'), n('b')});
		is_capture_predicate is_capture;
		auto replaced = apply_rule<char, is_capture_predicate>(r, root, is_capture) ;
		// chtree::get(replaced).print(std::cout << "replaced: ");
		// chtree::get(expected).print(std::cout << "expected: ");
		CHECK( cmp(replaced, expected) );
	}

	TEST_CASE("apply: given tree with one child and a environment that "
			"transform that ignore the children node and replace the root node, "
			"it returns the tree with the environment applied") {
		tref root {n('a', {n('b')})};
		tref pattern {n('a', {n('X')})};
		tref env {n('a')};
		rule r{ chtree::geth(pattern), chtree::geth(env) };
		tref expected = n('a');
		is_capture_predicate is_capture;
		auto replaced = apply_rule<char, is_capture_predicate>(r, root, is_capture) ;
		CHECK( cmp(replaced, expected) );
	}

	TEST_CASE("apply: given tree with two children and a environment that "
			"transform that swaps the children, it returns the tree with the "
			"environment applied") {
		tref root {n('a', {n('b'), n('c')})};
		tref pattern {n('a', {n('X'), n('Y')})};
		tref env {n('a', {n('Y'), n('X')})};
		rule r{ chtree::geth(pattern), chtree::geth(env) };
		tref expected = n('a', {n('c'), n('b')});
		is_capture_predicate is_capture;
		auto replaced = apply_rule<char, is_capture_predicate>(r, root, is_capture) ;
		CHECK( cmp(replaced, expected) );
	}

	TEST_CASE("apply: given tree with two children and a environment that "
			"transform that swaps the children, it returns the tree with the "
			"environment applied") {
		tref root {n('a', {n('b'), n('c', {n('d'), n('e')})})};
		tref pattern {n('a', {n('X'), n('c', {n('Y'), n('Z')})})};
		tref env {n('a', {n('Y'), n('c', {n('Z'), n('X')})})};
		rule r{ chtree::geth(pattern), chtree::geth(env) };
		tref expected = n('a', {n('d'), n('c', {n('e'), n('b')})});
		is_capture_predicate is_capture;
		auto replaced = apply_rule<char, is_capture_predicate>(r, root, is_capture) ;
		CHECK( cmp(replaced, expected) );
	}

	TEST_CASE("apply: given a tree with a diamond like DAG and a environment "
			"that breaks the diamond like shape, it returns the tree with the "
			"environment applied") {
		tref root {n('a', {n('b', {n('d')}), n('c', {n('d')})})};
		tref pattern {n('b', {n('X')})};
		tref env {n('b', {n('e')})};
		rule r{ chtree::geth(pattern), chtree::geth(env) };
		tref expected = n('a', {n('b', {n('e')}), n('c', {n('d')})});
		is_capture_predicate is_capture;
		auto replaced = apply_rule<char, is_capture_predicate>(r, root, is_capture) ;
		CHECK( cmp(replaced, expected) );
	}

	TEST_CASE("apply: given a tree with a diamond like DAG and a environment "
			"that swaps the intermediate children, it returns the tree with the "
			"environment applied") {
		tref root {n('a', {n('b', {n('d')}), n('c', {n('d')})})};
		tref pattern {n('a', {n('X'), n('Y')})};
		tref env {n('a', {n('Y'), n('X')})};
		rule r{ chtree::geth(pattern), chtree::geth(env) };
		tref expected = n('a', {n('c', {n('d')}), n('b', {n('d')})});
		is_capture_predicate is_capture;
		auto replaced = apply_rule<char, is_capture_predicate>(r, root, is_capture);
		CHECK( cmp(replaced, expected) );
	}
}
