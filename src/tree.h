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
#include <memory>


namespace idni {

template <typename T>
struct bintree;

struct htree {
	using sp =  std::shared_ptr<htree>; 
	using wp = 	std::weak_ptr<htree>;
	inline bool	operator==(const htree& r) const { return hnd == r.hnd; }
	inline bool operator< (const htree& r) const { return hnd <  r.hnd; }
	static sp& null() { 
		static sp hnull = get(-1);
		return hnull; 
	}
	static sp get(int_t h) {
		assert(h >= -1);
		auto res = M.emplace(h, wp()); //done with one search
		if(res.second) {
			sp nsp(new htree(h));
			return res.first->second = nsp, nsp;		
		}
		assert(!res.first->second.expired());
		return res.first->second.lock(); 
	}
	int_t get() { return hnd; }
	~htree(){ if(hnd >= -1) M.erase(hnd); }

	private:
	int_t hnd;
	explicit htree(int_t h = -1) : hnd(h) { }
	static std::unordered_map<int_t, wp> M;	
	template <typename U>
	friend struct bintree;
};
std::unordered_map<int_t, htree::wp> htree::M;


template <typename T>
struct bintree {
	protected: 
	static std::vector<bintree> V;
	static std::map<bintree, int_t> M;
	const T val;
	const int_t l, r;
	bintree( const T& _val, int_t _l = -1, int_t _r = -1): 
		val(_val), l(_l), r(_r) {}
	static bintree& get(int_t id) { return V[id]; }
	static int_t get(const T& v, int_t l = -1, int_t r =-1) {
 		bintree bn(v, l, r);		
		auto res = M.emplace( bn , V.size());
 		if (!res.second) return res.first->second;
		V.push_back(bn);
 		return V.size() - 1;
	}
	public:
	static bintree& get(htree::sp &h) { return get(h->get()); }
	static htree::sp get(const T& v, htree::sp &l = htree::null(),
						htree::sp &r = htree::null()) {
		return htree::get(get(v, l->get(), r->get()));
	}
	bool operator<(const bintree& o) const {
		if (val != o.val) return val < o.val;
		if (l != o.l) return l < o.l;
		return r < o.r;
	}
	bool operator==(const bintree& o) const {
		return val == o.val && l == o.l && r == o.r;
	}
	bintree& lt() { return V[l]; }
	bintree& rt() { return V[r]; }
	const T& v(){ return val;  }
	void print(size_t s = 0) {
		for (size_t i = 0; i < s; i++)
			std::cout<<" ";
		std::cout<<(int)this->val << std::endl;		
		if (l != -1) lt().print(s + 1);
		if (r != -1) rt().print(s + 1);
	}
	bool gc() {
		//mark all
		std::unordered_set<int_t> mark;
		for( auto x: M )
			if(htree::M.find(x.second) != htree::M.end())
				if(htree::M[x.second].expired())
					mark.insert(x.second);
		
		decltype(V) nv;
		std::unordered_map<int_t, int_t> shift;
		for (size_t i=0; i < V.size(); i++) {
			if(!mark.count(i))
				nv.push_back(V[i]),
				shift.emplace(i, nv.size() - 1),
				M[V[i]] = shift[i];
			else M.erase(V[i]); //gc from M
		}
		V = std::move(nv); //gc from V

		//update  htree handle
		decltype(htree::M) nm;
		//Todo

	

	}
};
template <typename T>
std::vector<bintree<T>> bintree<T>::V;
template <typename T>
std::map<bintree<T>, int_t> bintree<T>::M;

template <typename T> 
struct lcrs_tree : public bintree<T> {
	static htree::sp get( const T& v, std::vector<T>&& child = {}) {
		htree::sp prs = htree::null();
		for (int_t i = child.size()-1 ; i >= 0 ; i--) 
			prs = bintree<T>::get(child[i], htree::null(), prs);
		return bintree<T>::get(v, prs, htree::null());			
	}
	static lcrs_tree<T>& get(htree::sp& h) {
		return (lcrs_tree<T>&)bintree<T>::get(h); 
	}
	auto get_kary_child() {
		htree::sp curp = htree::get(this->l);
		std::vector<htree::sp> childs;
		while (curp != htree::null())
			childs.push_back(curp), curp = htree::get(get(curp).r);
		return childs;
	}
	auto get_lcrs_child() { return {this->l , this->r}; }	
};

template <typename T>
struct tree : protected lcrs_tree<T> {

	static htree::sp get( const T& v, std::vector<T>&& child = {}){
		return lcrs_tree<T>::get(v, std::move(child) );
	}
	static tree& get(htree::sp& h){
		return (tree&)bintree<T>::get(h);
	}
	auto get_child(){
		return lcrs_tree<T>::get_kary_child();
	}
	void print(size_t s = 0) {
		for (size_t i=0; i< s; i++)
			std::cout<<" ";
		std::cout<<(int)this->val << std::endl;		
		auto child = get_child();
		for (htree::sp& hch : child)
			if (!(hch == htree::null())) get(hch).print(s + 1);			
	}
};
} // idni namespace
#endif // __IDNI__TREE_H__
