#include <vector>
#include <map>
#include <memory>
#include "vec.h"

/// Instructions:
/// 1. you can create trees using tree::get, and access them using
/// tree:get, tree::data, tree::rank, tree::next.
/// 2. each tree gets a unique size_t id.
/// 3. the "next" is an array of tree size_t ids.
/// 4. you can also get a handle to tree, using htree::get.
/// 5. given a handle h, you can access its internal tree id by h->id().
/// 6. note that h->id() is "volatile": it can change with any call to gc.
/// 7. use handles only if you want gc to not delete your tree. do not use it
/// for temporary trees. otherwise it'll cause unnecessary calls to "new".
/// 8. if you have a handle to your tree, gc will not delete its subtrees, even
/// if they don't have a handle.

template<typename T> struct htree;
template<typename T> using sphtree = std::shared_ptr<htree<T>>;

template<typename T> struct tree {
	const T t;
	const size_t n; /// vec id of next tree ids
	tree(const T& t, size_t n) : t(t), n(n) {}
	bool operator<(const tree& x) const {
		return t != x.t ? t < x.t : (n < x.n);
	}
	/// be careful with inline statics. remove "inline" if needed
	inline static std::vector<tree> V;
	inline static std::map<tree, size_t> M;
	inline static size_t get(const T& t, size_t len, const size_t* n) {
		return get(tree(t, get_vec(len, n)));
	}
	inline static size_t get(const tree& t) {
		if (auto it = M.find(t); it != M.end()) return it->second;
		return M.emplace(t, V.size()), V.push_back(t), V.size() - 1;
	}
	inline static const T& data(size_t n) { return V[n].t; }
	inline static const size_t rank(size_t n) {return get_vec_len(V[n].n);}
	inline static const size_t* next(size_t n) { return get_vec(V[n].n); }
};

template<typename T> struct htree {
	size_t t;
	static sphtree<T> get(size_t t) {
		if (auto it = M.find(t); it != M.end()) return it->second.lock();
		sphtree h(new htree(t));
		return M.emplace(t, std::weak_ptr<htree<T>>(h)), h;
	}
	~htree() { M.erase(t); }

	static void gc() {
		std::set<size_t> ts, vs;
		for (const auto& x : M) mark_all(x.first, ts, vs);
		std::vector<size_t> t = gc_vec(vs);
		vs.clear();
		tree<T>::M.clear();
		std::vector<tree<T>> v;
		size_t n = 0;
		for (size_t x : ts) {
			if (auto it = M.find(x); it != M.end())
				it->second.lock()->t = v.size();
			tree y(tree<T>::V[x].t, t[n++]);
			tree<T>::M.emplace(y, v.size());
			v.push_back(y);
		}
		tree<T>::V = move(v);
	}
private:
	htree(size_t t) : t(t) { }
	size_t id() const { return t; }
	inline static std::map<size_t, std::weak_ptr<htree>> M;

	static void mark_all(size_t t,
		std::set<size_t>& ts, std::set<size_t>& vs) {
		if (ts.find(t) != ts.end()) return;
		vs.insert(tree<T>::V[t].n);
		const size_t r = tree<T>::rank(t), *x = tree<T>::next(t);
		for (size_t n = 0; n != r; ++n) mark_all(x[n], ts, vs);
	}
};
