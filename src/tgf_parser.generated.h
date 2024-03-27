// This file is generated from a file src/tgf.tgf by
//       https://github.com/IDNI/parser/tools/tgf
//
#ifndef __TGF_PARSER_H__
#define __TGF_PARSER_H__

#include <string.h>

#include "parser.h"

struct tgf_parser {
	using char_type       = char;
	using terminal_type   = char;
	using traits_type     = std::char_traits<char_type>;
	using int_type        = typename traits_type::int_type;
	using grammar_type    = idni::grammar<char_type, terminal_type>;
	using grammar_options = grammar_type::options;
	using symbol_type     = idni::lit<char_type, terminal_type>;
	using location_type   = std::array<size_t, 2>;
	using node_type       = std::pair<symbol_type, location_type>;
	using parser_type     = idni::parser<char_type, terminal_type>;
	using options         = parser_type::options;
	using parse_options   = parser_type::parse_options;
	using forest_type     = parser_type::pforest;
	using input_type      = parser_type::input;
	using decoder_type    = parser_type::input::decoder_type;
	using encoder_type    = std::function<std::basic_string<char_type>(
			const std::vector<terminal_type>&)>;
	tgf_parser() :
		nts(load_nonterminals()), cc(load_cc()),
		g(nts, load_prods(), nt(6), cc, load_grammar_opts()),
		p(g, load_opts()) {}
	std::unique_ptr<forest_type> parse(const char_type* data, size_t size,
		parse_options po = {}) { return p.parse(data, size, po); }
	std::unique_ptr<forest_type> parse(std::basic_istream<char_type>& is,
		parse_options po = {}) { return p.parse(is, po); }
	std::unique_ptr<forest_type> parse(const std::string& fn,
		parse_options po = {}) { return p.parse(fn, po); }
#ifndef WIN32
	std::unique_ptr<forest_type> parse(int fd, parse_options po = {})
		{ return p.parse(fd, po); }
#endif //WIN32
	bool found(int start = -1) { return p.found(start); }
	typename parser_type::error get_error() { return p.get_error(); }
	enum nonterminal {
		nul, alnum, alpha, eof, printable, space, start, _, statement, _Rstart_0, 
		_Rstart_1, directive, production, start_statement, use_directive, boolean_directive, nodisambig_directive, _Rdirective_2, __, use_from, 
		use_param, _Ruse_directive_3, _Ruse_directive_4, _Ruse_directive_5, _Ruse_directive_6, use_from_char_class, char_class_name, boolean_action, _Rboolean_directive_7, boolean_directive_name, 
		disambiguation_sym, enable_sym, disable_sym, _Rdisambiguation_sym_8, _Rdisambiguation_sym_9, _Rdisambiguation_sym_10, nodisambig_sym, sym, _Rnodisambig_directive_11, _Rnodisambig_directive_12, 
		n, _Rnodisambig_sym_13, _Rnodisambig_sym_14, _Rnodisambig_sym_15, _Rnodisambig_sym_16, _Rnodisambig_sym_17, _Rnodisambig_sym_18, _Rnodisambig_sym_19, expr1, disjunction, 
		expr2, conjunction, expr3, negation, literals, literal, _Rliterals_20, _Rliterals_21, terminal, nonterminal_, 
		terminal_char, terminal_string, nonliteral, sym_start, sym_rest, _Rsym_22, _Rsym_23, group, optional, repeat, 
		plus, multi, unescaped_c, escaped_c, _Rterminal_char_24, _Runescaped_c_25, _Rescaped_c_26, unescaped_s, escaped_s, _Rterminal_string_27, 
		_Rterminal_string_28, _Runescaped_s_29, _Rescaped_s_30, _R__31, comment, _R___32, _Rcomment_33, _Rcomment_34, _Rcomment_35, __neg_0, 
		__neg_1, 
	};
	size_t id(const std::basic_string<char_type>& name) {
		return nts.get(name);
	}
	const std::basic_string<char_type>& name(size_t id) {
		return nts.get(id);
	}
private:
	std::vector<terminal_type> ts{
		'\0', '@', ',', 'u', 's', 'e', '.', '_', 'c', 
		'h', 'a', 'r', 'l', 'f', 'n', 'm', 'p', 'b', 'k', 
		't', 'd', 'i', 'g', 'o', 'w', 'x', '-', '=', '>', 
		'|', '&', '~', '(', ')', '[', ']', '{', '}', '+', 
		'*', '\'', '\\', '/', '"', '\t', '\n', '\r', '#', 
	};
	idni::nonterminals<char_type, terminal_type> nts{};
	idni::char_class_fns<terminal_type> cc;
	idni::grammar<char_type, terminal_type> g;
	parser_type p;
	idni::prods<char_type, terminal_type> t(size_t tid) {
		return idni::prods<char_type, terminal_type>(ts[tid]);
	}
	idni::prods<char_type, terminal_type> nt(size_t ntid) {
		return idni::prods<char_type, terminal_type>(
			symbol_type(ntid, &nts));
	}
	idni::nonterminals<char_type, terminal_type> load_nonterminals() const {
		idni::nonterminals<char_type, terminal_type> nts{};
		for (const auto& nt : {
			"", "alnum", "alpha", "eof", "printable", "space", "start", "_", "statement", "_Rstart_0", 
			"_Rstart_1", "directive", "production", "start_statement", "use_directive", "boolean_directive", "nodisambig_directive", "_Rdirective_2", "__", "use_from", 
			"use_param", "_Ruse_directive_3", "_Ruse_directive_4", "_Ruse_directive_5", "_Ruse_directive_6", "use_from_char_class", "char_class_name", "boolean_action", "_Rboolean_directive_7", "boolean_directive_name", 
			"disambiguation_sym", "enable_sym", "disable_sym", "_Rdisambiguation_sym_8", "_Rdisambiguation_sym_9", "_Rdisambiguation_sym_10", "nodisambig_sym", "sym", "_Rnodisambig_directive_11", "_Rnodisambig_directive_12", 
			"n", "_Rnodisambig_sym_13", "_Rnodisambig_sym_14", "_Rnodisambig_sym_15", "_Rnodisambig_sym_16", "_Rnodisambig_sym_17", "_Rnodisambig_sym_18", "_Rnodisambig_sym_19", "expr1", "disjunction", 
			"expr2", "conjunction", "expr3", "negation", "literals", "literal", "_Rliterals_20", "_Rliterals_21", "terminal", "nonterminal_", 
			"terminal_char", "terminal_string", "nonliteral", "sym_start", "sym_rest", "_Rsym_22", "_Rsym_23", "group", "optional", "repeat", 
			"plus", "multi", "unescaped_c", "escaped_c", "_Rterminal_char_24", "_Runescaped_c_25", "_Rescaped_c_26", "unescaped_s", "escaped_s", "_Rterminal_string_27", 
			"_Rterminal_string_28", "_Runescaped_s_29", "_Rescaped_s_30", "_R__31", "comment", "_R___32", "_Rcomment_33", "_Rcomment_34", "_Rcomment_35", "__neg_0", 
			"__neg_1", 
		}) nts.get(nt);
		return nts;
	}
	idni::char_class_fns<terminal_type> load_cc() {
		return idni::predefined_char_classes<char_type, terminal_type>({
			"alnum",
			"alpha",
			"eof",
			"printable",
			"space",
		}, nts);
	}
	grammar_type::options load_grammar_opts() {
		grammar_type::options o;
		o.auto_disambiguate = true;
		return o;
	}
	options load_opts() {
		options o;
		return o;
	}
	idni::prods<char_type, terminal_type> load_prods() {
		idni::prods<char_type, terminal_type> q,
			nul(symbol_type{});
		//       _Rstart_0(9)         => _(7) statement(8).
		q(nt(9), (nt(7)+nt(8)));
		//       _Rstart_1(10)        => null.
		q(nt(10), (nul));
		//       _Rstart_1(10)        => _Rstart_0(9) _Rstart_1(10).
		q(nt(10), (nt(9)+nt(10)));
		//       start(6)             => _Rstart_1(10) _(7).
		q(nt(6), (nt(10)+nt(7)));
		//       statement(8)         => directive(11).
		q(nt(8), (nt(11)));
		//       statement(8)         => production(12).
		q(nt(8), (nt(12)));
		//       start_statement(13)  => _(7) statement(8) _(7).
		q(nt(13), (nt(7)+nt(8)+nt(7)));
		//       _Rdirective_2(17)    => use_directive(14).
		q(nt(17), (nt(14)));
		//       _Rdirective_2(17)    => boolean_directive(15).
		q(nt(17), (nt(15)));
		//       _Rdirective_2(17)    => nodisambig_directive(16).
		q(nt(17), (nt(16)));
		//       directive(11)        => '@' _Rdirective_2(17).
		q(nt(11), (t(1)+nt(17)));
		//       _Ruse_directive_3(21) => _(7) ',' _(7) use_param(20).
		q(nt(21), (nt(7)+t(2)+nt(7)+nt(20)));
		//       _Ruse_directive_4(22) => null.
		q(nt(22), (nul));
		//       _Ruse_directive_4(22) => _Ruse_directive_3(21) _Ruse_directive_4(22).
		q(nt(22), (nt(21)+nt(22)));
		//       use_directive(14)    => 'u' 's' 'e' __(18) use_from(19) __(18) use_param(20) _Ruse_directive_4(22) _(7) '.'.
		q(nt(14), (t(3)+t(4)+t(5)+nt(18)+nt(19)+nt(18)+nt(20)+nt(22)+nt(7)+t(6)));
		//       _Ruse_directive_5(23) => _(7) ',' _(7) use_param(20).
		q(nt(23), (nt(7)+t(2)+nt(7)+nt(20)));
		//       _Ruse_directive_6(24) => null.
		q(nt(24), (nul));
		//       _Ruse_directive_6(24) => _Ruse_directive_5(23) _Ruse_directive_6(24).
		q(nt(24), (nt(23)+nt(24)));
		//       use_directive(14)    => 'u' 's' 'e' '_' 'c' 'h' 'a' 'r' '_' 'c' 'l' 'a' 's' 's' __(18) use_param(20) _Ruse_directive_6(24) _(7) '.'.
		q(nt(14), (t(3)+t(4)+t(5)+t(7)+t(8)+t(9)+t(10)+t(11)+t(7)+t(8)+t(12)+t(10)+t(4)+t(4)+nt(18)+nt(20)+nt(24)+nt(7)+t(6)));
		//       use_from(19)         => use_from_char_class(25).
		q(nt(19), (nt(25)));
		//       use_from_char_class(25) => 'c' 'c'.
		q(nt(25), (t(8)+t(8)));
		//       use_from_char_class(25) => 'c' 'c' '_' 'f' 'n'.
		q(nt(25), (t(8)+t(8)+t(7)+t(13)+t(14)));
		//       use_from_char_class(25) => 'c' 'c' 'f' 'n'.
		q(nt(25), (t(8)+t(8)+t(13)+t(14)));
		//       use_from_char_class(25) => 'c' 'h' 'a' 'r' __(18) 'c' 'l' 'a' 's' 's'.
		q(nt(25), (t(8)+t(9)+t(10)+t(11)+nt(18)+t(8)+t(12)+t(10)+t(4)+t(4)));
		//       use_param(20)        => char_class_name(26).
		q(nt(20), (nt(26)));
		//       char_class_name(26)  => 'a' 'l' 'n' 'u' 'm'.
		q(nt(26), (t(10)+t(12)+t(14)+t(3)+t(15)));
		//       char_class_name(26)  => 'a' 'l' 'p' 'h' 'a'.
		q(nt(26), (t(10)+t(12)+t(16)+t(9)+t(10)));
		//       char_class_name(26)  => 'b' 'l' 'a' 'n' 'k'.
		q(nt(26), (t(17)+t(12)+t(10)+t(14)+t(18)));
		//       char_class_name(26)  => 'c' 'n' 't' 'r' 'l'.
		q(nt(26), (t(8)+t(14)+t(19)+t(11)+t(12)));
		//       char_class_name(26)  => 'd' 'i' 'g' 'i' 't'.
		q(nt(26), (t(20)+t(21)+t(22)+t(21)+t(19)));
		//       char_class_name(26)  => 'e' 'o' 'f'.
		q(nt(26), (t(5)+t(23)+t(13)));
		//       char_class_name(26)  => 'g' 'r' 'a' 'p' 'h'.
		q(nt(26), (t(22)+t(11)+t(10)+t(16)+t(9)));
		//       char_class_name(26)  => 'l' 'o' 'w' 'e' 'r'.
		q(nt(26), (t(12)+t(23)+t(24)+t(5)+t(11)));
		//       char_class_name(26)  => 'p' 'r' 'i' 'n' 't' 'a' 'b' 'l' 'e'.
		q(nt(26), (t(16)+t(11)+t(21)+t(14)+t(19)+t(10)+t(17)+t(12)+t(5)));
		//       char_class_name(26)  => 'p' 'u' 'n' 'c' 't'.
		q(nt(26), (t(16)+t(3)+t(14)+t(8)+t(19)));
		//       char_class_name(26)  => 's' 'p' 'a' 'c' 'e'.
		q(nt(26), (t(4)+t(16)+t(10)+t(8)+t(5)));
		//       char_class_name(26)  => 'u' 'p' 'p' 'e' 'r'.
		q(nt(26), (t(3)+t(16)+t(16)+t(5)+t(11)));
		//       char_class_name(26)  => 'x' 'd' 'i' 'g' 'i' 't'.
		q(nt(26), (t(25)+t(20)+t(21)+t(22)+t(21)+t(19)));
		//       _Rboolean_directive_7(28) => null.
		q(nt(28), (nul));
		//       _Rboolean_directive_7(28) => boolean_action(27) __(18).
		q(nt(28), (nt(27)+nt(18)));
		//       boolean_directive(15) => _Rboolean_directive_7(28) boolean_directive_name(29).
		q(nt(15), (nt(28)+nt(29)));
		//       boolean_directive_name(29) => disambiguation_sym(30).
		q(nt(29), (nt(30)));
		//       boolean_action(27)   => enable_sym(31).
		q(nt(27), (nt(31)));
		//       boolean_action(27)   => disable_sym(32).
		q(nt(27), (nt(32)));
		//       enable_sym(31)       => 'e' 'n' 'a' 'b' 'l' 'e'.
		q(nt(31), (t(5)+t(14)+t(10)+t(17)+t(12)+t(5)));
		//       disable_sym(32)      => 'd' 'i' 's' 'a' 'b' 'l' 'e'.
		q(nt(32), (t(20)+t(21)+t(4)+t(10)+t(17)+t(12)+t(5)));
		//       _Rdisambiguation_sym_8(33) => null.
		q(nt(33), (nul));
		//       _Rdisambiguation_sym_8(33) => '-'.
		q(nt(33), (t(26)));
		//       _Rdisambiguation_sym_9(34) => null.
		q(nt(34), (nul));
		//       _Rdisambiguation_sym_9(34) => _Rdisambiguation_sym_8(33).
		q(nt(34), (nt(33)));
		//       _Rdisambiguation_sym_10(35) => 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g'.
		q(nt(35), (t(20)+t(21)+t(4)+t(10)+t(15)+t(17)+t(21)+t(22)));
		//       _Rdisambiguation_sym_10(35) => 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' 'u' 'a' 't' 'e'.
		q(nt(35), (t(20)+t(21)+t(4)+t(10)+t(15)+t(17)+t(21)+t(22)+t(3)+t(10)+t(19)+t(5)));
		//       _Rdisambiguation_sym_10(35) => 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' 'u' 'a' 't' 'i' 'o' 'n'.
		q(nt(35), (t(20)+t(21)+t(4)+t(10)+t(15)+t(17)+t(21)+t(22)+t(3)+t(10)+t(19)+t(21)+t(23)+t(14)));
		//       disambiguation_sym(30) => 'a' 'u' 't' 'o' _Rdisambiguation_sym_9(34) _Rdisambiguation_sym_10(35).
		q(nt(30), (t(10)+t(3)+t(19)+t(23)+nt(34)+nt(35)));
		//       _Rnodisambig_directive_11(38) => _(7) ',' _(7) sym(37).
		q(nt(38), (nt(7)+t(2)+nt(7)+nt(37)));
		//       _Rnodisambig_directive_12(39) => null.
		q(nt(39), (nul));
		//       _Rnodisambig_directive_12(39) => _Rnodisambig_directive_11(38) _Rnodisambig_directive_12(39).
		q(nt(39), (nt(38)+nt(39)));
		//       nodisambig_directive(16) => nodisambig_sym(36) _(7) sym(37) _Rnodisambig_directive_12(39) '.'.
		q(nt(16), (nt(36)+nt(7)+nt(37)+nt(39)+t(6)));
		//       _Rnodisambig_sym_13(41) => null.
		q(nt(41), (nul));
		//       _Rnodisambig_sym_13(41) => n(40).
		q(nt(41), (nt(40)));
		//       _Rnodisambig_sym_14(42) => 'd' 'o' 'n' 't'.
		q(nt(42), (t(20)+t(23)+t(14)+t(19)));
		//       _Rnodisambig_sym_14(42) => _Rnodisambig_sym_13(41).
		q(nt(42), (nt(41)));
		//       _Rnodisambig_sym_15(43) => null.
		q(nt(43), (nul));
		//       _Rnodisambig_sym_15(43) => '-'.
		q(nt(43), (t(26)));
		//       _Rnodisambig_sym_15(43) => '_'.
		q(nt(43), (t(7)));
		//       _Rnodisambig_sym_16(44) => null.
		q(nt(44), (nul));
		//       _Rnodisambig_sym_16(44) => 'u' 'a' 't' 'e'.
		q(nt(44), (t(3)+t(10)+t(19)+t(5)));
		//       _Rnodisambig_sym_17(45) => null.
		q(nt(45), (nul));
		//       _Rnodisambig_sym_17(45) => _Rnodisambig_sym_16(44).
		q(nt(45), (nt(44)));
		//       _Rnodisambig_sym_18(46) => '-'.
		q(nt(46), (t(26)));
		//       _Rnodisambig_sym_18(46) => '_'.
		q(nt(46), (t(7)));
		//       _Rnodisambig_sym_19(47) => null.
		q(nt(47), (nul));
		//       _Rnodisambig_sym_19(47) => _Rnodisambig_sym_18(46) 'l' 'i' 's' 't'.
		q(nt(47), (nt(46)+t(12)+t(21)+t(4)+t(19)));
		//       nodisambig_sym(36)   => 'n' 'o' _Rnodisambig_sym_14(42) _Rnodisambig_sym_15(43) 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' _Rnodisambig_sym_17(45) _Rnodisambig_sym_19(47).
		q(nt(36), (t(14)+t(23)+nt(42)+nt(43)+t(20)+t(21)+t(4)+t(10)+t(15)+t(17)+t(21)+t(22)+nt(45)+nt(47)));
		//       production(12)       => sym(37) _(7) '=' '>' _(7) expr1(48) _(7) '.'.
		q(nt(12), (nt(37)+nt(7)+t(27)+t(28)+nt(7)+nt(48)+nt(7)+t(6)));
		//       expr1(48)            => disjunction(49).
		q(nt(48), (nt(49)));
		//       expr1(48)            => expr2(50).
		q(nt(48), (nt(50)));
		//       expr2(50)            => conjunction(51).
		q(nt(50), (nt(51)));
		//       expr2(50)            => expr3(52).
		q(nt(50), (nt(52)));
		//       expr3(52)            => negation(53).
		q(nt(52), (nt(53)));
		//       expr3(52)            => literals(54).
		q(nt(52), (nt(54)));
		//       disjunction(49)      => expr1(48) _(7) '|' _(7) expr2(50).
		q(nt(49), (nt(48)+nt(7)+t(29)+nt(7)+nt(50)));
		//       conjunction(51)      => expr2(50) _(7) '&' _(7) expr3(52).
		q(nt(51), (nt(50)+nt(7)+t(30)+nt(7)+nt(52)));
		//       negation(53)         => '~' _(7) expr3(52).
		q(nt(53), (t(31)+nt(7)+nt(52)));
		//       _Rliterals_20(56)    => __(18) literal(55).
		q(nt(56), (nt(18)+nt(55)));
		//       _Rliterals_21(57)    => null.
		q(nt(57), (nul));
		//       _Rliterals_21(57)    => _Rliterals_20(56) _Rliterals_21(57).
		q(nt(57), (nt(56)+nt(57)));
		//       literals(54)         => literal(55) _Rliterals_21(57).
		q(nt(54), (nt(55)+nt(57)));
		//       literal(55)          => terminal(58).
		q(nt(55), (nt(58)));
		//       literal(55)          => nonterminal_(59).
		q(nt(55), (nt(59)));
		//       terminal(58)         => terminal_char(60).
		q(nt(58), (nt(60)));
		//       terminal(58)         => terminal_string(61).
		q(nt(58), (nt(61)));
		//       nonterminal_(59)     => sym(37).
		q(nt(59), (nt(37)));
		//       nonterminal_(59)     => nonliteral(62).
		q(nt(59), (nt(62)));
		//       _Rsym_22(65)         => sym_rest(64).
		q(nt(65), (nt(64)));
		//       _Rsym_23(66)         => null.
		q(nt(66), (nul));
		//       _Rsym_23(66)         => _Rsym_22(65) _Rsym_23(66).
		q(nt(66), (nt(65)+nt(66)));
		//       sym(37)              => sym_start(63) _Rsym_23(66).
		q(nt(37), (nt(63)+nt(66)));
		//       sym_start(63)        => '_'.
		q(nt(63), (t(7)));
		//       sym_start(63)        => alpha(2).
		q(nt(63), (nt(2)));
		//       sym_rest(64)         => '_'.
		q(nt(64), (t(7)));
		//       sym_rest(64)         => alnum(1).
		q(nt(64), (nt(1)));
		//       nonliteral(62)       => group(67).
		q(nt(62), (nt(67)));
		//       nonliteral(62)       => optional(68).
		q(nt(62), (nt(68)));
		//       nonliteral(62)       => repeat(69).
		q(nt(62), (nt(69)));
		//       nonliteral(62)       => plus(70).
		q(nt(62), (nt(70)));
		//       nonliteral(62)       => multi(71).
		q(nt(62), (nt(71)));
		//       group(67)            => '(' _(7) expr1(48) _(7) ')'.
		q(nt(67), (t(32)+nt(7)+nt(48)+nt(7)+t(33)));
		//       optional(68)         => '[' _(7) expr1(48) _(7) ']'.
		q(nt(68), (t(34)+nt(7)+nt(48)+nt(7)+t(35)));
		//       repeat(69)           => '{' _(7) expr1(48) _(7) '}'.
		q(nt(69), (t(36)+nt(7)+nt(48)+nt(7)+t(37)));
		//       plus(70)             => literal(55) _(7) '+'.
		q(nt(70), (nt(55)+nt(7)+t(38)));
		//       multi(71)            => literal(55) _(7) '*'.
		q(nt(71), (nt(55)+nt(7)+t(39)));
		//       _Rterminal_char_24(74) => unescaped_c(72).
		q(nt(74), (nt(72)));
		//       _Rterminal_char_24(74) => escaped_c(73).
		q(nt(74), (nt(73)));
		//       terminal_char(60)    => '\'' _Rterminal_char_24(74) '\''.
		q(nt(60), (t(40)+nt(74)+t(40)));
		//       _Runescaped_c_25(75) => '\''.
		q(nt(75), (t(40)));
		//       _Runescaped_c_25(75) => '\\'.
		q(nt(75), (t(41)));
		//       __neg_0(89)          => _Runescaped_c_25(75).
		q(nt(89), (nt(75)));
		//       unescaped_c(72)      => printable(4) & ~( __neg_0(89) ).	 # conjunctive
		q(nt(72), (nt(4)) & ~(nt(89)));
		//       _Rescaped_c_26(76)   => '\''.
		q(nt(76), (t(40)));
		//       _Rescaped_c_26(76)   => '/'.
		q(nt(76), (t(42)));
		//       _Rescaped_c_26(76)   => '\\'.
		q(nt(76), (t(41)));
		//       _Rescaped_c_26(76)   => 'b'.
		q(nt(76), (t(17)));
		//       _Rescaped_c_26(76)   => 'f'.
		q(nt(76), (t(13)));
		//       _Rescaped_c_26(76)   => 'n'.
		q(nt(76), (t(14)));
		//       _Rescaped_c_26(76)   => 'r'.
		q(nt(76), (t(11)));
		//       _Rescaped_c_26(76)   => 't'.
		q(nt(76), (t(19)));
		//       escaped_c(73)        => '\\' _Rescaped_c_26(76).
		q(nt(73), (t(41)+nt(76)));
		//       _Rterminal_string_27(79) => unescaped_s(77).
		q(nt(79), (nt(77)));
		//       _Rterminal_string_27(79) => escaped_s(78).
		q(nt(79), (nt(78)));
		//       _Rterminal_string_28(80) => null.
		q(nt(80), (nul));
		//       _Rterminal_string_28(80) => _Rterminal_string_27(79) _Rterminal_string_28(80).
		q(nt(80), (nt(79)+nt(80)));
		//       terminal_string(61)  => '"' _Rterminal_string_28(80) '"'.
		q(nt(61), (t(43)+nt(80)+t(43)));
		//       _Runescaped_s_29(81) => '"'.
		q(nt(81), (t(43)));
		//       _Runescaped_s_29(81) => '\\'.
		q(nt(81), (t(41)));
		//       __neg_1(90)          => _Runescaped_s_29(81).
		q(nt(90), (nt(81)));
		//       unescaped_s(77)      => printable(4) & ~( __neg_1(90) ).	 # conjunctive
		q(nt(77), (nt(4)) & ~(nt(90)));
		//       _Rescaped_s_30(82)   => '"'.
		q(nt(82), (t(43)));
		//       _Rescaped_s_30(82)   => '/'.
		q(nt(82), (t(42)));
		//       _Rescaped_s_30(82)   => '\\'.
		q(nt(82), (t(41)));
		//       _Rescaped_s_30(82)   => 'b'.
		q(nt(82), (t(17)));
		//       _Rescaped_s_30(82)   => 'f'.
		q(nt(82), (t(13)));
		//       _Rescaped_s_30(82)   => 'n'.
		q(nt(82), (t(14)));
		//       _Rescaped_s_30(82)   => 'r'.
		q(nt(82), (t(11)));
		//       _Rescaped_s_30(82)   => 't'.
		q(nt(82), (t(19)));
		//       escaped_s(78)        => '\\' _Rescaped_s_30(82).
		q(nt(78), (t(41)+nt(82)));
		//       _R__31(83)           => null.
		q(nt(83), (nul));
		//       _R__31(83)           => __(18).
		q(nt(83), (nt(18)));
		//       _(7)                 => _R__31(83).
		q(nt(7), (nt(83)));
		//       _R___32(85)          => space(5).
		q(nt(85), (nt(5)));
		//       _R___32(85)          => comment(84).
		q(nt(85), (nt(84)));
		//       __(18)               => _R___32(85) _(7).
		q(nt(18), (nt(85)+nt(7)));
		//       _Rcomment_33(86)     => '\t'.
		q(nt(86), (t(44)));
		//       _Rcomment_33(86)     => printable(4).
		q(nt(86), (nt(4)));
		//       _Rcomment_34(87)     => null.
		q(nt(87), (nul));
		//       _Rcomment_34(87)     => _Rcomment_33(86) _Rcomment_34(87).
		q(nt(87), (nt(86)+nt(87)));
		//       _Rcomment_35(88)     => '\n'.
		q(nt(88), (t(45)));
		//       _Rcomment_35(88)     => '\r'.
		q(nt(88), (t(46)));
		//       _Rcomment_35(88)     => eof(3).
		q(nt(88), (nt(3)));
		//       comment(84)          => '#' _Rcomment_34(87) _Rcomment_35(88).
		q(nt(84), (t(47)+nt(87)+nt(88)));
		return q;
	}
};

#endif // __TGF_PARSER_H__
