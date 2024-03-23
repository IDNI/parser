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
#ifndef __IDNI__PARSER__PARSER_GEN_H__
#define __IDNI__PARSER__PARSER_GEN_H__
#include <filesystem>
#include "parser.h"
namespace idni {

template <typename C = char, typename T = C>
struct grammar_inspector { // provides access to private grammar members
	grammar_inspector(const grammar<C, T>& g) : g(g) {}
	const grammar<C, T>& g;
	lit<C, T> start() { return g.start; }
	nonterminals<C, T>& nts() { return g.nts; }
	const std::vector<std::pair<lit<C, T>, std::vector<lits<C, T>>>>& G() {
		return g.G; }
	const char_class_fns<C>& cc_fns() { return g.cc_fns; }
};

struct parser_gen_options {
	std::string ns            = "generated_parser_ns";
	std::string name          = "generated_parser";
	std::string char_type     = "char";
	std::string terminal_type = "char";
	std::string decoder       = "";
	std::string encoder       = "";
	bool traversals           = true;
	std::vector<std::string> rewriter_node_types = {};
};

template <typename C = char, typename T = C>
std::ostream& generate_parser_cpp(std::ostream& os,
	const std::string& tgf_filename,
	const grammar<C, T>& g,
	parser_gen_options opt = {})
{
	if (g.size() == 0) return os;
	std::string U = opt.terminal_type == "char32_t" ? "U" : "";
	std::vector<C> ts{ (C)0 };
	grammar_inspector<C, T> gi(g);
	// convert name to upper case and append __PARSER_GEN_H__ as a guard at the begining
	std::string guard = opt.name;
	std::transform(guard.begin(), guard.end(), guard.begin(), ::toupper);

	auto gen_ts = [&ts, &U]() {
		std::stringstream os;
		os << "\t\t";
		size_t n = 0;
		for (const C ch : ts) {
			if (++n == 10) n = 0, os << "\n\t\t";
			os << U << "'" <<
				( ch == (C)    0 ? "\\0"
				: ch == (C) '\r' ? "\\r"
				: ch == (C) '\n' ? "\\n"
				: ch == (C) '\t' ? "\\t"
				: ch == (C) '\\' ? "\\\\"
				: ch == (C) '\'' ? "\\'"
				: to_std_string(ch)) << "', ";
		}
		os << "\n";
		return os.str();
	};
	auto gen_nts_enum_cte = [&gi, &U]() {
		std::stringstream os;
		auto& x = gi.nts();
		for (size_t n = 0; n != x.size(); ++n)
			os << (n % 10 == 0 ? "\n\t\t" : "") << U <<
				(n == 0 ? "nul" : to_std_string(x[n]))
				<< ", ";
		os << "\n";
		return os.str();
	};
	auto gen_nts = [&gi]() {
		std::stringstream os;
		auto& x = gi.nts();
		for (size_t n = 0; n != x.size(); ++n)
			os << (n % 10 == 0 ? "\n\t\t\t" : "") <<
				"\"" << to_std_string(x[n]) << "\", ";
		os << "\n";
		return os.str();
	};
	auto gen_cc_fns = [&gi]() {
		std::stringstream os;
		auto& x = gi.nts();
		for (const auto& fn : gi.cc_fns().fns)
			if (x[fn.first].size()) os << "\t\t\t\""
				<< to_std_string(x[fn.first]) << "\",\n";
		return os.str();
	};
	auto gen_opts = [&opt]() {
		std::stringstream os;
		if (opt.decoder.size()) os << "\t\to.chars_to_terminals = "
			<< opt.decoder << ";\n";
		if (opt.encoder.size()) os << "\t\to.terminals_to_chars = "
			<< opt.encoder << ";\n";
		return os.str();
	};
	auto gen_prods = [&g, &gi, &ts]() {
		std::stringstream os;
		auto terminal = [&ts](const lit<C, T>& l) {
			for (size_t n = 0; n != ts.size(); ++n)
				if (ts[n] == l.t()) return n;
			ts.push_back(l.t());
			return ts.size() - 1;
		};
		for (const auto& p : gi.G()) {
			g.print_production(os << "\t\t// ", p) << "\n";
			os << "\t\tq(nt(" << p.first.n() << "), ";
			bool first_c = true;
			for (const auto& c : p.second) {
				if (!first_c) os << " & "; else first_c = false;
				if (c.neg) os << '~';
				os << '(';
				bool first_l = true;
				for (const auto& l : c) {
					if (!first_l) os << "+";
					else first_l = false;
					if (l.nt()) os << "nt(" << l.n() << ")";
					else if (l.is_null()) os << "nul";
					else os << "t(" << terminal(l) << ")";
				}
				os << ')';
			}
			os << ");\n";
		}
		return os.str();
	};
	auto strip_pwd = [](const std::string& p) {
		auto pwd = std::filesystem::current_path().string();
		if (pwd.back() != '/') pwd += '/';
		if (p.compare(0, pwd.size(), pwd) == 0)
			return p.substr(pwd.size());
		return p;
	};
	auto rw_symbol_type = [&opt]() {
		std::stringstream os;
		os << "	using rw_symbol_type  = std::variant<symbol_type";
		for (const auto& t : opt.rewriter_node_types) os << ", " << t;
		os << ">;\n";
		return os.str();
	};
	auto gen_traversals = [&opt]() {
		std::stringstream os;
		if (!opt.traversals) return std::string{};
		os << "\n"
		"using sp_node_type = " <<opt.name<< "::sp_rw_node_type;\n"
		"using rw_symbol_type = " <<opt.name<< "::rw_symbol_type;\n"
		"using symbol_type = " <<opt.name<< "::symbol_type;\n"
		"using nonterminal_type = " <<opt.name<< "::nonterminal;\n"
		"\n"
		"static inline bool is_non_terminal_node(const sp_node_type& n) {\n"
		"	return std::holds_alternative<symbol_type>(n->value)\n"
		"		&& get<symbol_type>(n->value).nt();\n"
		"};\n"
		"\n"
		"static inline std::function<bool(const sp_node_type&)> is_non_terminal_node() {\n"
		"	return [](const sp_node_type& n) { return is_non_terminal_node(n); };\n"
		"}\n"
		"\n"
		"static inline bool is_non_terminal(const nonterminal_type nt,\n"
		"	const sp_node_type& n) {\n"
		"	return is_non_terminal_node(n)\n"
		"		&& std::get<symbol_type>(n->value).n() == nt;\n"
		"}\n"
		"\n"
		"static inline std::function<bool(const sp_node_type&)> is_non_terminal(\n"
		"	const nonterminal_type nt)\n"
		"{\n"
		"	return [nt](const sp_node_type& n) { return is_non_terminal(nt, n); };\n"
		"}\n"
		"\n"
		"static inline std::optional<sp_node_type> operator|(const sp_node_type& n,\n"
		"	const nonterminal_type nt)\n"
		"{\n"
		"	auto v = n->child\n"
		"		| std::ranges::views::filter(is_non_terminal(nt))\n"
		"		| std::ranges::views::take(1);\n"
		"	return v.empty() ? std::optional<sp_node_type>()\n"
		"		: std::optional<sp_node_type>(v.front());\n"
		"}\n"
		"\n"
		"static inline std::optional<sp_node_type> operator|(\n"
		"	const std::optional<sp_node_type>& n,\n"
		"	const nonterminal_type nt)\n"
		"{\n"
		"	return n ? n.value() | nt : n;\n"
		"}\n"
		"\n"
		"static inline std::vector<sp_node_type> operator||(const sp_node_type& n,\n"
		"	const nonterminal_type nt)\n"
		"{\n"
		"	std::vector<sp_node_type> nv;\n"
		"	nv.reserve(n->child.size());\n"
		"	for (const auto& c : n->child | std::ranges::views::filter(\n"
		"					is_non_terminal(nt))) nv.push_back(c);\n"
		"	return nv;\n"
		"}\n"
		"\n"
		"static inline std::vector<sp_node_type> operator||(\n"
		"	const std::optional<sp_node_type>& n,\n"
		"	const nonterminal_type nt)\n"
		"{\n"
		"	if (n) return n.value() || nt;\n"
		"	return {};\n"
		"}\n"
		"\n"
		"static inline std::vector<sp_node_type> get_nodes(const nonterminal_type nt,\n"
		"	const sp_node_type& n)\n"
		"{\n"
		"	return n || nt;\n"
		"}\n"
		"\n"
		"template <nonterminal_type nt>\n"
		"std::vector<sp_node_type> get_nodes(const sp_node_type& n) {\n"
		"	return n || nt;\n"
		"}\n"
		"\n"
		"static inline auto get_nodes(const nonterminal_type nt) {\n"
		"	return [nt](const sp_node_type& n) { return get_nodes(nt, n); };\n"
		"}\n"
		"\n"
		"static inline std::vector<sp_node_type> operator||(\n"
		"	const std::vector<sp_node_type>& v,\n"
		"	const nonterminal_type nt) {\n"
		"	std::vector<sp_node_type> nv; nv.reserve(v.size());\n"
		"	for (const auto& n: v\n"
		"			| std::ranges::views::transform(get_nodes(nt))\n"
		"			| std::ranges::views::join)\n"
		"		nv.push_back(n);\n"
		"	return nv;\n"
		"}\n"
		"\n"
		"static const auto only_child_extractor = [](const sp_node_type& n)\n"
		"	-> std::optional<sp_node_type>\n"
		"{\n"
		"	if (n->child.size() != 1) return std::optional<sp_node_type>();\n"
		"	return std::optional<sp_node_type>(n->child[0]);\n"
		"};\n"
		"using only_child_extractor_t = decltype(only_child_extractor);\n"
		"\n"
		"static inline std::vector<sp_node_type> operator||(\n"
		"	const std::vector<sp_node_type>& v,\n"
		"	const only_child_extractor_t e)\n"
		"{\n"
		"	std::vector<sp_node_type> nv;\n"
		"	for (const auto& n : v | std::ranges::views::transform(e))\n"
		"		if (n.has_value()) nv.push_back(n.value());\n"
		"	return nv;\n"
		"}\n"
		"\n"
		"static inline std::optional<sp_node_type> operator|(\n"
		"	const std::optional<sp_node_type>& o,\n"
		"	const only_child_extractor_t e)\n"
		"{\n"
		"	return o.has_value() ? e(o.value()) : std::optional<sp_node_type>();\n"
		"}\n"
		"\n"
		"static inline std::optional<sp_node_type> operator|(const sp_node_type& o,\n"
		"	const only_child_extractor_t e)\n"
		"{\n"
		"	return e(o);\n"
		"}\n"
		"\n"
		"static const auto terminal_extractor = [](const sp_node_type& n)\n"
		"	-> std::optional<char>\n"
		"{\n"
		"	if (std::holds_alternative<symbol_type>(n->value)) {\n"
		"		auto& v = std::get<symbol_type>(n->value);\n"
		"		if (!v.nt() && !v.is_null()) return std::optional<char>(v.t());\n"
		"	}\n"
		"	return std::optional<char>();\n"
		"};\n"
		"\n"
		"using terminal_extractor_t = decltype(terminal_extractor);\n"
		"\n"
		"static inline std::optional<char> operator|(\n"
		"	const std::optional<sp_node_type>& o,\n"
		"	const terminal_extractor_t e)\n"
		"{\n"
		"	return o.has_value() ? e(o.value()) : std::optional<char>();\n"
		"}\n"
		"\n"
		"static const auto non_terminal_extractor = [](const sp_node_type& n)\n"
		"	-> std::optional<size_t>\n"
		"{\n"
		"	if (std::holds_alternative<symbol_type>(n->value)) {\n"
		"		auto& v = std::get<symbol_type>(n->value);\n"
		"		if (v.nt()) return std::optional<char>(v.n());\n"
		"	}\n"
		"	return std::optional<size_t>();\n"
		"};\n"
		"\n"
		"using non_terminal_extractor_t = decltype(non_terminal_extractor);\n"
		"\n"
		"static inline std::optional<size_t> operator|(\n"
		"	const std::optional<sp_node_type>& o,\n"
		"	const non_terminal_extractor_t e)\n"
		"{\n"
		"	return o.has_value() ? e(o.value()) : std::optional<size_t>();\n"
		"}\n"
		"\n"
		"static inline std::optional<size_t> operator|(const sp_node_type& o,\n"
		"	const non_terminal_extractor_t e)\n"
		"{\n"
		"	return e(o);\n"
		"}\n"
		"\n"
		"static const auto size_t_extractor = [](const sp_node_type& n)\n"
		"	-> std::optional<size_t>\n"
		"{\n"
		"	if (std::holds_alternative<size_t>(n->value))\n"
		"		return std::optional<size_t>(std::get<size_t>(n->value));\n"
		"	return std::optional<size_t>();\n"
		"};\n"
		"using size_t_extractor_t = decltype(size_t_extractor);\n"
		"\n"
		"static inline std::optional<size_t> operator|(\n"
		"	const std::optional<sp_node_type>& o,\n"
		"	const size_t_extractor_t e)\n"
		"{\n"
		"	return o.has_value() ? e(o.value()) : std::optional<size_t>();\n"
		"}\n"
		"\n"
		"static inline std::optional<size_t> operator|(const sp_node_type& o,\n"
		"	const size_t_extractor_t e)\n"
		"{\n"
		"	return e(o);\n"
		"}\n"
		"\n"
		"// is not a whitespace predicate\n"
		"static const auto not_whitespace_predicate = [](const sp_node_type& n) {\n"
		"	if (std::holds_alternative<symbol_type>(n->value)) {\n"
		"		auto& v = std::get<symbol_type>(n->value);\n"
		"		return !v.nt() || (v.n() != " <<opt.name<< "::_ &&\n"
		"				v.n() != " <<opt.name<< "::__);\n"
		"	}\n"
		"	return true;\n"
		"};\n"
		"\n"
		"using not_whitespace_predicate_t = decltype(not_whitespace_predicate);\n"
		"\n"
		"template <typename extractor_t>\n"
		"struct stringify {\n"
		"	stringify(const extractor_t& extractor,\n"
		"		std::basic_stringstream<char>& stream)\n"
		"		: stream(stream), extractor(extractor) {}\n"
		"	sp_node_type operator()(const sp_node_type& n) {\n"
		"		if (auto str = extractor(n); str) stream << str.value();\n"
		"		return n;\n"
		"	}\n"
		"	std::basic_stringstream<char>& stream;\n"
		"	const extractor_t& extractor;\n"
		"};\n"
		"\n"
		"template <typename extractor_t, typename predicate_t>\n"
		"std::string make_string_with_skip(const extractor_t& extractor,\n"
		"	predicate_t& skip, const sp_node_type& n)\n"
		"{\n"
		"	std::basic_stringstream<char> ss;\n"
		"	stringify<extractor_t> sy(extractor, ss);\n"
		"	idni::rewriter::post_order_tree_traverser<\n"
		"		stringify<extractor_t>, predicate_t, sp_node_type>(sy, skip)(n);\n"
		"	return ss.str();\n"
		"}\n";
		return os.str();
	};
	const auto ps = gen_prods();
	os <<	"// This file is generated from a file " <<
					strip_pwd(tgf_filename)<<" by\n"
		"//       https://github.com/IDNI/parser/tools/tgf\n"
		"//\n"
		"#ifndef __" <<guard<< "_H__\n"
		"#define __" <<guard<< "_H__\n"
		"\n"
		"#include <string.h>\n"
		"#include <ranges>\n"
		"\n"
		"#include \"parser.h\"\n"
		"#include \"rewriting.h\"\n"
		"\n"
		"namespace " <<opt.ns<< " {\n"
		"\n"
		"struct " <<opt.name<< " {\n"
		"	using char_type     = "<<opt.char_type<<";\n"
		"	using terminal_type = "<<opt.terminal_type<<";\n"
		"	using traits_type   = std::char_traits<char_type>;\n"
		"	using int_type      = typename traits_type::int_type;\n"
		"	using symbol_type   ="
				" idni::lit<char_type, terminal_type>;\n"
		"	using location_type = std::array<size_t, 2>;\n"
		"	using node_type     ="
				" std::pair<symbol_type, location_type>;\n"
		"	using parser_type   ="
				" idni::parser<char_type, terminal_type>;\n"
		"	using options       = parser_type::options;\n"
		"	using parse_options = parser_type::parse_options;\n"
		"	using forest_type   = parser_type::pforest;\n"
		"	using input_type    = parser_type::input;\n"
		"	using decoder_type  ="
				" parser_type::input::decoder_type;\n"
		"	using encoder_type  = "
				"std::function<std::basic_string<char_type>(\n"
		"			const std::vector<terminal_type>&)>;\n"
		<< rw_symbol_type() <<
		"	using rw_node_type    = idni::rewriter::node<rw_symbol_type>;\n"
		"	using sp_rw_node_type = idni::rewriter::sp_node<rw_symbol_type>;\n"
		"	" <<opt.name<< "() :\n"
		"		nts(load_nonterminals()), cc(load_cc()),\n"
		"		g(nts, load_prods(), nt(" <<gi.start().n()<<
				"), cc), p(g, load_opts()) {}\n"
		"	std::unique_ptr<forest_type> parse(const char_type* data, size_t size,\n"
		"		parse_options po = {}) { return p.parse(data, size, po); }\n"
		"	std::unique_ptr<forest_type> parse(std::basic_istream<char_type>& is,\n"
		"		parse_options po = {}) { return p.parse(is, po); }\n"
		"	std::unique_ptr<forest_type> parse(const std::string& fn,\n"
		"		parse_options po = {}) { return p.parse(fn, po); }\n"
		"#ifndef WIN32\n"
		"	std::unique_ptr<forest_type> parse(int fd, parse_options po = {})\n"
		"		{ return p.parse(fd, po); }\n"
		"#endif //WIN32\n"
		"	bool found(int start = -1) { return p.found(start); }\n"
		"	typename parser_type::error get_error() { return p.get_error(); }\n"
		"	enum nonterminal {" << gen_nts_enum_cte() <<
								"	};\n"
		"	size_t id(const std::basic_string<char_type>& name) {\n"
		"		return nts.get(name);\n"
		"	}\n"
		"	const std::basic_string<char_type>& name(size_t id) {\n"
		"		return nts.get(id);\n"
		"	}\n"
		"private:\n"
		"	std::vector<terminal_type> ts{\n" <<
				gen_ts() <<
		"	};\n"
		"	idni::nonterminals<char_type, terminal_type> nts{};\n"
		"	idni::char_class_fns<terminal_type> cc;\n"
		"	idni::grammar<char_type, terminal_type> g;\n"
		"	parser_type p;\n"
		"	idni::prods<char_type, terminal_type> t(size_t tid) {\n"
		"		return idni::prods<char_type, terminal_type>(ts[tid]);\n"
		"	}\n"
		"	idni::prods<char_type, terminal_type> nt(size_t ntid) {\n"
		"		return idni::prods<char_type, terminal_type>(\n"
		"			symbol_type(ntid, &nts));\n"
		"	}\n"
		"	idni::nonterminals<char_type, terminal_type> load_nonterminals() "
								"const {\n"
		"		idni::nonterminals<char_type, terminal_type> nts{};\n"
		"		for (const auto& nt : {" <<
					gen_nts() <<
		"		}) nts.get(nt);\n"
		"		return nts;\n"
		"	}\n"
		"	idni::char_class_fns<terminal_type> load_cc() {\n"
		"		return idni::predefined_char_classes<char_type, terminal_type>({\n" <<
					gen_cc_fns() <<
		"		}, nts);\n"
		"	}\n"
		"	options load_opts() {\n"
		"		options o;\n" <<
					gen_opts() <<
		"		return o;\n"
		"	}\n"
		"	idni::prods<char_type, terminal_type> load_prods() {\n"
		"		idni::prods<char_type, terminal_type> q,\n"
		"			nul(symbol_type{});\n"
				<< ps <<
		"		return q;\n"
		"	}\n"
		"};\n"
		<< gen_traversals() <<
		"} // " <<opt.ns<< " namespace\n"
		"#endif // __" << guard << "_H__\n";
	return os;
}

template <typename C = char, typename T = C>
std::ostream& generate_parser_cpp_from_string(std::ostream& os,
	const std::string& tgf_filename,
	const std::basic_string<C>& grammar_tgf,
	const std::basic_string<C>& start_nt = from_cstr<C>("start"),
	parser_gen_options opt = {})
{
	nonterminals<C, T> nts;
	return generate_parser_cpp(os, tgf_filename,
		tgf<C, T>::from_string(nts, grammar_tgf, start_nt), opt);
}

template <typename C = char, typename T = C>
void generate_parser_cpp_from_file(std::ostream& os,
	const std::string& tgf_filename,
	const std::basic_string<C>& start_nt = from_cstr<C>("start"),
	parser_gen_options opt = {})
{
	nonterminals<C, T> nts;
	generate_parser_cpp(os, tgf_filename,
		tgf<C, T>::from_file(nts, tgf_filename, start_nt), opt);
}

} // idni namespace
#endif //__IDNI__PARSER__PARSER_GEN_H__
