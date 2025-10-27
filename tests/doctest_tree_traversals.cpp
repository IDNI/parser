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