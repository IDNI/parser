#include <iostream>
#include "tree.h"
using namespace std;

template<typename T> void print(size_t n) {
	auto d = tree<T>::get(n);
	cout << d.t << '(';
	for (size_t k = 0; k != d.len; ++k) {
		print<size_t>(d.n[k]);
		cout << (k == d.len - 1 ? ')' : ',');
	}
}

int main() {
	vector<vector<size_t>> v;
	v.push_back({1, 2, 3}); // 0
	v.push_back({1, 2, 3}); // 0
	v.push_back({2, 1, 3}); // 4
	v.push_back({2, 2, 3}); // 8
	v.push_back({2, 2, 1}); // 12
	v.push_back({2, 2, 3}); // 8
	v.push_back({2, 1, 3}); // 4
	for (auto t : v)
		cout << get_vec(t.size(), &t[0]) << endl;
	typedef tree<size_t> T;
	const size_t v1[] = {T::get(0, 0, {})};
	const size_t v2[] = {T::get(2, 1, v1), T::get(3, 1, v1)};
	print<size_t>(T::get(1, 2, v2));
	return 0;
}