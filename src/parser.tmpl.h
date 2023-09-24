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

template <typename C, typename T>
parser<C, T>::item::item(size_t set, size_t prod, size_t con, size_t from,
	size_t dot) : set(set), prod(prod), con(con), from(from), dot(dot) {}
template <typename C, typename T>
bool parser<C, T>::item::operator<(const item& i) const {
	if (set  != i.set)  return set  < i.set;
	if (prod != i.prod) return prod < i.prod;
	if (con  != i.con)  return con  < i.con;
	if (from != i.from) return from < i.from;
	return dot < i.dot;
}
template <typename C, typename T>
bool parser<C, T>::item::operator==(const item& i) const {
	return set == i.set && prod == i.prod && con == i.con &&
		from == i.from && dot == i.dot;
}
//------------------------------------------------------------------------------
template <typename C, typename T>
parser<C, T>::input::input(const C* data, size_t length, decoder_type decoder,
	int_type eof) : itype(POINTER), e(eof), decoder(decoder), max_l(length),
	l(length), d(data), s({}) {}
template <typename C, typename T>
parser<C, T>::input::input(std::basic_istream<C>& is, size_t length,
	decoder_type decoder, int_type eof) : itype(STREAM), e(eof),
	decoder(decoder), max_l(length), l(0), d(0), s({})
	{ s.rdbuf(is.rdbuf()); }
template <typename C, typename T>
parser<C, T>::input::input(int fd, size_t length, decoder_type decoder,
	int_type eof) : itype(MMAP), e(eof), decoder(decoder), max_l(length),
	l(length), d(0), s({})
{
	if (fd != -1) {
		max_l = lseek(fd, 0, SEEK_END);
		if (l == 0 || l > max_l) l = max_l;
		void* r = mmap(nullptr, l, PROT_READ, MAP_PRIVATE, fd, 0);
		if (r == MAP_FAILED) d = 0, l = 0, max_l = 0;
		else d = reinterpret_cast<const C*>(r);
		close(fd);
	}
}
template <typename C, typename T>
parser<C, T>::input::~input() {
	if (itype == MMAP && d != 0) munmap(const_cast<C*>(d), l);
}
template <typename C, typename T>
bool parser<C, T>::input::isstream() const { return itype == STREAM; }
template <typename C, typename T>
void parser<C, T>::input::clear() { if (isstream()) s.clear(); }
template <typename C, typename T>
C parser<C, T>::input::cur() {
	if (isstream()) return s.good() ? s.peek() : e;
	return n < max_l ? d[n] : e;
}
template <typename C, typename T>
bool parser<C, T>::input::next() {
	C ch;
	if (isstream())	return !s.good() ? false
		: n = s.tellg(), s.get(ch), l = n + (ch == e ? 0 : 1), true;
	return n < max_l ? ++n, true : false;
}
template <typename C, typename T>
size_t parser<C, T>::input::pos() { return n; }
template <typename C, typename T>
bool parser<C, T>::input::eof() {
	return cur() == e;
}
template <typename C, typename T>
C parser<C, T>::input::at(size_t p) {
	if (p >= l) return e;
	if (isstream()) return s.seekg(p), s.get();
	return d[p];
}
template <typename C, typename T>
T parser<C, T>::input::tcur() {
	if (!decoder) if constexpr (std::is_same_v<C, T>) return cur();
	if (tp >= ts.size()) decode();
	if (ts.size() == 0 || tp >= ts.size()) return T();
	if (teof()) return /*std::cout << "{TEOF}",*/ T();
	return ts[tp];
}
template <typename C, typename T>
bool parser<C, T>::input::tnext() {
	if ((decoder && teof()) ||
		(!decoder && (eof() || !next()))) return false;
	return ++tp, true;
}
template <typename C, typename T>
size_t parser<C, T>::input::tpos() { return tp; }
template <typename C, typename T>
T parser<C, T>::input::tat(size_t p) {
	if constexpr (std::is_same_v<C, T>) if (!decoder) return d[p];
	return ts[p];
}
template <typename C, typename T>
bool parser<C, T>::input::teof() {
	if (!decoder) return eof();
	if (tp >= ts.size()) decode();
	return tp >= ts.size();
}
template <typename C, typename T>
void parser<C, T>::input::decode() {
	if (!decoder || eof()) return;
	std::vector<T> x = decoder(*this);
	ts.insert(ts.end(), x.begin(), x.end());
}
//------------------------------------------------------------------------------
template <typename C, typename T>
parser<C, T>::parser(grammar<C, T>& g, const options& o) : g(g), o(o) {}
template <typename C, typename T>
lit<C, T> parser<C, T>::get_lit(const item& i) const {
	return g[i.prod][i.con][i.dot];
}
template <typename C, typename T>
lit<C, T> parser<C, T>::get_nt(const item& i) const { return g(i.prod); }
template <typename C, typename T>
std::pair<typename parser<C, T>::container_iter, bool>
	parser<C, T>::add(container_t& t, const item& i)
{
	//DBG(print(std::cout << "adding ", i) << std::endl;)
	// now return iterator along with whether insertions
	// succeeded or not
	auto& cont = S[i.set];
	auto it = cont.find(i);
	if (it != cont.end()) return {it, false};
	if ((it = t.find(i)) != t.end()) return {it, false};
	it = t.insert(i).first;
	if (nullable(*it))
		add(t, item(it->set, it->prod, it->con, it->from, it->dot + 1));
	return {it, true} ;
}
template <typename C, typename T>
bool parser<C, T>::nullable(const item& i) const {
	return i.dot < g[i.prod][i.con].size() && g.nullable(get_lit(i));
}
template <typename C, typename T>
bool parser<C, T>::completed(const item& i) const {
	const auto& a = g[i.prod][i.con];
	bool r = a.size() == i.dot;
	return r;
}
template <typename C, typename T>
void parser<C, T>::complete(const item& i, container_t& t) {
	//DBG(print(std::cout << "completing ", i) << std::endl;)
	const container_t& cont = S[i.from];
	for (auto it = cont.begin(); it != cont.end(); ++it)
		if (g[it->prod][it->con].size() > it->dot &&
			get_lit(*it) == get_nt(i))
		{
			add(t, item(i.set,it->prod,it->con,it->from,it->dot+1));
			// whence the item is completed, then
			// decrement the refcount for the one
			// that predicted and inserted it
			if (refi.count(*it) && refi[*it] > 0) --refi[*it];
		}
	//gcready.insert(i);
}
template <typename C, typename T>
std::vector<typename parser<C, T>::item> 
parser<C, T>::sorted_citem(std::pair<size_t, size_t> ntpos) {
	std::vector<item> items;
	if (memo.find(ntpos) != memo.end()) return memo[ntpos];
	for (size_t set :fromS[ntpos.second])
	//for ( size_t set=0; set < S.size(); set++)
	for (auto& i : S[set])
		if (completed(i)){
			if (i.from == ntpos.second && g(i.prod).n() == ntpos.first)
				items.push_back(i);
		}
		else if (o.binarize && i.dot >=2 && i.from == ntpos.second) {
			std::vector<lit<C, T>> v(g[i.prod][i.con].begin(),
				g[i.prod][i.con].begin() + i.dot);
			lit<C, T> l;
			if (bin_tnt.find(v) == bin_tnt.end()) {
				l = g.nt(from_str<C>(get_fresh_tnt()));
				bin_tnt.insert({ v, l });
			}
			else l = bin_tnt[v];
			if (l.n() == ntpos.first) 
				items.push_back(i);
		}
	return memo.insert({ntpos, items}), items;
}
template <typename C, typename T>
std::vector<typename parser<C, T>::item> 
parser<C, T>::rsorted_citem(std::pair<size_t, size_t> ntpos) {
	std::vector<item> items;

	if (rmemo.find(ntpos) != rmemo.end()) return rmemo[ntpos];
	for (auto& i : S[ntpos.second])
		if (completed(i)) {
				if (g(i.prod).n() == ntpos.first)
					items.push_back(i);
		}
		else if (o.binarize && i.dot >=2) {
			
			std::vector<lit<C, T>> v(g[i.prod][i.con].begin(),
				g[i.prod][i.con].begin() + i.dot);
			lit<C, T> l;
			if (bin_tnt.find(v) == bin_tnt.end()) {
				l = g.nt(from_str<C>(get_fresh_tnt()));
				bin_tnt.insert({ v, l });
			}
			else l = bin_tnt[v];
			if (l.n() == ntpos.first) 
				items.push_back(i);
		}
	return rmemo.insert({ntpos, items}), items;
}

template <typename C, typename T>
void parser<C, T>::resolve_conjunctions(container_t& t)	const {
	//DBG(std::cout << "resolve conjunctions...\n";)
	//DBG(print_S(std::cout) << std::endl;)
	std::map<std::pair<size_t, size_t>,
		std::set<std::pair<size_t, item>>> ps;
	for (const item& x : t) if (g.conjunctive(x.prod) && completed(x)) {
		if (ps.find({ x.prod, x.from }) == ps.end())
			ps[{ x.prod, x.from }] = {};
		ps[{ x.prod, x.from }].insert({ x.con, x });
	}
	for (const auto& p : ps) {
		bool conj_failed = false;
		for (size_t c = 0; c != g[p.first.first].size(); ++c) {
			bool found = false;
			for (const auto& x : p.second) {
				//DBG(print(std::cout << "\tcheck p: " << p.first.first
				//	<< " c: " << c << " x: " << x.first
				//	<< " ",
				//	x.second) << std::endl;)
				if (x.first == c) { found = true; break; }
			}
			conj_failed = (g[p.first.first][c].neg && found) ||
					(!g[p.first.first][c].neg && !found);
			if (conj_failed) break;
		}
		if (conj_failed) for (const auto& x : p.second) {
			//DBG(print(std::cout << "UNCOMPLETING: ", x.second) << std::endl;)
			t.erase(x.second);
		}
	}
	//DBG(std::cout << "... conjunctions resolved\n";)
}
template <typename C, typename T>
void parser<C, T>::predict(const item& i, container_t& t) {
	//DBG(print(std::cout << "predicting ", i) << std::endl;)
	for (size_t p : g.prod_ids_of_literal(get_lit(i)))
		for (size_t c = 0; c != g[p].size(); ++c) {
			// predicting item should have ref count increased
			// since predicting item, for its advancement over
			// non-terminal, depends on the completion of
			// predicted item.
			// same item can predict one or more predicted
			// items, if added, must increment predicting
			// item accordingly.
			// if different items predict the same predicted
			// item, just use one
			// Should we use S[n] to see if new item is insertable
			//
			if (add(t, item(i.set, p, c, i.set, 0)).second)
				++refi[i]; //insert and increment
		}
}
template <typename C, typename T>
void parser<C, T>::scan(const item& i, size_t n, T ch) {
	//DBG(print(std::cout << "scanning ", i) << " ch: " << to_std_string(ch) << std::endl;)
	if (ch != get_lit(i).t()) {
    	//when the item fails, decrement refcount of items that
 		//predicted it i.e predicting items ( only the one) for
        // which refc was incremented when this item was predicted
        const container_t& cont = S[i.from];
        for (auto it = cont.begin(); it != cont.end(); ++it)
        if (!completed(*it) && get_lit(*it) == get_nt(i))
            if (refi.count(*it) && refi[*it] > 0) --refi[*it];
        //DBG(std::cout<< "GC: adding failing scan\n");
        gcready.insert(i);
        return;
	}
	// by this time, terminal is advanced over
	// and new item j will be created.
	// hence, the previous item i can be marked
	// for gc
	if (!o.binarize) gcready.insert(i);
	item j(n + (ch == static_cast<T>(0) ? 0 : 1),
		i.prod, i.con, i.from, i.dot + 1);
	if (j.set >= S.size()) S.resize(j.set + 1);
	S[j.set].insert(j), fromS[j.from].push_back(j.set);
}
template <typename C, typename T>
void parser<C, T>::scan_cc_function(const item&i, size_t n, T ch,
	container_t& c)
{
	//DBG(print(std::cout << "scanning cc function ", i) << " s: `" << to_std_string(ch) << "`[" << (int_t) ch << "]" << std::endl;)
	size_t p = 0; // character's prod rule
	lit<C, T> l = get_lit(i);
	bool eof = ch == static_cast<T>(0) || ch == static_cast<T>(-1);
	//DBG(std::cout << "\tEOF: " << (eof ? "true" : "false") << "\n";)
	if (g.is_eof_fn(l.n())) {
		if (!eof) return;
		p = g.add_char_class_production(l, ch);
	} else {
		p = g.get_char_class_production(l, ch);
		if (p == static_cast<size_t>(-1)) return;
	}
	if (!eof) n++;
	item j(n, i.prod, i.con, i.from, i.dot + 1); // complete char fn
	if (j.set >= S.size()) S.resize(j.set + 1);
	//DBG(print(std::cout << "\tadding from cc scan j ", j) << " into S["<<j.set<<"]" << std::endl;)
	//S[j.set].insert(j);
	//if (completed(j)) c.insert(j); //complete(j, S[j.set]);
	item k(j.set, p, 0, j.set - (eof ? 0 : 1), 1); // complete char functions's char
	//DBG(print(std::cout << "\tadding from cc scan k ", k) << " into S["<<k.set<<"]" << std::endl;)
	S[k.set].insert(k), fromS[k.from].push_back(k.set);
	c.insert(k);
	//if (completed(k)) c.insert(k); //complete(j, S[j.set]);
	// i is scanned over and
	// item k is completed so collectible.
	if (!o.binarize) gcready.insert(i);
	//gcready.insert(k);
}
template <typename C, typename T>
std::unique_ptr<typename parser<C, T>::pforest> parser<C, T>::parse(
	const C* data, size_t size, int_type eof)
{
	//DBG(std::cout << "CHAR* l: " << l << std::endl;)
	in = std::make_unique<input>(data, size, o.chars_to_terminals, eof);
	return _parse();
}
template <typename C, typename T>
std::unique_ptr<typename parser<C, T>::pforest> parser<C, T>::parse(
	int fd, size_t size, int_type eof)
{
	in = std::make_unique<input>(fd, size, o.chars_to_terminals, eof);
	return _parse();
}
template <typename C, typename T>
std::unique_ptr<typename parser<C, T>::pforest> parser<C, T>::parse(
	std::basic_istream<C>& is, size_t size, int_type eof)
{
	in = std::make_unique<input>(is, size, o.chars_to_terminals, eof);
	return _parse();
}
template <typename C, typename T>
std::unique_ptr<typename parser<C, T>::pforest> parser<C, T>::_parse() {
	MS(emeasure_time_start(tsr, ter);)
	//DBG(std::cout << "parse: `"<<to_std_string(s)<<"`["<<len<<"] g.start:"<<g.start<<"("<<g.start.nt()<<")"<<"\n";)
	auto f = std::make_unique<pforest>();
	//sorted_citem.clear(), rsorted_citem.clear();
	S.clear(), bin_tnt.clear(), refi.clear(), gcready.clear();
	memo.clear(), rmemo.clear();
	int gcnt = 0; // count of collected items
	tid = 0;
	S.resize(1);
	container_t t, c;
	for (size_t p : g.prod_ids_of_literal(g.start_literal())) 
		for (size_t c = 0; c != g[p].size(); ++c) 
			++refi[*add(t, {0, p, c, 0, 0}).first];
#if MEASURE_EACH_POS
	size_t r = 1, cb = 0; // row and cel beginning
	clock_t tsp, tep;
#endif
	//DBG(size_t proc = 0;)
	T ch = 0;
	size_t n = 0, cn = -1;
	bool new_pos = true;
	do {
		if ((new_pos = (cn != in->pos()))) cn = in->pos();
		ch = in->tcur(), n = in->tpos();
		if (n >= S.size()) S.resize(n+1);
		//DBG(std::cout << "\ntpos: " << n << " ch: '" << to_std_string(ch) << "'" << std::endl;)
#if MEASURE_EACH_POS
		if (new_pos) {
			if (in->cur() == (C)'\n') (cb = n), r++;
			tsp = clock();
		}
#endif
		do {
			for (const item& x : t) 
				S[x.set].insert(x), fromS[x.from].push_back(x.set);
			t.clear();
			c.clear();
			const auto& cont = S[n];
			for (auto it = cont.begin(); it != cont.end(); ++it) {
				//DBG(print(std::cout << "\nprocessing(" << proc++ << ") ", *it) << std::endl;)
				if (completed(*it)) c.insert(*it);
				else if (get_lit(*it).nt()) {
					if (g.is_cc_fn(get_lit(*it).n()))
						scan_cc_function(*it, n, ch, c);
					else predict(*it, t);
				} else scan(*it, n, ch);
			}
			resolve_conjunctions(c);
			for (auto& x : c) complete(x, t);
		} while (!t.empty());
#if MEASURE_EACH_POS
		if (new_pos) {
			std::cout << in->pos() << " \tln: " << r << " col: "
				<< (n-cb+1) << " :: ";
			emeasure_time_end(tsp, tep) << "\n";
		}
#endif
		if (o.incr_gen_forest) {
			const auto& cont = S[n];
			// for (auto it = cont.begin(); it != cont.end(); ++it)
			//	pre_process(*it);
			for (auto it = cont.begin(); it != cont.end(); ++it)
				if (completed(*it)) {
					pnode curroot(get_nt(*it),
						{ it->from, it->set });
					build_forest(*f, curroot);
				}
			if (gcready.size())
				for (auto& rm : gcready) {
					if (refi[rm] == 0 &&
						(rm.set + o.gc_lag) <= n)
					{
						// since the refi is zero, remove it from the
						// main container
						S[rm.set].erase(rm);
						refi.erase(rm);
						gcnt++;
					}
					else {
						//DBG(assert(refi[rm] == 0));
					}
				//gcready.clear();
				}
		}
	} while (in->tnext());
	MS(emeasure_time_end(tsr, ter) <<" :: parse time\n";)
	in->clear();
	// remaining total items
	size_t count = 0;
	for (size_t i = 0 ; i< S.size(); i++)
		count += S[i].size();
	MS(std::cout << "\nGC: total input size = " << n;)
	MS(std::cout << "\nGC: total remaining  = " << count;)
	MS(std::cout << "\nGC: total collected  = " << gcnt;)
	if (count + gcnt)
		MS(std::cout << "\nGC: % = " << 100*gcnt/(count+gcnt) << std::endl);

	if (!o.incr_gen_forest) init_forest(*f);
	else f->root(pnode(g.start_literal(), { 0, in->tpos() }));
#if defined(DEBUG) && defined(WITH_DEVHELPERS)
	bool nt = f->has_single_parse_tree();
	std::cout << "forest has more than one parse tree:"
		<< (!nt ? "true" : "false") << std::endl;
	if (!nt) {
        /*
		std::cout<<"# parsed forest contains more than one tree\n";
		static int_t c = 0;
		std::stringstream ssf, ptd;
		ssf<<"parse_rules"<<++c<<".tml";
		std::ofstream file2(ssf.str());
		to_tml_rules<C, T>(ptd, f->g);
		file2 << ptd.str();
		file2.close();
		size_t i = 0;
		auto cb_next_graph = [&](parser<C, T>::pforest::graph& g){
			ssf.str({});
			ptd.str({});
			ssf<<"parse_rules"<<c<<"_"<<i++<<".tml";
			std::ofstream filet(ssf.str());
			to_tml_rules<C, T>(ptd, g);
			filet << ptd.str();
			filet.close();
			// get parse tree
			g.extract_trees();
			return true;
		};
		f->extract_graphs(f->root(), cb_next_graph);
        */
	}
#endif
	return f;
}
template <typename C, typename T>
bool parser<C, T>::found() {
	bool f = false;
	//DBG(std::cout<<"finding completed `start` over the input string: "<<std::endl;)
	//DBG(std::cout<<" start_nt: "<<g.start_literal().to_std_string()<<std::endl;)
	//print_S(std::cout);
	//DBG(std::cout << "tpos: " << in->tpos() << std::endl;)
	//for (const auto& x : S[in->tpos()]) print(std::cout << "\t", x) << std::endl;
	for (size_t n : g.prod_ids_of_literal(g.start_literal())) {
		//DBG(std::cout << "N: " << n << std::endl;)
		for (size_t c = 0; c != g[n].size(); ++c) {
			//DBG(print(std::cout << "find: ",
			//	item(in->tpos(), n, c, 0, g.len(n, c))) << std::endl;)
			//DBG(std::cout << "C: " << c << std::endl;)
			bool t = S[in->tpos()].find(
				item(in->tpos(), n, c, 0, g.len(n, c)))
					!= S[in->tpos()].end();
			//DBG(std::cout << "~: " << g[n][c].neg << std::endl;)
			//DBG(std::cout << "T: " << t << std::endl;)
			if (!g[n][c].neg) f = t;
			else if (t) f = false;
			//DBG(std::cout << "F: " << f << std::endl;)
			if (!f) break;
		}
		if (f) break;
	}
//#ifdef DEBUG
//	if (!f) print_data(std::cout);
//#endif
	//DBG(std::cout << "found? returning: " << f << std::endl;)
	return f;
}
template <typename C, typename T>
std::string parser<C, T>::perror_t::to_str(info_lvl elvl) {
	std::stringstream ss;
	auto s = to_std_string(unexp);
	if (s.size() == 1) {
		if (s[0] == '\n') s = "\\n";
		if (s[0] == '\r') s = "\\r";
	}
	s = s.size() == 0 ? "end of file" : "'" + s + "'";
	ss << "\nSyntax Error: Unexpected " << s << " at " << line << ":"
		<< col << " (" << loc+1 << "): ";
	for (size_t i = ctxt.size() - 1; i != 0; --i)
		if constexpr (std::is_same_v<T, char32_t>)
			ss << to_std_string(ctxt[i]);
		else ss << ctxt[i];
	ss << "\n";
	if (elvl == INFO_BASIC) return ss.str();
	for (auto& e : expv) {
		ss << "...expecting " << e.exp << " due to (" << e.prod_nt
			<< " =>" << e.prod_body << ")\n";
		if (elvl == INFO_DETAILED) continue;
		size_t tab = 0;
		const size_t max_bk_nest = 10; // 0 to disable
		for (auto& bk : e.bktrk) {
			tab++;
			if (tab == max_bk_nest) break;
			ss << "\t";
			for (size_t i = 0; i < tab; i++) ss << " ";
			ss << "..required from \"" << bk.exp << "\" in (" <<
				bk.prod_nt << " =>" << bk.prod_body << ")\n";
		}
	}
	return ss.str();
}
template <typename C, typename T>
std::vector<typename parser<C, T>::item> parser<C, T>::back_track(
	const item& obj)
{
	std::vector<item> ret;
	std::unordered_set<item, hasher_t> exists;
	item cur = obj;
	while (true) {
	//cerr<< S[cur.from].size() << " " << "(" << cur.from << " " << cur.set <<") ";
	//cerr<< get_nt(cur)<< " "<< g[cur.prod][cur.con] << endl
		auto pit = std::find_if(S[cur.from].begin(), S[cur.from].end(),
			[&](const item& a)
		{
			return exists.find(a) == exists.end() &&
				get_lit(a) == get_nt(cur) &&
				cur.from == a.set;
		});
		if (pit != S[cur.from].end()) cur = *pit, exists.insert(cur);
		else break;
		ret.push_back(*pit);
	}
	return ret;
}
template <typename C, typename T>
typename parser<C, T>::perror_t parser<C, T>::get_error() {
	perror_t err;
	auto near_ctxt = [this](int_t from, int_t pos) {
		std::vector<T> errctxt;
		// using from as delimiter..
		while (pos >= from)
			errctxt.push_back(in->tat(pos--));
		return errctxt;
	};
	std::stringstream es;
	auto item2_ept = [this, &es](const item& t ) {
		typename perror_t::exp_prod_t ept;
		es.str(""), es << get_lit(t), ept.exp = es.str(),
		es.str(""), es << get_nt(t),  ept.prod_nt = es.str(), es.str("");
		if (g[t.prod][t.con].neg) es << "~( ";
		for (size_t i = 0; i < g[t.prod][t.con].size(); i++) {
			es << " ";
			if (t.dot == i ) es << "• " ;
			es << g[t.prod][t.con][i];
		}
		if (g[t.prod][t.con].neg) es << " )";
		es << " ["<< t.from << "," << t.set << "]";
		ept.prod_body = es.str();
		return ept;
	};	// find error location and build error stream
	in->clear();
	for (int_t i = (int_t) in->tpos(); i >= 0; i--) if (S[i].size()) {
		size_t from = 0;
		// smallest length item that may be used as delimiter
		for (const item& t : S[i]) {
			if (t.from > from && t.from != t.set)
				from = t.from;
			//ignoring empty string, completed and nts
			if (completed(t) ||
				(get_lit(t).nt() && !g.is_cc_fn(get_lit(t).n()))
				|| (!get_lit(t).nt() &&	get_lit(t).is_null()))
					continue;
			err.expv.emplace_back(item2_ept(t));
			auto bv = back_track(t);
			for (const item& b : bv) err.expv.back()
					.bktrk.emplace_back(item2_ept(b));
		}
		err.unexp = in->tat(i);
		err.loc = i;
		break;
	}
	err.line = 1;
	size_t line_loc = 0;
	for (int_t j = 0; err.loc > -1 && j != err.loc; ++j)
		if (in->tat(j) == (T)'\n') err.line++, line_loc = j;
	err.col = err.loc - line_loc + 1;
	err.ctxt = near_ctxt(line_loc, err.loc + 1);
	return err;
}
template <typename C, typename T>
void parser<C, T>::pre_process(const item& i) {
	//sorted_citem[G[i.prod][0].n()][i.from].emplace_back(i);
	return;
	/*if (completed(i))
		// sorted_citem[{ g(i.prod).n(), i.from }].emplace_back(i);
		//rsorted_citem[{ g(i.prod).n(), i.set }].emplace_back(i);
	else if (o.binarize) {
		// Precreating temporaries to help in binarisation later
		// each temporary represents a partial rhs production with
		// atleast 3 symbols
		if (i.dot >= 2) {
			std::vector<lit<C, T>> v(g[i.prod][i.con].begin(),
					g[i.prod][i.con].begin() + i.dot);
			lit<C, T> l;
			if (bin_tnt.find(v) == bin_tnt.end()) {
				l = g.nt(from_str<C>(get_fresh_tnt()));
				bin_tnt.insert({ v, l });
			}
			else l = bin_tnt[v];
			//DBG(print(std::cout, i);)
			//cout<< "\n" << d->get(tlit.n()) << v << std::endl;
			//sorted_citem[{ l.n(), i.from }].emplace_back(i);
			//rsorted_citem[{ l.n(), i.set }].emplace_back(i);
		}
	}*/
}
template <typename C, typename T>
bool parser<C, T>::init_forest(pforest& f) {
	bool ret = false;
	bin_tnt.clear();
	//sorted_citem.clear();
	//rsorted_citem.clear();
	tid = 0;
	// set the start root node
	pnode root(g.start_literal(), { 0, in->tpos() });
	f.root(root);
	// preprocess parser items for faster retrieval
	MS(emeasure_time_start(tspfo, tepfo);)
	int count = 0;
	for (size_t n = 0; n < in->tpos() + 1; n++)
		for (const item& i : S[n]) count++, pre_process(i);
	MS(emeasure_time_end(tspfo, tepfo) << " :: preprocess time, " <<
						"size : "<< count << "\n";)
	
	// build forest
	MS(emeasure_time_start(tsf, tef);)
	ret = build_forest(f, root);
	MS(emeasure_time_end(tsf, tef) <<" :: forest time\n";)
	MS(std::cout << "memo sizes : " << memo.size() << " " <<
						rmemo.size() << " \n";)
	return ret;
}
// collects all possible variations of the given item's rhs while respecting the
// span of the item and stores them in the set ambset.
template <typename C, typename T>
void parser<C, T>::sbl_chd_forest(const item& eitem,
	std::vector<pnode>& curchd, size_t xfrom,
	std::set<std::vector<pnode>>& ambset)
{
	//check if we have reached the end of the rhs of prod
	if (g.len(eitem.prod, eitem.con) <= curchd.size())  {
		// match the end of the span we are searching in.
		if (curchd.back().second[1] == eitem.set) ambset.insert(curchd);
		return;
	}
	// curchd.size() refers to index of cur literal to process in the rhs of production
	pnode nxtl = { g[eitem.prod][eitem.con][curchd.size()], {} };
	// set the span start/end of the terminal symbol
	if (!nxtl.first.nt()) {
		nxtl.second[0] = xfrom;
		// for empty, use same span edge as from
		if (nxtl.first.is_null()) nxtl.second[1] = xfrom;
		// ensure well-formed combination (matching input) early
		else if (xfrom < in->tpos()
			 && in->tat(xfrom) == nxtl.first.t())
				nxtl.second[1] = ++xfrom;
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

		//auto& nxtl_froms = sorted_citem[nxtl.n()][xfrom];
		auto nxtl_froms = sorted_citem({ nxtl.first.n(), xfrom });
		for (auto& v : nxtl_froms) {
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
template <typename C, typename T>
bool parser<C, T>::binarize_comb(const item& eitem,
	std::set<std::vector<pnode>>& ambset)
{
	std::vector<pnode> rcomb, lcomb;
	if (eitem.dot < 1) return false;
	pnode right = { g[eitem.prod][eitem.con][eitem.dot - 1], {} };
	if (!right.first.nt()) {
		right.second[1] = eitem.set;
		if (right.first.is_null())
			right.second[0] = right.second[1];
		else if (in->tat(eitem.set - 1) == right.first.t())
			right.second[0] = eitem.set - 1;
		else return false;
		rcomb.emplace_back(right);
	} else {
		auto rightit = rsorted_citem({ right.first.n(), eitem.set });
		for (auto& it : rightit)
			if (eitem.from <= it.from)
				right.second[1] = it.set,
				right.second[0] = it.from,
				rcomb.emplace_back(right);
	}
	// many literals in rhs
	if (eitem.dot > 2) {
		//DBG(print(std::cout, eitem);)
		std::vector<lit<C, T>> v(g[eitem.prod][eitem.con].begin(),
					 g[eitem.prod][eitem.con].begin() +
						eitem.dot - 1);
		//DBG(assert(bin_tnt.find(v) != bin_tnt.end());)
		pnode left = { bin_tnt[v], {} };
		//DBG(std::cout << "\n" << d->get(bin_tnt[v].n()) << std::endl);
		auto leftit = sorted_citem({ left.first.n(), eitem.from });
		// doing left right optimization
		for (auto& it : leftit) for (auto& rit : rcomb)
			if (it.set == rit.second[0]) left.second[0] = it.from,
				left.second[1]=it.set,
				ambset.insert({ left, rit });
	}
	// exact two literals in rhs
	else if (eitem.dot == 2) {
		pnode left = { g[eitem.prod][eitem.con][eitem.dot - 2], {} };
		auto& l = left.first;
		if (!l.nt()) {
			left.second[0] = eitem.from;
			if (l.is_null()) left.second[1] = left.second[0];
			else if (in->tat(eitem.from) == l.t() )
				left.second[1] = eitem.from + 1;
			else return false;
			//do Left right optimisation
			for (auto& rit : rcomb)
				if (left.second[1] == rit.second[0])
					ambset.insert({ left, rit });
		}
		else {
			auto leftit = sorted_citem({ l.n(), eitem.from });
			for (auto& it : leftit)	for (auto& rit : rcomb)
				if (it.set == rit.second[0])
					left.second[0] = it.from,
					left.second[1] = it.set,
					ambset.insert({ left, rit });
		}
	}
	else {
		DBG(assert(eitem.dot == 1));
		for (auto& rit : rcomb)
			if (eitem.from <= rit.second[0])
				ambset.insert({ rit });
	}
	return true;
}
// builds the forest starting with root
template <typename C, typename T>
bool parser<C, T>::build_forest(pforest& f, const pnode& root) {
	if (!root.first.nt()) return false;
	if (f.contains(root)) return false;
	//auto& nxtset = sorted_citem[root.n()][root.second[0]];
	auto nxtset = sorted_citem({ root.first.n(), root.second[0] });
	pnodes_set ambset;
	for (const item& cur : nxtset) {
		if (cur.set != root.second[1]) continue;
		pnode cnode(completed(cur) ? g(cur.prod) : g.nt(root.first.n()),
			{ cur.from, cur.set });
		if (o.binarize) binarize_comb(cur, ambset);
		else {
			pnodes nxtlits;
			sbl_chd_forest(cur, nxtlits, cur.from, ambset);
		}
		f[cnode] = ambset;
		for (auto& aset : ambset)
			for (const pnode& nxt : aset) build_forest(f, nxt);
	}
	return true;
}
template <typename C, typename T>
size_t parser<C, T>::hasher_t::hash_size_t(const size_t& val) const{
	return std::hash<size_t>()(val) +
		0x9e3779b9 + (val << 6) + (val >> 2);
}
template <typename C, typename T>
size_t parser<C, T>::hasher_t::operator()(const std::pair<size_t, size_t>& k)
	const
{
	size_t h = 0; // lets substitute with better if possible.
	return h ^= hash_size_t(k.first), h ^= hash_size_t(k.second), h;
}
template <typename C, typename T>
size_t parser<C, T>::hasher_t::operator()(const pnode& k) const {
	// lets substitute with better if possible.
	size_t h = 0;
	h ^= hash_size_t(k.second[0]);
	h ^= hash_size_t(k.second[1]);
	h ^= hash_size_t(size_t(
		k.first.nt() ? k.first.n() : k.first.t()));
	return h;
}
template <typename C, typename T>
size_t parser<C, T>::hasher_t::operator()(const item& k) const {
	// lets substitute with better if possible.
	size_t h = 0;
	h ^= hash_size_t(k.set);
	h ^= hash_size_t(k.from);
	h ^= hash_size_t(k.prod);
	h ^= hash_size_t(k.con);
	h ^= hash_size_t(k.dot);
	return h;
}

template <typename C, typename T>
std::basic_ostream<T>& terminals_to_stream(std::basic_ostream<T>& os,
	const typename parser<C, T>::pforest& f,
	const typename parser<C, T>::pnode& root)
{
	auto cb_enter = [&os](const auto& n) {
		if (!n.first.nt() && !n.first.is_null()) os << n.first.t();
	};
	f.traverse(root, cb_enter);
	return os;
}
template <typename C, typename T>
std::basic_string<T> terminals_to_str(
	const typename parser<C, T>::pforest& f,
	const typename parser<C, T>::pnode& root)
{
	std::basic_stringstream<T> ss;
	terminals_to_stream<C, T>(ss, f, root);
	return ss.str();
}
template <typename C, typename T>
int_t terminals_to_int(
	const typename parser<C, T>::pforest& f,
	const typename parser<C, T>::pnode& root)
{
	//DBG(std::cout << "terminals_to_int: `" << to_std_string(terminals_to_str<C, T>(f, root)) << "`"<<std::endl;)
	return stoi(to_std_string(terminals_to_str<C, T>(f, root)));
}
#if defined(DEBUG) || defined(WITH_DEVHELPERS)
template <typename C, typename T>
std::ostream& parser<C, T>::print(std::ostream& os, const item& i) const {
	os << "I(" << i.prod;
	if (i.con) os << "/" << i.con;
	os << "•"<<i.dot<<" ["<<i.from << "," << i.set << "]):\t "<< std::flush;
	const auto& x = g[i.prod][i.con];
	os << g(i.prod) << "\t => ";
	if (x.neg) os << "~( ";
	for (size_t n = 0; n != x.size(); ++n) {
		if (n == i.dot) os << "• ";
		os << x[n].to_std_string(from_cstr<C>("ε ")) << " ";
	}
	if (x.size() == i.dot) os << "•";
	if (x.neg) os << " )";
	return os;
}
template <typename C, typename T>
std::ostream& parser<C, T>::print_S(std::ostream& os) const {
	for (size_t n = 0; n != S.size(); ++n) {
		os << "S["<<n<<"]:\n";
		for (const item& x : S[n]) print(os << "\t", x) << "\n";
	}
	return os;
}
template<typename C, typename T>
std::ostream& parser<C, T>::print_data(std::ostream& os) const {
	os << "S:\n";
	for (auto& x : S) {
		for (auto& y : x) if (y.from != y.set) print(os << "\t " <<
				y.prod << "/" << y.con << " \t", y) <<"\n";
		os << "---:\n";
	}
	os << "completed S:\n";
	for (auto& x : S) {
		for (auto& y : x)
			if (y.from != y.set && y.dot == g.len(y.prod, y.con))
				print(os << "\t " << y.prod << " \t", y) <<"\n";
		os << "---:\n";
	}
	return os;
}
#endif // DEBUG

} // idni namespace
#endif // __IDNI__PARSER__PARSER_TMPL_H__
