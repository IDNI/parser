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

#include "bintree.h"

namespace idni2 {

template <typename T>
tref_range<T>::tref_range(tref first_) : first(first_) {}

template <typename T>
tref_range<T>::iterator::iterator(tref node) : current(node) {}

template <typename T>
tref tref_range<T>::iterator::operator*() const {
	return current;
}

template <typename T>
tref_range<T>::iterator& tref_range<T>::iterator::operator++() {
	if (current != nullptr)
		current = bintree<T>::get(current).r;
	return *this;
}

template <typename T>
bool tref_range<T>::iterator::operator!=(const iterator& other) const {
	return current != other.current;
}

template <typename T>
tref_range<T>::iterator tref_range<T>::begin() const { return iterator(first); }

template <typename T>
tref_range<T>::iterator tref_range<T>::end() const { return iterator(nullptr); }

//------------------------------------------------------------------------------

template <typename T>
const htree::sp bintree<T>::geth(tref h) {
	//DBG(assert(h != NULL);)
	if (h == NULL) return htree::null();
	auto res = M.find(*reinterpret_cast<const bintree*>(h)); //done with one search
	htree::sp ret;
	if (res != M.end()) res->second = ret = htree::sp(new htree(h));
	assert(!res->second.expired());
	return res->second.lock();
}

template <typename T>
const bintree<T>& bintree<T>::get(const tref id) {
	//std::cout<< id <<"\n";
	assert(id != NULL && sizeof(bintree<T>*) == sizeof(tref));
	return *(bintree<T>*)id;
}

template <typename T>
const bintree<T>& bintree<T>::get(const htree::sp& h) {
	return get(h->get());
}

template <typename T>
htree::sp bintree<T>::get(const T& v, const htree::sp& l, const htree::sp& r ) {
	return geth(get(v, l->get(), r->get()));
}

template <typename T>
tref bintree<T>::get(const T& v, tref l, tref r) {
	bintree bn(v, l, r);
	auto res = M.emplace(bn , htree::wp());
	return reinterpret_cast<tref>(&res.first->first);
}

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

	DBG(dump();)
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
		assert(psp->hnd == x.first);
		hm.emplace(psp->hnd = shift[x.first], psp);
	}
	htree::M = std::move(hm);
	*/
	//delete non-reachable garbage nodes from M
	for (auto it = M.begin(); it != M.end(); )
		if (next.count(reinterpret_cast<tref>(&it->first)) == 0)
			it = M.erase(it);
		else it++;

	DBG(dump();)
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
const htree::sp lcrs_tree<T>::geth(tref h) {
	return bintree<T>::geth(h);
}

template <typename T>
lcrs_tree<T>& lcrs_tree<T>::get(const htree::sp& h) {
	return (lcrs_tree<T>&) bintree<T>::get(h);
}

template <typename T>
const lcrs_tree<T>& lcrs_tree<T>::get(const tref id) {
	return (lcrs_tree<T>&) bintree<T>::get(id);
}

/*
template <typename T>
const htree::sp lcrs_tree<T>::get( const T& v, std::vector<T>&& child) {
	htree::sp prs = htree::null();
	for (int_t i = child.size() - 1; i >= 0 ; i--)
		prs = bintree<T>::get(child[i], htree::null(), prs);
	return bintree<T>::get(v, prs, htree::null());
}
*/
template <typename T>
template<typename U>
tref lcrs_tree<T>::get(const T& v, const U* child, size_t len) {
	if constexpr (std::is_same<U, tref>::value)
		return bintree<T>::get(v, child[0], child[1]);
	else {
		tref pr = NULL;
		for (int_t i = int_t(len) - 1; i >= 0; i--)
			pr = bintree<T>::get(child[i], NULL, pr);
		return bintree<T>::get(v, pr, NULL);
	}
}

template <typename T>
tref lcrs_tree<T>::get(const T& v, const std::vector<tref>& children) {
	tref pr = nullptr;
	for (auto& ch : children)
		pr = bintree<T>::get(get(ch).value, get(ch).l, pr);
	return bintree<T>::get(v, pr, (tref) nullptr);
}

/*
template <typename T>
const htree::sp tree<T>::get( const T& v, std::vector<T>&& child){
	return lcrs_tree<T>::get(v, std::move(child) );
}
*/

template <typename T>
size_t lcrs_tree<T>::get_child_size() const {
	size_t len = 0;
	get_kary_child(nullptr, len);
	return len;
}

//fills caller allocated child array upto len size
//with child ids. If child is nullptr, then just
//calculates number of children in len.
//caller's responsibility to free allocated memory
template <typename T>
bool lcrs_tree<T>::get_kary_child(tref* child, size_t& len) const {
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
auto lcrs_tree<T>::get_lcrs_child() {
	return std::make_pair<decltype(this->l), decltype(this->r)>(
						this->l, this->r);
}

template <typename T>
trefs lcrs_tree<T>::get_children_trefs() const {
	trefs ch;
	for (const auto& c : get_children()) ch.push_back(c);
	return ch;
}

template <typename T>
const lcrs_tree<T>& lcrs_tree<T>::get_child(size_t n) const {
	size_t i = 0;
	for (const auto& c : get_children()) {
		if (n > i) break;
		if (i == n) return c;
		++i;
	}
	assert(false);	
}

template <typename T>
tref_range<T> lcrs_tree<T>::get_children() const {
	return tref_range<T>(this->l);
}

//------------------------------------------------------------------------------

template <typename T>
const htree::sp tree<T>::geth(tref h) {
	return base_t::geth(h);
}

template <typename T>
const tree<T>& tree<T>::get(const htree::sp& h) {
	return (const tree&) bintree<T>::get(h);
}

template <typename T>
const tree<T>& tree<T>::get(const tref id) {
	return (const tree&) bintree<T>::get(id);
}

template <typename T>
template <typename U>
tref tree<T>::get(const T& v, const U* child, size_t len) {
	return base_t::get(v, child, len);
}

template <typename T>
tref tree<T>::get(const T& v, const std::vector<tref>& children) {
	return base_t::get(v, children);
}

template <typename T>
size_t tree<T>::get_child_size() const {
	return base_t::get_child_size();
}

template <typename T>
bool tree<T>::get_child(tref *child, size_t& len) const {
	return base_t::get_kary_child(child, len);
}

template <typename T>
const tree<T>& tree<T>::get_child(size_t n) const {
	return base_t::get_child(n);
}

template <typename T>
tref_range<T> tree<T>::get_children() const {
	return base_t::get_children();
}

template <typename T>
trefs tree<T>::get_children_trefs() const {
	return base_t::get_children_trefs();
}

template <typename T>
std::ostream& tree<T>::print(std::ostream& o, size_t s) const {
	for (size_t i = 0; i < s; i++) o << " ";
	o << this->str() << "\n";
	for (const auto& c : get_children()) get(c).print(o, s + 1);
	return o;
}

}
