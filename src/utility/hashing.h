// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILS__HASHING_H__
#define __IDNI__PARSER__UTILS__HASHING_H__
#include <functional>
#include <vector>
#include <tuple>
#include <utility>

namespace idni {

static inline constexpr std::size_t grcprime = sizeof(std::size_t) == 8
	? 0x9e3779b97f4a7c15  // 64-bit golden ratio constant
	: 0x9e3779b9;

template <typename T, typename... Rest>
constexpr void hash_combine(size_t& seed, const T& v, Rest... rest) {
	if constexpr (sizeof(std::size_t) == 8)
		seed ^= std::hash<T>()(v) + grcprime + (seed << 12) + (seed >> 4);
	else seed ^= std::hash<T>()(v) + grcprime + (seed << 6) + (seed >> 2);
        (hash_combine(seed, rest), ...);
}

template<typename T>
constexpr void hash_combine (size_t& seed, const T& v) {
	if constexpr (sizeof(std::size_t) == 8)
		seed ^= std::hash<T>{}(v) + grcprime + (seed << 12) + (seed >> 4);
	else seed ^= std::hash<T>{}(v) + grcprime + (seed << 6) + (seed >> 2);
}

} // namespace idni

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

} // namespace std

#endif // __IDNI__PARSER__UTILS__HASHING_H__