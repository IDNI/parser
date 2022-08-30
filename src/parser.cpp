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
#include <cassert>
#include "parser.h"
using namespace std;
namespace idni {

string to_std_string(const string& s) { return s; }
string to_std_string(const u32string& s) { return to_string(to_utf8string(s)); }
string to_std_string(const char32_t& ch) { return to_string(to_utf8string(ch));}

template <>
string from_cstr<char>(const char* s) { return std::string(s); }
template <>
u32string from_cstr<char32_t>(const char* s) {
	return to_u32string(to_utf8string(s));
}

template <typename CharT>
bool parser<CharT>::item::operator<(const item& i) const {
	if (set != i.set) return set < i.set;
	if (prod != i.prod) return prod < i.prod;
	if (from != i.from) return from < i.from;
	return dot < i.dot;
}

template <typename CharT>
bool parser<CharT>::item::operator==(const item& i) const {
	if (set != i.set || prod != i.prod ||
		from != i.from || dot != i.dot)
			return false;
	return true;
}

template <typename CharT>
size_t parser<CharT>::hasher_t::hash_size_t(const size_t &val) const{
	return std::hash<size_t>()(val) +
		0x9e3779b9 + (val << 6) + (val >> 2);
}
template <typename CharT>
size_t parser<CharT>::hasher_t::operator()(const std::pair<size_t, size_t> &k)
	const
{
	// lets substitute with better if possible.
	std::size_t h = 0;
	h ^= hash_size_t(k.first); 
	h ^= hash_size_t(k.second);
	return h;
}
template <typename CharT>
size_t parser<CharT>::hasher_t::operator()(const pnode &k) const {
	// lets substitute with better if possible.
	std::size_t h = 0;
	h ^= hash_size_t(k.second[0]);
	h ^= hash_size_t(k.second[1]);
	h ^= hash_size_t(size_t(
		k.first.nt() ? k.first.n() : k.first.c()));
	return h;
}
template <typename CharT>
size_t parser<CharT>::hasher_t::operator()(const item &k) const {
	// lets substitute with better if possible.
	std::size_t h = 0;
	h ^= hash_size_t(k.set);
	h ^= hash_size_t(k.from);
	h ^= hash_size_t(k.prod);
	h ^= hash_size_t(k.dot);
	return h;
}

template <typename CharT>
bool parser<CharT>::nullable(const item& i) const {
	return i.dot < G[i.prod].size() &&
		((get_lit(i).nt() &&
		nullables.find(get_lit(i).n()) !=
		nullables.end()) ||
		(!get_lit(i).nt() && get_lit(i).c() == '\0'));
}

template <typename CharT>
bool parser<CharT>::all_nulls(const vector<lit>& p) const {
	for (size_t k = 1; k != p.size(); ++k)
		if ((!p[k].nt() && p[k].c() != (CharT) '\0') || (p[k].nt() &&
			nullables.find(p[k].n()) == nullables.end()))
			return false;
	return true;
}

template <typename CharT>
parser<CharT>::parser(const grammar& g, const char_builtins_map& bm,
	bool _bin_lr, bool _incr_gen_forest)
	: bin_lr(_bin_lr), incr_gen_forest(_incr_gen_forest)
{
	set<string> nt;
	map<string, int_t> bmi;
	for (auto& bmp : bm) {
		d.get(bmp.first);
		nt.insert(bmp.first);
		bmi.emplace(bmp.first, builtins.size());
		builtins.push_back(bmp.second);
		builtin_char_prod.emplace_back();
	}
	for (const auto &x : g) nt.insert(x.first);
	for (const auto &x : g)
		for (auto &y : x.second) {
			G.push_back({ lit{ d.get(x.first) } });
			for (auto &s : y)
				if (nt.find(s) != nt.end()) {
					G.back().push_back(lit{ d.get(s) });
					auto it = bmi.find(s);
					if (it != bmi.end()) G.back().back()
						.builtin = it->second;
				} else if (s.size() == 0)
					G.back().push_back(lit{ (CharT) '\0' });
				else for (CharT c : s)
					G.back().push_back(lit{ c });
	}
	start = lit{ d.get(from_cstr<CharT>("start")) };
	for (size_t n = 0; n != G.size(); ++n) nts[G[n][0]].insert(n);
	size_t k;
	do {
		k = nullables.size();
		for (const auto& p : G)
			if (all_nulls(p)) nullables.insert(p[0].n());
	} while (k != nullables.size());
	DBG(print_grammar<CharT>(cout, g);)
	DBG(print_dictmap<CharT>(cout, d.m);)
}

template <typename CharT>
typename parser<CharT>::container_iter parser<CharT>::add(container_t& t, 
		const item& i) {
	//DBG(print(cout << "adding ", i) << endl;)
	auto& cont = S[i.set];
	auto it = cont.find(i);
	if (it != cont.end()) return it;
	if ((it = t.find(i)) != t.end()) return it;
	it = t.insert(i).first;
	if (nullable(*it))
		add(t, item(it->set, it->prod, it->from, it->dot + 1));
	return it;
}

template <typename CharT>
void parser<CharT>::complete(const item& i, container_t& t) {
	//DBG(print(cout << "completing ", i) << endl;)
	const container_t& cont = S[i.from];
	for (auto it = cont.begin(); it != cont.end(); ++it)
		if (G[it->prod].size() > it->dot &&
			get_lit(*it) == get_nt(i))
			add(t, item(i.set, it->prod, it->from, it->dot + 1));
}

template <typename CharT>
void parser<CharT>::predict(const item& i, container_t& t) {
	//DBG(print(cout << "predicting ", i) << endl;)
	for (size_t p : nts[get_lit(i)]) {
		item j(i.set, p, i.set, 1);
		add(t, j);
		//DBG(print(cout << "predicting added ", j) << endl;)
	}
}

template <typename CharT>
void parser<CharT>::scan_builtin(const item& i, size_t n, const string& s) {
	size_t p = 0; // character's prod rule
	int_t bid = get_lit(i).builtin;
	bool eof = n == s.size();
	CharT ch = eof ? std::char_traits<CharT>::eof() : s[n];
	auto it = builtin_char_prod[bid].find(ch);
	if (it == builtin_char_prod[bid].end()) {
		if (!builtins[bid](ch)) return; //char isn't in the builtn class
		p = G.size(); // its a new character in this builtin -> store it 
		G.push_back({ get_lit(i) });
		G.back().push_back(lit{ ch });
		builtin_char_prod[bid][ch] = p; // store prod of this ch
	} else p = it->second; // this ch has its prod already
	item j(n + (eof ? 0 : 1), i.prod, n, 2); // complete builtin
	S[j.set].insert(j);
	item k(n + (eof ? 0 : 1), p, n, 2);      // complete builtin's character
	S[k.set].insert(k);
}

template <typename CharT>
void parser<CharT>::scan(const item& i, size_t n, CharT ch) {
	if (ch != get_lit(i).c()) return;
	item j(n + 1, i.prod, i.from, i.dot + 1);
	S[j.set].insert(j);
}

template <typename CharT>
bool parser<CharT>::recognize(const typename parser<CharT>::string s) {
	emeasure_time_start(tsr, ter);
	inputstr = s;
	size_t len = s.size();
	pfgraph.clear();
	bin_tnt.clear();
	tid = 0;
	S.clear();//, S.resize(len + 1);//, C.clear(), C.resize(len + 1);
	S.resize(len+1);
	for (size_t n : nts[start]) {
		auto& cont = S[0];
		auto it = cont.emplace(0, n, 0, 1).first;
		// fix the bug for missing Start( 0 0) when start is nulllable
		if (nullable(*it))
			cont.emplace(0, n, 0, 2);
	}
	container_t t;
#ifdef DEBUG
	size_t r = 1, cb = 0; // row and cel beginning
#endif
	for (size_t n = 0; n != len + 1; ++n) {
#ifdef DEBUG
		if (s[n] == '\n') (cb = n), r++;
		emeasure_time_start(tsp, tep);
#endif
		do {
			for(const item &x : t) S[x.set].insert(x);
			t.clear();
			const auto& cont = S[n];
			for (auto it = cont.begin();
				it != cont.end(); ++it) {
				//DBG(print(cout << "processing ", *it) << endl;)
				if (completed(*it)) complete(*it, t);
				else if (get_lit(*it).is_builtin()) {
					if (n <= len) scan_builtin(*it, n, s);
				} else if (get_lit(*it).nt()) predict(*it, t);
				else if (n < len) scan(*it, n, s[n]);
			}
		} while (!t.empty());
#ifdef DEBUG
		if (print_optimizations) {
			cout<<n<<" \tln: "<<r<<" col: "<<(n-cb+1)<<" :: ";
			emeasure_time_end(tsp, tep)<<"\n";
		}
#endif
		if (true == incr_gen_forest) {
		DBG(cout << "set: " << n << endl;)
		const auto& cont = S[n];
		for (auto it = cont.begin(); it != cont.end(); ++it)
			if(completed(*it)) pre_process(*it);
			
		for (auto it = cont.begin(); it != cont.end(); ++it)
			if (completed(*it)) { 
				DBG(cout<<"\n"<< it->from <<it->set << 
					it->prod << it->dot <<endl);				
				pnode curroot(get_nt(*it), {it->from, it->set});
				build_forest(curroot);
			}
		}
	}
	bool found = false;
	for (size_t n : nts[start])
		if (S[len].find( item(len, n, 0, G[n].size())) != S[len].end()) 
			found = true;
	emeasure_time_end(tsr, ter) <<" :: recognize time\n";
	if(!incr_gen_forest) forest();

	return found;
}

template <typename CharT>
void parser<CharT>::pre_process(const item &i) {
	//sorted_citem[G[i.prod][0].n()][i.from].emplace_back(i);
	if (completed(i))
		sorted_citem[{ G[i.prod][0].n(), i.from }].emplace_back(i),
		rsorted_citem[{ G[i.prod][0].n(), i.set }].emplace_back(i);
	else if (bin_lr) {
		// Precreating temporaries to help in binarisation later
		// each temporary represents a partial rhs production with
		// atleast 3 symbols
		if (i.dot >= 3) {
			std::vector<lit> v(G[i.prod].begin() + 1,
						G[i.prod].begin() + i.dot);
			lit tlit;
			if (bin_tnt.find(v) == bin_tnt.end()) {
				stringstream ss;
				ss << "temp" << tid++;
				tlit = lit{ d.get(ss.str()) };
				bin_tnt.insert({ v, tlit });
			}
			else tlit = bin_tnt[v];
			
			//DBG(print(cout, i);)
			//cout<< "\n" << d.get(tlit.n()) << v << endl;
			sorted_citem[{ tlit.n(), i.from }].emplace_back(i),
			rsorted_citem[{ tlit.n(), i.set }].emplace_back(i);
		}
	}
}

template<> std::basic_string<char> parser<char>::epsilon() const { return "ε"; }
template<> std::basic_string<char32_t> parser<char32_t>::epsilon() const
								{ return U"ε"; }
template class parser<char>;
template class parser<char32_t>;

} // idni namespace
