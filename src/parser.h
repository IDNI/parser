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

#include <variant>
#include <unordered_set>
#include <iostream>
#include <cassert>
#include <sys/mman.h>
#include <sstream>

#include "defs.h"
#include "characters.h"
#include "charclasses.h"
#include "forest.h"

#define DEFAULT_BINARIZE false
#define DEFAULT_INCR_GEN_FOREST false

namespace idni {

template <typename CharT>
struct lit;

// nonterminals dict = vector with index being nonterminal's id
template <typename CharT>
struct nonterminals : public std::vector<std::basic_string<CharT>> {
	size_t get(const std::basic_string<CharT>& s);
	const std::basic_string<CharT>& get(size_t n) const;
	lit<CharT> operator()(const std::basic_string<CharT>& s);
private:
	std::map<std::basic_string<CharT>, size_t> m;
};

// literal containing terminal (c where if c = 0 then null) or nonterminal (n)
template <typename CharT>
struct lit : public std::variant<size_t, CharT> {
	typedef std::variant<size_t, CharT> lit_t;
	using typename lit_t::variant;
	nonterminals<CharT>* nts = 0;
	lit();
	lit(CharT c);
	lit(size_t n, nonterminals<CharT>* nts);
	bool  nt() const;
	size_t n() const;
	CharT  c() const;
	std::basic_string<CharT> to_string(
				const std::basic_string<CharT>& nll={}) const;
	std::string to_std_string(const std::basic_string<CharT>& nll={}) const;
	bool is_null() const;
	bool operator<(const lit& l) const;
	bool operator==(const lit& l) const;
};

template <typename CharT> //  literals which should match the parser item
struct lits : public std::vector<lit<CharT>> { bool neg = false; };
template <typename CharT>
using clause = std::set<lits<CharT>>; // conjunctions of literals
template <typename CharT>
using dnf = std::set<clause<CharT>>; // disjunctions of literal conjunctions
template <typename CharT>
using prod = std::pair<lit<CharT>, dnf<CharT>>; // production rule
template <typename CharT>
struct prods : public std::vector<prod<CharT>> { // productions
	typedef std::vector<prod<CharT>> prods_t;
	prods();
	prods(const lit<CharT>& l);
	prods(const std::basic_string<CharT>& s);
	void operator()(const lit<CharT>& l);
	void operator()(const std::basic_string<CharT>& s);
	void operator()(const prods<CharT>& l, const prods<CharT>& p);
	void operator()(const lit<CharT>& l, const prods<CharT>& p);
	bool operator==(const lit<CharT>& l) const;
	lit<CharT> to_lit() const;
	dnf<CharT> to_dnf() const;
};

// char class functions
template <typename CharT>
using char_class_fn = std::function<bool(CharT)>;
template <typename CharT>
struct char_class_fns {
	std::map<size_t, char_class_fn<CharT>> fns = {};
	std::map<size_t, std::map<CharT, size_t>> ps = {}; //char -> production
	// adds new char class function
	void operator()(size_t nt, const char_class_fn<CharT>& fn);
	// returns true if a nt is a cc function
	bool is_fn(size_t nt) const;
};
template <typename CharT>
char_class_fns<CharT> predefined_char_classes(
	const std::vector<std::basic_string<CharT>>& cc_fn_names,
	nonterminals<CharT>& nts);

// grammar struct required by parser.
// accepts nonterminals ref, prods and char class functions
template <typename CharT> struct parser;
template <typename CharT> struct grammar_inspector;
template <typename CharT>
struct grammar {
	friend parser<CharT>;
	friend grammar_inspector<CharT>;
	typedef std::pair<lit<CharT>, std::vector<lits<CharT>>> production;

	grammar(nonterminals<CharT>& nts) : nts(nts) {}
	grammar(nonterminals<CharT>& nts, const prods<CharT>& ps,
		const prods<CharT>& start, const char_class_fns<CharT>& cc_fns);
	// returns number of productions (every disjunction has a prod rule)
	size_t size() const;
	// returns head of the prod rule - literal
	lit<CharT> operator()(const size_t& i) const;
	// returns body of the prod rule - conjunctions of literals
	const std::vector<lits<CharT>>& operator[](const size_t& i) const;
	// returns length of the conjunction
	size_t len(const size_t& p, const size_t& c) const;
	// returns true if the literal is nullable
	bool nullable(lit<CharT> l) const;
	// returns true if the production has more then 1 conjunction
	bool conjunctive(size_t p) const;
	// checks if the char class function returns true on the char ch
	bool char_class_check(lit<CharT> l, CharT ch) const;
	// does the same check as char_class_check and if the check is true then
	// it adds new rule: "cc_fn_name => ch." into the grammar
	// returns id of the rule or (size_t)-1 if the check fails
	size_t get_char_class_production(lit<CharT> l, CharT ch);
#if defined(DEBUG) || defined(WITH_DEVHELPERS)
	std::ostream& print_internal_grammar(std::ostream& os,
		std::string prep = {}) const;
	std::ostream& print_data(std::ostream& os, std::string prep = {}) const;
#endif
private:
	nonterminals<CharT>& nts;
	lit<CharT> start;
	char_class_fns<CharT> cc_fns = {};
	std::map<lit<CharT>, std::set<size_t>> ntsm = {};
	std::set<size_t> nullables = {};
	std::set<size_t> conjunctives = {};
	std::vector<production> G;
	lit<CharT> nt(size_t n);
	lit<CharT> nt(const std::basic_string<CharT>& s);
	bool all_nulls(const lits<CharT>& a) const;
};

template <typename CharT>
class parser {
public:
	typedef CharT char_t;
	typedef typename std::pair<lit<CharT>, std::array<size_t, 2>> pnode;
	struct options {
		bool binarize = DEFAULT_BINARIZE;
		bool incr_gen_forest = DEFAULT_INCR_GEN_FOREST;
	};
	struct item {
		item(size_t set, size_t prod, size_t con, size_t from, size_t dot) :
			set(set), prod(prod), con(con), from(from), dot(dot) {}
		size_t set, prod, con, from, dot;
		bool operator<(const item& i) const;
		bool operator==(const item& i) const;
	};
	typedef forest<pnode> pforest;
	typedef pforest::nodes pnodes;
	typedef pforest::nodes_set pnodes_set;
	typedef pforest::node_graph pnode_graph;
	typedef pforest::graph pgraph;
	typedef pforest::tree ptree;
	typedef pforest::sptree psptree;
	typedef std::map<std::pair<size_t, size_t>,std::set<item>> conjunctions;
	parser(grammar<CharT>& g, const options& o = {});
	parser(const std::basic_string<CharT>& s, grammar<CharT>& g,
						const options& o = {});
	parser(const std::basic_string<CharT>& s, const options& o = {});
	~parser() { if (d!=0 && reads_mmap) munmap(const_cast<CharT*>(d), l); }
	std::unique_ptr<pforest> parse(const CharT* data, size_t size = 0,
		CharT eof = std::char_traits<CharT>::eof());
	std::unique_ptr<pforest> parse(int filedescriptor, size_t size = 0,
		CharT eof = std::char_traits<CharT>::eof());
	std::unique_ptr<pforest> parse(std::basic_istream<CharT>& is,
		size_t size = 0, CharT eof = std::char_traits<CharT>::eof());
	bool found();
#if defined(DEBUG) || defined(WITH_DEVHELPERS)
	std::ostream& print(std::ostream& os, const item& i) const;
	std::ostream& print_data(std::ostream& os) const;
	std::ostream& print_S(std::ostream& os) const;
#endif
	struct hasher_t {
		size_t hash_size_t(const size_t &val) const;
		size_t operator()(const std::pair<size_t, size_t> &k) const;
		size_t operator()(const pnode &k) const;
		size_t operator()(const item &k) const;
	};
	struct perror_t {
		enum info_lvl{
			INFO_BASIC,
			INFO_DETAILED,
			INFO_ROOT_CAUSE
		};
		int_t loc;	// location of error
		size_t line;    // line of error
		size_t col;     // column of error
		std::string ctxt; // closest matching ctxt
		std::string unexp; // unexpected character
		typedef struct _exp_prod {
			std::string exp;
			std::string prod_nt;
			std::string prod_body;
			// back track information to higher derivations
			std::vector<_exp_prod> bktrk;
		}exp_prod_t;

		// list of expected token and respective productions
		std::vector<exp_prod_t> expv;
		perror_t() : loc(-1) {}
		std::string to_str(info_lvl lv = INFO_ROOT_CAUSE);
	};
	perror_t get_error();
	std::vector<item> back_track(const item& obj);

private:
	typedef std::unordered_set<parser<CharT>::item, parser<CharT>::hasher_t>
			container_t;
	typedef typename container_t::iterator container_iter;
	grammar<CharT>& g;
	const CharT* d = 0;           // input data (for file)
	size_t max_length = 0;        // read up to max length of the input size
	size_t l = 0;                 // input size
	CharT e;                      // end of file (input) character
	size_t n = 0;                 // current input position
	std::basic_istream<CharT>* s; // input stream
	bool reads_stream = false;    // true if stream input
	bool reads_mmap   = false;    // true if file input
	options o;
	std::vector<container_t> S;
	std::unordered_map<std::pair<size_t, size_t>, std::vector<item>,
		hasher_t> sorted_citem, rsorted_citem;
	std::map<std::vector<lit<CharT>>, lit<CharT>> bin_tnt; // binariesed temporary intermediate non-terminals
	size_t tid; // id for temporary non-terminals
	CharT cur() const;
	bool next();
	CharT at(size_t p);
	lit<CharT> get_lit(const item& i) const;
	lit<CharT> get_nt(const item& i) const;
	container_iter add(container_t& t, const item& i);
	bool nullable(const item& i) const;
	void resolve_conjunctions(container_t& t) const;
	void predict(const item& i, container_t& t);
	void scan(const item& i, size_t n, CharT ch);
	void scan_cc_function(const item& i, size_t n, CharT ch);
	void complete(const item& i, container_t& t);
	bool completed(const item& i) const;
	void pre_process(const item &i);
	bool init_forest(pforest& f);
	bool build_forest(pforest& f, const pnode &root);
	bool binarize_comb(const item&, std::set<std::vector<pnode>>&);
	void sbl_chd_forest(const item&,
		std::vector<pnode>&, size_t, std::set<std::vector<pnode>>&);
	std::unique_ptr<pforest> _parse();
#ifdef DEBUG
	template <typename CharU>
	friend std::ostream& operator<<(std::ostream& os, lit<CharT>& l);
	template <typename CharU>
	friend std::ostream& operator<<(std::ostream& os, const std::vector<lit<CharT>>& v);
#endif
};

template <typename CharT> // flattens terminals of a subforest into an ostream
std::basic_ostream<CharT>& terminals_to_stream(std::basic_ostream<CharT>& os,
	const typename parser<CharT>::pforest& f,
	const typename parser<CharT>::pnode& r);
template <typename CharT> // flattens terminals of a subforest into a string
std::basic_string<CharT> terminals_to_str(
	const typename parser<CharT>::pforest& f,
	const typename parser<CharT>::pnode& r);
template <typename CharT> // flattens terminals of a subforest into an int
int_t terminals_to_int(const typename parser<CharT>::pforest& f,
	const typename parser<CharT>::pnode& r);

template <typename CharT>
bool operator==(const lit<CharT>& l, const prods<CharT>& p);
template <typename CharT>
prods<CharT> operator~(const prods<CharT>&);
template <typename CharT>
prods<CharT> operator&(const prods<CharT>&, const prods<CharT>&);
template <typename CharT>
prods<CharT> operator&(const prods<CharT>& x, const CharT& c);
template <typename CharT>
prods<CharT> operator&(const prods<CharT>& x, const CharT* c);
template <typename CharT>
prods<CharT> operator&(const prods<CharT>& x, const std::basic_string<CharT>& s);
prods<char32_t> operator&(const prods<char32_t>& x, const std::basic_string<char>& s);
template <typename CharT>
prods<CharT> operator|(const prods<CharT>&, const prods<CharT>&);
template <typename CharT>
prods<CharT> operator|(const prods<CharT>& x, const CharT& c);
template <typename CharT>
prods<CharT> operator|(const prods<CharT>& x, const CharT* s);
template <typename CharT>
prods<CharT> operator|(const prods<CharT>& x,const std::basic_string<CharT>& s);
prods<char32_t> operator|(const prods<char32_t>& x,
	const std::basic_string<char>& s);
template <typename CharT>
prods<CharT> operator+(const prods<CharT>& x, const CharT& c);
template <typename CharT>
prods<CharT> operator+(const prods<CharT>& x, const CharT* s);
template <typename CharT>
prods<CharT> operator+(const prods<CharT>& x,const std::basic_string<CharT>& s);
prods<char32_t> operator+(const prods<char32_t>& x,
	const std::basic_string<char>& s);

#if defined(DEBUG) || defined(WITH_DEVHELPERS)
template<typename CharT>
std::ostream& print_grammar(std::ostream& os, const grammar<CharT>& g);
template<typename CharT>
std::ostream& print_dictmap(std::ostream& os,
	const std::map<typename parser<CharT>::string, size_t>& dm);
#endif

} // idni namespace
#include "parser.tmpl.h"  // template definitions
#include "tgf.h"          // Tau Grammar Form
#ifdef WITH_DEVHELPERS
#include "devhelpers.h"   // various helpers for converting forest
#endif
// undef local macros
#undef DEFAULT_BINARIZE
#undef DEFAULT_INCR_GEN_FOREST
#endif
