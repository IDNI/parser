// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__TESTS__TEST_ENV_H__
#define __IDNI__PARSER__TESTS__TEST_ENV_H__

#include <cstdlib>

// Windows has no setenv and unsetenv.
inline void test_setenv(const char* name, const char* value) {
#ifdef _WIN32
	_putenv_s(name, value);
#else
	setenv(name, value, 1);
#endif
}

inline void test_unsetenv(const char* name) {
#ifdef _WIN32
	// an empty value removes the variable
	_putenv_s(name, "");
#else
	unsetenv(name);
#endif
}

#endif // __IDNI__PARSER__TESTS__TEST_ENV_H__
