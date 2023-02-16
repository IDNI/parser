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
#ifndef __IDNI__PARSER__GRAMMAR_TMPL_H__
#define __IDNI__PARSER__GRAMMAR_TMPL_H__
#include "parser.h"
namespace idni {
using namespace idni::charclasses;

// -----------------------------------------------------------------------------

template <typename C, typename T>
std::ostream& operator<<(std::ostream& os, const lit<C, T>& l) {
	return os << l.to_std_string(std::basic_string<C>{});
}
template <typename C, typename T>
std::ostream& operator<<(std::ostream& os, const lits<C, T>& a) {
	size_t n = 0;
	if (a.neg) os << "~(";
	for (const lit<C, T>& l : a) if (os << l; ++n != a.size()) os << " ";
	if (a.neg) os << ")";
	return os;
}
template <typename C, typename T>
std::ostream& operator<<(std::ostream& os, const clause<C, T>& c) {
	size_t n = 0;
	if (c.size()) os << "(";
	for (const lits<C, T>& a : c)
		if (os << a; ++n != c.size()) os << " & ";
	if (c.size()) os << ")";
	return os;
}
template <typename C, typename T>
std::ostream& operator<<(std::ostream& os, const dnf<C, T>& d) {
	size_t n = 0;
	for (const clause<C, T>& c : d)
		if (os << c; ++n != d.size()) os << " | ";
	return os;
}
template <typename C, typename T>
std::ostream& operator<<(std::ostream& os, const prods<C, T>& p) {
	for (size_t n = 0; n != p.size(); ++n)
		os << '\t' << p.at(n).first.to_std_string() << " => " <<
			p.at(n).second << '.' << std::endl;
	return os;
}

//-----------------------------------------------------------------------------

template <typename C, typename T>
size_t nonterminals<C, T>::get(const std::basic_string<C>& s) {
	if (auto it = m.find(s); it != m.end())
		return it->second;
	return m.emplace(s, this->size()),
		this->push_back(s), this->size() - 1;
}
template <typename C, typename T>
const std::basic_string<C>& nonterminals<C, T>::get(size_t n) const {
	assert(n < this->size());
	return this->at(n);
}
template <typename C, typename T>
lit<C, T> nonterminals<C, T>::operator()(const std::basic_string<C>& s) {
	return lit<C, T>{ get(s), &*this };
}

//-----------------------------------------------------------------------------

template <typename C, typename T>
lit<C, T>::lit() : lit_t((T) 0), is_null_(true) { }
template <typename C, typename T>
lit<C, T>::lit(T c) : lit_t(c), nts(0) { }
template <typename C, typename T>
lit<C, T>::lit(size_t n, nonterminals<C, T>* nts) : lit_t(n), nts(nts) { }
template <typename C, typename T>
bool lit<C, T>::nt() const { return std::holds_alternative<size_t>(*this); }
template <typename C, typename T>
size_t lit<C, T>::n() const { return std::get<size_t>(*this); }
template <typename C, typename T>
T lit<C, T>::t() const { return std::get<T>(*this); }
template <typename C, typename T>
bool lit<C, T>::is_null() const { return is_null_; }

template <typename C, typename T>
std::basic_string<C> lit<C, T>::to_string(const std::basic_string<C>& nll)const{
	if (nt()) return nts->get(n());
	else if (is_null()) return nll;
	else if constexpr (std::is_same_v<T, bool>)
		return from_cstr<C>(t() ? "1" : "0");
	else if constexpr (std::is_same_v<C, T>) return { t() };
	return nll;
}
template <typename C, typename T>
std::string lit<C, T>::to_std_string(const std::basic_string<C>& nll) const {
	return idni::to_std_string(to_string(nll));
}
template <typename C, typename T>
bool lit<C, T>::operator<(const lit<C, T>& l) const {
	if (nt() != l.nt()) return nt() < l.nt();
	if (nt()) return n() == l.n() ? *nts < *l.nts : n() < l.n();
	return t() < l.t();
}
template <typename C, typename T>
bool lit<C, T>::operator==(const lit<C, T>& l) const {
	if (nt() != l.nt()) return false;
	if (nt()) return n() == l.n() && *nts == *l.nts;
	else return t() == l.t();
}

//-----------------------------------------------------------------------------

template <typename C, typename T>
lits<C, T> operator+(const lit<C, T>& x, const lit<C, T>& y) {
	return lits<C, T>({ x, y });
}
template <typename C, typename T>
lits<C, T> operator+(const lits<C, T>& x, const lit<C, T>& l) {
	lits<C, T> a(x);
	return a.push_back(l), a;
}
template <typename C, typename T>
lits<C, T> operator+(const lits<C, T>& x, const lits<C, T>& y) {
	lits<C, T> a(x);
	for (const lit<C, T>& l : y) a.push_back(l);
	return a;
}
template <typename C, typename T>
lits<C, T> operator~(const lits<C, T>& x) {
	lits<C, T> r(x);
	return r.neg = !x.neg, r;
}

//-----------------------------------------------------------------------------

template <typename C, typename T>
bool operator<=(const clause<C, T>& x, const clause<C, T>& y) {
	for (const lits<C, T>& a : x) if (y.find(a) == y.end()) return false;
	return true;
}
template <typename C, typename T>
clause<C, T> simplify(const clause<C, T>& c) {
	for (lits<C, T> na : c) {
		na.neg = !na.neg;
		for (const lits<C, T>& a : c) if (a.neg == na.neg && a == na) {
			//cout << "simplified to {} a["<<a<<"] == na["<<na<<"]\n";
			return {};
		}
	}
	return c;
}
template <typename C, typename T>
clause<C, T> operator&(const clause<C, T>& x, const clause<C, T>& y) {
	clause<C, T> r = x;
	return r.insert(y.begin(), y.end()), simplify<C, T>(r);
}
template <typename C, typename T>
clause<C, T> operator+(const clause<C, T>& x, const clause<C, T>& y) {
	clause<C, T> r;
	for (const lits<C, T>& a : x)
		for (const lits<C, T>& b : y) r.insert(a + b);
	return simplify<C, T>(r);
}

//-----------------------------------------------------------------------------

template <typename C, typename T>
dnf<C, T>& top(prods<C, T>& p) {
	return p.back().second;
}
template <typename C, typename T>
const dnf<C, T>& top(const prods<C, T>& p) {
	return p.back().second;
}
template <typename C, typename T>
dnf<C, T> simplify(const dnf<C, T>& x) {
	dnf<C, T> y, z;
	for (clause<C, T> c : x)
		if (!(c = simplify(c)).empty()) y.insert(c);
	for (clause<C, T> c : y) {
		bool f = false;
		for (clause<C, T> d : y)
			if (c != d && c <= d) { f = true; break; }
		if (!f) z.insert(c);
	}
	return z;
}
template <typename C, typename T>
dnf<C, T> operator|(const dnf<C, T>& x, const dnf<C, T>& y) {
	dnf<C, T> r;
	return r.insert(x.begin(), x.end()), r.insert(y.begin(), y.end()), r;
}
template <typename C, typename T>
dnf<C, T> operator&(const dnf<C, T>& x, const clause<C, T>& y) {
	dnf<C, T> r;
	for (const clause<C, T>& c : x) r.insert(c & y);
	return r;
}
template <typename C, typename T>
dnf<C, T> operator&(const dnf<C, T>& x, const dnf<C, T>& y) {
	dnf<C, T> r;
	for (const clause<C, T>& c : x)
		for (const clause<C, T>& d : (y & c)) r.insert(d);
	return simplify<C, T>(r);
}
template <typename C, typename T>
dnf<C, T> operator+(const dnf<C, T>& x, const clause<C, T>& y) {
	dnf<C, T> r;
	for (const clause<C, T>& c : x) r.insert(c + y);
	return r;
}
template <typename C, typename T>
dnf<C, T> operator+(const dnf<C, T>& x, const dnf<C, T>& y) {
	dnf<C, T> r;
	for (const clause<C, T>& c : x)
		for (const clause<C, T>& d : y) r.insert(c + d);
	return r;
}
template <typename C, typename T>
dnf<C, T> operator~(const clause<C, T>& x) {
	dnf<C, T> r;
	for (const lits<C, T>& a : x) r.insert(clause<C, T>{~a});
	//cout << "\nnegating clause: " << x << " to: " << r << std::endl;
	return r;
}
template <typename C, typename T>
dnf<C, T> operator~(const dnf<C, T>& x) {
	std::set<dnf<C, T>> ncs; // negated conjunctions = disjunctions
	for (const clause<C, T>& c : x) ncs.insert(~c);
	if (ncs.size() == 0) return {}; // empty, return empty dnf
	if (ncs.size() == 1) return *(ncs.begin()); // return first if only one
	// otherwise do combinations
	std::vector<clause<C, T>> rcs; // disjuncted clauses to return
	std::vector<dnf<C, T>> ncsv(ncs.begin(), ncs.end()); // negated conjunctions vector = vector of disjunctions
	std::vector<clause<C, T>> cs(ncsv[0].begin(), ncsv[0].end());
	//cout << "cs("<<cs.size()<<"): " << dnf(cs.begin(), cs.end()) << std::endl;
	for (size_t i = 1; i != ncsv.size(); ++i) {
		std::vector<clause<C, T>> ncs(ncsv[i].begin(), ncsv[i].end());
		//cout << "\tncs("<<ncs.size()<<"): " << dnf(ncs.begin(), ncs.end()) << std::endl;
		for (size_t j = 0; j != ncs.size(); ++j)
			for (size_t k = 0; k != cs.size(); ++k) {
				clause<C, T> c(cs[k]);
				c.insert(*(ncs[j].begin()));
				rcs.push_back(c);
			}
		cs = rcs;
		//cout << "cs*("<<cs.size()<<"): " << dnf(cs.begin(), cs.end()) << std::endl;
	}
	dnf<C, T> r(cs.begin(), cs.end());
	//cout << "\nnegating dnf d: " << x << " to: `" << r << "`" << std::endl;
	return simplify<C, T>(r); // is simplify needed here?
}

//-----------------------------------------------------------------------------

template <typename C, typename T>
void simplify(prods<C, T>& p) {
	for (auto& x : p) x.second = simplify<C, T>(x.second);
}
template <typename C, typename T>
prods<C, T> operator|(const prods<C, T>& x, const prods<C, T>& y) {
	//assert(!x.empty() && !y.empty());
	if (x.empty()) return y;
	if (y.empty()) return x;
	prods<C, T> r;
	for (size_t n = 0; n != x.size() - 1; ++n) r.push_back(x[n]);
	for (size_t n = 0; n != y.size() - 1; ++n) r.push_back(y[n]);
	r.emplace_back(lit<C, T>{}, top<C, T>(x) | top<C, T>(y));
	return r;
}
template <typename C, typename T>
prods<C, T> operator|(const prods<C, T>& x, const C& c) {
	//assert(!x.empty());
	if (x.empty()) return prods<C, T>{ c };
	return x | prods<C, T>{ c };
}
template <typename C, typename T>
prods<C, T> operator|(const prods<C, T>& x, const C* s) {
	if (x.empty()) return prods<C, T>{ std::basic_string<C>(s) };
	return x | std::basic_string<C>(s);
}
template <typename C, typename T>
prods<C, T> operator|(const prods<C, T>& x,const std::basic_string<C>& s){
	//assert(!x.empty());
	if (x.empty()) return prods<C, T>(s);
	return x | prods<C, T>(s);
}
template <typename C, typename T>
prods<C, T> operator&(const prods<C, T>& x, const prods<C, T>& y) {
	//assert(!x.empty() && !y.empty());
	if (x.empty()) return y;
	if (y.empty()) return x;
	prods<C, T> r;
	for (size_t n = 0; n != x.size() - 1; ++n) r.push_back(x[n]);
	for (size_t n = 0; n != y.size() - 1; ++n) r.push_back(y[n]);
	return r.emplace_back(lit<C, T>{}, top<C, T>(x) & top<C, T>(y)), r;
}

template <typename C, typename T>
prods<C, T> operator&(const prods<C, T>& x, const C& c) {
	//assert(!x.empty());
	if (x.empty()) return prods<C, T>{ c };
	return x & prods<C, T>{ c };
}
template <typename C, typename T>
prods<C, T> operator&(const prods<C, T>& x, const C* s) {
	if (x.empty()) return prods<C, T>{ std::basic_string<C>(s) };
	return x & std::basic_string<C>(s);
}
template <typename C, typename T>
prods<C, T> operator&(const prods<C, T>& x,const std::basic_string<C>& s){
	//assert(!x.empty());
	if (x.empty()) return prods<C, T>(s);
	return x & prods<C, T>(s);
}
template <typename C, typename T>
prods<C, T> operator+(const prods<C, T>& x, const prods<C, T>& y) {
	if (x.empty()) return y;
	if (y.empty()) return x;
	prods<C, T> r;
	for (size_t n = 0; n != x.size() - 1; ++n) r.push_back(x[n]);
	for (size_t n = 0; n != y.size() - 1; ++n) r.push_back(y[n]);
	r.emplace_back(lit<C, T>{}, top<C, T>(x) + top<C, T>(y));
	return r;
}
template <typename C, typename T>
prods<C, T> operator+(const prods<C, T>& x, const T& c) {
	if (x.empty()) return prods<C, T>{ c };
	return x + prods<C, T>{ c };
}
template <typename C, typename T>
prods<C, T> operator+(const prods<C, T>& x, const T* s) {
	if (x.empty()) return prods<C, T>{ std::basic_string<C>(s) };
	return x + std::basic_string<C>(s);
}
template <typename C, typename T>
prods<C, T> operator+(const prods<C, T>& x,const std::basic_string<C>& s){
	//assert(!x.empty());
	prods<C, T> r(x);
	for (const auto& c : s) r = r + c;
	return r;
}
template <typename C, typename T>
prods<C, T> operator~(const prods<C, T>& x) {
	prods<C, T> r = x;
	top<C, T>(r) = ~top<C, T>(r);
	// cout << "\nnegating prods: " << x << " to: " << r << std::endl;
	return r;
}
template <typename C, typename T>
prods<C, T>::prods() : prods_t() { }
template <typename C, typename T>
prods<C, T>::prods(const lit<C, T>& l) : prods_t() { (*this)(l); }
template <typename C, typename T>
prods<C, T>::prods(const std::basic_string<C>& s) : prods_t() { (*this)(s); }
template <typename C, typename T>
prods<C, T>::prods(const std::vector<T>& v) : prods_t() { (*this)(v); }
template <typename C, typename T>
void prods<C, T>::operator()(const lit<C, T>& l) {
	this->emplace_back(lit<C, T>{},
		dnf<C, T>{clause<C, T>{lits<C, T>({l})}});
}
template <typename C, typename T>
void prods<C, T>::operator()(const std::basic_string<C>& s) {
	lits<C, T> a;
	for (const C& c : s) a.emplace_back(c);
	this->emplace_back(lit<C, T>{},
		dnf<C, T>{clause<C, T>{a}});
}
template <typename C, typename T>
void prods<C, T>::operator()(const std::vector<T>& s) {
	lits<C, T> a;
	for (const T& t : s) a.emplace_back(t);
	this->emplace_back(lit<C, T>{},
		dnf<C, T>{clause<C, T>{a}});
}
template <typename C, typename T>
void prods<C, T>::operator()(const prods<C, T>& l, const prods<C, T>& p) {
	(*this)(l.to_lit(), p);
}
template <typename C, typename T>
void prods<C, T>::operator()(const lit<C, T>& l, const prods<C, T>& p) {
	assert(p.size());
	this->emplace_back(l, p.to_dnf());
}
template <typename C, typename T>
bool prods<C, T>::operator==(const lit<C, T>& l) const { return l == to_lit(); }
template <typename C, typename T>
lit<C, T> prods<C, T>::to_lit() const {
	assert(this->size() && this->back().second.size() &&
		this->back().second.begin()->size() &&
		this->back().second.begin()->begin()->size());
	return this->back().second.begin()->begin()->back();
}
template <typename C, typename T>
dnf<C, T> prods<C, T>::to_dnf() const {
	assert(this->size());
	return this->back().second;
}
template <typename C, typename T>
bool operator==(const lit<C, T>& l, const prods<C, T>& p) {
	return p == l;
}

//-----------------------------------------------------------------------------

template <typename T>
void char_class_fns<T>::operator()(size_t nt, const char_class_fn<T>& fn) {
	this->fns.emplace(nt, fn);
	this->ps.emplace(nt, std::map<T, size_t>{});
}
template <typename T>
bool char_class_fns<T>::is_fn(size_t nt) const {
	return fns.find(nt) != fns.end();
}
template <typename T>
bool char_class_fns<T>::is_eof_fn(size_t nt) const {
	return nt == eof_fn;
}

template <typename C, typename T>
char_class_fns<T> predefined_char_classes(
	const std::vector<std::string>& cc_fn_names, nonterminals<C, T>& nts)
{
	char_class_fns<T> r;
	if constexpr (!std::is_same_v<T, char> && !std::is_same_v<T, char32_t>)
		return r;
	std::map<std::string, char_class_fn<T>> predef{
		{ "eof",       iseof<T>    },
		{ "alnum",     isalnum<T>  },
		{ "alpha",     isalpha<T>  },
		{ "blank",     isblank<T>  },
		{ "cntrl",     iscntrl<T>  },
		{ "digit",     isdigit<T>  },
		{ "graph",     isgraph<T>  },
		{ "lower",     islower<T>  },
		{ "printable", isprint<T>  },
		{ "punct",     ispunct<T>  },
		{ "space",     isspace<T>  },
		{ "upper",     isupper<T>  },
		{ "xdigit",    isxdigit<T> }
	};
	r(nts.get(std::basic_string<C>{}), [](T) { return false; });
	for (const auto& cc : cc_fn_names) {
		auto it = predef.find(cc);
		if (it == predef.end()) std::cerr << "Unknown character class: "
			<< to_string(cc) << "\n";
		else {
			auto nt = nts.get(from_str<C>(it->first));
			r(nt, it->second);
			if (cc == "eof") r.eof_fn = nt;
		}
	}
	return r;
}

//-----------------------------------------------------------------------------

template <typename C, typename T>
grammar<C, T>::grammar(nonterminals<C, T>& nts, const prods<C, T>& ps,
	const prods<C, T>& start, const char_class_fns<T>& cc_fns)
	: nts(nts), start(start.to_lit()), cc_fns(cc_fns)
{
	// load prods
	for (const prod<C, T>& p : ps) // every disjunction has its own prod rule
		for (const clause<C, T>& c : p.second) {
			G.emplace_back(p.first,
				std::vector<lits<C, T>>{ c.begin(), c.end() });
			if (c.size() > 1) conjunctives.insert(G.size()-1);
		}

	// cout << "conjunctive rules[" << conjunctives.size() << "]   ";
	// for (const auto& d : conjunctives) cout << " " << d;
	// cout << std::endl;
	// create ntsm: nt -> prod rule map
	for (size_t n = 0; n != G.size(); ++n) ntsm[G[n].first].insert(n);
	size_t k;
	do { // find all nullables
		k = nullables.size();
		for (const auto& p : G)
			for (const auto& a : p.second)
				if (all_nulls(a)) nullables.insert(p.first.n());
	} while (k != nullables.size());
	//DBG(print_data(std::cout << "\n", "\t") << "\n\n";)
}

template <typename C, typename T>
size_t grammar<C, T>::size() const { return G.size(); }
template <typename C, typename T>
lit<C, T> grammar<C, T>::operator()(const size_t&i) const {return G[i].first;}
template <typename C, typename T>
const std::vector<lits<C, T>>& grammar<C, T>::operator[](const size_t& i)
	const {	return G[i].second; }
template <typename C, typename T>
size_t grammar<C, T>::len(const size_t& p, const size_t& c)
	const { return G[p].second[c].size(); }
template <typename C, typename T>
bool grammar<C, T>::nullable(lit<C, T> l) const {
	return (!l.nt() && l.is_null()) ||
		(l.nt() && (nullables.find(l.n()) != nullables.end()));
}
template <typename C, typename T>
bool grammar<C, T>::conjunctive(size_t p) const {
	return conjunctives.find(p) != conjunctives.end();
}
template <typename C, typename T>
bool grammar<C, T>::char_class_check(lit<C, T> l, T ch) const
{
	//DBG(std::cout << "char_class_check: " << l.n() << std::endl;)
	auto& x = cc_fns.ps[l.n()];
	return (x.find(ch) == x.end()) && !cc_fns.fns[l.n()](ch);
}
template <typename C, typename T>
size_t grammar<C, T>::add_char_class_production(lit<C, T> l, T ch) {
	G.push_back(production{ l, {{{{ lit<C, T>{ ch } }}}}});
	ntsm[G.back().first].insert(G.size() - 1);
	return cc_fns.ps[l.n()][ch] = G.size() - 1;
}
template <typename C, typename T>
size_t grammar<C, T>::get_char_class_production(lit<C, T> l, T ch) {
	//DBG(std::cout << "get_char_class_prod: " << l.n() << std::endl;)
	auto& x = cc_fns.ps[l.n()];
	auto it = x.find(ch);
	if (it == x.end()) {
		if (!cc_fns.fns[l.n()](ch)) return//std::cout<<"\n   fn fail\n",
			static_cast<size_t>(-1);
		return add_char_class_production(l, ch);
	} else return it->second;
}

template <typename C, typename T>
const std::set<size_t>& grammar<C, T>::prod_ids_of_literal(const lit<C, T>& l) {
	return ntsm[l];
}
template <typename C, typename T>
const lit<C, T>& grammar<C, T>::start_literal() const { return start; }
template <typename C, typename T>
bool grammar<C, T>::is_cc_fn(const size_t &p) const { return cc_fns.is_fn(p); }
template <typename C, typename T>
bool grammar<C, T>::is_eof_fn(const size_t &p) const {
	return cc_fns.is_eof_fn(p);
}

template <typename C, typename T>
bool grammar<C, T>::all_nulls(const lits<C, T>& a) const {
	for (size_t k = 0; k != a.size(); ++k)
		if ((!a[k].nt() && !a[k].is_null()) || (a[k].nt() &&
			nullables.find(a[k].n()) == nullables.end()))
				return false;
	return true;
}
template <typename C, typename T>
lit<C, T> grammar<C, T>::nt(size_t n) { return lit<C, T>(n, &nts); }
template <typename C, typename T>
lit<C, T> grammar<C, T>::nt(const std::basic_string<C>& s) {
	return nt(nts.get(s));
}

#if defined(DEBUG) || defined(WITH_DEVHELPERS)
template <typename C, typename T>
std::ostream& grammar<C, T>::print_internal_grammar(std::ostream& os,
	std::string prep) const
{
	for (size_t i = 0; i != G.size(); ++i) {
		os << prep << i << ": " <<
			G[i].first.to_std_string(from_cstr<C>("ε")) <<" =>";
		size_t j = 0;
		for (const auto& c : G[i].second) {
			if (j++ != 0) os << " &";
			if (c.neg) os << " ~(";
			for (const auto& l : c) os << " " <<
				l.to_std_string(from_cstr<C>("ε"));
			if (c.neg) os << " )";
		}
		os << ".";
		if (conjunctive(i)) os << "\t # conjunctive";
		os << "\n";
	}
	return os;
}
template <typename C, typename T>
std::ostream& grammar<C, T>::print_data(std::ostream& os, std::string prep)
	const
{
	os << "nonterminals:\n";
	for (size_t i = 0; i != nts.size(); ++i)
		os << prep << i << ": " << to_std_string(nts[i]) <<
		(cc_fns.is_fn(i) ? (cc_fns.is_eof_fn(i) ? " (eof)" : " (char class)") : "") << "\n";
	print_internal_grammar(os << "productions:\n", prep);
	os << "nonterminals to productions map:\n";
	for (auto& x : ntsm) {
		os <<prep<< x.first.n() <<": "<< x.first.to_std_string() <<" -";
		for (auto& y : x.second) os << " " << y;
		os << "\n";
	}
	return os;
}
#endif

#if defined(DEBUG) || defined(WITH_DEVHELPERS)
template<typename C, typename T>
std::ostream& print_grammar(std::ostream& os, const grammar<C, T>& g) {
	for (size_t i = 0; i != g.size(); ++i) {
		const auto& x = g[i];
		for (const auto& y : x.second) {
			os << '\t' << to_std_string(x.first) << " =>";
			for (const auto& z : y.first)
				os << " \"" << to_std_string(z) << "\"";
			for (const auto& z : y.second)
				os << ", " << to_std_string(z);
			os << ".\n";
		}
	}
	return os;
}
template <typename C>
std::ostream& print_dictmap(std::ostream& os,
	const std::map<std::basic_string<C>, size_t>& dm)
{
	for (const auto& x : dm)
		os << '\t' << to_std_string(x.first) << ' ' << x.second << "\n";
	return os;
}
#endif // DEBUG

} // idni namespace
#endif // __IDNI__PARSER__GRAMMAR_TMPL_H__