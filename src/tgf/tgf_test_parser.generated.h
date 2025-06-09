// This file is generated from a file src/tgf/tgf_test.tgf by
//       https://github.com/IDNI/parser/src/tgf
//
#ifndef __TGF_TEST_PARSER_H__
#define __TGF_TEST_PARSER_H__

#include "parser.h"

namespace tgf_test_parser_data {

using char_type     = char;
using terminal_type = char;

inline std::vector<std::string> symbol_names{
	"", "eof", "alpha", "alnum", "space", "graph", "printable", "__", "_", "symbol", 
	"string", "unescaped_s", "escaped_s", "start", "__E_start_0", "test", "__E_start_1", "test_string", "__E_test_2", "__E_test_3", 
	"__E_symbol_4", "__E_symbol_5", "__E_symbol_6", "quoted_string", "__E_string_7", "quoted_string_char", "__E_quoted_string_8", "__E_unescaped_s_9", "__E_unescaped_s_10", "__E_escaped_s_11", 
	"__E___12", "__E____13", "comment", "__E_comment_14", "__E_comment_15", "__E_comment_16", "__N_0", 
};

inline ::idni::nonterminals<char_type, terminal_type> nts{symbol_names};

inline std::vector<terminal_type> terminals{
	'\0', '.', ',', ':', '_', '"', '\\', '/', 'b', 
	'f', 'n', 'r', 't', '\t', '\r', '\n', '#', 
};

inline ::idni::char_class_fns<terminal_type> char_classes =
	::idni::predefined_char_classes<char_type, terminal_type>({
		"eof",
		"alpha",
		"alnum",
		"space",
		"graph",
		"printable",
	}, nts);

inline struct ::idni::grammar<char_type, terminal_type>::options
	grammar_options
{
	.transform_negation = false,
	.auto_disambiguate = true,
	.shaping = {
		.to_trim = {
			7, 8
		},
		.trim_terminals = true,
		.dont_trim_terminals_of = {
			9, 10, 11, 12
		},
		.inline_char_classes = true
	}
};

inline ::idni::parser<char_type, terminal_type>::options parser_options{
};

inline ::idni::prods<char_type, terminal_type> start_symbol{ nts(13) };

inline idni::prods<char_type, terminal_type>& productions() {
	static bool loaded = false;
	static idni::prods<char_type, terminal_type>
		p, nul(idni::lit<char_type, terminal_type>{});
	if (loaded) return p;
	#define  T(x) (idni::prods<char_type, terminal_type>{ terminals[x] })
	#define NT(x) (idni::prods<char_type, terminal_type>{ nts(x) })
//G0:   __E_start_0(14)      => test(15) _(8) '.' _(8).
	p(NT(14), (NT(15)+NT(8)+T(1)+NT(8)));
//G1:   __E_start_1(16)      => null.
	p(NT(16), (nul));
//G2:   __E_start_1(16)      => __E_start_0(14) __E_start_1(16).
	p(NT(16), (NT(14)+NT(16)));
//G3:   start(13)            => _(8) __E_start_1(16).
	p(NT(13), (NT(8)+NT(16)));
//G4:   __E_test_2(18)       => _(8) ',' _(8) test_string(17).
	p(NT(18), (NT(8)+T(2)+NT(8)+NT(17)));
//G5:   __E_test_3(19)       => null.
	p(NT(19), (nul));
//G6:   __E_test_3(19)       => __E_test_2(18) __E_test_3(19).
	p(NT(19), (NT(18)+NT(19)));
//G7:   test(15)             => symbol(9) _(8) ':' _(8) test_string(17) __E_test_3(19).
	p(NT(15), (NT(9)+NT(8)+T(3)+NT(8)+NT(17)+NT(19)));
//G8:   __E_symbol_4(20)     => alpha(2).
	p(NT(20), (NT(2)));
//G9:   __E_symbol_4(20)     => '_'.
	p(NT(20), (T(4)));
//G10:  __E_symbol_5(21)     => alnum(3).
	p(NT(21), (NT(3)));
//G11:  __E_symbol_5(21)     => '_'.
	p(NT(21), (T(4)));
//G12:  __E_symbol_6(22)     => null.
	p(NT(22), (nul));
//G13:  __E_symbol_6(22)     => __E_symbol_5(21) __E_symbol_6(22).
	p(NT(22), (NT(21)+NT(22)));
//G14:  symbol(9)            => __E_symbol_4(20) __E_symbol_6(22).
	p(NT(9), (NT(20)+NT(22)));
//G15:  test_string(17)      => quoted_string(23).
	p(NT(17), (NT(23)));
//G16:  test_string(17)      => string(10).
	p(NT(17), (NT(10)));
//G17:  __E_string_7(24)     => graph(5).
	p(NT(24), (NT(5)));
//G18:  __E_string_7(24)     => graph(5) __E_string_7(24).
	p(NT(24), (NT(5)+NT(24)));
//G19:  string(10)           => __E_string_7(24).
	p(NT(10), (NT(24)));
//G20:  __E_quoted_string_8(26) => null.
	p(NT(26), (nul));
//G21:  __E_quoted_string_8(26) => quoted_string_char(25) __E_quoted_string_8(26).
	p(NT(26), (NT(25)+NT(26)));
//G22:  quoted_string(23)    => '"' __E_quoted_string_8(26) '"'.
	p(NT(23), (T(5)+NT(26)+T(5)));
//G23:  quoted_string_char(25) => unescaped_s(11).
	p(NT(25), (NT(11)));
//G24:  quoted_string_char(25) => escaped_s(12).
	p(NT(25), (NT(12)));
//G25:  __E_unescaped_s_9(27) => space(4).
	p(NT(27), (NT(4)));
//G26:  __E_unescaped_s_9(27) => printable(6).
	p(NT(27), (NT(6)));
//G27:  __E_unescaped_s_10(28) => '"'.
	p(NT(28), (T(5)));
//G28:  __E_unescaped_s_10(28) => '\\'.
	p(NT(28), (T(6)));
//G29:  __N_0(36)            => __E_unescaped_s_10(28).
	p(NT(36), (NT(28)));
//G30:  unescaped_s(11)      => __E_unescaped_s_9(27) & ~( __N_0(36) ).	 # conjunctive
	p(NT(11), (NT(27)) & ~(NT(36)));
//G31:  __E_escaped_s_11(29) => '"'.
	p(NT(29), (T(5)));
//G32:  __E_escaped_s_11(29) => '\\'.
	p(NT(29), (T(6)));
//G33:  __E_escaped_s_11(29) => '/'.
	p(NT(29), (T(7)));
//G34:  __E_escaped_s_11(29) => 'b'.
	p(NT(29), (T(8)));
//G35:  __E_escaped_s_11(29) => 'f'.
	p(NT(29), (T(9)));
//G36:  __E_escaped_s_11(29) => 'n'.
	p(NT(29), (T(10)));
//G37:  __E_escaped_s_11(29) => 'r'.
	p(NT(29), (T(11)));
//G38:  __E_escaped_s_11(29) => 't'.
	p(NT(29), (T(12)));
//G39:  escaped_s(12)        => '\\' __E_escaped_s_11(29).
	p(NT(12), (T(6)+NT(29)));
//G40:  __E___12(30)         => __(7).
	p(NT(30), (NT(7)));
//G41:  __E___12(30)         => null.
	p(NT(30), (nul));
//G42:  _(8)                 => __E___12(30).
	p(NT(8), (NT(30)));
//G43:  __E____13(31)        => space(4).
	p(NT(31), (NT(4)));
//G44:  __E____13(31)        => comment(32).
	p(NT(31), (NT(32)));
//G45:  __(7)                => __E____13(31) _(8).
	p(NT(7), (NT(31)+NT(8)));
//G46:  __E_comment_14(33)   => printable(6).
	p(NT(33), (NT(6)));
//G47:  __E_comment_14(33)   => '\t'.
	p(NT(33), (T(13)));
//G48:  __E_comment_15(34)   => null.
	p(NT(34), (nul));
//G49:  __E_comment_15(34)   => __E_comment_14(33) __E_comment_15(34).
	p(NT(34), (NT(33)+NT(34)));
//G50:  __E_comment_16(35)   => '\r'.
	p(NT(35), (T(14)));
//G51:  __E_comment_16(35)   => '\n'.
	p(NT(35), (T(15)));
//G52:  __E_comment_16(35)   => eof(1).
	p(NT(35), (NT(1)));
//G53:  comment(32)          => '#' __E_comment_15(34) __E_comment_16(35).
	p(NT(32), (T(16)+NT(34)+NT(35)));
	#undef T
	#undef NT
	return loaded = true, p;
}

inline ::idni::grammar<char_type, terminal_type> grammar(
	nts, productions(), start_symbol, char_classes, grammar_options);

} // namespace tgf_test_parser_data

struct tgf_test_parser : public idni::parser<char, char> {
	enum nonterminal {
		nul, eof, alpha, alnum, space, graph, printable, __, _, symbol, 
		string, unescaped_s, escaped_s, start, __E_start_0, test, __E_start_1, test_string, __E_test_2, __E_test_3, 
		__E_symbol_4, __E_symbol_5, __E_symbol_6, quoted_string, __E_string_7, quoted_string_char, __E_quoted_string_8, __E_unescaped_s_9, __E_unescaped_s_10, __E_escaped_s_11, 
		__E___12, __E____13, comment, __E_comment_14, __E_comment_15, __E_comment_16, __N_0, 
	};
	static tgf_test_parser& instance() {
		static tgf_test_parser inst;
		return inst;
	}
	tgf_test_parser() : idni::parser<char_type, terminal_type>(
		tgf_test_parser_data::grammar,
		tgf_test_parser_data::parser_options) {}
	size_t id(const std::basic_string<char_type>& name) {
		return tgf_test_parser_data::nts.get(name);
	}
	const std::basic_string<char_type>& name(size_t id) {
		return tgf_test_parser_data::nts.get(id);
	}
	symbol_type literal(const nonterminal& nt) {
		return symbol_type(nt, &tgf_test_parser_data::nts);
	}
};

#endif // __TGF_TEST_PARSER_H__
