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
parser<C, T>::input::input(const C* data, size_t l, size_t max_l,
	decoder_type decoder, int_type eof) :
		itype(POINTER), e(eof), decoder(decoder), mm(),
		l(l), max_l(max_l > l ? l : max_l), d(data), s(nullptr) {}
template <typename C, typename T>
parser<C, T>::input::input(std::basic_istream<C>& is, size_t max_l,
	decoder_type decoder, int_type eof) : itype(STREAM), e(eof),
	decoder(decoder), mm(), l(0), max_l(max_l), d(0), s(nullptr)
	{ s.rdbuf(is.rdbuf()); }
template <typename C, typename T>
parser<C, T>::input::input(const std::string& filename, size_t max_l,
	decoder_type decoder, int_type eof) : itype(MMAP), e(eof),
	decoder(decoder), mm(filename, 0, MMAP_READ), l(mm.size()),
	max_l(max_l), d(reinterpret_cast<const C*>(mm.data())), s(nullptr)
{
	if (mm.error) std::cerr << mm.error_message << std::endl;
}
#ifndef _WIN32
template <typename C, typename T>
parser<C, T>::input::input(int fd, size_t max_l, decoder_type decoder,
	int_type eof) : itype(MMAP), e(eof), decoder(decoder),
	l(0), max_l(max_l), d(0), s({})
{
	if (fd != -1) {
		l = lseek(fd, 0, SEEK_END);
		if (l == 0 || (max_l > 0 && l > max_l)) l = max_l;
		void* r = mmap(nullptr, l, PROT_READ, MAP_PRIVATE, fd, 0);
		if (r == MAP_FAILED) d = 0, l = 0, max_l = 0;
		else d = reinterpret_cast<const C*>(r);
		close(fd);
	}
}
#endif

template <typename C, typename T>
parser<C, T>::input::~input() {
	//if (itype == MMAP && d != 0) munmap(const_cast<C*>(d), l);
}
template <typename C, typename T>
bool parser<C, T>::input::isstream() const { return itype == STREAM; }
template <typename C, typename T>
void parser<C, T>::input::clear() { if (isstream()) s.clear(); }
template <typename C, typename T>
C parser<C, T>::input::cur() {
	if (isstream()) return s.good() ? s.peek() : e;
	return n < l && (max_l == 0 || n < max_l) ? d[n] : e;
}
template <typename C, typename T>
bool parser<C, T>::input::next() {
	C ch;
	if (isstream())	return !s.good() ? false
		: n = s.tellg(), s.get(ch), l = n + (ch == e ? 0 : 1), true;
	return n < l && (max_l == 0 || n < max_l) ? ++n, true : false;
}
template <typename C, typename T>
size_t parser<C, T>::input::pos() { return n; }
template <typename C, typename T>
bool parser<C, T>::input::eof() {
	return cur() == e;
}
template <typename C, typename T>
C parser<C, T>::input::at(size_t p) {
	if (isstream()) return s.seekg(p), s.get();
	if (p >= l || (max_l != 0 && p >= max_l)) return e;
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
	if (!decoder) return at(p);
	if (p >= ts.size()) return T();
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
parser<C, T>::parser(grammar<C, T>& g, options o) : g(g), o(o) {}
template <typename C, typename T>
lit<C, T> parser<C, T>::get_lit(const item& i) const {
	return g[i.prod][i.con][i.dot];
}
template <typename C, typename T>
lit<C, T> parser<C, T>::get_nt(const item& i) const { return g(i.prod); }
template <typename C, typename T>
bool parser<C, T>::nullable(const item& i) const {
	return i.dot < n_literals(i) && g.nullable(get_lit(i));
}
template <typename C, typename T>
std::pair<typename parser<C, T>::container_iter, bool>
	parser<C, T>::add(container_t& t, const item& i)
{
	//DBGP(print(std::cout <<" +  trying to add\t\t\t", i) <<"\n";)
	// do not add if uncompleted
	if (U.size() < S.size()) U.resize(S.size());
	auto& ucont = U[i.set];
	auto it = ucont.find(i);
	if (it != ucont.end()) return { it, false };
	// now return iterator along with whether insertions
	// succeeded or not
	auto& cont = S[i.set];
	it = cont.find(i);
	if (it != cont.end()) return { it, false };
	if ((it = t.find(i)) != t.end()) return { it, false };
	it = t.insert(i).first;
	if (nullable(*it)) {
		item j(it->set, it->prod, it->con, it->from, it->dot + 1);
		if (add(t, j).second) {
			DBGP(print(std::cout <<
				" +  adding to t from nullable\t\t", j) <<"\n";)
		}
	}
	return { it, true };
}
template <typename C, typename T>
size_t parser<C, T>::n_literals(const item& i) const {
	return g[i.prod][i.con].size();
}
template <typename C, typename T>
std::pair<typename parser<C, T>::item, bool> parser<C, T>::get_conj(size_t set,
	size_t prod, size_t con) const
{
	auto& cont = S[set];
	for (auto it = cont.begin(); it != cont.end(); ++it)
		if (it->prod == prod && it->con == con)
			return { *it, true };
	return { item(0, 0, 0, 0, 0), false };
}
template <typename C, typename T>
bool parser<C, T>::completed(const item& i) const {
	return n_literals(i) == i.dot;
}
template <typename C, typename T>
bool parser<C, T>::negative(const item& i) const {
	return g[i.prod][i.con].neg;
}
template <typename C, typename T>
void parser<C, T>::resolve_conjunctions(container_t& c, container_t& t) {
	if (c.size() == 0) return;
	DBGP(std::cout << COLOR BLUE << "resolve conjunctions...\n"
		<< COLOR CLEAR;)
	std::map<std::pair<size_t, size_t>, std::set<item>> ps;
	// reorder by prod/from
	for (const item& x : c) ps[{ x.prod, x.from }].insert(x);
	for (const auto& pr : ps) {
		bool conj_failed = false;
		auto p = pr.first.first;
		for (size_t con = 0; con != g[p].size(); ++con) {
			DBGP(std::cout << "`" << g[p][con] << "`" << std::endl;)
			bool neg = g[p][con].neg;
			bool found = false;
			auto check = [&neg, &p, &con, &found, this]
				(const container_t& cont)
			{
				for (auto it=cont.begin();it!=cont.end();++it) {
					DBGP(print(std::cout <<
						" ?  checking cont \t\t\t\t",
						*it) << "\n";)
					if (it->prod == p && it->con == con
						&& !neg && completed(*it)) {
							found = true; break; }
				}
				return found;
			};
			for (const auto& x : pr.second) {
				DBGP(print(std::cout << "\tcheck p: " << p
					<< " c: " << con << " x.con: " << x.con
					<< " ", x) << std::endl;)
				if (x.con == con) { found = true; break; }
				if (!neg) continue;
				const auto& cont = S[x.set];
				auto dbg =[](const std::string& /*DBG(msg)*/){
					//DBG(std::cout << msg << "\n";)
					return false;
				};
				dbg("check c")    || check(c) ||
				dbg("check t")    || check(t) ||
				dbg("check cont") || check(cont);
			}
			DBGP(std::cout << "found: " << found
					<< " neg: " << neg << "\n";)
			if ((conj_failed = neg == found)) break;
		}
		if (conj_failed) for (const auto& x : pr.second) {
			DBGP(print(std::cout << "UNCOMPLETING: ", x) << std::endl;)
			if (U.size() < S.size()) U.resize(S.size());
			U[x.set].insert(x);
			S[x.set].erase(x);
			fromS[x.from].erase(remove_if(fromS[x.from].begin(),
				fromS[x.from].end(),
				[&x](size_t n) { return x.set == n; }));
			c.erase(x);
		}
	}
	//DBG(std::cout << "... conjunctions resolved\n";)
}
template <typename C, typename T>
void parser<C, T>::complete(const item& i, container_t& t, container_t& c,
	bool conj_resolved)
{
	DBGP(std::cout << "    completing\n";)
	if (!conj_resolved && g.conjunctive(i.prod)) {
		DBGP(print(std::cout <<	COLOR YELLOW <<
		 	" +  adding to c1 \t\t\t", i) << COLOR CLEAR << "\n";)
		c.insert(i);
		return;
	} else {
		if (add(t, i).second) {
			DBGP(print(std::cout << COLOR PURPLE <<
				" +  adding to t1 \t\t\t", i) << COLOR CLEAR <<
				"\n";)
		}
		// whence the item is completed, then
		// decrement the refcount for the one
		// that predicted and inserted it
		if (refi.count(i) && refi[i] > 0) --refi[i];
		//return;
	}
	const container_t& cont = S[i.from];
	for (auto it = cont.begin(); it != cont.end(); ++it) {
		if (n_literals(*it) <= it->dot ||
			get_lit(*it) != get_nt(i)) continue;
		DBGP(print(std::cout << " ?  checking \t\t\t\t", *it) << "\n";)
		item j(*it); ++j.dot, j.set = i.set;
		if (!negative(j) && completed(j) && g.conjunctive(j.prod)) {
			DBGP(print(std::cout << COLOR YELLOW <<
				" +  adding to c2 \t\t\t", j) << COLOR CLEAR <<
				"\n";)
			c.insert(j);
			continue;
		}
		//DBGP(std::cout << "neg: " << g[it->prod][it->con].neg << "\n";)
		if (add(t, j).second) {
			DBGP(print(std::cout << COLOR PURPLE <<
				" +  adding to t2 \t\t\t", j) << COLOR CLEAR <<
				"\n";)
		}
		// whence the item is completed, then
		// decrement the refcount for the one
		// that predicted and inserted it
		if (refi.count(*it) && refi[*it] > 0) --refi[*it];
	}
	//gcready.insert(i);
}
template <typename C, typename T>
void parser<C, T>::predict(const item& i, container_t& t) {
	DBGP(std::cout << "    predicting\n";)
	for (size_t p : g.prod_ids_of_literal(get_lit(i))) {
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
		for (size_t c = 0; c != g.n_conjs(p); ++c) {
			item j(i.set, p, c, i.set, 0);
			if (add(t, j).second) {
				DBGP(print(std::cout <<" +  adding to t \t\t\t",
					j) << "\n";)
				++refi[i]; //insert and increment
			}
		}
	}
}
template <typename C, typename T>
void parser<C, T>::scan(const item& i, size_t n, T ch) {
	DBGP(std::cout << "    scanning character: '"<<to_std_string(ch)<<"' ";)
	item j(n + (ch == static_cast<T>(0) ? 0 : 1),
		i.prod, i.con, i.from, i.dot + 1);
	DBGP(std::cout << (ch == get_lit(i).t()?"SUCCESS":"FAIL") << "\n";)
	if (ch != get_lit(i).t()) {
		//when the item fails, decrement refcount of items that
		//predicted it i.e predicting items ( only the one) for
		// which refc was incremented when this item was predicted
		const container_t& cont = S[i.from];
		for (auto it = cont.begin(); it != cont.end(); ++it)
			if (!completed(*it) && get_lit(*it) == get_nt(i) &&
				refi.count(*it) && refi[*it] > 0) --refi[*it];
		//DBG(std::cout<< "GC: adding failing scan\n");
		gcready.insert(i);
		return;
	}
	// by this time, terminal is advanced over
	// and new item j will be created.
	// hence, the previous item i can be marked
	// for gc
	if (!o.binarize) gcready.insert(i);
	if (j.set >= S.size()) S.resize(j.set + 1);
	DBGP(print(std::cout << " +  adding from scan into S[" << j.set <<
		"]: \t", j) << std::endl;)
	S[j.set].insert(j), fromS[j.from].push_back(j.set);
}
template <typename C, typename T>
void parser<C, T>::scan_cc_function(const item& i, size_t n, T ch,
	container_t& c)
{
	DBGP(std::cout << "    scanning cc function for char: `"
		<< to_std_string(ch) << "`[" << (int_t) ch << "]" << std::endl;)
	size_t p = 0; // character's prod rule
	lit<C, T> l = get_lit(i);
	bool eof = ch == static_cast<T>(0) || ch == static_cast<T>(-1);
	//DBG(std::cout << "\tEOF: " << (eof ? "true" : "false") << "\n";)
	bool eof_fn = g.is_eof_fn(l.n());
	if (eof_fn) {
		if (!eof) return;
		auto eofprods = g.prod_ids_of_literal(l);
		p = eofprods.size() == 1 ? *eofprods.begin()
			: g.add_char_class_production(l, ch);
	} else {
		p = g.get_char_class_production(l, ch);
		if (p == static_cast<size_t>(-1)) return;
	}
	if (!eof) n++;
	item k(n, p, 0, n - (eof ? 0 : 1), 1); // complete char functions's char
	DBGP(print(std::cout << " +  adding from cc scan into S[" << k.set <<
		"] \t", k) << "\n";)
	if (S.size() <= k.set) S.resize(k.set + 1);
	S[k.set].insert(k), fromS[k.from].push_back(k.set);
	// i is scanned over and item k is completed so collectible.
	if (!o.binarize) gcready.insert(i);
	//gcready.insert(k);
	if (eof_fn) c.insert(k); // add current item for completion if eof_fn
}
template <typename C, typename T>
std::unique_ptr<typename parser<C, T>::pforest> parser<C, T>::parse(
	const C* data, size_t size, parse_options po)
{
	in = std::make_unique<input>(data, size,
		po.max_length, o.chars_to_terminals, po.eof);
	return _parse(po.start);
}
template <typename C, typename T>
std::unique_ptr<typename parser<C, T>::pforest> parser<C, T>::parse(
	std::basic_istream<C>& is, parse_options po)
{
	in = std::make_unique<input>(is,
		po.max_length, o.chars_to_terminals, po.eof);
	return _parse(po.start);
}
template <typename C, typename T>
std::unique_ptr<typename parser<C, T>::pforest> parser<C, T>::parse(
	const std::string& fn, parse_options po)
{
	in = std::make_unique<input>(fn,
		po.max_length, o.chars_to_terminals, po.eof);
	return _parse(po.start);
}
#ifndef _WIN32
template <typename C, typename T>
std::unique_ptr<typename parser<C, T>::pforest> parser<C, T>::parse(
	int fd, parse_options po)
{
	in = std::make_unique<input>(fd,
		po.max_length, o.chars_to_terminals, po.eof);
	return _parse(po.start);
}
#endif
template <typename C, typename T>
std::unique_ptr<typename parser<C, T>::pforest> parser<C, T>::_parse(
	int_t start)
{
	MS(emeasure_time_start(tsr, ter);)
	//DBGP(std::cout << "parse: `" << to_std_string(s) << "`[" << len <<
	//	"] g.start:" << g.start << "(" << g.start.nt() << ")" << "\n";)

	DBGP(g.print_internal_grammar(std::cout << "grammar: \n", "\t", true)
		<< std::endl;)
	auto f = std::make_unique<pforest>();
	S.clear(), U.clear(), bin_tnt.clear(), refi.clear(),
		gcready.clear(), sorted_citem.clear(), rsorted_citem.clear();
	MS(int gcnt = 0;) // count of collected items
	tid = 0;
	S.resize(1);
	lit<C, T> start_lit = start!=-1 ? g.nt(start)
					: g.start_literal();
	container_t t, c;
	for (size_t p : g.prod_ids_of_literal(start_lit))
		for (size_t c = 0; c != g.n_conjs(p); ++c)
			++refi[*add(t, { 0, p, c, 0, 0 }).first];
#if MEASURE_EACH_POS
	size_t r = 1, cb = 0; // row and cel beginning
	clock_t tsp, tep;
#endif
#if DEBUG_PARSING
	size_t proc = 0;
#endif
	T ch = 0;
	size_t n = 0, cn = -1;
	bool new_pos = true;
	DBGP(print(std::cout << "\ninitial t:\n", t);)
	do {
		if ((new_pos = (cn != in->pos()))) cn = in->pos();
		ch = in->tcur(), n = in->tpos();
		if (n >= S.size()) S.resize(n + 1);
#if DEBUG_PARSING
		if (debug_at.second > 0) debug =
			debug_at.first <= n && n <= debug_at.second;
#endif
		DBGP(std::cout << "\n" << COLOR RED << "=================="
			"======================================================"
			"========\nPOS: '" << cn << "' TPOS: " << n << " CH: '"
			<< to_std_string(ch) << "'" << COLOR CLEAR <<std::endl;)
#if MEASURE_EACH_POS
		if (new_pos) {
			if (in->cur() == (C)'\n') (cb = n), r++;
			tsp = clock();
		}
#endif
		do {
#if DEBUG_PARSING
			size_t lproc = 0;
#endif
			//DBGP(print(std::cout << "t:\n", t);)
			for (const item& x : t)
				//print(std::cout << "adding from t into S[" << x.set << "]: ", x) << std::endl,
				S[x.set].insert(x),
				fromS[x.from].push_back(x.set);
			t.clear();
			const auto cont = S[n];
			//DBGP(print(std::cout << "\nto process:\n", cont);)
			for (auto it = cont.begin(); it != cont.end(); ++it) {
				DBGP(print(std::cout << "----------------------"
					"-----------------\n... processing ("
					<< lproc++ << " / " << proc++ << ")" <<
					" at S[" << n << "]: \t", *it) << "\n";)
				if (completed(*it)) complete(*it, t, c);
				else if (get_lit(*it).nt()) {
					if (g.is_cc_fn(get_lit(*it).n()))
						scan_cc_function(*it, n, ch, c);
					else predict(*it, t);
				} else scan(*it, n, ch);
			}
			//DBGP(print_S(std::cout << "S loop:\n") << "\n";)
			//DBGP(print(std::cout << "t loop:\n", t) << "\n";)
			if (t.empty()) {
				resolve_conjunctions(c, t);
				//DBGP(print(std::cout << "c after resolve:\n", c) << std::endl;)
				for (const auto& x : c) {
					DBGP(print(std::cout << "complete after resolve\t\t", x) << "\n";)
					complete(x, t, t, true);
				}
				c.clear();
			}
			//DBGP(if (!t.empty()) print(std::cout << "t not empty:\n", t) << "\n";)
		} while (!t.empty());
#if MEASURE_EACH_POS
		if (new_pos) {
			std::cout << in->pos() << " \tln: " << r << " col: "
				<< (n - cb + 1) << " :: ";
			emeasure_time_end(tsp, tep) << "\n";
		}
#endif
		if (o.incr_gen_forest) {
			const auto& cont = S[n];
			for (auto it = cont.begin(); it != cont.end(); ++it)
				pre_process(*it);
			for (auto it = cont.begin(); it != cont.end(); ++it)
				if (completed(*it) /*&& !negative(*it)*/) {
					pnode curroot(get_nt(*it),
						{ it->from, it->set });
					build_forest(*f, curroot);
				}
		}
		if( o.enable_gc) {
			for (auto& rm : gcready) {
				if (refi[rm] == 0 &&
					(rm.set + o.gc_lag) <= n)
				{
					// since the refi is zero, remove it from the
					// main container
					S[rm.set].erase(rm);
					refi.erase(rm);
					MS(gcnt++;)
				}
			}
		}
		DBGP(print_S(std::cout << "\n") << "\n";)
	} while (in->tnext());
	MS(emeasure_time_end(tsr, ter) <<" :: parse time\n";)
	in->clear();
	// remaining total items
	size_t count = 0;
	for (size_t i = 0; i < S.size(); i++) count += S[i].size();
	MS(std::cout << "\nGC: total input size = " << n;)
	MS(std::cout << "\nGC: total remaining  = " << count;)
	MS(std::cout << "\nGC: total collected  = " << gcnt;)
	MS(if (count + gcnt)
		std::cout << "\nGC: % = " << 100*gcnt/(count+gcnt) <<std::endl);

	if (!o.incr_gen_forest) init_forest(*f, start_lit);
	else f->root(pnode(start_lit, { 0, in->tpos() }));
#if defined(DEBUG) && defined(WITH_DEVHELPERS)
	bool nt = f->has_single_parse_tree();
	//std::cout << "forest has more than one parse tree:"
	//	<< (!nt ? "true" : "false") << std::endl;
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
bool parser<C, T>::found(int_t start) {
	//DBGP(print(std::cout << "Slast:\n", S[in->tpos()]) << std::endl;)
	bool f = false;
	for (size_t n : g.prod_ids_of_literal(start != -1 ? g.nt(start)
							: g.start_literal()))
	{
		f = true;
		item all(in->tpos(), n, 0, 0, 0);
		for (size_t c = 0; c != g.n_conjs(n); ++c) {
			all.con = c, all.dot = g.len(n, c);
			bool neg = g[n][c].neg;
			//DBG(print(std::cout << "search: ", all) << "\n";)
			//DBG(std::cout << "neg: " << neg << "\n";)
			f = S[in->tpos()].find(all) != S[in->tpos()].end();
			f = f != neg;
			if (!f)	break;
		}
		if (f) break;
	}
	return f;
}

/*
template <typename C, typename T>
std::vector<typename parser<C, T>::item> parser<C, T>::sorted_citem(
	std::pair<size_t, size_t> ntpos)
{
	std::vector<item> items;
	if (memo.find(ntpos) != memo.end()) return memo[ntpos];
	for (size_t set : fromS[ntpos.second]) for (auto& i : S[set])
		if (completed(i)) {
			if (i.from == ntpos.second &&
				g(i.prod).n()==ntpos.first) items.push_back(i);
		}
		else if (o.binarize && i.dot >= 2 && i.from == ntpos.second) {
			std::vector<lit<C, T>> v(g[i.prod][i.con].begin(),
				g[i.prod][i.con].begin() + i.dot);
			lit<C, T> l;
			if (bin_tnt.find(v) == bin_tnt.end()) {
				l = g.nt(from_str<C>(get_fresh_tnt()));
				bin_tnt.insert({ v, l });
			}
			else l = bin_tnt[v];
			if (l.n() == ntpos.first) items.push_back(i);
		}
	return memo.insert({ ntpos, items }), items;
}
template <typename C, typename T>
std::vector<typename parser<C, T>::item> parser<C, T>::rsorted_citem(
	std::pair<size_t, size_t> ntpos)
{
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
			if (l.n() == ntpos.first) items.push_back(i);
		}
	return rmemo.insert({ ntpos, items }), items;
}
*/
//------------------------------------------------------------------------------
template <typename C, typename T>
std::string parser<C, T>::error::to_str(info_lvl elvl) {
	std::stringstream ss;
	for (const auto& t : unexp) {
		std::string s = t.to_std_string();
		ss << s.substr(1, s.size() - 2);
	}
	std::string s{ ss.str() }; ss = {};
	if (s.size() && s != "\\0") {
		char quote = unexp.size() > 1 ? '"' : '\'';
		ss << quote << s << quote;
	} else ss << "end of file";
	s = ss.str();
	ss = {}, ss << "Syntax Error: Unexpected " << s << " at " << line << ":"
		<< col << " (" << loc+1 << "): ";
	for (size_t i = ctxt.size() - 1; i != 0; --i)
		if constexpr (std::is_same_v<T, char32_t>)
			ss << to_std_string(ctxt[i]);
		else ss << ctxt[i];
	if (elvl == INFO_BASIC) return ss.str();
	for (auto& e : expv) {
		ss << "\n...expecting " << e.exp << " due to (" << e.prod_nt
			<< " =>" << e.prod_body << ")";
		if (elvl == INFO_DETAILED) continue;
		size_t tab = 0;
		const size_t max_bk_nest = 10; // 0 to disable
		for (auto& bk : e.bktrk) {
			tab++;
			if (tab == max_bk_nest) break;
			ss << "\n\t";
			for (size_t i = 0; i < tab; i++) ss << " ";
			ss << "..required from \"" << bk.exp << "\" in (" <<
				bk.prod_nt << " =>" << bk.prod_body << ")";
		}
	}
	return ss.str();
}
template <typename C, typename T>
std::vector<typename parser<C, T>::item> parser<C, T>::back_track(
	const item& obj)
{
	std::vector<item> ret;
	std::set<item> exists;
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
typename parser<C, T>::error parser<C, T>::get_error() {
	error err;
	auto near_ctxt = [this](int_t from, int_t pos) {
		std::vector<T> errctxt;
		// using from as delimiter..
		while (pos >= from)
			errctxt.push_back(in->tat(pos--));
		return errctxt;
	};
	std::stringstream es;
	auto item2_ept = [this, &es](const item& t) {
		typename error::exp_prod_t ept;
		es.str(""), es << get_lit(t), ept.exp = es.str(),
		es.str(""), es << get_nt(t), ept.prod_nt = es.str(), es.str("");
		if (negative(t)) es << "~( ";
		for (size_t i = 0; i < n_literals(t); i++) {
			es << " ";
			if (t.dot == i) es << "• " ;
			es << g[t.prod][t.con][i];
		}
		if (negative(t)) es << " )";
		es << " ["<< t.from << "," << t.set << "]";
		ept.prod_body = es.str();
		return ept;
	};	// find error location and build error stream
	in->clear();
	//DBGP(std::cout << "tpos: " << in->tpos() << "\n";)
	//DBGP(print_S(std::cout << "S:\n") << "\n";)
	for (int_t i = (int_t) in->tpos(); i >= 0; i--) if (S[i].size()) {
		//DBG(std::cout << "get_error i pos = " << i << "\n";)
		size_t from = 0;
		bool unexp_neg = false;
		// smallest length item that may be used as delimiter
		for (const item& t : S[i]) {
			//DBG(print(std::cout << "t0 = ", t) << "\n";)
			if (t.from > from && t.from != t.set)
				from = t.from;
			//DBG(print(std::cout << "t1 = ", t) << "\n";)
			item x(t);
			if (g.conjunctive(t.prod)) {
				for (size_t c = 0; c != g.n_conjs(t.prod); ++c){
					auto[cit,ok] = get_conj(t.set,t.prod,c);
					//DBGP(std::cout << "c: " << c << " ok: " << ok;)
					//DBGP(if (ok) print(std::cout<< " cit: ", cit);)
					//DBGP(std::cout << std::endl;)
					if (!ok || !completed(cit)) {
						//DBGP(std::cout << "err 1: " << unexp << std::endl;)
						x.con = c, x.dot = cit.dot;
						//DBG(print(std::cout << "x = ", x) << "\n";)
						err.expv.emplace_back(
							item2_ept(x));
						auto bv = back_track(t);
						for (const item& b : bv)
							err.expv.back()
							.bktrk.emplace_back(
								item2_ept(b));
					}
				}
			} else {
				//ignoring empty string, completed and nts
				if (completed(t) && (get_nt(t).to_std_string()
						.rfind("__neg_", 0) == 0))
				{
					err.unexp = {};
					for (size_t i = t.from; i < t.set; ++i)
						err.unexp.emplace_back(in->tat(i));
					err.loc = t.from, unexp_neg = true;
				}
				if (completed(t) || (get_lit(t).nt()
					&& !g.is_cc_fn(get_lit(t).n())) ||
						(!get_lit(t).nt()
					&& get_lit(t).is_null())) {
						//DBG(std::cout << "continue\n";)
						continue;
					}
				//DBG(std::cout << "err2 " << std::endl;)
				err.expv.emplace_back(item2_ept(x));
				auto bv = back_track(x);
				for (const item& b : bv) err.expv.back()
						.bktrk.emplace_back(item2_ept(b));
				unexp_neg = false;
			}
		}
		if (!unexp_neg)
			err.unexp = lits<C, T>{ { in->tat(i) } }, err.loc = i;

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
	//DBGP(print(std::cout << " *  preprocessing\t\t\t", i) << std::endl;)
	//sorted_citem[G[i.prod][0].n()][i.from].emplace_back(i);
	if (completed(i))
		sorted_citem[{ g(i.prod).n(), i.from }].emplace_back(&i),
		rsorted_citem[{ g(i.prod).n(), i.set }].emplace_back(&i);
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
			sorted_citem[{ l.n(), i.from }].emplace_back(&i);
			rsorted_citem[{ l.n(), i.set }].emplace_back(&i);
		}
	}
}
template <typename C, typename T>
bool parser<C, T>::init_forest(pforest& f, const lit<C, T>& start_lit) {
	bool ret = false;
	bin_tnt.clear();
	sorted_citem.clear();
	rsorted_citem.clear();
	tid = 0;
	// set the start root node
	pnode root(start_lit, { 0, in->tpos() });
	f.root(root);
	// preprocess parser items for faster retrieval
	MS(emeasure_time_start(tspfo, tepfo);)
	int count = 0;
	for (size_t n = 0; n < in->tpos() + 1; n++)
		for (const item& i : S[n]) count++, pre_process(i);
	MS(emeasure_time_end(tspfo, tepfo) << " :: preprocess time, " <<
						"size : "<< count << "\n";)
	MS(std::cout << "sorted sizes : " << sorted_citem.size() << " " <<
						rsorted_citem.size() << " \n";)

	// build forest
	MS(emeasure_time_start(tsf, tef);)
	ret = build_forest(f, root);
	MS(emeasure_time_end(tsf, tef) <<" :: forest time\n";)

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
		auto& nxtl_froms = sorted_citem[{ nxtl.first.n(), xfrom }];
		for (auto& vp : nxtl_froms) {
			auto &v = *vp;
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
		auto &rightit = rsorted_citem[{ right.first.n(), eitem.set }];
		for (auto& itp : rightit) {
			auto &it = *itp;
			if (eitem.from <= it.from)
				right.second[1] = it.set,
				right.second[0] = it.from,
				rcomb.emplace_back(right);
		}
	}
	// many literals in rhs
	if (eitem.dot > 2) {
		//DBG(print(std::cout, eitem);)
		std::vector<lit<C, T>> v(g[eitem.prod][eitem.con].begin(),
			g[eitem.prod][eitem.con].begin() + eitem.dot - 1);
		//DBG(assert(bin_tnt.find(v) != bin_tnt.end());)
		pnode left = { bin_tnt[v], {} };
		//DBG(std::cout << "\n" << d->get(bin_tnt[v].n()) << std::endl);
		auto &leftit = sorted_citem[{ left.first.n(), eitem.from }];
		// doing left right optimization
		for (auto& itp : leftit) for (auto& rit : rcomb) {
			auto& it = *itp;
			if (it.set == rit.second[0]) left.second[0] = it.from,
				left.second[1] = it.set,
				ambset.insert({ left, rit });
		}
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
			auto &leftit = sorted_citem[{ l.n(), eitem.from }];
			for (auto& itp : leftit)	for (auto& rit : rcomb){
				auto& it = *itp;
				if (it.set == rit.second[0])
					left.second[0] = it.from,
					left.second[1] = it.set,
					ambset.insert({ left, rit });
			}
		}
	}
	else {
		DBG(assert(eitem.dot == 1));
		for (auto& rit : rcomb)
			if (eitem.from <= rit.second[0]) ambset.insert({ rit });
	}
	return true;
}
// builds the forest starting with root
template <typename C, typename T>
bool parser<C, T>::build_forest(pforest& f, const pnode& root) {
	if (!root.first.nt()) return false;
	if (f.contains(root)) return false;
	//auto& nxtset = sorted_citem[root.n()][root.second[0]];
	auto &nxtset = sorted_citem[{ root.first.n(), root.second[0] }];
	
	pnodes_set ambset;
	std::set<pnode> snodes;
	size_t last_p = SIZE_MAX;
	for (auto& curp : nxtset) {
		auto& cur = *curp;
		if (cur.set != root.second[1]) continue;
		pnode cnode(completed(cur) /*&& !negative(cur)*/
			? g(cur.prod) : g.nt(root.first.n()),
			{ cur.from, cur.set });
		pnodes_set cambset;
		if (o.binarize) binarize_comb(cur, cambset);
		else {
			pnodes nxtlits;
			//std::cout << "\n" << cur.prod << " " << last_p << " " << ambset.size();
			sbl_chd_forest(cur, nxtlits, cur.from, cambset);
		}

		// resolve ambiguity across productions, due to different earley items
		// with different prod id
		if(cambset.size()) {
			if( ambset.size() == 0)
				last_p = cur.prod, ambset = cambset;
			else if ( cur.prod < last_p)
				ambset.clear(), last_p = cur.prod, ambset = cambset;	
		}		
		f[cnode] = ambset;
		snodes.insert(cnode);	
		//std::cout << "\n A " << cur.prod << " " << last_p << " " << ambset.size();
	}

	// resolve ambiguity WITHIN production, where same production with same symbols 
	// of different individual span
	std::vector<int> gi; 
	int gspan = -1;
	int k = 0;
	std::vector<int> idxs;
	for(size_t i = 0; i <ambset.size(); i++)
		idxs.push_back(i);

	do {
		gi.clear();
		for (auto packidx : idxs) {
			auto apack = *next(ambset.begin(), packidx);
			pnode lt = apack[k];
			int span = lt.second[1] - lt.second[0];
			if (gspan == span) gi.push_back(k);
			if (gspan < span ) gspan = span, gi.clear(), gi.push_back(k);
			
		}
		k++;
		idxs.clear();
		idxs.insert(idxs.begin(),gi.begin(),gi.end());
		std::cout<<k;
	}
	while( k < int(ambset.size()) && gi.size() > 1 );
	//std::cout << gi.size() << std::endl;

	pnodes_set cambset;
	if(ambset.size())
		cambset.insert(*next(ambset.begin(), gi[0]));

	//std::cout <<" camb "<< cambset.size() << std::endl;
	if(snodes.size()) { 
		DBG( assert(snodes.size() == 1));
		f[*snodes.begin()] = cambset;
	}

	for (auto& aset : cambset)
		for (const pnode& nxt : aset) build_forest(f, nxt);

	return true;
}
template <typename C, typename T>
std::basic_string<C> parser<C, T>::input::get() {
	if (!isstream()) return std::basic_string<C>(d, l);
	std::basic_stringstream<C> ss;
	return ss << s.rdbuf(), clear(), ss.str();
}
template <typename C, typename T>
std::basic_string<C> parser<C, T>::get_input() {
	return in->get();
}
template <typename C, typename T>
std::basic_ostream<T>& terminals_to_stream(std::basic_ostream<T>& os,
	const typename parser<C, T>::pforest& f,
	const typename parser<C, T>::pnode& root)
{
	auto cb_enter = [&os](const auto& n) {
		if (!n.first.nt() && !n.first.is_null()) os << n.first.t();
	};
	auto ambig = [](const auto&, const auto& ns) {
		return decltype(ns){ *ns.begin() };
	};
	f.traverse(root, cb_enter, NO_EXIT, DO_REVISIT, ambig);
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
	const typename parser<C, T>::pnode& root, bool& error)
{
	//DBG(std::cout << "terminals_to_int: `" << to_std_string(terminals_to_str<C, T>(f, root)) << "`"<<std::endl;)
	std::stringstream is(to_std_string(terminals_to_str<C, T>(f, root)));
	int_t result = 0;
	error = false;
	if (!(is >> result)) error = true;
	return result;
}
#if defined(DEBUG) || defined(WITH_DEVHELPERS)
template <typename C, typename T>
std::ostream& parser<C, T>::print(std::ostream& os, const item& i) const {
	os << (completed(i) ? COLOR BRIGHT AND GREEN
		: i.dot == 0 ? COLOR BLACK : COLOR BRIGHT AND CLEAR);
	os << "(G" << (i.prod < 10 ? " " : "") << i.prod;
	if (g.conjunctive(i.prod)) os << "/" << i.con;
	else os << "  ";
	os << "• " << i.dot << " [" << i.from << ", " << i.set << "]):\t " << std::flush;
	const auto& x = g[i.prod][i.con];
	os << g(i.prod) << "\t => ";
	if (x.neg) os << "~( ";
	for (size_t n = 0; n != x.size(); ++n) {
		if (n == i.dot) os << "• ";
		os << x[n].to_std_string(from_cstr<C>("ε ")) << " ";
	}
	if (x.neg) os << ") ";
	if (i.dot == x.size()) os << "•";
	os << COLOR CLEAR;
	return os;
}
template <typename C, typename T>
std::ostream& parser<C, T>::print(std::ostream& os, const container_t& c,
	bool only_completed) const
{
	for (const item& x : c) if (!only_completed || completed(x))
		print(os << "\t", x) << "\n";
	return os;
}

template <typename C, typename T>
std::ostream& parser<C, T>::print_S(std::ostream& os,
	bool only_completed) const
{
	for (size_t n = 0; n != S.size(); ++n) {
		os << "S["<<n<<"]:\n";
		print(os, S[n], only_completed);
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
