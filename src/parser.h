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
#include <array>
#include <variant>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <istream>
#include <span>
#include <cassert>
#include <algorithm>
#include "memory_map.h"
#include "defs.h"
#include "characters.h"
#include "charclasses.h"
#include "forest.h"
#include "term_colors.h"

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

	// IDEA maybe we could use directly the default operator<=> for lit
	auto operator<=>(const lit<C, T>& l) const;
	// IDEA maybe we could use directly the default operator== for lit
	bool operator==(const lit<C, T>& l) const;

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

struct shaping_options {
	std::set<size_t> to_trim{};
	// nonterminal ids which children to trim by shaping coming from @trim children ...
	std::set<size_t> to_trim_children{};
	bool trim_terminals = false; // @trim all terminals.
	// nonterminal ids to inline by shaping coming from @inline ...
	std::set<std::vector<size_t>> to_inline{};
	bool inline_char_classes = false; // @inline char classes.
};

// grammar struct required by parser.
// accepts nonterminals ref, prods and char class functions
template <typename C, typename T> struct grammar_inspector;
template <typename C = char, typename T = C>
struct grammar {
	friend struct grammar_inspector<C, T>;
	typedef std::pair<lit<C, T>, std::vector<lits<C, T>>> production;
	struct options {
		shaping_options shaping = {};
		bool auto_disambiguate = true; // @enable/disable disambig.
		std::set<size_t> nodisambig_list{}; // @nodisambig nonterminal ids.
		bool transform_negation = true; // false to disable negation transformation
	} opt;
	grammar(nonterminals<C, T>& nts, options opt = {});
	grammar(nonterminals<C, T>& nts, const prods<C, T>& ps,
		const prods<C, T>& start, const char_class_fns<T>& cc_fns,
		options opt = {});
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
	// returns true if the production rule has more then 1 conjunction
	bool conjunctive(size_t p) const;
	// returns number of conjunctions for a given production p
	size_t n_conjs(size_t p) const;
	// checks if the char class function returns true on the char ch
	bool char_class_check(lit<C, T> l, T ch) const;
	// does the same check as char_class_check and if the check is true then
	// it adds new rule: "cc_fn_name => ch." into the grammar
	// returns id of the rule or (size_t)-1 if the check fails
	size_t get_char_class_production(lit<C, T> l, T ch);
	size_t add_char_class_production(lit<C, T> l, T ch);
	const std::set<size_t>& prod_ids_of_literal(const lit<C, T>& l) const;
	const lit<C, T>& start_literal() const;
	bool is_cc_fn(const size_t& p) const;
	bool is_eof_fn(const size_t& p) const;
	std::set<size_t> reachable_productions(const lit<C, T>& l) const;
	std::set<size_t> unreachable_productions(const lit<C, T>& l) const;
	std::ostream& check_nullable_ambiguity(std::ostream& os) const;
	std::ostream& print_production(std::ostream& os,
		size_t p, bool print_ids = false,
		const term::colors& TC = {false}) const;
	std::ostream& print_internal_grammar(std::ostream& os,
		std::string prep = {}, bool print_ids = false,
		const term::colors& TC = {false}) const;
	std::ostream& print_internal_grammar_for(std::ostream& os,
		const std::string& nt,	std::string prep = {},
		bool print_ids = false,
		const term::colors& TC = {false}) const;
#if defined(DEBUG) || defined(WITH_DEVHELPERS)
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
	using pnode         = std::pair<lit<C, T>, std::array<size_t, 2>>;
	using pforest       = forest<pnode>;
	using pnodes        = pforest::nodes;
	using pnodes_set    = pforest::nodes_set;
	using pnode_graph   = pforest::node_graph;
	using pgraph        = pforest::graph;
	using ptree         = pforest::tree;
	using psptree       = pforest::sptree;

	// earley item
	struct item {
		item(size_t set, size_t prod,size_t con,size_t from,size_t dot);
		bool operator<(const item& i) const;
		bool operator==(const item& i) const;
		size_t set, prod, con, from, dot;
	};

	// parser options for its constructor
	struct options {
		// applying binarization to ensure every forest node
		// has atmost 2 or less children nodes
		bool binarize = DEFAULT_BINARIZE;
		// build forest incrementally as soon any
		// item is completed
		bool incr_gen_forest = DEFAULT_INCR_GEN_FOREST;
		// enable garbage collection
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
	// input manager and decoder used by the parser
	struct input {
		using decoder_type =
				std::function<std::vector<T>(input&)>;
		input(const C* data, size_t size, size_t max_length = 0,
			decoder_type decoder = 0,
			int_type e = std::char_traits<C>::eof());
		input(std::basic_istream<C>& is, size_t max_length = 0,
			decoder_type decoder = 0,
			int_type e = std::char_traits<C>::eof());
		input(const std::string& filename, size_t max_length = 0,
			decoder_type decoder = 0,
			int_type e = std::char_traits<C>::eof());
#ifndef _WIN32
		input(int filedescriptor, size_t max_length = 0,
			decoder_type decoder = 0,
			int_type e = std::char_traits<C>::eof());
#endif
		~input();
		inline bool isstream() const;
		void clear(); // resets stream (if used) to reenable at()/tat()
		std::basic_string<C> get_string(); // returns input data as a string
		std::basic_string<T> get_terminals(size_t start, size_t end);
		std::basic_string<T> get_terminals(
			std::array<size_t, 2> pos_span);
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
		memory_map mm{};
		size_t n = 0;         // input position
		size_t l = 0;         // size of input data (0 for streams)
		size_t max_l = 0;     // read up to max length of the input size
		const C* d = 0;       // input data pointer if needed
		std::basic_istream<C> s;
		std::vector<T> ts{};  // all collected terminals
		size_t tp = 0;        // current terminal pos
	};
	// parse error
	struct error {
		enum info_lvl {
			INFO_BASIC,
			INFO_DETAILED,
			INFO_ROOT_CAUSE
		};
		int_t loc;	     // location of error
		size_t line;         // line of error
		size_t col;          // column of error
		std::vector<T> ctxt; // closest matching ctxt
		lits<C, T> unexp;    // unexpected literal sequence
		typedef struct _exp_prod {
			std::string exp;
			std::string prod_nt;
			std::string prod_body;
			// back track information to higher derivations
			std::vector<_exp_prod> bktrk;
		} exp_prod_t;

		// list of expected token and respective productions
		std::vector<exp_prod_t> expv;
		error() : loc(-1) {}
		std::string to_str(info_lvl lv = INFO_ROOT_CAUSE) const;
	};
	// result of the parse call
	struct result {
		// true if the parse was successful
		const bool found;
		// contains error information if the parse was unsuccessful
		const error parse_error;
		// tree shaping options from grammar
		const shaping_options shaping;

		// constructor
		result(grammar<C, T>& g, std::unique_ptr<input> in,
			std::unique_ptr<pforest> f, bool found, error err);

		// returns the parsed forest
		pforest* get_forest() const;
		// transforms forest into tree and shapes it according to
		// shaping options (trimming and inlining)
		// grammar shaping options are used by default
		// also transforms ambiguous nodes as children of __AMB__ nodes
		psptree get_shaped_tree() const;
		psptree get_shaped_tree(const shaping_options opts) const;
		psptree get_shaped_tree(const pnode& n) const;
		psptree get_shaped_tree(const pnode& n,	const shaping_options
								opts) const;
		// extracts the first parse tree from the parsed forest
		psptree get_tree();
		psptree get_tree(const pnode& n);

		// returns the input as a string (input's char type, ie. C)
		std::basic_string<C> get_input();
		// read terminals from input (input's terminal type, ie. T)
		std::basic_string<T> get_terminals() const;
		// read terminals from input according to the position span of
		// a provided node
		std::basic_string<T> get_terminals(const pnode& n) const;
		std::basic_ostream<T>& get_terminals_to_stream(
			std::basic_ostream<T>& os, const pnode& n) const;
		// reads terminals of a node and converts them to int
		// if the conversion fails or the int is out of range
		// returns no value
		std::optional<int_t> get_terminals_to_int(const pnode& n) const;

		// returns true if the parse foreest is ambiguous (contains >1 tree)
		bool is_ambiguous() const;
		// returns true if the parse forest is not ambiguous (contains 1 tree)
		bool has_single_parse_tree() const;
		// returns ambiguous nodes
		std::set<std::pair<pnode, pnodes_set>> ambiguous_nodes() const;
		std::ostream& print_ambiguous_nodes(std::ostream& os) const;

		// returns all nodes and edges of the forest
		using node_edge       = std::pair<pnode, pnode>;
		using edges           = std::vector<typename pforest::edge>;
		using nodes_and_edges = std::pair<pnodes, edges>;
		nodes_and_edges get_nodes_and_edges() const;

		// removes all prefixed symbols from the graph everywhere
		// by replacing them with their immediate children nodes
		bool inline_prefixed_nodes(pgraph& g,const std::string& prefix);
		// removes EBNF and binarize transformation prefixes
		bool inline_grammar_transformations(pgraph& g);

		// private members are accessible by parser
		friend parser<C, T>;
	private:
		// input moved here from the parse call
		std::unique_ptr<input> in = 0;
		// forest moved here from the parse call
		std::unique_ptr<pforest> f = 0;
		// if ambiguous, this is __AMB__ node lit used in a shaped tree
		lit<C, T> amb_node{};
		// recursive part of get_shaped_tree()
		void _get_shaped_tree_children(const shaping_options& opts,
			const std::vector<pnode>& nodes,
			std::vector<psptree>& child) const;
	};
	// parse options for parse() call
	struct parse_options {
		size_t max_length = 0; // read up to max length of the input size
		size_t start = SIZE_MAX; // start non-terminal, SIZE_MAX = use default
		C eof = std::char_traits<C>::eof(); // end of a stream
		bool measure = false; // measure time taken for parsing
		bool measure_each_pos = false; // for each string pos
		bool measure_forest = false; // forest building
		bool measure_preprocess = false; // preprocessing
		bool debug = false;
	};

	// constructor
	parser(grammar<C, T>& g, options o = {});

	// parse call
	result parse(const C* data, size_t size, parse_options po = {});
	result parse(std::basic_istream<C>& is, parse_options po = {});
	result parse(const std::string& fn, parse_options po = {});
#ifndef _WIN32
	result parse(int filedescriptor, parse_options po = {});
#endif
	bool found(size_t start = SIZE_MAX);
	error get_error();
	bool debug = false;
	std::pair<size_t, size_t> debug_at = { DEBUG_POS_FROM, DEBUG_POS_TO };
private:
	using container_t    = std::set<item>;
	using container_iter = typename container_t::iterator;
public:
	std::ostream& print(std::ostream& os, const item& i) const;
	std::ostream& print(std::ostream& os, const container_t& c,
		bool only_completed = false) const;
	std::ostream& print_data(std::ostream& os) const;
	std::ostream& print_S(std::ostream& os, bool only_completed=false)const;
	friend std::ostream& operator<<(std::ostream& os, const error& err) {
		return os << err.to_str(error::info_lvl::INFO_BASIC);
	}
	friend std::ostream& operator<<(std::ostream& os, const result& res) {
		return res.print_error(os);
	}
private:

	grammar<C, T>& g;
	options o;
	std::unique_ptr<input> in = 0;
	std::vector<container_t> S;
	std::vector<container_t> U; // uncompleted
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

	// binarized temporary intermediate non-terminals
	std::map<std::vector<lit<C, T>>, lit<C, T>> bin_tnt;
	size_t tid; // id for temporary non-terminals

	// helpers
	lit<C, T> get_lit(const item& i) const;
	lit<C, T> get_nt(const item& i) const;
	std::basic_string<C> get_fresh_tnt();
	std::vector<item> back_track(const item& obj);

	// parsing
	result _parse(const parse_options& po);
	std::pair<container_iter, bool> add(container_t& t, const item& i);
	bool nullable(const item& i) const;
	void resolve_conjunctions(container_t& c, container_t& t);
	void predict(const item& i, container_t& t);
	void scan(const item& i, size_t n, T ch);
	void scan_cc_function(const item& i, size_t n, T ch, container_t& t);
	void complete(const item& i, container_t& t, container_t& c,
		bool conj_resolved = false);
	bool completed(const item& i) const;
	bool negative(const item& i) const;
	// returns number of literals for a given item
	size_t n_literals(const item& i) const;
	std::pair<item, bool> get_conj(size_t set, size_t prod, size_t con) const;
	void pre_process(const item& i);
	bool init_forest(pforest& f, const lit<C, T>& start_lit,
		const parse_options& po);
	bool build_forest(pforest& f, const pnode& root);
	bool binarize_comb(const item&, std::set<std::vector<pnode>>&);
	void sbl_chd_forest(const item&,
		std::vector<pnode>&, size_t, std::set<std::vector<pnode>>&);
#ifdef DEBUG
	template <typename CharU>
	friend std::ostream& operator<<(std::ostream& os, lit<C, T>& l);
	template <typename CharU>
	friend std::ostream& operator<<(std::ostream& os,
		const std::vector<lit<C, T>>& v);
#endif
};

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
// template definitions
#include "grammar.tmpl.h"       // for grammar and related
#include "parser.tmpl.h"        // for parser
#include "parser_result.tmpl.h" // for parse::result
#include "tgf.h"                // Tau Grammar Form
#ifdef WITH_DEVHELPERS
#include "devhelpers.h"   // various helpers for converting forest
#endif
// undef local macros
#undef DEFAULT_BINARIZE
#undef DEFAULT_INCR_GEN_FOREST
#endif // __IDNI__PARSER__PARSER_H__
