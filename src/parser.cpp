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
#ifdef WITH_CHARCLASSES
#include "charclasses.h"
#endif
using namespace std;
namespace idni {
#ifdef WITH_CHARCLASSES
using namespace idni::charclasses;
#endif

#define tdiff(start, end) ((double(end - start) / CLOCKS_PER_SEC) * 1000)
#define emeasure_time_start(start, end) clock_t end, start = clock()
#define emeasure_time_end(start, end) end = clock(), cout << fixed <<\
	setprecision(2) << (start, end) << " ms"

string to_std_string(const string& s) { return s; }
string to_std_string(const u32string& s) { return to_string(to_utf8string(s)); }
string to_std_string(const char32_t& ch) { return to_string(to_utf8string(ch));}

template <>
string from_cstr<char>(const char* s) { return string(s); }
template <>
u32string from_cstr<char32_t>(const char* s) {
	return to_u32string(to_utf8string(s));
}
template <>
string from_str<char>(const std::string& s) { return string(s); }
template <>
u32string from_str<char32_t>(const std::string& s) {
	return to_u32string(to_utf8string(s));
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
parser<CharT>::parser(const grammar& g, const parser_options& o) : o(o) {
	set<string> nt;       // non terminals
#ifdef WITH_CHARCLASSES
	map<string, int_t> m; // char function name to id
	char_class_fns_map cfm = o.cc_fns_map;
	if (o.cc_fns.size()) add_cc_fns(cfm, o.cc_fns);
	for (auto& cf : cfm)  // load char functions
		d.get(cf.first), nt.insert(cf.first),
		m.emplace(cf.first, cc_fns.size()),
		cc_fns.push_back(cf.second),
		cc_fns_prod.emplace_back();
#endif
	for (const auto &r : g) nt.insert(r.first);
	for (const auto &r : g)
		for (auto &y : r.second) {
			G.push_back({ lit{ d.get(r.first) } });
			for (auto &s : y)
				if (nt.find(s) != nt.end())
					G.back().push_back(lit{ d.get(s) });
				else if (s.size() == 0)
					G.back().push_back(lit{ (CharT) 0 });
				else for (CharT c : s)
					G.back().push_back(lit{ c });
		}
	start = lit{ d.get(o.start) };
	for (size_t n = 0; n != G.size(); ++n) nts[G[n][0]].insert(n);
	size_t k;
	do {
		k = nullables.size();
		for (const auto& p : G)
			if (all_nulls(p)) nullables.insert(p[0].n());
	} while (k != nullables.size());
	//DBG(print_grammar(cout, g);)
	//DBG(print_dictmap(cout, d.m);)
}

template <typename CharT>
typename parser<CharT>::container_iter
	parser<CharT>::add(container_t& t, const item& i)
{
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
			get_lit(*it) == get_nt(i)) add(t,
				item(i.set, it->prod, it->from, it->dot + 1));
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
void parser<CharT>::scan(const item& i, size_t n, CharT ch) {
	if (ch != get_lit(i).c()) return;
	item j(n + 1, i.prod, i.from, i.dot + 1);
	S[j.set].insert(j);
}

#ifdef WITH_CHARCLASSES
template <typename CharT>
bool parser<CharT>::add_cc_fns(char_class_fns_map& ccm,
	const vector<std::string>& ccs) const
{
	std::map<std::string, char_class_fn> m{
		{ "eof",       iseof<CharT>    },
		{ "alnum",     isalnum<CharT>  },
		{ "alpha",     isalpha<CharT>  },
		{ "blank",     isblank<CharT>  },
		{ "cntrl",     iscntrl<CharT>  },
		{ "digit",     isdigit<CharT>  },
		{ "graph",     isgraph<CharT>  },
		{ "lower",     islower<CharT>  },
		{ "printable", isprint<CharT>  },
		{ "punct",     ispunct<CharT>  },
		{ "space",     isspace<CharT>  },
		{ "upper",     isupper<CharT>  },
		{ "xdigit",    isxdigit<CharT> }
	};
	for (auto& cc : ccs) {
		auto it = m.find(cc); if (it == m.end()) return false;
		ccm.emplace(from_str<CharT>(it->first), it->second);
	}
	return true;
}
template <typename CharT>
void parser<CharT>::scan_cc_function(const item&i, size_t n, const string&s) {
	size_t p = 0; // character's prod rul
	int_t id = get_lit(i).n();
	bool eof = n == s.size();
	CharT ch = eof ? char_traits<CharT>::eof() : s[n];
	auto it = cc_fns_prod[id].find(ch);
	if (it == cc_fns_prod[id].end()) {
		if (!cc_fns[id](ch)) return; // not a char class function char
		p = G.size(); // its a new character in this fn -> store it 
		G.push_back({ get_lit(i) });
		G.back().push_back(lit{ ch });
		cc_fns_prod[id][ch] = p; // store prod of this ch
	} else p = it->second; // this ch has its prod already
	item j(n + (eof ? 0 : 1), i.prod, n, 2); // complete char function
	S[j.set].insert(j);
	item k(n + (eof ? 0 : 1), p, n, 2); // complete char functions's char
	S[k.set].insert(k);
}
#endif

template <typename CharT>
bool parser<CharT>::recognize(const typename parser<CharT>::string s) {
	MS(emeasure_time_start(tsr, ter);)
	inputstr = s;
	size_t len = s.size();
	f.clear();
	bin_tnt.clear();
	tid = 0;
	S.clear();//, S.resize(len + 1);//, C.clear(), C.resize(len + 1);
	S.resize(len+1);
	for (size_t n : nts[start]) {
		auto& cont = S[0];
		auto it = cont.emplace(0, n, 0, 1).first;
		// fix the bug for missing Start( 0 0) when start is nulllable
		if (nullable(*it)) cont.emplace(0, n, 0, 2);
	}
	container_t t;
#if MEASURE_EACH_POS
	size_t r = 1, cb = 0; // row and cel beginning
#endif
	for (size_t n = 0; n != len + 1; ++n) {
#if MEASURE_EACH_POS
		if (s[n] == '\n') (cb = n), r++;
		emeasure_time_start(tsp, tep);
#endif
		do {
			for(const item &x : t) S[x.set].insert(x);
			t.clear();
			const auto& cont = S[n];
			for (auto it = cont.begin(); it != cont.end(); ++it) {
				//DBG(print(cout << "processing ", *it) << endl;)
				if (completed(*it)) complete(*it, t);
				else if (get_lit(*it).nt())
#ifdef WITH_CHARCLASSES					
					if (get_lit(*it).n() < cc_fns.size()) {
						if (n <= len) scan_cc_function(
							*it, n, s);
					} else
#endif
						predict(*it, t);
				else if (n < len) scan(*it, n, s[n]);
			}
		} while (!t.empty());
#if MEASURE_EACH_POS
		cout << n << " \tln: " << r << " col: " << (n-cb+1) << " :: ";
		emeasure_time_end(tsp, tep)<<"\n";
#endif
		if (true == o.incr_gen_forest) {
			//DBG(cout << "set: " << n << endl;)
			const auto& cont = S[n];
			for (auto it = cont.begin(); it != cont.end(); ++it)
				if(completed(*it)) pre_process(*it);
				
			for (auto it = cont.begin(); it != cont.end(); ++it)
				if (completed(*it)) { 
					//DBG(cout<<"\n"<< it->from <<it->set << 
					//	it->prod << it->dot <<endl);				
					pnode curroot(get_nt(*it),
						{it->from, it->set});
					build_forest(curroot);
				}
		}
	}
	bool found = false;
	for (size_t n : nts[start])
		if (S[len].find(item(len, n, 0, G[n].size())) != S[len].end())
			found = true;
	MS(emeasure_time_end(tsr, ter) <<" :: recognize time\n";)
	if (!o.incr_gen_forest) init_forest();
#ifdef DEBUG
	if (!found) print_data(cout);
#endif
	return found;
}

template <typename CharT>
void parser<CharT>::pre_process(const item &i) {
	//sorted_citem[G[i.prod][0].n()][i.from].emplace_back(i);
	if (completed(i))
		sorted_citem[{ G[i.prod][0].n(), i.from }].emplace_back(i),
		rsorted_citem[{ G[i.prod][0].n(), i.set }].emplace_back(i);
	else if (o.bin_lr) {
		// Precreating temporaries to help in binarisation later
		// each temporary represents a partial rhs production with
		// atleast 3 symbols
		if (i.dot >= 3) {
			vector<lit> v(G[i.prod].begin() + 1,
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
template <typename CharT>
bool parser<CharT>::init_forest() {
	bool ret = false;
	// clear forest structure if any
	f.clear();
	bin_tnt.clear();
	tid = 0;
	// set the start root node
	size_t len = inputstr.length();
	pnode root(start, {0,len});
	// preprocess parser items for faster retrieval
	MS(emeasure_time_start(tspfo, tepfo);)
	int count = 0;
	for (size_t n = 0; n < len + 1; n++)
		for (const item& i : S[n]) count++, pre_process(i);
	MS(emeasure_time_end(tspfo, tepfo) << " :: preprocess time, " <<
						"size : "<< count << "\n";)
	MS(cout << "sort sizes : " << sorted_citem.size() << " " <<
						rsorted_citem.size() << " \n";)
	// build forest
	MS(emeasure_time_start(tsf, tef);)
	ret = o.bin_lr ? build_forest2(root) : build_forest(root);
	MS(emeasure_time_end(tsf, tef) <<" :: forest time\n";)
#ifdef DEBUG
	auto n = f.count_trees(root);
	cout <<"# parse trees " << n << "\n";
	if (n > 1) print_data(cout) << "\n\n";
#endif
	return ret;
}

// collects all possible variations of the given item's rhs while respecting the
// span of the item and stores them in the set ambset. 
template <typename CharT>
void parser<CharT>::sbl_chd_forest(const item &eitem,
	vector<pnode> &curchd, size_t xfrom,
	set<vector<pnode>> &ambset)
{
	//check if we have reached the end of the rhs of prod
	if (G[eitem.prod].size() <= curchd.size() + 1)  {
		// match the end of the span we are searching in.
		if (curchd.back().second[1] == eitem.set)
			ambset.insert(curchd);
		return;
	}
	// curchd.size() refers to index of cur literal to process in the rhs of production
	pnode nxtl = { G[eitem.prod][curchd.size() + 1], {} };
	// set the span start/end of the terminal symbol 
	if (!nxtl.first.nt()) {
		nxtl.second[0] = xfrom;
		// for empty, use same span edge as from
		if (nxtl.first.c() == (CharT) '\0') nxtl.second[1] = xfrom;
		// ensure well-formed combination (matching input) early
		else if (xfrom < inputstr.size() &&
					inputstr.at(xfrom) == nxtl.first.c())
			nxtl.second[1] = ++xfrom ;
		else // if not building the correction variation, prune this path quickly 
			return;
		// build from the next in the line
		size_t lastpos = curchd.size();
		curchd.push_back(nxtl),
		sbl_chd_forest(eitem, curchd, xfrom, ambset);
		curchd.erase(curchd.begin() + lastpos, curchd.end());
	} else {
		// get the from/to span of all non-terminals in the rhs of production.
		nxtl.second[0] = xfrom;
		
		//auto &nxtl_froms = sorted_citem[nxtl.n()][xfrom];
		auto &nxtl_froms = sorted_citem[{ nxtl.first.n(), xfrom }];
		for (auto &v : nxtl_froms) {
			// ignore beyond the span
			if (v.set > eitem.set) continue;
			// store current and recursively build for next nt
			size_t lastpos = curchd.size(); 
			nxtl.second[1] = v.set,
			curchd.push_back(nxtl), xfrom = v.set,
			sbl_chd_forest(eitem, curchd, xfrom, ambset);
			curchd.erase(curchd.begin() + lastpos, curchd.end());
		}
	}
}

// builds the forest starting with root
template <typename CharT>
bool parser<CharT>::build_forest(const pnode& root) {
	if (!root.first.nt()) return false;
	if (f.contains(root)) return false;

	//auto &nxtset = sorted_citem[root.n()][root.second[0]];
	auto &nxtset = sorted_citem[{ root.first.n(), root.second[0] }];
	pnodes_set ambset;
	for (const item &cur : nxtset) {
		if (cur.set != root.second[1]) continue;
		DBG(assert(root.first.n() == G[cur.prod][0].n());)
		pnode cnode(G[cur.prod][0], { cur.from, cur.set });
		pnodes nxtlits;
		sbl_chd_forest(cur, nxtlits, cur.from, ambset);
		f[cnode] = ambset;
		for (auto &aset : ambset)
			for (const pnode& nxt : aset)
				build_forest(nxt);
	}	
	return true;
}

template <typename CharT>
bool parser<CharT>::bin_lr_comb(const item& eitem,
	set<vector<pnode>>& ambset)
{
	vector<pnode> rcomb, lcomb;
	if (eitem.dot < 2) return false;

	pnode right = { G[eitem.prod][eitem.dot-1], {} };

	if (!right.first.nt()) {
		right.second[1] = eitem.set;
		if (right.first.c() == (CharT) '\0')
			right.second[0] = right.second[1];
		else if (inputstr.at(eitem.set -1) == right.first.c())
			right.second[0] = eitem.set -1 ;
		else return false;
		rcomb.emplace_back(right);
	} else {
		auto &rightit = rsorted_citem[{ right.first.n(), eitem.set }];
		for (auto &it : rightit)
			if (eitem.from <= it.from) 
				right.second[1] = it.set,
				right.second[0] = it.from,
				rcomb.emplace_back(right);
	}
	// many literals in rhs
	if (eitem.dot > 3) {
		//DBG(print(cout, eitem);)
		vector<lit> v(G[eitem.prod].begin() + 1,
					G[eitem.prod].begin() + eitem.dot - 1);
		DBG(assert( bin_tnt.find(v) != bin_tnt.end());)
		pnode left = { bin_tnt[v], {} };
		//DBG(cout << "\n" << d.get(bin_tnt[v].n()) << endl);
		auto &leftit = sorted_citem[{ left.first.n(), eitem.from }];
		// doing left right optimization
		for (auto &it : leftit) 
			for (auto &rit : rcomb)    
				if (it.set == rit.second[0]) {
					left.second[0] = it.from;
					left.second[1] = it.set;
					ambset.insert({ left, rit });
				} 
	}
	// exact two literals in rhs
	else if (eitem.dot == 3) {
		pnode left = { G[eitem.prod][eitem.dot-2], {} };
		lit& l = left.first;
		if (!l.nt()) {
			left.second[0] = eitem.from;
			if (l.c() == (CharT) '\0')
				left.second[1] = left.second[0];
			else if (inputstr.at(eitem.from) == l.c() )
				left.second[1] = eitem.from + 1  ;
			else return false;
			//do Left right optimisation
			for (auto &rit : rcomb)  
				if (left.second[1] == rit.second[0])
					ambset.insert({ left, rit });
		}
		else {
			auto &leftit = sorted_citem[{ l.n(), eitem.from }];
			for (auto &it : leftit) 
				for (auto &rit : rcomb)    
					if (it.set == rit.second[0])
						left.second[0] = it.from,
						left.second[1] = it.set,
						ambset.insert({ left, rit });
		}
	}
	else {
		DBG(assert(eitem.dot == 2));
		for (auto &rit : rcomb)
			if (eitem.from <= rit.second[0])
				ambset.insert({ rit });
	}
	return true;
}

template <typename CharT>
bool parser<CharT>::build_forest2(const pnode &root) {
	if (!root.first.nt()) return false;
	if (f.contains(root)) return false;
	//auto &nxtset = sorted_citem[root.n()][root.second[0]];
	auto &nxtset = sorted_citem[{ root.first.n(), root.second[0] }];
	pnodes_set ambset;
	for (const item &cur: nxtset) {
		if (cur.set != root.second[1]) continue;
		pnode cnode(completed(cur)
				? G[cur.prod][0] : lit{ root.first.n() },
			{ cur.from, cur.set });
		bin_lr_comb(cur, ambset);
		f[cnode] = ambset;
		for (auto &aset: ambset)
			for (const pnode& nxt : aset) build_forest2(nxt);
	}
	return true;
}

template <typename CharT>
typename parser<CharT>::string parser<CharT>::to_string(const lit& l) const {
	stringstream ss; l.nt() ? ss << d.get(l.n()) : ss << l.c();
	return ss.str();
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
	return hash<size_t>()(val) +
		0x9e3779b9 + (val << 6) + (val >> 2);
}
template <typename CharT>
size_t parser<CharT>::hasher_t::operator()(const pair<size_t, size_t> &k)
	const
{
	// lets substitute with better if possible.
	size_t h = 0;
	h ^= hash_size_t(k.first); 
	h ^= hash_size_t(k.second);
	return h;
}
template <typename CharT>
size_t parser<CharT>::hasher_t::operator()(const pnode &k) const {
	// lets substitute with better if possible.
	size_t h = 0;
	h ^= hash_size_t(k.second[0]);
	h ^= hash_size_t(k.second[1]);
	h ^= hash_size_t(size_t(
		k.first.nt() ? k.first.n() : k.first.c()));
	return h;
}
template <typename CharT>
size_t parser<CharT>::hasher_t::operator()(const item &k) const {
	// lets substitute with better if possible.
	size_t h = 0;
	h ^= hash_size_t(k.set);
	h ^= hash_size_t(k.from);
	h ^= hash_size_t(k.prod);
	h ^= hash_size_t(k.dot);
	return h;
}

template class parser<char>;
template class parser<char32_t>;

} // idni namespace
