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
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <span>
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

template <typename C, typename T>
struct lit;

// nonterminals dict = vector with index being nonterminal's id
template <typename C = char, typename T = C>
struct nonterminals : public std::vector<std::basic_string<C>> {
	size_t get(const std::basic_string<C>& s);
	const std::basic_string<C>& get(size_t n) const;
	lit<C, T> operator()(const std::basic_string<C>& s);
private:
	std::map<std::basic_string<C>, size_t> m;
};

// literal containing terminal (c where if c = 0 then null) or nonterminal (n)
template <typename C = char, typename T = C>
struct lit : public std::variant<size_t, T> {
	typedef std::variant<size_t, T> lit_t;
	using typename lit_t::variant;
	nonterminals<C, T>* nts = 0;
	bool is_null_ = false;
	lit();
	lit(T t);
	lit(size_t n, nonterminals<C, T>* nts);
	bool  nt() const;
	size_t n() const;
	T      t() const;
	bool is_null() const;
	bool operator<(const lit<C, T>& l) const;
	bool operator==(const lit<C, T>& l) const;
	//std::ostream& operator<<(std::ostream& os)
	std::vector<T> to_terminals() const;
	std::basic_string<C> to_string(const std::basic_string<C>& nll={})const;
	std::string to_std_string(const std::basic_string<C>& nll = {}) const;
};

// production rules in a disjunctive normal form of conjunction clauses of lits
template <typename C = char, typename T = C> //  literals
struct lits : public std::vector<lit<C, T>> { bool neg = false; };
template <typename C = char, typename T = C>
using conjs = std::set<lits<C, T>>; // conjunctions of literals
template <typename C = char, typename T = C>
using disjs = std::set<conjs<C, T>>; // disjunctions of literal conjunctions
template <typename C = char, typename T = C>
using prod = std::pair<lit<C, T>, disjs<C, T>>; // production rule
template <typename C = char, typename T = C>
struct prods : public std::vector<prod<C, T>> { // productions
	typedef std::vector<prod<C, T>> prods_t;
	prods();
	prods(const lit<C, T>& l);
	prods(const std::basic_string<C>& s);
	prods(const std::vector<T>& v);
	void operator()(const lit<C, T>& l);
	void operator()(const std::basic_string<C>& s);
	void operator()(const std::vector<T>& v);
	void operator()(const prods<C, T>& l, const prods<C, T>& p);
	void operator()(const lit<C, T>& l, const prods<C, T>& p);
	bool operator==(const lit<C, T>& l) const;
	lit<C, T> to_lit() const;
	disjs<C, T> to_disjs() const;
};

// char class functions
template <typename T = char>
using char_class_fn = std::function<bool(T)>;
template <typename T = char>
struct char_class_fns {
	std::map<size_t, char_class_fn<T>> fns = {};
	std::map<size_t, std::map<T, size_t>> ps = {}; //char -> production
	// adds new char class function
	void operator()(size_t nt, const char_class_fn<T>& fn);
	// returns true if a nt is a cc function
	bool is_fn(size_t nt) const;
	bool is_eof_fn(size_t nt) const; // returns true if a nt is an eof fn
	size_t eof_fn = -1; // id of an eof fn
};
template <typename C = char, typename T = C>
char_class_fns<T> predefined_char_classes(
	const std::vector<std::string>& cc_fn_names, nonterminals<C, T>& nts);

// grammar struct required by parser.
// accepts nonterminals ref, prods and char class functions
template <typename C, typename T> struct grammar_inspector;
template <typename C = char, typename T = C>
struct grammar {
	friend struct grammar_inspector<C, T>;
	typedef std::pair<lit<C, T>, std::vector<lits<C, T>>> production;
	grammar(nonterminals<C, T>& nts) : nts(nts) {}
	grammar(nonterminals<C, T>& nts, const prods<C, T>& ps,
		const prods<C, T>& start, const char_class_fns<T>& cc_fns);
	// returns number of productions (every disjunction has a prod rule)
	size_t size() const;
	// returns head of the prod rule - literal
	lit<C, T> operator()(const size_t& i) const;
	// returns body of the prod rule - conjunctions of literals
	const std::vector<lits<C, T>>& operator[](const size_t& i) const;
	// returns length of the conjunction
	size_t len(const size_t& p, const size_t& c) const;
	// returns true if the literal is nullable
	bool nullable(lit<C, T> l) const;
	// returns true if the production has more then 1 conjunction
	bool conjunctive(size_t p) const;
	// checks if the char class function returns true on the char ch
	bool char_class_check(lit<C, T> l, T ch) const;
	// does the same check as char_class_check and if the check is true then
	// it adds new rule: "cc_fn_name => ch." into the grammar
	// returns id of the rule or (size_t)-1 if the check fails
	size_t get_char_class_production(lit<C, T> l, T ch);
	size_t add_char_class_production(lit<C, T> l, T ch);
	const std::set<size_t>& prod_ids_of_literal(const lit<C, T>& l);
	const lit<C, T>& start_literal() const;
	bool is_cc_fn(const size_t& p) const;
	bool is_eof_fn(const size_t& p) const;
	std::ostream& print_production(std::ostream& os,
		const production& p) const;
#if defined(DEBUG) || defined(WITH_DEVHELPERS)
	std::ostream& print_internal_grammar(std::ostream& os,
		std::string prep = {}) const;
	std::ostream& print_data(std::ostream& os, std::string prep = {}) const;
#endif
	lit<C, T> nt(size_t n);
	lit<C, T> nt(const std::basic_string<C>& s);
private:
	bool all_nulls(const lits<C, T>& a) const;
	nonterminals<C, T>& nts;
	lit<C, T> start;
	char_class_fns<T> cc_fns = {};
	std::map<lit<C, T>, std::set<size_t>> ntsm = {};
	std::set<size_t> nullables = {};
	std::set<size_t> conjunctives = {};
	std::vector<production> G;
};

template <typename C = char, typename T = C>
class parser {
public:
	struct input;
	using char_type     = C;
	using terminal_type = T;
	using traits_type   = std::char_traits<C>;
	using int_type      = typename traits_type::int_type;
	using parser_type   = parser<C, T>;
	using input_type    = parser_type::input;
	using decoder_type  = parser_type::input::decoder_type;
	using encoder_type  = std::function<
				std::basic_string<C>(const std::vector<T>&)>;
	typedef typename std::pair<lit<C, T>, std::array<size_t, 2>> pnode;
	struct options {
		// applying binarization to ensure every forest node
		// has atmost 2 or less children nodes
		bool binarize = DEFAULT_BINARIZE;
		// build forest incrementally as soon any
		// item is completed
		bool incr_gen_forest = DEFAULT_INCR_GEN_FOREST;
		// enable garbage collection during parsing
		bool enable_gc = false;
		// number of steps garbage collection lags behind
		// parsing position n. should be greater than
		// 0 and less than the size of the input
		// for any collection activity. We cannot use
		// % since in the streaming case, we do not know
		// exact size in advance
		size_t gc_lag = 1;
		decoder_type chars_to_terminals = 0;
		encoder_type terminals_to_chars = 0;
	};
	struct item {
		item(size_t set, size_t prod,size_t con,size_t from,size_t dot);
		bool operator<(const item& i) const;
		bool operator==(const item& i) const;
		size_t set, prod, con, from, dot;
	};
	struct input {
		using decoder_type =
				std::function<std::vector<T>(input&)>;
		input(const C* data, size_t length = 0,
			decoder_type decoder = 0,
			int_type e = std::char_traits<C>::eof());
		input(std::basic_istream<C>& is, size_t length = 0,
			decoder_type decoder = 0,
			int_type e = std::char_traits<C>::eof());
		input(int filedescriptor, size_t length = 0,
			decoder_type decoder = 0,
			int_type e = std::char_traits<C>::eof());
		~input();
		inline bool isstream() const;
		void clear(); // resets stream (if used) to reenable at()/tat()
		// source stream access
		C cur();
		size_t pos();
		bool next();
		bool eof();
		C at(size_t p); // reads value at pos (uses seek, if stream)
		// transformed stream access
		T tcur();
		size_t tpos();
		bool tnext();
		bool teof();
		T tat(size_t p); // reads value at tpos
	private:
		void decode();
		enum type { POINTER, STREAM, MMAP } itype = POINTER;//input type
		int_type e = std::char_traits<C>::eof(); // end of a stream
		decoder_type decoder = 0;
		size_t max_l = 0;   // read up to max length of the input size
		size_t n = 0;         // input position
		size_t l = 0;         // input length
		const C* d = 0;       // input data pointer if needed
		std::basic_istream<C> s{};
		std::vector<T> ts{};  // all collected terminals
		size_t tp = 0;        // current terminal pos
//		C c = e;
	};
	typedef forest<pnode> pforest;
	typedef pforest::nodes pnodes;
	typedef pforest::nodes_set pnodes_set;
	typedef pforest::node_graph pnode_graph;
	typedef pforest::graph pgraph;
	typedef pforest::tree ptree;
	typedef pforest::sptree psptree;
	typedef std::map<std::pair<size_t, size_t>,std::set<item>> conjunctions;
	parser(grammar<C, T>& g, const options& o = {});
	std::unique_ptr<pforest> parse(const C* data, size_t size = 0,
		int_type eof = std::char_traits<C>::eof());
	std::unique_ptr<pforest> parse(int filedescriptor, size_t size = 0,
		int_type eof = std::char_traits<C>::eof());
	std::unique_ptr<pforest> parse(std::basic_istream<C>& is,
		size_t size = 0, int_type eof = std::char_traits<C>::eof());
	bool found();
#if defined(DEBUG) || defined(WITH_DEVHELPERS)
	std::ostream& print(std::ostream& os, const item& i) const;
	std::ostream& print_data(std::ostream& os) const;
	std::ostream& print_S(std::ostream& os) const;
#endif
	struct perror_t {
		enum info_lvl {
			INFO_BASIC,
			INFO_DETAILED,
			INFO_ROOT_CAUSE
		};
		int_t loc;	     // location of error
		size_t line;         // line of error
		size_t col;          // column of error
		std::vector<T> ctxt; // closest matching ctxt
		T unexp;             // unexpected token
		typedef struct _exp_prod {
			std::string exp;
			std::string prod_nt;
			std::string prod_body;
			// back track information to higher derivations
			std::vector<_exp_prod> bktrk;
		} exp_prod_t;

		// list of expected token and respective productions
		std::vector<exp_prod_t> expv;
		perror_t() : loc(-1) {}
		std::string to_str(info_lvl lv = INFO_ROOT_CAUSE);
	};
	perror_t get_error();
	
	static std::basic_string<C> tnt_prefix() {
		static std::basic_string<C> pr = { '_','_','t','e','m','p' };
		return pr; }
private:
	std::vector<item> back_track(const item& obj);
	typedef std::set<item> container_t;
	typedef typename container_t::iterator container_iter;
	grammar<C, T>& g;
	options o;
	std::unique_ptr<input> in = 0;
	std::vector<container_t> S;
	//mapping from to position of end in S for items
	std::unordered_map<size_t, std::vector<size_t>> fromS;
	
	// refcounter for the earley item
	// default value is 0, which means it can be garbaged
	// non-zero implies, its not to be collected
	std::map<item, int_t> refi;
	// items ready for collection
	std::set<item> gcready;
	std::map<std::pair<size_t, size_t>, std::vector<const item*> > 
		sorted_citem, rsorted_citem;
	/*
	std::map<std::pair<size_t, size_t>, 
									std::vector<item>> memo;
	std::map<std::pair<size_t, size_t>, 
									std::vector<item>> rmemo;
	*/

	// binarized temporary intermediate non-terminals
	std::map<std::vector<lit<C, T>>, lit<C, T>> bin_tnt;
	size_t tid; // id for temporary non-terminals
	std::basic_string<C> get_fresh_tnt() {
		std::basic_stringstream<C> ss;
		ss << tnt_prefix() << tid++;
		return ss.str();
	}
	lit<C, T> get_lit(const item& i) const;
	lit<C, T> get_nt(const item& i) const;
	std::pair<container_iter, bool> add(container_t& t, const item& i);
	bool nullable(const item& i) const;
	void resolve_conjunctions(container_t& t) const;
	void predict(const item& i, container_t& t);
	void scan(const item& i, size_t n, T ch);
	void scan_cc_function(const item& i, size_t n, T ch, container_t& c);
	void complete(const item& i, container_t& t);
	bool completed(const item& i) const;
	void pre_process(const item& i);
	bool init_forest(pforest& f);
	bool build_forest(pforest& f, const pnode& root);
	bool binarize_comb(const item&, std::set<std::vector<pnode>>&);
	void sbl_chd_forest(const item&,
		std::vector<pnode>&, size_t, std::set<std::vector<pnode>>&);
	std::unique_ptr<pforest> _parse();
#ifdef DEBUG
	template <typename CharU>
	friend std::ostream& operator<<(std::ostream& os, lit<C, T>& l);
	template <typename CharU>
	friend std::ostream& operator<<(std::ostream& os,
		const std::vector<lit<C, T>>& v);
#endif
};

template <typename C = char, typename T = C> // flattens terminals of a subforest into an ostream
std::basic_ostream<T>& terminals_to_stream(std::basic_ostream<T>& os,
	const typename parser<C, T>::pforest& f,
	const typename parser<C, T>::pnode& r);
template <typename C = char, typename T = C> // flattens terminals of a subforest into a string
std::basic_string<T> terminals_to_str(
	const typename parser<C, T>::pforest& f,
	const typename parser<C, T>::pnode& r);
template <typename C = char, typename T = C> // flattens terminals of a subforest into an int
int_t terminals_to_int(const typename parser<C, T>::pforest& f,
	const typename parser<C, T>::pnode& r);

template <typename C, typename T>
bool operator==(const lit<C, T>& l, const prods<C, T>& p);
template <typename C, typename T>
prods<C, T> operator~(const prods<C, T>&);
template <typename C, typename T>
prods<C, T> operator&(const prods<C, T>&, const prods<C, T>&);
template <typename C, typename T>
prods<C, T> operator&(const prods<C, T>& x, const T& c);
template <typename C, typename T>
prods<C, T> operator&(const prods<C, T>& x, const T* c);
template <typename C, typename T>
prods<C, T> operator&(const prods<C, T>& x, const std::basic_string<C>& s);
prods<char32_t> operator&(const prods<char32_t>& x,
					const std::basic_string<char>& s);
template <typename C, typename T>
prods<C, T> operator|(const prods<C, T>&, const prods<C, T>&);
template <typename C, typename T>
prods<C, T> operator|(const prods<C, T>& x, const C& c);
template <typename C, typename T>
prods<C, T> operator|(const prods<C, T>& x, const C* s);
template <typename C, typename T>
prods<C, T> operator|(const prods<C, T>& x, const std::basic_string<C>& s);
prods<char32_t> operator|(const prods<char32_t>& x,
	const std::basic_string<char>& s);
template <typename C, typename T>
prods<C, T> operator+(const prods<C, T>& x, const T& c);
template <typename C, typename T>
prods<C, T> operator+(const prods<C, T>& x, const T* s);
template <typename C, typename T>
prods<C, T> operator+(const prods<C, T>& x,const std::basic_string<C>& s);
prods<char32_t> operator+(const prods<char32_t>& x,
	const std::basic_string<char>& s);

#if defined(DEBUG) || defined(WITH_DEVHELPERS)
template<typename C = char, typename T = C>
std::ostream& print_grammar(std::ostream& os, const grammar<C, T>& g);
template<typename C>
std::ostream& print_dictmap(std::ostream& os,
	const std::map<std::basic_string<C>, size_t>& dm);
#endif

} // idni namespace
#include "grammar.tmpl.h" // template definitions for grammar and related
#include "parser.tmpl.h"  // template definitions for parser
#include "tgf.h"          // Tau Grammar Form
#ifdef WITH_DEVHELPERS
#include "devhelpers.h"   // various helpers for converting forest
#endif
// undef local macros
#undef DEFAULT_BINARIZE
#undef DEFAULT_INCR_GEN_FOREST
#endif // __IDNI__PARSER__PARSER_H__
