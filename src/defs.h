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
namespace idni {

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

typedef int32_t int_t;

} // idni namespace
#endif // __IDNI__PARSER__DEFS_H__
