// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__UNSIGNED_CHAR_TRAITS_H__
#define __IDNI__PARSER__UTILITY__UNSIGNED_CHAR_TRAITS_H__

// std::char_traits<unsigned char> for emscripten

#ifdef __EMSCRIPTEN__
namespace std {

template<>
struct char_traits<unsigned char> : char_traits<char> {
        typedef unsigned char char_type;
        typedef int int_type;
        typedef streamoff off_type;
        typedef streampos pos_type;
        typedef mbstate_t state_type;

        static void assign(char_type& r, const char_type& a) noexcept {
                r = a;
        }
        static bool eq(char_type a, char_type b) noexcept {
                return a == b;
        }
        static bool lt(char_type a, char_type b) noexcept {
                return a < b;
        }

        static int compare(const char_type* s1, const char_type* s2,
                size_t n)
        {
                if (!s1 || !s2) return 0;
                for (size_t i = 0; i < n; ++i) {
                        if (lt(s1[i], s2[i])) return -1;
                        if (lt(s2[i], s1[i])) return 1;
                }
                return 0;
        }

        static size_t length(const char_type* s) {
                if (!s) return 0;
                size_t len = 0;
                while (len < SIZE_MAX && !eq(s[len], char_type(0)))
                        ++len;
                return len;
        }

        static const char_type* find(const char_type* s, size_t n,
                const char_type& a)
        {
                if (!s) return nullptr;
                for (size_t i = 0; i < n; ++i) if (eq(s[i], a))
                        return s + i;
                return nullptr;
        }

        static char_type* move(char_type* s1, const char_type* s2, size_t n) {
                if (n == 0 || !s1 || !s2) return s1;
                return static_cast<char_type*>(memmove(s1, s2, n));
        }

        static char_type* copy(char_type* s1, const char_type* s2, size_t n) {
                if (n == 0 || !s1 || !s2) return s1;
                return static_cast<char_type*>(memcpy(s1, s2, n));
        }

        static char_type* assign(char_type* s, size_t n, char_type a) {
                if (!s) return nullptr;
                for (size_t i = 0; i < n; ++i) s[i] = a;
                return s;
        }

        static int_type not_eof(int_type c) noexcept {
                return eq_int_type(c, eof()) ? 0 : c;
        }

        static char_type to_char_type(int_type c) noexcept {
                return static_cast<char_type>(c);
        }

        static int_type to_int_type(char_type c) noexcept {
                return static_cast<int_type>(c);
        }

        static bool eq_int_type(int_type c1, int_type c2) noexcept {
                return c1 == c2;
        }

        static int_type eof() noexcept {
                return static_cast<int_type>(EOF);
        }
};

} // namespace std
#endif
#endif // __IDNI__PARSER__UTILITY__UNSIGNED_CHAR_TRAITS_H__
