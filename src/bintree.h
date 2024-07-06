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
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <iostream>
#include <cassert>
#include "defs.h"
namespace idni {

template <typename T>
struct bintree;

struct htree {
	using sp =  std::shared_ptr<htree>; 
	using wp = 	std::weak_ptr<htree>;
	inline bool	operator==(const htree& r) const { return hnd == r.hnd; }
	inline bool operator< (const htree& r) const { return hnd <  r.hnd; }
	static const sp& null() { 
		static const sp hnull(new htree(-1));
		return hnull; 
	}
	static sp get(int_t h);
	inline int_t get() const { return hnd; }
	~htree() { if (hnd >= 0) M.erase(hnd); }

	private:
    int_t hnd;
	explicit htree(int_t h = -1) : hnd(h) { }
	static std::unordered_map<int_t, wp> M;
	static void dump();
	template <typename U>
	friend struct bintree;
};


template <typename T>
struct bintree {
	protected: 
	static std::vector<const bintree*> V;
	static std::map<bintree, int_t> M;
	bintree( const T& _val, int_t _l = null_ref, int_t _r = null_ref): 
		val(_val), l(_l), r(_r) {}
	
	public:
    static const int_t null_ref = -1;
	static const bintree& get(const htree::sp &h);
	// there's a problem here. get() shouldnt return a handle, it'll create
	// a ptr with each call. normally the user has to keep a handle only to
	// the root of the tree of interest. functions that recurse over trees
	// and return new trees, should get a handle to the root, but internally
	// work only with the ids, not with the handles, then return a single
	// handle to the result.

    // resp:actually this internally uses id only to get. We make it available only
    // so user can use or maintain tree of interest to build.
	static htree::sp get(const T& v, const htree::sp &l = htree::null(),
						const htree::sp &r = htree::null());
    
    // This function(made public now) uses and returns ids only instead of handle
    // to address the previous concern
    static int_t get(const T& v, int_t l, int_t r);
    static const bintree& get(const int_t id);

	bool operator<(const bintree& o) const {
		if (val != o.val) return val < o.val;
		if (l != o.l) return l < o.l;
		return r < o.r;
	}
	bool operator==(const bintree& o) const {
		return val == o.val && l == o.l && r == o.r;
	}
	const T val;
	const int_t l, r;
	void print(size_t s = 0) const;
	const std::string str() const;
	static void dump() ;
	static void gc();
	
};

template <typename T> 
struct lcrs_tree : public bintree<T> {
    protected:
	//static const htree::sp get( const T& v, std::vector<T>&& child = {});
    static int_t get( const T& v, const T* child, size_t len);
	static lcrs_tree<T>& get(const htree::sp& h) {
		return (lcrs_tree<T>&)bintree<T>::get(h); 
	}

    size_t get_child_size() const {
        size_t len = 0;
        get_kary_child(nullptr, len);
        return len;
    }
    //fills caller allocated child array upto len size 
    //with child ids. If child is nullptr, then just 
    //calculates number of children in len.
    //caller's responsibility to free allocated memory
	bool get_kary_child(int_t* child, size_t &len) const {
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
        if (len > 0 && child == nullptr) return false;
        len = 0;
        int_t cur = this->l;
        while (cur != bintree<T>::null_ref) {
            if (child != nullptr) child[len] = cur;
            ++len, 
            cur = bintree<T>::get(cur).r;
        }
        return true;
	}
	auto get_lcrs_child() { return {this->l , this->r}; }	
};

template <typename T>
struct tree : public lcrs_tree<T> {
	//static const htree::sp get( const T& v, std::vector<T>&& child = {});
	static int_t get( const T& v, const T* child, size_t len);
    static const tree& get(const htree::sp& h);
    static const tree& get(const int_t id);
    //fills caller allocated child array upto len size 
    //with child ids. If child is nullptr, then just 
    //calculates number of children in len.
    //caller's responsibility to free allocated memory
	bool get_child(int_t *child, size_t &len) const;
    size_t get_child_size() const;
	void print(size_t s = 0) const;
};

template <typename T>
std::vector<const bintree<T>*> bintree<T>::V;
template <typename T>
std::map<bintree<T>, int_t> bintree<T>::M;


template <typename T>
htree::sp bintree<T>::get(const T& v, const htree::sp &l,
						const htree::sp &r ) {
	return htree::get(get(v, l->get(), r->get()));
}

template <typename T>
int_t bintree<T>::get(const T& v, int_t l, int_t r) {
    bintree bn(v, l, r);		
    auto res = M.emplace( bn , V.size());
    if (!res.second) return res.first->second;
    V.push_back(&(res.first->first));
    return V.size() - 1;
}

template <typename T>
const bintree<T>& bintree<T>::get(const htree::sp &h) {
    return get(h->get()); }

template<typename T>
const bintree<T>& bintree<T>::get(const int_t id) { 
    //std::cout<< id <<"\n";
    assert( id >=0 && (size_t)id < V.size());
    return *V[id]; 
}

template<typename T>
void bintree<T>::print(size_t s) const {
    for (size_t i = 0; i < s; i++)
        std::cout<<" ";	
    std::cout<<str()<< std::endl;
    if (l != -1) get(l).print(s + 1);
    if (r != -1) get(r).print(s + 1);
}

template<typename T>
const std::string bintree<T>::str() const {
    std::stringstream ss;
    ss << val;
    return "{" + ss.str() + "," + 
    std::to_string(l) + "," + std::to_string(r) + "} <-" + 
        std::to_string(M[*this]) ;
}

template<typename T>
void bintree<T>::dump() {
    std::cout<<"-----\n";
    std::cout<<"MB:"<<M.size()<<"\n";
    std::cout<<"VB:"<<M.size()<<"\n";
    for (auto &x : M){
        std::cout<<x.first.str() << " " <<x.second<<"\n";
    }
    std::cout<<"-----\n";
}

template<typename T>
void bintree<T>::gc() {
    DBG(dump();)
    DBG(htree::dump();)

    //mark all
    std::unordered_set<int_t> next;
    auto mark = [&next](int_t r, auto &mark)->void {
        if (!next.insert(r).second) return;
        auto &bn = get(r);
        if (bn.r != -1) mark(bn.r, mark);
        if (bn.l != -1) mark(bn.l, mark);
    };
    //check if any handle expired and needs to be garbaged
    // otherwise mark all to be retained live
    for (auto &x: htree::M)
        if (!x.second.expired()) mark(x.first, mark);

    // do not run gc, if no handle expired or V empty.
    if (!V.size() || (next.size() == V.size()) ) 
        return 
            DBG(std::cout<<"gc-do nothing:"<<next.size()<<"\n"), 
            void();
    
    DBG(assert(next.size() != V.size()));
    decltype(V) nv;
    std::unordered_map<int_t, int_t> shift;
    shift.emplace(-1,-1); // just for null
    //calculate shifts to handle
    for (size_t np=0 ,i = 0; i < V.size(); i++) 
        if (next.count(i)) shift.emplace(i, np++);
    for( auto &x: shift)
		DBG(std::cout<<x.first<< " "<< x.second <<"\n");

    M.clear();
    //reconstruct new V and M with updated handles
    for (size_t i = 0; i < V.size(); i++) 
        if (next.count(i)) {
            auto &bn = *V[i];
            bintree nbn(bn.val, shift[bn.l], shift[bn.r]);
            auto res = M.emplace(nbn, nv.size());
            nv.push_back(&(res.first->first));
        }

    V = std::move(nv); //gc from V
    
    //update htree handle as well
    decltype(htree::M) hm;
    for( auto &x : htree::M)
        if (!x.second.expired()) {
            auto psp = x.second.lock();
            assert(psp->hnd == x.first);
            hm.emplace(psp->hnd = shift[x.first], psp);
        }
    htree::M = std::move(hm);
		
    DBG(dump();)
    DBG(htree::dump();)
}

/*
template< typename T>
const htree::sp lcrs_tree<T>::get( const T& v, std::vector<T>&& child) {
    htree::sp prs = htree::null();
    for (int_t i = child.size()-1 ; i >= 0 ; i--) 
        prs = bintree<T>::get(child[i], htree::null(), prs);
    return bintree<T>::get(v, prs, htree::null());			
}
*/
template<typename T>
int_t lcrs_tree<T>::get( const T& v, const T* child, size_t len) {
    int_t nul = bintree<T>::null_ref;
    int_t pr = nul; 
    for ( int_t i = len-1; i >=0; i--)
        pr = bintree<T>::get(child[i], nul, pr);
    return bintree<T>::get(v, pr, nul);
}

/*
template<typename T>
const htree::sp tree<T>::get( const T& v, std::vector<T>&& child){
	return lcrs_tree<T>::get(v, std::move(child) );
}
*/
template<typename T>
int_t tree<T>::get( const T& v, const T* child, size_t len) {
	return lcrs_tree<T>::get(v, child, len );
}

template<typename T>
size_t tree<T>::get_child_size() const {
	return lcrs_tree<T>::get_child_size();
}

template<typename T>
const tree<T>& tree<T>::get(const htree::sp& h) {
	return (const tree&)bintree<T>::get(h);
}

template<typename T>
const tree<T>& tree<T>::get(const int_t id) {
	return (const tree&)bintree<T>::get(id);
}

template<typename T>
bool tree<T>::get_child(int_t *child, size_t &len) const {
	return lcrs_tree<T>::get_kary_child(child, len);
}

template<typename T>
void tree<T>::print(size_t s) const {
    for (size_t i=0; i< s; i++)
        std::cout<<" ";
    std::cout<<this->str()<< std::endl;		
    size_t nc = get_child_size();
    if (nc > 0) {
        int_t *ch_id = new int_t[nc];
        get_child(ch_id, nc);
        assert(ch_id != nullptr);
        for (size_t i=0 ; i < nc ; i++)
            get(ch_id[i]).print(s + 1);    
        if (ch_id != nullptr) delete [] ch_id;
    }    		
}

} // idni namespace
#endif // __IDNI__TREE_H__
