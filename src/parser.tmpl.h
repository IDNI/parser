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
#ifndef __IDNI__PARSER__PARSER_TMPL_H__
#define __IDNI__PARSER__PARSER_TMPL_H__
#include "parser.h"
namespace idni {

using namespace idni::charclasses;

template <typename CharT>
std::ostream& operator<<(std::ostream& os, const lit<CharT>& l) {
	return os << l.to_std_string();
}
template <typename CharT>
std::ostream& operator<<(std::ostream& os, const lits<CharT>& a) {
	size_t n = 0;
	if (a.neg) os << "~(";
	for (const lit<CharT>& l : a) if (os << l; ++n != a.size()) os << " ";
	if (a.neg) os << ")";
	return os;
}
template <typename CharT>
std::ostream& operator<<(std::ostream& os, const clause<CharT>& c) {
	size_t n = 0;
	if (c.size()) os << "(";
	for (const lits<CharT>& a : c)
		if (os << a; ++n != c.size()) os << " & ";
	if (c.size()) os << ")";
	return os;
}
template <typename CharT>
std::ostream& operator<<(std::ostream& os, const dnf<CharT>& d) {
	size_t n = 0;
	for (const clause<CharT>& c : d)
		if (os << c; ++n != d.size()) os << " | ";
	return os;
}
template <typename CharT>
std::ostream& operator<<(std::ostream& os, const prods<CharT>& p) {
	for (size_t n = 0; n != p.size(); ++n)
		os << '\t' << p.at(n).first.to_std_string() << " => " <<
			p.at(n).second << '.' << std::endl;
	return os;
}

//-----------------------------------------------------------------------------

template <typename CharT>
size_t nonterminals<CharT>::get(const std::basic_string<CharT>& s) {
	if (auto it = m.find(s); it != m.end())
		return it->second;
	return m.emplace(s, this->size()),
		this->push_back(s), this->size() - 1;
}
template <typename CharT>
const std::basic_string<CharT>& nonterminals<CharT>::get(size_t n) const {
	assert(n < this->size());
	return this->at(n);
}
template <typename CharT>
lit<CharT> nonterminals<CharT>::operator()(const std::basic_string<CharT>& s) {
	return lit<CharT>{ get(s), &*this };
}

//-----------------------------------------------------------------------------

template <typename CharT>
lit<CharT>::lit() : lit(0) { }
template <typename CharT>
lit<CharT>::lit(CharT c) : lit_t(c), nts(0) { }
template <typename CharT>
lit<CharT>::lit(size_t n, nonterminals<CharT>* nts) : lit_t(n), nts(nts) { }
template <typename CharT>
bool lit<CharT>::nt() const { return std::holds_alternative<size_t>(*this); }
template <typename CharT>
size_t lit<CharT>::n() const { return std::get<size_t>(*this); }
template <typename CharT>
CharT lit<CharT>::c() const { return std::get<CharT>(*this); }
template <typename CharT>
std::basic_string<CharT> lit<CharT>::to_string(
	const std::basic_string<CharT>& nll) const
{
	return nt() ? nts->get(n())
		: c() == (CharT)0 ? nll : std::basic_string<CharT>{ c() };
}
template <typename CharT>
std::string lit<CharT>::to_std_string(const std::basic_string<CharT>& nll)
	const { return idni::to_std_string(to_string(nll)); }
template <typename CharT>
bool lit<CharT>::is_null() const { return c() == (CharT)0; }
template <typename CharT>
bool lit<CharT>::operator<(const lit& l) const {
	if (nt() != l.nt()) return nt() < l.nt();
	if (nt()) return n() == l.n() ? *nts < *l.nts : n() < l.n();
	return c() < l.c();
}
template <typename CharT>
bool lit<CharT>::operator==(const lit& l) const {
	if (nt() != l.nt()) return false;
	if (nt()) return n() == l.n() && *nts == *l.nts;
	else return c() == l.c();
}

//-----------------------------------------------------------------------------

template <typename CharT>
lits<CharT> operator+(const lit<CharT>& x, const lit<CharT>& y) {
	return lits<CharT>({ x, y });
}
template <typename CharT>
lits<CharT> operator+(const lits<CharT>& x, const lit<CharT>& l) {
	lits<CharT> a(x);
	return a.push_back(l), a;
}
template <typename CharT>
lits<CharT> operator+(const lits<CharT>& x, const lits<CharT>& y) {
	lits<CharT> a(x);
	for (const lit<CharT>& l : y) a.push_back(l);
	return a;
}
template <typename CharT>
lits<CharT> operator~(const lits<CharT>& x) {
	lits<CharT> r(x);
	return r.neg = !x.neg, r;
}

//-----------------------------------------------------------------------------

template <typename CharT>
bool operator<=(const clause<CharT>& x, const clause<CharT>& y) {
	for (const lits<CharT>& a : x) if (y.find(a) == y.end()) return false;
	return true;
}
template <typename CharT>
clause<CharT> simplify(const clause<CharT>& c) {
	for (lits<CharT> na : c) {
		na.neg = !na.neg;
		for (const lits<CharT>& a : c) if (a.neg == na.neg && a == na) {
			//cout << "simplified to {} a["<<a<<"] == na["<<na<<"]\n";
			return {};
		}
	}
	return c;
}
template <typename CharT>
clause<CharT> operator&(const clause<CharT>& x, const clause<CharT>& y) {
	clause<CharT> r = x;
	return r.insert(y.begin(), y.end()), simplify<CharT>(r);
}
template <typename CharT>
clause<CharT> operator+(const clause<CharT>& x, const clause<CharT>& y) {
	clause<CharT> r;
	for (const lits<CharT>& a : x)
		for (const lits<CharT>& b : y) r.insert(a + b);
	return simplify<CharT>(r);
}

//-----------------------------------------------------------------------------

template <typename CharT>
dnf<CharT>& top(prods<CharT>& p) {
	return p.back().second;
}
template <typename CharT>
const dnf<CharT>& top(const prods<CharT>& p) {
	return p.back().second;
}
template <typename CharT>
dnf<CharT> simplify(const dnf<CharT>& x) {
	dnf<CharT> y, z;
	for (clause<CharT> c : x)
		if (!(c = simplify(c)).empty()) y.insert(c);
	for (clause<CharT> c : y) {
		bool f = false;
		for (clause<CharT> d : y)
			if (c != d && c <= d) { f = true; break; }
		if (!f) z.insert(c);
	}
	return z;
}
template <typename CharT>
dnf<CharT> operator|(const dnf<CharT>& x, const dnf<CharT>& y) {
	dnf<CharT> r;
	return r.insert(x.begin(), x.end()), r.insert(y.begin(), y.end()), r;
}
template <typename CharT>
dnf<CharT> operator&(const dnf<CharT>& x, const clause<CharT>& y) {
	dnf<CharT> r;
	for (const clause<CharT>& c : x) r.insert(c & y);
	return r;
}
template <typename CharT>
dnf<CharT> operator&(const dnf<CharT>& x, const dnf<CharT>& y) {
	dnf<CharT> r;
	for (const clause<CharT>& c : x)
		for (const clause<CharT>& d : (y & c)) r.insert(d);
	return simplify<CharT>(r);
}
template <typename CharT>
dnf<CharT> operator+(const dnf<CharT>& x, const clause<CharT>& y) {
	dnf<CharT> r;
	for (const clause<CharT>& c : x) r.insert(c + y);
	return r;
}
template <typename CharT>
dnf<CharT> operator+(const dnf<CharT>& x, const dnf<CharT>& y) {
	dnf<CharT> r;
	for (const clause<CharT>& c : x)
		for (const clause<CharT>& d : y) r.insert(c + d);
	return r;
}
template <typename CharT>
dnf<CharT> operator~(const clause<CharT>& x) {
	dnf<CharT> r;
	for (const lits<CharT>& a : x) r.insert(clause<CharT>{~a});
	//cout << "\nnegating clause: " << x << " to: " << r << endl;
	return r;
}
template <typename CharT>
dnf<CharT> operator~(const dnf<CharT>& x) {
	std::set<dnf<CharT>> ncs; // negated conjunctions = disjunctions
	for (const clause<CharT>& c : x) ncs.insert(~c);
	if (ncs.size() == 0) return {}; // empty, return empty dnf
	if (ncs.size() == 1) return *(ncs.begin()); // return first if only one
	// otherwise do combinations
	std::vector<clause<CharT>> rcs; // disjuncted clauses to return
	std::vector<dnf<CharT>> ncsv(ncs.begin(), ncs.end()); // negated conjunctions vector = vector of disjunctions
	std::vector<clause<CharT>> cs(ncsv[0].begin(), ncsv[0].end());
	//cout << "cs("<<cs.size()<<"): " << dnf(cs.begin(), cs.end()) << endl;
	for (size_t i = 1; i != ncsv.size(); ++i) {
		std::vector<clause<CharT>> ncs(ncsv[i].begin(), ncsv[i].end());
		//cout << "\tncs("<<ncs.size()<<"): " << dnf(ncs.begin(), ncs.end()) << endl;
		for (size_t j = 0; j != ncs.size(); ++j)
			for (size_t k = 0; k != cs.size(); ++k) {
				clause<CharT> c(cs[k]);
				c.insert(*(ncs[j].begin()));
				rcs.push_back(c);
			}
		cs = rcs;
		//cout << "cs*("<<cs.size()<<"): " << dnf(cs.begin(), cs.end()) << endl;
	}
	dnf<CharT> r(cs.begin(), cs.end());
	//cout << "\nnegating dnf d: " << x << " to: `" << r << "`" << endl;
	return simplify<CharT>(r); // is simplify needed here?
}

//-----------------------------------------------------------------------------

template <typename CharT>
void simplify(prods<CharT>& p) {
	for (auto& x : p) x.second = simplify<CharT>(x.second);
}
template <typename CharT>
prods<CharT> operator|(const prods<CharT>& x, const prods<CharT>& y) {
	//assert(!x.empty() && !y.empty());
	if (x.empty()) return y;
	if (y.empty()) return x;
	prods<CharT> r;
	for (size_t n = 0; n != x.size() - 1; ++n) r.push_back(x[n]);
	for (size_t n = 0; n != y.size() - 1; ++n) r.push_back(y[n]);
	r.emplace_back(lit<CharT>{}, top<CharT>(x) | top<CharT>(y));
	return r;
}
template <typename CharT>
prods<CharT> operator|(const prods<CharT>& x, const CharT& c) {
	//assert(!x.empty());
	if (x.empty()) return prods<CharT>{ c };
	return x | prods<CharT>{ c };
}
template <typename CharT>
prods<CharT> operator|(const prods<CharT>& x, const CharT* s) {
	if (x.empty()) return prods<CharT>{ std::basic_string<CharT>(s) };
	return x | std::basic_string<CharT>(s);
}
template <typename CharT>
prods<CharT> operator|(const prods<CharT>& x,const std::basic_string<CharT>& s){
	//assert(!x.empty());
	if (x.empty()) return prods<CharT>(s);
	return x | prods<CharT>(s);
}
template <typename CharT>
prods<CharT> operator&(const prods<CharT>& x, const prods<CharT>& y) {
	//assert(!x.empty() && !y.empty());
	if (x.empty()) return y;
	if (y.empty()) return x;
	prods<CharT> r;
	for (size_t n = 0; n != x.size() - 1; ++n) r.push_back(x[n]);
	for (size_t n = 0; n != y.size() - 1; ++n) r.push_back(y[n]);
	return r.emplace_back(lit<CharT>{}, top<CharT>(x) & top<CharT>(y)), r;
}

template <typename CharT>
prods<CharT> operator&(const prods<CharT>& x, const CharT& c) {
	//assert(!x.empty());
	if (x.empty()) return prods<CharT>{ c };
	return x & prods<CharT>{ c };
}
template <typename CharT>
prods<CharT> operator&(const prods<CharT>& x, const CharT* s) {
	if (x.empty()) return prods<CharT>{ std::basic_string<CharT>(s) };
	return x & std::basic_string<CharT>(s);
}
template <typename CharT>
prods<CharT> operator&(const prods<CharT>& x,const std::basic_string<CharT>& s){
	//assert(!x.empty());
	if (x.empty()) return prods<CharT>(s);
	return x & prods<CharT>(s);
}
template <typename CharT>
prods<CharT> operator+(const prods<CharT>& x, const prods<CharT>& y) {
	if (x.empty()) return y;
	if (y.empty()) return x;
	prods<CharT> r;
	for (size_t n = 0; n != x.size() - 1; ++n) r.push_back(x[n]);
	for (size_t n = 0; n != y.size() - 1; ++n) r.push_back(y[n]);
	r.emplace_back(lit<CharT>{}, top<CharT>(x) + top<CharT>(y));
	return r;
}
template <typename CharT>
prods<CharT> operator+(const prods<CharT>& x, const CharT& c) {
	if (x.empty()) return prods<CharT>{ c };
	return x + prods<CharT>{ c };
}
template <typename CharT>
prods<CharT> operator+(const prods<CharT>& x, const CharT* s) {
	if (x.empty()) return prods<CharT>{ std::basic_string<CharT>(s) };
	return x + std::basic_string<CharT>(s);
}
template <typename CharT>
prods<CharT> operator+(const prods<CharT>& x,const std::basic_string<CharT>& s){
	//assert(!x.empty());
	prods<CharT> r(x);
	for (const auto& c : s) r = r + c;
	return r;
}
template <typename CharT>
prods<CharT> operator~(const prods<CharT>& x) {
	prods<CharT> r = x;
	top<CharT>(r) = ~top<CharT>(r);
	// cout << "\nnegating prods: " << x << " to: " << r << endl;
	return r;
}
template <typename CharT>
prods<CharT>::prods() : prods_t() { }
template <typename CharT>
prods<CharT>::prods(const lit<CharT>& l) : prods_t() { (*this)(l); }
template <typename CharT>
prods<CharT>::prods(const std::basic_string<CharT>& s) : prods_t() { (*this)(s); }
template <typename CharT>
void prods<CharT>::operator()(const lit<CharT>& l) {
	this->emplace_back(lit<CharT>{},
		dnf<CharT>{clause<CharT>{lits<CharT>({l})}});
}
template <typename CharT>
void prods<CharT>::operator()(const std::basic_string<CharT>& s) {
	lits<CharT> a;
	for (const CharT& c : s) a.emplace_back(c);
	this->emplace_back(lit<CharT>{},
		dnf<CharT>{clause<CharT>{a}});
}
template <typename CharT>
void prods<CharT>::operator()(const prods<CharT>& l, const prods<CharT>& p) {
	(*this)(l.to_lit(), p);
}
template <typename CharT>
void prods<CharT>::operator()(const lit<CharT>& l, const prods<CharT>& p) {
	assert(p.size());
	this->emplace_back(l, p.to_dnf());
}
template <typename CharT>
bool prods<CharT>::operator==(const lit<CharT>& l) const { return l == to_lit(); }
template <typename CharT>
lit<CharT> prods<CharT>::to_lit() const {
	assert(this->size() && this->back().second.size() &&
		this->back().second.begin()->size() &&
		this->back().second.begin()->begin()->size());
	return this->back().second.begin()->begin()->back();
}
template <typename CharT>
dnf<CharT> prods<CharT>::to_dnf() const {
	assert(this->size());
	return this->back().second;
}
template <typename CharT>
bool operator==(const lit<CharT>& l, const prods<CharT>& p) {
	return p == l;
}

//-----------------------------------------------------------------------------

template <typename CharT>
void char_class_fns<CharT>::operator()(size_t nt,
	const char_class_fn<CharT>& fn)
{
	this->fns.emplace(nt, fn);
	this->ps.emplace(nt, std::map<CharT, size_t>{});
}
template <typename CharT>
bool char_class_fns<CharT>::is_fn(size_t nt) const {
	return fns.find(nt) != fns.end();
}

template <typename CharT>
char_class_fns<CharT> predefined_char_classes(
	const std::vector<std::basic_string<CharT>>& cc_fn_names,
	nonterminals<CharT>& nts)
{
	auto U = [](const char* s){ return from_cstr<CharT>(s); };
	std::map<std::basic_string<CharT>, char_class_fn<CharT>> predef{
		{ U("eof"),       iseof<CharT>    },
		{ U("alnum"),     isalnum<CharT>  },
		{ U("alpha"),     isalpha<CharT>  },
		{ U("blank"),     isblank<CharT>  },
		{ U("cntrl"),     iscntrl<CharT>  },
		{ U("digit"),     isdigit<CharT>  },
		{ U("graph"),     isgraph<CharT>  },
		{ U("lower"),     islower<CharT>  },
		{ U("printable"), isprint<CharT>  },
		{ U("punct"),     ispunct<CharT>  },
		{ U("space"),     isspace<CharT>  },
		{ U("upper"),     isupper<CharT>  },
		{ U("xdigit"),    isxdigit<CharT> }
	};
	char_class_fns<CharT> r;
	r(nts.get(U("")), [](CharT) { return false; });
	for (const auto& cc : cc_fn_names) {
		auto it = predef.find(cc);
		if (it == predef.end())
			std::cerr<< "Unknown character class: "<< to_string(cc);
		else r(nts.get(it->first), it->second);
	}
	return r;
}

//-----------------------------------------------------------------------------

template <typename CharT>
grammar<CharT>::grammar(nonterminals<CharT>& nts, const prods<CharT>& ps,
	const prods<CharT>& start, const char_class_fns<CharT>& cc_fns)
	: nts(nts), start(start.to_lit()), cc_fns(cc_fns)
{
	// load prods
	for (const prod<CharT>& p : ps) // every disjunction has its own prod rule
		for (const clause<CharT>& c : p.second) {
			G.emplace_back(p.first,
				std::vector<lits<CharT>>{ c.begin(), c.end() });
			if (c.size() > 1) conjunctives.insert(G.size()-1);
		}
		
	// cout << "conjunctive rules[" << conjunctives.size() << "]   ";
	// for (const auto& d : conjunctives) cout << " " << d;
	// cout << endl;
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

template <typename CharT>
size_t grammar<CharT>::size() const { return G.size(); }
template <typename CharT>
lit<CharT> grammar<CharT>::operator()(const size_t&i) const {return G[i].first;}
template <typename CharT>
const std::vector<lits<CharT>>& grammar<CharT>::operator[](const size_t& i)
	const {	return G[i].second; }
template <typename CharT>
size_t grammar<CharT>::len(const size_t& p, const size_t& c)
	const { return G[p].second[c].size(); }
template <typename CharT>
bool grammar<CharT>::nullable(lit<CharT> l) const {
	return (!l.nt() && l.c() == (CharT) 0) ||
		(l.nt() && (nullables.find(l.n()) != nullables.end()));
}
template <typename CharT>
bool grammar<CharT>::conjunctive(size_t p) const {
	return conjunctives.find(p) != conjunctives.end();
}
template <typename CharT>
bool grammar<CharT>::char_class_check(lit<CharT> l, CharT ch) const
{
	//DBG(std::cout << "char_class_check: " << l.n() << std::endl;)
	auto& x = cc_fns.ps[l.n()];
	return (x.find(ch) == x.end()) && !cc_fns.fns[l.n()](ch);
}
template <typename CharT>
size_t grammar<CharT>::get_char_class_production(lit<CharT> l, CharT ch) {
	//DBG(std::cout << "get_char_class_prod: " << l.n() << std::endl;)
	auto& x = cc_fns.ps[l.n()];
	auto it = x.find(ch);
	if (it == x.end()) {
		if (!cc_fns.fns[l.n()](ch)) return static_cast<size_t>(-1);
		G.push_back(production{ l, {{{{ lit<CharT>{ ch } }}}}});
		ntsm[G.back().first].insert(G.size() - 1);
		return x[ch] = G.size() - 1;
	} else return it->second;
}
template <typename CharT>
bool grammar<CharT>::all_nulls(const lits<CharT>& a) const {
	for (size_t k = 0; k != a.size(); ++k)
		if ((!a[k].nt() && a[k].c() != (CharT) 0) || (a[k].nt() &&
			nullables.find(a[k].n()) == nullables.end()))
				return false;
	return true;
}
template <typename CharT>
lit<CharT> grammar<CharT>::nt(size_t n) { return lit<CharT>(n, &nts); }
template <typename CharT>
lit<CharT> grammar<CharT>::nt(const std::basic_string<CharT>& s) {
	return nt(nts.get(s));
}

#if defined(DEBUG) || defined(WITH_DEVHELPERS)
template<typename CharT>
std::ostream& grammar<CharT>::print_internal_grammar(std::ostream& os,
	std::string prep) const
{
	for (size_t i = 0; i != G.size(); ++i) {
		os << prep << i << ": " <<
			G[i].first.to_std_string(from_cstr<CharT>("ε")) <<" =>";
		size_t j = 0;
		for (const auto& c : G[i].second) {
			if (j++ != 0) os << " &";
			if (c.neg) os << " ~(";
			for (const auto& l : c) os << " " <<
				l.to_std_string(from_cstr<CharT>("ε"));
			if (c.neg) os << " )";
		}
		os << ".";
		if (conjunctive(i)) os << "\t # conjunctive";
		os << "\n";
	}
	return os;
}
template<typename CharT>
std::ostream& grammar<CharT>::print_data(std::ostream& os, std::string prep)
	const
{
	os << "nonterminals:\n";
	for (size_t i = 0; i != nts.size(); ++i)
		os << prep << i << ": " << to_std_string(nts[i]) <<
		(cc_fns.is_fn(i) ? " (char class)" : "") << "\n";
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

template <typename CharT>
std::basic_ostream<CharT>& terminals_to_stream(std::basic_ostream<CharT>& os,
	const typename parser<CharT>::pforest& f,
	const typename parser<CharT>::pnode& root)
{
	auto cb_enter = [&os](const auto& n) {
		if (!n.first.nt() && (n.first.c()!=(CharT)0)) os << n.first.c();
	};
	f.traverse(root, cb_enter);
	return os;
}
template <typename CharT>
std::basic_string<CharT> terminals_to_str(
	const typename parser<CharT>::pforest& f,
	const typename parser<CharT>::pnode& root)
{
	std::basic_stringstream<CharT> ss;
	terminals_to_stream<CharT>(ss, f, root);
	return ss.str();
}
template <typename CharT>
int_t terminals_to_int(
	const typename parser<CharT>::pforest& f,
	const typename parser<CharT>::pnode& root)
{
	return stoi(to_std_string(terminals_to_str<CharT>(f, root)));
}
#ifdef DEBUG
template <typename CharT>
std::ostream& operator<<(std::ostream& os,
	const typename idni::parser<CharT>::lit& l)
{
	return os << l.to_string();
}
template <typename CharT>
std::ostream& operator<<(std::ostream& os,
	const std::vector<typename idni::parser<CharT>::lit>& v)
{
	int i = 0;
	for (auto &l : v) os << l, i++ == 0 ? os << "->": os <<' ';
	return os;
}
#endif
#if defined(DEBUG) || defined(WITH_DEVHELPERS)
template <typename CharT>
std::ostream& parser<CharT>::print(std::ostream& os, const item& i) const {
	os << "I(" << i.prod << "." << i.con << ", " << i.dot << ", "
		 << i.from << ", " << i.set << "):\t ";
	//os << "g[i.prod].size():" << g[i.prod].size() << " ";
	//size_t j = 0;
	const auto& x = g[i.prod][i.con];
	//os << i.from << " " << i.set << " " << "x.size():" << x.size() << " ";
	os << g(i.prod).to_std_string() << "\t => ";
	if (x.neg) os << "~( ";
	for (size_t n = 0; n != x.size(); ++n) {
		if (n == i.dot) os << "• ";
		os << x[n].to_std_string(from_cstr<CharT>("ε ")) << " ";
	}
	if (x.size() == i.dot) os << "•";
	if (x.neg) os << " )";
	return os << ".";
}
template <typename CharT>
std::ostream& parser<CharT>::print_S(std::ostream& os) const {
	for (size_t n = 0; n != S.size(); ++n) {
		os << "S["<<n<<"]:\n";
		for(const item &x : S[n])
			print(os << "\t", x) << "\n";
	}
	return os;
}
template<typename CharT>
std::ostream& parser<CharT>::print_data(std::ostream& os) const {
	os << "S:\n";
	for (auto& x : S) {
		for (auto& y : x) {
			if (y.from != y.set)
				print(os << "\t " << y.prod << "/" << y.con << " \t", y) <<"\n";
		}
		os << "---:\n";
	}
	os << "completed S:\n";
	for (auto& x : S) {
		for (auto& y : x) {
			if (y.from != y.set && y.dot == g.len(y.prod, y.con))
				print(os << "\t " << y.prod << " \t", y) <<"\n";
		}
		os << "---:\n";
	}
	return os;
}
template<typename CharT>
std::ostream& print_grammar(std::ostream& os,	const grammar<CharT>& g) {
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
template<typename CharT>
std::ostream& print_dictmap(std::ostream& os,
	const std::map<typename parser<CharT>::string, size_t>& dm)
{
	for (const auto& x : dm)
		os << '\t' << to_std_string(x.first) << ' ' << x.second << "\n";
	return os;
}
#endif // DEBUG

} // idni namespace
#endif // __IDNI__PARSER__PARSER_TMPL_H__
