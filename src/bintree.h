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
#ifndef __IDNI__TREE_H__
#define __IDNI__TREE_H__

#include <vector>
#include <utility>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <iostream>
#include <cassert>
#include "defs.h"

namespace idni2 {

using namespace idni;

template <typename T>
struct bintree;

using tref = const intptr_t*;
using opt_tref = std::optional<tref>;
using trefs    = std::vector<tref>;

template <typename T>
struct tref_range {
	tref first;
	tref_range(tref first);
	struct iterator {
		tref current;
		iterator(tref node);
		tref operator*() const;
		iterator& operator++();
		bool operator!=(const iterator& other) const;
	};
	iterator begin() const;
	iterator end() const;
};

struct htree {
	using sp =  std::shared_ptr<htree>;
	using wp =  std::weak_ptr<htree>;
	inline bool operator==(const htree& r) const { return hnd == r.hnd; }
	inline bool operator< (const htree& r) const { return hnd <  r.hnd; }
	static const sp& null() {
		static const sp hnull(new htree());
		return hnull;
	}
	inline tref get() const { return hnd; }
	//~htree() { if (hnd != NULL) M.erase(hnd); }
private:
	tref hnd;
	explicit htree(tref h = nullptr) : hnd(h) { }
	template <typename U>
	friend struct bintree;
};

template <typename T>
struct bintree {
	const T value;
	const tref l, r;

	static const htree::sp geth(tref id);
	static const bintree& get(const tref id);
	static const bintree& get(const htree::sp& h);
	// there's a problem here. get() shouldnt return a handle, it'll create
	// a ptr with each call. normally the user has to keep a handle only to
	// the root of the tree of interest. functions that recurse over trees
	// and return new trees, should get a handle to the root, but internally
	// work only with the ids, not with the handles, then return a single
	// handle to the result.

	// resp:actually this internally uses id only to get. We make it available only
	// so user can use or maintain tree of interest to build.
	static htree::sp get(const T& v, const htree::sp& l = htree::null(),
					const htree::sp& r = htree::null());
	// This function(made public now) uses and returns ids only instead of handle
	// to address the previous concern
	static tref get(const T& v, tref l, tref r);

	static void dump();
	static void gc();

	bool operator<(const bintree& o) const;
	bool operator==(const bintree& o) const;
protected:
	bintree(const T& _value, tref _l = NULL, tref _r = NULL);
	std::ostream& print(std::ostream& o, size_t s = 0) const;
	virtual const std::string str() const;

	//static std::vector<const bintree*> V;
	static std::map<const bintree, htree::wp> M;
};

template <typename T>
struct lcrs_tree : public bintree<T> {
protected:
	static const htree::sp geth(tref h);
	static lcrs_tree<T>& get(const htree::sp& h);
	static const lcrs_tree<T>& get(const tref id);

	//static const htree::sp get( const T& v, std::vector<T>&& child = {});
	template<typename U = T>
	static tref get(const T& v, const U* child, size_t len);
	static tref get(const T& v, const std::vector<tref>& child);

	size_t get_child_size() const;
	//fills caller allocated child array upto len size
	//with child ids. If child is nullptr, then just
	//calculates number of children in len.
	//caller's responsibility to free allocated memory
	bool get_kary_child(tref* child, size_t& len) const;
	auto get_lcrs_child();
	trefs get_children_trefs() const;
	const lcrs_tree<T>& get_child(size_t n) const;
	tref_range<T> get_children() const;
};

template <typename T>
struct tree : public lcrs_tree<T> {
	using base_t = lcrs_tree<T>;

	static const htree::sp geth(tref h);
	static const tree& get(const htree::sp& h);
	static const tree& get(const tref id);

	//fills caller allocated child array upto len size
	//with child ids. If child is nullptr, then just
	//calculates number of children in len.
	//caller's responsibility to free allocated memory
	template<typename U = T>
	static tref get(const T& v, const U* child, size_t len);
	//static const htree::sp get(const T& v, std::vector<T>&& child = {});
	static tref get(const T& v, const std::vector<tref>& child);

	size_t get_child_size() const;
	bool get_child(tref *child, size_t& len) const;
	const tree& get_child(size_t n) const;
	tref_range<T> get_children() const;
	trefs get_children_trefs() const;
	std::ostream& print(std::ostream& o, size_t s = 0) const;
};

} // idni2 namespace

#include "bintree.tmpl.h"

#endif // __IDNI__TREE_H__
