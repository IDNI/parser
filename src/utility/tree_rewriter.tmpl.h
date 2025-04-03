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

#include "tree.h"

namespace idni {

// former rewriter predicate API

namespace rewriter {

// ---------------------------------------------------------------------
// predicates

template <typename predicate_t>
select_top_predicate<predicate_t>::select_top_predicate(predicate_t& query,
	trefs& selected) : query(query), selected(selected) {}

template <typename predicate_t>
bool select_top_predicate<predicate_t>::operator()(tref n) {
	if (!query(n)) return true;
	// we return false to avoid visiting the children of the node
	// since we are only interested in the top nodes.
	if (std::find(selected.begin(), selected.end(), n)
			== selected.end()) selected.push_back(n);
	return false;
}

template <typename predicate_t, typename extractor_t>
select_subnodes_predicate<predicate_t, extractor_t>::
	select_subnodes_predicate(predicate_t& query, extractor_t extractor,
	trefs& selected)
	: query(query), extractor(extractor), selected(selected) {}

template <typename predicate_t, typename extractor_t>
bool select_subnodes_predicate<predicate_t, extractor_t>::operator()(tref n) {
	if (!query(n)) return true;
	auto extracted = extractor(n);
	for (tref e : extracted)
		if (std::find(selected.begin(), selected.end(), e)
			== selected.end()) selected.push_back(e);
	return false;
}

template <typename predicate_t>
select_all_predicate<predicate_t>::select_all_predicate(predicate_t& query,
	trefs& selected) : query(query), selected(selected) {}

template <typename predicate_t>
bool select_all_predicate<predicate_t>::operator()(tref n) {
	if (query(n)) selected.push_back(n);
	// we always return true to visit all the nodes.
	return true;
}

template <typename predicate_t>
find_top_predicate<predicate_t>::find_top_predicate(predicate_t& query, tref& found)
	: query(query), found(found) {}

template <typename predicate_t>	
bool find_top_predicate<predicate_t>::operator()(tref n) {
	if (found == nullptr && query(n)) found = n;
	return found != nullptr;
}

// -----------------------------------------------------------------------------
// traversers

template <typename node_t, typename wrapped_t, typename predicate_t>
post_order_traverser<node_t, wrapped_t, predicate_t>
	::post_order_traverser(wrapped_t& wrapped, predicate_t& query)
	: wrapped(wrapped), query(query) {}

template <typename node_t, typename wrapped_t, typename predicate_t>
tref post_order_traverser<node_t, wrapped_t, predicate_t>::operator()(tref n) {
	// we kept track of the visited nodes to avoid visiting the same node
	// twice. However, we do not need to keep track of the root node, since
	// it is the one we start from and we will always be visited.
	std::set<tref> visited;
	// if the root node matches the query predicate, we traverse it, otherwise
		// we return the result of apply the wrapped transform to the node.
	return query(n) ? traverse(n, visited) : wrapped(n);
}

template <typename node_t, typename wrapped_t, typename predicate_t>
tref post_order_traverser<node_t, wrapped_t, predicate_t>::traverse(
	tref n, std::set<tref>& visited)
{
	// we traverse the children of the node in post-order, i.e. we visit
	// the children first and then the node itself.
	for (tref c : lcrs_tree<node_t>::get(n).children())
		// we skip already visited nodes and nodes that do not match the
		// query predicate if it is present.
		if (!visited.contains(c) && query(c)) {
			if (traverse(c, visited) == nullptr) return nullptr;
			// we assume we have no cycles, i.e. there is no way we could
			// visit the same node again down the tree.
			// thus we can safely add the node to the visited set after
			// visiting it.
			visited.insert(c);
		}
	// finally we apply the wrapped visitor to the node if it is present.
	return wrapped(n);
}

// visitor that traverse the tree in post-order (avoiding visited nodes).
template <typename node_t, typename wrapped_t, typename predicate_t>
post_order_query_traverser<node_t, wrapped_t, predicate_t>::
	post_order_query_traverser(wrapped_t& wrapped, predicate_t& query)
		: wrapped(wrapped), query(query) {}

template <typename node_t, typename wrapped_t, typename predicate_t>
tref post_order_query_traverser<node_t, wrapped_t, predicate_t>::
	operator()(tref n)
{
	// we kept track of the visited nodes to avoid visiting the same node
	// twice. However, we do not need to keep track of the root node, since
	// it is the one we start from and we will always be visited.
	std::set<tref> visited;
	// if the root node matches the query predicate, we traverse it, otherwise
	// we return the result of apply the wrapped transform to the node.
	return traverse(n, visited);
}

template <typename node_t, typename wrapped_t, typename predicate_t>
tref post_order_query_traverser<node_t, wrapped_t, predicate_t>::traverse(
	tref n, std::set<tref>& visited)
{
	// we traverse the children of the node in post-order, i.e. we visit
	// the children first and then the node itself.
	if (found) return found;
	for (tref c : lcrs_tree<node_t>::get(n).children())
		// we skip already visited nodes and nodes that do not match the
		// query predicate if it is present.
		if (!visited.contains(c) && found == nullptr) {
			if (traverse(c, visited) == nullptr)
				return nullptr;
			// we assume we have no cycles, i.e. there is no way we could
			// visit the same node again down the tree.
			// thus we can safely add the node to the visited set after
			// visiting it.
			visited.insert(c);
			if (found == nullptr) found = query(c)
				? wrapped(c) : nullptr;
		}
	// finally we apply the wrapped visitor to the node if it is present.
	if (found == nullptr) found = query(n) ? wrapped(n) : nullptr;
	return found ? found : wrapped(n);
}

// TODO (LOW) add a post_order_traverser that does not have a wrapped transformer so
// it is faster when dealing with only predicate operations (searches,...) and
// change all the related code.

// visitor that traverse the tree in post-order (repeating visited nodes if necessary).
template <typename node_t, typename wrapped_t, typename predicate_t>
post_order_tree_traverser<node_t, wrapped_t, predicate_t>::
	post_order_tree_traverser(wrapped_t& wrapped, predicate_t& query) :
		wrapped(wrapped), query(query) {}

template <typename node_t, typename wrapped_t, typename predicate_t>
tref post_order_tree_traverser<node_t, wrapped_t, predicate_t>::
	operator()(tref n)
{
	// if the root node matches the query predicate, we traverse it, otherwise
	// we return the result of apply the wrapped transform to the node.
	return query(n) ? traverse(n) : wrapped(n);
}

template <typename node_t, typename wrapped_t, typename predicate_t>
tref post_order_tree_traverser<node_t, wrapped_t, predicate_t>::traverse(tref n) {
	// we traverse the children of the node in post-order, i.e. we visit
	// the children first and then the node itself.
	for (tref c : lcrs_tree<node_t>::get(n).children())
		// we skip already visited nodes and nodes that do not match the
		// query predicate if it is present.
		if (query(c)) if (traverse(c) == nullptr)
			return nullptr;
	// finally we apply the wrapped visitor to the node if it is present.
	return wrapped(n);
}

// Do not use. Use post_order or pre_order instead depending on the needs.
template <typename node_t>
auto post_order_recursive_traverser<node_t>::operator()(
	tref n, auto& query, auto& wrapped)
{
	if (query(n)) return traverse(n, query, wrapped);
	else return wrapped(n, lcrs_tree<node_t>::get(n).get_children());
}

template <typename node_t>
tref post_order_recursive_traverser<node_t>::traverse(
	tref n, auto& query, auto& wrapped)
{
	trefs children;
	for (tref c : lcrs_tree<node_t>::get(n).children()) {
		if (query(c))
			children.push_back(traverse(c, query, wrapped));
		else children.push_back(
			wrapped(c, lcrs_tree<node_t>::get(n).get_children()));
	}
	return wrapped(n, children);
}

// visitor that produces nodes transformed accordingly to the
// given transformer. It only works with post order traversals.

template <typename node_t, typename wrapped_t>
map_transformer<node_t, wrapped_t>::map_transformer(wrapped_t& wrapped) : wrapped(wrapped) {}

template <typename node_t, typename wrapped_t>
tref map_transformer<node_t, wrapped_t>::operator()(tref& n) {
	trefs ch;
	const auto& nt = lcrs_tree<node_t>::get(n);
	for (tref c : nt.children())
		if (auto it = changes.find(c); it != changes.end())
			ch.push_back(it->second);
	return changes[n] = lcrs_tree<node_t>::get(wrapped(nt.value), ch);
}

// visitor that produces nodes transformed accordingly to the
// given transformer. It only works with post order traversals.
// REVIEW (MEDIUM) check the implementation of this transformer, it seems buggy
template <typename node_t, typename wrapped_t>
map_node_transformer<node_t, wrapped_t>::map_node_transformer(
	wrapped_t& wrapped) : wrapped(wrapped) {}

template <typename node_t, typename wrapped_t>
tref map_node_transformer<node_t, wrapped_t>::operator()(tref n) {
	auto nn = wrapped(n);
	if (nn != n) { changes[n] == nn; return nn; };
	trefs ch;
	const auto& nt = lcrs_tree<node_t>::get(n);
	for (tref c : nt.children())
		if (auto it = changes.find(c); it != changes.end())
			ch.push_back(it->second);
		else ch.push_back(c);
	return changes[n] = lcrs_tree<node_t>::get(nt.value, std::move(ch));
}

// visitor that produces nodes transformed accordingly to the
// given transformer. It only works with post order traversals.
//
// TODO (MEDIUM) merge replace and replace_node transformers into one.
template <typename node_t, typename wrapped_t>
replace_node_transformer<node_t, wrapped_t>::replace_node_transformer(
	wrapped_t& wrapped) : wrapped(wrapped) {}

template <typename node_t, typename wrapped_t>
tref replace_node_transformer<node_t, wrapped_t>::operator()(tref n) {
	trefs ch;
	const auto& nt = lcrs_tree<node_t>::get(n);
	for (tref c : nt.children())
		if (auto it = changes.find(c); it != changes.end())
			ch.push_back(it->second);
	return changes[n] = lcrs_tree<node_t>::get(wrapped(nt.value), std::move(ch));
}

// visitor that produces nodes transformed accordingly to the
// given transformer. It only works with post order traversals.
template <typename node_t>
replace_transformer<node_t>::replace_transformer(std::map<tref, tref>& changes)
	: changes(changes) {}

template <typename node_t>
tref replace_transformer<node_t>::operator()(tref n) {
	auto it = changes.find(n);
	return it != changes.end() ? it->second : replace(n);
}

template <typename node_t>
tref replace_transformer<node_t>::replace(tref n) {
	trefs ch;
	const auto& nt = lcrs_tree<node_t>::get(n);
	for (tref c : nt.children())
		if (auto it = changes.find(c); it != changes.end())
			ch.push_back(it->second);
		else ch.push_back(c);
	return changes[n] = lcrs_tree<node_t>::get(nt.value, std::move(ch));
}

// delete all top nodes that satisfy a predicate.
template <typename node_t, typename predicate_t>
tref trim_top(tref input, predicate_t& query) {
	const auto t = [&query](tref n) {
		trefs ch; size_t l = 0;
		const auto& nt = lcrs_tree<node_t>::get(n);
		for (tref c : nt.children()) {
			l++;
			if (!query(c)) ch.push_back(c);
		}
		if (l != ch.size()) return lcrs_tree<node_t>::get(nt.value, std::move(ch));
		return n;
	};
	return pre_order<node_t>(input).apply_unique_until_change(t);
}


// select all top nodes that satisfy a predicate and return them.
template <typename node_t, typename predicate_t>
trefs select_top(tref input, predicate_t& query) {
	trefs selected;
	auto select = [&query, &selected](const auto& n) {
		if (!query(n)) return true;
		// we return false to avoid visiting the children of the node
		// since we are only interested in the top nodes.
		if (std::find(selected.begin(), selected.end(), n)
				== selected.end()) selected.push_back(n);
		return false;
	};
	pre_order<node_t>(input).visit_unique(select);
	return selected;
}


// select all subnodes that satisfy a predicate according to the extractor and return them.
template <typename node_t, typename predicate_t, typename extractor_t>
trefs select_subnodes(tref input, predicate_t& query, extractor_t extractor) {
	trefs selected;
	rewriter::select_subnodes_predicate<predicate_t, extractor_t> select(
		query, extractor, selected);
	pre_order<node_t>(input).visit_unique(select);
	return selected;
}

// select all nodes that satisfy a predicate and return them.
template <typename node_t, typename predicate_t>
trefs select_all(tref input, predicate_t& query) {
	trefs selected;
	auto select = [&query, &selected](const auto& n) {
		if (query(n)) selected.push_back(n);
		// we always return true to visit all the nodes.
		return true;
	};
	pre_order<node_t>(input).visit_unique(select);
	return selected;
}


// Select all nodes in input that satisfy query ignoring subtrees under a node satisfying until
template <typename node_t>
trefs select_all_until(tref input, const auto& query, const auto& until) {
	trefs selected;
	auto select = [&](tref n) {
		if (query(n)) selected.push_back(n);
		// we always return true until a node satisfies until.
		if (until(n)) return false;
		else return true;
	};
	pre_order<node_t>(input).visit_unique(select);
	return selected;
}


// Select top nodes in input that satisfy query ignoring subtrees under a node satisfying until
template <typename node_t>
trefs select_top_until(tref input, const auto& query, const auto& until) {
	trefs selected;
	auto select = [&](tref n) {
		if (until(n)) return false;
		if (!query(n)) return true;
		// we return false to avoid visiting the children of the node
		// since we are only interested in the top nodes.
		if (std::find(selected.begin(), selected.end(), n) == selected.end())
			selected.push_back(n);
		return false;
	};
	pre_order<node_t>(input).visit_unique(select);
	return selected;
}


// find the first node that satisfy a predicate and return it.
template <typename node_t, typename predicate_t>
tref find_top(tref input, predicate_t& query) {
	tref found = nullptr;
	auto find_top = [&query, &found](const auto& n) {
		if (found == nullptr && query(n)) found = n;
		return found == nullptr;
	};
	pre_order<node_t>(input).search_unique(find_top);
	return found;
}

// Select single top node in input that satisfy query ignoring subtrees under a node satisfying until
template <typename node_t>
tref find_top_until(tref input, const auto& query, const auto& until) {
	tref node;
	auto select = [&](tref n) {
		if (!query(n)) return true;
		if (node != nullptr) return false;
		// Query is true and node has not been found, hence set node to n
		return node = n, false;
	};
	const auto neg_until = [&until](const auto& p) {
		return !until(p);
	};
	pre_order<node_t>(input).search_unique(select, neg_until);
	return node;
}


template <typename node_t>
tref replace(tref n, const std::map<tref, tref>& changes) {
	const auto r = [&changes](tref el) {
		if (const auto it = changes.find(el); it != changes.end())
			return it->second;
		else return el;
	};
	return pre_order<node_t>(n).apply_unique_until_change(r);
}


template <typename node_t>
tref replace(tref n, tref replace, tref with) {
	const auto r = [&](tref el) {
		if (el == replace) return with;
		else return el;
	};
	return pre_order<node_t>(n).apply_unique_until_change(r);
}


// Replace nodes in n according to changes while skipping subtrees that don't satisfy query
template <typename node_t, typename predicate_t>
tref replace_if(tref n, const std::map<tref, tref>& changes, predicate_t& query) {
	const auto r = [&changes](tref el) {
		if (const auto it = changes.find(el); it != changes.end())
			return it->second;
		else return el;
	};
	return pre_order<node_t>(n).apply_unique_until_change(r, query);
}

// Replace nodes in n according to changes while skipping subtrees that satisfy query
template <typename node_t, typename predicate_t>
tref replace_until(tref n, const std::map<tref, tref>& changes, predicate_t& query) {
	const auto r = [&changes](tref el) {
		if (const auto it = changes.find(el); it != changes.end())
			return it->second;
		else return el;
	};
	auto neg_query = [&query](tref el) {
		return !query(el);
	};
	return pre_order<node_t>(n).apply_unique_until_change(r, neg_query);
}

// TODO (LOW) consider adding a similar functino for replace_node...


// find the first node that satisfy a predicate and return it.
template <typename node_t, typename predicate_t>
tref find_bottom(tref input, predicate_t& query) {
	tref found;
	auto find_top = [&query, &found](const auto& n){
		if (found != nullptr && query(n)) found = n;
		return found != nullptr;
	};
	post_order<node_t>(input).search_unique(find_top);
	return found;
}

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
pattern_matcher<node_t, is_capture_t>::pattern_matcher(tref pattern,
	rewriter::environment& env, const is_capture_t& is_capture)
	: pattern(pattern), env(env), is_capture(is_capture) {}

template <typename node_t, typename is_capture_t>
bool pattern_matcher<node_t, is_capture_t>::operator()(tref n) {
	// if we have matched the pattern, we never try again to unify
	if (matched) return false;
	// we clear previous environment attempts
	env.clear();
	// then we try to match the pattern against the node and if the match
	// was successful, we save the node that matched.
	if (match(pattern, n)) matched = n;
	else env.clear();
	// we continue visiting until we found a match.
	return matched != nullptr;
}

template <typename node_t, typename is_capture_t>
bool pattern_matcher<node_t, is_capture_t>::match(tref p, tref n) {
	// if we already have captured a node associated to the current capture
	// we check if it is the same as the current node, if it is not, we
	// return false...
	if (is_capture(p))
		if (auto it = env.find(p);
			it != env.end() && it->second != n) return false;
		// ...otherwise we save the current node as the one associated to the
		// current capture and return true.
		else return env.emplace(p, n), true;
	// otherwise, we check the symbol of the current node and if it is the
	// same as the one of the current pattern, we check if the children
	// match recursively.
	else {
		const auto& pt = lcrs_tree<node_t>::get(p);
		const auto& nt = lcrs_tree<node_t>::get(n);
		if (pt.value == nt.value) {
			auto n_it = nt.children().begin();
			for (tref ptc : pt.children()) {
				if (n_it == nt.children().end()) return false;
				if (ptc == *n_it) { ++n_it; continue; }
				else if (match(ptc, *n_it)) { ++n_it; continue; }
				else return false;
			}
			return true;
		}
	}
	return false;
}

template <typename node_t, typename is_capture_t>
pattern_matcher2<node_t, is_capture_t>::pattern_matcher2(
	const rewriter::rule& r, const is_capture_t& is_capture)
	: r(r), is_capture(is_capture) {}

template <typename node_t, typename is_capture_t>
bool pattern_matcher2<node_t, is_capture_t>::operator()(tref n) {
	const auto it = changes.find(n);
	if (it != changes.end()) return true;
	// we clear previous environment attempts
	env.clear();
	// then we try to match the pattern against the node and if the match
	// was successful, we save the node that matched.
	auto [pattern, body] = r;
	if (match(pattern->get(), n)) {
		auto nn = replace<node_t>(body->get(), env);
		if (nn == nullptr) return false;
		changes[n] = nn;
		return true;
	}
	trefs ch;
	const auto& nt = lcrs_tree<node_t>::get(n);
	for (tref c : nt.children())
		if (auto it = changes.find(c); it != changes.end())
			ch.push_back(it->second);
		else ch.push_back(c);
	auto res = lcrs_tree<node_t>::get(nt.value, std::move(ch));
	if (res == nullptr) return false;
	return changes[n] = res, true;
}

template <typename node_t, typename is_capture_t>
tref pattern_matcher2<node_t, is_capture_t>::replace_root(tref n) {
	auto it = changes.find(n);
	return it != changes.end() ? it->second : n;
}

template <typename node_t, typename is_capture_t>
bool pattern_matcher2<node_t, is_capture_t>::match(tref p, tref n) {
	// if we already have captured a node associated to the current capture
	// we check if it is the same as the current node, if it is not, we
	// return false...
	if (is_capture(p)) {
		if (auto it = env.find(p);
			it != env.end() && it->second != n) return false;
		// ...otherwise we save the current node as the one associated to the
		// current capture and return true.
		else return env.emplace(p, n), true;
	}
	// otherwise, we check the symbol of the current node and if it is the
	// same as the one of the current pattern, we check if the children
	// match recursively.
	const auto& pt = lcrs_tree<node_t>::get(p);
	const auto& nt = lcrs_tree<node_t>::get(changes.contains(n)
						? changes[n] : n);
	if (pt.value == nt.value) {
		auto n_it = nt.children().begin();
		for (tref ptc : pt.children()) {
			if (n_it == nt.children().end()) return false;
			if (ptc == *n_it) { ++n_it; continue; }
			else if (match(ptc, *n_it)) { ++n_it; continue; }
			else return false;
		}
		return true;
	}
	return false;
}


// this predicate matches when there exists a environment that makes the
// pattern match the node ignoring the nodes detected as skippable.
//
// TODO (LOW) create an env in operator() and pass it as a parameter to match, if
// a  match occurs, copy the data from the temp env to the env passed as
// parameter.

template <typename node_t, typename is_capture_t, typename predicate_t>
pattern_matcher_if<node_t, is_capture_t, predicate_t>::pattern_matcher_if(
	tref pattern, rewriter::environment& env, is_capture_t& is_capture,
	predicate_t& predicate) : pattern(pattern), env(env),
		is_capture(is_capture), predicate(predicate) {}

template <typename node_t, typename is_capture_t, typename predicate_t>
bool pattern_matcher_if<node_t, is_capture_t, predicate_t>::operator()(tref n) {
	// if we have matched the pattern, we never try again to unify
	if (matched) return false;
	// we clear previous environment attempts
	env.clear();
	// then we try to match the pattern against the node and if the match
	// was successful, we save the node that matched.
	if (match(pattern, n) && predicate(n)) matched = n;
	else env.clear();
	// we continue visiting until we found a match.
	return matched != nullptr;
}

template <typename node_t, typename is_capture_t, typename predicate_t>
bool pattern_matcher_if<node_t, is_capture_t, predicate_t>::match(tref p, tref n) {
	const auto& pt = lcrs_tree<node_t>::get(p);
	const auto& nt = lcrs_tree<node_t>::get(n);
	// if we already have captured a node associated to the current capture
	// we check if it is the same as the current node, if it is not, we
	// return false...
	if (is_capture(p))
		if (auto it = env.find(p); it != env.end()
			&& it->second != n) return false;
		// ...otherwise we save the current node as the one associated to the
		// current capture and return true.
		else return env.emplace(p, n), true;
	// otherwise, we check the symbol of the current node and if it is the
	// same as the one of the current pattern, we check if the children
	// match recursively.
	else if (pt.value != nt.value) return false;
	auto p_it = pt.children().begin();
	auto n_it = nt.children().begin();
	while (p_it != pt.children().end() && n_it != nt.children().end()) {
		if (*p_it == *n_it) { ++p_it; ++n_it; continue; }
		if (match(*p_it, *n_it)) { ++p_it; ++n_it; continue; }
		return false;
	}
	return true;
}

// apply a rule to a tree using the predicate to pattern_matcher.
template <typename node_t, typename is_capture_t>
tref apply_rule(const rewriter::rule& r, tref n, const is_capture_t& c) {
	pattern_matcher2<node_t, is_capture_t> matcher{ r, c };
	post_order<node_t>(n).search_unique(matcher);
	auto nn = matcher.replace_root(n);
#ifdef LOG_REWRITING
	if constexpr (LOG_REWRITING) {
		if (nn != n) {
			auto [p , s] = r;
			LOG_DEBUG << "(R) " << p << " := " << s << LOG_END;
			LOG_DEBUG << "(F) " << nn << LOG_END;
		}
	}
#endif // LOG_REWRITING
	return nn;
}

// apply a rule to a tree using the predicate to pattern_matcher and skipping
// unnecessary subtrees
template <typename node_t, typename is_capture_t, typename predicate_t>
tref apply_if(const rewriter::rule& r, tref n, is_capture_t& c,
	predicate_t& predicate)
{
	auto [p , s] = r;
	rewriter::environment u;
	pattern_matcher_if<node_t, is_capture_t, predicate_t>
						matcher{ p, u, c, predicate };
	auto nn = apply(s, n, matcher);
#ifdef LOG_REWRITING
	if constexpr (LOG_REWRITING) {
		if (nn != n) {
			LOG_DEBUG << "(R) " << p << " := " << s << LOG_END;
			LOG_DEBUG << "(F) " << nn << LOG_END;
		}
	}
#endif // LOG_REWRITING
	return nn;
}

// apply a substitution to a rule according to a given matcher, this method is
// use internaly by apply and apply with skip.
template <typename node_t, typename matcher_t>
tref apply(tref s, tref n, matcher_t& matcher) {
	post_order_query_traverser<node_t, decltype(idni::identity), matcher_t>(
		idni::identity, matcher)(n);
	if (matcher.matched) {
		auto nn = replace<node_t>(s, matcher.env);
		rewriter::environment nenv { {matcher.matched, nn} };
		return replace<node_t>(n, nenv);
	}
	return n;
}

} // rewriter namespace

template <typename T>
template <typename predicate_t>
tref lcrs_tree<T>::trim_top(predicate_t& query) {
	return rewriter::trim_top<T>(get(), query);
}

template <typename T>
template <typename predicate_t>
trefs lcrs_tree<T>::select_top(predicate_t& query) {
	return rewriter::select_top<T>(get(), query);
}
template <typename T>
template <typename predicate_t, typename extractor_t>
trefs lcrs_tree<T>::select_subnodes(predicate_t& query, extractor_t extractor) {
	return rewriter::select_subnodes<T>(get(), query, extractor);
}

template <typename T>
template <typename predicate_t>
trefs lcrs_tree<T>::select_all(predicate_t& query) {
	return rewriter::select_all<T>(get(), query);
}

template <typename T>
trefs lcrs_tree<T>::select_all_until(const auto& query, const auto& until) {
	return rewriter::select_all_until<T>(get(), query, until);
}

template <typename T>
trefs lcrs_tree<T>::select_top_until(const auto& query, const auto& until) {
	return rewriter::select_top_until<T>(get(), query, until);
}

template <typename T>
template <typename predicate_t>
tref lcrs_tree<T>::find_top(predicate_t& query) {
	return rewriter::find_top<T>(get(), query);
}
template <typename T>
tref lcrs_tree<T>::find_top_until(const auto& query, const auto& until) {
	return rewriter::find_top_until<T>(get(), query, until);
}

template <typename T>
tref lcrs_tree<T>::replace(const std::map<tref, tref>& changes) {
	return rewriter::replace<T>(get(), changes);
}
template <typename T>
tref lcrs_tree<T>::replace(tref replace, tref with) {
	return rewriter::replace<T>(get(), replace, with);
}

template <typename T>
template <typename predicate_t>
tref lcrs_tree<T>::replace_if(const std::map<tref, tref>& changes, predicate_t& query) {
	return rewriter::replace_if<T>(get(), changes, query);
}

template <typename T>
template <typename predicate_t>
tref lcrs_tree<T>::replace_until(const std::map<tref, tref>& changes, predicate_t& query) {
	return rewriter::replace_until<T>(get(), changes, query);
}

template <typename T>
template <typename predicate_t>
tref lcrs_tree<T>::find_bottom(predicate_t& query) {
	return rewriter::find_bottom<T>(get(), query);
}

template <typename T>
template <typename is_capture_t>
tref lcrs_tree<T>::apply_rule(const rewriter::rule& r, const is_capture_t& c) {
	return rewriter::apply_rule<T>(r, get(), c);
}

template <typename T>
template <typename is_capture_t, typename predicate_t>
tref lcrs_tree<T>::apply_if(const rewriter::rule& r, const is_capture_t& c,
	predicate_t& predicate)
{
	return rewriter::apply_if<T>(r, get(), c, predicate);
}

template <typename T>
template <typename matcher_t>
tref lcrs_tree<T>::apply(tref s, matcher_t& matcher) {
	return rewriter::apply<T>(s, get(), matcher);
}

} // idni::rewriter namespace
