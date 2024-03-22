// This file is generated from a file src/tgf.tgf by
//       https://github.com/IDNI/parser/tools/tgf
//
#ifndef __TGF_PARSER_H__
#define __TGF_PARSER_H__

#include <string.h>
#include <ranges>

#include "parser.h"
#include "rewriting.h"

namespace tgf_parsing {

struct tgf_parser {
	using char_type     = char;
	using terminal_type = char;
	using traits_type   = std::char_traits<char_type>;
	using int_type      = typename traits_type::int_type;
	using symbol_type   = idni::lit<char_type, terminal_type>;
	using location_type = std::array<size_t, 2>;
	using node_type     = std::pair<symbol_type, location_type>;
	using parser_type   = idni::parser<char_type, terminal_type>;
	using options       = parser_type::options;
	using parse_options = parser_type::parse_options;
	using forest_type   = parser_type::pforest;
	using input_type    = parser_type::input;
	using decoder_type  = parser_type::input::decoder_type;
	using encoder_type  = std::function<std::basic_string<char_type>(
			const std::vector<terminal_type>&)>;
	using rw_symbol_type  = std::variant<symbol_type, size_t>;
	using rw_node_type    = idni::rewriter::node<rw_symbol_type>;
	using sp_rw_node_type = idni::rewriter::sp_node<rw_symbol_type>;
	tgf_parser() :
		nts(load_nonterminals()), cc(load_cc()),
		g(nts, load_prods(), nt(6), cc), p(g, load_opts()) {}
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
		nul, eof, alnum, alpha, space, printable, start, _, statement, _Rstart_0, 
		_Rstart_1, directive, production, __, directive_params, directive_param, _Rdirective_params_2, _Rdirective_params_3, sym, expr1, 
		disjunction, expr2, conjunction, expr3, negation, literals, literal, _Rliterals_4, _Rliterals_5, terminal, 
		nonterminal_, terminal_char, terminal_string, nonliteral, sym_start, sym_rest, _Rsym_6, _Rsym_7, group, optional, 
		repeat, plus, multi, unescaped_c, escaped_c, _Rterminal_char_8, _Runescaped_c_9, _Rescaped_c_10, unescaped_s, escaped_s, 
		_Rterminal_string_11, _Rterminal_string_12, _Runescaped_s_13, _Rescaped_s_14, _R__15, comment, _R___16, _Rcomment_17, _Rcomment_18, _Rcomment_19, 
		__neg_0, __neg_1, 
	};
	size_t id(const std::basic_string<char_type>& name) {
		return nts.get(name);
	}
	const std::basic_string<char_type>& name(size_t id) {
		return nts.get(id);
	}
private:
	std::vector<terminal_type> ts{
		'\0', '@', 'u', 's', 'e', '_', 'c', 'h', 'a', 
		'r', 'l', '.', ',', 'n', 'm', 'p', 'b', 'k', 't', 
		'd', 'i', 'g', 'o', 'f', 'w', 'x', '=', '>', '|', 
		'&', '~', '(', ')', '[', ']', '{', '}', '+', '*', 
		'\'', '\\', '/', '"', '\t', '\n', '\r', '#', 
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
			"", "eof", "alnum", "alpha", "space", "printable", "start", "_", "statement", "_Rstart_0", 
			"_Rstart_1", "directive", "production", "__", "directive_params", "directive_param", "_Rdirective_params_2", "_Rdirective_params_3", "sym", "expr1", 
			"disjunction", "expr2", "conjunction", "expr3", "negation", "literals", "literal", "_Rliterals_4", "_Rliterals_5", "terminal", 
			"nonterminal_", "terminal_char", "terminal_string", "nonliteral", "sym_start", "sym_rest", "_Rsym_6", "_Rsym_7", "group", "optional", 
			"repeat", "plus", "multi", "unescaped_c", "escaped_c", "_Rterminal_char_8", "_Runescaped_c_9", "_Rescaped_c_10", "unescaped_s", "escaped_s", 
			"_Rterminal_string_11", "_Rterminal_string_12", "_Runescaped_s_13", "_Rescaped_s_14", "_R__15", "comment", "_R___16", "_Rcomment_17", "_Rcomment_18", "_Rcomment_19", 
			"__neg_0", "__neg_1", 
		}) nts.get(nt);
		return nts;
	}
	idni::char_class_fns<terminal_type> load_cc() {
		return idni::predefined_char_classes<char_type, terminal_type>({
			"eof",
			"alnum",
			"alpha",
			"space",
			"printable",
		}, nts);
	}
	options load_opts() {
		options o;
		return o;
	}
	idni::prods<char_type, terminal_type> load_prods() {
		idni::prods<char_type, terminal_type> q,
			nul(symbol_type{});
		// _Rstart_0 => _ statement.
		q(nt(9), (nt(7)+nt(8)));
		// _Rstart_1 => null.
		q(nt(10), (nul));
		// _Rstart_1 => _Rstart_0 _Rstart_1.
		q(nt(10), (nt(9)+nt(10)));
		// start => _Rstart_1 _.
		q(nt(6), (nt(10)+nt(7)));
		// statement => directive.
		q(nt(8), (nt(11)));
		// statement => production.
		q(nt(8), (nt(12)));
		// directive => '@' 'u' 's' 'e' '_' 'c' 'h' 'a' 'r' '_' 'c' 'l' 'a' 's' 's' __ directive_params _ '.'.
		q(nt(11), (t(1)+t(2)+t(3)+t(4)+t(5)+t(6)+t(7)+t(8)+t(9)+t(5)+t(6)+t(10)+t(8)+t(3)+t(3)+nt(13)+nt(14)+nt(7)+t(11)));
		// _Rdirective_params_2 => _ ',' _ directive_param.
		q(nt(16), (nt(7)+t(12)+nt(7)+nt(15)));
		// _Rdirective_params_3 => null.
		q(nt(17), (nul));
		// _Rdirective_params_3 => _Rdirective_params_2 _Rdirective_params_3.
		q(nt(17), (nt(16)+nt(17)));
		// directive_params => directive_param _Rdirective_params_3.
		q(nt(14), (nt(15)+nt(17)));
		// directive_param => 'a' 'l' 'n' 'u' 'm'.
		q(nt(15), (t(8)+t(10)+t(13)+t(2)+t(14)));
		// directive_param => 'a' 'l' 'p' 'h' 'a'.
		q(nt(15), (t(8)+t(10)+t(15)+t(7)+t(8)));
		// directive_param => 'b' 'l' 'a' 'n' 'k'.
		q(nt(15), (t(16)+t(10)+t(8)+t(13)+t(17)));
		// directive_param => 'c' 'n' 't' 'r' 'l'.
		q(nt(15), (t(6)+t(13)+t(18)+t(9)+t(10)));
		// directive_param => 'd' 'i' 'g' 'i' 't'.
		q(nt(15), (t(19)+t(20)+t(21)+t(20)+t(18)));
		// directive_param => 'e' 'o' 'f'.
		q(nt(15), (t(4)+t(22)+t(23)));
		// directive_param => 'g' 'r' 'a' 'p' 'h'.
		q(nt(15), (t(21)+t(9)+t(8)+t(15)+t(7)));
		// directive_param => 'l' 'o' 'w' 'e' 'r'.
		q(nt(15), (t(10)+t(22)+t(24)+t(4)+t(9)));
		// directive_param => 'p' 'r' 'i' 'n' 't' 'a' 'b' 'l' 'e'.
		q(nt(15), (t(15)+t(9)+t(20)+t(13)+t(18)+t(8)+t(16)+t(10)+t(4)));
		// directive_param => 'p' 'u' 'n' 'c' 't'.
		q(nt(15), (t(15)+t(2)+t(13)+t(6)+t(18)));
		// directive_param => 's' 'p' 'a' 'c' 'e'.
		q(nt(15), (t(3)+t(15)+t(8)+t(6)+t(4)));
		// directive_param => 'u' 'p' 'p' 'e' 'r'.
		q(nt(15), (t(2)+t(15)+t(15)+t(4)+t(9)));
		// directive_param => 'x' 'd' 'i' 'g' 'i' 't'.
		q(nt(15), (t(25)+t(19)+t(20)+t(21)+t(20)+t(18)));
		// production => sym _ '=' '>' _ expr1 _ '.'.
		q(nt(12), (nt(18)+nt(7)+t(26)+t(27)+nt(7)+nt(19)+nt(7)+t(11)));
		// expr1 => disjunction.
		q(nt(19), (nt(20)));
		// expr1 => expr2.
		q(nt(19), (nt(21)));
		// expr2 => conjunction.
		q(nt(21), (nt(22)));
		// expr2 => expr3.
		q(nt(21), (nt(23)));
		// expr3 => negation.
		q(nt(23), (nt(24)));
		// expr3 => literals.
		q(nt(23), (nt(25)));
		// disjunction => expr1 _ '|' _ expr2.
		q(nt(20), (nt(19)+nt(7)+t(28)+nt(7)+nt(21)));
		// conjunction => expr2 _ '&' _ expr3.
		q(nt(22), (nt(21)+nt(7)+t(29)+nt(7)+nt(23)));
		// negation => '~' _ expr3.
		q(nt(24), (t(30)+nt(7)+nt(23)));
		// _Rliterals_4 => __ literal.
		q(nt(27), (nt(13)+nt(26)));
		// _Rliterals_5 => null.
		q(nt(28), (nul));
		// _Rliterals_5 => _Rliterals_4 _Rliterals_5.
		q(nt(28), (nt(27)+nt(28)));
		// literals => literal _Rliterals_5.
		q(nt(25), (nt(26)+nt(28)));
		// literal => terminal.
		q(nt(26), (nt(29)));
		// literal => nonterminal_.
		q(nt(26), (nt(30)));
		// terminal => terminal_char.
		q(nt(29), (nt(31)));
		// terminal => terminal_string.
		q(nt(29), (nt(32)));
		// nonterminal_ => sym.
		q(nt(30), (nt(18)));
		// nonterminal_ => nonliteral.
		q(nt(30), (nt(33)));
		// _Rsym_6 => sym_rest.
		q(nt(36), (nt(35)));
		// _Rsym_7 => null.
		q(nt(37), (nul));
		// _Rsym_7 => _Rsym_6 _Rsym_7.
		q(nt(37), (nt(36)+nt(37)));
		// sym => sym_start _Rsym_7.
		q(nt(18), (nt(34)+nt(37)));
		// sym_start => '_'.
		q(nt(34), (t(5)));
		// sym_start => alpha.
		q(nt(34), (nt(3)));
		// sym_rest => '_'.
		q(nt(35), (t(5)));
		// sym_rest => alnum.
		q(nt(35), (nt(2)));
		// nonliteral => group.
		q(nt(33), (nt(38)));
		// nonliteral => optional.
		q(nt(33), (nt(39)));
		// nonliteral => repeat.
		q(nt(33), (nt(40)));
		// nonliteral => plus.
		q(nt(33), (nt(41)));
		// nonliteral => multi.
		q(nt(33), (nt(42)));
		// group => '(' _ expr1 _ ')'.
		q(nt(38), (t(31)+nt(7)+nt(19)+nt(7)+t(32)));
		// optional => '[' _ expr1 _ ']'.
		q(nt(39), (t(33)+nt(7)+nt(19)+nt(7)+t(34)));
		// repeat => '{' _ expr1 _ '}'.
		q(nt(40), (t(35)+nt(7)+nt(19)+nt(7)+t(36)));
		// plus => literal _ '+'.
		q(nt(41), (nt(26)+nt(7)+t(37)));
		// multi => literal _ '*'.
		q(nt(42), (nt(26)+nt(7)+t(38)));
		// _Rterminal_char_8 => unescaped_c.
		q(nt(45), (nt(43)));
		// _Rterminal_char_8 => escaped_c.
		q(nt(45), (nt(44)));
		// terminal_char => '\'' _Rterminal_char_8 '\''.
		q(nt(31), (t(39)+nt(45)+t(39)));
		// _Runescaped_c_9 => '\''.
		q(nt(46), (t(39)));
		// _Runescaped_c_9 => '\\'.
		q(nt(46), (t(40)));
		// __neg_0 => _Runescaped_c_9.
		q(nt(60), (nt(46)));
		// unescaped_c => printable & ~( __neg_0 ).
		q(nt(43), (nt(5)) & ~(nt(60)));
		// _Rescaped_c_10 => '\''.
		q(nt(47), (t(39)));
		// _Rescaped_c_10 => '/'.
		q(nt(47), (t(41)));
		// _Rescaped_c_10 => '\\'.
		q(nt(47), (t(40)));
		// _Rescaped_c_10 => 'b'.
		q(nt(47), (t(16)));
		// _Rescaped_c_10 => 'f'.
		q(nt(47), (t(23)));
		// _Rescaped_c_10 => 'n'.
		q(nt(47), (t(13)));
		// _Rescaped_c_10 => 'r'.
		q(nt(47), (t(9)));
		// _Rescaped_c_10 => 't'.
		q(nt(47), (t(18)));
		// escaped_c => '\\' _Rescaped_c_10.
		q(nt(44), (t(40)+nt(47)));
		// _Rterminal_string_11 => unescaped_s.
		q(nt(50), (nt(48)));
		// _Rterminal_string_11 => escaped_s.
		q(nt(50), (nt(49)));
		// _Rterminal_string_12 => null.
		q(nt(51), (nul));
		// _Rterminal_string_12 => _Rterminal_string_11 _Rterminal_string_12.
		q(nt(51), (nt(50)+nt(51)));
		// terminal_string => '"' _Rterminal_string_12 '"'.
		q(nt(32), (t(42)+nt(51)+t(42)));
		// _Runescaped_s_13 => '"'.
		q(nt(52), (t(42)));
		// _Runescaped_s_13 => '\\'.
		q(nt(52), (t(40)));
		// __neg_1 => _Runescaped_s_13.
		q(nt(61), (nt(52)));
		// unescaped_s => printable & ~( __neg_1 ).
		q(nt(48), (nt(5)) & ~(nt(61)));
		// _Rescaped_s_14 => '"'.
		q(nt(53), (t(42)));
		// _Rescaped_s_14 => '/'.
		q(nt(53), (t(41)));
		// _Rescaped_s_14 => '\\'.
		q(nt(53), (t(40)));
		// _Rescaped_s_14 => 'b'.
		q(nt(53), (t(16)));
		// _Rescaped_s_14 => 'f'.
		q(nt(53), (t(23)));
		// _Rescaped_s_14 => 'n'.
		q(nt(53), (t(13)));
		// _Rescaped_s_14 => 'r'.
		q(nt(53), (t(9)));
		// _Rescaped_s_14 => 't'.
		q(nt(53), (t(18)));
		// escaped_s => '\\' _Rescaped_s_14.
		q(nt(49), (t(40)+nt(53)));
		// _R__15 => null.
		q(nt(54), (nul));
		// _R__15 => __.
		q(nt(54), (nt(13)));
		// _ => _R__15.
		q(nt(7), (nt(54)));
		// _R___16 => space.
		q(nt(56), (nt(4)));
		// _R___16 => comment.
		q(nt(56), (nt(55)));
		// __ => _R___16 _.
		q(nt(13), (nt(56)+nt(7)));
		// _Rcomment_17 => '\t'.
		q(nt(57), (t(43)));
		// _Rcomment_17 => printable.
		q(nt(57), (nt(5)));
		// _Rcomment_18 => null.
		q(nt(58), (nul));
		// _Rcomment_18 => _Rcomment_17 _Rcomment_18.
		q(nt(58), (nt(57)+nt(58)));
		// _Rcomment_19 => '\n'.
		q(nt(59), (t(44)));
		// _Rcomment_19 => '\r'.
		q(nt(59), (t(45)));
		// _Rcomment_19 => eof.
		q(nt(59), (nt(1)));
		// comment => '#' _Rcomment_18 _Rcomment_19.
		q(nt(55), (t(46)+nt(58)+nt(59)));
		return q;
	}
};

using sp_node_type = tgf_parser::sp_rw_node_type;
using rw_symbol_type = tgf_parser::rw_symbol_type;
using symbol_type = tgf_parser::symbol_type;
using nonterminal_type = tgf_parser::nonterminal;

static inline bool is_non_terminal_node(const sp_node_type& n) {
	return std::holds_alternative<symbol_type>(n->value)
		&& get<symbol_type>(n->value).nt();
};

static inline std::function<bool(const sp_node_type&)> is_non_terminal_node() {
	return [](const sp_node_type& n) { return is_non_terminal_node(n); };
}

static inline bool is_non_terminal(const nonterminal_type nt, const sp_node_type& n) {
	return is_non_terminal_node(n)
		&& std::get<symbol_type>(n->value).n() == nt;
}

static inline std::function<bool(const sp_node_type&)> is_non_terminal(
	const nonterminal_type nt)
{
	return [nt](const sp_node_type& n) { return is_non_terminal(nt, n); };
}

static inline std::optional<sp_node_type> operator|(const sp_node_type& n,
	const nonterminal_type nt)
{
	auto v = n->child
		| std::ranges::views::filter(is_non_terminal(nt))
		| std::ranges::views::take(1);
	return v.empty() ? std::optional<sp_node_type>()
		: std::optional<sp_node_type>(v.front());
}

static inline std::optional<sp_node_type> operator|(const std::optional<sp_node_type>& n,
	const nonterminal_type nt)
{
	return n ? n.value() | nt : n;
}

static inline std::vector<sp_node_type> operator||(const sp_node_type& n,
	const nonterminal_type nt)
{
	std::vector<sp_node_type> nv;
	nv.reserve(n->child.size());
	for (const auto& c : n->child | std::ranges::views::filter(
					is_non_terminal(nt))) nv.push_back(c);
	return nv;
}

static inline std::vector<sp_node_type> operator||(const std::optional<sp_node_type>& n,
	const nonterminal_type nt)
{
	if (n) return n.value() || nt;
	return {};
}

static const auto only_child_extractor = [](const sp_node_type& n)
	-> std::optional<sp_node_type>
{
	if (n->child.size() != 1) return std::optional<sp_node_type>();
	return std::optional<sp_node_type>(n->child[0]);
};
using only_child_extractor_t = decltype(only_child_extractor);

static inline std::vector<sp_node_type> operator||(const std::vector<sp_node_type>& v,
	const only_child_extractor_t e)
{
	std::vector<sp_node_type> nv;
	for (const auto& n : v | std::ranges::views::transform(e))
		if (n.has_value()) nv.push_back(n.value());
	return nv;
}

static inline std::optional<sp_node_type> operator|(const std::optional<sp_node_type>& o,
	const only_child_extractor_t e)
{
	return o.has_value() ? e(o.value()) : std::optional<sp_node_type>();
}

static inline std::optional<sp_node_type> operator|(const sp_node_type& o,
	const only_child_extractor_t e)
{
	return e(o);
}

static const auto terminal_extractor = [](const sp_node_type& n)
	-> std::optional<char>
{
	if (std::holds_alternative<symbol_type>(n->value)) {
		auto& v = std::get<symbol_type>(n->value);
		if (!v.nt() && !v.is_null()) return std::optional<char>(v.t());
	}
	return std::optional<char>();
};

using terminal_extractor_t = decltype(terminal_extractor);

static inline std::optional<char> operator|(const std::optional<sp_node_type>& o,
	const terminal_extractor_t e)
{
	return o.has_value() ? e(o.value()) : std::optional<char>();
}

static const auto non_terminal_extractor = [](const sp_node_type& n)
	-> std::optional<size_t>
{
	if (std::holds_alternative<symbol_type>(n->value)) {
		auto& v = std::get<symbol_type>(n->value);
		if (v.nt()) return std::optional<char>(v.n());
	}
	return std::optional<size_t>();
};

using non_terminal_extractor_t = decltype(non_terminal_extractor);

static inline std::optional<size_t> operator|(const std::optional<sp_node_type>& o,
	const non_terminal_extractor_t e)
{
	return o.has_value() ? e(o.value()) : std::optional<size_t>();
}

static inline std::optional<size_t> operator|(const sp_node_type& o,
	const non_terminal_extractor_t e)
{
	return e(o);
}

static const auto size_t_extractor = [](const sp_node_type& n)
	-> std::optional<size_t>
{
	if (std::holds_alternative<size_t>(n->value))
		return std::optional<size_t>(std::get<size_t>(n->value));
	return std::optional<size_t>();
};
using size_t_extractor_t = decltype(size_t_extractor);

static inline std::optional<size_t> operator|(const std::optional<sp_node_type>& o,
	const size_t_extractor_t e)
{
	return o.has_value() ? e(o.value()) : std::optional<size_t>();
}

static inline std::optional<size_t> operator|(const sp_node_type& o,
	const size_t_extractor_t e)
{
	return e(o);
}

// is not a whitespace predicate
static const auto not_whitespace_predicate = [](const sp_node_type& n) {
	if (std::holds_alternative<symbol_type>(n->value)) {
		auto& v = std::get<symbol_type>(n->value);
		return !v.nt() || (v.n() != tgf_parser::_ &&
				v.n() != tgf_parser::__);
	}
	return true;
};

using not_whitespace_predicate_t = decltype(not_whitespace_predicate);

template <typename extractor_t>
struct stringify {
	stringify(const extractor_t& extractor,
		std::basic_stringstream<char>& stream)
		: stream(stream), extractor(extractor) {}
	sp_node_type operator()(const sp_node_type& n) {
		if (auto str = extractor(n); str) stream << str.value();
		return n;
	}
	std::basic_stringstream<char>& stream;
	const extractor_t& extractor;
};

template <typename extractor_t, typename predicate_t>
std::string make_string_with_skip(const extractor_t& extractor,
	predicate_t& skip, const sp_node_type& n)
{
	std::basic_stringstream<char> ss;
	stringify<extractor_t> sy(extractor, ss);
	idni::rewriter::post_order_tree_traverser<
		stringify<extractor_t>, predicate_t, sp_node_type>(sy, skip)(n);
	return ss.str();
}
} // tgf_parsing namespace
#endif // __TGF_PARSER_H__
