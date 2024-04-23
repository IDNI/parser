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
	return f.get();
}

template <typename C, typename T>
std::basic_string<C> parser<C, T>::result::get_input() {
	return in->get_string();
}
template <typename C, typename T>
parser<C, T>::psptree parser<C, T>::result::get_shaped_tree() const
{
	return get_shaped_tree(f->root(), shaping);
}

template <typename C, typename T>
parser<C, T>::psptree parser<C, T>::result::get_shaped_tree(
	const shaping_options opts) const
{
	return get_shaped_tree(f->root(), opts);
}

template <typename C, typename T>
parser<C, T>::psptree parser<C, T>::result::get_shaped_tree(const pnode& n)
	const
{
	return get_shaped_tree(n, shaping);
}

template <typename C, typename T>
parser<C, T>::psptree parser<C, T>::result::get_shaped_tree(const pnode& n,
	const shaping_options opts) const
{
	//std::cout << "getting tree for " << n.first.to_std_string() << std::endl;
	psptree t = std::make_shared<ptree>(n);
	pnodes_set pack;
	auto one_of = [](const pnode& n, const std::set<size_t>& list) {
		if (n.first.nt()) for (auto& nt : list)
			if (n.first.n() == nt) return true;
		return false;
	};
	if (!n.first.nt() && n.first.is_null()) return NULL;
	if (n.first.nt() && !one_of(n, opts.to_trim_children)) {
		auto it = f->g.find(n);
		if (it == f->g.end()) {
			//std::cout << "Not existing node " << n.first.to_std_string() << std::endl;
			return NULL;
		}
		pack = it->second;
		if (pack.size() > 1) {
			// move ambiguous children sets each into its separate child
			// copy them also a value of amb. node
			// and replace value of amb. node with nt: __AMB_<ID>
			auto x = t->value;
			x.first = amb_node;
			t = std::make_shared<ptree>(x);
			//std::cout << "Ambigous node " << n.first.to_std_string() << std::endl;
			for (auto& nodes : pack) {
				psptree tc = std::make_shared<ptree>(n);
				//std::cout << "getting children for AMBIGUOUS "
				//	<< n.first.to_std_string() << std::endl;
				_get_shaped_tree_children(
					opts, nodes, tc->child);
				t->child.push_back(tc);
			}
		} else for (auto& nodes : pack)
			//std::cout << "getting children for " << n.first.to_std_string() << std::endl,
			_get_shaped_tree_children(opts, nodes, t->child);
	}
	//std::cout << "returning tree for " << n.first.to_std_string() << " children size = " << t->child.size() << std::endl;
	return t;
}

template <typename C, typename T>
void parser<C, T>::result::_get_shaped_tree_children(
	const shaping_options& opts,
	const std::vector<pnode>& nodes,
	std::vector<psptree>& child) const
{
	auto matches_inline_prefix = [](const pnode& n) {
		static const std::vector<std::string> prefixes = {
			"__E_", // ebnf prefix
			"__B_"  // binarization prefix
			//"__N_"  // negation prefix
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
	auto one_of = [](const pnode& n, const std::set<size_t>& list) {
		if (n.first.nt()) for (auto& nt : list)
			if (n.first.n() == nt) return true;
		return false;
	};
	auto one_of_str = [](const pnode& n,
		const std::vector<std::string>& list)
	{
		return std::find(list.begin(), list.end(),
			n.first.to_std_string()) != list.end();
	};
	auto get_children = [&](const pnode& n) -> std::set<std::vector<pnode>>{
		if (one_of(n, opts.to_trim_children)) return {};
		auto it = f->g.find(n);
		if (it != f->g.end()) return it->second;
		return {};
	};
	auto inline_children = [&](const pnode& n) {
		for (const auto& cnodes : get_children(n))
			//std::cout << "getting children for inlined " << chd.first.to_std_string() << std::endl,
			_get_shaped_tree_children(opts, cnodes, child);
	};
	std::function<const pnode*(const pnode& n,
		const std::vector<size_t>& tp, size_t& i)> go;
	go = [&](const pnode& n, const std::vector<size_t>& tp, size_t& i)
		-> const pnode*
	{
		if (!n.first.nt()) return 0;
		if (matches_inline_prefix(n))
			for (const auto& cnodes : get_children(n))
				for (const auto& c : cnodes) {
					const pnode* y = go(c, tp, i);
					if (y) return go(*y, tp, i);
				}
		else if (n.first.nt() && n.first.n() == tp[i]) {
			if (++i == tp.size()) return &n;
			for (const auto& cnodes : get_children(n))
				for (const auto& c : cnodes) {
					const pnode* y = go(c, tp, i);
					if (y) return i == tp.size() ? y
								: go(*y, tp, i);
				}
		}
		return 0;
	};
	// returns node pointer according to path from n or 0 if not found
	auto treepath = [&go](const pnode& n, const std::vector<size_t>& tp)
		-> const pnode*
	{
		size_t i = 0;
		return go(n, tp, i);
	};
	std::function<bool(const pnode&)> do_inline = [&](const pnode& n) {
		for (auto& tp : opts.to_inline)
			if (auto p = treepath(n, tp); p) {
				if (tp.size() == 1)
					inline_children(*p);
				else { // if path is more than 1 level deep
					auto x = get_shaped_tree(*p, opts);
					if (x && !do_inline(x->value))
						child.push_back(x);
				}
				return true;
			}
		return false;
	};
	for (auto& chd : nodes) {
		if (one_of(chd, opts.to_trim)
			|| (opts.trim_terminals && !chd.first.nt())) continue;
		if (do_inline(chd)) continue;
		if (matches_inline_prefix(chd)
			|| (opts.inline_char_classes
				&& one_of_str(chd, cc_names)))
					inline_children(chd);
		else if (!chd.first.is_null()) {
			auto x = get_shaped_tree(chd, opts);
			if (x) child.push_back(x);
		}
	}
};

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

template <typename C, typename T>
std::basic_string<T> parser<C, T>::result::get_terminals() const {
	return in->get_terminals(f->root().second);
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

template <typename C, typename T>
bool parser<C, T>::result::is_ambiguous() const {
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
			for (auto nt : ns) ss << " `" << nt.first << "`["
				<< nt.second[0] << "," << nt.second[1] << "] ";
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
	std::vector<pnode> s;
	for (auto& kv : f->g) {
		auto name = kv.first.first.to_std_string();
		if (name.find(prefix) != decltype(name)::npos)
			s.insert(s.end(), kv.first);
	}
	return f->replace_nodes(g, s);
}

} // idni namespace
#endif // __IDNI__PARSER__PARSER_RESULT_TMPL_H__
