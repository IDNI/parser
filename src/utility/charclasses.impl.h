// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <cctype>
#include <cstdint>
#include <cstddef>
#include <type_traits>
#include "charclasses.h"
#include "charclasses_unicode.generated.h"

namespace idni::charclasses {

template<> inline bool isalnum<char>(char c) { return std::isalnum(static_cast<unsigned char>(c)); }
template<> inline bool isalpha<char>(char c) { return std::isalpha(static_cast<unsigned char>(c)); }
template<> inline bool isblank<char>(char c) { return std::isblank(static_cast<unsigned char>(c)); }
template<> inline bool iscntrl<char>(char c) { return std::iscntrl(static_cast<unsigned char>(c)); }
template<> inline bool isdigit<char>(char c) { return std::isdigit(static_cast<unsigned char>(c)); }
template<> inline bool isgraph<char>(char c) { return std::isgraph(static_cast<unsigned char>(c)); }
template<> inline bool islower<char>(char c) { return std::islower(static_cast<unsigned char>(c)); }
template<> inline bool isprint<char>(char c) { return std::isprint(static_cast<unsigned char>(c)); }
template<> inline bool ispunct<char>(char c) { return std::ispunct(static_cast<unsigned char>(c)); }
template<> inline bool isspace<char>(char c) { return std::isspace(static_cast<unsigned char>(c)); }
template<> inline bool isupper<char>(char c) { return std::isupper(static_cast<unsigned char>(c)); }
template<> inline bool isxdigit<char>(char c){ return std::isxdigit(static_cast<unsigned char>(c));}

inline uint16_t char_props(char32_t cp) {
	if (cp > 0x10FFFF) return 0;
	return stage2[static_cast<size_t>(stage1[cp >> 8]) * 256 + (cp & 0xFF)];
}

// The table agrees with <cctype> in the C locale for ASCII.
inline uint16_t ascii_props(char32_t cp) {
	return cp <= 0x7F ? char_props(cp) : 0;
}

template<> inline bool isalnum<char32_t>(char32_t cp) { return ascii_props(cp) & UC_ALNUM; }
template<> inline bool isalpha<char32_t>(char32_t cp) { return ascii_props(cp) & UC_ALPHA; }
template<> inline bool isblank<char32_t>(char32_t cp) { return ascii_props(cp) & UC_BLANK; }
template<> inline bool iscntrl<char32_t>(char32_t cp) { return ascii_props(cp) & UC_CNTRL; }
template<> inline bool isdigit<char32_t>(char32_t cp) { return ascii_props(cp) & UC_DIGIT; }
template<> inline bool isgraph<char32_t>(char32_t cp) { return ascii_props(cp) & UC_GRAPH; }
template<> inline bool islower<char32_t>(char32_t cp) { return ascii_props(cp) & UC_LOWER; }
template<> inline bool isprint<char32_t>(char32_t cp) { return ascii_props(cp) & UC_PRINT; }
template<> inline bool ispunct<char32_t>(char32_t cp) { return ascii_props(cp) & UC_PUNCT; }
template<> inline bool isspace<char32_t>(char32_t cp) { return ascii_props(cp) & UC_SPACE; }
template<> inline bool isupper<char32_t>(char32_t cp) { return ascii_props(cp) & UC_UPPER; }
template<> inline bool isxdigit<char32_t>(char32_t cp){ return ascii_props(cp) & UC_XDIGIT;}

// A code point of any integral type reads the char32_t class.
template <typename C> char32_t code_point(C c) {
	static_assert(std::is_integral_v<C>, "a code point is integral");
	return static_cast<char32_t>(c);
}

template <typename C> bool isalnum(C c) { return isalnum<char32_t>(code_point(c)); }
template <typename C> bool isalpha(C c) { return isalpha<char32_t>(code_point(c)); }
template <typename C> bool isblank(C c) { return isblank<char32_t>(code_point(c)); }
template <typename C> bool iscntrl(C c) { return iscntrl<char32_t>(code_point(c)); }
template <typename C> bool isdigit(C c) { return isdigit<char32_t>(code_point(c)); }
template <typename C> bool isgraph(C c) { return isgraph<char32_t>(code_point(c)); }
template <typename C> bool islower(C c) { return islower<char32_t>(code_point(c)); }
template <typename C> bool isprint(C c) { return isprint<char32_t>(code_point(c)); }
template <typename C> bool ispunct(C c) { return ispunct<char32_t>(code_point(c)); }
template <typename C> bool isspace(C c) { return isspace<char32_t>(code_point(c)); }
template <typename C> bool isupper(C c) { return isupper<char32_t>(code_point(c)); }
template <typename C> bool isxdigit(C c) { return isxdigit<char32_t>(code_point(c)); }

namespace unicode {

template<> inline bool isalnum<char>(char c) { return charclasses::isalnum<char>(c); }
template<> inline bool isalpha<char>(char c) { return charclasses::isalpha<char>(c); }
template<> inline bool isblank<char>(char c) { return charclasses::isblank<char>(c); }
template<> inline bool iscntrl<char>(char c) { return charclasses::iscntrl<char>(c); }
template<> inline bool isdigit<char>(char c) { return charclasses::isdigit<char>(c); }
template<> inline bool isgraph<char>(char c) { return charclasses::isgraph<char>(c); }
template<> inline bool islower<char>(char c) { return charclasses::islower<char>(c); }
template<> inline bool isprint<char>(char c) { return charclasses::isprint<char>(c); }
template<> inline bool ispunct<char>(char c) { return charclasses::ispunct<char>(c); }
template<> inline bool isspace<char>(char c) { return charclasses::isspace<char>(c); }
template<> inline bool isupper<char>(char c) { return charclasses::isupper<char>(c); }
template<> inline bool isxdigit<char>(char c){ return charclasses::isxdigit<char>(c);}

template<> inline bool isalnum<char32_t>(char32_t cp) { return char_props(cp) & UC_ALNUM; }
template<> inline bool isalpha<char32_t>(char32_t cp) { return char_props(cp) & UC_ALPHA; }
template<> inline bool isblank<char32_t>(char32_t cp) { return char_props(cp) & UC_BLANK; }
template<> inline bool iscntrl<char32_t>(char32_t cp) { return char_props(cp) & UC_CNTRL; }
template<> inline bool isdigit<char32_t>(char32_t cp) { return char_props(cp) & UC_DIGIT; }
template<> inline bool isgraph<char32_t>(char32_t cp) { return char_props(cp) & UC_GRAPH; }
template<> inline bool islower<char32_t>(char32_t cp) { return char_props(cp) & UC_LOWER; }
template<> inline bool isprint<char32_t>(char32_t cp) { return char_props(cp) & UC_PRINT; }
template<> inline bool ispunct<char32_t>(char32_t cp) { return char_props(cp) & UC_PUNCT; }
template<> inline bool isspace<char32_t>(char32_t cp) { return char_props(cp) & UC_SPACE; }
template<> inline bool isupper<char32_t>(char32_t cp) { return char_props(cp) & UC_UPPER; }
template<> inline bool isxdigit<char32_t>(char32_t cp){ return char_props(cp) & UC_XDIGIT;}

template <typename C> bool isalnum(C c) { return isalnum<char32_t>(code_point(c)); }
template <typename C> bool isalpha(C c) { return isalpha<char32_t>(code_point(c)); }
template <typename C> bool isblank(C c) { return isblank<char32_t>(code_point(c)); }
template <typename C> bool iscntrl(C c) { return iscntrl<char32_t>(code_point(c)); }
template <typename C> bool isdigit(C c) { return isdigit<char32_t>(code_point(c)); }
template <typename C> bool isgraph(C c) { return isgraph<char32_t>(code_point(c)); }
template <typename C> bool islower(C c) { return islower<char32_t>(code_point(c)); }
template <typename C> bool isprint(C c) { return isprint<char32_t>(code_point(c)); }
template <typename C> bool ispunct(C c) { return ispunct<char32_t>(code_point(c)); }
template <typename C> bool isspace(C c) { return isspace<char32_t>(code_point(c)); }
template <typename C> bool isupper(C c) { return isupper<char32_t>(code_point(c)); }
template <typename C> bool isxdigit(C c) { return isxdigit<char32_t>(code_point(c)); }

} // idni::charclasses::unicode namespace

} // idni::charclasses namespace
