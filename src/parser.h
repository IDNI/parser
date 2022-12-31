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
#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include <iomanip>
#include "defs.h"
#include "grammar.h"
#include "forest.h"
namespace idni {

template <typename CharT>
class parser {
public:
	typedef std::basic_stringstream<CharT> stringstream;
	typedef std::basic_ostream<CharT> ostream;
	typedef std::basic_string<CharT> string;
	typedef std::vector<string> strings;
	struct options {
		bool bin_lr = DEFAULT_BIN_LR;
		bool incr_gen_forest = DEFAULT_INCR_GEN_FOREST;
	};
	struct item {
		item(size_t set, size_t prod, size_t con, size_t from, size_t dot) :
			set(set), prod(prod), con(con), from(from), dot(dot) {}
		size_t set, prod, con, from, dot;
		bool operator<(const item& i) const;
		bool operator==(const item& i) const;
	};
	typedef typename std::pair<lit<CharT>, std::array<size_t, 2>> pnode;
	typedef forest<pnode> pforest;
	typedef pforest::nodes pnodes;
	typedef pforest::nodes_set pnodes_set;
	typedef pforest::node_graph pnode_graph;
	typedef pforest::graph pgraph;
	typedef pforest::tree ptree;
	typedef pforest::sptree psptree;

	parser(grammar<CharT>& g, const options& o = {});
	parser(const string& s, grammar<CharT>& g, const options& o = {});
	parser(const string& s, const options& o = {});
	std::unique_ptr<pforest> parse(const string& s);
	std::unique_ptr<pforest> parse(const string& s, bool& found);
#if defined(DEBUG) || defined(WITH_DEVHELPERS)
	ostream_t& print(ostream_t& os, const item& i) const;
	ostream_t& print_data(ostream_t& os) const;
	ostream_t& print_S(ostream_t& os) const;
#endif
	struct hasher_t {
		size_t hash_size_t(const size_t &val) const;
		size_t operator()(const std::pair<size_t, size_t> &k) const;
		size_t operator()(const pnode &k) const;
		size_t operator()(const item &k) const;
	};
private:
	typedef std::unordered_set<parser<CharT>::item, parser<CharT>::hasher_t> container_t;
	typedef typename container_t::iterator container_iter;
	grammar<CharT>& g;
	string inputstr;
	options o;
	std::vector<container_t> S;
	std::unordered_map<std::pair<size_t, size_t>, std::vector<item>,
		hasher_t> sorted_citem, rsorted_citem;
	std::map<std::vector<lit<CharT>>, lit<CharT>> bin_tnt; // binariesed temporary intermediate non-terminals
	size_t tid; // id for temporary non-terminals
	container_iter add(container_t& t, const item& i);
	lit<CharT> get_lit(const item& i) const {
		return g[i.prod][i.con][i.dot];
	}
	lit<CharT> get_nt(const item& i) const { return g(i.prod); }
	bool nullable(const item& i) const;
	void uncomplete_if_not_conj(container_t& t);
	void predict(const item& i, container_t& t);
	void scan(const item& i, size_t n, CharT ch);
	void scan_cc_function(const item& i, size_t n, const string& s);
	void complete(const item& i, container_t& t);
	bool completed(const item& i) const;
	void pre_process(const item &i);
	bool init_forest(pforest& f);
	bool build_forest(pforest& f, const pnode &root);
	bool bin_lr_comb(const item&, std::set<std::vector<pnode>>&);
	void sbl_chd_forest(const item&,
		std::vector<pnode>&, size_t, std::set<std::vector<pnode>>&);
#ifdef DEBUG
	template <typename CharU>
	friend ostream_t& operator<<(ostream_t& os, lit<CharT>& l);
	template <typename CharU>
	friend ostream_t& operator<<(ostream_t& os, const std::vector<lit<CharT>>& v);
#endif
};

template <typename CharT>
typename parser<CharT>::ostream& flatten(typename parser<CharT>::ostream& os,
	const typename parser<CharT>::pforest& f,
	const typename parser<CharT>::pnode& r);
template <typename CharT>
typename parser<CharT>::string flatten(const typename parser<CharT>::pforest& f,
	const typename parser<CharT>::pnode& r);

#if defined(DEBUG) || defined(WITH_DEVHELPERS)
template<typename CharT>
ostream_t& print_grammar(ostream_t& os, const grammar<CharT>& g);
template<typename CharT>
ostream_t& print_dictmap(ostream_t& os,
	const std::map<typename parser<CharT>::string, size_t>& dm);
#endif

} // idni namespace
#include "parser.tmpl.h"
#ifdef WITH_DEVHELPERS
#include "devhelpers.h"
#endif
// undefs to not polute
#undef DEFAULT_BIN_LR
#undef DEFAULT_INCR_GEN_FOREST
#endif
