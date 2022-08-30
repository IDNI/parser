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
#ifndef __IDNI__PARSER_H__
#define __IDNI__PARSER_H__
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <variant>
#include <functional>
#include <iomanip>
#include "characters.h"
namespace idni {

#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif

#define tdiff(start, end) ((double(end - start) / CLOCKS_PER_SEC) * 1000)
#define emeasure_time_start(start, end) clock_t end, start = clock()
#define emeasure_time_end(start, end) end = clock(), std::cout << std::fixed <<\
	std::setprecision(2) << (start, end) << " ms"

typedef int32_t int_t;

template <typename CharT>
class parser {
	typedef std::variant<size_t, CharT> lit_t;
	struct lit : public lit_t {
		using typename lit_t::variant;
		bool nt() const { return std::holds_alternative<size_t>(*this);}
		size_t n() const { return std::get<size_t>(*this); }
		CharT c() const { return std::get<CharT>(*this); }
		int_t builtin = -1;
		bool is_builtin() const { return builtin != -1; }
	};
	struct item {
		item(size_t set, size_t prod, size_t from, size_t dot) :
			set(set), prod(prod), from(from), dot(dot) {}
		size_t set, prod, from, dot;
		// mutable std::set<item> advancers, completers;
		bool operator<(const item& i) const;
		bool operator==(const item& i) const;
	};
public:
	typedef std::basic_stringstream<CharT> stringstream;
	typedef std::basic_ostream<CharT> ostream;
	typedef std::basic_string<CharT> string;
	typedef std::vector<string> strings;

	typedef std::vector<std::pair<string,
			std::vector<std::vector<string>>>> grammar;
	typedef std::function<bool(CharT)> char_builtin_t;
	typedef std::map<string, char_builtin_t> char_builtins_map;

	typedef std::pair<lit, std::array<size_t, 2>> pnode;
	typedef std::map<item, std::set<std::vector<item>>> item_forest_t;
	typedef std::map<pnode, std::set<std::vector<pnode>>> parse_forest_t;
	typedef parse_forest_t ptree_t;

	typedef std::vector<std::variant<size_t, string>> arg_t;

	typedef std::vector<std::pair<string, const pnode>> children;
	typedef std::vector<children> variations;

	typedef std::function<void(const pnode&, const variations&)> action_fn;
	typedef std::pair<string, action_fn> action_pair;
	typedef std::map<string, action_fn> actions;

	parser(const grammar& g, const char_builtins_map& bm = {},
		bool _bin_lr = false, bool _incr_gen_forest = false);
	parser(const grammar& g, bool _bin_lr = false,
		bool _incr_gen_forest = false)
		: parser(g, {}, _bin_lr, _incr_gen_forest) {}

	bool recognize(const string s);

	// TODO to be removed
	DBG(bool print_ambiguity = false;)
	DBG(bool print_traversing = false;)
	DBG(bool print_optimizations = false;)

	// forest and traversing
	std::vector<arg_t> get_parse_graph_facts();
	size_t count_parsed_trees();
	ptree_t get_parsed_tree();
	template <typename cb_enter_t, typename cb_exit_t,
		typename cb_revisit_t, typename cb_ambig_t>
	bool traverse_forest(const pnode &root, cb_enter_t cb_enter, 
		cb_exit_t cb_exit, cb_revisit_t cb_revisit,cb_ambig_t cb_ambig);
	void topdown(const string& start, const actions& a) const;
	void down(const pnode& nd, const actions& a,
		bool auto_passthrough = true) const;
	void down(const children& nc, const actions& a) const;
	void down(const variations& ncs, const actions& a) const;
	string flatten(const pnode nd) const;
private:
	struct hasher_t {
		size_t hash_size_t(const size_t &val) const;
		size_t operator()(const std::pair<size_t, size_t> &k) const;
		size_t operator()(const pnode &k) const;
		size_t operator()(const item &k) const;
	};
	struct {
		std::map<string, size_t> m;
		std::vector<string> v;
		size_t get(const string& s) {
			if (auto it=m.find(s); it != m.end()) return it->second;
			return m.emplace(s,v.size()),v.push_back(s), v.size()-1;
		}
		string get(size_t n) const { return v[n]; }
	} d;
	std::vector<std::vector<lit>> G;
	std::map<std::vector<lit>, int_t> priority;
	std::set<std::vector<lit>> prefer;
	lit start;
	std::map<lit, std::set<size_t>> nts;
	std::set<size_t> nullables;
	string inputstr;

	typedef std::unordered_set<parser<CharT>::item, parser<CharT>::hasher_t>
		container_t;
	typedef typename container_t::iterator container_iter;
	std::vector<container_t> S;

	container_iter add(container_t& t, const item& i);
	lit get_lit(const item& i) const { return G[i.prod][i.dot]; }
	lit get_nt(const item& i) const { return G[i.prod][0]; }
	bool all_nulls(const std::vector<lit>& p) const;
	bool nullable(const item& i) const;
	void predict(const item& i, container_t& t);
	void scan(const item& i, size_t n, CharT ch);
	void scan_builtin(const item& i, size_t n, const string& s);
	void complete(const item& i, container_t& t);
	bool completed(const item& i) const { return G[i.prod].size()==i.dot; }
	void pre_process(const item &i);

	std::unordered_map<std::pair<size_t, size_t>, std::vector<item>,
		hasher_t> sorted_citem, rsorted_citem;
	parse_forest_t pfgraph;
	std::map<std::vector<parser::lit>, parser::lit> bin_tnt; // binariesed temporary intermediate non-terminals
	size_t tid; // id for temporary non-terminals
	std::vector<char_builtin_t> builtins;
	std::vector<std::map<CharT, size_t>> builtin_char_prod; // char -> prod

	string epsilon() const;

	// forest
	bool bin_lr;  //enables binarizaion and left right optimization
	bool incr_gen_forest; //enables incremental generation of forest
	bool forest();
	bool build_forest (const pnode &root);
	bool build_forest2 (const pnode &root);
	bool bin_lr_comb(const item&, std::set<std::vector<pnode>>&);
	void sbl_chd_forest(const item&, std::vector<pnode>&, size_t,
		std::set<std::vector<pnode>>&);
	template <typename T, typename P = ptree_t>
	bool iterate_forest(T, P&& pt = ptree_t()) const;
	template <typename cb_enter_t, typename cb_exit_t,
		typename cb_revisit_t, typename cb_ambig_t>
	bool _traverse_forest(const pnode &root, std::set<pnode> &done,
		cb_enter_t cb_enter, cb_exit_t cb_exit, cb_revisit_t cb_revisit,
		cb_ambig_t cb_ambig);
	variations get_children(const pnode nd, bool all = false)
		const;
#ifdef DEBUG
	ostream& print(ostream& os, const item& i) const;
	template <typename CharU> friend ostream_t& operator<<(
		ostream_t& os, const typename parser<CharU>::lit& l);
	template <typename CharU> friend ostream_t& operator<<(
		ostream_t& os,
		const std::vector<typename parser<CharU>::lit>& v);
#endif
#ifdef WITH_TO_METHODS
	std::string grammar_text() const;
	std::string dot_safe(const std::string& s) const;
public:
	template<typename P = ptree_t>
	bool to_dot(ostream_t& os, P && p = ptree_t()) const;
	template<typename P = ptree_t>
	bool to_tml_facts(ostream_t& os, P && p = ptree_t()) const;
	template<typename P = ptree_t>
	bool to_tml_rule(ostream_t& os, P && p = ptree_t()) const;
	std::string to_tml_rule(const pnode nd) const;
private:
#endif // WITH_TO_METHODS
};

std::string to_std_string(const std::string& s);
std::string to_std_string(const std::u32string& s);
std::string to_std_string(const char32_t& ch);

template <typename CharT>
std::basic_string<CharT> from_cstr(const char *);

#ifdef DEBUG
template<typename CharT>
ostream_t& print_grammar(ostream_t& os,
	const typename parser<CharT>::grammar &g)
{
	os << "g:\n";
	for (auto x : g)
		for (auto y : x.second) {
			os << '\t' << to_std_string(x.first) << '=';
			for (auto z : y) os << to_std_string(z);
			os << "\n";
		}
	return os;
}
template<typename CharT>
ostream_t& print_dictmap(ostream_t& os,
	const std::map<std::basic_string<CharT>, size_t>& dm)
{
	os << "d.m:\n";
	for (auto x : dm) os <<
		'\t' << to_std_string(x.first) << ' ' << x.second << "\n";
	return os;
}
template <typename CharT>
ostream_t& operator<<(ostream_t& os, const typename parser<CharT>::lit& l) {
	if (l.nt()) return os << l.n();
	else return os << (l.c() == '\0' ? 'e' : l.c());
}
template <typename CharT>
ostream_t& operator<<(ostream_t& os,
	const std::vector<typename parser<CharT>::lit>& v)
{
	int i = 0;
	for (auto &l : v) os << l, i++ == 0 ? os << "->": os <<' ';
	return os;
}
#endif

} // idni namespace
#include "forest.h" // include forest and traversing templates for now (TODO)
#endif
