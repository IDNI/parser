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
std::ostream& operator<<(std::ostream& os, const conjs<C, T>& c) {
	size_t n = 0;
	if (c.size()) os << "(";
	for (const lits<C, T>& a : c)
		if (os << a; ++n != c.size()) os << " & ";
	if (c.size()) os << ")";
	return os;
}
template <typename C, typename T>
std::ostream& operator<<(std::ostream& os, const disjs<C, T>& d) {
	size_t n = 0;
	for (const conjs<C, T>& c : d)
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
lit<C, T>::lit() : lit_t(static_cast<T>(0)), is_null_(true) { }
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
std::vector<T> lit<C, T>::to_terminals() const {
	if (nt() || is_null()) return {};
	return { t() };
}
template <typename C, typename T>
std::basic_string<C> lit<C, T>::to_string(const std::basic_string<C>& nll)const{
	auto escape = [](const char& t) {
		return t == '\n' ? "\\n" :
			t == '\r' ? "\\r" :
			t == '\t' ? "\\t" :
			t == '\v' ? "\\v" :
			t == '\'' ? "\\'" :
			t == '\\' ? "\\\\" :
			t == '\0' ? "\\0" : "";
	};
	auto escape32 = [](const char32_t& t) {
		return t == U'\n' ? "\\n" :
			t == U'\r' ? "\\r" :
			t == U'\t' ? "\\t" :
			t == U'\v' ? "\\v" :
			t == U'\'' ? "\\'" :
			t == U'\\' ? "\\\\" :
			t == U'\0' ? "\\0" : "";
	};
	if (nt()) return nts->get(n());
	else if (is_null()) return nll;
	else if constexpr (std::is_same_v<T, bool>)
		return from_cstr<C>(t() ? "1" : "0");
	else if constexpr (std::is_same_v<C,char> && std::is_same_v<C, T>) {
		std::basic_stringstream<C> ss;
		std::string r = escape(t());
		ss << "'" << (r.size() ? r : idni::to_std_string(t())) << "'";
		return ss.str();
	} else if constexpr ((std::is_same_v<C,char32_t> &&
							std::is_same_v<C, T>) ||
		(std::is_same_v<C,char> && std::is_same_v<T,char32_t>))
	{
		std::basic_stringstream<C> ss;
		std::string r = escape32(t());
		ss << "'" << (r.size() ? r : idni::to_std_string(t())) << "'";
		return ss.str();
	}
	return nll;
}
template <typename C, typename T>
std::string lit<C, T>::to_std_string(const std::basic_string<C>& nll) const {
	return idni::to_std_string(this->to_string(nll));
}
template <typename C, typename T>
auto lit<C, T>::operator<=>(const lit<C, T>& l) const {
	if (nt() != l.nt()) return nt() <=> l.nt();
	if (nt()) return n() == l.n() ? nts <=> l.nts : n() <=> l.n();
	return t() <=> l.t();
}
template <typename C, typename T>
bool lit<C, T>::operator==(const lit<C, T>& l) const {
	if (nt() != l.nt()) return false;
	if (nt()) return n() == l.n() && nts == l.nts;
	if (is_null() != l.is_null()) return is_null() == l.is_null();
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
bool operator<=(const conjs<C, T>& x, const conjs<C, T>& y) {
	for (const lits<C, T>& a : x) if (y.find(a) == y.end()) return false;
	return true;
}
template <typename C, typename T>
conjs<C, T> simplify(const conjs<C, T>& c) {
	for (lits<C, T> na : c) {
		na.neg = !na.neg;
		for (const lits<C, T>& a : c) if (a.neg == na.neg && a == na)
			return {};
	}
	return c;
}
template <typename C, typename T>
conjs<C, T> operator&(const conjs<C, T>& x, const conjs<C, T>& y) {
	conjs<C, T> r = x;
	return r.insert(y.begin(), y.end()), simplify<C, T>(r);
}
template <typename C, typename T>
conjs<C, T> operator+(const conjs<C, T>& x, const conjs<C, T>& y) {
	conjs<C, T> r;
	for (const lits<C, T>& a : x)
		for (const lits<C, T>& b : y) r.insert(a + b);
	return simplify<C, T>(r);
}
//-----------------------------------------------------------------------------
template <typename C, typename T>
disjs<C, T>& top(prods<C, T>& p) { return p.back().second; }
template <typename C, typename T>
const disjs<C, T>& top(const prods<C, T>& p) { return p.back().second; }
template <typename C, typename T>
disjs<C, T> simplify(const disjs<C, T>& x) {
	disjs<C, T> y, z;
	for (conjs<C, T> c : x)
		if (!(c = simplify(c)).empty()) y.insert(c);
	for (conjs<C, T> c : y) {
		bool f = false;
		for (conjs<C, T> d : y)
			if (c != d && c <= d) { f = true; break; }
		if (!f) z.insert(c);
	}
	return z;
}
template <typename C, typename T>
disjs<C, T> operator|(const disjs<C, T>& x, const disjs<C, T>& y) {
	disjs<C, T> r;
	return r.insert(x.begin(), x.end()), r.insert(y.begin(), y.end()), r;
}
template <typename C, typename T>
disjs<C, T> operator&(const disjs<C, T>& x, const conjs<C, T>& y) {
	disjs<C, T> r;
	for (const conjs<C, T>& c : x) r.insert(c & y);
	return r;
}
template <typename C, typename T>
disjs<C, T> operator&(const disjs<C, T>& x, const disjs<C, T>& y) {
	disjs<C, T> r;
	for (const conjs<C, T>& c : x)
		for (const conjs<C, T>& d : (y & c)) r.insert(d);
	return simplify<C, T>(r);
}
template <typename C, typename T>
disjs<C, T> operator+(const disjs<C, T>& x, const conjs<C, T>& y) {
	disjs<C, T> r;
	for (const conjs<C, T>& c : x) r.insert(c + y);
	return r;
}
template <typename C, typename T>
disjs<C, T> operator+(const disjs<C, T>& x, const disjs<C, T>& y) {
	disjs<C, T> r;
	for (const conjs<C, T>& c : x)
		for (const conjs<C, T>& d : y) r.insert(c + d);
	return r;
}
template <typename C, typename T>
disjs<C, T> operator~(const conjs<C, T>& x) {
	disjs<C, T> r;
	for (const lits<C, T>& a : x) r.insert(conjs<C, T>{ ~a });
	//cout << "\nnegating conjs: " << x << " to: " << r << std::endl;
	return r;
}
template <typename C, typename T>
disjs<C, T> operator~(const disjs<C, T>& x) {
	std::set<disjs<C, T>> ncs; // negated conjunctions = disjunctions
	for (const conjs<C, T>& c : x) ncs.insert(~c);
	if (ncs.size() == 0) return {}; // empty, return empty disjs
	if (ncs.size() == 1) return *(ncs.begin()); // return first if only one
	// otherwise do combinations
	std::vector<conjs<C, T>> rcs; // disjuncted conjss to return
	std::vector<disjs<C, T>> ncsv(ncs.begin(), ncs.end()); // negated conjunctions vector = vector of disjunctions
	std::vector<conjs<C, T>> cs(ncsv[0].begin(), ncsv[0].end());
	//cout << "cs("<<cs.size()<<"): " << disjs(cs.begin(), cs.end()) << std::endl;
	for (size_t i = 1; i != ncsv.size(); ++i) {
		std::vector<conjs<C, T>> ncs(ncsv[i].begin(), ncsv[i].end());
		//cout << "\tncs("<<ncs.size()<<"): " << disjs(ncs.begin(), ncs.end()) << std::endl;
		for (size_t j = 0; j != ncs.size(); ++j)
			for (size_t k = 0; k != cs.size(); ++k) {
				conjs<C, T> c(cs[k]);
				c.insert(*(ncs[j].begin()));
				rcs.push_back(c);
			}
		cs = rcs;
		//cout << "cs*("<<cs.size()<<"): " << disjs(cs.begin(), cs.end()) << std::endl;
	}
	disjs<C, T> r(cs.begin(), cs.end());
	//cout << "\nnegating disjs d: " << x << " to: `" << r << "`" << std::endl;
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
		disjs<C, T>{ conjs<C, T>{ lits<C, T>({ l }) } });
}
template <typename C, typename T>
void prods<C, T>::operator()(const std::basic_string<C>& s) {
	lits<C, T> a;
	for (const C& c : s) a.emplace_back(c);
	this->emplace_back(lit<C, T>{},
		disjs<C, T>{ conjs<C, T>{ a } });
}
template <typename C, typename T>
void prods<C, T>::operator()(const std::vector<T>& s) {
	lits<C, T> a;
	for (const T& t : s) a.emplace_back(t);
	this->emplace_back(lit<C, T>{},
		disjs<C, T>{ conjs<C, T>{ a } });
}
template <typename C, typename T>
void prods<C, T>::operator()(const prods<C, T>& l, const prods<C, T>& p) {
	(*this)(l.to_lit(), p);
}
template <typename C, typename T>
void prods<C, T>::operator()(const lit<C, T>& l, const prods<C, T>& p) {
	assert(p.size());
	this->emplace_back(l, p.to_disjs());
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
disjs<C, T> prods<C, T>::to_disjs() const {
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
		{ "eof",       charclasses::iseof<T>    },
		{ "alnum",     charclasses::isalnum<T>  },
		{ "alpha",     charclasses::isalpha<T>  },
		{ "blank",     charclasses::isblank<T>  },
		{ "cntrl",     charclasses::iscntrl<T>  },
		{ "digit",     charclasses::isdigit<T>  },
		{ "graph",     charclasses::isgraph<T>  },
		{ "lower",     charclasses::islower<T>  },
		{ "printable", charclasses::isprint<T>  },
		{ "punct",     charclasses::ispunct<T>  },
		{ "space",     charclasses::isspace<T>  },
		{ "upper",     charclasses::isupper<T>  },
		{ "xdigit",    charclasses::isxdigit<T> }
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
	size_t neg_id = 0;
	// load prods
	// every disjunction has its own prod rule
	for (const prod<C, T>& p : ps) for (const conjs<C, T>& c : p.second) {
		std::vector<lits<C, T>> new_conjs;
		for (const lits<C, T>& l : c) if (l.neg) {
			// create a negative rule for a negative conjunction
			std::basic_stringstream<C> ss;
			ss << from_cstr<C>("__neg_") << neg_id++;
			auto l_nt = nts(ss.str());
			lits<C, T> ot(l), nt({ l_nt });
			nt.neg = true, ot.neg = false;
			G.emplace_back(l_nt, std::vector<lits<C, T>>{{ ot }});
			new_conjs.push_back(nt);
		} else new_conjs.push_back(l);
		G.emplace_back(p.first, new_conjs);
		if (c.size() > 1) conjunctives.insert(G.size()-1);
	}
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
lit<C, T> grammar<C, T>::operator()(const size_t&i) const  {return G[i].first; }
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
size_t grammar<C, T>::n_conjs(size_t p) const {
	return G[p].second.size();
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
	G.push_back(production{ l, { { { { lit<C, T>{ ch } } } } } });
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
const std::set<size_t>& grammar<C, T>::prod_ids_of_literal(const lit<C, T>& l)
	const
{
	static std::set<size_t> empty{};
	if (!ntsm.contains(l)) return empty;
	return ntsm.at(l);
}
template <typename C, typename T>
const lit<C, T>& grammar<C, T>::start_literal() const { return start; }
template <typename C, typename T>
bool grammar<C, T>::is_cc_fn(const size_t& p) const { return cc_fns.is_fn(p); }
template <typename C, typename T>
bool grammar<C, T>::is_eof_fn(const size_t& p) const {
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

template <typename C, typename T>
std::ostream& grammar<C, T>::check_nullable_ambiguity(std::ostream& os) const {
	const auto report = [&os, this](const lit<C, T>& first,
		const lit<C, T>& second, const production& p)
	{
		os << "Warning: possible ambiguity from a nullable literal: ("
			<< first << ") neighbors (" << second
			<< ") in production: ";
		print_production(os, p);
		os << "\n";
	};
	const auto first_nullable = [this](const lit<C, T>& l) {
		if (!l.nt()) return false;
		const auto& prods = prod_ids_of_literal(l);
		for (const auto& p : prods) for (const auto& a : G[p].second)
			if (nullable(a[0])) return true;
		return false;
	};
	const auto last_nullable = [this](const lit<C, T>& l) {
		if (!l.nt()) return false;
		const auto& prods = prod_ids_of_literal(l);
		for (const auto& p : prods) for (const auto& a : G[p].second)
			if (nullable(a.back())) return true;
		return false;
	};
	for (const auto& p : G) for (const auto& a : p.second) if (a.size() > 1)
		for (size_t i = 1; i != a.size() - 1; ++i) {
			const auto& last = a[i-1], curr = a[i];
			if (last.nt() && curr.nt() &&
				(nullable(last) || last_nullable(last)) &&
				(nullable(curr) || first_nullable(curr)))
					report(last, curr, p);
		}
	return os;
}

template <typename C, typename T>
std::ostream& grammar<C, T>::print_production(std::ostream& os,
	const production& p) const
{
	os << p.first.to_std_string(from_cstr<C>("null")) << " =>";
	size_t j = 0;
	for (const auto& c : p.second) {
		if (j++ != 0) os << " &";
		if (c.neg) os << " ~(";
		for (const auto& l : c) os << " " <<
			(!l.nt() && !l.is_null()
				? l.to_std_string()
				: l.to_std_string(from_cstr<C>("null")));
		if (c.neg) os << " )";
	}
	return os << ".";
}
template <typename C, typename T>
std::ostream& grammar<C, T>::print_internal_grammar(std::ostream& os,
	std::string prep, bool print_ids) const
{
	for (size_t i = 0; i != G.size(); ++i) {
		os << prep, print_ids ? os << "G" << i << ": " : os;
		print_production(os, G[i]);
		if (conjunctive(i)) os << "\t # conjunctive";
		os << "\n";
	}
	return os;
}
template <typename C, typename T>
std::ostream& grammar<C, T>::print_internal_grammar_for(std::ostream& os,
	const std::string& nt, std::string prep, bool print_ids) const
{
	auto root = nts(nt);
	std::set<size_t> ids;
	std::set<size_t> to_visit = prod_ids_of_literal(root);
	auto not_visited = [&ids](size_t i) {
		return ids.find(i) == ids.end();
	};
	while (to_visit.size()) {
		std::set<size_t> next_to_visit;
		for (auto p : to_visit) {
			if (!not_visited(p)) continue;
			ids.insert(p);
			for (auto pb : G[p].second) for (auto l : pb) {
				if (l.nt() && not_visited(l.n())) {
					auto pids = prod_ids_of_literal(l);
					next_to_visit.insert(pids.begin(), pids.end());
				}
			}
		}
		to_visit = next_to_visit;
	}
	for (size_t i : ids) {
		os << prep, print_ids ? os << "G" << i << ": " : os;
		print_production(os, G[i]);
		if (conjunctive(i)) os << "\t # conjunctive";
		os << "\n";
	}
	return os;
}
#if defined(DEBUG) || defined(WITH_DEVHELPERS)
template <typename C, typename T>
std::ostream& grammar<C, T>::print_data(std::ostream& os, std::string prep)
	const
{
	os << "nonterminals:\n";
	for (size_t i = 0; i != nts.size(); ++i)
		os << prep << i << ": " << to_std_string(nts[i]) <<
		(cc_fns.is_fn(i) ? (cc_fns.is_eof_fn(i) ?
				" (eof)" : " (char class)") : "") << "\n";
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
