// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.txt

#include "parser.h"

#ifdef TAU_PARSER_MEASURE
#include "utility/measure.h"
#endif // TAU_PARSER_MEASURE

namespace idni {

template <typename C, typename T>
std::ostream& operator<<(std::ostream& os,
	const std::pair<lit<C, T>, std::array<size_t, 2>>& obj)
{
	return os << obj.first << "[" << obj.second[0] << ","
					<< obj.second[1] << "]";
}

#ifndef PARSER_BINTREE_FOREST
template <typename C, typename T>
typename forest<typename parser<C,T>::pnode>::node
	parser<C,T>::pnode::ptrof(const typename parser<C,T>::pnode& pn)
{
	auto r = nid().emplace( pn, nullptr );
	if (r.second) r.first->second = typename
		forest<typename parser<C,T>::pnode>::node(&(r.first->first));
	return r.first->second;
}
#endif

//------------------------------------------------------------------------------

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
bool parser<C, T>::input::good() const {
	return    itype == STREAM ? s.good()
		: itype == MMAP   ? !mm.error
		:                   true;
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
	C ch{0};
	if (isstream())	return !s.good() ? false
		: n = s.tellg(), s.get(ch), l = n + (ch == e ? 0 : 1), true;
	return n < l && (max_l == 0 || n < max_l) ? ++n, true : false;
}
template <typename C, typename T>
size_t parser<C, T>::input::pos() { return n; }
template <typename C, typename T>
bool parser<C, T>::input::eof() { return cur() == e; }
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
std::basic_string<C> parser<C, T>::get_fresh_tnt() {
	static std::basic_string<C> prefix = { '_','_','B','_' };
	std::basic_stringstream<C> ss;
	ss << prefix << tid++;
	return ss.str();
}
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
void parser<C, T>::remove_item(const item& i){
	if(i.set == 0) return;
	bool inserted = gcready.insert(i).second;
	if (!inserted) return; // return if already in gc_ready
	const container_t& cont = S[i.from];
	for (auto it = cont.begin(); it != cont.end(); ++it){
		if (it->set == 0) continue;
		if (!completed(*it) && get_lit(*it) == get_nt(i)) {
			auto rit = refi.find(*it);
			if (rit == refi.end()){
				continue;
			}
			if (--rit->second == 0) {
				refi.erase(rit);
				if (o.enable_gc) {
					remove_item(*it);
				}
			}
		}
	}
	return;
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
	DBGP(std::cout << TC.BLUE() << "resolve conjunctions...\n"
		<< TC.CLEAR();)
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
		DBGP(print(std::cout <<	TC.YELLOW() <<
		 	" +  adding to c1 \t\t\t", i) << TC.CLEAR() << "\n";)
		c.insert(i);
		return;
	} else {
		if (add(t, i).second) {
			DBGP(print(std::cout << TC.MAGENTA() <<
				" +  adding to t1 \t\t\t", i) << TC.CLEAR() <<
				"\n";)
		}
		// whence the item is completed, then
		// decrement the refcount for the one
		// that predicted and inserted it
		//if (refi.count(i) && refi[i] > 0) --refi[i];
		//return;
	}
	//const container_t& cont = S[i.from];
	auto smbl = get_nt(i);
	auto &rng = cache[{smbl, i.from}];
	//for (auto it = cont.begin(); it != cont.end(); ++it) {
	for (auto eit : rng) {

		auto it = &eit;
		if (n_literals(*it) <= it->dot ||
			get_lit(*it) != get_nt(i)) continue;
		DBGP(print(std::cout << " ?  checking \t\t\t\t", *it) << "\n";)
		item j(*it); ++j.dot, j.set = i.set;
		if (!negative(j) && completed(j) && g.conjunctive(j.prod)) {
			DBGP(print(std::cout << TC.YELLOW() <<
				" +  adding to c2 \t\t\t", j) << TC.CLEAR() <<
				"\n";)
			c.insert(j);
			continue;
		}
		//DBGP(std::cout << "neg: " << g[it->prod][it->con].neg << "\n";)
		// j is parent of i, but with next non-terminal advanced
		if (add(t, j).second && o.enable_gc) {
			// — record the new GC‐edge for parent → j
			DBGP(print(std::cout << "Add Edge: ", *it) << " --> ";)
			DBGP(print(std::cout, j) << std::endl;)
			++refi[*it];
			
			// — your existing “added to t2” debug line
			DBGP(print(std::cout << TC.MAGENTA() <<
				" +  adding to t2 \t\t\t", j) << TC.CLEAR() << "\n";)

			// — now remove the old GC‐edge for parent → i
			DBGP(print(std::cout << "Remove Edge: ", *it) << " --> ";)
			DBGP(print(std::cout, i) << std::endl;)
			auto rit = refi.find(*it);
			DBG(assert(rit != refi.end() && rit->second>0);)
			if (--rit->second == 0) {
				refi.erase(rit);
				remove_item(*it);
			}
		}
	}
	//gcready.insert(i);
}
template <typename C, typename T>
void parser<C, T>::predict(const item& i, container_t& t) {
	DBGP(std::cout << "    predicting\n";)
	lit<C, T> parl = get_lit(i);
	//assert(parl.nt());
	//assert(!completed(i));
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
			//just once
			if( c==0 )cache[{parl, i.set}].insert(i);
			item j(i.set, p, c, i.set, 0);
			if (add(t, j).second) {
				DBGP(print(std::cout <<" +  adding to t \t\t\t",
					j) << "\n";)
				// if item is added, then it is a new item
				//if(S[i.set].find(j) != S[i.set].end()) {
					// j item is already present
				//}		
			}
			if(o.enable_gc) ++refi[i]; //insert and increment
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
		if(o.enable_gc) {
			//when the item fails, decrement refcount of items that
			//predicted it i.e predicting items ( only the one) for
			// which refc was incremented when this item was predicted
			auto rit = refi.find(i);
			if (rit != refi.end() && --rit->second == 0) {
				refi.erase(rit);
			}
			remove_item(i);
		}
		return;
	}
	// by this time, terminal is advanced over
	// and new item j will be created.
	// hence, the previous item i can be marked
	// for gc
	if (!o.binarize && o.enable_gc) gcready.insert(i);
	if (j.set >= S.size()) S.resize(j.set + 1);
	DBGP(print(std::cout << " +  adding from scan into S[" << j.set <<
		"]: \t", j) << std::endl;)
	S[j.set].insert(j), fromS[j.from].push_back(j.set);
	DBGP(print(std::cout << "Add Edge: ", i) << " --> ";)
    DBGP(print(std::cout, j) << std::endl;)
	//++refi[i];
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
	cache[{l, i.set}].insert(i);
	item k(n, p, 0, n - (eof ? 0 : 1), 1); // complete char functions's char
	DBGP(print(std::cout << " +  adding from cc scan into S[" << k.set <<
		"] \t", k) << "\n";)
	if (S.size() <= k.set) S.resize(k.set + 1);
	S[k.set].insert(k), fromS[k.from].push_back(k.set);
	DBGP(print(std::cout << "Add Edge: ", i) << " --> ";)  
	DBGP(print(std::cout, k) << "\n";)
	//++refi[i];
	// i is scanned over and item k is completed so collectible.
	if (!o.binarize && o.enable_gc ) gcready.insert(i);
	//gcready.insert(k);
	if (eof_fn) c.insert(k); // add current item for completion if eof_fn
}

template <typename C, typename T>
parser<C, T>::result parser<C, T>::parse(const C* data, size_t size,
	parse_options po)
{
	in_ = std::make_unique<input>(data, size,
		po.max_length, o.chars_to_terminals, po.eof);
	return _parse(po);
}
template <typename C, typename T>
parser<C, T>::result parser<C, T>::parse(std::basic_istream<C>& is,
	parse_options po)
{
	in_ = std::make_unique<input>(is,
		po.max_length, o.chars_to_terminals, po.eof);
	return _parse(po);
}
template <typename C, typename T>
parser<C, T>::result  parser<C, T>::parse(const std::string& fn,
	parse_options po)
{
	in_ = std::make_unique<input>(fn,
		po.max_length, o.chars_to_terminals, po.eof);
	return _parse(po);
}
#ifndef _WIN32
template <typename C, typename T>
parser<C, T>::result parser<C, T>::parse(int fd, parse_options po) {
	in_ = std::make_unique<input>(fd,
		po.max_length, o.chars_to_terminals, po.eof);
	return _parse(po);
}
#endif
template <typename C, typename T>
parser<C, T>::result parser<C, T>::_parse(const parse_options& po) {
	#ifdef TAU_PARSER_MEASURE
	measures::start_timer("parsing", po.measure);
	#endif // TAU_PARSER_MEASURE
	debug = po.debug;
	if (debug) {
		std::basic_string<C> tmp = in_->get_string();
		std::cout << "parse: `" << to_std_string(tmp) << "`"
			<< "`[" << tmp.size() << "] start sym:" << g.get_start()
			<< "(" << g.get_start().nt() << ")" << "\n";
		g.print_internal_grammar(std::cout << "grammar: \n", "\t", true)
			<< "\n";
	}
#ifndef PARSER_BINTREE_FOREST
	auto f = std::make_unique<pforest>();
#endif
	S.clear(), U.clear(), bin_tnt.clear(), refi.clear(), cache.clear(),
		gcready.clear(), sorted_citem.clear(), rsorted_citem.clear();
	//pnode::nid().clear();
	MS(int gcnt = 0;) // count of collected items
	tid = 0;
	S.resize(1);
	lit<C, T> start_lit = po.start != SIZE_MAX ? g.nt(po.start)
							: g.start_literal();
	container_t t, c;
	for (size_t p : g.prod_ids_of_literal(start_lit))
		for (size_t c = 0; c != g.n_conjs(p); ++c)
			++refi[*add(t, { 0, p, c, 0, 0 }).first];
	size_t r = 1, cb = 0; // row and cel beginning
	size_t proc = 0;
	T ch = 0;
	size_t n = 0, cn = -1;
	bool new_pos = true;
	DBGP(print(std::cout << "\ninitial t:\n", t);)
	do {
		if ((new_pos = (cn != in_->pos()))) cn = in_->pos();
		ch = in_->tcur(), n = in_->tpos();
		if (n >= S.size()) S.resize(n + 1);
		if (debug && debug_at.second > 0) debug =
			debug_at.first <= n && n <= debug_at.second;
		DBGP(std::cout << "\n" << TC.RED() << "=================="
			"======================================================"
			"========\nPOS: '" << cn << "' TPOS: " << n << " CH: '"
			<< to_std_string(ch) << "' (" << ((int) ch) << ") "
			<< TC.CLEAR() <<std::endl;)
		if (po.measure_each_pos && new_pos) {
			if (in_->cur() == (C)'\n') (cb = n), r++;
			#ifdef TAU_PARSER_MEASURE
			measures::start_timer("current character parsing");
			#endif // TAU_PARSER_MEASURE
		}

		do {
			size_t lproc = 0;
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

		if (po.measure_each_pos && new_pos) {
			std::cout << in_->pos() << " \tln: " << r << " col: "
				<< (n - cb + 1) << " :: ";
			#ifdef TAU_PARSER_MEASURE
			measures::print_timer("current character parsing");
			measures::restart_timer("current character parsing");
			#endif // TAU_PARSER_MEASURE
		}

#ifndef PARSER_BINTREE_FOREST
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
#endif
		if (o.enable_gc) {
			for (auto it = gcready.begin(); it != gcready.end();) {
				auto rm = *it;
				if ((rm.set + o.gc_lag) <= n) {
					if(refi.find(rm) != refi.end() && refi[rm] <= 0)
						refi.erase(rm);
					auto its = S[rm.set].find(rm);
					if( its != S[rm.set].end() ){
						S[rm.set].erase(its);
						MS(gcnt++;)
					}
					it = gcready.erase(it);
				}
				else it++;
			}
		}
		//DBGP(print_S(std::cout << "\n") << "\n";)

	} while (in_->tnext());

	#ifdef TAU_PARSER_MEASURE
	measures::print_timer("parsing");
	measures::stop_timer("parsing");
	#endif // TAU_PARSER_MEASURE

	in_->clear();
	// remaining total items
	size_t count = 0;
	for (size_t i = 0; i < S.size(); i++) count += S[i].size();
	MS(std::cout << "\nGC: total input size = " << n;)
	MS(std::cout << "\nGC: total remaining  = " << count;)
	MS(std::cout << "\nGC: total collected  = " << gcnt;)
	//MS(std::cout << "\nGC: unique collected  = " << unique_collected.size();)
	MS(std::cout << "\nGC: gcready size = " << gcready.size();)
	MS(std::cout << "\nGC: refi size = " << refi.size();)
	MS(if (count + gcnt)
		std::cout << "\nGC: % = " << 100*gcnt/(count+gcnt) <<std::endl);

#ifdef PARSER_BINTREE_FOREST
	tref fr;
	if (!o.incr_gen_forest) {
		fr = init_forest(start_lit, po);
		// tree::get(fr).print(std::cout << "forest in_ tree: ") << std::endl;
	}
#else
	if (!o.incr_gen_forest) init_forest(*f, start_lit, po);
		else f->root(pnode(start_lit, { 0, in_->tpos() }));
#endif

	if (debug) debug = false;

	bool fnd = found(po.start);
	error err = fnd ? error{} : get_error();

	MS(auto usen = f->count_useful_nodes(f->root());)
	MS(auto usenc = usen.first + usen.second;)
	
	MS(std::cout << "\nGC: Useful nodes"
		<<usen.first <<"+"<<usen.second << "=" << usenc <<"\n" );
	
	MS(if (count + gcnt)
		std::cout << "\nGC: useful% = " << 100*(usenc)
		/(count+gcnt) <<std::endl );
	MS(if (count + gcnt -usenc)
		std::cout << "\nGC: achieved% = " << 100*gcnt
		/(count+gcnt - usenc) <<std::endl);
	
	MS(if (count + gcnt)
		std::cout << "\nGC: potenial% = " << 100*(count+gcnt 
		- usenc)/(count + gcnt) <<std::endl);

#ifdef PARSER_BINTREE_FOREST
	return result(g, std::move(in_), fr, fnd, err);
#else
	return result(g, std::move(in_), std::move(f), fnd, err);
#endif
}
template <typename C, typename T>
bool parser<C, T>::found(size_t start) {
	//DBGP(print(std::cout << "Slast:\n", S[in_->tpos()]) << std::endl;)
	bool f = false;
	for (size_t n : g.prod_ids_of_literal(start != SIZE_MAX ? g.nt(start)
							: g.start_literal()))
	{
		f = true;
		item all(in_->tpos(), n, 0, 0, 0);
		for (size_t c = 0; c != g.n_conjs(n); ++c) {
			all.con = c, all.dot = g.len(n, c);
			bool neg = g[n][c].neg;
			//DBG(print(std::cout << "search: ", all) << "\n";)
			//DBG(std::cout << "neg: " << neg << "\n";)
			f = S[in_->tpos()].find(all) != S[in_->tpos()].end();
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
std::string parser<C, T>::error::to_str(info_lvl elvl, size_t line_start) const{
	if (ctxt.size() == 0) return "";
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
	ss = {}, ss << "Syntax Error: Unexpected " << s << " at "
		<< (line_start+line) << ":" << col << " (" << loc+1 << "): ";
	for (size_t i = ctxt.size() - 1; i != 0; --i)
		if (ctxt[i] != static_cast<T>(0)
			&& ctxt[i] != static_cast<T>(-1))
		{
			if constexpr (std::is_same_v<T, char32_t>)
				ss << to_std_string(ctxt[i]);
			else ss << ctxt[i];
		}
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
			return !completed(a) && exists.find(a) == exists.end()
				&& get_lit(a) == get_nt(cur)
				&& cur.from == a.set;
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
			errctxt.push_back(in_->tat(pos--));
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
	in_->clear();
	//DBGP(std::cout << "tpos: " << in_->tpos() << "\n";)
	//DBGP(print_S(std::cout << "S:\n") << "\n";)
	for (int_t i = (int_t) in_->tpos(); i >= 0; i--) if (S[i].size()) {
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
						.rfind("__N_", 0) == 0))
				{
					err.unexp = {};
					for (size_t i = t.from; i < t.set; ++i)
						err.unexp.emplace_back(in_->tat(i));
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
			err.unexp = lits<C, T>{ { in_->tat(i) } }, err.loc = i;

		break;
	}
	err.line = 1;
	size_t line_loc = 0;
	for (int_t j = 0; err.loc > -1 && j != err.loc; ++j)
		if (in_->tat(j) == (T)'\n') err.line++, line_loc = j;
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

#ifdef PARSER_BINTREE_FOREST
// build forest directly into tree (skip forest struct)
template <typename C, typename T>
tref parser<C, T>::init_forest(const lit<C, T>& start_lit,
	[[maybe_unused]] const parse_options& po)
{
	bin_tnt.clear();
	sorted_citem.clear();
	rsorted_citem.clear();
	tid = 0;
	// set the start root node
	pnode root(start_lit, { 0, in_->tpos() });
	// f.root(root);

	// preprocess parser items for faster retrieval
	#ifdef PARSER_MEASURE
	measures::start_timer("preprocess", po.measure_preprocess);
	#endif // PARSER_MEASURE

	int count = 0;
	for (size_t n = 0; n < in_->tpos() + 1; n++)
		for (const item& i : S[n]) count++, pre_process(i);

	#ifdef PARSER_MEASURE
	measures::print_timer("preprocess");
	measures::stop_timer("preprocess");
	std::cout << "preprocess size: " << count << "\n";
	std::cout << "sorted sizes : " << sorted_citem.size()
		<< " " << rsorted_citem.size() << " \n";
	#endif // PARSER_MEASURE

	// build forest
	#ifdef PARSER_MEASURE
	measures::start_timer("forest building", po.measure_forest);
	#endif // PARSER_MEASURE

	tref ret = build_forest(root);
	// f.print_data(std::cout) << "\n";

	#ifdef PARSER_MEASURE
	measures::print_timer("forest building");
	measures::stop_timer("forest building");
	#endif // PARSER_MEASURE

	return ret;
}
#else
template <typename C, typename T>
bool parser<C, T>::init_forest(pforest& f, const lit<C, T>& start_lit,
	[[maybe_unused]] const parse_options& po)
{
	bool ret = false;
	bin_tnt.clear();
	sorted_citem.clear();
	rsorted_citem.clear();
	tid = 0;
	// set the start root node
	pnode root(start_lit, { 0, in_->tpos() });
	f.root(root);

	// preprocess parser items for faster retrieval
	#ifdef TAU_PARSER_MEASURE
	measures::start_timer("preprocess", po.measure_preprocess);
	#endif // TAU_PARSER_MEASURE

	int count = 0;
	for (size_t n = 0; n < in_->tpos() + 1; n++)
		for (const item& i : S[n]) count++, pre_process(i);

	#ifdef TAU_PARSER_MEASURE
	measures::print_timer("preprocess");
	measures::stop_timer("preprocess");
	std::cout << "preprocess size: " << count << "\n";
	std::cout << "sorted sizes : " << sorted_citem.size()
		<< " " << rsorted_citem.size() << " \n";
	#endif // TAU_PARSER_MEASURE

	// build forest
	#ifdef TAU_PARSER_MEASURE
	measures::start_timer("forest building", po.measure_forest);
	#endif // TAU_PARSER_MEASURE

	ret = build_forest(f, root);
	// f.print_data(std::cout) << "\n";

	#ifdef TAU_PARSER_MEASURE
	measures::print_timer("forest building");
	measures::stop_timer("forest building");
	#endif // TAU_PARSER_MEASURE

	return ret;
}
#endif
// collects all possible variations of the given item's rhs while respecting the
// span of the item and stores them in the set ambset.
template <typename C, typename T>
void parser<C, T>::sbl_chd_forest(const item& eitem,
	pnodes& curchd, size_t xfrom,
	pnodes_set& ambset)
{
	//check if we have reached the end of the rhs of prod
	if (g.len(eitem.prod, eitem.con) <= curchd.size())  {
		// match the end of the span we are searching in.
#ifdef PARSER_BINTREE_FOREST
		static auto loc = [](pnode& n) -> location_type& { return n.second; };
		if (loc(curchd.back())[1] == eitem.set) ambset.insert(curchd);
#else
		if (curchd.back()->second[1] == eitem.set) ambset.insert(curchd);
#endif
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
		else if (xfrom < in_->tpos()
			 && in_->tat(xfrom) == nxtl.first.t())
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
	pnodes_set& ambset)
{
	std::vector<pnode> rcomb, lcomb;
	if (eitem.dot < 1) return false;
	pnode right = { g[eitem.prod][eitem.con][eitem.dot - 1], {} };
	if (!right.first.nt()) {
		right.second[1] = eitem.set;
		if (right.first.is_null())
			right.second[0] = right.second[1];
		else if (in_->tat(eitem.set - 1) == right.first.t())
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
			else if (in_->tat(eitem.from) == l.t() )
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
#ifdef PARSER_BINTREE_FOREST
// builds the forest from root
template <typename C, typename T>
tref parser<C, T>::build_forest(const pnode& root) {

	// checks if disambiguation is allowed for a node
	auto check_allowed = [this](const pnode &cnode) {
		if (g.opt.auto_disambiguate == false) return false;
		for (auto &nt : g.opt.nodisambig_list)
			if (cnode.first.nt() && cnode.first.n() == nt)
				return false;
		return true;
	};

	// find all relevant parse items for the node
	auto find_items = [&](const pnode& node) -> pnodes_set{
		auto &nxtset = sorted_citem[{ node.first.n(), node.second[0] }];
		if (nxtset.empty()) return {};

		pnodes_set ambset, cambset;
		std::set<pnode> snodes;
		size_t last_p = SIZE_MAX;

		for (auto& curp : nxtset) {
			auto& cur = *curp;
			// print(std::cout << "cur: ", cur) << "\n";
			if (cur.set != node.second[1]) continue;
			
			pnode cnode(completed(cur) /*&& !negative(cur)*/
				? g(cur.prod) : g.nt(node.first.n()),
				{ cur.from, cur.set });			
			cambset.clear();
			bool allowed_disambg = check_allowed(cnode);
			if (o.binarize) binarize_comb(cur,
					allowed_disambg ? cambset : ambset);
			else {
				pnodes nxtlits;
				// std::cout << "\n" << cur.prod << " " << last_p << " " << ambset.size();
				sbl_chd_forest(cur, nxtlits, cur.from,
					allowed_disambg ? cambset : ambset);
			}

			// resolve ambiguity across productions, due to different earley items
			// with different prod id
			if (allowed_disambg) {
				if (cambset.size()) { // any new sub forest
					if (ambset.size() == 0) // first time if
						last_p = cur.prod, ambset = cambset;
					else // get the smallest one
						if (last_p > cur.prod) ambset.clear(),
							last_p = cur.prod,
							ambset = cambset;
				}
				snodes.insert(cnode);
			}
		}

		// resolve ambiguity within the same production
		if (snodes.size() && check_allowed(*snodes.begin())) {
			std::vector<int> gi;
			int gspan = INT_MAX;
			int k = 0;
			std::vector<int> idxs;
			for(size_t i = 0; i < ambset.size(); i++)
				idxs.push_back(static_cast<int>(i));

			// choose the one with the smallest span for each level
			do {
				gi.clear();
				for (auto packidx : idxs) {
					auto apack = *next(ambset.begin(), packidx);
					pnode lt = apack[k];
					int span = static_cast<int>(lt.second[1] - lt.second[0]);					
					if (gspan == span) gi.push_back(packidx);
					if (gspan > span) gspan = span,
						gi.clear(),gi.push_back(packidx);
				}
				k++;
				idxs.clear();
				idxs.insert(idxs.begin(), gi.begin(), gi.end());
			} while (k < static_cast<int>(ambset.size()) && gi.size() > 1);

			cambset.clear();
			if (ambset.size() && gi.size())
				cambset.insert(*next(ambset.begin(), gi[0]));
		}

		// choose which set to use for building
		return (snodes.size() && check_allowed(*snodes.begin())) 
			? cambset : ambset;
	};

	auto is_ebnf = [](const pnode& node) -> bool {
		std::string name = node.first.to_std_string();
		return name.size() >= 4 && name.substr(0, 4) == "__E_";
	};

	std::set<pnode> visited;
	std::function<void(const pnode&, trefs&)> build_children;
	build_children = [&visited, &find_items, &is_ebnf, &build_children]
		(const pnode& node, trefs& children)
	{
		// std::cout << "build_children for node: `" << node << "`" << std::endl;
		if (!node.first.nt() || visited.count(node)) return;
		visited.insert(node);
		for (const auto& aset : find_items(node))
			for (const auto &nxt : aset)
				if (is_ebnf(nxt))
					build_children(nxt, children);
				else {
					trefs ch;
					build_children(nxt, ch);
					children.push_back(tree::get(nxt, ch));
				}
	};
	trefs ch;
	build_children(root, ch);
	return tree::get(root, ch);
}
#else
// builds the forest starting with root
template <typename C, typename T>
bool parser<C, T>::build_forest(pforest& f, const pnode& root) {
	if (!root.first.nt()) return false;
	if (f.contains(root)) return false;
	// std::cout << "build_forest for node: `" << root << "`" << std::endl;
	//auto& nxtset = sorted_citem[root.n()][root.second[0]];
	auto &nxtset = sorted_citem[{ root.first.n(), root.second[0] }];

	pnodes_set ambset, cambset;
	std::set<pnode> snodes;
	size_t last_p = SIZE_MAX;
	auto check_allowed = [this](const pnode &cnode) {
		if (g.opt.auto_disambiguate == false) return false;
		for (auto &nt : g.opt.nodisambig_list)
			if (cnode.first.nt() && cnode.first.n() == nt)
				return false;
		return true;
	};

	for (auto& curp : nxtset) {
		auto& cur = *curp;
		// print(std::cout << "cur: ", cur) << std::endl;
		if (cur.set != root.second[1]) continue;
		pnode cnode(completed(cur) /*&& !negative(cur)*/
			? g(cur.prod) : g.nt(root.first.n()),
			{ cur.from, cur.set });
		cambset.clear();
		bool allowed_disambg = check_allowed(cnode);
		if (o.binarize) binarize_comb(cur,
						allowed_disambg ? cambset : ambset);
		else {
			pnodes nxtlits;
			//std::cout << "\n" << cur.prod << " " << last_p << " " << ambset.size();
			sbl_chd_forest(cur, nxtlits, cur.from,
						allowed_disambg ? cambset : ambset);
		}

		// resolve ambiguity across productions, due to different earley items
		// with different prod id
		if (allowed_disambg) {
			if (cambset.size()) { // any new sub forest
				if (ambset.size() == 0) // first time if
					last_p = cur.prod, ambset = cambset;
				else {
					// get the smallest one
					if (last_p > cur.prod) ambset.clear(),
						last_p = cur.prod,
						ambset = cambset;
				}
			}

			snodes.insert(cnode);
		}
		f[cnode] = ambset;
		//std::cout << "\n A " << cur.prod << " " << last_p << " " << ambset.size();
	}

	if (snodes.size() && check_allowed(*snodes.begin())) {

		// resolve ambiguity if WITHIN production, where same production with same symbols
		// of different individual span
		std::vector<int> gi;
		int gspan = INT_MAX;
		int k = 0;
		std::vector<int> idxs;
		for(size_t i = 0; i < ambset.size(); i++) idxs.push_back(i);

		//choose the one with the first smallest span
		do {
			gi.clear();
			for (auto packidx : idxs) {
				auto apack = *next(ambset.begin(), packidx);
				pnode lt = apack[k];
				int span = lt.second[1] - lt.second[0];
				if (gspan == span) gi.push_back(packidx);
				if (gspan > span) gspan = span,
						gi.clear(), gi.push_back(packidx);

			}
			k++;
			idxs.clear();
			idxs.insert(idxs.begin(),gi.begin(),gi.end());
		}
		while (k < int(ambset.size()) && gi.size() > 1);

		cambset.clear();
		if (ambset.size())
			cambset.insert(*next(ambset.begin(), gi[0]));

	//std::cout <<" camb "<< cambset.size() << std::endl;
		if (snodes.size()) {
			DBG(assert(snodes.size() == 1));
			f[*snodes.begin()] = cambset;
		}
	//std::cout << gi.size() << std::endl;
	}

	for (auto& aset : (snodes.size() && check_allowed(*snodes.begin()))
			? cambset : ambset)
		for (const auto &nxt : aset) build_forest(f, nxt);

	return true;
}
#endif
template <typename C, typename T>
std::basic_string<C> parser<C, T>::input::get_string() {
	if (!isstream()) return std::basic_string<C>(d, l);
	std::basic_stringstream<C> ss;
	return ss << s.rdbuf(), clear(), ss.str();
}
template <typename C, typename T>
std::basic_string<T> parser<C, T>::input::get_terminals(
	std::array<size_t, 2> pos_span)
{
	return get_terminals(pos_span[0], pos_span[1]);
}
template <typename C, typename T>
std::basic_string<T> parser<C, T>::input::get_terminals(
	size_t start, size_t end)
{
	if (decoder) {
		if (start > ts.size()) start = ts.size();
		if (end > ts.size()) end = ts.size();
		return std::basic_string<T>(ts.begin() + start,
			ts.begin() + end);
	}
	if (!isstream()) return std::basic_string<T>(d + start, end - start);
	std::basic_stringstream<T> ss;
	s.seekg(start);
	while (end > start++) ss << s.get();
	return clear(), ss.str();
}

template <typename C, typename T>
std::ostream& parser<C, T>::print(std::ostream& os, const item& i) const {
	using namespace idni::term;
	os << (completed(i) ? TC(color::BRIGHT, color::GREEN)
		: i.dot == 0 ? TC.BLACK() : TC(color::BRIGHT, color::CLEAR));
	os << "(G" << (10 >= i.prod ? " " : "") << i.prod;
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
	os << TC.CLEAR();
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

} // idni namespace
