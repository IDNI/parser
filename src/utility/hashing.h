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
#include <string>
#include <string_view>
#include <type_traits>

namespace idni {

static inline constexpr std::uint64_t grcprime = 0x9e3779b97f4a7c15ull;

// Hashing is done in uint64_t regardless of the platform's size_t width, so
// the mixing below is identical on 32-bit targets (e.g. wasm32) and 64-bit
// ones. A string leaf uses portable_string_hash, an integral or enum leaf its
// own value, a leaf with a uint64_t hash member that member, and every other
// leaf std::hash<T>.
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

#ifndef TAU_USE_PORTABLE_HASH
#define TAU_USE_PORTABLE_HASH 1
#endif

#if TAU_USE_PORTABLE_HASH
// MurmurHash64A by Austin Appleby (public domain, SMHasher MurmurHash2.cpp),
// little-endian, seed 0xc70f6907: the Linux std::hash values, so the tree
// order is the same on every platform. TAU_USE_PORTABLE_HASH=0 uses std::hash.
constexpr std::uint64_t portable_string_hash(std::string_view s) {
	constexpr std::uint64_t m = 0xc6a4a7935bd1e995ull;
	constexpr int r = 47;
	auto byte = [&s](std::size_t i) {
		return static_cast<std::uint64_t>(
			static_cast<unsigned char>(s[i]));
	};
	const std::size_t len = s.size();
	std::uint64_t h = 0xc70f6907ull ^ (static_cast<std::uint64_t>(len) * m);
	std::size_t i = 0;
	for (; i + 8 <= len; i += 8) {
		std::uint64_t k = 0;
		for (std::size_t b = 0; b < 8; ++b) k |= byte(i + b) << (8 * b);
		k *= m;
		k ^= k >> r;
		k *= m;
		h ^= k;
		h *= m;
	}
	if (i < len) {
		for (std::size_t b = 0; i + b < len; ++b) h ^= byte(i + b) << (8 * b);
		h *= m;
	}
	h ^= h >> r;
	h *= m;
	h ^= h >> r;
	return h;
}
#else
inline std::uint64_t portable_string_hash(std::string_view s) {
	return std::hash<std::string_view>{}(s);
}
#endif

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
	} else if constexpr (std::is_same_v<T, std::string>
			|| std::is_same_v<T, std::string_view>) {
		return portable_string_hash(v);
	} else if constexpr (std::is_enum_v<T>) {
		return static_cast<std::uint64_t>(
			static_cast<std::underlying_type_t<T>>(v));
	} else if constexpr (std::is_integral_v<T>) {
		// The value itself, as libstdc++ and libc++ give on a 64-bit target.
		// MSVC mixes an integer, and wasm32 libc++ mixes a 64-bit one. A
		// negative value converts modulo 2^64.
		return static_cast<std::uint64_t>(v);
	} else if constexpr (requires { requires std::is_same_v<
			std::remove_cv_t<decltype(v.hash)>, std::uint64_t>; }) {
		// A value that caches a 64-bit hash, such as a tree node. std::hash
		// returns size_t and keeps only 32 bits of it on wasm32.
		return v.hash;
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
