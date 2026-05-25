// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <optional>
#include "parser.h"
namespace idni {

// ---------------------------------------------------------------------------
// Record a parser error in a diagnostics report
// ---------------------------------------------------------------------------
template <typename C, typename T>
inline void report_parse_error(idni::diagnostics::report& r,
	const typename parser<C, T>::error& err,
	const idni::parser_strings::keys& k,
	typename parser<C, T>::error::info_lvl verbosity =
		parser<C, T>::error::INFO_DETAILED)
{
	using namespace idni::diagnostics;
	auto msg = err.to_str(verbosity);
	r.error(code::parse_error, msg, err.loc,
		{{k.line, err.line},
		 {k.col,  err.col}});
}

// __AMB__ is registered only when an ambiguity wrapper is actually present.
// Both result constructors are defined unconditionally. At instantiation
// time `if constexpr` in _parse selects the one used for the active mode.

template <typename C, typename T>
const lit<C, T>& parser<C, T>::result::ambiguity_literal() const
{
	return amb_node;
}


// bintree-mode constructor — stores tref root, forest is null
template <typename C, typename T>
parser<C, T>::result::result(parser<C, T>& p, std::unique_ptr<input> in_,
	tref f, bool fnd, error err) :
		found(fnd), parse_error(err),
		shaping(p.get_grammar().opt.shaping),
		measure_scopes(p.po.measure_scopes),
		measure_counters(p.po.measure_counters),
		diag_report(std::move(p.report_)),
		p(p), in_(std::move(in_)), froot(tree::geth(f))
{
	if (!fnd) report_parse_error<C, T>(diag_report, err, p.keys(),
		p.po.error_verbosity);
	if (!froot) return;
	auto amb_name = from_str<C>(std::string("__AMB__"));
	auto& nts = p.get_grammar().nts;
	for (size_t i = 0; i < nts.size(); ++i)
		if (nts.get(i) == amb_name) {
			lit<C, T> amb(i, &nts);
			if (tree::get(froot->get()).find_top([&](tref n) {
				return tree::get(n).value.first == amb;
			}) != nullptr)
				amb_node = amb;
			break;
		}
}

// default-mode constructor — stores forest directly
template <typename C, typename T>
parser<C, T>::result::result(parser<C, T>& p, std::unique_ptr<input> in_,
	std::unique_ptr<pforest> f, bool fnd, error err) :
		found(fnd), parse_error(err),
		shaping(p.get_grammar().opt.shaping),
		measure_scopes(p.po.measure_scopes),
		measure_counters(p.po.measure_counters),
		diag_report(std::move(p.report_)),
		p(p), in_(std::move(in_)), f(std::move(f))
{
	if (!fnd) report_parse_error<C, T>(diag_report, err, p.keys(),
		p.po.error_verbosity);
	if (is_ambiguous())
		amb_node = p.get_grammar().nt(from_str<C>(std::string("__AMB__")));
}

template <typename C, typename T>
parser<C, T>& parser<C, T>::result::get_parser() const { return p; }

// Iterative post-order count of distinct parse trees under an AMB-wrapped
// bintree. Folds child counts into the parent via the traversal callback —
// no manual children() walk, no recursion. Semantics:
//   - leaf (no children visited):      count = 1
//   - non-AMB internal node:           count = product of children counts
//   - AMB internal node:               count = sum of children counts
//                                              (empty AMB → 1)
template <typename C, typename T>
size_t count_bintree_parses(tref n, const lit<C, T>& amb) {
	using tree  = typename parser<C, T>::tree;
	using pnode = typename parser<C, T>::pnode;
	if (!n) return 1;
	std::unordered_map<tref, size_t> cnt;
	auto fold = [&](tref node, tref parent) {
		// On first visit, default a leaf's count to 1 (product
		// identity / AMB-empty fallback). Internal nodes will have
		// been pre-initialized by their first child below.
		auto [it_n, _n] = cnt.try_emplace(node, 1);
		size_t n_count = it_n->second;
		if (!parent) return true;
		const auto& pt = tree::get(parent);
		bool parent_is_amb = pt.value.first.nt()
			&& pt.value.first == amb;
		auto [it_p, _p] = cnt.try_emplace(parent,
			parent_is_amb ? 0 : 1);
		if (parent_is_amb) it_p->second += n_count;
		else               it_p->second *= n_count;
		return true;
	};
	post_order<pnode>(n).search(fold);
	auto it = cnt.find(n);
	return it == cnt.end() ? 1 : it->second;
}

// Lazy reconstruction of a pforest from the tref (bintree path).
// In forest_path the forest is already populated and this returns it.
template <typename C, typename T>
typename parser<C, T>::pforest* parser<C, T>::result::get_forest() const {
	if (f) return f.get();
	if (froot != 0) {
		auto _rf = diag_report.open_if(
			measure_scopes, p.keys().reconstruct_forest);
		auto reconstruct = [&]() {
			f = std::make_unique<pforest>();
			if (!froot) return;
			tref root_t = froot->get();
			const auto& rt = tree::get(root_t);
			using pnt = pnode_type<C, T>;
			auto root_value = [&]() -> pnt {
				if (rt.value.first.nt() && rt.value.first == amb_node) {
					if (tref alt0 = rt.first(); alt0) {
						const auto& at = tree::get(alt0);
						return pnt(at.value.first, at.value.second);
					}
				}
				return pnt(rt.value.first, rt.value.second);
			};
			f->root(root_value());
			std::set<pnt> visited;

			auto child_pn = [&](tref c) -> pnt {
				const auto& ct = tree::get(c);
				if (ct.value.first.nt() && ct.value.first == amb_node) {
					tref alt0 = ct.first();
					if (alt0) {
						const auto& at = tree::get(alt0);
						return pnt(at.value.first, at.value.second);
					}
				}
				return pnt(ct.value.first, ct.value.second);
			};

			// Iterative pre-order walk replaces the std::function recursion.
			// Returning false from enter skips the current subtree (used
			// both for non-NT leaves and for nodes whose pn is already in
			// the forest map).
			auto enter = [&](tref n, tref parent) -> bool {
				if (!n) return false;
				const auto& nt = tree::get(n);
				if (!nt.value.first.nt()) return false;
				if (nt.value.first == amb_node) {
					tref alt0 = nt.first();
					if (!alt0) return false;
					const auto& a0 = tree::get(alt0);
					pnt shared(a0.value.first, a0.value.second);
					if (!visited.insert(shared).second) return false;
					typename pforest::nodes_set packs;
					for (tref alt : nt.children()) {
						const auto& at = tree::get(alt);
						typename pforest::nodes pack;
						for (tref c : at.children())
							pack.push_back(child_pn(c));
						packs.insert(pack);
					}
					(*f)[shared] = packs;
					return true;  // descend into alts to reach their children
				}
				// Alt-wrapper node: a direct child of __AMB__. Its pnode
				// equals shared (already in visited), so skip the forest
				// insertion — but return true to descend into its children
				// (the actual pack-member nodes that need forest entries).
				if (parent && tree::get(parent).value.first == amb_node)
					return true;
				pnt me(nt.value.first, nt.value.second);
				if (!visited.insert(me).second) return false;
				typename pforest::nodes pack;
				for (tref c : nt.children()) pack.push_back(child_pn(c));
				typename pforest::nodes_set packs;
				packs.insert(pack);
				(*f)[me] = packs;
				return true;
			};
			auto all_pass = [](tref) { return true; };
			auto noop_leave = [](tref, tref) {};
			auto noop_between = [](tref, tref) {};
			pre_order<pnode>(root_t).visit(enter, all_pass,
				noop_leave, noop_between);
		};
		reconstruct();
	}
	return f.get();
}

template <typename C, typename T>
bool parser<C, T>::result::good() const { return in_->good(); }

template <typename C, typename T>
std::basic_string<C> parser<C, T>::result::get_input() {
	return in_->get_string();
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
	auto name = n.first.to_std_string();
	auto matches_inline_prefix = [&name]() {
		static const std::vector<std::string> prefixes = {
			"__E_", // ebnf prefix
			"__B_"  // binarization prefix
		};
		for (auto& prefix : prefixes)
			if (name.find(prefix) != std::string::npos) return true;
		return false;
	};
	auto is_char_class = [&name]() {
		static const std::vector<std::string> list = {
			"eof",  "alnum", "alpha", "blank",
				"cntrl", "digit", "graph", "lower", "printable",
				"punct", "space", "upper", "xdigit"
		};
		return std::find(list.begin(), list.end(), name) != list.end();
	};
	if ((opts.inline_char_classes && is_char_class())
		|| matches_inline_prefix()) return true;
	for (const auto& i : opts.to_inline)
		if (i.size() == 1 && i[0] == n.first.n()) return true;
	return false;
}

template <typename C, typename T>
typename parser<C, T>::psptree
	parser<C, T>::result::get_trimmed_forest_tree(
		const pnode& n, const shaping_options opts) const
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
			auto x = get_trimmed_forest_tree(c, opts);
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
typename parser<C, T>::psptree
	parser<C, T>::result::get_trimmed_forest_tree(const pnode& n) const
{
	return get_trimmed_forest_tree(n, shaping);
}

template <typename C, typename T>
typename parser<C, T>::psptree
	parser<C, T>::result::inline_forest_tree_nodes(const psptree& t,
		psptree& parent, const shaping_options opts) const
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
		auto x = inline_forest_tree_nodes(c, rf, opts);
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
typename parser<C, T>::psptree
	parser<C, T>::result::inline_forest_tree_nodes(const psptree& t,
		psptree& parent) const
{
	return inline_forest_tree_nodes(t, parent, shaping);
}

template <typename C, typename T>
typename parser<C, T>::psptree
	parser<C, T>::result::inline_forest_tree_paths(const psptree& t,
		const shaping_options opts) const
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
		if (p) return inline_forest_tree_paths(p, opts);
	}
	r = std::make_shared<ptree>(t->value);
	for (auto& c : t->child) {
		auto x = inline_forest_tree_paths(c, opts);
		if (x) r->child.push_back(x);
	}
	return r;
}
template <typename C, typename T>
typename parser<C, T>::psptree
	parser<C, T>::result::inline_forest_tree_paths(const psptree& t) const
{
	return inline_forest_tree_paths(t, shaping);
}

template <typename C, typename T>
typename parser<C, T>::psptree
	parser<C, T>::result::inline_forest_tree(psptree& t,
		const shaping_options opts) const
{
	auto inlined = inline_forest_tree_nodes(t, t, opts);
	return inline_forest_tree_paths(inlined, opts);
}
template <typename C, typename T>
typename parser<C, T>::psptree
	parser<C, T>::result::inline_forest_tree(psptree& t) const
{
	return inline_forest_tree(t, shaping);
}

template <typename C, typename T>
typename parser<C, T>::psptree
	parser<C, T>::result::trim_forest_tree_child_terminals(
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
			auto x = trim_forest_tree_child_terminals(c, opts);
			if (x) r->child.push_back(x);
		}
	}
	return r;
}
template <typename C, typename T>
typename parser<C, T>::psptree
	parser<C, T>::result::trim_forest_tree_child_terminals(
		const psptree& t) const
{
	return trim_forest_tree_child_terminals(t, shaping);
}

template <typename C, typename T>
typename parser<C, T>::psptree
	parser<C, T>::result::get_shaped_forest_tree(const pnode& n,
		const shaping_options opts) const
{
	//std::cout << "getting tree for " << n.first.to_std_string() << std::endl;
	auto trimmed = get_trimmed_forest_tree(n, opts);
	auto inlined = inline_forest_tree(trimmed, opts);
	auto shaped  = trim_forest_tree_child_terminals(inlined, opts);
	return shaped;
}
template <typename C, typename T>
typename parser<C, T>::psptree
	parser<C, T>::result::get_shaped_forest_tree(const pnode& n) const
{
	return get_shaped_forest_tree(n, shaping);
}
template <typename C, typename T>
typename parser<C, T>::psptree
	parser<C, T>::result::get_shaped_forest_tree(
		const shaping_options opts) const
{
	return get_shaped_forest_tree(get_forest()->root(), opts);
}
template <typename C, typename T>
typename parser<C, T>::psptree
	parser<C, T>::result::get_shaped_forest_tree() const
{
	return get_shaped_forest_tree(shaping);
}

/// get a first parse tree from the parse_forest optionally provide root of the tree.
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::get_forest_tree() {
	return get_forest_tree(get_forest()->root());
}

template <typename C, typename T>
typename parser<C, T>::psptree
	parser<C, T>::result::get_forest_tree(const pnode& n)
{
	psptree t;
	auto extract = [this, &t, &n] {
		get_forest()->extract_graphs(n, [this, &t](auto& g) {
			inline_grammar_transformations(g);
			t = g.extract_trees();
			return false;
		});
	};
	diag_report.step(measure_scopes, p.keys().extract_graph, [&] {
		extract();
	});
	return t;
}

template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::get_trimmed_tree(
	const pnode& n, const shaping_options opts) const
{
	return get_trimmed_forest_tree(n, opts);
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::get_trimmed_tree(
	const pnode& n) const
{
	return get_trimmed_forest_tree(n);
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::inline_tree_nodes(
	const psptree& t, psptree& parent, const shaping_options opts) const
{
	return inline_forest_tree_nodes(t, parent, opts);
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::inline_tree_nodes(
	const psptree& t, psptree& parent) const
{
	return inline_forest_tree_nodes(t, parent);
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::inline_tree_paths(
	const psptree& t, const shaping_options opts) const
{
	return inline_forest_tree_paths(t, opts);
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::inline_tree_paths(
	const psptree& t) const
{
	return inline_forest_tree_paths(t);
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::inline_tree(
	psptree& t, const shaping_options opts) const
{
	return inline_forest_tree(t, opts);
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::inline_tree(
	psptree& t) const
{
	return inline_forest_tree(t);
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::trim_children_terminals(
	const psptree& t, const shaping_options opts) const
{
	return trim_forest_tree_child_terminals(t, opts);
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::trim_children_terminals(
	const psptree& t) const
{
	return trim_forest_tree_child_terminals(t);
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::get_shaped_tree(
	const pnode& n, const shaping_options opts) const
{
	return get_shaped_forest_tree(n, opts);
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::get_shaped_tree(
	const pnode& n) const
{
	return get_shaped_forest_tree(n);
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::get_shaped_tree(
	const shaping_options opts) const
{
	return get_shaped_forest_tree(opts);
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::get_shaped_tree() const
{
	return get_shaped_forest_tree();
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::get_tree() {
	return get_forest_tree();
}
template <typename C, typename T>
typename parser<C, T>::psptree parser<C, T>::result::get_tree(
	const pnode& n)
{
	return get_forest_tree(n);
}

template <typename C, typename T>
std::basic_string<T> parser<C, T>::result::get_terminals() const {
	if (f) return in_->get_terminals(f->root()->second);
	if (froot != 0)
		return in_->get_terminals(tree::get(froot).value.second);
	return {};
}

template <typename C, typename T>
std::basic_string<T> parser<C, T>::result::get_terminals(const pnode& n) const {
	return in_->get_terminals(n.second);
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
	if (froot != 0) {
		if (!froot || !amb_node.nt()) return false;
		return tree::get(froot->get()).find_top([this](tref n) {
			return tree::get(n).value.first == amb_node;
		}) != nullptr;
	}
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
		parser<C, T>::result::ambiguous_nodes_from_bintree() const
{
	std::set<std::pair<pnode, pnodes_set>> r;
	if (!froot || !amb_node.nt()) return r;

	// Iterative pre-order over the bintree, gathering an entry per AMB:
	//   - amb_ctx tracks the active AMB (shared pnode + accumulated packs);
	//   - alt_ctx tracks the active alt under its AMB (collecting children
	//     of the alt into a pack).
	// Stacks handle nested AMBs (an alt that is itself an AMB). No
	// .children() walks here — the traversal visits every node and the
	// (n, parent) callback signature provides the structural context.
	struct amb_ctx {
		tref amb;
		tref first_alt = nullptr; // tref, since pnode is non-assignable
		pnodes_set packs{};
	};
	struct alt_ctx { tref alt; pnodes pack{}; };
	std::vector<amb_ctx> ambs;
	std::vector<alt_ctx> alts;

	// When a pack member is itself a nested __AMB__ node, project it to
	// the AMB's first alternative — mirrors child_pn() in get_forest, so
	// the public ambiguity packs match the forest reconstruction.
	auto unwrap_amb = [&](tref n, const pnode& v) -> pnode {
		if (v.first != amb_node) return v;
		tref alt0 = tree::get(n).first();
		if (!alt0) return v;
		return tree::get(alt0).value;
	};
	auto enter = [&](tref n, tref parent) {
		if (!n) return true;
		const auto& nt = tree::get(n);
		// Pack-member of the innermost active alt (not a nested AMB):
		if (!alts.empty() && parent == alts.back().alt
			&& tree::get(alts.back().alt).value.first != amb_node)
			alts.back().pack.push_back(unwrap_amb(n, nt.value));
		// Alt of the innermost active AMB (skip when alt is __AMB__):
		if (!ambs.empty() && parent == ambs.back().amb
			&& nt.value.first != amb_node) {
			if (!ambs.back().first_alt) ambs.back().first_alt = n;
			alts.push_back({n, {}});
		}
		// Open a new amb_ctx if this node is itself an AMB:
		if (nt.value.first == amb_node)
			ambs.push_back({n});
		return true;
	};
	auto leave = [&](tref n, tref) {
		// Close inner AMB first (depth matches: amb opened in enter
		// after the alt push, so it must close before the alt).
		if (!ambs.empty() && ambs.back().amb == n) {
			if (ambs.back().first_alt)
				r.insert({tree::get(ambs.back().first_alt)
						.value,
					std::move(ambs.back().packs)});
			ambs.pop_back();
		}
		// Then close the alt for the now-current AMB (not a nested AMB).
		if (!alts.empty() && alts.back().alt == n
			&& tree::get(n).value.first != amb_node) {
			if (!ambs.empty()) ambs.back().packs.insert(
					std::move(alts.back().pack));
			alts.pop_back();
		}
	};
	auto all = [](tref) { return true; };
	auto no_between = [](tref, tref) {};
	pre_order<pnode>(froot->get()).visit(enter, all, leave, no_between);
	return r;
}

template <typename C, typename T>
std::set<std::pair<typename parser<C, T>::pnode,
	typename parser<C, T>::pnodes_set>>
		parser<C, T>::result::ambiguous_nodes_from_forest() const
{
	std::set<std::pair<pnode, pnodes_set>> r;
	if (!f) return r;
	for (auto& kv : f->g) if (kv.second.size() > 1) r.insert(kv);
	return r;
}

template <typename C, typename T>
std::set<std::pair<typename parser<C, T>::pnode,
	typename parser<C, T>::pnodes_set>>
		parser<C, T>::result::ambiguous_nodes() const
{
	if (froot != 0) return ambiguous_nodes_from_bintree();
	return ambiguous_nodes_from_forest();
}

template <typename C, typename T>
std::ostream& parser<C, T>::result::print_ambiguous_nodes(std::ostream& os)
	const
{
	if (!is_ambiguous()) return os;
	if (froot != 0) {
		os << "# n trees: " << count_bintree_parses(froot->get(), amb_node)
			<< "\n# ambiguous nodes:\n";
		for (auto& n : ambiguous_nodes()) {
			os << "\t `" << n.first.first << "` [" << n.first.second[0]
				<< "," << n.first.second[1] << "]\n";
			size_t d = 0;
			for (auto& ns : n.second) {
				std::stringstream ss;
				ss << "\t\t " << d++ << "\t";
				for (auto& nt : ns) ss << " `" << nt->first << "`["
					<< nt->second[0] << "," << nt->second[1] << "] ";
				os << ss.str() << "\n";
			}
		}
		return os;
	}
	auto* pf = get_forest();
	if (!pf) return os;
	os << "# n trees: " << pf->count_trees() << "\n# ambiguous nodes:\n";
	for (auto& n : ambiguous_nodes()) {
		os << "\t `" << n.first.first << "` [" << n.first.second[0]
			<< "," << n.first.second[1] << "]\n";
		size_t d = 0;
		for (auto& ns : n.second) {
			std::stringstream ss;
			ss << "\t\t " << d++ << "\t";
			for (auto& nt : ns) ss << " `" << nt->first << "`["
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
	using node = std::pair<lit<C, T>, std::array<size_t, 2>>;
	std::map<node, size_t> nid;
	std::map<size_t, node> ns;
	std::vector<node> n;
	edges es;
	size_t id = 0;
	auto* pf = get_forest();
	if (!pf) return nodes_and_edges{ n, es };
	for (auto& it : pf->g) {
		nid[it.first] = id;
		// skip ids for one child ambig node
		id += it.second.size() == 1 ? 0 : it.second.size(); // ambig node ids;
		//DBG(assert(it.second.size()!= 0));
		id++;
	}
	for (auto& it : pf->g) {
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
	static const std::set<std::basic_string<C>> prefixes = {
		from_cstr<C>("__E_"), // ebnf prefix
		from_cstr<C>("__B_")  // binarization prefix
		//from_cstr<C>("__N_")  // negation prefix
	};
	return inline_nodes(g, get_nts_by_prefixes(prefixes));
}

template <typename C, typename T>
std::set<size_t> parser<C, T>::result::get_nts_by_prefixes(
	const std::set<std::basic_string<C>>& prefixes) const
{
	std::set<size_t> r;
	nonterminals<C, T>& nts = p.get_grammar().nts;
	for (size_t i = 0; i < nts.size(); ++i)
		for (const auto& prefix : prefixes)
			if (nts.get(i).find(prefix)
				!= std::basic_string<C>::npos)
			{
				// std::cout << "found prefix " << prefix << " in " << nts.get(i) << std::endl;
				r.insert(i);
			}
	return r;
}

template <typename C, typename T>
bool parser<C, T>::result::inline_prefixed_nodes(pgraph& g,
	const std::basic_string<C>& prefix)
{
	return inline_nodes(g, get_nts_by_prefixes({ prefix }));
}

template <typename C, typename T>
bool parser<C, T>::result::inline_nodes(pgraph& g,
	const std::set<size_t>& nts_to_inline)
{
	//collect all nodes for replacement
	pnodes s;
	for (auto& kv : f->g) {
		const lit<C, T>& l = kv.first->first;
		if (l.nt() && nts_to_inline.find(l.n()) != nts_to_inline.end())
		{
			s.insert(s.end(), kv.first);
			// std::cout << "inserted node " << l.to_std_string() << std::endl;
		}
	}
	return f->replace_nodes(g, s);
}

} // idni namespace
