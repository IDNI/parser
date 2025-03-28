// This file is generated from a file examples/csv_parser10/csv.tgf by
//       https://github.com/IDNI/parser/tools/tgf
//
#ifndef __CSV_PARSER_H__
#define __CSV_PARSER_H__

#include "parser.h"

namespace csv_parser_data {

using char_type     = char;
using terminal_type = char;

inline static constexpr size_t nt_bits = 5;
inline const std::vector<std::string> symbol_names{
	"", "digit", "printable", "integer", "__E_integer_0", "__E_integer_1", "quote", "esc", "escaping", "unescaped", 
	"escaped", "strchar", "str", "__E_str_2", "nullvalue", "val", "eol", "__E_eol_3", "row", "__E_row_4", 
	"__E_row_5", "rows", "__E_rows_6", "__E_rows_7", "start", "__N_0", 
};

inline ::idni::nonterminals<char_type, terminal_type> nts{symbol_names};

inline std::vector<terminal_type> terminals{
	'\0', '-', '"', '\\', '\r', '\n', ',', 
};

inline ::idni::char_class_fns<terminal_type> char_classes =
	::idni::predefined_char_classes<char_type, terminal_type>({
		"digit",
		"printable",
	}, nts);

inline struct ::idni::grammar<char_type, terminal_type>::options
	grammar_options
{
	.transform_negation = false,
	.auto_disambiguate = true,
	.shaping = {
		.trim_terminals = false,
		.inline_char_classes = false
	}
};

inline ::idni::parser<char_type, terminal_type>::options parser_options{
};

inline ::idni::prods<char_type, terminal_type> start_symbol{ nts(24) };

inline idni::prods<char_type, terminal_type>& productions() {
	static bool loaded = false;
	static idni::prods<char_type, terminal_type>
		p, nul(idni::lit<char_type, terminal_type>{});
	if (loaded) return p;
	#define  T(x) (idni::prods<char_type, terminal_type>{ terminals[x] })
	#define NT(x) (idni::prods<char_type, terminal_type>{ nts(x) })
//G0:   __E_integer_0(4)     => '-'.
	p(NT(4), (T(1)));
//G1:   __E_integer_0(4)     => null.
	p(NT(4), (nul));
//G2:   __E_integer_1(5)     => digit(1).
	p(NT(5), (NT(1)));
//G3:   __E_integer_1(5)     => digit(1) __E_integer_1(5).
	p(NT(5), (NT(1)+NT(5)));
//G4:   integer(3)           => __E_integer_0(4) __E_integer_1(5).
	p(NT(3), (NT(4)+NT(5)));
//G5:   quote(6)             => '"'.
	p(NT(6), (T(2)));
//G6:   esc(7)               => '\\'.
	p(NT(7), (T(3)));
//G7:   escaping(8)          => quote(6).
	p(NT(8), (NT(6)));
//G8:   escaping(8)          => esc(7).
	p(NT(8), (NT(7)));
//G9:   __N_0(25)            => escaping(8).
	p(NT(25), (NT(8)));
//G10:  unescaped(9)         => printable(2) & ~( __N_0(25) ).	 # conjunctive
	p(NT(9), (NT(2)) & ~(NT(25)));
//G11:  escaped(10)          => esc(7) escaping(8).
	p(NT(10), (NT(7)+NT(8)));
//G12:  strchar(11)          => unescaped(9).
	p(NT(11), (NT(9)));
//G13:  strchar(11)          => escaped(10).
	p(NT(11), (NT(10)));
//G14:  __E_str_2(13)        => null.
	p(NT(13), (nul));
//G15:  __E_str_2(13)        => strchar(11) __E_str_2(13).
	p(NT(13), (NT(11)+NT(13)));
//G16:  str(12)              => quote(6) __E_str_2(13) quote(6).
	p(NT(12), (NT(6)+NT(13)+NT(6)));
//G17:  nullvalue(14)        => null.
	p(NT(14), (nul));
//G18:  val(15)              => integer(3).
	p(NT(15), (NT(3)));
//G19:  val(15)              => str(12).
	p(NT(15), (NT(12)));
//G20:  val(15)              => nullvalue(14).
	p(NT(15), (NT(14)));
//G21:  __E_eol_3(17)        => '\r'.
	p(NT(17), (T(4)));
//G22:  __E_eol_3(17)        => null.
	p(NT(17), (nul));
//G23:  eol(16)              => __E_eol_3(17) '\n'.
	p(NT(16), (NT(17)+T(5)));
//G24:  __E_row_4(19)        => ',' val(15).
	p(NT(19), (T(6)+NT(15)));
//G25:  __E_row_5(20)        => null.
	p(NT(20), (nul));
//G26:  __E_row_5(20)        => __E_row_4(19) __E_row_5(20).
	p(NT(20), (NT(19)+NT(20)));
//G27:  row(18)              => val(15) __E_row_5(20).
	p(NT(18), (NT(15)+NT(20)));
//G28:  __E_rows_6(22)       => eol(16) row(18).
	p(NT(22), (NT(16)+NT(18)));
//G29:  __E_rows_7(23)       => null.
	p(NT(23), (nul));
//G30:  __E_rows_7(23)       => __E_rows_6(22) __E_rows_7(23).
	p(NT(23), (NT(22)+NT(23)));
//G31:  rows(21)             => row(18) __E_rows_7(23).
	p(NT(21), (NT(18)+NT(23)));
//G32:  start(24)            => rows(21).
	p(NT(24), (NT(21)));
	#undef T
	#undef NT
	return loaded = true, p;
}

inline ::idni::grammar<char_type, terminal_type> grammar(
	nts, productions(), start_symbol, char_classes, grammar_options);

} // namespace csv_parser_data

struct csv_parser_nonterminals {
	enum nonterminal {
		nul, digit, printable, integer, __E_integer_0, __E_integer_1, quote, esc, escaping, unescaped, 
		escaped, strchar, str, __E_str_2, nullvalue, val, eol, __E_eol_3, row, __E_row_4, 
		__E_row_5, rows, __E_rows_6, __E_rows_7, start, __N_0, 
	};
};

struct csv_parser : public idni::parser<char, char>, public csv_parser_nonterminals {
	static csv_parser& instance() {
		static csv_parser inst;
		return inst;
	}
	csv_parser() : idni::parser<char_type, terminal_type>(
		csv_parser_data::grammar,
		csv_parser_data::parser_options) {}
	size_t id(const std::basic_string<char_type>& name) {
		return csv_parser_data::nts.get(name);
	}
	const std::basic_string<char_type>& name(size_t id) {
		return csv_parser_data::nts.get(id);
	}
	symbol_type literal(const nonterminal& nt) {
		return symbol_type(nt, &csv_parser_data::nts);
	}
};

#endif // __CSV_PARSER_H__
