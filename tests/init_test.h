// To view the license please visit https://github.com/IDNI/tau-lang/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__TESTS__INIT_TEST_H__
#define __IDNI__PARSER__TESTS__INIT_TEST_H__

#include <cassert>
#include "doctest.h"

// namespace testing = doctest;

#include "utility/tree.h"

using namespace idni;
using namespace idni::rewriter;

using chtree = idni::lcrs_tree<char>;
using strtree = idni::lcrs_tree<std::string>;
using inttree = idni::lcrs_tree<int_t>;

template <typename T = char>
tref n(const char& value) {
	return lcrs_tree<T>::get(value);
}

template <typename T = char>
tref n(const char& value, const trefs& children) {
	return lcrs_tree<T>::get(value, children);
}

template <typename T = char>
const lcrs_tree<T>& d(const char& value) {
	return lcrs_tree<T>::get(n<T>(value));
}

template <typename T = char>
const lcrs_tree<T>& d(const char& value, const trefs& children) {
	return lcrs_tree<T>::get(n<T>(value, children));
}


template <typename T = char>
bool cmp(tref a, tref b) {
	if (a == nullptr) return b == nullptr;
	if (b == nullptr) return false;
	// std::cout << lcrs_tree<T>::get(a).value << " *" << a << " >>" << lcrs_tree<T>::get(a).right_sibling() << " __" << lcrs_tree<T>::get(a).left_child() << "\t ==\t ";
	// std::cout << lcrs_tree<T>::get(b).value << " *" << b << " >>" << lcrs_tree<T>::get(b).right_sibling() << " __" << lcrs_tree<T>::get(b).left_child() << "\t =\t ";
	// std::cout << (lcrs_tree<T>::get(a) == lcrs_tree<T>::get(b)) << " ";
	// std::cout << (lcrs_tree<T>::get(a) == lcrs_tree<T>::get(b)) << " ";
	// std::cout << (bintree<T>::get(a) == bintree<T>::get(b)) << " ";
	// std::cout << (lcrs_tree<T>::get(a).value == lcrs_tree<T>::get(b).value) << " ";
	// std::cout << "\n";
	return lcrs_tree<T>::subtree_equals(a, b);
}

template <typename T = char>
bool cmp(const trefs& a, const trefs& b) {
	// std::cout << "--------\n";
	// std::cout << "cmp got: " << a.size() << " exp: " << b.size() << "\n";
	// for (size_t i = 0; i < a.size(); ++i) {
	// 	std::cout << "cmp got: " << lcrs_tree<T>::get(a[i]).value << " exp: " << lcrs_tree<T>::get(b[i]).value << "\n";
	// }
	if (a.size() != b.size()) return false;
	for (size_t i = 0; i < a.size(); ++i) {
		if (!cmp<T>(a[i], b[i])) return false;
	}
	return true;
}

template <typename T = char>
bool cmp(const environment<T>& a, const environment<T>& b) {
	// std::cout << "--------\n";
	// std::cout << "cmp got: " << a.size() << " exp: " << b.size() << "\n";
	if (a.size() != b.size()) return false;
	auto a_it = a.begin();
	auto b_it = b.begin();
	while (a_it != a.end() && b_it != b.end()) {
		// std::cout << "cmp got: " << a_it->first << " exp: " << b_it->first << "\n";
		if (!cmp<T>(a_it->first, b_it->first)) return false;
		if (!cmp<T>(a_it->second, b_it->second)) return false;
		a_it++;
		b_it++;
	}
	return true;
}

struct test_tree_fixture {
	test_tree_fixture() {
		in = n('e', {
                        n('b', {
                                n('d', {
                                        n('g'),
                                        n('h')
                                }),
                                n('e'),
                                n('c', {
                                        n('g', {
                                                n('o'),
                                                n('p')
                                        }),
                                        n('h'),
                                        n('b', {
                                                n('d'),
                                                n('e', {
                                                        n('i'),
                                                        n('j')
                                                })
                                        })
                                })
                        }),
                        n('e', {
                                n('f'),
                                n('b', {
                                        n('e', {
                                                n('f'),
                                                n('c')
                                        }),
                                        n('g', {
                                                n('o'),
                                                n('p')
                                        }),
                                        n('e')
                                })
                        })
                });
	}
	tref in;
};

#endif // __IDNI__PARSER__TESTS__INIT_TEST_H__