// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.txt

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
#include <initializer_list>
#include <map>
#include <set>

// #define PARSER_BINTREE_FOREST true

#include "defs.h"
#include "utility/memory_map.h"
#include "utility/characters.h"
#include "utility/charclasses.h"
#include "utility/term_colors.h"
#include "utility/tree.h"

#ifndef PARSER_BINTREE_FOREST
#include "utility/forest.h"
#endif

#define DEFAULT_BINARIZE false
#define DEFAULT_INCR_GEN_FOREST false

namespace idni {

template <typename C, typename T>
struct lit;

/**
 * @brief A container for maping ids of non-terminal literals to their names.
 * 
 * Since this library sees nonterminals only as their ids this container
 * provides some convenience to use string names and get string names back when
 * using the library.
 */
template <typename C = char, typename T = C>
struct nonterminals : public std::vector<std::basic_string<C>> {
	/// Create new empty container for non-terminals.
	nonterminals() = default;
	/// Create a container and initialize it with the provided list.
	nonterminals(const std::vector<std::basic_string<C>>& init_list);
	/// Create a container and initialize it with the provided list.
	nonterminals(std::initializer_list<std::basic_string<C>> init_list);
	/// Adds new name \p s of a non-terminal into the container and returns its id
	size_t get(const std::basic_string<C>& s);
	/// Returns a name of a non-terminal by a provided id \p nt
	const std::basic_string<C>& get(size_t n) const;
	/// Returns a non-terminal literal by it's name. If the name does not exist in the container yet it adds it.
	lit<C, T> operator()(const std::basic_string<C>& s);
	/// Returns a non-terminal literal by it's id.
	lit<C, T> operator()(size_t n);
private:
	std::map<std::basic_string<C>, size_t> m;
};

/// Literal containing terminal (c where if c = 0 then null) or nonterminal (n)
template <typename C = char, typename T = C>
struct lit {
	std::variant<size_t, T> data;
	nonterminals<C, T>* nts = 0;
	bool is_null_ = false;
	/**
	 * @brief Creates a null literal.
	 * 
	 * Null literal can be used as a body of a production rule or as one of its
	 * disjuncted bodies to satisfy a production rule when there is no sequence
	 * matched.
	 */
	lit();
	/// Creates a terminal literal where \p t is a template type of the terminal.
	lit(T t);
	/// Creates a non-terminal literal.
	lit(size_t n, nonterminals<C, T>* nts);
	/**
	 * @brief Returns true if the literal is nonterminal or false if it is terminal.
	 * 
	 * Before accessing a value of the literal by calling n() or t() it is
	 * required to determine if it is a nonterminal or a terminal first.
	 */
	bool  nt() const;
	/**
	 * @brief Returns the id of an non-terminal if the literal is non-terminal.
	 * 
	 * Before calling this method one has to be sure that it is a non-terminal.
	 * Use nt() to find out.
	 */
	size_t n() const;
	/**
	 * @brief Returns the terminal character (or terminal of a type T) if the literal is terminal.
	 * 
	 * Before calling this method one has to be sure that it is a terminal. Use
	 * nt() to find out.
	 */
	T      t() const;
	/// Returns true if the literal is null.
	bool is_null() const;
	size_t hashit() const {
		std::size_t seed = grcprime;
		hash_combine(seed, static_cast<bool>(nt()));
		if (nt()) hash_combine(seed, n());
		else hash_combine(seed, t());
		return seed;
	}

	// IDEA maybe we could use directly the default operator<=> for lit
	auto operator<=>(const lit<C, T>& l) const;
	// IDEA maybe we could use directly the default operator== for lit
	bool operator==(const lit<C, T>& l) const;

	/**
	 * @brief Returns a vector of terminals.
	 * 
	 * It would have no elements if is non-terminal or null or it would have a
	 * single terminal element if the literal is terminal.
	 */
	std::vector<T> to_terminals() const;
	/**
	 * @brief Returns the literal as a string.
	 * 
	 * \p nll is a string returned when the literal is null. If the literal is
	 * a terminal it returns the string containing just the terminal (escaped)
	 * in apostrophes ('). If the literal is a non-terminal it returns the name
	 * of the non-terminal literal.
	 */
	std::basic_string<C> to_string(const std::basic_string<C>& nll={})const;
	/**
	 * @brief Returns the literal as a standard string (basic_string<char>).
	 * 
	 * This is useful for printing literals into standard output.
	 * This method works in a same way as the previous method to_string() and
	 * converts the result into a std::string if it isn't already.
	 */
	std::string to_std_string(const std::basic_string<C>& nll = {}) const;
};

template <typename C = char, typename T = C>
std::ostream& operator<<(std::ostream& os,
	const std::pair<lit<C, T>, std::array<size_t, 2>>& obj);

// production rules in a disjunctive normal form of conjunction clauses of lits
/// Literals
template <typename C = char, typename T = C> 
struct lits : public std::vector<lit<C, T>> { 
	/**
	 * @brief Whether the sequence is negated or not.
	 * 
	 * Negated literal sequence satisfies a part of a production rule if it
	 * does not match the input.
	 */
	bool neg = false; 
};
template <typename C = char, typename T = C>
using conjs = std::set<lits<C, T>>; /// conjunctions of literals
template <typename C = char, typename T = C>
using disjs = std::set<conjs<C, T>>; /// disjunctions of literal conjunctions
/// Production rule of a grammar.
template <typename C = char, typename T = C>
struct prod {
	/// Rule's head literal
	lit<C, T>                  first;
	/// Rule's body in a form of a disjunction
	disjs<C, T>                second;
	std::optional<std::string> guard;
	bool operator==(const prod& p) const;
};
/**
 * @brief Represents a grammar in DNF.
 * 
 * It is a list of production rules. It can also represent a single production
 * rule or any element of a production rule since prods is used for expressions
 * to make building of a grammar programatically more convenient.
 */
template <typename C = char, typename T = C>
struct prods : public std::vector<prod<C, T>> {
	typedef std::vector<prod<C, T>> prods_t;
	/// Creates an empty prods.
	prods();
	/**
	 * Creates prods with a single prod rule with an empty head literal and
	 * with a single literal l as body of the rule. Such a prods represents a
	 * literal lit usable in expressions.
	 */
	prods(const lit<C, T>& l);
	/**
	 * Same as previous constructor but instead of a single literal l in body
	 * of the rule it represents a sequence of terminal literals from the
	 * string \p s.
	 */
	prods(const std::basic_string<C>& s);
	/**
	 * Same as previous constructor but terminal literals are taken from the
	 * vector v.
	 */
	prods(const std::vector<T>& v);
	/**
	 * Adds a prod rule with an empty head literal and with a body with a
	 * single literal \p l.
	 */
	void operator()(const lit<C, T>& l);
	/**
	 * Adds a prod rule with an empty head literal and with a body with a
	 * sequence of terminal literals. Sequence is provided as a string \p s.
	 */
	void operator()(const std::basic_string<C>& s);
	/**
	 * Adds a prod rule with an empty head literal and with a body with a
	 * sequence of terminal literals. Sequence is provided as a vector \p v.
	 */
	void operator()(const std::vector<T>& v);
	/**
	 * Adds a rule prod with a head literal \p l and body \p p represented by
	 * prods.
	 */
	void operator()(const prods<C, T>& l, const prods<C, T>& p);
	/**
	 * Adds a rule prod with a head literal \p l (represented by prods) and
	 * body \p p (represented by prods)
	 */
	void operator()(const lit<C, T>& l, const prods<C, T>& p);
	/// 
	bool operator==(const lit<C, T>& l) const;
	/**
	 * Returns the last literal from a body of the last rule. This simplifies
	 * getting of a lit if prods represents a single lit element.
	 */
	lit<C, T> to_lit() const;
	/// Returns a disjs (body) of the last rule prod.
	disjs<C, T> to_disjs() const;
};

/// Char class functions
template <typename T = char>
using char_class_fn = std::function<bool(T)>;
/**
 * @brief Container for character class functions.
 * 
 * It can be passed to a grammar when instantiated.
 */
template <typename T = char>
struct char_class_fns {
	/// Functions container.
	std::map<size_t, char_class_fn<T>> fns = {};
	/// char -> production
	std::map<size_t, std::map<T, size_t>> ps = {};
	/// Adds new char class function.
	void operator()(size_t nt, const char_class_fn<T>& fn);
	/// Returns true if a \p nt is a character class function.
	bool is_fn(size_t nt) const;
	/// Returns true if a \p nt is an eof character class function.
	bool is_eof_fn(size_t nt) const; 
	/// id of an eof function.
	size_t eof_fn = -1;
};

/**
 * @brief Return a char_class_fns container with specified functions.
 * 
 * Helper function which returns a `char_class_fns` container with one or more
 * of specified predefined character functions.
 */
template <typename C = char, typename T = C>
char_class_fns<T> predefined_char_classes(
	const std::vector<std::string>& cc_fn_names, nonterminals<C, T>& nts);

/// @brief Settings which are used to shape parsed trees when shaping is used.
struct shaping_options {
	/**
	 * @brief Nonterminal ids of symbols to trim (including their children) away from the resulting tree.
	 * 
	 * List can be provided in TGF with @trim whitespace, comment.
	 */
	std::set<size_t> to_trim{};
	/// nonterminal ids which children to trim by shaping coming from @trim children ...
	std::set<size_t> to_trim_children{};
	/**
	 * @brief Nonterminal ids of symbols which children will be trimmed away from the resulting tree.
	 * 
	 * List can be provided in TGF with @trim children sym1, sym2.
	 */
	std::set<size_t> to_trim_children_terminals{};
	/**
	 * @brief Whetther to trim all terminals from the resulting parse tree.
	 * 
	 * If trim_terminals is true it trims all terminals from the resulting parse tree. It is false by default.
	 * 
	 * This can be set to true in TGF by @trim all terminals.
	 */
	bool trim_terminals = false;
	std::set<size_t> dont_trim_terminals_of{}; /// except children of ...
	/// nonterminal ids to inline by shaping coming from @inline ...
	/**
	 * @brief Replaces a node with its children.
	 * 
	 * Contains vectors of nonterminal ids.
	 * 
	 * If a vector contains only a single nonterminal, it means that the
	 * nonterminal is replaced by its children.
	 * 
	 * If a vector contains more than one nonterminal, it searches the parsed
	 * tree for occurance of a tree path of nonterminals denoted by nonterminal
	 * ids in the vector. First nonterminal is replaced by the last nonterminal
	 * node in the vector.
	 * 
	 * This can be populated by @inline directive.
	 * 
	 * @inline chars. Node chars is replaced by its children.
	 * 
	 * @inline expr > block > expr. Node expr containing a child node block
	 * which contains a child node expr is replaced by the expr child.
	 */
	std::set<std::vector<size_t>> to_inline{};
	/**
	 * @brief Whether shaping will also inline all character class functions.
	 * 
	 * If inline_char_classes is true shaping will also inline all character
	 * class functions. Is a short alternative to adding them into to_inline or
	 * @inline one by one.
	 * 
	 * Default value is false.
	 * 
	 * In TGF this can be set to true by adding char classes to @inline directive.
	 * 
	 * @inline char classes.
	 */
	bool inline_char_classes = false;
};


template <typename C, typename T> struct grammar_inspector;
template <typename C, typename T> class parser;
/**
 * @brief Grammar struct required by parser.
 * 
 * Accepts nonterminals ref, prods and char class functions.
 */
template <typename C = char, typename T = C>
struct grammar {
	friend struct grammar_inspector<C, T>;
	friend struct parser<C, T>;
	typedef std::pair<lit<C, T>, std::vector<lits<C, T>>> production;
	struct options {
		/**
		 * @brief Whether negations in a grammar are transformed into a negation tracking rule.
		 * 
		 * If this is true, and it is true by default, any negation in a
		 * grammar is transformed into a negation tracking rule when the
		 * grammar is instantiated. false is used for example to instantiate
		 * grammar from a generated parser because its rules are already
		 * product of this negation transormation.
		 */
		bool transform_negation = true;
		/**
		 * @brief Disambiguates according to an order of production rule in the grammar.
		 * 
		 * This is true by default. Parser disambiguates parse trees according
		 * to an order of production rule appearance in the grammar. Setting
		 * this to false would make parser to provide all possible parse trees.
		 * 
		 * This can be also disabled by TGF directive @disable disambiguation.
		 */
		bool auto_disambiguate = true;
		/**
		 * If auto_disambiguate is set to true this list contains nonterminal
		 * ids of symbols we don't want to disambiguate and we want to keep
		 * ambiguity in the resulting parse forest.
		 * 
		 * This list can be set by TGF directive: @ambiguous symbol1, symbol2.
		 */
		std::set<size_t> nodisambig_list{};
		/// Parsed tree shaping_options
		shaping_options shaping = {};
		/// Production guard names
		std::set<std::string> enabled_guards = {}; 
	} opt;
	grammar(nonterminals<C, T>& nts, options opt = {});
	grammar(nonterminals<C, T>& nts, const prods<C, T>& ps,
		const prods<C, T>& start, const char_class_fns<T>& cc_fns,
		options opt = {});
	/// Sets guards of enabled productions
	void set_enabled_productions(const std::set<std::string>&);
	void productions_enable(const std::string& guard);
	void productions_disable(const std::string& guard);
	/// Returns number of productions (every disjunction has a prod rule)
	size_t size() const;
	/// Returns head of the prod rule - literal
	lit<C, T> operator()(const size_t& i) const;
	/// Returns body of the prod rule - conjunctions of literals
	const std::vector<lits<C, T>>& operator[](const size_t& i) const;
	/// Returns length of the conjunction
	size_t len(const size_t& p, const size_t& c) const;
	/// Returns true if the literal is nullable
	bool nullable(lit<C, T> l) const;
	/// Returns true if the production rule has more then 1 conjunction
	bool conjunctive(size_t p) const;
	/// Returns number of conjunctions for a given production p
	size_t n_conjs(size_t p) const;
	/// Checks if the char class function returns true on the char ch
	bool char_class_check(lit<C, T> l, T ch) const;
	/**
	 * Does the same check as char_class_check and if the check is true then
	 * it adds new rule: "cc_fn_name => ch." into the grammar
	 * returns id of the rule or (size_t)-1 if the check fails
	 */
	size_t get_char_class_production(lit<C, T> l, T ch);
	/// Adds a new production rule: l => ch and returns index of it.
	size_t add_char_class_production(lit<C, T> l, T ch);
	/// Returns indexes of all production rules for a nonterminal l.
	const std::set<size_t>& prod_ids_of_literal(const lit<C, T>& l) const;
	/// Returns the starting nonterminal literal.
	const lit<C, T>& start_literal() const;
	/// Returns true if the production rule with index p is a character class function.
	bool is_cc_fn(const size_t& p) const;
	/// Returns true if the production rule with index p is the eof character class.
	bool is_eof_fn(const size_t& p) const;
	std::set<size_t> reachable_productions(const lit<C, T>& l) const;
	std::set<size_t> unreachable_productions(const lit<C, T>& l) const;
	std::ostream& check_nullable_ambiguity(std::ostream& os) const;
	/// Prints a production rule with index p into ostream os.
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
#ifdef DEBUG
	std::ostream& print_data(std::ostream& os, std::string prep = {}) const;
#endif // DEBUG
	/// Returns a literal of a nonterminal with id n.
	lit<C, T> nt(size_t n);
	/** 
	 * Returns a literal of a nonterminal named s. It is added into
	 * nonterminals if it's not contained already.
	 */
	lit<C, T> nt(const std::basic_string<C>& s);
	const lit<C, T>& get_start() const;
private:
	bool all_nulls(const lits<C, T>& a) const;
	nonterminals<C, T>& nts;
	lit<C, T> start;
	char_class_fns<T> cc_fns = {};
	std::map<lit<C, T>, std::set<size_t>> ntsm = {};
	std::map<size_t, size_t> grdm = {};
	std::vector<std::string> guards = {};
	std::set<size_t> nullables = {};
	std::set<size_t> conjunctives = {};
	std::vector<production> G;
};

#ifndef PARSER_BINTREE_FOREST
/**
* @brief A structure used as a forest node.
*
* It maps nodes to pointers to make sure the forest does not contain
* duplicities.
*/
template<typename C, typename T = C>
struct pnode_type : public std::pair<lit<C, T>, std::array<size_t, 2>> {
	using node_type = std::pair<lit<C, T>, std::array<size_t, 2>>;
	friend forest<pnode_type>;
private:
	static typename forest<pnode_type>::node ptrof(const pnode_type& p);
	static std::map<const pnode_type, typename forest<pnode_type>::node>&
		nid()
	{
		static std::map<const pnode_type,
				typename forest<pnode_type>::node> instance;
		return instance;
	}
public:
	const size_t hash;

	pnode_type() : hash(hashit()) {}
	pnode_type(const lit<C, T>& _f, const std::array<size_t, 2>& _s)
		: node_type(_f, _s), hash(hashit()) {}
	inline operator typename forest<pnode_type>::node() const {
		return ptrof(*this);
	}
	friend std::ostream& operator<<<>(std::ostream& os, const node_type& n);
	inline size_t _mpsize() const { return nid().size(); }
	std::size_t hashit() const {
		std::size_t seed = this->first.hashit();
		hash_combine(seed, this->second[0], this->second[1]);
		return seed;
	}
	//inline lit<C,T> &first() const { return this->first; }
	//inline std::array<size_t, 2>& second() const { return this->second; }
};
#endif

template <typename C = char, typename T = C>
class parser {
public:
	struct input;
	using char_type       = C;
	using terminal_type   = T;
	using traits_type     = std::char_traits<char_type>;
	using int_type        = typename traits_type::int_type;
	using grammar_type    = idni::grammar<char_type, terminal_type>;
	using grammar_options = grammar_type::options;
	using symbol_type     = idni::lit<char_type, terminal_type>;
	using location_type   = std::array<size_t, 2>;
	using node_type       = std::pair<symbol_type, location_type>;
	using parser_type     = idni::parser<char_type, terminal_type>;

#ifdef PARSER_BINTREE_FOREST
	using pnode = node_type;
#else
	using pnode = pnode_type<C, T>;
#endif
	struct tree : public lcrs_tree<pnode> {
		using base_t = lcrs_tree<pnode>;

		tref get() const;
		static const tree& get(const tref id);
		static const tree& get(const htref& h);
		static const htref geth(tref h);

		static tref get(const pnode& v, const tref* ch, size_t len);
		static tref get(const pnode& v, const trefs& ch);
		static tref get(const pnode& v, tref ch); // with single child
		static tref get(const pnode& v, tref ch1, tref ch2); // with two children
		static tref get(const pnode& v); // leaf node

		static tref get(const pnode& v, const pnode* ch, size_t len);
		static tref get(const pnode& v, const std::vector<pnode>& ch);
		static tref get(const pnode& v, const pnode& ch);
		static tref get(const pnode& v, const pnode& ch1, const pnode& ch2); 

		size_t children_size() const;

		bool get_children(tref *ch, size_t& len) const;
		trefs get_children() const;
		tref_range<pnode> children() const;
		tree_range<tree> children_trees() const;

		tref child(size_t n) const;
		tref first()  const;
		tref second() const;
		tref third()  const;
		tref only_child() const;

		const tree& operator[](size_t n) const;
		const tree& child_tree(size_t n) const;
		const tree& first_tree()  const;
		const tree& second_tree() const;
		const tree& third_tree()  const;
		const tree& only_child_tree() const;
		const tree& right_sibling_tree() const;

		std::ostream& print(std::ostream& o, size_t s = 0) const;

		// fast access helpers
		bool is_nt() const;
		bool is(size_t nt) const;
		bool is_t() const;
		std::string get_terminals() const;
		size_t get_nt() const;
		char get_t() const;

		// provide also same api as literal
		bool nt() const { return this->value.first.nt(); }
		char t() const { return this->value.first.t(); }
		size_t n() const { return this->value.first.n(); }
		bool is_null() const { return this->value.first.is_null(); }

		// const pnode& get_value() const { return this->value; }
		lit<C, T> get_literal() const { return this->value.first; }
		location_type get_location() const { return this->value.second; }

		// tree wrapper for simple traversing using | and || operators
		// and | extractor<result_type>
		//                only_child, opt_nonterminal, nonterminal,
		//                terminals, first, second, third)
		struct traverser {
			traverser();
			traverser(tref r);
			traverser(const trefs& n);
			bool has_value() const;
			explicit operator bool() const;
			tref value() const;
			const tree& value_tree() const;
			const tree& operator[](size_t n) const;
			const trefs& values() const;
			std::vector<traverser> traversers() const;
			std::vector<traverser> operator()() const;

			template <typename result_type>
			struct extractor {
				using function = std::function<
						result_type(const traverser&)>;
				extractor(const function& e) : e(e) {}
				result_type operator()(const traverser& t) const
								{ return e(t); }
			private:
				function e;
			};

			static inline const extractor<tref> ref{
				[](const traverser& t) -> tref {
					return t.value();
				}};
			static inline const extractor<traverser> only_child{
				[](const traverser& t) {
					if (!t) return traverser();
					tref r = t.value_tree().only_child();
					if (!r) return traverser();
					return traverser(r);
				}};
			static inline const extractor<traverser> children{
				[](const traverser& t) {
					if (!t) return traverser();
					return traverser(t.value_tree()
							.get_children());
				}};
			static inline const extractor<tref_range<pnode>>
							children_range{
				[](const traverser& t) -> tref_range<pnode> {
					if (!t) return { nullptr };
					return t.value_tree().children();
				}};
			static inline const extractor<tree_range<tree>>
							children_trees_range{
				[](const traverser& t) -> tree_range<tree> {
					if (!t) return { nullptr };
					return t.value_tree().children_trees();
				}};
			static inline const extractor<traverser> first{
				[](const traverser& t) {
					if (!t) return traverser();
					tref r = t.value_tree().first();
					if (!r) return traverser();
					return traverser(r);
				}};
			static inline const extractor<traverser> second{
				[](const traverser& t) {
					if (!t) return traverser();
					tref r = t.value_tree().second();
					if (!r) return traverser();
					return traverser(r);
				}};
			static inline const extractor<traverser> third{
				[](const traverser& t) {
					if (!t) return traverser();
					tref r = t.value_tree().third();
					if (!r) return traverser();
					return traverser(r);
				}};
			static inline const extractor<std::string> terminals{
				[](const traverser& t) {
					if (!t) return std::string();
					return t.value_tree().get_terminals();
				}};
			static inline const extractor<size_t> nonterminal{
				[](const traverser& t) -> size_t {
					if (!t) return 0;
					return t.value_tree().get_nt();
				}};
			static inline const extractor<std::optional<size_t>>
							opt_nonterminal{
				[](const traverser& t) -> std::optional<size_t> {
					if (!t) return std::optional<size_t>{};
					const auto& x = t.value_tree();
					if (x.value.first.nt())
						return { x.value.first.n() };
					return {};
				}};
			static inline const extractor<traverser> dump{
				[](const traverser& t) {
					if (!t) return t;
					t.value_tree().dump(std::cout) << "\n";
					return t;
				}};

			traverser operator|(size_t nt) const;
			traverser operator||(size_t nt) const;
			template <typename result_type>
			result_type operator|(const extractor<result_type>&)
									const;
			template <typename result_type>
			result_type operator||(const extractor<result_type>&)
									const;
		private:
			bool has_value_ = true;
			trefs values_{};
		};
		traverser operator|(size_t nt) const;
		traverser operator||(size_t nt) const;
	};
	using tt = tree::traverser;
#ifdef PARSER_BINTREE_FOREST
	using pnodes          = std::vector<pnode>;
	using pnodes_set      = std::set<pnodes>;
#else
	using pforest         = forest<pnode>;
	using pnodes          = pforest::nodes;
	using pnodes_set      = pforest::nodes_set;
	using pnode_graph     = pforest::node_graph;
	using pgraph          = pforest::graph;
	using ptree           = pforest::tree;
	using psptree         = pforest::sptree;
	using forest_type     = pforest;
	using sptree_type     = psptree;
#endif
	using encoder_type    = std::function<std::basic_string<char_type>(
					const std::vector<terminal_type>&)>;

	/// earley item
	struct item {
		item(size_t set, size_t prod,size_t con,size_t from,size_t dot);
		bool operator<(const item& i) const;
		bool operator==(const item& i) const;
		size_t set, prod, con, from, dot;
	};
	struct item_hash {
		size_t operator()(const item& i) const {
			size_t seed = grcprime;
			hash_combine(seed, i.set, i.prod, i.con, i.from, i.dot);
			return seed;
		}
	};

	/// Input manager and decoder used by the parser
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
		inline bool good() const;
		inline bool isstream() const;
		/// Resets stream (if used) to reenable at()/tat()
		void clear();
		/// Returns input data as a string
		std::basic_string<C> get_string();
		std::basic_string<T> get_terminals(size_t start, size_t end);
		std::basic_string<T> get_terminals(
			std::array<size_t, 2> pos_span);
		// source stream access
		/// The current character.
		C cur();
		/// The position of the current character.
		size_t pos();
		/**
		 * @brief Moves to the next character.
		 * 
		 * Increments position and updates the current character.
		 */
		bool next();
		/// Whether the input has reached the end of file or a size stated in parse() call.
		bool eof();
		/// Reads value at pos (uses seek, if stream)
		C at(size_t p);
		/// Transformed stream access
		T tcur();
		size_t tpos();
		bool tnext();
		bool teof();
		/// Reads value at tpos 
		T tat(size_t p);
	private:
		void decode();
		/// input type
		enum type { POINTER, STREAM, MMAP } itype = POINTER;
		/// end of a stream
		int_type e = std::char_traits<C>::eof();
		decoder_type decoder = 0;
		memory_map mm{};
		/// input position
		size_t n = 0;
		/// size of input data (0 for streams)
		size_t l = 0;
		/// read up to max length of the input size
		size_t max_l = 0;
		/// input data pointer if needed
		const C* d = 0;
		std::basic_istream<C> s;
		/// all collected terminals
		std::vector<T> ts{};
		/// current terminal pos
		size_t tp = 0;
	};
	using decoder_type = input::decoder_type;

	/// Parser options for its constructor
	struct options {
		/// Applying binarization to ensure every forest node
		/// has atmost 2 or less children nodes
		bool binarize = DEFAULT_BINARIZE;
		/// Build forest incrementally as soon any
		/// item is completed
		bool incr_gen_forest = DEFAULT_INCR_GEN_FOREST;
		/// Enable garbage collection
		bool enable_gc = false;
		/// Number of steps garbage collection lags behind
		/// parsing position n. should be greater than
		/// 0 and less than the size of the input
		/// for any collection activity. We cannot use
		/// % since in the streaming case, we do not know
		/// exact size in advance
		size_t gc_lag = 1;
		/**
		 * Decoder function reading an input converting element or elements of
		 * a type C to a vector of elements of type T according to template
		 * type parameters T and C. More about these recorders here.
		 * 
		 * Default value is 0, ie. no decoder function.
		 */
		decoder_type chars_to_terminals = 0;
		/**
		 * Encoder function converting vector of terminals of a type T to a
		 * std::basic_string\<C> according to template type parameters T and C.
		 * 
		 * Default value is 0, ie. no encoder function.
		 */
		encoder_type terminals_to_chars = 0;
	};

	/// Parse error
	struct error {
		enum info_lvl {
			INFO_BASIC,
			INFO_DETAILED,
			INFO_ROOT_CAUSE
		};
		/// Location of error.
		int_t loc;
		/// Line of error.
		size_t line = 0;
		/// Column of error.
		size_t col = 0;
		 /// Closest matching context.
		std::vector<T> ctxt;
		/// Terminal which was scanned in a place where it was not expected.
		lits<C, T> unexp;
		typedef struct _exp_prod {
			std::string exp;
			std::string prod_nt;
			std::string prod_body;
			/// Back track information to higher derivations
			std::vector<_exp_prod> bktrk;
		} exp_prod_t;

		/// List of expected token and respective productions
		std::vector<exp_prod_t> expv;
		error() : loc(-1) {}
		/// Produces a string containing human readable information about the error.
		std::string to_str(info_lvl lv = INFO_DETAILED,
			size_t line_start = 0) const;
	};
	/// Result of the parse call.
	struct result {
		/// True if the parse was successful
		const bool found;
		/// Contains error information if the parse was unsuccessful
		const error parse_error;
		/// Tree shaping options from grammar
		const shaping_options shaping;

#ifdef PARSER_BINTREE_FOREST
		// constructor
		result(parser<C, T>& p, std::unique_ptr<input> in_,
			tref f, bool found, error err);
#else
		// constructor
		result(parser<C, T>& p, std::unique_ptr<input> in_,
			std::unique_ptr<pforest> f, bool found, error err);
		parser<C, T>& get_parser() const;
		// returns the parsed forest
		pforest* get_forest() const;
		/// Transforms forest into tree and applies trimming
		/// grammar shaping options are used by default
		/// also transforms ambiguous nodes as children of __AMB__ nodes
		psptree get_trimmed_tree(const pnode& n) const;
		psptree get_trimmed_tree(const pnode& n,
			const shaping_options opts) const;
		/// Applies inlining to a tree (w/o tree paths)
		psptree inline_tree_nodes(const psptree& t, psptree& parent)
			const;
		psptree inline_tree_nodes(const psptree& t, psptree& parent,
			const shaping_options opts) const;
		/// Applies tree paths inlining to a tree
		psptree inline_tree_paths(const psptree& t) const;
		psptree inline_tree_paths(const psptree& t,
			const shaping_options opts) const;
		/// Applies inlining to a tree
		psptree inline_tree(psptree& t) const;
		psptree inline_tree(psptree& t,
			const shaping_options opts) const;
		psptree trim_children_terminals(const psptree& t) const;
		psptree trim_children_terminals(const psptree& t,
			const shaping_options opts) const;
		/// Transforms forest into a tree and applies shaping
		psptree get_shaped_tree() const;
		psptree get_shaped_tree(const shaping_options opts) const;
		psptree get_shaped_tree(const pnode& n) const;
		psptree get_shaped_tree(const pnode& n,
			const shaping_options opts) const;

		/// Extracts the first parse tree from the parsed forest
		psptree get_tree();
		psptree get_tree(const pnode& n);
#endif
		// applies trimming grammar shaping options are used by default
		tref get_trimmed_tree2(tref ref) const;
		tref get_trimmed_tree2(tref ref, const shaping_options opts)
									const;
		// applies inlining to a tree (w/o tree paths)
		tref inline_tree_nodes2(tref t) const;
		tref inline_tree_nodes2(tref t, const shaping_options opts)
									const;
		// applies tree paths inlining to a tree
		tref inline_tree_paths2(tref t) const;
		tref inline_tree_paths2(tref t, const shaping_options opts)
									const;
		// applies inlining to a tree
		tref inline_tree2(tref t) const;
		tref inline_tree2(tref t, const shaping_options opts) const;
		tref trim_children_terminals2(tref ref) const;
		tref trim_children_terminals2(tref ref,
					const shaping_options opts) const;
		// transforms forest into a tree and applies shaping
		tref get_shaped_tree2();
		tref get_shaped_tree2(const shaping_options opts);
		tref get_shaped_tree2(tref t);
		tref get_shaped_tree2(tref t, const shaping_options opts);
		// returns parsed tree
		tref get_tree2();
		tref get_tree2(const pnode& n);

		/// Is input good = stream is good or mmap is opened
		bool good() const;
		/// Returns the input as a string (input's char type, ie. C)
		std::basic_string<C> get_input();
		/// Read terminals from input (input's terminal type, ie. T)
		std::basic_string<T> get_terminals() const;
		/// Read terminals from input according to the position span of
		/// a provided node
		std::basic_string<T> get_terminals(const pnode& n) const;
		std::basic_ostream<T>& get_terminals_to_stream(
			std::basic_ostream<T>& os, const pnode& n) const;
		/// Reads terminals of a node and converts them to int
		/// if the conversion fails or the int is out of range
		/// returns no value
		std::optional<int_t> get_terminals_to_int(const pnode& n) const;

		/// Returns true if the parse foreest is ambiguous (contains >1 tree)
		bool is_ambiguous() const;
		/// Returns true if the parse forest is not ambiguous (contains 1 tree)
		bool has_single_parse_tree() const;
		/// Returns ambiguous nodes
		std::set<std::pair<pnode, pnodes_set>> ambiguous_nodes() const;
		/// Prints ambiguous nodes.
		std::ostream& print_ambiguous_nodes(std::ostream& os) const;

		using node_edge       = std::pair<pnode, pnode>;
#ifdef PARSER_BINTREE_FOREST
		using edge            = std::pair<size_t, size_t>;
		using edges           = std::vector<edge>;
#else
		using edges           = std::vector<typename pforest::edge>;
#endif
		using nodes_and_edges = std::pair<std::vector<node_type>, edges>;
		/// Returns all nodes and edges of the forest
		nodes_and_edges get_nodes_and_edges() const;

#ifndef PARSER_BINTREE_FOREST
		/// Removes EBNF and binarize transformation prefixes
		bool inline_grammar_transformations(pgraph& g);
		// removes all prefixed symbols from the graph everywhere
		// by replacing them with their immediate children nodes
		bool inline_prefixed_nodes(pgraph& g,
			const std::basic_string<C>& prefix);
		// removes all nodes with the given nt from the graph everywhere
		bool inline_nodes(pgraph& g,
				  const std::set<size_t>& nts_to_inline);
#endif
		// private members are accessible by parser
		friend parser<C, T>;
	private:
		parser<C, T>& p;
		// input moved here from the parse call
		std::unique_ptr<input> in_ = 0;
		// forest moved here from the parse call
#ifdef PARSER_BINTREE_FOREST
		htref froot = 0;
#else
		std::unique_ptr<pforest> f = 0;
		/// Filters nonterminals by prefixes
		std::set<size_t> get_nts_by_prefixes(
			const std::set<std::basic_string<C>>& prefixes) const;
#endif
		// if ambiguous, this is __AMB__ node lit used in a shaped tree
		lit<C, T> amb_node{};
		/// Recursive part of get_shaped_tree()
		void _get_shaped_tree_children(const shaping_options& opts,
			const pnodes& nodes,
			std::vector<psptree>& child) const;
	};
	/// Parse options for parse() call
	struct parse_options {
		/// Read up to max length of the input size
		size_t max_length = 0;
		/// Start non-terminal, SIZE_MAX = use default
		size_t start = SIZE_MAX;
		/// End of a stream
		C eof = std::char_traits<C>::eof();
		/// Measure time taken for parsing
		bool measure = false;
		/// For each string pos
		bool measure_each_pos = false;
		/// Forest building
		bool measure_forest = false;
		/// Preprocessing
		bool measure_preprocess = false;
		bool debug = false;
	};

	// constructor
	parser(grammar<C, T>& g, options o = {});
	virtual ~parser() {};

	// parse call
	result parse(const C* data, size_t size, parse_options po = {});
	result parse(std::basic_istream<C>& is, parse_options po = {});
	result parse(const std::string& fn, parse_options po = {});
#ifndef _WIN32
	result parse(int filedescriptor, parse_options po = {});
#endif
	/**
	 * @brief Whether the last parse method call matched a starting literal production rule.
	 * 
	 * This means that the parsed input was parsed fully and successfully and
	 * there exists at least one tree with a root being the starting literal.
	 * If this method returns false use parser<C, T>::get_error() to obtain
	 * more information.
	 */
	bool found(size_t start = SIZE_MAX);
	/**
	 * Returns parser<C, T>::error object containing information about a
	 * parsing error if any happened, ie. found() call returned false.
	 */
	error get_error();
	grammar<C, T>& get_grammar() { return g; }
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
	std::unique_ptr<input> in_ = 0;
	std::vector<container_t> S;
	std::vector<container_t> U; /// uncompleted
		///mapping from to position of end in S for items
	std::unordered_map<size_t, std::vector<size_t>> fromS;
	std::unordered_map<std::pair<lit<C, T>,size_t> , std::set<item>> cache;

	/// refcounter for the earley item
	/// default value is 0, which means it can be garbaged
	/// non-zero implies, its not to be collected
	std::map<item, int_t> refi;
	/// items ready for collection
	std::set<item> gcready;
	std::map<std::pair<size_t, size_t>, std::vector<const item*> >
		sorted_citem, rsorted_citem;

	/// binarized temporary intermediate non-terminals
	std::map<std::vector<lit<C, T>>, lit<C, T>> bin_tnt;
	size_t tid; /// id for temporary non-terminals

	// helpers
	lit<C, T> get_lit(const item& i) const;
	lit<C, T> get_nt(const item& i) const;
	std::basic_string<C> get_fresh_tnt();
	std::vector<item> back_track(const item& obj);
	void remove_item(const item& i);
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
	/// returns number of literals for a given item
	size_t n_literals(const item& i) const;
	std::pair<item, bool> get_conj(size_t set, size_t prod, size_t con)
									const;
	void pre_process(const item& i);
#ifdef PARSER_BINTREE_FOREST
	tref init_forest(const lit<C, T>& start_lit, const parse_options& po);
	tref build_forest(const pnode& root);
#else
	bool init_forest(pforest& f, const lit<C, T>& start_lit,
						const parse_options& po);
	bool build_forest(pforest& f, const pnode& root);
#endif
	bool binarize_comb(const item&, pnodes_set&);
	void sbl_chd_forest(const item&, pnodes&, size_t, pnodes_set&);
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
prods<char32_t, char32_t> operator+(const prods<char32_t, char32_t>& x,
	const std::string& s);
prods<char32_t, char32_t> operator|(const prods<char32_t, char32_t>& x,
	const std::string& s);
prods<char32_t, char32_t> operator&(const prods<char32_t, char32_t>& x,
	const std::string& s);

#ifdef DEBUG
template<typename C = char, typename T = C>
std::ostream& print_grammar(std::ostream& os, const grammar<C, T>& g);
template<typename C>
std::ostream& print_dictmap(std::ostream& os,
	const std::map<std::basic_string<C>, size_t>& dm);
#endif // DEBUG

} // idni namespace

/// Hash specialization for lit
template<typename C, typename T>
struct std::hash<idni::lit<C, T>> {
	size_t operator()(const idni::lit<C,T>& l) const noexcept {
		return l.hashit();
	}
};

// Hash for pnode
template <typename C, typename T>
struct std::hash<idni::pnode_type<C,T>> {
	size_t operator()(const idni::pnode_type<C,T>& pn) const noexcept {
		return pn.hash;
	}
};

// template definitions
#include "grammar.tmpl.h"       // for grammar and related
#include "parser.tmpl.h"        // for parser
#include "parser_tree.tmpl.h" // for parser::tree
#include "parser_result.tmpl.h" // for parser::result
#include "get_shaped_tree2.tmpl.h"
#include "tgf.h"                // Tau Grammar Format

#ifdef DEBUG
#include "utility/devhelpers.h"   // various helpers for converting forest
#endif // DEBUG

#include "parser.impl.h"

// undef local macros
#undef DEFAULT_BINARIZE
#undef DEFAULT_INCR_GEN_FOREST

#ifndef TAU_PARSER_BUILD_HEADER_ONLY
// explicit template instantiations to avoid recompilation
extern template class idni::parser<char, char>;
#endif

#endif // __IDNI__PARSER__PARSER_H__
