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
#ifndef __IDNI__PARSER__PARSER_RESULT_TMPL_H__
#define __IDNI__PARSER__PARSER_RESULT_TMPL_H__
#include <optional>
#include "parser.h"
namespace idni {

#ifdef PARSER_BINTREE_FOREST
template <typename C, typename T>
parser<C, T>::result::result(grammar<C, T>& g, std::unique_ptr<input> in,
	tref f, bool fnd, error err) :
		found(fnd), parse_error(err), shaping(g.opt.shaping),
		in(std::move(in)), froot(tree::geth(f))
{}
#else
template <typename C, typename T>
parser<C, T>::result::result(grammar<C, T>& g, std::unique_ptr<input> in,
	std::unique_ptr<pforest> f, bool fnd, error err) :
		found(fnd), parse_error(err), shaping(g.opt.shaping),
		in(std::move(in)), f(std::move(f))
{
	// if is ambiguous add __AMB__ node to the nonterminals dict so it can
	// be added to the resulting parse tree when get_shaped_tree() is called
	static const std::string amb = "__AMB__";
	if (is_ambiguous()) amb_node = g.nt(from_str<C>(amb));
}

template <typename C, typename T>
typename parser<C, T>::pforest* parser<C, T>::result::get_forest() const {
	if (f) return f.get();
	return nullptr;
}
#endif

template <typename C, typename T>
bool parser<C, T>::result::good() const { return in->good(); }

template <typename C, typename T>
std::basic_string<C> parser<C, T>::result::get_input() {
	return in->get_string();
}

template <typename C, typename T>
bool trimmable_node(const typename parser<C, T>::pnode& n,
	const shaping_options& opts)
{
	if (!n.first.nt()) return n.first.is_null();
	if (opts.to_trim.find(n.first.n()) != opts.to_trim.end()) return true;
	return false;
}

template <typename C, typename T>
bool trimmable_child(const typename parser<C, T>::pnode& n,
	const typename parser<C, T>::pnode& c,
	const shaping_options& opts)
{
	if (trimmable_node<C, T>(c, opts)) return true;
	if (n.first.nt() && opts.to_trim_children.find(n.first.n())
			!= opts.to_trim_children.end()) return true;
	return !c.first.nt() && opts.to_trim_children_terminals.find(n.first.n())
		!= opts.to_trim_children_terminals.end();
}

template <typename C, typename T>
bool node_to_inline(const typename parser<C, T>::pnode& n,
	const shaping_options& opts)
{
	if (!n.first.nt()) return false;
	auto matches_inline_prefix = [](const parser<C, T>::pnode& n) {
		static const std::vector<std::string> prefixes = {
			"__E_", // ebnf prefix
			"__B_"  // binarization prefix
		};
		if (n.first.nt()) {
			auto s = n.first.to_std_string();
			for (auto& prefix : prefixes)
				if (s.find(prefix) != decltype(s)::npos)
					return true;
		}
		return false;
	};
	static const std::vector<std::string> cc_names = {
		"eof",  "alnum", "alpha", "blank",
                        "cntrl", "digit", "graph", "lower", "printable",
                        "punct", "space", "upper", "xdigit"
	};
	auto one_of_str = [](const parser<C, T>::pnode& n,
		const std::vector<std::string>& list)
	{
		return std::find(list.begin(), list.end(),
			n.first.to_std_string()) != list.end();
	};
	if ((opts.inline_char_classes && one_of_str(n, cc_names))
		|| matches_inline_prefix(n)) return true;
	for (const auto& i : opts.to_inline)
		if (i.size() == 1 && i[0] == n.first.n()) return true;
	return false;
}

#ifndef PARSER_BINTREE_FOREST
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::get_trimmed_tree(
	const typename parser<C, T>::pnode& n, const shaping_options opts) const
{
	//std::cerr << "get_trimmed_tree for node: `" << n.first.to_std_string() << "`" << std::endl;
	auto get_children_nodes = [&](const pnode& n,
		const pnodes& nodes, std::vector<psptree>& output)
	{
		//std::cerr << "getting children for nodes of `" << n.first.to_std_string() << "`" << std::endl;
		for (auto& c : nodes) {
			//std::cerr << "getting child `" << c.first.to_std_string() << "` for node `" << n.first.to_std_string() << "`" << std::endl;
			if (trimmable_child<C, T>(n, c, opts)) {
				//std::cerr << "trimmable child: " << c.first.to_std_string();
				//std::cerr << " in: " << n.first.to_std_string() << std::endl;
				continue;
			}
			auto x = get_trimmed_tree(c, opts);
			if (x) output.push_back(x);
		}
	};
	psptree t = 0;
	if (n.first.is_null()) return t;
	if (trimmable_node<C, T>(n, opts)) {
		//std::cerr << "\t is trimmable, skipping" << std::endl;
		return t;
	}
	if (!n.first.nt()) {
		//std::cerr << "\t terminal node, returning" << std::endl;
		t = std::make_shared<ptree>(n);
		return t;
	}
	auto f = get_forest();
	auto it = f->g.find(n);
	if (it == f->g.end()) {
		//std::cerr << "\t not existing node, skipping" << std::endl;
		return t;
	}
	t = std::make_shared<ptree>(n);
	pnodes_set pack = it->second;
	if (pack.size() > 1) {
		//std::cerr << "Ambigous node: `" << n.first.to_std_string() << "`" << std::endl;
		// move ambiguous children sets each into its separate child
		// copy them also a value of amb. node
		// and replace value of amb. node with nt: __AMB_<ID>
		pnode x = t->value;
		x.first = amb_node;
		t = std::make_shared<ptree>(x);
		//std::cout << "Ambigous node " << n.first.to_std_string() << std::endl;
		for (auto& nodes : pack) {
			psptree tc = std::make_shared<ptree>(n);
			get_children_nodes(n, nodes, tc->child);
			t->child.push_back(tc);
		}
	} else for (auto& nodes : pack) get_children_nodes(n, nodes, t->child);
	return t;
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::get_trimmed_tree(
	const typename parser<C, T>::pnode& n) const
{
	return get_trimmed_tree(n, shaping);
}

template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::inline_tree_nodes(
	const psptree& t, psptree& parent, const shaping_options opts) const
{
	psptree r = 0;
	if (!t) return r;
	auto& l = t->value->first;
	if (l.is_null()) return r;
	if (!l.nt()) return r = std::make_shared<ptree>(t->value), r;
	//std::cerr << "inlining tree for node: `" << l.to_std_string() << "`" << std::endl;
	bool do_inline = (t != parent) && node_to_inline<C, T>(t->value, opts);
	//std::cerr << "do_inline: " << do_inline << std::endl;
	if (!do_inline) r = std::make_shared<ptree>(t->value);
	psptree& rf = do_inline ? parent : r;
	for (auto& c : t->child) {
		auto x = inline_tree_nodes(c, rf, opts);
		if (x) //{
			//std::cerr << "inlining child: `" << c->value.first.to_std_string() << "`" << std::endl;
			//std::cerr << (do_inline ? "- inlined\t" : "- passed\t") << " `"
			//	<< x->value.first.to_std_string() << "`" << std::endl,
			rf->child.push_back(x);
		//} else std::cerr << "skipped\t" << c->value.first.to_std_string() << std::endl;
	}
	return r;
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::inline_tree_nodes(
	const psptree& t, psptree& parent) const
{
	return inline_tree_nodes(t, parent, shaping);
}

template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::inline_tree_paths(
	const psptree& t, const shaping_options opts) const
{
	psptree r = 0;
	if (!t) return r;
	auto& l = t->value->first;
	if (l.is_null()) return r;
	if (!l.nt()) return r = std::make_shared<ptree>(t->value), r;
	//std::cerr << "inlining treepath for node: `" << l.to_std_string() << "`" << std::endl;
	for (auto& tp : opts.to_inline) {
		if (tp.size() < 2) continue;
		std::function<const psptree(const psptree&, size_t)>
			go = [&](const psptree& t, size_t i) -> const psptree
		{
			if (!t->value->first.nt()
				|| t->value->first.n() != tp[i]) return 0;
			//std::cerr << "treepath go " << i << ": `"
			//	<< t->value.first.to_std_string() << "`" << std::endl;
			if (i == tp.size() - 1) return t;
			for (const auto& c : t->child) {
				auto y = go(c, i + 1);
				if (y) return y;
			}
			return 0;
		};
		const psptree p = go(t, 0);
		if (p) return inline_tree_paths(p, opts);
	}
	r = std::make_shared<ptree>(t->value);
	for (auto& c : t->child) {
		auto x = inline_tree_paths(c, opts);
		if (x) r->child.push_back(x);
	}
	return r;
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::inline_tree_paths(
	const psptree& t) const
{
	return inline_tree_paths(t, shaping);
}

template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::inline_tree(
	psptree& t, const shaping_options opts) const
{
	auto inlined = inline_tree_nodes(t, t, opts);
	return inline_tree_paths(inlined, opts);
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::inline_tree(
	psptree& t) const
{
	return inline_tree(t, shaping);
}

template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::trim_children_terminals(
	const psptree& t, const shaping_options opts) const
{
	psptree r = 0;
	if (!t) return r;
	auto& l = t->value->first;
	if (!l.nt()) return r = t, r;
	bool trim = (opts.trim_terminals
				&& opts.dont_trim_terminals_of.find(l.n())
					== opts.dont_trim_terminals_of.end())
			|| opts.to_trim_children_terminals.find(l.n())
				!= opts.to_trim_children_terminals.end();
	r = std::make_shared<ptree>(t->value);
	for (auto& c : t->child) {
		auto& cl = c->value->first;
		if (cl.nt() || cl.is_null() || !trim) {
			auto x = trim_children_terminals(c, opts);
			if (x) r->child.push_back(x);
		}
	}
	return r;
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::trim_children_terminals(
	const psptree& t) const
{
	return trim_children_nodes(t, shaping);
}

template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::get_shaped_tree(
	const typename parser<C, T>::pnode& n, const shaping_options opts) const
{
	//std::cout << "getting tree for " << n.first.to_std_string() << std::endl;
	auto trimmed = get_trimmed_tree(n, opts);
	auto inlined = inline_tree(trimmed, opts);
	auto shaped  = trim_children_terminals(inlined, opts);
	return shaped;
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::get_shaped_tree(
	const typename parser<C, T>::pnode& n) const
{
	return get_shaped_tree(n, shaping);
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::get_shaped_tree(
	const shaping_options opts) const
{
	return get_shaped_tree(get_forest()->root(), opts);
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::get_shaped_tree() const
{
	return get_shaped_tree(shaping);
}

// get a first parse tree from the parse_forest optionally provide root of the tree.
template <typename C, typename T>
parser<C, T>::psptree parser<C, T>::result::get_tree() {
	return get_tree(f->root());
}

template <typename C, typename T>
parser<C, T>::psptree parser<C, T>::result::get_tree(const pnode& n) {
	psptree t;
	f->extract_graphs(n, [this, &t] (auto& g) {
		inline_grammar_transformations(g);
		t = g.extract_trees();
		return false;
	});
	return t;
}

#endif // not PARSER_BINTREE_FOREST

template <typename C, typename T>
std::basic_string<T> parser<C, T>::result::get_terminals() const {
#ifdef PARSER_BINTREE_FOREST
	return in->get_terminals(tree::get(froot).value.second);
#else
	return in->get_terminals(f->root()->second);
#endif
}

template <typename C, typename T>
std::basic_string<T> parser<C, T>::result::get_terminals(const pnode& n) const {
	return in->get_terminals(n.second);
}

template <typename C, typename T>
std::optional<int_t> parser<C, T>::result::get_terminals_to_int(const pnode& n)
	const
{
	std::stringstream is(to_std_string(get_terminals(n)));
	int_t result = 0;
	if (!(is >> result)) return {};
	return result;
}

#ifndef PARSER_BINTREE_FOREST
template <typename C, typename T>
bool parser<C, T>::result::is_ambiguous() const {
	if (!f) return false;
	for (auto& kv : f->g) if (kv.second.size() > 1) return true;
	return false;
}

template <typename C, typename T>
bool parser<C, T>::result::has_single_parse_tree() const {
	return !is_ambiguous();
}

template <typename C, typename T>
std::set<std::pair<typename parser<C, T>::pnode,
	typename parser<C, T>::pnodes_set>>
		parser<C, T>::result::ambiguous_nodes() const
{
	if (!f) return {};
	std::set<std::pair<pnode, pnodes_set>> r;
	for (auto& kv : f->g) if (kv.second.size() > 1) r.insert(kv);
	return r;
}

template <typename C, typename T>
std::ostream& parser<C, T>::result::print_ambiguous_nodes(std::ostream& os)
	const
{
	if (!is_ambiguous()) return os;
	os << "# n trees: " << f->count_trees() << "\n# ambiguous nodes:\n";
	for (auto& n : ambiguous_nodes()) {
		os << "\t `" << n.first.first << "` [" << n.first.second[0]
			<< "," << n.first.second[1] << "]\n";
		size_t d = 0;
		for (auto ns : n.second) {
			std::stringstream ss;
			ss << "\t\t " << d++ << "\t";
			for (auto nt : ns) ss << " `" << nt->first << "`["
				<< nt->second[0] << "," << nt->second[1] << "] ";
			os << ss.str() << "\n";
		}
	}
	return os;
}

template <typename C, typename T>
typename parser<C, T>::result::nodes_and_edges
	parser<C, T>::result::get_nodes_and_edges() const
{
	std::map<pnode, size_t> nid;
	std::map<size_t, pnode> ns;
	pnodes n;
	edges es;
	size_t id = 0;
	if (!f) return nodes_and_edges{ n, es };
	for (auto& it : f->g) {
		nid[it.first] = id;
		// skip ids for one child ambig node
		id += it.second.size() == 1 ? 0 : it.second.size(); // ambig node ids;
		//DBG(assert(it.second.size()!= 0));
		id++;
	}
	for (auto& it : f->g) {
		ns[nid[it.first]] = it.first;
		size_t p = 0;
		for (auto& pack : it.second) {
			if (it.second.size() > 1) {  //skipping if only one ambigous node, an optimization
				++p;
				ns[nid[it.first] + p] = it.first;
				es.emplace_back(nid[it.first], nid[it.first]+p);
			}
			for (auto& nn : pack) {
				if (nid.find(nn) == nid.end()) nid[nn] = id++; // for terminals, not seen before
				ns[nid[nn]] = nn;
				es.emplace_back(nid[it.first] + p, nid[nn]);
			}
		}
	}
	n.resize(id);
	for (auto& p : ns) n[p.first] = p.second;
	return nodes_and_edges{ n, es };
}

template <typename C, typename T>
bool parser<C, T>::result::inline_grammar_transformations(pgraph& g) {
	bool r = false;
	std::set<std::string> prefixes = {
		"__E_", // ebnf prefix
		"__B_"  // binarization prefix
		//"__N_"  // negation prefix
	};
	for (auto& prefix : prefixes) r |= inline_prefixed_nodes(g, prefix);
	return r;
}

template <typename C, typename T>
bool parser<C, T>::result::inline_prefixed_nodes(pgraph& g,
	const std::string& prefix)
{
	//collect all prefix like nodes for replacement
	pnodes s;
	for (auto& kv : f->g) {
		auto name = kv.first->first.to_std_string();
		if (name.find(prefix) != decltype(name)::npos)
			s.insert(s.end(), kv.first);
	}
	return f->replace_nodes(g, s);
}
#endif

} // idni namespace
#endif // __IDNI__PARSER__PARSER_RESULT_TMPL_H__
