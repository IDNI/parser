#include <vector>
#include <map>
#include "vec.h"

template<typename T> struct tree {
	const T t;
	const size_t n; // vec id of next tree ids
	tree(const T& t, size_t n) : t(t), n(n) {}
	bool operator<(const tree& x) const {
		if (t != x.t) return t < x.t;
		return n < x.n;
	}
	// be careful with inline statics. remove "inline" if needed
	inline static std::vector<tree> V;
	inline static std::map<tree, size_t> M;
	inline static size_t get(const T& t, size_t len, const size_t* n) {
		return get(tree(t, get_vec(len, n)));
	}
	inline static size_t get(const tree& t) {
		if (auto it = M.find(t); it != M.end()) return it->second;
		M.emplace(t, V.size());
		V.push_back(t);
		return V.size() - 1;
	}
	struct data {
		const T& t;
		const size_t len, *n;
		data(const T& t, size_t len, const size_t* n) :
			t(t), len(len), n(n) {}
	};
	inline static data get(size_t n) {
		return data(V[n].t, get_vec_len(V[n].n), get_vec(V[n].n));
	}
};
