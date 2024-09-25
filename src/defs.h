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
#ifndef __IDNI__PARSER__DEFS_H__
#define __IDNI__PARSER__DEFS_H__
#include <cstdint>
#include <iomanip>
#include <functional>

namespace idni {

#ifdef DEBUG
#	define DBG(x) x
#else
#	define DBG(x)
#endif
#ifdef PARSER_MEASURE
#	define MS(x) x
#else
#	define MS(x)
#endif // PARSER_MEASURE

#define DBGP(x) if (debug) { x }
#define DEBUG_POS_FROM 0
#define DEBUG_POS_TO   0
//if DEBUG_POS_TO > 0 show debugging information only for a given position range

#define tdiff(start, end) ((double(end - start) / CLOCKS_PER_SEC) * 1000)
#define emeasure_time_start(start, end) clock_t end, start = clock()
#define emeasure_time_end_to(start, end, os) end = clock(), os << std::fixed <<\
	std::setprecision(2) << tdiff(start, end) << " ms"
#define emeasure_time_end(start, end) emeasure_time_end_to(start, end,std::cout)

typedef int32_t int_t;

//-----------------------------------------------------------------------------

// GIT_* macros are populated at compile time by -D or they're set to "n/a"
#ifndef GIT_DESCRIBED
#define GIT_DESCRIBED   "n/a"
#endif
#ifndef GIT_COMMIT_HASH
#define GIT_COMMIT_HASH "n/a"
#endif
#ifndef GIT_BRANCH
#define GIT_BRANCH      "n/a"
#endif
extern const std::size_t grcprime;

template <typename T, typename... Rest>
void hashCombine(size_t& seed, const T& v, Rest... rest) {
    seed ^= std::hash<T>()(v) + grcprime + (seed << 6) + (seed >> 2);
    (hashCombine(seed, rest), ...);
}

} // idni namespace
#endif // __IDNI__PARSER__DEFS_H__
