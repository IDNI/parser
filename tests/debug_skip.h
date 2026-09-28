// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__TESTS__DEBUG_SKIP_H__
#define __IDNI__PARSER__TESTS__DEBUG_SKIP_H__

namespace idni::testing {

// The cases gated on this flag test the release fallback for input a debug
// assert rejects; under DEBUG the assert aborts before the fallback can run,
// so there is nothing left to verify there.
#ifdef DEBUG
inline constexpr bool skip_under_debug_assert = true;
#else
inline constexpr bool skip_under_debug_assert = false;
#endif

} // namespace idni::testing

#endif // __IDNI__PARSER__TESTS__DEBUG_SKIP_H__
