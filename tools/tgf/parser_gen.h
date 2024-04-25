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
	std::string output_dir                       = "";
	std::string output                           = "";
	std::string name                             = "";
	std::string ns                               = "";
	std::string char_type                        = "char";
	std::string terminal_type                    = "char";
	std::string decoder                          = "";
	std::string encoder                          = "";
	bool auto_disambiguate                       = true;
	std::vector<std::string> nodisambig_list     = {};
};

template <typename C = char, typename T = C>
void generate_parser_cpp(const std::string& tgf_filename,
	const grammar<C, T>& g, parser_gen_options opt = {})
{
	if (g.size() == 0) return;

	auto strip_pwd = [](const std::string& p) {
		auto pwd = std::filesystem::current_path().string();
		if (pwd.back() != '/') pwd += '/';
		if (p.compare(0, pwd.size(), pwd) == 0)
			return p.substr(pwd.size());
		return p;
	};
	auto strip_path = [](const std::string& p) {
		auto pos = p.rfind('/');
		return (pos == std::string::npos) ? p : p.substr(pos + 1);
	};
	auto strip_filename = [](const std::string& p) {
		auto pos = p.rfind('/');
		return (pos == std::string::npos) ? "" : p.substr(0, pos);
	};
	auto strip_ext = [](const std::string& p) {
		auto pos = p.rfind('.');
		return (pos == std::string::npos) ? p : p.substr(0, pos);
	};

	std::string tgf_filename_stripped = strip_pwd(tgf_filename);
	std::string basename = strip_ext(strip_path(tgf_filename_stripped));
	if (opt.name.size() == 0) opt.name = basename + "_parser";
	if (opt.output_dir.size() == 0) opt.output_dir = strip_filename(tgf_filename);
	if (opt.output_dir.size() && opt.output_dir.back() != '/')
		opt.output_dir += '/';
	if (opt.output.size() == 0) opt.output = opt.name + ".generated.h";

	std::ofstream os(opt.output_dir + opt.output);

	// convert name to upper case and append __PARSER_GEN_H__ as a guard at the begining
	std::string guard = opt.name;
	std::transform(guard.begin(), guard.end(), guard.begin(), ::toupper);

	std::string U = opt.terminal_type == "char32_t" ? "U" : "";
	std::vector<C> ts{ (C)0 };
	grammar_inspector<C, T> gi(g);

	auto gen_ts = [&ts, &U]() {
		std::stringstream os;
		os << "\t";
		size_t n = 0;
		for (const C ch : ts) {
			if (++n == 10) n = 0, os << "\n\t";
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
			os << (n % 10 == 0 ? "\n\t" : "") <<
				"\"" << to_std_string(x[n]) << "\", ";
		os << "\n";
		return os.str();
	};
	auto gen_cc_fns = [&gi]() {
		std::stringstream os;
		auto& x = gi.nts();
		for (const auto& fn : gi.cc_fns().fns)
			if (x[fn.first].size()) os << "\t\t\""
				<< to_std_string(x[fn.first]) << "\",\n";
		return os.str();
	};
	auto gen_grammar_opts = [&g]() {
		std::stringstream os;
		std::array<const char*, 2> pbool = { "false", "true" };
		auto plist = [](std::ostream& os,
			const std::set<size_t>& list) -> std::ostream&
		{
			size_t i = 0;
			for (const auto& s : list) os << (i ? "," : "")
				<< (i % 10 == 0 ? "\n\t\t\t" : " ") << s, i++;
			return os;
		};
		auto pvlist = [](std::ostream& os,
			const std::set<std::vector<size_t>>& vlist)
				-> std::ostream&
		{
			size_t i = 0;
			for (const auto& v : vlist) {
				os << (i++ ? "," : "") << "\n\t\t\t{ ";
				size_t j = 0;
				for (const auto& s : v)
					os << (j++ ? ", " : "") << s;
				os << " }";
			}
			return os;
		};
		os << "\t.auto_disambiguate = "
			<< pbool[g.opt.auto_disambiguate] << ",\n";
		if (g.opt.nodisambig_list.size()) {
			os << "\t.nodisambig_list = {";
			plist(os, g.opt.nodisambig_list) << "\n\t},\n";
		}
		os << "\t.shaping = {\n";
		if (g.opt.shaping.to_trim.size()) {
			os << "\t\t.to_trim = {";
			plist(os, g.opt.shaping.to_trim) << "\n\t\t},\n";
		}
		if (g.opt.shaping.to_trim_children.size()) {
			os << "\t\t.to_trim_children = {";
			plist(os, g.opt.shaping.to_trim_children) << "\n\t\t},\n";
		}
		os << "\t\t.trim_terminals = "
			<< pbool[g.opt.shaping.trim_terminals] << ",\n";
		if (g.opt.shaping.to_inline.size()) {
			os << "\t\t.to_inline = {";
			pvlist(os, g.opt.shaping.to_inline) << "\n\t\t},\n";
		}
		os << "\t\t.inline_char_classes = "
			<< pbool[g.opt.shaping.inline_char_classes] << "\n";
		os << "\t}\n";
		return os.str();
	};
	auto gen_opts = [&opt]() {
		std::stringstream os;
		if (opt.decoder.size()) os << "\t.chars_to_terminals = "
			<< opt.decoder << ",\n";
		if (opt.encoder.size()) os << "\t.terminals_to_chars = "
			<< opt.encoder << "\n";
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
		for (size_t i = 0; i != gi.G().size(); ++i) {
			const auto& p = gi.G()[i];
			g.print_production(os << "//", i, true) << "\n";
			os << "\tp(NT(" << p.first.n() << "), ";
			bool first_c = true;
			for (const auto& c : p.second) {
				if (!first_c) os << " & "; else first_c = false;
				if (c.neg) os << '~';
				os << '(';
				bool first_l = true;
				for (const auto& l : c) {
					if (!first_l) os << "+";
					else first_l = false;
					if (l.nt()) os << "NT(" << l.n() << ")";
					else if (l.is_null()) os << "nul";
					else os << "T(" << terminal(l) << ")";
				}
				os << ')';
			}
			os << ");\n";
		}
		return os.str();
	};
	const auto ps = gen_prods();
	os <<	"// This file is generated from a file " <<
					tgf_filename_stripped << " by\n"
		"//       https://github.com/IDNI/parser/tools/tgf\n"
		"//\n"
		"#ifndef __" << guard << "_H__\n"
		"#define __" << guard << "_H__\n"
		"\n"
		"#include \"parser.h\"\n"
		"\n";
	if (opt.ns.size()) os << "namespace " << opt.ns << " {\n\n";
	os << "namespace " << opt.name << "_data {\n\n";
	os <<
		"using char_type     = " << opt.char_type << ";\n"
		"using terminal_type = " << opt.terminal_type << ";\n"
		"\n"
		"static inline std::vector<std::string> symbol_names{"
			<< gen_nts() <<
		"};\n"
		"\n"
		"static inline ::idni::nonterminals<char_type, terminal_type> nts{symbol_names};\n"
		"\n"
		"static inline std::vector<terminal_type> terminals{\n"
			<< gen_ts() <<
		"};\n"
		"\n"
		"static inline ::idni::char_class_fns<terminal_type> char_classes =\n"
		"	::idni::predefined_char_classes<char_type, terminal_type>({\n"
			<< gen_cc_fns() <<
		"	}, nts);\n"
		"\n"
		"static inline struct ::idni::grammar<char_type, terminal_type>::options\n"
		"	grammar_options\n"
		"{\n"
		"	.transform_negation = false,\n"
			<< gen_grammar_opts() <<
		"};\n"
		"static inline ::idni::parser<char_type, terminal_type>::options parser_options{\n"
			<< gen_opts() <<
		"};\n"
		"\n"
		"static inline ::idni::prods<char_type, terminal_type> start_symbol{ nts("
						<< gi.start().n() << ") };\n"
		"\n"
		"static inline idni::prods<char_type, terminal_type>& productions() {\n"
		"	static bool loaded = false;\n"
		"	static idni::prods<char_type, terminal_type>\n"
		"		p, nul(idni::lit<char_type, terminal_type>{});\n"
		"	if (loaded) return p;\n"
		"	#define  T(x) (idni::prods<char_type, terminal_type>{ terminals[x] })\n"
		"	#define NT(x) (idni::prods<char_type, terminal_type>{ nts(x) })\n"
			<< ps <<
		"	#undef T\n"
		"	#undef NT\n"
		"	return loaded = true, p;\n"
		"}\n"
		"\n"
		"static inline ::idni::grammar<char_type, terminal_type> grammar(\n"
		"	nts, productions(), start_symbol, char_classes, grammar_options);\n"
		"\n"
		"} // namespace " << opt.name << "_data\n"
		"\n"
		"struct " << opt.name << " : public idni::parser<"
			<< opt.char_type << ", " << opt.terminal_type << "> {\n"
		"	enum nonterminal {"
				<< gen_nts_enum_cte() <<
		"	};\n"
		"	static " << opt.name << "& instance() {\n"
		"		static " << opt.name << " inst;\n"
		"		return inst;\n"
		"	}\n"
		"	" << opt.name << "() : idni::parser<char_type, terminal_type>(\n"
		"		" << opt.name << "_data::grammar,\n"
		"		" << opt.name << "_data::parser_options) {}\n"
		"};\n"
		"\n";
	if (opt.ns.size()) os << "\n} // " <<opt.ns<< " namespace\n";
	os <<	"#endif // __" << guard << "_H__\n";
}

template <typename C = char, typename T = C>
void generate_parser_cpp_from_string(const std::string& tgf_filename,
	const std::basic_string<C>& grammar_tgf,
	parser_gen_options opt = {})
{
	nonterminals<C, T> nts;
	generate_parser_cpp(tgf_filename,
		tgf<C, T>::from_string(nts, grammar_tgf), opt);
}

template <typename C = char, typename T = C>
void generate_parser_cpp_from_file(const std::string& tgf_filename,
	parser_gen_options opt = {})
{
	nonterminals<C, T> nts;
	generate_parser_cpp(tgf_filename,
		tgf<C, T>::from_file(nts, tgf_filename), opt);
}

} // idni namespace
#endif //__IDNI__PARSER__PARSER_GEN_H__
