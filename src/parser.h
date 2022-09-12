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
#ifndef __IDNI__PARSER__PARSER_H__
#define __IDNI__PARSER__PARSER_H__
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <variant>
#include <functional>
#include <iomanip>
#include <iostream>
#include "characters.h"
#include "forest.h"
namespace idni {

#define DEFAULT_BIN_LR false
#define DEFAULT_INCR_GEN_FOREST false
#define DEFAULT_STARTING_RULE "start"
#ifdef WITH_CHARCLASSES
#define DEFAULT_CHAR_CLASS_FNS { "eof", "alnum", "alpha", "blank", "cntrl", \
	"digit", "graph", "lower", "printable", "punct", "space", "upper", \
	"xdigit" }
#endif

#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif

#ifdef MEASURE
#define MS(x) x
#else
#define MS(x)
#endif

typedef std::basic_ostream<char> ostream_t;
typedef int32_t int_t;

template <typename CharT>
typename std::basic_string<CharT> from_cstr(const char *);
template <typename CharT>
typename std::basic_string<CharT> from_str(const std::string&);

template <typename CharT>
class parser {
	typedef std::variant<size_t, CharT> lit_t;
public:
	typedef std::basic_stringstream<CharT> stringstream;
	typedef std::basic_ostream<CharT> ostream;
	typedef std::basic_string<CharT> string;
	typedef std::vector<string> strings;

	typedef std::pair<string, std::vector<strings>> grammar_rule;
	typedef std::vector<grammar_rule> grammar;
#ifdef WITH_CHARCLASSES
	typedef std::function<bool(CharT)> char_class_fn;
	typedef std::map<string, char_class_fn> char_class_fns_map;
#endif
	struct parser_options {
		bool bin_lr = DEFAULT_BIN_LR,
			incr_gen_forest = DEFAULT_INCR_GEN_FOREST;
		string start = from_cstr<CharT>(DEFAULT_STARTING_RULE);
#ifdef WITH_CHARCLASSES
		std::vector<std::string> cc_fns = DEFAULT_CHAR_CLASS_FNS;
		char_class_fns_map cc_fns_map = {};
#endif
	};
	struct lit : public lit_t {
		using typename lit_t::variant;
		bool nt() const { return std::holds_alternative<size_t>(*this);}
		size_t n() const { return std::get<size_t>(*this); }
		CharT c() const { return std::get<CharT>(*this); }
	};
	struct item {
		item(size_t set, size_t prod, size_t from, size_t dot) :
			set(set), prod(prod), from(from), dot(dot) {}
		size_t set, prod, from, dot;
		// mutable std::set<item> advancers, completers;
		bool operator<(const item& i) const;
		bool operator==(const item& i) const;
	};
	typedef std::pair<lit, std::array<size_t, 2>> pnode;
	typedef forest<pnode> parse_forest;
	typedef parse_forest::nodes pnodes;
	typedef parse_forest::nodes_set pnodes_set;
	typedef parse_forest::node_graph pnode_graph;

	parser(const grammar& g, const parser_options& o = {});
	bool recognize(const string s);
	pnode root() const { return pnode(start, { 0, inputstr.length() }); }
	const strings& dict() { return d.v; }
	const string& dict(int n) { return d.v[n]; }
	string to_string(const lit& l) const;
	parse_forest parsed_forest() const { return f; }
	void parsed_forest(parse_forest pf) { f = pf; };
	ostream& flatten(ostream& os, const pnode& r);
	string flatten(const pnode& r);
#ifdef DEBUG
	ostream_t& print_data(ostream_t& os) const;
	ostream_t& print_grammar(ostream_t& os, const grammar &g) const;
	ostream_t& print_dictmap(ostream_t& os,
		const std::map<string, size_t>& dm) const;
	ostream_t& print(ostream_t& os, const item& i) const;
#endif
private:
	parser_options o;
	struct hasher_t {
		size_t hash_size_t(const size_t &val) const;
		size_t operator()(const std::pair<size_t, size_t> &k) const;
		size_t operator()(const pnode &k) const;
		size_t operator()(const item &k) const;
	};
	struct {
		std::map<string, size_t> m;
		strings v;
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
	void complete(const item& i, container_t& t);
	bool completed(const item& i) const { return G[i.prod].size()==i.dot; }
	void pre_process(const item &i);
	std::unordered_map<std::pair<size_t, size_t>, std::vector<item>,
		hasher_t> sorted_citem, rsorted_citem;
	std::map<std::vector<lit>, lit> bin_tnt; // binariesed temporary intermediate non-terminals
	size_t tid; // id for temporary non-terminals
#ifdef WITH_CHARCLASSES
	bool add_cc_fns(char_class_fns_map& ccm,
		const std::vector<std::string>& cc) const;
	void scan_cc_function(const item& i, size_t n, const string& s);
	std::vector<char_class_fn> cc_fns;
	std::vector<std::map<CharT, size_t>> cc_fns_prod; // char -> cc_fn prod
#endif
	parse_forest f; // forest
	bool init_forest();
	bool build_forest(const pnode &root);
	bool build_forest2(const pnode &root);
	bool bin_lr_comb(const item&, std::set<std::vector<pnode>>&);
	void sbl_chd_forest(const item&, std::vector<pnode>&, size_t,
		std::set<std::vector<pnode>>&);
	parse_forest resolve_priorities(parse_forest) const;
	template <typename CharU>
	friend ostream_t& operator<<(ostream_t& os, lit& l);
	template <typename CharU>
	friend ostream_t& operator<<(ostream_t& os, const std::vector<lit>& v);
#ifdef WITH_TO_METHODS
	std::string grammar_text() const;
	std::string dot_safe(const std::string& s) const;
public:
	bool to_tml_facts(ostream_t& os) const { return to_tml_facts(os, f); };
	bool to_tml_facts(ostream_t& os, const parse_forest& frst) const;
	template<typename P = pnode_graph>
	bool to_dot(ostream_t& os, P && p = pnode_graph()) const;
	template<typename P = pnode_graph>
	bool to_tml_rules(ostream_t& os, P && p = pnode_graph()) const;
	std::string to_tml_rule(const pnode nd) const;
private:
#endif // WITH_TO_METHODS
};

std::string to_std_string(const std::string& s);
std::string to_std_string(const std::u32string& s);
std::string to_std_string(const char32_t& ch);

template <typename CharT>
ostream_t& operator<<(ostream_t& os, typename parser<CharT>::lit& l);
template <typename CharT>
ostream_t& operator<<(ostream_t& os,
	const std::vector<typename parser<CharT>::lit>& v);

} // idni namespace
#include "parser.tmpl.h"
#endif
