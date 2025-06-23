#include <algorithm>
#include "parser.h"

// TODO change all recursive transformations to post_order and pre_order traversals where applicable

namespace idni {

template <typename C, typename T>
tref parser<C, T>::result::get_trimmed_tree2(tref ref,
	const shaping_options opts) const
{
	if (!ref) return nullptr;
	const auto& t = tree::get(ref);
	// std::cerr << "get_trimmed_tree for node: `" << t.value.first.to_std_string() << "`" << std::endl;
	if (t.is_null()) return nullptr;
	if (trimmable_node<C, T>(t.value, opts)) return nullptr;

	if (t.is_t()) return ref; // terminal leaf node 

	trefs ch;
	for (tref c : t.children()) {
		// std::cerr << "child: " << tree::get(c).value.first.to_std_string() << std::endl;
		if (trimmable_child<C, T>(t.value, tree::get(c).value, opts)) {
			// std::cerr << "\t is trimmable, skipping" << std::endl;
			continue;
		}
		if (auto x = get_trimmed_tree2(c, opts); x) ch.push_back(x);
	}
	return tree::get(t.value, ch);
}
template <typename C, typename T>
tref parser<C, T>::result::get_trimmed_tree2(tref n) const {
	return get_trimmed_tree2(n, shaping);
}

template <typename C, typename T>
tref parser<C, T>::result::inline_tree_nodes2(tref ref,
	const shaping_options opts) const
{
	if (!ref) return nullptr;
	auto inliner = [&opts](tref n) {
		const auto& t = tree::get(n);
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
tref parser<C, T>::result::inline_tree_nodes2(tref ref) const {
	return inline_tree_nodes2(ref, shaping);
}

template <typename C, typename T>
tref parser<C, T>::result::inline_tree_paths2(tref ref,
	const shaping_options opts) const
{
	if (!ref) return nullptr;
	const auto& t = tree::get(ref);
	if (t.is_null()) return nullptr;
	if (t.is_t()) return tree::get(t.value);
	//std::cerr << "inlining treepath for node: `" << t.value.first.to_std_string() << "`" << std::endl;
	for (auto& tp : opts.to_inline) {
		if (tp.size() < 2) continue;
		std::function<tref(tref, size_t)> go =
			[&](tref ref, size_t i) -> tref
		{
			const auto& t = tree::get(ref);
			if (!t.is_nt() || !t.is(tp[i])) return nullptr;
			//std::cerr << "treepath go " << i << ": `"
			//	<< t->value.first.to_std_string() << "`" << std::endl;
			if (i == tp.size() - 1) return ref;
			for (tref c : t.children())
				if (auto y = go(c, i + 1); y) return y;
			return nullptr;
		};
		if (auto p = go(ref, 0); p) return inline_tree_paths2(p, opts);
	}
	trefs ch;
	for (const auto& c : t.children()) {
		if (auto x = inline_tree_paths2(c, opts); x)
			ch.push_back(x);
	}
	return tree::get(t.value, ch);
}
template <typename C, typename T>
tref parser<C, T>::result::inline_tree_paths2(tref ref) const {
	return inline_tree_paths2(ref, shaping);
}

template <typename C, typename T>
tref parser<C, T>::result::inline_tree2(tref ref, const shaping_options opts)
	const
{
	ref = inline_tree_nodes2(ref, opts);
	// tree::get(ref).print(std::cout << "inlined tree nodes: ") << "\n\n";
	return inline_tree_paths2(ref, opts);
}
template <typename C, typename T>
tref parser<C, T>::result::inline_tree2(tref ref) const {
	return inline_tree2(ref, shaping);
}

template <typename C, typename T>
tref parser<C, T>::result::trim_children_terminals2(tref ref,
	const shaping_options opts) const
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
			auto x = trim_children_terminals2(c, opts);
			if (x) ch.push_back(x);
		}
	}
	return tree::get(t.value, ch);
}
template <typename C, typename T>
tref parser<C, T>::result::trim_children_terminals2(tref ref) const {
	return trim_children_terminals2(ref, shaping);
}

template <typename C, typename T>
tref parser<C, T>::result::get_shaped_tree2(tref t, const shaping_options opts){
	// if (!t) return std::cerr << "error getting shaped tree. tree is null" << "\n", nullptr;
	// tree::get(t).print(std::cout << "parsed tree: ") << "\n\n";
	t = get_trimmed_tree2(t, opts);
	// tree::get(t).print(std::cout << "trimmed tree: ") << "\n\n";
	t = inline_tree2(t, opts);
	// tree::get(t).print(std::cout << "inlined tree: ") << "\n\n";
	t = trim_children_terminals2(t, opts);
	// tree::get(t).print(std::cout << "trimmed terminals: ") << "\n\n";
	return t;
}
template <typename C, typename T>
tref parser<C, T>::result::get_shaped_tree2(tref t)
{
	return get_shaped_tree2(t, shaping);
}
template <typename C, typename T>
tref parser<C, T>::result::get_shaped_tree2(const shaping_options opts) {
#ifdef PARSER_BINTREE_FOREST
	return get_shaped_tree2(froot->get(), opts);
#else
	return get_shaped_tree2(get_tree2(), opts);
#endif // PARSER_BINTREE_FOREST
}
template <typename C, typename T>
tref parser<C, T>::result::get_shaped_tree2() {
	return get_shaped_tree2(shaping);
}

template <typename C, typename T>
tref parser<C, T>::result::get_tree2() {
#ifdef PARSER_BINTREE_FOREST
	return froot->get();
#else
	return get_tree2(f->root());
#endif // PARSER_BINTREE_FOREST
}

#ifdef PARSER_BINTREE_FOREST
template <typename C, typename T>
tref parser<C, T>::result::get_tree2(const pnode&) {
	// TODO get tree from pnode
	return froot->get();
}
#else
template <typename C, typename T>
tref parser<C, T>::result::get_tree2(const pnode& n) {
	htree::sp t;
	MS(emeasure_time_start(s,e);)
	MS(std::cout<<"\n extract_graph ";)
	auto g = f->extract_first_graph(n);
	/*f->extract_graphs(n, [this, &t] (auto& g) {
		MS(emeasure_time_start(s1,e1);)
		MS(std::cout<<"\n inline_grammar ";)
		inline_grammar_transformations(g);
		MS(emeasure_time_end(s1, e1);)
		t = g.extract_tree2();
	return false;
	});
	*/
	MS(emeasure_time_end(s, e);)
	
	MS(emeasure_time_start(s1,e1);)
	MS(std::cout<<"\n inline_grammar ";)
	inline_grammar_transformations(g);
	MS(emeasure_time_end(s1, e1);)
	
	
	MS(emeasure_time_start(s2,e2);)
	MS(std::cout<<"\n extract_tree2 ";)
	t = g.extract_tree2();
	MS(emeasure_time_end(s2, e2);)

	/*
	f->extract_graphs(n, [this, &t] (auto& g) {
		inline_grammar_transformations(g);
		t = g.extract_tree2();
		return false;
	});
	*/
	
	
	return t->get();
	
}
#endif // PARSER_BINTREE_FOREST

} // idni namespace
