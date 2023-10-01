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

namespace idni {
#define MEASURE
#define DEBUG
#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif
#ifdef MEASURE
#define MS(x) x
#else
#define MS(x)
#endif

#define tdiff(start, end) ((double(end - start) / CLOCKS_PER_SEC) * 1000)
#define emeasure_time_start(start, end) clock_t end, start = clock()
#define emeasure_time_end(start, end) end = clock(), std::cout << std::fixed <<\
	std::setprecision(2) << tdiff(start, end) << " ms"

typedef int32_t int_t;

} // idni namespace
#endif // __IDNI__PARSER__DEFS_H__
