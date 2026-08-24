// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILS__HASHING_H__
#define __IDNI__PARSER__UTILS__HASHING_H__
#include <functional>
#include <vector>
#include <tuple>
#include <utility>
#include <array>
#include <string>
#include <cstdint>
#include <concepts>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

// Hash policy: exactly one of TAU_HASH_POLICY_DEFAULT / _FNV1A / _WYHASH is
// defined by CMake (TAU_PARSER_HASH_POLICY / TAU_HASH_POLICY). Falls back to
// `default` if none is defined (e.g. this header used outside the project's
// own build).
//
//   default -- std::hash<T> for every leaf type, exactly as this file
//     shipped before hash portability was a concern. Reproduces committed
//     behaviour bit-for-bit on every platform where size_t is 64 bits
//     (all of this project's native targets). NOT portable across pointer
//     widths: wasm32's size_t is 32 bits, so the same logical value can
//     hash differently there. That is the accepted trade for `default`
//     meaning "what shipped" -- portable modes are opt-in.
//   fnv1a / wyhash -- canonical: string/integral/enum leaves are hashed by
//     content through a fixed-width primitive, so the same logical value
//     hashes the same on every target regardless of size_t width.
#if !defined(TAU_HASH_POLICY_DEFAULT) && !defined(TAU_HASH_POLICY_FNV1A) \
	&& !defined(TAU_HASH_POLICY_WYHASH)
#define TAU_HASH_POLICY_DEFAULT
#endif

#if defined(TAU_HASH_POLICY_WYHASH)
#include "ankerl/unordered_dense.h"
#endif

namespace idni {

static inline constexpr std::uint64_t grcprime = 0x9e3779b97f4a7c15ull;

#if defined(TAU_HASH_POLICY_WYHASH)
// wyhash over raw bytes, via the low-level primitive ankerl::unordered_dense
// ships (not its ankerl::unordered_dense::hash<T> functor, whose fallback
// and pointer specializations hash addresses). wyhash reads memory as
// native-width integers internally, so it is little-endian only by
// construction: the guarantee is "identical across little-endian targets"
// (all of this project's, including wasm32), not "identical everywhere".
inline std::uint64_t hash_bytes(const unsigned char* data, std::size_t len) {
	return ankerl::unordered_dense::detail::wyhash::hash(data, len);
}
#elif defined(TAU_HASH_POLICY_FNV1A)
// FNV-1a over raw bytes, read explicitly a byte at a time so the result
// does not depend on host endianness or struct layout.
constexpr std::uint64_t hash_bytes(const unsigned char* data, std::size_t len) {
	std::uint64_t h = 0xcbf29ce484222325ull;
	for (std::size_t i = 0; i < len; ++i) {
		h ^= data[i];
		h *= 0x100000001b3ull;
	}
	return h;
}
#endif

// std::hash<T> is implementation-defined: libstdc++ (native) and libc++
// (Emscripten) are free to -- and do -- hash identical content to
// different bit patterns, and it operates on sizeof(T), so the same
// logical value hashes differently when T's width differs by platform
// (e.g. size_t is 32 bits on wasm32, 64 bits natively). Under a portable
// policy, portable_hash gives integral/enum and string content a hash that
// depends only on the logical value, not on the platform's std::hash
// implementation or type widths; everything else falls back to
// std::hash<T>, which is fine for this codebase's own types since their
// specializations bottom out in this same path (see e.g.
// node<BAs...>::hashit()). Under the default policy, portable_hash is
// std::hash<T> for every leaf type -- see the policy comment above.
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
// This composite handling (vector/array/pair/tuple) is the same shape under
// every policy -- committed HEAD gave each its own std::hash specialization
// doing exactly this local-loop combine; only the leaf primitive below
// (string/integral/enum) varies by policy.
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
#if defined(TAU_HASH_POLICY_DEFAULT)
	else return std::hash<T>{}(v);
#else
	else if constexpr (std::same_as<T, std::string>)
		return hash_bytes(reinterpret_cast<const unsigned char*>(v.data()),
			v.size());
	else if constexpr (std::integral<T> || std::is_enum_v<T>) {
		unsigned char bytes[8];
		std::uint64_t w = static_cast<std::uint64_t>(v);
		for (int i = 0; i < 8; ++i)
			bytes[i] = static_cast<unsigned char>(w >> (i * 8));
		return hash_bytes(bytes, 8);
	} else return std::hash<T>{}(v);
#endif
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
