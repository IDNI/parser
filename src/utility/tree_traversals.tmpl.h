// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.

#ifndef __IDNI__PARSER__TREE_TRAVERSALS_TMPL_H__
#define __IDNI__PARSER__TREE_TRAVERSALS_TMPL_H__

#include "tree.h"

namespace idni {

// #define LOG_TRAVERSALS_ENABLED

#ifdef LOG_TRAVERSALS_ENABLED

#define DBGT(x) x

template <typename node>
void print_stack(const std::vector<tref>& stack, tref current) {
	std::cout << "\tstack: ";
	for (tref n : stack) {
		std::cout << "\n\t\t";
		std::stringstream ss;
		const auto& c_tree = lcrs_tree<node>::get(n);
		ss << c_tree.value << ":";
		while (ss.tellp() < 16) ss << " ";
		ss      << (n == current ? "[" : " ") << n
			<< (n == current ? "]" : " ");
		if (c_tree.l) ss << " __ " << c_tree.l;
		if (c_tree.r) ss << " >> " << c_tree.r;
		std::cout << ss.str();
	}
	std::cout << "\n";
};

#else
#	define DBGT(x)
#endif // LOG_TRAVERSALS_ENABLED

#ifdef MEASURE_TRAVERSER_DEPTH

static size_t depth = 0;
static size_t max_depth = 0;

static void inc_depth() {
	++depth;
	static bool printed = false;
	if (depth > max_depth) {
		max_depth = depth;
		if (max_depth % 5000 != 0) printed = false;
		else if (!printed) {
			std::cout << "max_depth: " << max_depth << "\n";
			printed = true;
		}
	}
}

static void dec_depth() {
	--depth;
}

template <typename node>
std::pair<size_t, size_t> pre_order<node>::get_tree_depth_and_size() {
	size_t c_depth = depth;
	size_t t_max_depth = depth;
	size_t size;
	auto find_max_depth = [&t_max_depth, &size] (const auto&){
		++size;
		if (depth > t_max_depth)
			t_max_depth = depth;
		return true;
	};
	visit(find_max_depth);
	return {t_max_depth - c_depth, size};
}

#endif // MEASURE_TRAVERSER_DEPTH

} // idni namespace

#include "tree_traversals_pre_order.tmpl.h"
#include "tree_traversals_post_order.tmpl.h"
#include "tree_traversals_morris.tmpl.h"

#endif // __IDNI__PARSER__TREE_TRAVERSALS_TMPL_H__