// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.txt

#ifndef __IDNI__PARSER__PARSER_TERM_COLOR_MACROS_H__
#define __IDNI__PARSER__PARSER_TERM_COLOR_MACROS_H__

/// terminal color presets
#define TC_DEFAULT         TC.DEFAULT()
#define TC_CLEARED_DEFAULT (TC.CLEAR() + TC_DEFAULT)
#define TC_BG_DEFAULT      TC.BG_DEFAULT()
#define TC_STATUS          TC.BG_LIGHT_GRAY()
#define TC_STATUS_START    TC(term::color::MAGENTA, term::color::BG_LIGHT_GRAY, term::color::BRIGHT)
#define TC_STATUS_FILE     TC(term::color::BLUE,    term::color::BG_LIGHT_GRAY)
#define TC_PROMPT          TC(term::color::WHITE,   term::color::BRIGHT)
#define TC_NT              TC.YELLOW()
#define TC_CC              TC.BLUE()
#define TC_NT_ID           TC.DARK_GRAY()
#define TC_T               TC(term::color::CYAN,    term::color::BRIGHT)
#define TC_NULL            TC.DARK_GRAY()
#define TC_RANGE           TC.DARK_GRAY()
#define TC_NEG             TC.RED()
#else

#undef __IDNI__PARSER__PARSER_TERM_COLOR_MACROS_H__
/// terminal color presets
#undef TC_CLEARED_DEFAULT
#undef TC_DEFAULT
#undef TC_BG_DEFAULT
#undef TC_STATUS
#undef TC_STATUS_START
#undef TC_STATUS_FILE
#undef TC_PROMPT
#undef TC_NT
#undef TC_CC
#undef TC_NT_ID
#undef TC_T
#undef TC_NULL
#undef TC_RANGE
#undef TC_NEG

#endif // __IDNI__PARSER__PARSER_TERM_COLOR_MACROS_H__