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
		os << "\t\to.trim_terminals = "
			<< pbool[g.opt.trim_terminals] << ";\n";
		os << "\t\to.inline_char_classes = "
			<< pbool[g.opt.inline_char_classes] << ";\n";
		os << "\t\to.auto_disambiguate = "
			<< pbool[g.opt.auto_disambiguate] << ";\n";
		if (g.opt.nodisambig_list.size()) {
			os << "\t\to.nodisambig_list = {";
			plist(os, g.opt.nodisambig_list) << "\n\t\t};\n";
		}
		if (g.opt.to_trim.size()) {
			os << "\t\to.to_trim = {";
			plist(os, g.opt.to_trim) << "\n\t\t};\n";
		}
		if (g.opt.to_trim_children.size()) {
			os << "\t\to.to_trim_children = {";
			plist(os, g.opt.to_trim_children) << "\n\t\t};\n";
		}
		if (g.opt.to_inline.size()) {
			os << "\t\to.to_inline = {";
			pvlist(os, g.opt.to_inline) << "\n\t\t};\n";
		}
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
		for (size_t i = 0; i != gi.G().size(); ++i) {
			const auto& p = gi.G()[i];
			g.print_production(os << "\t\t// ", i) << "\n";
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
	const auto ps = gen_prods();
	os <<	"// This file is generated from a file " <<
					tgf_filename_stripped << " by\n"
		"//       https://github.com/IDNI/parser/tools/tgf\n"
		"//\n"
		"#ifndef __" <<guard<< "_H__\n"
		"#define __" <<guard<< "_H__\n"
		"\n"
		"#include <string.h>\n"
		"\n"
		"#include \"parser.h\"\n"
		"\n";
	if (opt.ns.size()) os << "namespace " <<opt.ns<< " {\n\n";
	os <<	"struct " <<opt.name<< " {\n"
		"	using char_type       = "<<opt.char_type<<";\n"
		"	using terminal_type   = "<<opt.terminal_type<<";\n"
		"	using traits_type     = std::char_traits<char_type>;\n"
		"	using int_type        = typename traits_type::int_type;\n"
		"	using grammar_type    = idni::grammar<char_type, terminal_type>;\n"
		"	using grammar_options = grammar_type::options;\n"
		"	using symbol_type     ="
				" idni::lit<char_type, terminal_type>;\n"
		"	using location_type   = std::array<size_t, 2>;\n"
		"	using node_type       ="
				" std::pair<symbol_type, location_type>;\n"
		"	using parser_type     ="
				" idni::parser<char_type, terminal_type>;\n"
		"	using options         = parser_type::options;\n"
		"	using parse_options   = parser_type::parse_options;\n"
		"	using forest_type     = parser_type::pforest;\n"
		"	using sptree_type     = parser_type::psptree;\n"
		"	using input_type      = parser_type::input;\n"
		"	using decoder_type    ="
				" parser_type::input::decoder_type;\n"
		"	using encoder_type    = "
				"std::function<std::basic_string<char_type>(\n"
		"			const std::vector<terminal_type>&)>;\n"
		" static " <<opt.name<< "& instance() { static " <<opt.name<< " i; return i; }"
		"	" <<opt.name<< "() :\n"
		"		nts(load_nonterminals()), cc(load_cc()),\n"
		"		g(nts, load_prods(), nt(" <<gi.start().n()<<
				"), cc, load_grammar_opts()),\n"
		"		p(g, load_opts()) {}\n"
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
		"	sptree_type shape(forest_type* f, const node_type& n) {\n"
		"		idni::tree_shaping_options opt;\n"
		"		opt.to_trim = g.opt.to_trim;\n"
		"		opt.to_trim_children = g.opt.to_trim_children;\n"
		"		opt.trim_terminals = g.opt.trim_terminals;\n"
		"		opt.to_inline = g.opt.to_inline;\n"
		"		opt.inline_char_classes = g.opt.inline_char_classes;\n"
		"		return f->get_shaped_tree(n, opt);\n"
		"	}\n"
		"	sptree_type parse_and_shape(const char_type* data, size_t size,\n"
		"		const node_type& n, parse_options po = {})\n"
		"	{\n"
		"		return shape(p.parse(data, size, po).get(), n);\n"
		"	}\n"
		"	sptree_type parse_and_shape(const char_type* data, size_t size,\n"
		"		parse_options po = {})\n"
		"	{\n"
		"		auto f = p.parse(data, size, po);\n"
		"		return shape(f.get(), f->root());\n"
		"	}\n"
		"	sptree_type parse_and_shape(std::basic_istream<char_type>& is,\n"
		"		const node_type& n, parse_options po = {})\n"
		"	{\n"
		"		return shape(p.parse(is, po).get(), n);\n"
		"	}\n"
		"	sptree_type parse_and_shape(std::basic_istream<char_type>& is,\n"
		"		parse_options po = {})\n"
		"	{\n"
		"		auto f = p.parse(is, po);\n"
		"		return shape(f.get(), f->root());\n"
		"	}\n"
		"	sptree_type parse_and_shape(const std::string& fn,\n"
		"		const node_type& n, parse_options po = {})\n"
		"	{\n"
		"		return shape(p.parse(fn, po).get(), n);\n"
		"	}\n"
		"	sptree_type parse_and_shape(const std::string& fn,\n"
		"		parse_options po = {})\n"
		"	{\n"
		"		auto f = p.parse(fn, po);\n"
		"		return shape(f.get(), f->root());\n"
		"	}\n"
		"#ifndef WIN32\n"
		"	sptree_type parse_and_shape(int fd, const node_type& n, parse_options po = {})\n"
		"	{\n"
		"		return shape(p.parse(fd, po).get(), n);\n"
		"	}\n"
		"	sptree_type parse_and_shape(int fd, parse_options po = {})\n"
		"	{\n"
		"		auto f = p.parse(fd, po);\n"
		"		return shape(f.get(), f->root());\n"
		"	}\n"
		"#endif //WIN32\n"
		"	bool found(size_t start = SIZE_MAX) { return p.found(start); }\n"
		"	typename parser_type::error get_error() { return p.get_error(); }\n"
		"	enum nonterminal {" << gen_nts_enum_cte() <<
								"	};\n"
		"	size_t id(const std::basic_string<char_type>& name) {\n"
		"		return nts.get(name);\n"
		"	}\n"
		"	const std::basic_string<char_type>& name(size_t id) {\n"
		"		return nts.get(id);\n"
		"	}\n"
		"	symbol_type literal(const nonterminal& nt) {\n"
		"		return symbol_type(nt, &nts);\n"
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
		"	grammar_type::options load_grammar_opts() {\n"
		"		grammar_type::options o;\n"
		"		o.transform_negation = false;\n" <<
				gen_grammar_opts() <<
		"		return o;\n"
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
