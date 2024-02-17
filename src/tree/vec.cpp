#include <vector>
#include <map>
#include <cstring>
using namespace std;

vector<size_t> V;
map<size_t, vector<size_t>> M;

size_t get_vec_len(size_t n) { return V[n]; }
const size_t* get_vec(size_t n) { return &V[n + 1]; }

size_t get_vec(const size_t n, const size_t* v) {
	static size_t sz = sizeof(size_t);
	size_t h = (n << 32);
	for (size_t k = 0; k != n; ++k) h ^= v[k];
	const size_t k = V.size();
	if (auto it = M.find(h); it == M.end()) M[h].push_back(k);
	else {
		for (size_t t : it->second)
			if (V[t] == n && !memcmp(&V[t + 1], v, n * sz))
				return t;
		it->second.push_back(k);
	}
	return V.push_back(n), V.insert(V.end(), v, &v[n]), k;
}
