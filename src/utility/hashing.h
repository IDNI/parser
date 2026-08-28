// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILS__HASHING_H__
#define __IDNI__PARSER__UTILS__HASHING_H__
#include <functional>
#include <vector>
#include <tuple>
#include <utility>
#include <array>
#include <cstdint>
#include <type_traits>

namespace idni {

static inline constexpr std::uint64_t grcprime = 0x9e3779b97f4a7c15ull;

// Hashing is done in uint64_t regardless of the platform's size_t width, so
// the mixing below is identical on 32-bit targets (e.g. wasm32) and 64-bit
// ones. Leaves are hashed with std::hash<T>.
template<typename> inline constexpr bool is_hashable_seq = false;
template<typename T, typename A>
inline constexpr bool is_hashable_seq<std::vector<T, A>> = true;
template<typename T, std::size_t N>
inline constexpr bool is_hashable_seq<std::array<T, N>> = true;

template<typename> inline constexpr bool is_hashable_pair = false;
template<typename A, typename B>
inline constexpr bool is_hashable_pair<std::pair<A, B>> = true;

template<typename> inline constexpr bool is_hashable_tuple = false;
template<typename... Ts>
inline constexpr bool is_hashable_tuple<std::tuple<Ts...>> = true;

template <typename T> constexpr std::uint64_t portable_hash(const T& v);

template <typename T, typename... Rest>
constexpr void hash_combine(std::uint64_t& seed, const T& v, Rest... rest) {
	seed ^= portable_hash(v) + grcprime + (seed << 12) + (seed >> 4);
        (hash_combine(seed, rest), ...);
}

template<typename T>
constexpr void hash_combine (std::uint64_t& seed, const T& v) {
	seed ^= portable_hash(v) + grcprime + (seed << 12) + (seed >> 4);
}

// Composites are hashed here rather than through std::hash: that interface
// must return size_t, which is 32 bits on wasm32, so routing a 64-bit value
// through it truncates and the result stops depending only on the content.
template <typename T>
constexpr std::uint64_t portable_hash(const T& v) {
	if constexpr (is_hashable_seq<T>) {
		std::uint64_t seed = v.size();
		for (auto& i : v) hash_combine(seed, i);
		return seed;
	} else if constexpr (is_hashable_pair<T>) {
		std::uint64_t seed = 0;
		hash_combine(seed, v.first, v.second);
		return seed;
	} else if constexpr (is_hashable_tuple<T>) {
		std::uint64_t seed = 0;
		std::apply([&seed](auto&&... xs) {
			(hash_combine(seed, xs), ...); }, v);
		return seed;
	}
	else return std::hash<T>{}(v);
}

} // namespace idni

namespace std {

// std::hash must return size_t, so these narrow. That is fine here: they
// serve unordered containers, where the value is a bucket index and not an
// ordering key. Anything ordered by hash must call idni::portable_hash.
template<typename T>
struct hash<vector<T>> {
	size_t operator()(const vector<T>& vec) const noexcept {
		return static_cast<size_t>(idni::portable_hash(vec));
	}
};

template<typename T, size_t N>
struct hash<array<T, N>> {
	size_t operator()(const std::array<T, N>& a) const noexcept {
		return static_cast<size_t>(idni::portable_hash(a));
	}
};

template<typename T1, typename T2>
struct hash<pair<T1, T2>> {
	size_t operator()(const std::pair<T1, T2>& p) const noexcept {
		return static_cast<size_t>(idni::portable_hash(p));
	}
};

template<typename... Ts>
struct hash<std::tuple<Ts...>> {
	size_t operator()(const std::tuple<Ts...>& p) const noexcept {
		return static_cast<size_t>(idni::portable_hash(p));
	}
};

} // namespace std

#endif // __IDNI__PARSER__UTILS__HASHING_H__
