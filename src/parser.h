// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__PARSER_H__
#define __IDNI__PARSER__PARSER_H__
#include <array>
#include <variant>
#include <iostream>
#include <istream>
#include <algorithm>
#include <initializer_list>
#include <map>
#include <optional>
#include <set>

#include "ankerl/unordered_dense.h"

#include "defs.h"
#include "utility/memory_map.h"
#include "utility/characters.h"
#include "utility/charclasses.h"
#include "utility/term_colors.h"
#include "utility/tree.h"
#include "utility/forest.h"

#include "parser_strings.h"

#define DEFAULT_BINARIZE false
#define DEFAULT_INCR_GEN_FOREST false
#define DEFAULT_ENABLE_GC false
#define DEFAULT_GC_LAG 1

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
	/// Identifies this table across the process-wide node store, so two
	/// tables with an equal-id nonterminal do not hash as equal.
	size_t id() const { return id_; }
private:
	std::map<std::basic_string<C>, size_t> m;
	static size_t next_id() { static size_t counter = 0; return counter++; }
	size_t id_ = next_id();
};

/// Literal containing terminal (c where if c = 0 then null) or nonterminal (n)
template <typename C = char, typename T = C>
struct lit {
	std::variant<size_t, T> data;
	const nonterminals<C, T>* nts = 0;
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
	lit(size_t n, const nonterminals<C, T>* nts);
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
		std::uint64_t seed = grcprime;
		hash_combine(seed, static_cast<bool>(nt()));
		// hash what operator== compares: the table for a nonterminal,
		// the null flag for a terminal
		if (nt()) hash_combine(seed, n(), nts ? nts->id() : (size_t)0);
		else hash_combine(seed, t(), is_null_);
		return static_cast<size_t>(seed);
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
	const std::vector<std::string>& cc_fn_names, nonterminals<C, T>& nts,
	idni::diagnostics::report* diag = nullptr);

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


/**
 * @brief Values a dynamic_grow_nts hook has confirmed across parses.
 *
 * Supplied through parser::parse_options::dynamic_ctx; a parser owns one
 * internally for when that pointer is null. Two parsers (or two parses)
 * using different containers never see each other's grown values.
 */
template <typename C = char>
struct dynamic_context {
	/// Nonterminal id -> values kept from earlier successful parses.
	std::map<size_t, std::set<std::basic_string<C>>> values = {};
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
	template <typename, typename> friend class parser;
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
		/// Alternatives supplied by the host for a @dynamic nonterminal.
		std::map<std::basic_string<C>, std::vector<std::basic_string<C>>>
			dynamic = {};
	} opt;
	grammar(nonterminals<C, T>& nts, options opt = {});
	grammar(nonterminals<C, T>& nts, const prods<C, T>& ps,
		const prods<C, T>& start, const char_class_fns<T>& cc_fns,
		options opt = {});
	/// Sets guards of enabled productions
	void set_enabled_productions(const std::set<std::string>&);
	void productions_enable(const std::string& guard);
	void productions_disable(const std::string& guard);
	/**
	 * Adds an alternative for every value of a @dynamic nonterminal nt.
	 * Every value becomes a terminal string production. A value added
	 * already is skipped. The grammar object is shared by every parser
	 * using it, so this affects all of them and it must not run while a
	 * parse is running.
	 */
	void add_dynamic(const std::basic_string<C>& nt,
		const std::vector<std::basic_string<C>>& values);
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
	/**
	 * Adds one pending alternative to a @dynamic nonterminal l, skipping
	 * the full rebuild add_dynamic does (same trade as
	 * add_char_class_production). Dedups against dynamic_idx_: a value
	 * already host or committed adds nothing; a value already pending for
	 * l from another registered parent is not re-added, but the calling
	 * parent's pair joins that entry's grown_by so either parent can
	 * still confirm it; a retired value is re-linked with its old index.
	 * Returns the production index, or nullopt if nothing was added (an
	 * empty value, or a value already host or committed). The caller
	 * resolves the value's span against its own input, so this takes a
	 * plain string rather than depending on parser::input.
	 */
	std::optional<size_t> add_dynamic_production_from(const lit<C, T>& l,
		const std::basic_string<C>& value);
	/**
	 * Decides every value add_dynamic_production_from added since the last
	 * commit or rollback: a confirmed one moves to the committed state and
	 * is written into ctx's values, staying linked in ntsm; an unconfirmed
	 * one is retired the same way rollback_dynamic retires it. Never
	 * writes into opt.dynamic, which holds @dynamic host values only.
	 * Returns true if anything was confirmed.
	 */
	bool commit_dynamic(dynamic_context<C>& ctx);
	/**
	 * Retires every value add_dynamic_production_from added since the last
	 * commit or rollback, confirmed or not. A retired production keeps its
	 * place in G, so this call never invalidates a production index: only
	 * its index leaves ntsm and its entry moves to the retired state.
	 * predict() reads ntsm fresh on every call, so a retired production
	 * stops matching from that point on. A retired value reuses its
	 * production's index if it is added again.
	 */
	void rollback_dynamic();
	/// True when add_dynamic_production_from added a value not yet
	/// decided by commit_dynamic or rollback_dynamic.
	bool has_pending_dynamic() const { return !pending_.empty(); }
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
	std::ostream& check_nullable_recursive_production(
		std::ostream& os) const;
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
	lit<C, T> nt(size_t n) const;
	/**
	 * Returns a literal of a nonterminal named s. It is added into
	 * nonterminals if it's not contained already.
	 */
	lit<C, T> nt(const std::basic_string<C>& s);
	const lit<C, T>& get_start() const;
	// Bottom-up derivation fixpoint.
	// seeds: (nonterminal_literal, position) pairs representing known memberships.
	// Returns: set of all derivable (literal, span) pairs via unit-rule closure.
	std::set<std::pair<lit<C,T>, std::array<size_t,2>>>
	derive_all(const std::vector<std::pair<lit<C,T>, size_t>>& seeds) const;
private:
	bool all_nulls(const lits<C, T>& a) const;
	/// Adds each value as a host value for l: a brand new production, or
	/// an existing pending/committed/retired one promoted to host so
	/// sync_dynamic_context never retires it. Shared by the
	/// constructor's opt.dynamic loop and add_dynamic.
	void host_dynamic_values(const lit<C, T>& l,
		const std::vector<std::basic_string<C>>& values);
	void compute_nullables();
	/**
	 * Relinks every nonterminal dynamic_idx_ or ctx has an opinion about
	 * so exactly ctx's values are committed, alongside host values, which
	 * stay live regardless. A committed value absent from ctx is retired;
	 * a ctx value without a live production is (re)linked, reusing a
	 * retired index when dynamic_idx_ already has one. Called once per
	 * parser::_parse() call, before the Earley algorithm itself starts.
	 */
	void sync_dynamic_context(const dynamic_context<C>& ctx);
	/**
	 * Marks a pending value added by add_dynamic_production_from as
	 * confirmed, identified by the completing (parent, l) pair (matched
	 * against an entry's grown_by tags, not l's own target literal) and
	 * value, given in the terminal alphabet (matching what
	 * parser::input::get_terminals resolves a span to). A no-op if no
	 * pending entry matches. parser calls this when an item whose head is
	 * a registered parent completes, passing that head as parent.
	 */
	void confirm_dynamic(size_t parent, const lit<C, T>& l,
		const std::basic_string<T>& value);
	/// True iff idx names a dynamic production currently retired.
	bool is_retired(size_t idx) const;
	/// Links idx into ntsm. The only place a dynamic entry gets linked.
	void link(size_t idx);
	/// Marks idx's entry retired and unlinks it from ntsm. The only
	/// place a dynamic entry gets unlinked.
	void retire(size_t idx);
	/// Removes idx from pending_, for an entry promoted out of the
	/// pending state without going through commit_dynamic or
	/// rollback_dynamic (a host or context value grown earlier this
	/// parse, then claimed before the parse's own end decides it).
	void unpend(size_t idx);
	nonterminals<C, T>& nts;
	lit<C, T> start;
	char_class_fns<T> cc_fns = {};
	std::map<lit<C, T>, std::set<size_t>> ntsm = {};
	/// ntsm restricted to nonterminals, indexed by nonterminal id, so the
	/// per-prediction lookup is O(1) instead of a map walk over lit<=>.
	std::vector<std::set<size_t>> ntsm_by_nt = {};
	std::map<size_t, size_t> grdm = {};
	std::vector<std::string> guards = {};
	std::set<size_t> nullables = {};
	std::set<size_t> conjunctives = {};
	std::vector<production> G;
	/// A dynamic production's full lifecycle state. host: from opt.dynamic
	/// or add_dynamic, never retired by sync_dynamic_context. committed:
	/// confirmed by a parse, or supplied by sync_dynamic_context's ctx.
	/// pending: grown by add_dynamic_production_from this parse, not yet
	/// decided. retired: linked into neither ntsm nor any live parse.
	struct dynamic_entry {
		enum class state { host, committed, pending, retired };
		state st;
		lit<C, T> l;
		std::basic_string<C> value;
		/// value in the terminal alphabet, matching what
		/// parser::input::get_terminals resolves a span to; this is
		/// what confirm_dynamic compares against.
		std::basic_string<T> value_t;
		/// Meaningful in pending: every (parent, child) dynamic_grow_nts
		/// pair this grow is associated with, for confirm_dynamic's
		/// lookup. active_grow_ at each add call for this (l, value), or
		/// ((size_t)-1, l's own id) when a call happens outside a
		/// grow-hook firing. More than one pair joins here when separate
		/// registered parents grow the same value.
		std::set<std::pair<size_t, size_t>> grown_by;
		/// Meaningful in pending: whether some registered parent has
		/// completed over this value's span during the current parse.
		bool confirmed = false;
	};
	/// The single owner of dynamic lifecycle state, keyed by the
	/// production's index in G.
	std::map<size_t, dynamic_entry> dynamic_ = {};
	/// (l, value) -> production index. Append-only and never erased, so
	/// a production index is never reused for another value: this cannot
	/// drift from dynamic_.
	std::map<lit<C, T>, std::map<std::basic_string<C>, size_t>>
		dynamic_idx_ = {};
	/// Production indices with dynamic_[idx].st == pending, in the order
	/// add_dynamic_production_from grew them this parse. Cleared by
	/// commit_dynamic and rollback_dynamic.
	std::vector<size_t> pending_ = {};
	/// The (parent, child) pair parser is currently firing
	/// on_dynamic_grow for, read by add_dynamic_production_from to tag
	/// new entries; parser sets this immediately around the callback.
	/// ((size_t)-1, (size_t)-1) outside a firing.
	std::pair<size_t, size_t> active_grow_ =
		{ static_cast<size_t>(-1), static_cast<size_t>(-1) };
};

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
		std::uint64_t seed = this->first.hashit();
		hash_combine(seed, this->second[0], this->second[1]);
		return static_cast<size_t>(seed);
	}
	//inline lit<C,T> &first() const { return this->first; }
	//inline std::array<size_t, 2>& second() const { return this->second; }
};

/// Runtime selection between building a parse forest eagerly or a binary tree (tref).
/// - forest_path: build forest from Earley S-items via sbl_chd_forest, derive tree later if needed.
/// - bintree_path: build a binary tree (tref) directly from S-items via build_bintree, reconstruct forest later if needed.
enum class parse_tree_path { forest_path, bintree_path };

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
	using label           = idni::parser_strings::label;

	using pnode = pnode_type<C, T>;
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
	using pforest         = forest<pnode_type<C, T>>;
	using pnode_graph     = pforest::node_graph;
	using pgraph          = pforest::graph;
	using forest_type     = pforest;
	using pnodes          = pforest::nodes;
	using pnodes_set      = pforest::nodes_set;
	using ptree           = pforest::tree;
	using psptree         = pforest::sptree;
	using sptree_type     = psptree;
	using encoder_type    = std::function<std::basic_string<char_type>(
					const std::vector<terminal_type>&)>;
	using counters        = idni::parser_strings::counters;

	/// earley item
	struct item {
		item(size_t set, size_t prod,size_t con,size_t from,size_t dot);
		bool operator<(const item& i) const;
		bool operator==(const item& i) const;
		size_t set, prod, con, from, dot;
	};
	struct item_hash {
		size_t operator()(const item& i) const {
			std::uint64_t seed = grcprime;
			hash_combine(seed, i.set, i.prod, i.con, i.from, i.dot);
			return static_cast<size_t>(seed);
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
			int_type e = std::char_traits<C>::eof(),
			idni::diagnostics::report* diag = nullptr);
#ifdef _WIN32
		input(const std::wstring& filename, size_t max_length = 0,
			decoder_type decoder = 0,
			int_type e = std::char_traits<C>::eof(),
			idni::diagnostics::report* diag = nullptr);
#else
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
		bool at_eof() const;
		/// Produces a string containing human readable information about the error.
		std::string to_str(info_lvl lv = INFO_DETAILED,
			size_t line_start = 0) const;
	};
	/// Parse options for parse() call
	struct parse_options {
		/// Read up to max length of the input size
		size_t max_length = 0;
		/// Start non-terminal, SIZE_MAX = use default
		size_t start = SIZE_MAX;
		/// End of a stream
		C eof = std::char_traits<C>::eof();
		/// Record timed scopes in diagnostics report
		bool measure_scopes = false;
		/// Record parse counters and RSS metrics in report
		bool measure_counters = false;
		/// Measure time taken for parsing (legacy aggregate flag,
		/// distinct from the scope-level measure_scopes).
		bool measure = false;
		/// For each string pos
		bool measure_each_pos = false;
		/// Forest building scope
		bool measure_forest = false;
		/// Preprocessing scope
		bool measure_preprocess = false;
		/// Enables `if (po.debug)` diagnostic print paths inside the
		/// build_bintree / init_forest helpers (e.g. "preprocess size: N",
		/// "sorted sizes : ..."). Independent from the DBG()/DBGP() macros,
		/// which are compile-time gated.
		bool debug = false;
		/// Verbosity used when recording a parse error into the diagnostics
		/// report. Plumbed from the CLI/REPL `error-verbosity` option so
		/// `basic` / `detailed` / `root-cause` settings actually take effect.
		typename error::info_lvl error_verbosity = error::INFO_DETAILED;
		/// Runtime tree path selection: build forest eagerly (forest_path) or
		/// build binary tree directly (bintree_path, the default).
		parse_tree_path tree_path = parse_tree_path::bintree_path;
		/// Enable garbage collection
		bool enable_gc = DEFAULT_ENABLE_GC;
		/// Garbage collection lag
		size_t gc_lag = DEFAULT_GC_LAG;
		/// Values this parse can match, and where its confirmed grows
		/// are kept. Null uses the parser's own internal container.
		dynamic_context<C>* dynamic_ctx = nullptr;
	};
	/// Bundled decode + encode for character-terminal conversion.
	template <typename C2, typename T2>
	struct terminal_codec {
		/// Decode input chars -> terminals.  Called by parser::input::tcur().
		/// May consume 0..N input bytes, emit 0..N terminals.
		using decode_fn = std::function<std::vector<T2>(
			typename parser<C2,T2>::input&)>;

		/// Encode terminals → input chars.
		using encode_fn = std::function<std::basic_string<C2>(
			const std::vector<T2>&)>;

		decode_fn decode = {};
		encode_fn encode = {};
	};

	/// Parser options for its constructor
	struct options {
		/// Applying binarization to ensure every forest node
		/// has atmost 2 or less children nodes
		bool binarize = DEFAULT_BINARIZE;
		/// Build forest incrementally as soon any
		/// item is completed
		bool incr_gen_forest = DEFAULT_INCR_GEN_FOREST;
		/// Bundled decode + encode for character-terminal conversion.
		terminal_codec<C,T> codec;

		/// Default parse options for parse call.
		/// Default per-parse options for this parser. The parse(...)
		/// overloads that take a parse_options use it for that one parse
		/// and restore this default afterwards.
		parse_options parse_opts = {};

		/// Parent nonterminal id, child nonterminal id. The parent
		/// advancing past a registered child grows the grammar; the
		/// parent's own completion later confirms exactly the spans it
		/// grew. A pair whose parent equals its child is dropped
		/// wherever this option is consumed (the constructor,
		/// set_dynamic_grow). Empty disables the hook.
		std::set<std::pair<size_t, size_t>> dynamic_grow_nts = {};

		/// Fired when a parent in dynamic_grow_nts advances to a
		/// strictly greater input position after consuming a registered
		/// child. Args: the input, the child id, and the child span
		/// [from, to).
		using dynamic_grow_fn =
			std::function<void(input&, size_t, size_t, size_t)>;
		/// Called for progress past a child of any pair in
		/// dynamic_grow_nts. Unset (default) disables the hook.
		dynamic_grow_fn on_dynamic_grow = {};
	};

	/// Result of the parse call.
	struct result {
		using label = idni::parser_strings::label;
		/// True if the parse was successful
		const bool found;
		/// Contains error information if the parse was unsuccessful
		const error parse_error;
		/// Tree shaping options from grammar
		const shaping_options shaping;
		/// Whether parse recorded timed scopes. Copied at construction
		/// from parse_options because `parser::po` may be overwritten by
		/// a subsequent parse before the result is consumed lazily.
		const bool measure_scopes;
		/// Whether parse recorded counters and RSS metrics. Same
		/// lifetime rationale as measure_scopes.
		const bool measure_counters;
		[[nodiscard]] idni::diagnostics::report& report() & {
			return diag_report;
		}
		[[nodiscard]] const idni::diagnostics::report& report() const& {
			return diag_report;
		}
		[[nodiscard]] idni::diagnostics::report&& report() && {
			return std::move(diag_report);
		}

		[[nodiscard]] bool has_error() const {
			return diag_report.has_error();
		}

		result(parser& p, std::unique_ptr<input> in_,
			tref f, bool found, error err);
		result(parser& p, std::unique_ptr<input> in_,
			std::unique_ptr<pforest> f, bool found, error err);
		parser& get_parser() const;

		/// FOREST APIs

		/// Returns the parsed forest. In bintree mode this reconstructs a
		/// forest lazily; use bintree APIs to avoid that reconstruction.
		pforest* get_forest() const;

		/// `psptree` APIs. These are the original
		/// forest-backed tree APIs and may reconstruct the forest when the
		/// result was produced in bintree mode.
		psptree get_trimmed_forest_tree(const pnode& n) const;
		psptree get_trimmed_forest_tree(const pnode& n,
			const shaping_options opts) const;
		psptree inline_forest_tree_nodes(const psptree& t,
			psptree& parent) const;
		psptree inline_forest_tree_nodes(const psptree& t,
			psptree& parent, const shaping_options opts) const;
		psptree inline_forest_tree_paths(const psptree& t) const;
		psptree inline_forest_tree_paths(const psptree& t,
			const shaping_options opts) const;
		psptree inline_forest_tree(psptree& t) const;
		psptree inline_forest_tree(psptree& t,
			const shaping_options opts) const;
		psptree trim_forest_tree_child_terminals(const psptree& t) const;
		psptree trim_forest_tree_child_terminals(const psptree& t,
			const shaping_options opts) const;
		psptree get_shaped_forest_tree() const;
		psptree get_shaped_forest_tree(const shaping_options opts) const;
		psptree get_shaped_forest_tree(const pnode& n) const;
		psptree get_shaped_forest_tree(const pnode& n,
			const shaping_options opts) const;
		psptree get_forest_tree();
		psptree get_forest_tree(const pnode& n);

		/// Legacy names for forest-tree (`psptree`) APIs.
		psptree get_trimmed_tree(const pnode& n) const;
		psptree get_trimmed_tree(const pnode& n,
			const shaping_options opts) const;
		psptree inline_tree_nodes(const psptree& t, psptree& parent)
			const;
		psptree inline_tree_nodes(const psptree& t, psptree& parent,
			const shaping_options opts) const;
		psptree inline_tree_paths(const psptree& t) const;
		psptree inline_tree_paths(const psptree& t,
			const shaping_options opts) const;
		psptree inline_tree(psptree& t) const;
		psptree inline_tree(psptree& t,
			const shaping_options opts) const;
		psptree trim_children_terminals(const psptree& t) const;
		psptree trim_children_terminals(const psptree& t,
			const shaping_options opts) const;
		psptree get_shaped_tree() const;
		psptree get_shaped_tree(const shaping_options opts) const;
		psptree get_shaped_tree(const pnode& n) const;
		psptree get_shaped_tree(const pnode& n,
			const shaping_options opts) const;
		psptree get_tree();
		psptree get_tree(const pnode& n);

		/// BINTREE APIs

		/// aka `tref` APIs. These are native in bintree mode and
		/// derive a bintree from the forest when the result was produced in
		/// forest mode.
		tref get_trimmed_bintree(tref ref) const;
		tref get_trimmed_bintree(tref ref, const shaping_options opts) const;
		tref inline_bintree_nodes(tref t) const;
		tref inline_bintree_nodes(tref t, const shaping_options opts) const;
		tref inline_bintree_paths(tref t) const;
		tref inline_bintree_paths(tref t, const shaping_options opts) const;
		tref inline_bintree(tref t) const;
		tref inline_bintree(tref t, const shaping_options opts) const;
		tref trim_bintree_child_terminals(tref ref) const;
		tref trim_bintree_child_terminals(tref ref,
			const shaping_options opts) const;
		tref get_shaped_bintree();
		tref get_shaped_bintree(const shaping_options opts);
		tref get_shaped_bintree(tref t);
		tref get_shaped_bintree(tref t, const shaping_options opts);
		tref get_bintree();
		/// Returns the bintree rooted at pnode n. In bintree mode this
		/// searches froot directly and preserves an __AMB__ wrapper when n
		/// is ambiguous; it does not reconstruct the forest.
		tref get_bintree(const pnode& n);

		/// Legacy names for bintree (`tref`) APIs.
		tref get_trimmed_tree2(tref ref) const;
		tref get_trimmed_tree2(tref ref, const shaping_options opts) const;
		tref inline_tree_nodes2(tref t) const;
		tref inline_tree_nodes2(tref t, const shaping_options opts) const;
		tref inline_tree_paths2(tref t) const;
		tref inline_tree_paths2(tref t, const shaping_options opts) const;
		tref inline_tree2(tref t) const;
		tref inline_tree2(tref t, const shaping_options opts) const;
		tref trim_children_terminals2(tref ref) const;
		tref trim_children_terminals2(tref ref,
			const shaping_options opts) const;
		tref get_shaped_tree2();
		tref get_shaped_tree2(const shaping_options opts);
		tref get_shaped_tree2(tref t);
		tref get_shaped_tree2(tref t, const shaping_options opts);
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
		using edges           = std::vector<typename pforest::edge>;
		using nodes_and_edges = std::pair<std::vector<node_type>, edges>;
		/// Returns all nodes and edges of the forest
		nodes_and_edges get_nodes_and_edges() const;

		/// Removes EBNF and binarize transformation prefixes
		bool inline_grammar_transformations(pgraph& g);
		// removes all prefixed symbols from the graph everywhere
		// by replacing them with their immediate children nodes
		bool inline_prefixed_nodes(pgraph& g,
			const std::basic_string<C>& prefix);
		// removes all nodes with the given nt from the graph everywhere
		bool inline_nodes(pgraph& g,
				  const std::set<size_t>& nts_to_inline);
		// private members are accessible by parser
		friend parser;
		const lit<C, T>& ambiguity_literal() const;
	private:
		/// Bintree-mode helper used by ambiguous_nodes() when the
		/// result holds a bintree (froot != 0).
		std::set<std::pair<pnode, pnodes_set>>
			ambiguous_nodes_from_bintree() const;
		/// Forest-mode helper used by ambiguous_nodes() when the
		/// result holds a built forest (f != null).
		std::set<std::pair<pnode, pnodes_set>>
			ambiguous_nodes_from_forest() const;
		/// Metrics and diagnostics from the parse. Mutable so that
		/// lazy operations on a const @ref result (e.g. @ref get_forest)
		/// can still record timing scopes.
		mutable idni::diagnostics::report diag_report;
		parser& p;
		// input moved here from the parse call
		std::unique_ptr<input> in_ = 0;
		// In bintree mode, the parse stores the tref root here.
		// In forest mode this is 0 (the forest is built eagerly).
		htref froot = 0;
		// The parsed forest. In forest mode it is populated by parse().
		// In bintree mode it is null until get_forest() lazily builds it
		// from `froot`. `mutable` so lazy build stays inside const
		// get_forest().
		mutable std::unique_ptr<pforest> f = 0;
		/// Filters nonterminals by prefixes
		std::set<size_t> get_nts_by_prefixes(
			const std::set<std::basic_string<C>>& prefixes) const;
		// if ambiguous, this is __AMB__ node lit used in a shaped tree
		lit<C, T> amb_node{};
		tref shape_tree2_impl(tref t, const shaping_options& opts);
		/// Recursive part of get_shaped_tree()
		void _get_shaped_tree_children(const shaping_options& opts,
			const pnodes& nodes,
			std::vector<psptree>& child) const;
	};

	// constructor
	parser(grammar<C, T>& g, options o = {});
	virtual ~parser() {};

	// parse call
	result parse(const C* data, size_t size);
	result parse(const C* data, size_t size, parse_options popts);
	result parse(std::basic_istream<C>& is);
	result parse(std::basic_istream<C>& is, parse_options popts);
	result parse(const std::string& fn);
	result parse(const std::string& fn, parse_options popts);
#ifdef _WIN32
	result parse(const std::wstring& fn);
	result parse(const std::wstring& fn, parse_options popts);
#else
	result parse(int filedescriptor);
	result parse(int filedescriptor, parse_options popts);
#endif
	grammar<C, T>& get_grammar() { return g; }
	const grammar<C, T>& get_grammar() const { return g; }
	/**
	 * Sets the dynamic grow hook (options::on_dynamic_grow and
	 * options::dynamic_grow_nts) on a parser already constructed, such
	 * as a generated singleton whose options are built at construction.
	 * Call this between parses. Returns false and changes nothing if any
	 * pair has an equal parent and child.
	 */
	bool set_dynamic_grow(typename options::dynamic_grow_fn fn,
		std::set<std::pair<size_t, size_t>> nts)
	{
		if (!dynamic_grow_nts_valid(nts)) return false;
		o.on_dynamic_grow = fn;
		o.dynamic_grow_nts = std::move(nts);
		return true;
	}
	bool debug = false;
	std::pair<size_t, size_t> debug_at = { DEBUG_POS_FROM, DEBUG_POS_TO };
private:
	/// True unless some pair has an equal parent and child. Such a pair
	/// would confirm on the child's own completion, confirming every
	/// prefix: exactly the defect dynamic_grow_nts's split design removes.
	static bool dynamic_grow_nts_valid(
		const std::set<std::pair<size_t, size_t>>& nts)
	{
		for (const auto& [p, c] : nts) if (p == c) return false;
		return true;
	}
	using container_t    = ankerl::unordered_dense::set<item, item_hash>;
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
	/// Streams the full diagnostics report (scopes, counters, errors,
	/// infos). For error-only output use res.parse_error.to_str(...).
	friend std::ostream& operator<<(std::ostream& os, const result& res) {
		return os << res.report();
	}
private:
	mutable idni::diagnostics::report report_;
	MC(mutable counters cnt;)
	grammar<C, T>& g;
	options o;
	parse_options po; /// current parse options
	std::unique_ptr<input> in_ = 0;
	bool found(size_t start = SIZE_MAX);
	error get_error();
	std::vector<container_t> S;
	std::vector<container_t> U; /// uncompleted
	/// reused across fixpoint iterations so the item snapshot allocates once
	std::vector<item> snapshot_ = {};
		///mapping from to position of end in S for items
	ankerl::unordered_dense::map<size_t,
		ankerl::unordered_dense::set<size_t>> fromS;
	/// true iff fromS writes are needed this parse (enable_gc || any_conj)
	bool need_fromS = false;
	ankerl::unordered_dense::map<std::pair<size_t /*nt_id*/, size_t>,
		container_t> cache;

	/// refcounter for the earley item
	/// default value is 0, which means it can be garbaged
	/// non-zero implies, its not to be collected
	std::map<item, int_t> refi;
	/// items ready for collection
	container_t gcready;
	ankerl::unordered_dense::map<std::pair<size_t, size_t>,
		std::vector<item>> sorted_citem, rsorted_citem;

	/// completion key (nt_id, from, set) for dependency tracking
	using completion_key = std::tuple<size_t, size_t, size_t>;
	std::unordered_map<completion_key, std::vector<item>>
		completion_deps;
	/// O(1) "is anything still completed?" predicate.
	ankerl::unordered_dense::map<completion_key, size_t> completion_count;
	/// completed items currently counted in completion_count (one count per live item)
	ankerl::unordered_dense::set<item, item_hash> counted_completions;
	/// predecessor->derived edges: items produced by advancing the key item
	ankerl::unordered_dense::map<item, std::vector<item>, item_hash>
		forward_deps;
	/// True iff any production in `g` is conjunctive. Cascade machinery
	/// is dead code when this is false, so its bookkeeping is skipped.
	bool any_conj = false;
	/// Per-item memoization for complete(): index into the underlying
	/// cache vector at the last call. complete() only re-processes cache
	/// entries [last, current_size) instead of the whole cache. The cache
	/// only grows during a parse, so indices into its underlying vector
	/// are stable.
	ankerl::unordered_dense::map<item, size_t, item_hash> complete_memo;
	/// One registered child's span, consumed on the way to some item's
	/// own parent in o.dynamic_grow_nts. fired marks that on_dynamic_grow
	/// already ran for this span, so a later propagation step (the entry
	/// is copied forward as-is) does not run it again.
	struct dyn_child_entry { size_t child_nt, from, to; bool fired = false; };
	/// Per-parse: item -> the registered children's spans consumed on
	/// the way to that item, one entry per child completion crossed.
	/// Propagated forward through complete() and scan() as an item's
	/// remaining literals match. on_dynamic_grow fires there, for an
	/// entry whose span the parent has just progressed strictly past;
	/// read again when a parent completes, filtered to entries whose
	/// (parent, child_nt) is a registered pair, to confirm them.
	/// Cleared at the start of each parse.
	ankerl::unordered_dense::map<item, std::vector<dyn_child_entry>,
		item_hash> dyn_child_span;
	/// Nonterminal ids that are a child in some o.dynamic_grow_nts pair.
	/// Left empty when on_dynamic_grow is unset, so a registered pair
	/// with no callback never grows an annotation to fire on. Recomputed
	/// at the start of each parse.
	std::set<size_t> dyn_child_ids;
	/// Used by parse_options::dynamic_ctx when null.
	dynamic_context<C> internal_dynamic_ctx;
	/// Merges incoming into dyn_child_span[j]: an entry whose
	/// (child_nt, from, to) is not already there is appended; a present
	/// one keeps fired once either side has set it. Never drops an
	/// entry another derivation already recorded for j.
	void merge_dyn_child_span(const item& j,
		const std::vector<dyn_child_entry>& incoming)
	{
		auto it = dyn_child_span.find(j);
		std::vector<dyn_child_entry> merged = it != dyn_child_span.end()
			? it->second : std::vector<dyn_child_entry>{};
		for (const auto& e : incoming) {
			auto mit = std::find_if(merged.begin(), merged.end(),
				[&e](const dyn_child_entry& m) {
					return m.child_nt == e.child_nt &&
						m.from == e.from && m.to == e.to;
				});
			if (mit == merged.end()) merged.push_back(e);
			else mit->fired = mit->fired || e.fired;
		}
		// copy-then-insert: dyn_child_span is a flat hashmap, so this
		// operator[] can reallocate its backing storage; incoming and
		// the old entries are already fully read by this point.
		dyn_child_span[j] = std::move(merged);
	}
	/// Calls o.on_dynamic_grow with g.active_grow_ scoped to
	/// (parent_nt, child_nt), restoring the previous pair after so a
	/// nested firing does not clobber an outer one. A no-op when
	/// on_dynamic_grow is unset, so a registered pair with no callback
	/// never calls an empty std::function.
	void fire_dynamic_grow(size_t parent_nt, size_t child_nt, size_t from,
		size_t to)
	{
		if (!o.on_dynamic_grow) return;
		auto prev = g.active_grow_;
		g.active_grow_ = { parent_nt, child_nt };
		o.on_dynamic_grow(*in_, child_nt, from, to);
		g.active_grow_ = prev;
	}

	/// binarized temporary intermediate non-terminals
	std::map<std::vector<lit<C, T>>, lit<C, T>> bin_tnt;
	size_t tid; /// id for temporary non-terminals

	// helpers
	// Shared body of the preprocess step in build_bintree() and
	// init_forest(): walks every item in every position calling
	// pre_process(), wrapped in a measured scope. Returns the
	// number of items visited (for the caller's debug printing).
	int do_preprocess();

#ifdef TAU_PARSER_MEASURE_COUNTERS
	void count(size_t& c, size_t n = 1) {
		if (po.measure_counters) c += n;
	}
	void maks(size_t& peak, size_t v) {
		if (po.measure_counters)
			peak = std::max(peak, v);
	}
#endif
	lit<C, T> get_lit(const item& i) const;
	lit<C, T> get_nt(const item& i) const;
	std::basic_string<C> get_fresh_tnt();
	std::vector<item> back_track(const item& obj);
	void remove_item(const item& i);
	// parsing
	result _parse();
	result _parse(const parse_options& po);
	std::pair<container_iter, bool> add(container_t& t, const item& i);
	bool nullable(const item& i) const;
	void resolve_conjunctions(container_t& c);
	void cascade_uncomplete(size_t nt_id, size_t from, size_t set,
		container_t& c);
	void retract_item(const item& x, container_t& c);
	bool nt_still_completed(size_t nt_id, size_t from, size_t set) const;
	/// Predict the productions of the nonterminal after the dot of `i`.
	/// `ch` is the character at the item's set; a production whose first
	/// literal is a terminal or character-class function that cannot
	/// match it is not predicted (one-character lookahead).
	void predict(const item& i, container_t& t, T ch);
	/// true iff conjunct `c` of production `p` may start with `ch`.
	bool first_can_match(size_t p, size_t c, T ch) const;
	/// Predict, without lookahead, everything predictable at set `n`,
	/// so an error report lists every alternative the lookahead skipped.
	void predict_all_at(size_t n);
	void scan(const item& i, size_t n, T ch);
	void scan_cc_function(const item& i, size_t n, T ch, container_t& t);
	void complete(const item& i, container_t& t, container_t& c,
						bool conj_resolved = false);
	/// Confirms every registered child span x's completion grew, once x
	/// is genuinely completed (not a conjunct parked for later
	/// resolution). Called from complete(), never for a negative x.
	void confirm_dynamic_parent(const item& x);
	bool completed(const item& i) const;
	bool negative(const item& i) const;
	/// returns number of literals for a given item
	size_t n_literals(const item& i) const;
	std::pair<item, bool> get_conj(size_t set, size_t prod, size_t con)
									const;
	void pre_process(const item& i);
	// Init/build forest:
	//   bintree path:  build_bintree returns a tref
	//   forest path: init_forest/build_forest operate on a pforest reference
	tref build_bintree(const lit<C, T>& start_lit, const parse_options& po);
	bool init_forest(pforest& f, const lit<C, T>& start_lit,
						const parse_options& po);
	bool build_forest(pforest& f, const idni::pnode_type<C, T>& root);
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
// every generated parser needs idni::default_parser_options() from here,
// which used to arrive only via tgf.h
#include "recoders.h"
#ifndef TAU_PARSER_NO_TGF
#include "format/tgf/tgf.h" // Tau Grammar Format
#endif // TAU_PARSER_NO_TGF

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
extern template class idni::parser<char, char32_t>;
#endif

#endif // __IDNI__PARSER__PARSER_H__
