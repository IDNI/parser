// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.txt

#include "tree.h"

namespace idni {

// former rewriter predicate API

namespace rewriter {

inline rules merge(const rules& rs1, const rules& rs2) {
	rules nrs;
	nrs.insert(nrs.end(), rs1.begin(), rs1.end());
	nrs.insert(nrs.end(), rs2.begin(), rs2.end());
	return nrs;
}

// ---------------------------------------------------------------------
// predicates

template <typename predicate_t>
select_top_predicate<predicate_t>::select_top_predicate(predicate_t& query,
	trefs& selected) : query(query), selected(selected) {}

template <typename predicate_t>
bool select_top_predicate<predicate_t>::operator()(tref n) {
	DBG(assert(n != nullptr);)
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
	DBG(assert(n != nullptr);)
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
	DBG(assert(n != nullptr);)
	if (query(n)) selected.push_back(n);
	// we always return true to visit all the nodes.
	return true;
}

template <typename predicate_t>
find_top_predicate<predicate_t>::find_top_predicate(predicate_t& query, tref& found)
	: query(query), found(found) {}

template <typename predicate_t>	
bool find_top_predicate<predicate_t>::operator()(tref n) {
	DBG(assert(n != nullptr);)
	if (found == nullptr && query(n)) found = n;
	return found == nullptr;
}

// -----------------------------------------------------------------------------
// traversers

template <typename node, typename wrapped_t, typename predicate_t>
post_order_traverser<node, wrapped_t, predicate_t>
	::post_order_traverser(wrapped_t& wrapped, predicate_t& query)
	: wrapped(wrapped), query(query) {}

template <typename node, typename wrapped_t, typename predicate_t>
tref post_order_traverser<node, wrapped_t, predicate_t>::operator()(tref n) {
	// we kept track of the visited nodes to avoid visiting the same node
	// twice. However, we do not need to keep track of the root node, since
	// it is the one we start from and we will always be visited.
	std::set<tref> visited;
	// if the root node matches the query predicate, we traverse it, otherwise
		// we return the result of apply the wrapped transform to the node.
	return query(n) ? traverse(n, visited) : wrapped(n);
}

template <typename node, typename wrapped_t, typename predicate_t>
tref post_order_traverser<node, wrapped_t, predicate_t>::traverse(
	tref n, std::set<tref>& visited)
{
	DBG(assert(n != nullptr);)
	// we traverse the children of the node in post-order, i.e. we visit
	// the children first and then the node itself.
	for (tref c : lcrs_tree<node>::get(n).children())
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
template <typename node, typename wrapped_t, typename predicate_t>
post_order_query_traverser<node, wrapped_t, predicate_t>::
	post_order_query_traverser(wrapped_t& wrapped, predicate_t& query)
		: wrapped(wrapped), query(query) {}

template <typename node, typename wrapped_t, typename predicate_t>
tref post_order_query_traverser<node, wrapped_t, predicate_t>::
	operator()(tref n)
{
	DBG(assert(n != nullptr);)
	// we kept track of the visited nodes to avoid visiting the same node
	// twice. However, we do not need to keep track of the root node, since
	// it is the one we start from and we will always be visited.
	std::set<tref> visited;
	// if the root node matches the query predicate, we traverse it, otherwise
	// we return the result of apply the wrapped transform to the node.
	return traverse(n, visited);
}

template <typename node, typename wrapped_t, typename predicate_t>
tref post_order_query_traverser<node, wrapped_t, predicate_t>::traverse(
	tref n, std::set<tref>& visited)
{
	DBG(assert(n != nullptr);)
	// we traverse the children of the node in post-order, i.e. we visit
	// the children first and then the node itself.
	if (found) return found;
	for (tref c : lcrs_tree<node>::get(n).children())
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
template <typename node, typename wrapped_t, typename predicate_t>
post_order_tree_traverser<node, wrapped_t, predicate_t>::
	post_order_tree_traverser(wrapped_t& wrapped, predicate_t& query) :
		wrapped(wrapped), query(query) {}

template <typename node, typename wrapped_t, typename predicate_t>
tref post_order_tree_traverser<node, wrapped_t, predicate_t>::
	operator()(tref n)
{
	DBG(assert(n != nullptr);)
	// if the root node matches the query predicate, we traverse it, otherwise
	// we return the result of apply the wrapped transform to the node.
	return query(n) ? traverse(n) : wrapped(n);
}

template <typename node, typename wrapped_t, typename predicate_t>
tref post_order_tree_traverser<node, wrapped_t, predicate_t>::traverse(tref n)
{
	DBG(assert(n != nullptr);)
	// we traverse the children of the node in post-order, i.e. we visit
	// the children first and then the node itself.
	for (tref c : lcrs_tree<node>::get(n).children())
		// we skip already visited nodes and nodes that do not match the
		// query predicate if it is present.
		if (query(c)) if (traverse(c) == nullptr)
			return nullptr;
	// finally we apply the wrapped visitor to the node if it is present.
	return wrapped(n);
}

// Do not use. Use post_order or pre_order instead depending on the needs.
template <typename node>
auto post_order_recursive_traverser<node>::operator()(
	tref n, auto& query, auto& wrapped)
{
	DBG(assert(n != nullptr);)
	if (query(n)) return traverse(n, query, wrapped);
	else return wrapped(n, lcrs_tree<node>::get(n).get_children());
}

template <typename node>
tref post_order_recursive_traverser<node>::traverse(
	tref n, auto& query, auto& wrapped)
{
	DBG(assert(n != nullptr);)
	trefs children;
	for (tref c : lcrs_tree<node>::get(n).children()) {
		if (query(c))
			children.push_back(traverse(c, query, wrapped));
		else children.push_back(
			wrapped(c, lcrs_tree<node>::get(n).get_children()));
	}
	return wrapped(n, children);
}

// visitor that produces nodes transformed accordingly to the
// given transformer. It only works with post order traversals.

template <typename node, typename wrapped_t>
map_transformer<node, wrapped_t>::map_transformer(wrapped_t& wrapped) : wrapped(wrapped) {}

template <typename node, typename wrapped_t>
tref map_transformer<node, wrapped_t>::operator()(tref& n) {
	DBG(assert(n != nullptr);)
	trefs ch;
	const auto& nt = lcrs_tree<node>::get(n);
	for (tref c : nt.children())
		if (auto it = changes.find(c); it != changes.end())
			ch.push_back(it->second);
	return changes[n] = lcrs_tree<node>::get(wrapped(nt.value), ch);
}

// visitor that produces nodes transformed accordingly to the
// given transformer. It only works with post order traversals.
// REVIEW (MEDIUM) check the implementation of this transformer, it seems buggy
template <typename node, typename wrapped_t>
map_node_transformer<node, wrapped_t>::map_node_transformer(
	wrapped_t& wrapped) : wrapped(wrapped) {}

template <typename node, typename wrapped_t>
tref map_node_transformer<node, wrapped_t>::operator()(tref n) {
	DBG(assert(n != nullptr);)
	auto nn = wrapped(n);
	if (nn != n) { changes[n] == nn; return nn; };
	trefs ch;
	const auto& nt = lcrs_tree<node>::get(n);
	for (tref c : nt.children())
		if (auto it = changes.find(c); it != changes.end())
			ch.push_back(it->second);
		else ch.push_back(c);
	return changes[n] = lcrs_tree<node>::get(nt.value, ch);
}

// visitor that produces nodes transformed accordingly to the
// given transformer. It only works with post order traversals.
//
// TODO (MEDIUM) merge replace and replace_node transformers into one.
template <typename node, typename wrapped_t>
replace_node_transformer<node, wrapped_t>::replace_node_transformer(
	wrapped_t& wrapped) : wrapped(wrapped) {}

template <typename node, typename wrapped_t>
tref replace_node_transformer<node, wrapped_t>::operator()(tref n) {
	DBG(assert(n != nullptr);)
	trefs ch;
	const auto& nt = lcrs_tree<node>::get(n);
	for (tref c : nt.children())
		if (auto it = changes.find(c); it != changes.end())
			ch.push_back(it->second);
	return changes[n] = lcrs_tree<node>::get(wrapped(nt.value), ch);
}

// visitor that produces nodes transformed accordingly to the
// given transformer. It only works with post order traversals.
template <typename node>
replace_transformer<node>::replace_transformer(
	subtree_map<node, tref>& changes) : changes(changes) {}

template <typename node>
tref replace_transformer<node>::operator()(tref n) {
	DBG(assert(n != nullptr);)
	// std::cout << "----------------------------------\n";
	// std::cout << "replace_transformer: " << lcrs_tree<node>::get(n).value << "\n";
	tref nn = get_cached<node>(n, changes);
	// std::cout << "was changed and already replaced to: " << lcrs_tree<node>::get(nn).value << "\n";
	// std::cout << "nn: " << nn << " n: " << n << "\n";
	if (nn != n) return nn;
	// std::cout << "replacing\n";
	return replace(n);
}

template <typename node>
tref replace_transformer<node>::replace(tref n) {
	DBG(assert(n != nullptr);)
	trefs ch;
	const auto& nt = lcrs_tree<node>::get(n);
	for (tref c : nt.children())
		ch.push_back(get_cached<node>(c, changes));
	auto nn = lcrs_tree<node>::get(nt.value, ch);
	// std::cout << "nt: " << nt.value << " ";
	// std::cout << "replaced to: " << lcrs_tree<node>::get(nn).value << "\n";
	return changes[n] = nn;
}

// delete all top nodes that satisfy a predicate.
template <typename node, typename predicate_t>
tref trim_top(tref input, predicate_t& query) {
	DBG(assert(input != nullptr);)
	// std::cout << "trim_top: " << lcrs_tree<node>::get(input).value << " " << input << "\n";
	const auto t = [&query](tref n) {
		// std::cout << "trim_top: " << lcrs_tree<node>::get(n).value << " " << n << "\n";
		trefs ch; size_t l = 0;
		const auto& nt = lcrs_tree<node>::get(n);
		for (tref c : nt.children()) {
			l++;
			if (!query(c))
				// std::cout << "storing child: " << lcrs_tree<node>::get(c).value << " " << c << "\n",
				ch.push_back(c);
		}
		if (l == ch.size()) return
			// std::cout << "not trimmed\n",
			n;
		return
			// std::cout << "trimmed\n",
			lcrs_tree<node>::get(nt.value, ch);
	};
	return pre_order<node>(input).apply_unique_until_change(t);
}


// select all top nodes that satisfy a predicate and return them.
template <typename node, typename predicate_t>
trefs select_top(tref input, predicate_t& query) {
	DBG(assert(input != nullptr);)
	trefs selected;
	auto select = [&query, &selected](const auto& n) {
		using tree = lcrs_tree<node>;
		if (!query(n)) return true;
		// we return false to avoid visiting the children of the node
		// since we are only interested in the top nodes.
		if (std::ranges::find_if(selected, [&n](const auto& el)
			{return tree::get(el) == tree::get(n);})
				== selected.end()) selected.push_back(n);
		return false;
	};
	pre_order<node>(input).visit_unique(select);
	return selected;
}


// select all subnodes that satisfy a predicate according to the extractor and return them.
template <typename node, typename predicate_t, typename extractor_t>
trefs select_subnodes(tref input, predicate_t& query, extractor_t extractor) {
	DBG(assert(input != nullptr);)
	trefs selected;
	rewriter::select_subnodes_predicate<predicate_t, extractor_t> select(
		query, extractor, selected);
	pre_order<node>(input).visit_unique(select);
	return selected;
}

// select all nodes that satisfy a predicate and return them.
template <typename node, typename predicate_t>
trefs select_all(tref input, predicate_t& query) {
	DBG(assert(input != nullptr);)
	trefs selected;
	auto select = [&query, &selected](const auto& n) {
		if (query(n)) selected.push_back(n);
		// we always return true to visit all the nodes.
		return true;
	};
	pre_order<node>(input).visit_unique(select);
	return selected;
}


// Select all nodes in input that satisfy query ignoring subtrees under a node satisfying until
template <typename node>
trefs select_all_until(tref input, const auto& query, const auto& until) {
	DBG(assert(input != nullptr);)
	trefs selected;
	auto select = [&](tref n) {
		if (query(n)) selected.push_back(n);
		// we always return true until a node satisfies until.
		if (until(n)) return false;
		else return true;
	};
	pre_order<node>(input).visit_unique(select);
	return selected;
}


// Select top nodes in input that satisfy query ignoring subtrees under a node satisfying until
template <typename node>
trefs select_top_until(tref input, const auto& query, const auto& until) {
	DBG(assert(input != nullptr);)
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
	pre_order<node>(input).visit_unique(select);
	return selected;
}


// find the first node that satisfy a predicate and return it.
template <typename node, typename predicate_t>
tref find_top(tref input, predicate_t& query) {
	DBG(assert(input != nullptr);)
	tref found = nullptr;
	auto find_top = [&query, &found](const auto& n) {
		if (found == nullptr && query(n)) found = n;
		return found == nullptr;
	};
	pre_order<node>(input).search_unique(find_top);
	return found;
}

// Select single top node in input that satisfy query ignoring subtrees under a node satisfying until
template <typename node>
tref find_top_until(tref input, const auto& query, const auto& until) {
	DBG(assert(input != nullptr);)
	tref n = nullptr;
	auto select = [&](tref nn) {
		if (!query(nn)) return true;
		if (n != nullptr) return false;
		// Query is true and node has not been found, hence set node to n
		return n = nn, false;
	};
	const auto neg_until = [&until](const auto& p) {
		return !until(p);
	};
	pre_order<node>(input).search_unique(select, neg_until, identity);
	return n;
}

template <typename node>
tref replace(tref n, const std::map<tref, tref>& changes) {
	DBG(assert(n != nullptr);)
	const auto r = [&changes](tref el) {
		// lcrs_tree<node>::dump(std::cout << "el: ", el) << "\n";
		auto ret = get_cached<node>(el, changes);
		// lcrs_tree<node>::dump(std::cout << "\tret: ", ret) << "\n";
		return ret;
	};
	return pre_order<node>(n).apply_unique_until_change(r);
}

template <typename node>
tref replace(tref n, const subtree_map<node, tref>& changes) {
	DBG(assert(n != nullptr);)
	// #DEBUG_TREE_REWRITER
	#if DEBUG_TREE_REWRITER
		LOG_TRACE << "changes: " << dump_to_str<node>(changes);
	#endif // DEBUG_TREE_REWRITER
	const auto r = [&changes](tref el) {
		auto ret = get_cached<node>(el, changes);
		#if DEBUG_TREE_REWRITER
		if (el != ret) LOG_TRACE << "el: "
			<< lcrs_tree<node>::dump_to_str(el, false) << "\n"
			<< "\tret: "
			<< lcrs_tree<node>::dump_to_str(ret, false) LOG_END;
		#endif // DEBUG_TREE_REWRITER
		return ret;
	};
	return pre_order<node>(n).apply_unique_until_change(r);
}

template <typename node>
tref replace(tref n, tref replace, tref with) {
	DBG(assert(n != nullptr && replace != nullptr && with != nullptr);)
	const auto r = [&](tref el) {
		if (lcrs_tree<node>::subtree_equals(el, replace)) return with;
		else return el;
	};
	return pre_order<node>(n).apply_unique_until_change(r);
}

// Replace nodes in n according to changes while skipping subtrees that don't satisfy query
template <typename node, typename predicate_t>
tref replace_if(tref n, const subtree_map<node, tref>& changes, predicate_t& query) {
	DBG(assert(n != nullptr);)
	const auto r = [&changes](tref el) {
		return get_cached<node>(el, changes);
	};
	return pre_order<node>(n).apply_unique_until_change(r, query);
}

template<typename node, typename P>
tref replace_if(tref n, tref replace, tref with, P& query) {
	DBG(assert(n != nullptr && replace != nullptr && with != nullptr);)
	const auto r = [&](tref el) {
		if (lcrs_tree<node>::subtree_equals(el, replace)) return with;
		else return el;
	};
	return pre_order<node>(n).apply_unique_until_change(r, query);
}

// Replace nodes in n according to changes while skipping subtrees that satisfy query
template <typename node, typename predicate_t>
tref replace_until(tref n, const subtree_map<node, tref>& changes, predicate_t& query) {
	DBG(assert(n != nullptr);)
	const auto r = [&changes](tref el) {
		return get_cached<node>(el, changes);
	};
	auto neg_query = [&query](tref el) {
		return !query(el);
	};
	return pre_order<node>(n).apply_unique_until_change(r, neg_query);
}

// TODO (LOW) consider adding a similar functino for replace_node...


// find the first node that satisfy a predicate and return it.
template <typename node, typename predicate_t>
tref find_bottom(tref input, predicate_t& query) {
	DBG(assert(input != nullptr);)
	tref found = nullptr;
	auto find_top = [&query, &found](const auto& n){
		if (found == nullptr && query(n)) found = n;
		return found == nullptr;
	};
	post_order<node>(input).search_unique(find_top);
	return found;
}

// // TODO (MEDIUM) simplify matchers code and extract common code.

// // this predicate matches when there exists a environment that makes the
// // pattern match the node.
// //
// // TODO (LOW) create and env in operator() and pass it as a parameter to match, if
// // a  match occurs, copy the data from the temp env to the env passed as
// // parameter.
// //
// // It should allow to detects matches in the middle of a tree.
// template <typename node, typename is_capture_t>
// pattern_matcher<node, is_capture_t>::pattern_matcher(tref pattern,
// 	tree::structural_tref_map& env,
// 	const is_capture_t& is_capture)
// 	: pattern(pattern), env(env), is_capture(is_capture) {}

// template <typename node, typename is_capture_t>
// bool pattern_matcher<node, is_capture_t>::operator()(tref n) {
// 	// if we have matched the pattern, we never try again to unify
// 	if (matched) return false;
// 	// we clear previous environment attempts
// 	env.clear();
// 	// then we try to match the pattern against the node and if the match
// 	// was successful, we save the node that matched.
// 	if (match(pattern, n)) matched = n;
// 	else env.clear();
// 	// we continue visiting until we found a match.
// 	return matched != nullptr;
// }

// template <typename node, typename is_capture_t>
// bool pattern_matcher<node, is_capture_t>::match(tref p, tref n) {
// 	// if we already have captured a node associated to the current capture
// 	// we check if it is the same as the current node, if it is not, we
// 	// return false...
// 	if (is_capture(p)) {
// 		auto nn = get_cached<node>(n, env);
// 		if (nn == nullptr) return false;
// 		if (tree::subtree_equals(p, nn)) return true;
// 		// ...otherwise we save the current node as the one associated to the
// 		// current capture and return true.
// 		else return changes.emplace(p, n), true;
// 	}
// 	// otherwise, we check the symbol of the current node and if it is the
// 	// same as the one of the current pattern, we check if the children
// 	// match recursively.
// 	else {
// 		const auto& pt = tree::get(p);
// 		const auto& nt = tree::get(n);
// 		if (pt.value == nt.value) {
// 			auto n_it = nt.children().begin();
// 			for (tref ptc : pt.children()) {
// 				if (n_it == nt.children().end()) return false;
// 				if (tree::subtree_equals(ptc, *n_it)) {
// 						++n_it; continue; }
// 				else if (match(ptc, *n_it)) { ++n_it; continue; }
// 				else return false;
// 			}
// 			return true;
// 		}
// 	}
// 	return false;
// }

template <typename node, typename is_capture_t>
pattern_matcher2<node, is_capture_t>::pattern_matcher2(
	const rewriter::rule& r, const is_capture_t& is_capture)
	: r(r), is_capture(is_capture) {}

template <typename node, typename is_capture_t>
bool pattern_matcher2<node, is_capture_t>::operator()(tref n) {
	DBG(assert(n != nullptr);)
	const auto it = changes.find(n);
	if (it != changes.end()) return true;
	// we clear previous environment attempts
	env.clear();
	// then we try to match the pattern against the node and if the match
	// was successful, we save the node that matched.
	auto [pattern, body] = r;
	if (match(pattern->get(), n)) {
		// tree::get(body->get()).dump(std::cout << "body: ", true) << "\n";
		// dump<node>(std::cout << "env: ", env);
		auto nn = replace<node>(body->get(), env);
		// tree::get(nn).dump(std::cout << "nn: ", true) << "\n";
		if (nn == nullptr) return false;
		// tree::get(n).dump(std::cout << "pattern_matcher2: we got a match ") << "\n";
		// tree::get(nn).dump(std::cout << "pattern_matcher2: replaced with ") << "\n";
		changes[n] = nn;
		// tree::get(changes[n]).dump(std::cout << "changes: ", true) << "\n";
		return true;
	}
	trefs ch;
	const auto& nt = lcrs_tree<node>::get(n);
	for (tref c : nt.children())
		ch.push_back(get_cached<node>(c, changes));
	auto res = lcrs_tree<node>::get(nt.value, ch);
	if (res == nullptr) return false;
	return changes[n] = res, true;
}

template <typename node, typename is_capture_t>
tref pattern_matcher2<node, is_capture_t>::replace_root(tref n) {
	DBG(assert(n != nullptr);)
	auto x = get_cached<node>(n, changes);
	// std::cout << "replace_root: ";
	// lcrs_tree<node>::get(n).dump();
	// lcrs_tree<node>::get(x).dump();
	// std::cout << "\n";
	return x;
}

template <typename node, typename is_capture_t>
bool pattern_matcher2<node, is_capture_t>::match(tref p, tref n) {
	DBG(assert(p != nullptr && n != nullptr);)
	// if we already have captured a node associated to the current capture
	// we check if it is the same as the current node, if it is not, we
	// return false...
	// tree::get(p).dump(std::cout << "match: ");
	// tree::get(n).dump(std::cout << "\nto:    ");
	// std::cout << "\n";
	if (is_capture(p)) {
		if (auto it = env.find(p); it != env.end()
			&& !tree::subtree_equals(it->second, n)) return false;
		// ...otherwise we save the current node as the one associated to the
		// current capture and return true.
		else return env.emplace(p, n), true;
	}
	// otherwise, we check the symbol of the current node and if it is the
	// same as the one of the current pattern, we check if the children
	// match recursively.
	const auto& pt = tree::get(p);
	const auto& nt = tree::get(get_cached<node>(n, changes));
	if (pt.value == nt.value) {
		auto n_it = nt.children().begin();
		for (tref ptc : pt.children()) {
			if (n_it == nt.children().end()
				|| (!tree::subtree_equals(ptc, *n_it)
					&& !match(ptc, *n_it))) return false;
			++n_it;
		}
		if (n_it != nt.children().end()) return false;
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

template <typename node, typename is_capture_t, typename predicate_t>
pattern_matcher_if<node, is_capture_t, predicate_t>::pattern_matcher_if(
	tref pattern, subtree_map<node, tref>& env,
	is_capture_t& is_capture, predicate_t& predicate)
	: pattern(pattern), env(env), is_capture(is_capture),
		predicate(predicate) {}

template <typename node, typename is_capture_t, typename predicate_t>
bool pattern_matcher_if<node, is_capture_t, predicate_t>::operator()(tref n) {
	DBG(assert(n != nullptr);)
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

template <typename node, typename is_capture_t, typename predicate_t>
bool pattern_matcher_if<node, is_capture_t, predicate_t>::match(tref p, tref n) {
	DBG(assert(p != nullptr && n != nullptr);)
	const auto& pt = lcrs_tree<node>::get(p);
	const auto& nt = lcrs_tree<node>::get(n);
	// if we already have captured a node associated to the current capture
	// we check if it is the same as the current node, if it is not, we
	// return false...
	if (is_capture(p))
		if (auto it = env.find(p); it != env.end()
			&& lcrs_tree<node>::get(it->second) != nt)
				return false;
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
		if (lcrs_tree<node>::subtree_equals(*p_it, *n_it)) {
				++p_it; ++n_it; continue; }
		if (match(*p_it, *n_it)) { ++p_it; ++n_it; continue; }
		return false;
	}
	return true;
}

// apply a rule to a tree using the predicate to pattern_matcher.
template <typename node, typename is_capture_t>
tref apply_rule(const rewriter::rule& r, tref n, const is_capture_t& c) {
	DBG(assert(n != nullptr);)
	pattern_matcher2<node, is_capture_t> matcher{ r, c };
	post_order<node>(n).search_unique(matcher);
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
template <typename node, typename is_capture_t, typename predicate_t>
tref apply_if(const rewriter::rule& r, tref n, is_capture_t& c,
	predicate_t& predicate)
{
	DBG(assert(n != nullptr);)
	auto [p , s] = r;
	rewriter::environment<node> u;
	pattern_matcher_if<node, is_capture_t, predicate_t>
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
template <typename node, typename matcher_t>
tref apply(tref s, tref n, matcher_t& matcher) {
	DBG(assert(s != nullptr && n != nullptr);)
	post_order_query_traverser<node, decltype(idni::identity), matcher_t>(
		idni::identity, matcher)(n);
	if (matcher.matched) {
		auto nn = replace<node>(s, matcher.env);
		rewriter::environment<node> nenv { {matcher.matched, nn} };
		return replace<node>(n, nenv);
	}
	return n;
}

} // rewriter namespace

template <typename node>
tref lcrs_tree<node>::trim_top(const auto& query) const {
	return rewriter::trim_top<node, decltype(query)>(get(), query);
}

template <typename node>
trefs lcrs_tree<node>::select_top(const auto& query) const {
	return rewriter::select_top<node, decltype(query)>(get(), query);
}
template <typename node>
trefs lcrs_tree<node>::select_subnodes(const auto& query, const auto& extractor)
	const
{
	return rewriter::select_subnodes<node,
		decltype(query), decltype(extractor)>(get(), query, extractor);
}

template <typename node>
trefs lcrs_tree<node>::select_all(const auto& query) const {
	return rewriter::select_all<node, decltype(query)>(get(), query);
}

template <typename node>
trefs lcrs_tree<node>::select_all_until(const auto& query, const auto& until)
	const
{
	return rewriter::select_all_until<node>(get(), query, until);
}

template <typename node>
trefs lcrs_tree<node>::select_top_until(const auto& query, const auto& until)
	const
{
	return rewriter::select_top_until<node>(get(), query, until);
}

template <typename node>
tref lcrs_tree<node>::find_top(const auto& query) const {
	return rewriter::find_top<node, decltype(query)>(get(), query);
}
template <typename node>
tref lcrs_tree<node>::find_top_until(const auto& query, const auto& until) const
{
	return rewriter::find_top_until<node>(get(), query, until);
}

template <typename node>
tref lcrs_tree<node>::find_bottom(const auto& query) const {
	return rewriter::find_bottom<node, decltype(query)>(get(), query);
}

template <typename node>
tref lcrs_tree<node>::replace(const subtree_map<node, tref>& changes) const {
	return rewriter::replace<node>(get(), changes);
}
template <typename node>
tref lcrs_tree<node>::replace(tref replace, tref with) const {
	DBG(assert(replace != nullptr && with != nullptr);)
	return rewriter::replace<node>(get(), replace, with);
}

template <typename node>
tref lcrs_tree<node>::replace_if(const subtree_map<node, tref>& changes,
	const auto& query) const
{
	return rewriter::replace_if<node, decltype(query)>(get(), changes, query);
}

template <typename node>
tref lcrs_tree<node>::replace_until(const subtree_map<node, tref>& changes,
	const auto& query) const
{
	return rewriter::replace_until<node, decltype(query)>(get(), changes,
									query);
}

template <typename node>
tref lcrs_tree<node>::apply_rule(const rewriter::rule& r, const auto& is_capture)
	const
{
	return rewriter::apply_rule<node, decltype(is_capture)>(r, get(),
								is_capture);
}

template <typename node>
tref lcrs_tree<node>::apply_if(const rewriter::rule& r,
				const auto& is_capture, const auto& query) const
{
	return rewriter::apply_if<node, decltype(is_capture), decltype(query)>(
						r, get(), is_capture, query);
}

template <typename node>
tref lcrs_tree<node>::apply(tref s, const auto& matcher) const {
	DBG(assert(s != nullptr);)
	return rewriter::apply<node, decltype(matcher)>(s, get(), matcher);
}

} // idni::rewriter namespace
