// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.txt

#ifndef __IDNI__PARSER__DEFS_H__
#define __IDNI__PARSER__DEFS_H__

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <functional>
#ifdef DEBUG
#	include <cxxabi.h>     // unmangle symbol names for debugging
#endif

#include "utility/hashing.h"

namespace idni {

using int_t = int32_t;

} // idni namespace

#ifdef DEBUG
#	define DBG(x) x
#else
#	define DBG(x)
#endif
#ifdef TAU_PARSER_MEASURE
#	define MS(x) x
#else
#	define MS(x)
#endif // TAU_PARSER_MEASURE

#define DBGP(x) if (debug) { x }
#define DEBUG_POS_FROM 0
#define DEBUG_POS_TO   0
//if DEBUG_POS_TO > 0 show debugging information only for a given position range

#define tdiff(start, end) ((double(end - start) / CLOCKS_PER_SEC) * 1000)
#define emeasure_time_start(start, end) clock_t end, start = clock()
#define emeasure_time_end_to(start, end, os) end = clock(), os << std::fixed <<\
	std::setprecision(2) << tdiff(start, end) << " ms"
#define emeasure_time_end(start, end) emeasure_time_end_to(start, end,std::cout)

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

#include "./version_license.h" // include generated version and license constants

#endif // __IDNI__PARSER__DEFS_H__
