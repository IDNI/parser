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
using namespace std;
namespace idni {

template <typename CharT>
ostream_t& operator<<(ostream_t& os, const lit<CharT>& l) {
	return os << l.to_std_string();
}
template <typename CharT>
ostream_t& operator<<(ostream_t& os, const alt<CharT>& a) {
	size_t n = 0;
	if (a.neg) os << "~(";
	for (const lit<CharT>& l : a) if (os << l; ++n != a.size()) os << " ";
	if (a.neg) os << ")";
	return os;
}
template <typename CharT>
ostream_t& operator<<(ostream_t& os, const clause<CharT>& c) {
	size_t n = 0;
	if (c.size()) os << "(";
	for (const alt<CharT>& a : c)
		if (os << a; ++n != c.size()) os << " & ";
	if (c.size()) os << ")";
	return os;
}
template <typename CharT>
ostream_t& operator<<(ostream_t& os, const dnf<CharT>& d) {
	size_t n = 0;
	for (const clause<CharT>& c : d)
		if (os << c; ++n != d.size()) os << " | ";
	return os;
}
template <typename CharT>
ostream_t& operator<<(ostream_t& os, const prods<CharT>& p) {
	for (size_t n = 0; n != p.size(); ++n)
		os << '\t' << p.at(n).first.to_std_string() << " => " <<
			p.at(n).second << '.' << endl;
	return os;
}

//-----------------------------------------------------------------------------

template <typename CharT>
size_t nonterminals<CharT>::get(const string& s) {
	if (auto it = m.find(s); it != m.end())
		return it->second;
	return m.emplace(s, this->size()),
		this->push_back(s), this->size() - 1;
}
template <typename CharT>
const typename nonterminals<CharT>::string& nonterminals<CharT>::get(size_t n)
	const
{
	assert(n < this->size());
	return this->at(n);
}

//-----------------------------------------------------------------------------

template <typename CharT>
typename lit<CharT>::string lit<CharT>::to_string(const string& nll) const {
	return nt() ? nts->get(n()) : c() == (CharT)0 ? nll : string{c()};
}
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
alt<CharT> operator+(const lit<CharT>& x, const lit<CharT>& y) {
	return alt<CharT>({ x, y });
}
template <typename CharT>
alt<CharT> operator+(const alt<CharT>& x, const lit<CharT>& l) {
	alt<CharT> a(x);
	return a.push_back(l), a;
}
template <typename CharT>
alt<CharT> operator+(const alt<CharT>& x, const alt<CharT>& y) {
	alt<CharT> a(x);
	for (const lit<CharT>& l : y) a.push_back(l);
	return a;
}
template <typename CharT>
alt<CharT> operator~(const alt<CharT>& x) {
	alt<CharT> r(x);
	return r.neg = !x.neg, r;
}

//-----------------------------------------------------------------------------

template <typename CharT>
bool operator<=(const clause<CharT>& x, const clause<CharT>& y) {
	for (const alt<CharT>& a : x) if (y.find(a) == y.end()) return false;
	return true;
}
template <typename CharT>
clause<CharT> simplify(const clause<CharT>& c) {
	for (alt<CharT> na : c) {
		na.neg = !na.neg;
		for (const alt<CharT>& a : c) if (a.neg == na.neg && a == na) {
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
	for (const alt<CharT>& a : x)
		for (const alt<CharT>& b : y) r.insert(a + b);
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
	for (const alt<CharT>& a : x) r.insert(clause<CharT>{~a});
	//cout << "\nnegating clause: " << x << " to: " << r << endl;
	return r;
}
template <typename CharT>
dnf<CharT> operator~(const dnf<CharT>& x) {
	set<dnf<CharT>> ncs; // negated conjunctions = disjunctions
	for (const clause<CharT>& c : x) ncs.insert(~c);
	if (ncs.size() == 0) return {}; // empty, return empty dnf
	if (ncs.size() == 1) return *(ncs.begin()); // return first if only one
	// otherwise do combinations
	vector<clause<CharT>> rcs; // disjuncted clauses to return
	vector<dnf<CharT>> ncsv(ncs.begin(), ncs.end()); // negated conjunctions vector = vector of disjunctions
	vector<clause<CharT>> cs(ncsv[0].begin(), ncsv[0].end());
	//cout << "cs("<<cs.size()<<"): " << dnf(cs.begin(), cs.end()) << endl;
	for (size_t i = 1; i != ncsv.size(); ++i) {
		vector<clause<CharT>> ncs(ncsv[i].begin(), ncsv[i].end());
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
	assert(!x.empty() && !y.empty());
	prods<CharT> r;
	for (size_t n = 0; n != x.size() - 1; ++n) r.push_back(x[n]);
	for (size_t n = 0; n != y.size() - 1; ++n) r.push_back(y[n]);
	r.emplace_back(lit<CharT>{}, top<CharT>(x) | top<CharT>(y));
	return r;
}
template <typename CharT>
prods<CharT> operator|(const prods<CharT>& x, const CharT& c) {
	assert(!x.empty());
	return x | prods<CharT>{ c };
}
template <typename CharT>
prods<CharT> operator|(const prods<CharT>& x, const CharT* s) {
	if (x.empty()) return prods<CharT>{ std::basic_string<CharT>(s) };
	return x | std::basic_string<CharT>(s);
}
template <typename CharT>
prods<CharT> operator|(const prods<CharT>& x,const std::basic_string<CharT>& s){
	assert(!x.empty());
	return x | prods<CharT>(s);
}
template <typename CharT>
prods<CharT> operator&(const prods<CharT>& x, const prods<CharT>& y) {
	assert(!x.empty() && !y.empty());
	prods<CharT> r;
	for (size_t n = 0; n != x.size() - 1; ++n) r.push_back(x[n]);
	for (size_t n = 0; n != y.size() - 1; ++n) r.push_back(y[n]);
	return r.emplace_back(lit<CharT>{}, top<CharT>(x) & top<CharT>(y)), r;
}

template <typename CharT>
prods<CharT> operator&(const prods<CharT>& x, const CharT& c) {
	assert(!x.empty());
	return x & prods<CharT>{ c };
}
template <typename CharT>
prods<CharT> operator&(const prods<CharT>& x, const CharT* s) {
	if (x.empty()) return prods<CharT>{ std::basic_string<CharT>(s) };
	return x & std::basic_string<CharT>(s);
}
template <typename CharT>
prods<CharT> operator&(const prods<CharT>& x,const std::basic_string<CharT>& s){
	assert(!x.empty());
	return x & prods<CharT>(s);
}
template <typename CharT>
prods<CharT> operator+(const prods<CharT>& x, const prods<CharT>& y) {
	assert(!x.empty() && !y.empty());
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
	assert(!x.empty());
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

//-----------------------------------------------------------------------------

template <typename CharT>
void char_class_fns<CharT>::operator()(size_t nt,
	const char_class_fn<CharT>& fn)
{
	this->push_back(nt);
	this->fns.emplace(nt, fn);
	this->ps.emplace_back();
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
		for (const clause<CharT>& c : p.second)
			G.push_back(production{ p.first,
				conjunctions{ c.begin(), c.end() }});
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
const typename grammar<CharT>::conjunctions& grammar<CharT>::operator[](
	const size_t& i) const
{
	return G[i].second;
}
template <typename CharT>
size_t grammar<CharT>::len(const size_t& p, const size_t& c) const {
	return G[p].second[c].size();
}
template <typename CharT>
bool grammar<CharT>::nullable(literal l) const {
	return (!l.nt() && l.c() == (CharT) 0) ||
		(l.nt() && (nullables.find(l.n()) != nullables.end()));
}
template <typename CharT>
size_t grammar<CharT>::char_class_check(literal l, CharT ch) {
	auto f = l.n();
	auto it = cc_fns.ps[f].find(ch);
	if (it == cc_fns.ps[f].end()) {
		if (!cc_fns.fns[f](ch)) return static_cast<size_t>(-1);
		G.push_back(production{ l, {{{{ literal{ ch } }}}}});
		ntsm[G.back().first].insert(G.size() - 1);
		return (cc_fns.ps[f][ch] = G.size() - 1);
	} else return it->second;
}
template <typename CharT>
bool grammar<CharT>::all_nulls(const alt<CharT>& a) const {
	for (size_t k = 0; k != a.size(); ++k)
		if ((!a[k].nt() && a[k].c() != (CharT) 0) || (a[k].nt() &&
			nullables.find(a[k].n()) == nullables.end()))
				return false;
	return true;
}
template <typename CharT>
typename grammar<CharT>::literal grammar<CharT>::nt(size_t n) {
	return literal(n, &nts);
}
template <typename CharT>
typename grammar<CharT>::literal grammar<CharT>::nt(const string& s) {
	return nt(nts.get(s));
}

#if defined(DEBUG) || defined(WITH_DEVHELPERS)
template<typename CharT>
ostream_t& grammar<CharT>::print_internal_grammar(ostream_t& os,
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
		os << ".\n";
	}
	return os;
}
template<typename CharT>
ostream_t& grammar<CharT>::print_data(ostream_t& os, std::string prep) const {
	os << "nonterminals:\n";
	for (size_t i = 0; i != nts.size(); ++i)
		os << prep << i << ": " << to_std_string(nts[i]) << "\n";
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
} // idni namespace
#endif // __IDNI__PARSER__GRAMMAR_TMPL_H__
