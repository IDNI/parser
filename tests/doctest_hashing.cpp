// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Unit tests for the portable string hash (utility/hashing.h). The tree order
// compares hashes first, so a string hash must be the same on every platform.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "utility/hashing.h"

#include <cstdint>
#include <string>
#include <string_view>

using idni::portable_hash;
using idni::portable_string_hash;

// The values of the 64-bit libstdc++ std::hash<std::string>.
static_assert(portable_string_hash("") == 0x553e93901e462a6eull);
static_assert(portable_string_hash("a") == 0x454ddee488c1ed6bull);

TEST_SUITE("portable string hash") {

	TEST_CASE("pinned values") {
		CHECK(portable_string_hash("") == 0x553e93901e462a6eull);
		CHECK(portable_string_hash("a") == 0x454ddee488c1ed6bull);
		CHECK(portable_string_hash("b") == 0x96694f8e93f5c77dull);
		CHECK(portable_string_hash("abcdefgh") == 0x783db3e38db898bbull);
		CHECK(portable_string_hash("abcdefghi") == 0xb4ec9257851c8aafull);
		CHECK(portable_string_hash(
			"The quick brown fox jumps over the lazy dog")
			== 0xdccbf2541704bc75ull);
	}

	TEST_CASE("portable_hash of a string uses the portable string hash") {
		const std::string s = "bv[2]";
		CHECK(portable_hash(s) == portable_string_hash(s));
		CHECK(portable_hash(std::string_view{s}) == portable_string_hash(s));
		CHECK(portable_hash(s) == 0x87dec06375483aa2ull);
	}

	TEST_CASE("an integral or enum leaf hashes to its own value") {
		enum class color : unsigned char { red = 3 };
		CHECK(portable_hash(0) == 0ull);
		CHECK(portable_hash(42) == 42ull);
		CHECK(portable_hash(-1) == 0xffffffffffffffffull);
		CHECK(portable_hash(-1LL) == 0xffffffffffffffffull);
		CHECK(portable_hash(std::uint64_t{0x123456789abcdef0ull})
			== 0x123456789abcdef0ull);
		CHECK(portable_hash(std::size_t{7}) == 7ull);
		CHECK(portable_hash(true) == 1ull);
		CHECK(portable_hash(color::red) == 3ull);
	}

	TEST_CASE("a leaf with a 64-bit hash member hashes to that member") {
		struct cached { const std::uint64_t hash; };
		CHECK(portable_hash(cached{0xfedcba9876543210ull})
			== 0xfedcba9876543210ull);
	}

#if defined(__GLIBCXX__)
	TEST_CASE("equal to libstdc++ std::hash") {
		if constexpr (sizeof(std::size_t) == sizeof(std::uint64_t)) {
			std::string s;
			for (int len = 0; len < 300; ++len) {
				CHECK(portable_string_hash(s)
					== std::hash<std::string>{}(s));
				s.push_back(static_cast<char>((len * 37 + 11) & 0xff));
			}
			for (long long i : { 0LL, 1LL, -1LL, 1LL << 40, -(1LL << 40) })
				CHECK(portable_hash(i) == std::hash<long long>{}(i));
		}
	}
#endif

}
