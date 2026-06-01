// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <climits>

#include "parser.h"

#if defined(TAU_PARSER_MEASURE_SCOPES) \
	|| defined(TAU_PARSER_MEASURE_COUNTERS)
#include "utility/measure.h"
#endif

namespace idni {

template <typename C, typename T>
std::ostream& operator<<(std::ostream& os,
	const std::pair<lit<C, T>, std::array<size_t, 2>>& obj)
{
	return os << obj.first << "[" << obj.second[0] << ","
					<< obj.second[1] << "]";
}

template <typename C, typename T>
typename forest<pnode_type<C,T>>::node pnode_type<C,T>::ptrof(const pnode_type<C,T>& pn)
{
	auto r = nid().try_emplace(pn);
	if (r.second) {
		r.first->second.id = &(r.first->first);
		r.first->second.hash = r.first->first.hash;
	}
	return r.first->second;
}

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
	decoder_type decoder, int_type eof,
	idni::diagnostics::report* diag) : itype(MMAP), e(eof),
	decoder(decoder),
#ifdef _WIN32
	mm(idni::utf8_to_wide(filename), 0, MMAP_READ),
#else
	mm(filename, 0, MMAP_READ),
#endif
	l(mm.size()),
	max_l(max_l), d(reinterpret_cast<const C*>(mm.data())), s(nullptr)
{
	if (mm.error) {
		if (diag) diag->error(idni::diagnostics::code::io_error,
			mm.error_message);
		else std::cerr << mm.error_message << std::endl;
	}
}
#ifdef _WIN32
template <typename C, typename T>
parser<C, T>::input::input(const std::wstring& filename, size_t max_l,
	decoder_type decoder, int_type eof,
	idni::diagnostics::report* diag) : itype(MMAP), e(eof),
	decoder(decoder), mm(filename, 0, MMAP_READ), l(mm.size()),
	max_l(max_l), d(reinterpret_cast<const C*>(mm.data())), s(nullptr)
{
	if (mm.error) {
		if (diag) diag->error(idni::diagnostics::code::io_error,
			mm.error_message);
		else std::cerr << mm.error_message << std::endl;
	}
}
#else
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
parser<C, T>::parser(grammar<C, T>& g, options o) : g(g), o(o), po(o.parse_opts)
{
	for (size_t p = 0; p < g.size(); p++)
		if (g.conjunctive(p)) { any_conj = true; break; }
}
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
	MC(count(cnt.remove_item_calls);)
	if(i.set == 0) return;
	bool inserted = gcready.insert(i).second;
	if (!inserted) return; // return if already in gc_ready
	const container_t& cont = S[i.from];
	MC(count(cnt.remove_item_scan_total, cont.size());)
	for (auto it = cont.begin(); it != cont.end(); ++it){
		if (it->set == 0) continue;
		if (!completed(*it) && get_lit(*it) == get_nt(i)) {
			auto rit = refi.find(*it);
			if (rit == refi.end()){
				continue;
			}
			MC(count(cnt.refi_decrements);)
			if (--rit->second == 0) {
				MC(count(cnt.refi_zero_hits);)
				refi.erase(rit);
				if (po.enable_gc) remove_item(*it);
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
	// U is empty for non-conjunctive grammars (cascade never fires);
	// avoid the find on the per-set bucket entirely in that case.
	if (any_conj) {
		if (U.size() < S.size()) U.resize(S.size());
		auto& ucont = U[i.set];
		if (auto uit = ucont.find(i); uit != ucont.end())
			return { uit, false };
	}
	auto& cont = S[i.set];
	if (auto sit = cont.find(i); sit != cont.end())
		return { sit, false };
	auto [it, inserted] = t.insert(i);
	if (!inserted) return { it, false };
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
bool parser<C, T>::nt_still_completed(size_t nt_id, size_t from, size_t set)
	const
{
	completion_key ckey{ nt_id, from, set };
	auto it = completion_count.find(ckey);
	return it != completion_count.end() && it->second > 0;
}

template <typename C, typename T>
void parser<C, T>::cascade_uncomplete(size_t nt_id, size_t from, size_t set,
	container_t& c)
{
	MC(count(cnt.cascade_uncomplete_calls);)
	if (nt_still_completed(nt_id, from, set)) return;
	completion_key ckey{ nt_id, from, set };
	auto dit = completion_deps.find(ckey);
	if (dit == completion_deps.end()) return;
	auto deps = std::move(dit->second);
	completion_deps.erase(dit);
	MC(count(cnt.cascade_dep_total, deps.size());)
	for (const item& dep : deps) {
		if (U.size() < S.size()) U.resize(S.size());
		auto sit = S[dep.set].find(dep);
		if (sit == S[dep.set].end()) continue;
		S[dep.set].erase(sit);
		U[dep.set].insert(dep);
		if (!completed(dep) && get_lit(dep).nt()) {
			auto cit = cache.find({get_lit(dep).n(), dep.set});
			if (cit != cache.end()) cit->second.erase(dep);
		}
		if (auto fit = fromS.find(dep.from); fit != fromS.end()) {
			MC(count(cnt.fromS_reads);)
			fit->second.erase(dep.set);
			if (fit->second.empty()) fromS.erase(fit);
		}
		c.erase(dep);
		if (completed(dep) && get_nt(dep).nt()) {
			completion_key dkey{
				get_nt(dep).n(), dep.from, dep.set };
			auto dcit = completion_count.find(dkey);
			if (dcit != completion_count.end()
				&& dcit->second > 0)
				--dcit->second;
			cascade_uncomplete(
				get_nt(dep).n(), dep.from, dep.set, c);
		}
	}
}

template <typename C, typename T>
void parser<C, T>::resolve_conjunctions(container_t& c) {
	if (c.size() == 0) return;
	MC(count(cnt.conjunction_attempts);)
	MC(maks(cnt.c_size_peak, c.size());)
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
			for (const auto& x : pr.second) {
				DBGP(print(std::cout << "\tcheck p: " << p
					<< " c: " << con << " x.con: " << x.con
					<< " ", x) << std::endl;)
				if (x.con == con) { found = true; break; }
				if (!neg) continue;
				if (g[p][con].size() == 1
					&& g[p][con][0].nt()
					&& nt_still_completed(g[p][con][0].n(),
						x.from, x.set))
				{
					found = true; break;
				}
			}
			DBGP(std::cout << "found: " << found
					<< " neg: " << neg << "\n";)
			if ((conj_failed = neg == found)) break;
		}
		if (conj_failed) {
			MC(count(cnt.conjunction_failures);)
			for (const auto& x : pr.second) {
				DBGP(print(std::cout << "UNCOMPLETING: ", x) << std::endl;)
				if (U.size() < S.size()) U.resize(S.size());
				U[x.set].insert(x);
				S[x.set].erase(x);
				if (auto fit = fromS.find(x.from); fit != fromS.end()) {
					MC(count(cnt.fromS_reads);)
					fit->second.erase(x.set);
					if (fit->second.empty()) fromS.erase(fit);
				}
				c.erase(x);
			}
			// decrement count for head NT and cascade
			auto head = g(p);
			auto it0 = pr.second.begin();
			if (head.nt()) {
				completion_key hkey{
					head.n(), it0->from, it0->set };
				auto cit = completion_count.find(hkey);
				if (cit != completion_count.end()
					&& cit->second > 0) --cit->second;
			}
			if (head.nt()) cascade_uncomplete(
				head.n(), it0->from, it0->set, c);
		}
	}
	//DBG(std::cout << "... conjunctions resolved\n";)
}
template <typename C, typename T>
void parser<C, T>::complete(const item& i, container_t& t, container_t& c,
	bool conj_resolved)
{
	MC(count(cnt.complete_calls);)
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
	auto &rng = cache[{smbl.n(), i.from}];
	completion_key ckey{ smbl.n(), i.from, i.set };
	if (any_conj) ++completion_count[ckey];
	// per-item memoization: only iterate cache entries new since last call.
	const size_t cur_size = rng.size();
	size_t& last_idx = complete_memo[i];
	if (last_idx >= cur_size) return;
	const auto& vec = rng.values();
	const size_t start_idx = last_idx;
	last_idx = cur_size;
	for (size_t k = start_idx; k < cur_size; k++) {
		const auto& eit = vec[k];
		const auto* it = &eit;
		if (n_literals(*it) <= it->dot ||
			get_lit(*it) != get_nt(i)) continue;
		// Predictor may have been evicted from S; it can still drive
		// completion only if conjunction-cascade machinery kept it in U.
		bool in_S = S[it->set].find(*it) != S[it->set].end();
		bool in_U = any_conj && it->set < U.size()
			&& U[it->set].find(*it) != U[it->set].end();
		if (!in_S && !in_U) continue;
		DBGP(print(std::cout << " ?  checking \t\t\t\t", *it) << "\n";)
		item j(*it); ++j.dot, j.set = i.set;
		if (!negative(j) && completed(j) && g.conjunctive(j.prod)) {
			DBGP(print(std::cout << TC.YELLOW() <<
				" +  adding to c2 \t\t\t", j) << TC.CLEAR() <<
				"\n";)
			c.insert(j);
			if (any_conj) {
				completion_deps[ckey].push_back(j);
				MC(maks(cnt.completion_deps_size_peak,
					completion_deps[ckey].size());)
			}
			continue;
		}
		//DBGP(std::cout << "neg: " << g[it->prod][it->con].neg << "\n";)
		// j is parent of i, but with next non-terminal advanced
		if (add(t, j).second) {
			if (any_conj) {
				completion_deps[ckey].push_back(j);
				MC(maks(cnt.completion_deps_size_peak,
					completion_deps[ckey].size());)
			}
			if (po.enable_gc) {
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
	}
	//gcready.insert(i);
}
template <typename C, typename T>
void parser<C, T>::predict(const item& i, container_t& t) {
	MC(count(cnt.predict_calls);)
	DBGP(std::cout << "    predicting\n";)
	lit<C, T> parl = get_lit(i);
	DBG(assert(parl.nt()));
	DBG(assert(!completed(i)));
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
			if (c==0 && parl.nt()) cache[{parl.n(), i.set}].insert(i);
			item j(i.set, p, c, i.set, 0);
			MC(count(cnt.predict_inserts);)
			if (add(t, j).second) {
				DBGP(print(std::cout <<" +  adding to t \t\t\t",
					j) << "\n";)
				// if item is added, then it is a new item
				//if(S[i.set].find(j) != S[i.set].end()) {
					// j item is already present
				//}
			}
			if (po.enable_gc) {
				MC(count(cnt.refi_increments);)
				++refi[i];
				MC(maks(cnt.refi_size_peak, refi.size());)
			}
		}
	}
}
template <typename C, typename T>
void parser<C, T>::scan(const item& i, size_t n, T ch) {
	MC(count(cnt.scan_calls);)
	DBGP(std::cout << "    scanning character: '"<<to_std_string(ch)<<"' ";)
	item j(n + (ch == static_cast<T>(0) ? 0 : 1),
		i.prod, i.con, i.from, i.dot + 1);
	DBGP(std::cout << (ch == get_lit(i).t()?"SUCCESS":"FAIL") << "\n";)
	if (ch != get_lit(i).t()) {
		if(po.enable_gc) {
			//when the item fails, decrement refcount of items that
			//predicted it i.e predicting items ( only the one) for
			// which refc was incremented when this item was predicted
			auto rit = refi.find(i);
			if (rit != refi.end()) {
				MC(count(cnt.refi_decrements);)
				if (--rit->second == 0) {
					MC(count(cnt.refi_zero_hits);)
					refi.erase(rit);
				}
			}
			remove_item(i);
		}
		return;
	}
	// by this time, terminal is advanced over
	// and new item j will be created.
	// hence, the previous item i can be marked
	// for gc
	if (!o.binarize && po.enable_gc) gcready.insert(i);
	if (j.set >= S.size()) S.resize(j.set + 1);
	DBGP(print(std::cout << " +  adding from scan into S[" << j.set <<
		"]: \t", j) << std::endl;)
	S[j.set].insert(j);
	if (need_fromS) {
		MC(count(cnt.fromS_writes);)
		fromS[j.from].insert(j.set);
	}
	DBGP(print(std::cout << "Add Edge: ", i) << " --> ";)
	DBGP(print(std::cout, j) << std::endl;)
	//++refi[i];
}
template <typename C, typename T>
void parser<C, T>::scan_cc_function(const item& i, size_t n, T ch,
	container_t& c)
{
	MC(count(cnt.scan_cc_calls);)
	DBGP(std::cout << "    scanning cc function for char: `"
		<< to_std_string(ch) << "`[" << (int_t) ch << "]" << std::endl;)
	size_t p = 0; // character's prod rule
	DBG(assert(!completed(i)));
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
	if( l.nt()) cache[{l.n(), i.set}].insert(i);
	item k(n, p, 0, n - (eof ? 0 : 1), 1); // complete char functions's char
	DBGP(print(std::cout << " +  adding from cc scan into S[" << k.set <<
		"] \t", k) << "\n";)
	if (S.size() <= k.set) S.resize(k.set + 1);
	S[k.set].insert(k);
	if (need_fromS) {
		MC(count(cnt.fromS_writes);)
		fromS[k.from].insert(k.set);
	}
	DBGP(print(std::cout << "Add Edge: ", i) << " --> ";)
	DBGP(print(std::cout, k) << "\n";)
	//++refi[i];
	// i is scanned over and item k is completed so collectible.
	if (!o.binarize && po.enable_gc ) gcready.insert(i);
	//gcready.insert(k);
	if (eof_fn) c.insert(k); // add current item for completion if eof_fn
}

template <typename C, typename T>
parser<C, T>::result parser<C, T>::parse(const C* data, size_t size,
	parse_options popts)
{
	po = popts;
	return parse(data, size);
}
template <typename C, typename T>
parser<C, T>::result parser<C, T>::parse(const C* data, size_t size) {
	in_ = std::make_unique<input>(data, size,
		po.max_length, o.chars_to_terminals, po.eof);
	return _parse();
}
template <typename C, typename T>
parser<C, T>::result parser<C, T>::parse(std::basic_istream<C>& is,
	parse_options popts)
{
	po = popts;
	return parse(is);
}
template <typename C, typename T>
parser<C, T>::result parser<C, T>::parse(std::basic_istream<C>& is) {
	in_ = std::make_unique<input>(is,
		po.max_length, o.chars_to_terminals, po.eof);
	return _parse();
}
template <typename C, typename T>
parser<C, T>::result  parser<C, T>::parse(const std::string& fn,
	parse_options popts)
{
	po = popts;
	return parse(fn);
}
template <typename C, typename T>
parser<C, T>::result  parser<C, T>::parse(const std::string& fn) {
	in_ = std::make_unique<input>(fn,
		po.max_length, o.chars_to_terminals, po.eof, &report_);
	return _parse();
}
#ifdef _WIN32
template <typename C, typename T>
parser<C, T>::result parser<C, T>::parse(const std::wstring& fn,
	parse_options popts)
{
	po = popts;
	return parse(fn);
}
template <typename C, typename T>
parser<C, T>::result parser<C, T>::parse(const std::wstring& fn) {
	in_ = std::make_unique<input>(fn,
		po.max_length, o.chars_to_terminals, po.eof, &report_);
	return _parse();
}
#else
template <typename C, typename T>
parser<C, T>::result parser<C, T>::parse(int fd, parse_options popts) {
	po = popts;
	return parse(fd);
}
template <typename C, typename T>
parser<C, T>::result parser<C, T>::parse(int fd) {
	in_ = std::make_unique<input>(fd,
		po.max_length, o.chars_to_terminals, po.eof);
	return _parse();
}
#endif
template <typename C, typename T>
parser<C, T>::result parser<C, T>::_parse() {
	// Fresh report per parse.
	report_.clear();
	std::optional<idni::diagnostics::report::scope_guard> parse_scope;
	if (po.measure_scopes)
		parse_scope.emplace(report_.open(label::parse));
	else
		report_.reset(label::parse);
#ifdef TAU_PARSER_MEASURE_COUNTERS
	cnt = idni::parser_strings::counters{};
#endif
	debug = po.debug;
	if (debug) {
		std::basic_string<C> tmp = in_->get_string();
		std::cout << "parse: `" << to_std_string(tmp) << "`"
			<< "`[" << tmp.size() << "] start sym:" << g.get_start()
			<< "(" << g.get_start().nt() << ")" << "\n";
		g.print_internal_grammar(std::cout << "grammar: \n", "\t", true)
			<< "\n";
	}
	// In forest_path the forest is built eagerly; in bintree_path
	// the tref (stored in `fr` below) is built instead.
	std::unique_ptr<pforest> f;
	if (po.tree_path == parse_tree_path::forest_path)
		f = std::make_unique<pforest>();
#ifdef TAU_PARSER_MEASURE_COUNTERS
	auto flush_parsing_counters = [&]() {
		flush_counters(report_, cnt);
		report_.kb(label::rss_after,
			measures::current_rss_kb());
		report_.kb(label::peak_rss_after,
			measures::peak_rss_kb());
		report_.count(label::input_length, in_->tpos());
		report_.count(label::position_count, in_->tpos());
	};
#endif

	lit<C, T> start_lit;
	auto run_earley = [&]() {
	size_t n = 0;
	// fromS is only read by GC drain and conjunctive cascade machinery.
	// Skip every write for non-conjunctive grammars with GC off.
	need_fromS = po.enable_gc || any_conj;
	S.clear(), U.clear(), fromS.clear(), bin_tnt.clear(), refi.clear(),
		cache.clear(), gcready.clear(), sorted_citem.clear(),
		rsorted_citem.clear(), completion_deps.clear(),
		completion_count.clear(), complete_memo.clear();
	//pnode::nid().clear();
	tid = 0;
	S.resize(1);
	start_lit = po.start != SIZE_MAX ? g.nt(po.start)
						: g.start_literal();
	container_t t, c;
	for (size_t p : g.prod_ids_of_literal(start_lit))
		for (size_t c = 0; c != g.n_conjs(p); ++c) {
			auto [it, _] = add(t, { 0, p, c, 0, 0 });
			if (po.enable_gc) ++refi[*it];
		}
	size_t r = 1, cb = 0; // row and cel beginning
	size_t proc = 0;
	T ch = 0;
	size_t cn = -1;
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
		}

		MC(size_t pos_iters = 0;)
		do {
			MC(count(cnt.inner_loop_iterations);)
			MC(count(pos_iters);)
			MC(maks(cnt.t_size_peak, t.size());)
			size_t lproc = 0;
			//DBGP(print(std::cout << "t:\n", t);)
			for (const item& x : t) {
				S[x.set].insert(x);
				if (need_fromS) {
					MC(count(cnt.fromS_writes);)
					fromS[x.from].insert(x.set);
				}
			}
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
				resolve_conjunctions(c);
				//DBGP(print(std::cout << "c after resolve:\n", c) << std::endl;)
				for (const auto& x : c) {
					DBGP(print(std::cout << "complete after resolve\t\t", x) << "\n";)
					complete(x, t, t, true);
				}
				c.clear();
			}
			//DBGP(if (!t.empty()) print(std::cout << "t not empty:\n", t) << "\n";)
			} while (!t.empty());
			MC(maks(cnt.inner_loop_iterations_max, pos_iters);)
			MC(maks(cnt.s_max_per_pos, S[n].size());)

		if (po.measure_each_pos && new_pos) {
			std::cout << in_->pos() << " \tln: " << r << " col: "
				<< (n - cb + 1) << " :: ";
		}

		if (po.tree_path == parse_tree_path::forest_path
			&& o.incr_gen_forest)
		{
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
		if (po.enable_gc) {
			for (auto it = gcready.begin(); it != gcready.end();) {
				auto rm = *it;
				if ((rm.set + po.gc_lag) <= n) {
					bool can_remove = true;
					if (auto rit = refi.find(rm); rit != refi.end()) {
						if (rit->second > 0) {
							can_remove = false;
							MC(count(cnt.gcready_skipped_refcounted);)
						} else refi.erase(rit);
					}
					if (!can_remove) { ++it; continue; }
					auto its = S[rm.set].find(rm);
					if ( its != S[rm.set].end() ){
						S[rm.set].erase(its);
						if (!completed(rm) && get_lit(rm).nt()) {
							auto cit = cache.find(
								{get_lit(rm).n(), rm.set});
							if (cit != cache.end())
								cit->second.erase(rm);
						}

						// also clean from fromS
						if (auto fit = fromS.find(rm.from);
							fit != fromS.end() ) {
								fit->second.erase(rm.set);
								if (fit->second.empty())
									fromS.erase(fit);
						}

						MC(count(cnt.gc_collected);)
					}
					it = gcready.erase(it);
				}
				else it++;
			}
		}
		//DBGP(print_S(std::cout << "\n") << "\n";)

	} while (in_->tnext());
	};

	{
		auto _ = report_.open_if(po.measure_scopes, label::parsing);
		#ifdef TAU_PARSER_MEASURE_COUNTERS
		if (po.measure_counters) {
			report_.kb(label::rss_before,
				measures::current_rss_kb());
			report_.kb(label::peak_rss_before,
				measures::peak_rss_kb());
		}
		#endif
		run_earley();
		MC(if (po.measure_counters) flush_parsing_counters();)
	}

	in_->clear();
	MC({
		size_t s_remaining = 0;
		for (size_t i = 0; i < S.size(); i++) s_remaining += S[i].size();
		count(cnt.s_total_remaining, s_remaining);
		count(cnt.gcready_size_final, gcready.size());
		count(cnt.refi_size_final, refi.size());
	})

	tref fr = 0;
	if (po.tree_path == parse_tree_path::bintree_path) {
		// bintree is always built post-parse; incr_gen_forest is ignored.
		fr = build_bintree(start_lit, po);
	} else {
		if (!o.incr_gen_forest) init_forest(*f, start_lit, po);
		else f->root(pnode(start_lit, { 0, in_->tpos() }));
	}

	if (debug) debug = false;

	bool fnd = found(po.start);
	error err = fnd ? error{} : get_error();

	if (f) {
		DBGP(
		MC({
			auto usen = f->count_useful_nodes(f->root());
			auto usenc = usen.first + usen.second;
			const size_t total = cnt.s_total_remaining
				+ cnt.gc_collected;
			std::cout << "\nGC: Useful nodes"
				<< usen.first << "+" << usen.second
				<< "=" << usenc << "\n";
			if (total)
				std::cout << "\nGC: useful% = "
					<< 100*usenc / total << std::endl;
			if (total > usenc)
				std::cout << "\nGC: achieved% = "
					<< 100*cnt.gc_collected
						/ (total - usenc) << std::endl;
			if (total)
				std::cout << "\nGC: potential% = "
					<< 100*(total - usenc) / total
					<< std::endl;
		})
		)
	}

	parse_scope.reset();
	if (po.tree_path == parse_tree_path::bintree_path) {
		result r(*this, std::move(in_), fr, fnd, err);
		po = o.parse_opts;
		return r;
	}
	result r(*this, std::move(in_), std::move(f), fnd, err);
	po = o.parse_opts;
	return r;
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
bool parser<C, T>::error::at_eof() const {
	if (unexp.size() == 0) return false;
	for (const auto& t : unexp)
		if (t.nt() || t.is_null()
			|| (t.t() != static_cast<T>(0)
				&& t.t() != static_cast<T>(-1)))
			return false;
	return true;
}
//------------------------------------------------------------------------------
template <typename C, typename T>
std::string parser<C, T>::error::to_str(info_lvl elvl, size_t line_start) const{
	if (ctxt.size() == 0) return "";
	std::stringstream ss;
	bool eof = unexp.size() != 0;
	for (const auto& t : unexp) {
		if (t.nt() || t.is_null() || (t.t() != static_cast<T>(0)
						&& t.t() != static_cast<T>(-1)))
		{
			eof = false;
			std::string s = t.to_std_string();
			if (s.size() >= 2) ss << s.substr(1, s.size() - 2);
		}
	}
	std::string s{ ss.str() }; ss = {};
	if (!eof && s.size()) {
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
	auto& in = *in_;
	auto near_ctxt = [&in](int_t from, int_t pos) {
		std::vector<T> errctxt;
		// using from as delimiter..
		while (pos >= from)
			errctxt.push_back(in.tat(pos--));
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
	in.clear();
	//DBGP(std::cout << "tpos: " << in.tpos() << "\n";)
	//DBGP(print_S(std::cout << "S:\n") << "\n";)
	for (int_t i = (int_t) in.tpos(); i >= 0; i--) if (S[i].size()) {
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
						err.unexp.emplace_back(in.tat(i));
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
			err.unexp = lits<C, T>{ { in.tat(i) } }, err.loc = i;

		break;
	}
	err.line = 1;
	size_t line_loc = 0;
	for (int_t j = 0; err.loc > -1 && j != err.loc; ++j)
		if (in.tat(j) == (T)'\n') err.line++, line_loc = j;
	err.col = err.loc - line_loc + 1;
	err.ctxt = near_ctxt(line_loc, err.loc + 1);
	return err;
}
template <typename C, typename T>
int parser<C, T>::do_preprocess() {
#if defined(TAU_PARSER_MEASURE_SCOPES) \
	|| defined(TAU_PARSER_MEASURE_COUNTERS)
	int count = 0;
	auto run = [&]() {
		for (size_t n = 0; n < in_->tpos() + 1; n++)
			for (const item& i : S[n]) {
				if (po.measure_counters) ++count;
				pre_process(i);
			}
	};
	report_.step(po.measure_scopes && po.measure_preprocess,
		label::preprocess, [&] { run(); });
	return count;
#else
	for (size_t n = 0; n < in_->tpos() + 1; n++)
		for (const item& i : S[n]) pre_process(i);
	return 0;
#endif
}

template <typename C, typename T>
void parser<C, T>::pre_process(const item& i) {
	//DBGP(print(std::cout << " *  preprocessing\t\t\t", i) << std::endl;)
	//sorted_citem[G[i.prod][0].n()][i.from].emplace_back(i);
	if (completed(i))
		sorted_citem[{ g(i.prod).n(), i.from }].emplace_back(i),
		rsorted_citem[{ g(i.prod).n(), i.set }].emplace_back(i);
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
			sorted_citem[{ l.n(), i.from }].emplace_back(i);
			rsorted_citem[{ l.n(), i.set }].emplace_back(i);
		}
	}
}

// build_bintree: preprocesses S items and builds a tref directly from them.
// When auto_disambiguate is on, picks the parse with smallest production id
// and within it the alternative with smallest spans. When off, wraps
// alternatives under __AMB__ nodes.
template <typename C, typename T>
tref parser<C, T>::build_bintree(const lit<C, T>& start_lit,
	[[maybe_unused]] const parse_options& po)
{
	bin_tnt.clear();
	sorted_citem.clear();
	rsorted_citem.clear();
	tid = 0;
	pnode root(start_lit, { 0, in_->tpos() });

	// preprocess parser items for faster retrieval
	int preprocess_count = do_preprocess();
	if (po.debug) report_.info("preprocess size", preprocess_count);

	auto check_allowed = [this](const pnode& n) {
		if (!g.opt.auto_disambiguate) return false;
		for (auto& nt : g.opt.nodisambig_list)
			if (n.first.nt() && n.first.n() == nt) return false;
		return true;
	};

	auto pick_best = [](pnodes_set& packs) -> pnodes {
		if (packs.size() <= 1)
			return packs.empty() ? pnodes{} : *packs.begin();
		int maxk = INT_MAX;
		for (auto& p : packs)
			if ((int)p.size() < maxk) maxk = (int)p.size();
		std::vector<int> idxs;
		for (size_t i = 0; i < packs.size(); i++)
			idxs.push_back((int)i);
		int k = 0;
		while (k < maxk && idxs.size() > 1) {
			int gspan = INT_MAX;
			std::vector<int> gi;
			for (auto idx : idxs) {
				auto& pk = *std::next(packs.begin(), idx);
				int span = (int)(pk[k]->second[1]
						- pk[k]->second[0]);
				if (span < gspan) {
					gspan = span; gi.clear();
					gi.push_back(idx);
				} else if (span == gspan)
					gi.push_back(idx);
			}
			idxs = std::move(gi);
			k++;
		}
		return *std::next(packs.begin(), idxs[0]);
	};

	auto is_ebnf = [](const pnode& n) -> bool {
		auto s = n.first.to_std_string();
		return s.size() >= 4 && s.substr(0, 4) == "__E_";
	};

	auto is_amb = [](tref n) {
		if (!n) return false;
		auto& l = tree::get(n).value.first;
		return l.nt() && l.to_std_string() == "__AMB__";
	};

	std::set<pnode> visiting;
	std::function<tref(const pnode&)> build;
	std::function<tref(const pnode&, const pnodes&)> build_pack;

	auto build_children = [&](const pnodes& pack, trefs& out) {
		for (auto& nxt : pack) {
			if (is_ebnf(nxt)) {
				auto ch = build(nxt);
				if (ch && is_amb(ch)) out.push_back(ch);
				else if (ch) for (auto c
					: tree::get(ch).children())
					out.push_back(c);
			} else if (auto ch = build(nxt); ch)
				out.push_back(ch);
		}
	};

	build_pack = [&](const pnode& node, const pnodes& pack) {
		trefs children;
		build_children(pack, children);
		return tree::get(node, children);
	};

	build = [&](const pnode& node) -> tref {
		if (!node.first.nt()) return tree::get(node);
		if (visiting.count(node)) return tree::get(node);
		visiting.insert(node);

		auto& items = sorted_citem[{ node.first.n(),
						node.second[0] }];
		pnodes_set packs;
		bool ad_allowed = check_allowed(node);

		if (ad_allowed) {
			size_t best_prod = SIZE_MAX;
			for (auto& cur : items) {
				if (cur.set != node.second[1]) continue;
				if (cur.prod >= best_prod) continue;
				pnodes nxtlits;
				pnodes_set cur_packs;
				if (o.binarize)
					binarize_comb(cur, cur_packs);
				else
					sbl_chd_forest(cur, nxtlits,
						cur.from, cur_packs);
				if (!cur_packs.empty()) {
					best_prod = cur.prod;
					packs = std::move(cur_packs);
				}
			}
		} else {
			for (auto& cur : items) {
				if (cur.set != node.second[1]) continue;
				pnodes nxtlits;
				pnodes_set cur_packs;
				if (o.binarize)
					binarize_comb(cur, cur_packs);
				else
					sbl_chd_forest(cur, nxtlits,
						cur.from, cur_packs);
				for (auto& p : cur_packs)
					packs.insert(std::move(p));
			}
		}

		tref r;
		if (packs.empty()) {
			r = tree::get(node);
		} else if (packs.size() == 1) {
			r = build_pack(node, *packs.begin());
		} else if (ad_allowed) {
			r = build_pack(node, pick_best(packs));
		} else {
			auto amb = g.nt(from_str<C>(
				std::string("__AMB__")));
			trefs alts;
			for (auto& pack : packs)
				if (auto alt = build_pack(node,
						pack); alt)
					alts.push_back(alt);
			r = tree::get(pnode(amb, node.second), alts);
		}

		visiting.erase(node);
		return r;
	};

	tref ret = report_.step(po.measure_scopes && po.measure_forest,
		label::build_bintree, [&] { return build(root); });

	return ret;
}

// default-mode init_forest: builds a pforest eagerly from S-items
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
	int preprocess_count = do_preprocess();
	if (po.debug) {
		report_.info("preprocess size", preprocess_count);
		report_.info("sorted_citem size", sorted_citem.size());
		report_.info("rsorted_citem size", rsorted_citem.size());
	}

	ret = report_.step(po.measure_scopes && po.measure_forest,
		label::build_forest, [&] { return build_forest(f, root); });
	// f.print_data(std::cout) << "\n";

	return ret;
}
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
		if (curchd.back()->second[1] == eitem.set) ambset.insert(curchd);
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
		for (auto& it : rightit) {
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
		for (auto& it : leftit) for (auto& rit : rcomb) {
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
			for (auto& it : leftit) for (auto& rit : rcomb) {
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
			if (eitem.from == rit.second[0]) ambset.insert({ rit });
	}
	return true;
}
// default-mode build_forest — builds a pforest from a root pnode
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

	for (auto& cur : nxtset) {
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
		int k = 0;
		std::vector<int> idxs;
		for(size_t i = 0; i < ambset.size(); i++) idxs.push_back(i);
		// smallest node count in ambset
		int maxk = INT_MAX;
		for(auto& pack : ambset)
			if ((int)pack.size() < maxk) maxk = pack.size();


		//choose the one with the first smallest span from upto size of
		// smallest set in ambset
		do {
			gi.clear();
			int gspan = INT_MAX;
			for (auto packidx : idxs) {
				auto apack = *next(ambset.begin(), packidx);
				pnode lt = apack[k];
				int span = lt.second[1] - lt.second[0];
				if (gspan == span) gi.push_back(packidx);
				if (gspan > span) gspan = span,
						gi.clear(), gi.push_back(packidx);
			}
			idxs.clear();
			idxs.insert(idxs.begin(),gi.begin(),gi.end());
		}
		while(++k < maxk && gi.size() > 1);

		cambset.clear();
		if (ambset.size())
			cambset.insert(
				gi.size()
					? *next(ambset.begin(), gi[0])
					: *ambset.begin());

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
	// No decoder: Only valid when the parser's char_type and terminal_type match
	if constexpr (std::is_same_v<C, T>) {
		if (!isstream()) return std::basic_string<T>(d + start,
								end - start);
		std::basic_stringstream<T> ss;
		s.seekg(start);
		while (end > start++) ss << s.get();
		return clear(), ss.str();
	} else {
		return std::basic_string<T>{};
	}
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
