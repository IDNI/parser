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
#ifndef __IDNI__PARSER__UTILS__HASHING_H__
#define __IDNI__PARSER__UTILS__HASHING_H__
#include <cstdint>
#include <functional>
#include <vector>
#include <tuple>
#include <utility>
#include <ostream>

namespace idni {

static inline constexpr std::size_t grcprime = sizeof(std::size_t) == 8
	? 0x9e3779b97f4a7c15  // 64-bit golden ratio constant
	: 0x9e3779b9;

template <typename T, typename... Rest>
constexpr void hash_combine(size_t& seed, const T& v, Rest... rest) {
        seed ^= std::hash<T>()(v) + grcprime + (seed << 12) + (seed >> 4);
        (hash_combine(seed, rest), ...);
}

template<typename T>
constexpr void hash_combine (size_t& seed, const T& v) {
	seed ^= std::hash<T>{}(v) + grcprime + (seed << 12) + (seed >> 4);
}

}

namespace std {

template<typename T>
struct hash<vector<T>> {
	size_t operator()(const vector<T>& vec) const {
		size_t seed = vec.size();
		for (auto& i : vec) idni::hash_combine(seed, i);
		return seed;
	}
};

template<typename T1, typename T2>
struct hash<pair<T1, T2>> {
	size_t operator()(const std::pair<T1, T2>& p) const noexcept {
		size_t seed = 0;
		idni::hash_combine(seed, p.first);
		idni::hash_combine(seed, p.second);
		return seed;
	}
};

template<typename... Ts>
struct hash<std::tuple<Ts...>> {
	size_t operator()(const std::tuple<Ts...>& p) const noexcept {
		size_t seed = 0;
		std::apply([&seed](auto&&... xs) {
			(idni::hash_combine(seed, xs),...); }, p);
		return seed;
	}
};

template<typename T>
ostream& operator<<(ostream& os, const vector<T>& vec) {
	os << "[";
	for (size_t i = 0; i < vec.size(); ++i)
		if (i + 1 < vec.size()) os << vec[i] << ",";
	if (vec.size()) os << vec.back();
	return os << "]";
}

}

#endif // __IDNI__PARSER__UTILS__HASHING_H__