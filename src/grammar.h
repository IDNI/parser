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
#ifndef __IDNI__PARSER__GRAMMAR_H__
#define __IDNI__PARSER__GRAMMAR_H__
#include "defs.h"
#include "characters.h"
#include "charclasses.h"
namespace idni {
using namespace idni::charclasses;

// nonterminals dict = vector with index being nonterminal's id
template <typename CharT>
struct nonterminals : public std::vector<std::basic_string<CharT>> {
	typedef CharT char_t;
	typedef std::basic_string<CharT> string;
	typedef std::vector<string> strings;
	std::map<string, size_t> m;
	size_t get(const string& s);
	const string& get(size_t n) const;
};

// literal containing terminal (c where if c = 0 then null) or nonterminal (n)
template <typename CharT>
struct lit : public std::variant<size_t, CharT> {
	typedef CharT char_t;
	typedef std::basic_string<CharT> string;
	typedef std::vector<string> strings;
	typedef std::variant<size_t, CharT> lit_t;
	using typename lit_t::variant;
	nonterminals<CharT>* nts = 0;
	lit() : lit(0) {};
	lit(CharT c) : lit_t(c), nts(0) {};
	lit(size_t n, nonterminals<CharT>* nts)
		: lit_t(n), nts(nts) {};
	bool  nt() const { return std::holds_alternative<size_t>(*this);}
	size_t n() const { return std::get<size_t>(*this); }
	CharT  c() const { return std::get<CharT>(*this); }
	string to_string(const string& nll = {}) const;
	std::string to_std_string(const string& nll = {}) const {
		return idni::to_std_string(to_string(nll));
	}
	bool operator<(const lit& l) const;
	bool operator==(const lit& l) const;
};

// alt as a vector of literals which should match to the parser item.
template <typename CharT>
struct alt : public std::vector<lit<CharT>> {
	bool neg = false;
};

template <typename CharT>
using clause = std::set<alt<CharT>>; // conjunctions of alts
template <typename CharT>
using dnf = std::set<clause<CharT>>; // disjunctions of conjunctions
template <typename CharT>
using prod = std::pair<lit<CharT>, dnf<CharT>>; // production rule
template <typename CharT>
struct prods : public std::vector<prod<CharT>> { // productions
	typedef CharT char_t;
	typedef std::vector<prod<CharT>> prods_t;
	prods() : prods_t() {}
	prods(const lit<CharT>& l) : prods_t() {
		this->emplace_back(lit<CharT>{},
			dnf<CharT>{clause<CharT>{alt<CharT>({l})}});
	}
	prods(const std::basic_string<CharT>& s) : prods_t() {
		alt<CharT> a;
		for (const CharT& c : s) a.emplace_back(c);
		this->emplace_back(lit<CharT>{},
			dnf<CharT>{clause<CharT>{a}});
	}
	void operator()(const prods<CharT>& l, const prods<CharT>& p) {
		(*this)(l.to_lit(), p);
	}
	void operator()(const lit<CharT>& l, const prods<CharT>& p) {
		assert(p.size());
		this->emplace_back(l, p.to_dnf());
	}
	bool operator==(const lit<CharT>& l) const { return l == to_lit(); }
	lit<CharT> to_lit() const {
		assert(this->size() && this->back().second.size() &&
			this->back().second.begin()->size() &&
			this->back().second.begin()->begin()->size());
		return this->back().second.begin()->begin()->back();
	}
	dnf<CharT> to_dnf() const {
		assert(this->size());
		return this->back().second;
	}
};

template <typename CharT>
bool operator==(const lit<CharT>& l, const prods<CharT>& p) {
	return p == l;
}

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

template <typename CharT>
using char_class_fn = std::function<bool(CharT)>;
template <typename CharT>
struct char_class_fns : public std::vector<size_t> {
	std::map<size_t, char_class_fn<CharT>> fns = {};
	std::vector<std::map<CharT, size_t>> ps = {}; //char -> production
	// adds new char class function
	void operator()(size_t nt, const char_class_fn<CharT>& fn);
};

template <typename CharT>
char_class_fns<CharT> predefined_char_classes(
	const std::vector<std::basic_string<CharT>>& cc_fn_names,
	nonterminals<CharT>& nts);

template <typename CharT>
struct parser;

template <typename CharT>
struct grammar {
	friend parser<CharT>;
	typedef CharT char_t;
	typedef std::basic_string<CharT> string;
	typedef std::vector<string> strings;
	typedef lit<CharT> literal;
	typedef alt<CharT> literals;
	typedef std::vector<literals> conjunctions;
	typedef std::pair<literal, conjunctions> production;
	typedef std::vector<production> productions;

	grammar(nonterminals<CharT>& nts, const prods<CharT>& ps,
		const prods<CharT>& start, const char_class_fns<CharT>& cc_fns);
	// returns number of productions (every disjunction has a prod rule)
	size_t size() const;
	// returns head of the prod rule - literal
	literal operator()(const size_t& i) const;
	// returns body of the prod rule - conjunctions of alts of literals
	const conjunctions& operator[](const size_t& i) const;
	// returns length of the conjunction
	size_t len(const size_t& p, const size_t& c) const;
	// returns true if the literal is nullable
	bool nullable(literal l) const;
	// checks if the char class function returns true on the char ch
	// if true then adds new production rule: name-of-cc_fn => ch.
	// returns id of the production rule or (size_t)-1 if the check fails
	size_t char_class_check(literal l, CharT ch);
#if defined(DEBUG) || defined(WITH_DEVHELPERS)
	ostream_t& print_internal_grammar(ostream_t& os, std::string prep = {})
		const;
	ostream_t& print_data(ostream_t& os, std::string prep = {})
		const;
#endif
private:
	nonterminals<CharT>& nts;
	literal start;
	char_class_fns<CharT> cc_fns = {};
	std::map<literal, std::set<size_t>> ntsm = {};
	std::set<size_t> nullables = {};
	productions G;
	literal nt(size_t n);
	literal nt(const string& s);
	bool all_nulls(const literals& a) const;
};

} // idni namespace
#include "grammar.tmpl.h"
#endif // __IDNI__PARSER__GRAMMAR_H__
