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

// DO NOT INCLUDE THIS FILE DIRECTLY!

// This file is included into tree.h
// rewriter namespace is separated from tree.h to here and tree_rewriter.tmpl.h
// for clarity.

#include "tree.h"

//------------------------------------------------------------------------------
// rewriter types

namespace idni::rewriter {

using rule = std::pair<htree::sp, htree::sp>;
using rules = std::vector<rule>;
using library = rules;

// a environment is a map from captures to tree nodes, it is used
// to keep track of the captures that have been unified and their
// corresponding tree nodes.
using environment = std::map<tref, tref>;


//------------------------------------------------------------------------------

// visitor that selects top nodes that satisfy a predicate and stores them in the
// supplied vector. It only works with post order traversals and never produces
// duplicates.
// TODO (MEDIUM) replace vector by set
template <typename predicate_t>
struct select_top_predicate {
	select_top_predicate(predicate_t& query, trefs& selected);
	bool operator()(tref n);
	predicate_t& query;
	trefs& selected;
};

// visitor that selects nodes that satisfy a predicate and stores the subnodes
// extracted from them in the supplied vector. It only works with post order
// traversals and never produces duplicates.
// TODO (MEDIUM) replace vector by set
template <typename predicate_t, typename extractor_t>
struct select_subnodes_predicate {
	select_subnodes_predicate(predicate_t& query, extractor_t extractor,
		trefs& selected);
	bool operator()(tref n);
	predicate_t& query;
	extractor_t& extractor;
	trefs& selected;
};

// visitor that selects nodes that satisfy a predicate and stores them in the
// supplied vector.
// TODO (MEDIUM) replace vector by set
template <typename predicate_t>
struct select_all_predicate {
	select_all_predicate(predicate_t& query, trefs& selected);
	bool operator()(tref n);
	predicate_t& query;
	trefs& selected;
};

// visitor that selects nodes that satisfy a predicate and stores them in the
// supplied vector.
template <typename predicate_t>
struct find_top_predicate {
	find_top_predicate(predicate_t& query, tref& found);
	bool operator()(tref n);
	predicate_t& query;
	tref& found;
};

// always true predicate
//
// TODO (LOW) define a const version of the predicate, move it to rewriter and use
// it in all the code. Review the connection with all predicate and use only
// one of them.
// Check if we have other cases like this one
static const auto true_predicate = [](tref) { return true; };
using true_predicate_t = decltype(true_predicate);

// always false predicate
//
// TODO (LOW) define a const version of the predicate as a lambda, move it to rewriter and use
// it in all the code.
// Check if we have other cases like this one
static const auto false_predicate = [](tref) { return false; };
using false_predicate_t = decltype(false_predicate);

// disjuction of the wrapped predicates.
//
// IDEA we use combinators to build logical predicates. This could be simplified
// by overloading the operators &&, ||, !, etc.
template <typename l_predicate_t, typename r_predicate_t>
struct and_predicate {
	and_predicate(l_predicate_t& p1, r_predicate_t& p2) : p1(p1), p2(p2) {}
	bool operator()(tref n) const { return p1(n) && p2(n);	}
	l_predicate_t& p1;
	r_predicate_t& p2;
};

template <typename l_predicate_t, typename r_predicate_t>
using and_predicate_t = and_predicate<l_predicate_t, r_predicate_t>;

// disjuction of the wrapped predicates.
//
// IDEA we use combinators to build logical predicates. This could be simplified
// by overloading the operators &&, ||, !, etc.
template <typename l_predicate_t, typename r_predicate_t>
struct or_predicate {
	or_predicate(l_predicate_t& p1, r_predicate_t& p2) : p1(p1), p2(p2) {}
	bool operator()(tref n) const { return p1(n) || p2(n); }
	l_predicate_t& p1;
	r_predicate_t& p2;
};

template <typename l_predicate_t, typename r_predicate_t>
using or_predicate_t = or_predicate<l_predicate_t, r_predicate_t>;

// negation of the wrapped predicate.
//
// IDEA we use combinators to build logical predicates. This could be simplified
// by overloading the operators &&, ||, !, etc.
template <typename predicate_t>
struct neg_predicate {
	neg_predicate(predicate_t& p) : p(p) {}
	bool operator()(tref n) const { return !p(n); }
	predicate_t& p;
};

template <typename predicate_t>
using neg_predicate_t = neg_predicate<predicate_t>;

// true while found is not set (found), it aborts the traversal once found has
// been set.
struct while_not_found_predicate {
	while_not_found_predicate(tref& found) : found(found) {}
	bool operator()(tref) const { return found != nullptr; }
	tref& found;
};

// to be used in conjunction with while_not_found_predicate. It sets found when
// the predicate is satisfied by a node (set in found).
template <typename predicate_t>
struct find_visitor {
	find_visitor(predicate_t& query, tref& found);
	tref operator()(tref n) const;
	predicate_t& query;
	tref& found;
};

//----------------------------------------------------------------------
// former rewriter predicate API traversing and transformations

// visitor that traverse the tree in post-order (avoiding visited nodes).
template <typename node_t, typename wrapped_t, typename predicate_t>
struct post_order_traverser {
	post_order_traverser(wrapped_t& wrapped, predicate_t& query);
	tref operator()(tref n);
	wrapped_t& wrapped;
	predicate_t& query;
private:
	tref traverse(tref n, std::set<tref>& visited);
};

// visitor that traverse the tree in post-order (avoiding visited nodes).
template <typename node_t, typename wrapped_t, typename predicate_t>
struct post_order_query_traverser {
	post_order_query_traverser(wrapped_t& wrapped, predicate_t& query);
	tref operator()(tref n);
	wrapped_t& wrapped;
	predicate_t& query;
private:
	tref found = nullptr;
	tref traverse(tref n, std::set<tref>& visited);
};

// visitor that traverse the tree in post-order (repeating visited nodes if necessary).
template <typename node_t, typename wrapped_t, typename predicate_t>
struct post_order_tree_traverser {
	post_order_tree_traverser(wrapped_t& wrapped, predicate_t& query);
	tref operator()(tref n);
	wrapped_t& wrapped;
	predicate_t& query;
private:
	tref traverse(tref n);
};

// Do not use. Use post_order or pre_order instead depending on the needs.
template <typename node_t>
struct post_order_recursive_traverser {
	auto operator() (tref n, auto& query, auto& wrapped);
private:
	tref traverse(tref n, auto& query, auto& wrapped);
};

// visitor that produces nodes transformed accordingly to the
// given transformer. It only works with post order traversals.
template <typename node_t, typename wrapped_t>
struct map_transformer {
	map_transformer(wrapped_t& wrapped);
	tref operator()(tref& n);
	std::map<tref, tref> changes;
	wrapped_t& wrapped;
};

// visitor that produces nodes transformed accordingly to the
// given transformer. It only works with post order traversals.
template <typename node_t, typename wrapped_t>
struct map_node_transformer {
	// REVIEW (MEDIUM) check the implementation of this transformer, it seems buggy
	map_node_transformer(wrapped_t& wrapped);
	tref operator()(tref n);
	std::map<tref, tref> changes;
	wrapped_t& wrapped;
};

// visitor that produces nodes transformed accordingly to the
// given transformer. It only works with post order traversals.
//
// TODO (MEDIUM) merge replace and replace_node transformers into one.
template <typename node_t, typename wrapped_t>
struct replace_node_transformer {
	replace_node_transformer(wrapped_t& wrapped);
	tref operator()(tref n);
	std::map<tref, tref> changes;
	wrapped_t& wrapped;
};

// visitor that produces nodes transformed accordingly to the
// given transformer. It only works with post order traversals.
template <typename node_t>
struct replace_transformer {
	replace_transformer(std::map<tref, tref>& changes);
	tref operator()(tref n);
	std::map<tref, tref>& changes;
private:
	tref replace(tref n);
};

// TODO (MEDIUM) simplify matchers code and extract common code.

// this predicate matches when there exists a environment that makes the
// pattern match the node.
//
// TODO (LOW) create and env in operator() and pass it as a parameter to match, if
// a  match occurs, copy the data from the temp env to the env passed as
// parameter.
//
// It should allow to detects matches in the middle of a tree.
template <typename node_t, typename is_capture_t>
struct pattern_matcher {
	pattern_matcher(tref pattern, rewriter::environment& env,
		const is_capture_t& is_capture);
	bool operator()(tref n);
	tref matched = nullptr;
	tref pattern;
	rewriter::environment& env;
	const is_capture_t& is_capture;
private:
	bool match(tref p, tref n);
};

template <typename node_t, typename is_capture_t>
struct pattern_matcher2 {
	pattern_matcher2(const rewriter::rule& r,
		const is_capture_t& is_capture);
	bool operator()(tref n);
	const rewriter::rule& r;
	rewriter::environment env;
	rewriter::environment changes;
	const is_capture_t& is_capture;
	tref replace_root(tref n);
private:
	bool match(tref p, tref n);
};


// this predicate matches when there exists a environment that makes the
// pattern match the node ignoring the nodes detected as skippable.
//
// TODO (LOW) create an env in operator() and pass it as a parameter to match, if
// a  match occurs, copy the data from the temp env to the env passed as
// parameter.
template <typename node_t, typename is_capture_t, typename predicate_t>
struct pattern_matcher_if {
	pattern_matcher_if(tref pattern, rewriter::environment& env,
		is_capture_t& is_capture, predicate_t& predicate);
	bool operator()(tref n);
	tref matched = nullptr;
	tref pattern;
	rewriter::environment& env;
	is_capture_t& is_capture;
	predicate_t& predicate;
private:
	bool match(tref p, tref n);
};

// delete all top nodes that satisfy a predicate.
template <typename node_t, typename predicate_t>
tref trim_top(tref input, predicate_t& query);

// select all top nodes that satisfy a predicate and return them.
template <typename node_t, typename predicate_t>
trefs select_top(tref input, predicate_t& query);

// select all subnodes that satisfy a predicate according to the extractor and return them.
template <typename node_t, typename predicate_t, typename extractor_t>
trefs select_subnodes(tref input, predicate_t& query, extractor_t extractor);

// select all nodes that satisfy a predicate and return them.
template <typename node_t, typename predicate_t>
trefs select_all(tref input, predicate_t& query);

// Select all nodes in input that satisfy query ignoring subtrees under a node satisfying until
template <typename node_t>
trefs select_all_until(tref input, const auto& query, const auto& until);

// Select top nodes in input that satisfy query ignoring subtrees under a node satisfying until
template <typename node_t>
trefs select_top_until(tref input, const auto& query, const auto& until);

// find the first node that satisfy a predicate and return it.
template <typename node_t, typename predicate_t>
tref find_top(tref input, predicate_t& query);

// Select single top node in input that satisfy query ignoring subtrees under a node satisfying until
template <typename node_t>
tref find_top_until(tref input, const auto& query, const auto& until);

template <typename node_t>
tref replace(tref n, const std::map<tref, tref>& changes);

template <typename node_t>
tref replace(tref n, tref replace, tref with);

// Replace nodes in n according to changes while skipping subtrees that don't satisfy query
template <typename node_t, typename predicate_t>
tref replace_if(tref n, const std::map<tref, tref>& changes, predicate_t& query);

// Replace nodes in n according to changes while skipping subtrees that satisfy query
template <typename node_t, typename predicate_t>
tref replace_until(tref n, const std::map<tref, tref>& changes, predicate_t& query);

// TODO (LOW) consider adding a similar functino for replace_node...


// find the first node that satisfy a predicate and return it.
template <typename node_t, typename predicate_t>
tref find_bottom(tref input, predicate_t& query);

// apply a rule to a tree using the predicate to pattern_matcher.
template <typename node_t, typename is_capture_t>
tref apply_rule(const rewriter::rule& r, tref n, const is_capture_t& c);

// apply a rule to a tree using the predicate to pattern_matcher and skipping
// unnecessary subtrees
template <typename node_t, typename is_capture_t, typename predicate_t>
tref apply_if(const rewriter::rule& r, tref n,
	is_capture_t& c, predicate_t& predicate);

// apply a substitution to a rule according to a given matcher, this method is
// use internaly by apply and apply with skip.
template <typename node_t, typename matcher_t>
tref apply(tref s, tref n, matcher_t& matcher);

} // idni::rewriter namespace

