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
#ifndef __IDNI_PARSER_TERM_COLOR_PRESETS_H__

#define __IDNI_PARSER_TERM_COLOR_PRESETS_H__
// terminal color presets
#define TC_DEFAULT       TC.DEFAULT()
#define TC_BG_DEFAULT    TC.BG_DEFAULT()
#define TC_STATUS        TC.BG_LIGHT_GRAY()
#define TC_STATUS_START  TC(term::color::MAGENTA, term::color::BG_LIGHT_GRAY, term::color::BRIGHT)
#define TC_STATUS_FILE   TC(term::color::BLUE,    term::color::BG_LIGHT_GRAY)
#define TC_PROMPT        TC(term::color::WHITE,   term::color::BRIGHT)
#define TC_NT            TC.YELLOW()
#define TC_NT_ID         TC.DARK_GRAY()
#define TC_T             TC(term::color::CYAN,    term::color::BRIGHT)
#define TC_NULL          TC.DARK_GRAY()
#define TC_RANGE         TC.DARK_GRAY()
#define TC_NEG           TC.RED()

#else

#undef __IDNI_PARSER_TERM_COLOR_PRESETS_H__
// terminal color presets
#undef TC_DEFAULT
#undef TC_BG_DEFAULT
#undef TC_STATUS
#undef TC_STATUS_START
#undef TC_STATUS_FILE
#undef TC_PROMPT
#undef TC_NT
#undef TC_CLEAR_NT
#undef TC_NT_ID
#undef TC_T
#undef TC_NULL
#undef TC_RANGE
#undef TC_NEG

#endif