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

#include "tree.h"

namespace idni {

//------------------------------------------------------------------------------

template <typename T>
tref_range<T>::tref_range(tref first_) : first(first_) {}

template <typename T>
tref_range<T>::iterator::iterator(tref node) : current(node) {}

template <typename T>
tref tref_range<T>::iterator::operator*() const {
	return current;
}

template <typename T>
typename tref_range<T>::iterator& tref_range<T>::iterator::operator++() {
	if (current != nullptr) current = bintree<T>::get(current).r;
	return *this;
}

template <typename T>
bool tref_range<T>::iterator::operator!=(const iterator& other) const {
	return current != other.current;
}

template <typename T>
bool tref_range<T>::iterator::operator==(const iterator& other) const {
	return current == other.current;
}

template <typename T>
typename tref_range<T>::iterator tref_range<T>::begin() const {
	return iterator(first);
}

template <typename T>
typename tref_range<T>::iterator tref_range<T>::end() const {
	return iterator(nullptr);
}

template <typename T>
tree_range<T>::tree_range(tref first_) : first(first_) {}

template <typename T>
tree_range<T>::iterator::iterator(tref node) : current(node) {}

template <typename T>
const T& tree_range<T>::iterator::operator*() const {
	return T::get(current);
}

template <typename T>
typename tree_range<T>::iterator& tree_range<T>::iterator::operator++() {
	if (current != nullptr)
		current = bintree<T>::get(current).r;
	return *this;
}

template <typename T>
bool tree_range<T>::iterator::operator!=(const iterator& other) const {
	return current != other.current;
}

template <typename T>
bool tree_range<T>::iterator::operator==(const iterator& other) const {
	return current == other.current;
}

template <typename T>
typename tree_range<T>::iterator tree_range<T>::begin() const {
	return iterator(first);
}

template <typename T>
typename tree_range<T>::iterator tree_range<T>::end() const {
	return iterator(nullptr);
}

//------------------------------------------------------------------------------

inline const htree::sp& htree::null() {
	static const sp hnull(new htree());
	return hnull;
}
inline tref htree::get() const { return hnd; }
inline bool htree::operator==(const htree& r) const { return hnd == r.hnd; }
inline bool htree::operator< (const htree& r) const { return hnd <  r.hnd; }
//inline htree::~htree() { if (hnd != NULL) M.erase(hnd); }
inline htree::htree(tref h) : hnd(h) { }

//------------------------------------------------------------------------------

template <typename T>
tref bintree<T>::get() const { return reinterpret_cast<tref>(this); }

template <typename T>
const htree::sp bintree<T>::geth(tref h) {
	//DBG(assert(h != NULL);)
	if (h == NULL) return htree::null();
	auto res = M.find(*reinterpret_cast<const bintree*>(h)); //done with one search
	htree::sp ret;
	if (res != M.end()) res->second = ret = htree::sp(new htree(h));
	DBG(assert(!res->second.expired());)
	return res->second.lock();
}

template <typename T>
const bintree<T>& bintree<T>::get(const tref id) {
	//std::cout<< id <<"\n";
	DBG(assert(id != NULL && sizeof(bintree<T>*) == sizeof(tref));)
	return *(bintree<T>*)id;
}

template <typename T>
const bintree<T>& bintree<T>::get(const htree::sp& h) {
	return get(h->get());
}

template <typename T>
tref bintree<T>::get(const T& v, tref l, tref r) {
	bintree bn(v, l, r);
	auto res = bintree<T>::M.emplace(bn, htree::wp());
	return reinterpret_cast<tref>(std::addressof(res.first->first));
}

template <typename T>
tref bintree<T>::replace_value(const T& v) const { return get(v, l, r); }

template <typename T>
void bintree<T>::dump() {
	std::cout << "-----\n";
	std::cout << "MB:" << M.size() << "\n";
	for (auto& x : M) {
		std::cout << x.first.str() << " " << x.second.lock() << " "
			<< x.second.use_count() << "\n";
	}
	std::cout << "-----\n";
}

template <typename T>
void bintree<T>::gc() {
	std::unordered_set<tref> keep{};
	gc(keep);
}

template <typename T>
void bintree<T>::gc(std::unordered_set<tref>& keep) {

	// DBG(dump();)
	//DBG(htree::dump();)

	//mark all
	std::unordered_set<tref> next;
	auto mark = [&next](tref r, auto& mark) -> void {
		if (!next.insert(r).second) return;
		auto& bn = get(r);
		if (bn.r != NULL) mark(bn.r, mark);
		if (bn.l != NULL) mark(bn.l, mark);
	};
	//check if any handle expired and needs to be garbaged
	// otherwise mark all to be retained live

	for (auto& x : M)
		if (!x.second.expired()) mark(x.second.lock()->get(), mark);
	for (auto& x : keep) mark(x, mark);

	// do not run gc, if no handle expired or empty.
	if (!M.size() || (next.size() == M.size())) {
		DBG(std::cout << "gc-do nothing:" << next.size() << "\n";)
		return;
	}

	/*
	DBG(assert(next.size() != M.size()));
	decltype(V) nv;
	std::unordered_map<int_t, int_t> shift;
	shift.emplace(-1,-1); // just for null
	//calculate shifts to handle
	for (size_t np = 0, i = 0; i < V.size(); i++)
		if (next.count(i)) shift.emplace(i, np++);
	for( auto& x : shift)
		DBG(std::cout<<x.first<< " "<< x.second <<"\n");

	M.clear();
	//reconstruct new V and M with updated handles
	for (size_t i = 0; i < V.size(); i++)
		if (next.count(i)) {
			auto& bn = *V[i];
			bintree nbn(bn.value, shift[bn.l], shift[bn.r]);
			auto res = M.emplace(nbn, nv.size());
			nv.push_back(&(res.first->first));
		}

	V = std::move(nv); //gc from V

	//update htree handle as well
	decltype(htree::M) hm;
	for( auto& x : htree::M)
	if (!x.second.expired()) {
		auto psp = x.second.lock();
		DBG(assert(psp->hnd == x.first);)
		hm.emplace(psp->hnd = shift[x.first], psp);
	}
	htree::M = std::move(hm);
	*/
	//delete non-reachable garbage nodes from M
	for (auto it = M.begin(); it != M.end(); )
		if (next.count(it->first.get()) == 0)
			it = M.erase(it);
		else it++;

	// DBG(dump();)
	//DBG(htree::dump();)
}

template <typename T>
bool bintree<T>::operator<(const bintree<T>& o) const {
	if (value != o.value) return value < o.value;
	if (l != o.l) return l < o.l;
	return r < o.r;
}
template <typename T>
bool bintree<T>::operator==(const bintree<T>& o) const {
	return value == o.value && l == o.l && r == o.r;
}

template <typename T>
bintree<T>::bintree(const T& _value, tref _l, tref _r)
	: value(_value), l(_l), r(_r) { }

template <typename T>
std::ostream& bintree<T>::print(std::ostream& o, size_t s) const {
	for (size_t i = 0; i < s; i++)
		o << " " << str() << "\n";
	if (l != NULL) get(l).print(o, s + 1);
	if (r != NULL) get(r).print(o, s + 1);
	return o;
}

template <typename T>
const std::string bintree<T>::str() const {
	std::stringstream ss;
	ss << value << " { " << l << ", " << r << " } <- " << this;
	return ss.str();
}

/*template <typename T>
std::vector<const bintree<T>*> bintree<T>::V;
*/

template <typename T>
std::map<const bintree<T>, htree::wp> bintree<T>::M;

//------------------------------------------------------------------------------

template <typename T>
size_t hash_tref<T>::operator()(tref r) const {
	return std::hash<const lcrs_tree<T>&>(lcrs_tree<T>::get(r));
}

template <typename T>
bool subtree_equality<T>::operator()(tref a, tref b) const {
	return lcrs_tree<T>::subtree_less(a, b);
}

template <typename T, typename PT>
bool subtree_pair_equality<T, PT>::operator()(const std::pair<tref, PT>& a,
					const std::pair<tref, PT>& b) const
{
	bool r = lcrs_tree<T>::subtree_less(a.first, b.first);
	if constexpr (std::is_same_v<PT, tref>)
		r = r && lcrs_tree<T>::subtree_less(a.second, b.second);
	else r = r && a.second < b.second;
	return r;
}

template <typename T>
bool lcrs_tree<T>::operator<(const lcrs_tree<T>& o) const {
	if (this->value != o.value) return this->value < o.value;
	return this->l < o.l;
}
template <typename T>
bool lcrs_tree<T>::operator==(const lcrs_tree<T>& o) const {
	return this->value == o.value && this->l == o.l;
}

template <typename T>
bool lcrs_tree<T>::subtree_equals(tref a, tref b) {
	if (a == b) return true;
	if (a == nullptr || b == nullptr) return false;
	return lcrs_tree<T>::get(a) == lcrs_tree<T>::get(b);
}

template <typename T>
bool lcrs_tree<T>::subtree_less(tref a, tref b) {
	if (a == b) return false;
	if (a == nullptr) return true;
	if (b == nullptr) return false;
	return lcrs_tree<T>::get(a) < lcrs_tree<T>::get(b);
}

//------------------------------------------------------------------------------
// gc

template <typename T>
void lcrs_tree<T>::gc() {
	std::unordered_set<tref> keep{};
	gc(keep);
}

template <typename T>
void lcrs_tree<T>::gc(std::unordered_set<tref>& keep) {

	// DBG(dump();)
	//DBG(htree::dump();)

	//mark subtree
	std::unordered_set<tref> next;
	auto mark = [&next](tref r, auto& mark) -> void {
		if (!next.insert(r).second) return;
		if (bintree<T>::get(r).l != nullptr)
			mark(bintree<T>::get(r).l, mark);
	};
	//check if any handle expired and needs to be garbaged
	// otherwise mark all to be retained live

	for (auto& x : bintree<T>::M)
		if (!x.second.expired()) mark(x.second.lock()->get(), mark);
	for (auto& x : keep) mark(x, mark);

	// do not run gc, if no handle expired or empty.
	if (!bintree<T>::M.size() || (next.size() == bintree<T>::M.size())) {
		DBG(std::cout << "gc-do nothing:" << next.size() << "\n";)
		return;
	}
	//delete non-reachable garbage nodes from M
	for (auto it = bintree<T>::M.begin(); it != bintree<T>::M.end(); )
		if (next.count(it->first.get()) == 0)
			it = bintree<T>::M.erase(it);
		else it++;

	// DBG(dump();)
	//DBG(htree::dump();)
}

//------------------------------------------------------------------------------
// handles

template <typename T>
tref lcrs_tree<T>::get() const { return reinterpret_cast<tref>(this); }

template <typename T>
const lcrs_tree<T>& lcrs_tree<T>::get(const htree::sp& h) {
	return (const lcrs_tree<T>&) bintree<T>::get(h);
}

template <typename T>
const lcrs_tree<T>& lcrs_tree<T>::get(tref id) {
	DBG(assert(id != nullptr);)
	return (const lcrs_tree<T>&) bintree<T>::get(id);
}

template <typename T>
const htree::sp lcrs_tree<T>::geth(tref h) { return bintree<T>::geth(h); }

//------------------------------------------------------------------------------
// creation with tref childs

template <typename T>
tref lcrs_tree<T>::get_raw(const T& v, const tref* ch, size_t len, tref r) {
	tref pr = nullptr;
	for (size_t i = len; i > 0; ) --i,
		pr = bintree<T>::get(get(ch[i]).value, get(ch[i]).l, pr);
	return bintree<T>::get(v, pr, r);
}

template <typename T>
tref lcrs_tree<T>::get(const T& v, const tref* ch, size_t len, tref r) {
	if (hook == nullptr) return get_raw(v, ch, len, r);
	return hook(v, ch, len, r);
}

template <typename T>
tref lcrs_tree<T>::get(const T& v, const trefs& ch, tref r) {
	return get(v, ch.data(), ch.size(), r);
}

template <typename T>
tref lcrs_tree<T>::get(const T& v, const std::initializer_list<tref>& ch,
	tref r)
{
	return get(v, std::data(ch), ch.size(), r);
}

template <typename T>
tref lcrs_tree<T>::get(const T& v) {
	return get(v, (const tref*) nullptr, 0, nullptr);
}

template <typename T>
tref lcrs_tree<T>::get(const T& v, tref child) {
	return get(v, &child, 1);
}

template <typename T>
tref lcrs_tree<T>::get(const T& v, tref ch1, tref ch2) {
	return get(v, { ch1, ch2 });
}

template <typename T>
tref lcrs_tree<T>::get(tref n, tref r) {
	if (get(n).r == r) return n;
	return bintree<T>::get(get(n).value, get(n).l, r);
}

//------------------------------------------------------------------------------
// creation with node childs

template <typename T>
tref lcrs_tree<T>::get(const T& v, const T* ch, size_t len, tref r) {
	// because of possible hook, this has to go through get with trefs
	trefs nch;
	for (size_t i = 0; i < len; ++i) nch.push_back(get(ch[i]));
	return get(v, std::data(nch), len, r);
}

template <typename T>
tref lcrs_tree<T>::get(const T& v, const std::vector<T>& children, tref r) {
	return get(v, children.data(), children.size(), r);
}

template <typename T>
tref lcrs_tree<T>::get(const T& v, const std::initializer_list<T>& ch, tref r) {
	return get(v, std::data(ch), ch.size(), r);
}

template <typename T>
tref lcrs_tree<T>::get(const T& v, const T& child) {
	return get(v, get(child));
}

template <typename T>
tref lcrs_tree<T>::get(const T& v, const T& ch1, const T& ch2) {
	return get(v, { ch1, ch2 });
}

template <typename T>
bool lcrs_tree<T>::has_right_sibling() const { return this->r != nullptr; }

template <typename T>
bool lcrs_tree<T>::has_child() const { return this->l != nullptr; }

template <typename T>
tref lcrs_tree<T>::right_sibling() const { return this->r; }

template <typename T>
const lcrs_tree<T>& lcrs_tree<T>::right_sibling_tree() const {
	return get(this->r);
}

template <typename T>
tref lcrs_tree<T>::left_child() const { return this->l; }

template <typename T>
size_t lcrs_tree<T>::children_size() const {
	size_t len = 0;
	get_kary_children(nullptr, len);
	return len;
}

//fills caller allocated child array upto len size
//with child ids. If child is nullptr, then just
//calculates number of children in len.
//caller's responsibility to free allocated memory
template <typename T>
bool lcrs_tree<T>::get_kary_children(tref* child, size_t& len) const {
	/*
	htree::sp curp = htree::get(this->l);
	// having a vector here misses the whole point. got to be
	// something like get_next_child, or, get_nchilds and then
	// the user provides an array (ptr alloced with alloca()) to
	// fill all childs.
	// that's problem #1. problem #2 is that we shouldn't return
	// handles, only ids. we definitely dont want to create handles
	// for all trees in the system.
	std::vector<htree::sp> childs;
	while (curp != htree::null())
		childs.push_back(curp), curp = htree::get(get(curp).r);
	return childs;

	//resp: now no vector, and only ids returned in user defined array
	*/
	// DBG(std::cout<<"\n----");
	if (len > 0 && child == nullptr) return len = 0, false;
	len = 0;
	tref cur = this->l;
	while (cur != NULL) {
		//DBG(std::cout<<"\n"<<bintree<T>::get(cur).str()<<"\n");
		if (child != nullptr) child[len] = cur;
		++len, cur = bintree<T>::get(cur).r;
	}
	// DBG(std::cout<<"\n----");
	return true;
}

template <typename T>
bool lcrs_tree<T>::get_children(tref* ch, size_t& len) const {
	return get_kary_children(ch, len);
}

template <typename T>
trefs lcrs_tree<T>::get_children() const {
	trefs ch;
	for (tref c : children()) ch.push_back(c);
	return ch;
}

template <typename T>
tref lcrs_tree<T>::child(size_t n) const {
	size_t i = 0;
	for (tref c : children()) {
		if (i > n) break;
		if (i == n) return c;
		++i;
	}
	return nullptr;
}

template <typename T>
tref_range<T> lcrs_tree<T>::children() const { return tref_range<T>(this->l); }

template <typename T>
const lcrs_tree<T>& lcrs_tree<T>::operator[](size_t n) const {
	return child_tree(n);
}

template <typename T>
tref lcrs_tree<T>::first() const { return this->l; }

template <typename T>
tref lcrs_tree<T>::second() const { return child(1); }

template <typename T>
tref lcrs_tree<T>::third() const { return child(2); }

template <typename T>
tref lcrs_tree<T>::only_child() const {
	tref r = nullptr;
	for (tref c : children())
		if (c == nullptr || r != nullptr) return nullptr;
		else r = c;
	return r;
}

template <typename T>
const lcrs_tree<T>& lcrs_tree<T>::child_tree(size_t n) const {
	return lcrs_tree<T>::get(child(n));
}

template <typename T>
tree_range<lcrs_tree<T>> lcrs_tree<T>::children_trees() const {
	return tree_range<lcrs_tree<T>>(this->l);
}

template <typename T>
const lcrs_tree<T>& lcrs_tree<T>::first_tree() const {
	return lcrs_tree<T>::get(first());
}

template <typename T>
const lcrs_tree<T>& lcrs_tree<T>::second_tree() const {
	return lcrs_tree<T>::get(second());
}

template <typename T>
const lcrs_tree<T>& lcrs_tree<T>::third_tree() const {
	return lcrs_tree<T>::get(third());
}

template <typename T>
const lcrs_tree<T>& lcrs_tree<T>::only_child_tree() const {
	return lcrs_tree<T>::get(only_child());
}

template <typename T>
std::ostream& lcrs_tree<T>::print(std::ostream& os, size_t s) const {
	for (size_t i = 0; i < s; i++) os << "\t";
	os << this->value;
	// DBG(os << " *" << get();)
	os << "\n";
	for (const auto& c : children()) get(c).print(os, s + 1);
	return os;
}

template <typename T>
std::string lcrs_tree<T>::print_to_str(size_t s) const {
	std::stringstream ss;
	return print(ss, s), ss.str();
}

template <typename T>
std::ostream& lcrs_tree<T>::print_in_line(std::ostream& os,
	std::string open, std::string close, std::string sep) const
{

// #define DEBUG_PRINT_IN_LINE

	auto on_enter = [&os, open](tref n, [[maybe_unused]] tref parent) {
#ifdef DEBUG_PRINT_IN_LINE
		auto nt = lcrs_tree<T>::get(n).value;
		auto pnt = parent ? lcrs_tree<T>::get(parent).value : T{};
		std::cerr << "[" << nt << " " << pnt << "]";
		os << "<" << get(n).value << ">";
#else // DEBUG_PRINT_IN_LINE

		os << get(n).value;
		if (bool print_lcrs_pointers = false;
			print_lcrs_pointers)
		{
			const auto& t = lcrs_tree<T>::get(n);
			os << " [" << n ;
			if (t.has_child())         os << " __ " << t.left_child();
			if (t.has_right_sibling()) os << " >> " << t.right_sibling() << "] ";
			os << "] ";
		}

#endif // DEBUG_PRINT_IN_LINE

		if (get(n).has_child()) os << open;
	};
	auto on_leave = [&os, close](tref n, [[maybe_unused]] tref parent) {
#ifdef DEBUG_PRINT_IN_LINE
		auto nt = lcrs_tree<T>::get(n).value;
		auto pnt = parent ? lcrs_tree<T>::get(parent).value : T{};
		std::cerr << "\n\t[/" << nt << " " << pnt << "/]";
#endif // DEBUG_PRINT_IN_LINE

		if (get(n).has_child()) os << close;
	};
	auto on_between = [&os, sep](
		[[maybe_unused]] tref n, [[maybe_unused]] tref parent) {
#ifdef DEBUG_PRINT_IN_LINE
		auto nt = lcrs_tree<T>::get(n).value;
		auto pnt = parent ? lcrs_tree<T>::get(parent).value : T{};
		std::cerr << " [|" << nt << " " << pnt << "|]";
		os << "<" << sep << ">";
#else // DEBUG_PRINT_IN_LINE

		os << sep;

#endif // DEBUG_PRINT_IN_LINE
	};
	pre_order<T>(get()).visit(on_enter, all, on_leave, on_between);
	return os;
}

template <typename T>
std::string lcrs_tree<T>::print_in_line_to_str(
	std::string open, std::string close, std::string sep) const
{
	std::stringstream ss;
	return print_in_line(ss, open, close, sep), ss.str();
}

template <typename T>
std::ostream& lcrs_tree<T>::dump(std::ostream& os, tref n, bool subtree) {
	const auto& t = get(n);
	os << t.value << " [" << n ;
	if (t.has_child())         os << " __ " << t.left_child();
	if (t.has_right_sibling()) os << " >> " << t.right_sibling();
	os << "] ";
	if (subtree) t.print_in_line(os);
	return os;
}


template <typename T>
std::string lcrs_tree<T>::dump_to_str(tref n, bool subtree) {
	std::stringstream ss;
	return dump(ss, n, subtree), ss.str();
}

template <typename T>
std::ostream& lcrs_tree<T>::dump(std::ostream& os, bool subtree) const {
	return dump(os, get(), subtree);
}

template <typename T>
std::string lcrs_tree<T>::dump_to_str(bool subtree) const {
	return dump_to_str(get(), subtree);
}

template <typename T>
const lcrs_tree<T>& lcrs_tree<T>::dump(bool subtree) const {
	return dump(std::cout, subtree), *this;
}

template <typename node>
std::ostream& dump(std::ostream& os, const std::map<tref, tref>& m,
	bool subtree)
{
	using tree = lcrs_tree<node>;
	std::cout << "tref_map: " << m.size() << "\n";
	for (const auto& [k, v] : m) {
		os << "\t";
		tree::dump(os, k, subtree);
		os << " -> ";
		tree::dump(os, v, subtree);
		os << "\n";
	}
	return os << "----------------------------------\n";
}

template <typename node>
std::string dump_to_str(const std::map<tref, tref>& m, bool subtree) {
	std::stringstream ss;
	return dump<node>(ss, m, subtree), ss.str();
}

template <typename node>
std::ostream& dump(std::ostream& os, const subtree_map<node, tref>& m,
	bool subtree)
{
	using tree = lcrs_tree<node>;
	std::cout << "subtree_map: " << m.size() << "\n";
	for (const auto& [k, v] : m) {
		os << "\t";
		tree::dump(os, k, subtree);
		os << " -> ";
		tree::dump(os, v, subtree);
		os << "\n";
	}
	return os << "----------------------------------\n";
}

template <typename node>
std::string dump_to_str(const subtree_map<node, tref>& m, bool subtree) {
	std::stringstream ss;
	return dump<node>(ss, m, subtree), ss.str();
}

//------------------------------------------------------------------------------
// hooks

template <typename node>
void lcrs_tree<node>::set_hook(hook_function h) { hook = h; }

template <typename node>
void lcrs_tree<node>::reset_hook() { hook = nullptr; }

template <typename node>
bool lcrs_tree<node>::is_hooked() { return hook != nullptr; }

//------------------------------------------------------------------------------

// helper to get value from a cache
template <typename node>
tref get_cached(tref n, const std::map<tref, tref>& cache) {
	if (auto it = cache.find(n); it != cache.end())
		return it->second;
	return n;
}

// helper to get value from a subtree cached map (using subtree_equality)
template <typename node>
tref get_cached(tref n, const subtree_map<node, tref>& cache) {
	if (auto it = cache.find(n); it != cache.end())
		return it->second;
	return n;
}

// helper to get a value from a cache using subtree_equality
template <typename node>
tref get_cached_subtree(tref n, const std::map<tref, tref>& cache) {
	using tree = lcrs_tree<node>;
	const auto& t = tree::get(n);
	for (auto it = cache.begin(); it != cache.end(); ++it) {
		if (tree::get(it->first) == t) {
			n = it->second;
			break;
		}
	}
	return n;
}

template <typename node>
bool is_cached_subtree(tref n, const std::unordered_set<tref>& cache) {
	using tree = lcrs_tree<node>;
	const auto& t = tree::get(n);
	for (auto it = cache.begin(); it != cache.end(); ++it)
		if (tree::get(*it) == t) return true;
	return false;
}


//------------------------------------------------------------------------------

} // idni namespace
