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
using namespace idni::charclasses;

#define tdiff(start, end) ((double(end - start) / CLOCKS_PER_SEC) * 1000)
#define emeasure_time_start(start, end) clock_t end, start = clock()
#define emeasure_time_end(start, end) end = clock(), cout << fixed <<\
	setprecision(2) << (start, end) << " ms"

template <typename CharT>
parser<CharT>::parser(grammar<CharT>& g, const options& o) : g(g), o(o) {};

template <typename CharT>
bool parser<CharT>::nullable(const item& i) const {
	return i.dot < g[i.prod][i.con].size() && g.nullable(get_lit(i));
}

template <typename CharT>
void parser<CharT>::uncomplete_if_not_conj(container_t& t) {
	//DBG(cout << "uncomplete_if_not_conj..." << endl;)
	std::map<size_t, std::set<std::pair<size_t, item>>> ps;
	for (const item &x : t) {
		if (ps.find(x.prod) == ps.end()) ps[x.prod] = {};
		if (completed(x)) ps[x.prod].insert({ x.con, x });
	}
	//DBG(cout << "ps.size(): " << ps.size() << endl;)
	
	for (const auto& p : ps) {
		bool uncomplete = false;
		for (size_t c = 0; c != g[p.first].size(); ++c) {
			bool found = false;
			for (const auto& x : p.second) {
				//DBG(print(cout << "\tcheck p: " << p.first <<
				// 	" c: " << c <<
				//	" x: " << x.first << " ",
				//	x.second) << endl;)
				if (x.first == c) { found = true; break; }
			}
			uncomplete = (g[p.first][c].neg && found) ||
					(!g[p.first][c].neg && !found);
			if (uncomplete) break;
		}
		if (uncomplete) for (const auto& x : p.second) {
			//DBG(print(cout << "uncompleting: ", x.second) << endl;)
			t.erase(x.second);
		}
	}
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
		add(t, item(it->set, it->prod, it->con, it->from, it->dot + 1));
	return it;
}

template <typename CharT>
bool parser<CharT>::completed(const item& i) const {
	const auto& a = g[i.prod][i.con];
	bool r = a.size() == i.dot;
	return r;
}

template <typename CharT>
void parser<CharT>::complete(const item& i, container_t& t) {
	//DBG(print(cout << "\tcompleting ", i) << endl;)
	const container_t& cont = S[i.from];
	for (auto it = cont.begin(); it != cont.end(); ++it)
		if (g[it->prod][it->con].size() > it->dot &&
			get_lit(*it) == get_nt(i)) add(t, item(i.set,
				it->prod, it->con, it->from, it->dot + 1));
}

template <typename CharT>
void parser<CharT>::predict(const item& i, container_t& t) {
	//DBG(print(cout << "predicting ", i) << endl;)
	for (size_t p : g.ntsm[get_lit(i)])
		for (size_t c = 0; c != g[p].size(); ++c)
			add(t, item(i.set, p, c, i.set, 0));
}

template <typename CharT>
void parser<CharT>::scan(const item& i, size_t n, CharT ch) {
	if (ch != get_lit(i).c()) return;
	item j(n + 1, i.prod, i.con, i.from, i.dot + 1);
	S[j.set].insert(j);
}

template <typename CharT>
void parser<CharT>::scan_cc_function(const item&i, size_t n, const string&s) {
	size_t p = 0; // character's prod rule
	lit<CharT> l = get_lit(i);
	bool eof = n == s.size();
	CharT ch = eof ? char_traits<CharT>::eof() : s[n];
	p = g.char_class_check(l, ch);
	if (p == static_cast<size_t>(-1)) return;
	item j(n + (eof ? 0 : 1), i.prod, i.con, n, 1); // complete char fn
	S[j.set].insert(j);
	item k(n + (eof ? 0 : 1), p, 0, n, 1); // complete char functions's char
	S[k.set].insert(k);
}

template <typename CharT>
std::unique_ptr<typename parser<CharT>::pforest> parser<CharT>::parse(
	const typename parser<CharT>::string& s, bool &found)
{
	auto f = parse(s);
	size_t len = inputstr.size();
	found = false;
	//DBG(cout<<"finding completed `start` over the input string: "<<endl;)
	//for (const auto& x : S[len]) print(cout << "\t", x) << endl;
	for (size_t n : g.ntsm[g.start]) {
		//DBG(cout << "N: " << n << endl;)
		for (size_t c = 0; c != g[n].size(); ++c) {
			//DBG(print(cout << "find: ",
			//	item(len, n, c, 0, g.len(n, c))) << endl;)
			//DBG(cout << "C: " << c << endl;)
			bool t = S[len].find(item(len, n, c, 0, g.len(n, c)))
					!= S[len].end();
			//DBG(cout << "~: " << g[n][c].neg << endl;)
			//DBG(cout << "T: " << t << endl;)
			if (!g[n][c].neg) found = t;
			else if (t) found = false;
			//DBG(cout << "F: " << found << endl;)
			if (!found) break;
		}
		if (found) break;
	}
#ifdef DEBUG
	//if (!found) print_data(cout);
#endif
	return f;
}

template <typename CharT>
std::unique_ptr<typename parser<CharT>::pforest> parser<CharT>::parse(
	const typename parser<CharT>::string& s)
{
	MS(emeasure_time_start(tsr, ter);)
	inputstr = s;
	size_t len = s.size();
	//DBG(cout << "parse: `"<<to_std_string(s)<<"`["<<len<<"] g.start:"<<g.start<<"("<<g.start.nt()<<")"<<"\n";)
	auto f = make_unique<pforest>();
	f->root(pnode(g.start, { 0, len }));
	sorted_citem.clear();
	rsorted_citem.clear();
	bin_tnt.clear();
	tid = 0;
	S.clear();//, S.resize(len + 1);//, C.clear(), C.resize(len + 1);
	S.resize(len+1);
	for (size_t n : g.ntsm[g.start]) {
		//cout << "n:"<<n << endl;
		auto& cont = S[0];
		for (size_t c = 0; c != g[n].size(); ++c) {
			auto it = cont.emplace(0, n, c, 0, 0).first;
			// fix the bug for missing Start( 0 0) when start is nulllable
			if (nullable(*it)) cont.emplace(0, n, c, 0, 1);
		}
	}
	container_t t;
#if MEASURE_EACH_POS
	size_t r = 1, cb = 0; // row and cel beginning
#endif
	//size_t proc = 0;
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
				//DBG(print(cout << "\nprocessing(" << proc++ << ") ", *it) << endl;)
				if (completed(*it)) complete(*it, t);
				else if (get_lit(*it).nt()) {
					if (get_lit(*it).n() < g.cc_fns.size()){
						if (n <= len) scan_cc_function(
								*it, n, s);
					} else predict(*it, t);
				} else if (n < len) scan(*it, n, s[n]);
			}
		} while (!t.empty());
		//DBG(print_S(cout);)
		uncomplete_if_not_conj(S[n]);
#if MEASURE_EACH_POS
		cout << n << " \tln: " << r << " col: " << (n-cb+1) << " :: ";
		emeasure_time_end(tsp, tep)<<"\n";
#endif
		if (o.incr_gen_forest) {
			//DBG(cout << "set: " << n << endl;)
			const auto& cont = S[n];
			for (auto it = cont.begin(); it != cont.end(); ++it)
				pre_process(*it);
			for (auto it = cont.begin(); it != cont.end(); ++it)
				if (completed(*it)) { 
					//DBG(cout<<"\n"<< it->from <<it->set << 
					//	it->prod << it->dot <<endl);				
					pnode curroot(get_nt(*it),
						{ it->from, it->set });
					build_forest(*f, curroot);
				}
		}
	}
	MS(emeasure_time_end(tsr, ter) <<" :: parse time\n";)
	if (!o.incr_gen_forest) init_forest(*f);
#ifdef DEBUG
	auto n = f->count_trees();
	if (n > 1) cout << "# parse trees: " << n << "\n";
#endif
	return f;
}

template <typename CharT>
void parser<CharT>::pre_process(const item &i) {
	//sorted_citem[G[i.prod][0].n()][i.from].emplace_back(i);
	if (completed(i))
		sorted_citem[{ g(i.prod).n(), i.from }].emplace_back(i),
		rsorted_citem[{ g(i.prod).n(), i.set }].emplace_back(i);
	else if (o.bin_lr) {
		// Precreating temporaries to help in binarisation later
		// each temporary represents a partial rhs production with
		// atleast 3 symbols
		if (i.dot >= 3) {
			vector<lit<CharT>> v(g[i.prod][i.con].begin(),
					g[i.prod][i.con].begin() + i.dot);
			lit<CharT> tlit;
			if (bin_tnt.find(v) == bin_tnt.end()) {
				stringstream ss;
				ss << "temp" << tid++;
				tlit = g.nt(ss.str());
				bin_tnt.insert({ v, tlit });
			}
			else tlit = bin_tnt[v];
			
			//DBG(print(cout, i);)
			//cout<< "\n" << d->get(tlit.n()) << v << endl;
			sorted_citem[{ tlit.n(), i.from }].emplace_back(i),
			rsorted_citem[{ tlit.n(), i.set }].emplace_back(i);
		}
	}
}
template <typename CharT>
bool parser<CharT>::init_forest(pforest& f) {
	bool ret = false;
	bin_tnt.clear();
	sorted_citem.clear();
	rsorted_citem.clear();
	tid = 0;
	// set the start root node
	size_t len = inputstr.length();
	pnode root(g.start, { 0, len });
	f.root(root);
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
	ret = build_forest(f, root);
	MS(emeasure_time_end(tsf, tef) <<" :: forest time\n";)
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
	if (g.len(eitem.prod, eitem.con) <= curchd.size())  {
		// match the end of the span we are searching in.
		if (curchd.back().second[1] == eitem.set)
			ambset.insert(curchd);
		return;
	}
	// curchd.size() refers to index of cur literal to process in the rhs of production
	pnode nxtl = { g[eitem.prod][eitem.con][curchd.size()], {} };
	// set the span start/end of the terminal symbol 
	if (!nxtl.first.nt()) {
		nxtl.second[0] = xfrom;
		// for empty, use same span edge as from
		if (nxtl.first.c() == (CharT) 0) nxtl.second[1] = xfrom;
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

template <typename CharT>
bool parser<CharT>::bin_lr_comb(const item& eitem, set<vector<pnode>>& ambset) {
	vector<pnode> rcomb, lcomb;
	if (eitem.dot < 2) return false;
	pnode right = { g[eitem.prod][eitem.con][eitem.dot-1], {} };
	if (!right.first.nt()) {
		right.second[1] = eitem.set;
		if (right.first.c() == (CharT) 0)
			right.second[0] = right.second[1];
		else if (inputstr.at(eitem.set - 1) == right.first.c())
			right.second[0] = eitem.set - 1;
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
		vector<lit<CharT>> v(g[eitem.prod][eitem.con].begin(),
					g[eitem.prod][eitem.con].begin() +
						eitem.dot - 1);
		DBG(assert(bin_tnt.find(v) != bin_tnt.end());)
		pnode left = { bin_tnt[v], {} };
		//DBG(cout << "\n" << d->get(bin_tnt[v].n()) << endl);
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
		pnode left = { g[eitem.prod][eitem.con][eitem.dot - 2], {} };
		auto& l = left.first;
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
// builds the forest starting with root
template <typename CharT>
bool parser<CharT>::build_forest(pforest& f, const pnode &root) {
	if (!root.first.nt()) return false;
	if (f.contains(root)) return false;
	//auto &nxtset = sorted_citem[root.n()][root.second[0]];
	auto &nxtset = sorted_citem[{ root.first.n(), root.second[0] }];
	pnodes_set ambset;
	for (const item &cur: nxtset) {
		if (cur.set != root.second[1]) continue;
		pnode cnode(completed(cur) ? g(cur.prod) : g.nt(root.first.n()),
			{ cur.from, cur.set });
		if(o.bin_lr) bin_lr_comb(cur, ambset);
		else {
			pnodes nxtlits;
			sbl_chd_forest(cur, nxtlits, cur.from, ambset);
		}
		f[cnode] = ambset;
		for (auto &aset: ambset)
			for (const pnode& nxt : aset) build_forest(f, nxt);
	}
	return true;
}
template <typename CharT>
bool parser<CharT>::item::operator<(const item& i) const {
	if (set  != i.set)  return set  < i.set;
	if (prod != i.prod) return prod < i.prod;
	if (con  != i.con)  return con  < i.con;
	if (from != i.from) return from < i.from;
	return dot < i.dot;
}
template <typename CharT>
bool parser<CharT>::item::operator==(const item& i) const {
	return set == i.set && prod == i.prod && con == i.con &&
		from == i.from && dot == i.dot;
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
	h ^= hash_size_t(k.con);
	h ^= hash_size_t(k.dot);
	return h;
}

std::string to_std_string(const std::string& s) { return s; }
std::string to_std_string(const utf8string& s) { return to_string(s); }
std::string to_std_string(const std::u32string& s) {
	return to_std_string(to_utf8string(s)); }
std::string to_std_string(const char32_t& ch) {
	return to_std_string(to_utf8string(ch));}
template <>
std::string from_cstr<char>(const char* s) { return std::string(s); }
template <>
utf8string from_cstr<utf8char>(const char* s) {
	return to_utf8string(from_cstr<char>(s)); }
template <>
std::u32string from_cstr<char32_t>(const char* s) {
	return to_u32string(to_utf8string(s)); }
template <>
std::string from_str<char>(const std::string& s) {
	return s; }
template <>
utf8string from_str<utf8char>(const std::string& s) {
	return to_utf8string(s); }
template <>
std::u32string from_str<char32_t>(const std::string& s) {
	return to_u32string(to_utf8string(s)); }

template class parser<char>;
template class parser<char32_t>;

} // idni namespace
