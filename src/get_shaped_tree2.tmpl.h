// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include "parser.h"

// TODO change all recursive transformations to post_order and pre_order traversals where applicable

namespace idni {

template <typename C, typename T>
tref parser<C, T>::result::get_trimmed_bintree(tref ref,
	const shaping_options& opts) const
{
	if (!ref) return nullptr;
	const auto& rt = tree::get(ref);
	if (rt.is_null() || trimmable_node<C, T>(rt.value, opts)) return nullptr;
	auto dropped = [&opts](tref parent, tref c) {
		const auto& ct = tree::get(c);
		return ct.is_null() || trimmable_node<C, T>(ct.value, opts)
			|| trimmable_child<C, T>(tree::get(parent).value,
				ct.value, opts);
	};
	// A child that the trim drops is not worth a visit. The root has no
	// parent and is checked above.
	auto visit = [&dropped](tref c, tref parent = nullptr) {
		return !parent || !dropped(parent, c);
	};
	auto trim = [&dropped](tref n) {
		const auto& t = tree::get(n);
		if (t.is_t()) return n; // terminal leaf node
		trefs ch;
		bool changed = false;
		for (tref c : t.children()) {
			if (dropped(n, c)) changed = true;
			else ch.push_back(c);
		}
		return changed ? tree::get(t.value, ch) : n;
	};
	return post_order<pnode>(ref).apply_unique(trim, visit);
}
template <typename C, typename T>
tref parser<C, T>::result::get_trimmed_bintree(tref n) const {
	return get_trimmed_bintree(n, shaping);
}

template <typename C, typename T>
tref parser<C, T>::result::inline_bintree_nodes(tref ref,
	const shaping_options& opts) const
{
	if (!ref) return nullptr;
	const lit<C, T> amb = amb_node;
	auto inliner = [&opts, &amb](tref n) {
		const auto& t = tree::get(n);
		// AMB protects children
		if (t.value.first.nt() && t.value.first == amb) return n;
		trefs ch;
		bool changed = false;
		for (tref c : t.children()) {
			const auto& ct = tree::get(c);
			if (node_to_inline<C, T>(ct.value, opts)) {
				for (auto cc : ct.children()) ch.push_back(cc);
				changed = true;
			} else ch.push_back(c);
		}
		if (!changed) return n;
		auto nn = tree::get(t.value, ch);
		return nn;
	};
	return post_order<pnode>(ref).apply_unique(inliner);
}
template <typename C, typename T>
tref parser<C, T>::result::inline_bintree_nodes(tref ref) const {
	return inline_bintree_nodes(ref, shaping);
}

template <typename C, typename T>
tref parser<C, T>::result::inline_bintree_paths(tref ref,
	const shaping_options& opts) const
{
	if (!ref || tree::get(ref).is_null()) return nullptr;
	// The end of the first tree path of to_inline that starts at n.
	auto path_end = [&opts](tref n) -> tref {
		for (auto& tp : opts.to_inline) {
			if (tp.size() < 2) continue;
			// a child goes above its right sibling, so the walk is pre order
			std::vector<std::pair<tref, size_t>> todo{ { n, 0 } };
			while (!todo.empty()) {
				auto [r, i] = todo.back();
				todo.pop_back();
				const auto& t = tree::get(r);
				if (i > 0) if (tref s = t.right_sibling(); s)
					todo.emplace_back(s, i);
				if (!t.is_nt() || !t.is(tp[i])) continue;
				if (i == tp.size() - 1) return r;
				if (tref c = t.first(); c) todo.emplace_back(c, i + 1);
			}
		}
		return nullptr;
	};
	// Pre order, so the children of a path end are inlined as well.
	auto inliner = [&path_end](tref n) {
		if (tree::get(n).is_t()) return tree::get(tree::get(n).value);
		while (tref hit = path_end(n)) n = hit;
		const auto& t = tree::get(n);
		trefs ch;
		bool changed = false;
		for (tref c : t.children())
			if (tree::get(c).is_null()) changed = true;
			else ch.push_back(c);
		return changed ? tree::get(t.value, ch) : n;
	};
	return pre_order<pnode>(ref).apply(inliner);
}
template <typename C, typename T>
tref parser<C, T>::result::inline_bintree_paths(tref ref) const {
	return inline_bintree_paths(ref, shaping);
}

template <typename C, typename T>
tref parser<C, T>::result::inline_bintree(tref ref,
	const shaping_options& opts) const
{
	ref = inline_bintree_nodes(ref, opts);
	// tree::get(ref).print(std::cout << "inlined tree nodes: ") << "\n\n";
	return inline_bintree_paths(ref, opts);
}
template <typename C, typename T>
tref parser<C, T>::result::inline_bintree(tref ref) const {
	return inline_bintree(ref, shaping);
}

template <typename C, typename T>
tref parser<C, T>::result::trim_bintree_child_terminals(tref ref,
	const shaping_options& opts) const
{
	if (!ref) return nullptr;
	const auto& t = tree::get(ref);
	if (t.is_t()) return ref;
	bool trim = (opts.trim_terminals
			&& opts.dont_trim_terminals_of.find(t.get_nt())
					== opts.dont_trim_terminals_of.end())
		|| opts.to_trim_children_terminals.find(t.get_nt())
				!= opts.to_trim_children_terminals.end();
	trefs ch;
	for (const auto& c : t.children()) {
		auto& ct = tree::get(c);
		if (ct.is_nt() || ct.is_null() || !trim) {
			auto x = trim_bintree_child_terminals(c, opts);
			if (x) ch.push_back(x);
		}
	}
	return tree::get(t.value, ch);
}
template <typename C, typename T>
tref parser<C, T>::result::trim_bintree_child_terminals(tref ref) const {
	return trim_bintree_child_terminals(ref, shaping);
}

template <typename C, typename T>
tref parser<C, T>::result::shape_tree2_impl(tref t,
	const shaping_options& opts)
{
	auto _ = diag_report.open_if(measure_scopes, label::shaped_tree);
	t = diag_report.step(measure_scopes, label::trim, [&] {
		return get_trimmed_bintree(t, opts);
	});
	t = diag_report.step(measure_scopes, label::inline_nodes, [&] {
		return inline_bintree_nodes(t, opts);
	});
	t = diag_report.step(measure_scopes, label::inline_paths, [&] {
		return inline_bintree_paths(t, opts);
	});
	t = diag_report.step(measure_scopes, label::trim_terminals, [&] {
		return trim_bintree_child_terminals(t, opts);
	});
	return t;
}

template <typename C, typename T>
tref parser<C, T>::result::get_shaped_bintree(tref t,
	const shaping_options& opts)
{
	return shape_tree2_impl(t, opts);
}
template <typename C, typename T>
tref parser<C, T>::result::get_shaped_bintree(tref t)
{
	return get_shaped_bintree(t, shaping);
}
template <typename C, typename T>
tref parser<C, T>::result::get_shaped_bintree(const shaping_options& opts) {
	return shape_tree2_impl(get_bintree(), opts);
}
template <typename C, typename T>
tref parser<C, T>::result::get_shaped_bintree() {
	return get_shaped_bintree(shaping);
}

template <typename C, typename T>
tref parser<C, T>::result::get_bintree() {
	if (froot != 0)
		return froot->get();
	else if (f)
		return get_bintree(f->root());
	return nullptr;
}

template <typename C, typename T>
tref parser<C, T>::result::get_bintree(const pnode& n) {
	auto same_pnode = [](const pnode& a, const pnode& b) {
		return a.first == b.first && a.second == b.second;
	};
	if (froot != 0) {
		tref root = froot->get();
		if (!root) return nullptr;
		tref hit = nullptr;
		auto search = [&](tref ref) {
			const auto& t = tree::get(ref);
			if (amb_node.nt() && t.value.first == amb_node) {
				for (tref c : t.children())
					if (same_pnode(tree::get(c).value, n)) {
						hit = ref;
						return false;
					}
			} else if (same_pnode(t.value, n)) {
				hit = ref;
				return false;
			}
			return true;
		};
		pre_order<pnode>(root).search_unique(search);
		return hit;
	}
	if (!f) return nullptr;
	htref t;
	auto _rb = diag_report
			.open_if(measure_scopes, label::reconstruct_bintree);
	typename pforest::graph fg;
	fg = diag_report.step(measure_scopes, label::extract_graph, [&] {
		return f->extract_first_graph(n);
	});
	diag_report.step(measure_scopes, label::inline_grammar, [&] {
		inline_grammar_transformations(fg);
	});
	t = diag_report.step(measure_scopes, label::extract_tree2, [&] {
		return fg.extract_tree2();
	});
	return t->get();
}

template <typename C, typename T>
tref parser<C, T>::result::get_tree2() {
	return get_bintree();
}

template <typename C, typename T>
tref parser<C, T>::result::get_tree2(const pnode& n) {
	return get_bintree(n);
}

template <typename C, typename T>
tref parser<C, T>::result::get_trimmed_tree2(tref ref,
	const shaping_options& opts) const
{
	return get_trimmed_bintree(ref, opts);
}
template <typename C, typename T>
tref parser<C, T>::result::get_trimmed_tree2(tref ref) const {
	return get_trimmed_bintree(ref);
}
template <typename C, typename T>
tref parser<C, T>::result::inline_tree_nodes2(tref t,
	const shaping_options& opts) const
{
	return inline_bintree_nodes(t, opts);
}
template <typename C, typename T>
tref parser<C, T>::result::inline_tree_nodes2(tref t) const {
	return inline_bintree_nodes(t);
}
template <typename C, typename T>
tref parser<C, T>::result::inline_tree_paths2(tref t,
	const shaping_options& opts) const
{
	return inline_bintree_paths(t, opts);
}
template <typename C, typename T>
tref parser<C, T>::result::inline_tree_paths2(tref t) const {
	return inline_bintree_paths(t);
}
template <typename C, typename T>
tref parser<C, T>::result::inline_tree2(tref t,
	const shaping_options& opts) const
{
	return inline_bintree(t, opts);
}
template <typename C, typename T>
tref parser<C, T>::result::inline_tree2(tref t) const {
	return inline_bintree(t);
}
template <typename C, typename T>
tref parser<C, T>::result::trim_children_terminals2(tref ref,
	const shaping_options& opts) const
{
	return trim_bintree_child_terminals(ref, opts);
}
template <typename C, typename T>
tref parser<C, T>::result::trim_children_terminals2(tref ref) const {
	return trim_bintree_child_terminals(ref);
}
template <typename C, typename T>
tref parser<C, T>::result::get_shaped_tree2(tref t,
	const shaping_options& opts)
{
	return get_shaped_bintree(t, opts);
}
template <typename C, typename T>
tref parser<C, T>::result::get_shaped_tree2(tref t) {
	return get_shaped_bintree(t);
}
template <typename C, typename T>
tref parser<C, T>::result::get_shaped_tree2(const shaping_options& opts) {
	return get_shaped_bintree(opts);
}
template <typename C, typename T>
tref parser<C, T>::result::get_shaped_tree2() {
	return get_shaped_bintree();
}

} // idni namespace
