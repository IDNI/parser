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

template <typename C = char, typename T = C>
std::ostream& generate_parser_cpp(std::ostream& os, const std::string& name,
	const std::string& tgf_filename,
	const grammar<C, T>& g,
	const std::string& char_type = "char",
	const std::string& terminal_type = "char",
	const std::string& decoder = "",
	const std::string& encoder = "")
{
	if (g.size() == 0) return os;
	std::stringstream ss; ss << char_type;
	if (terminal_type.size()) ss << ", " << terminal_type;
	std::string tn = ss.str();
	std::string U = terminal_type == "char32_t" ? "U" : "";
	std::vector<C> ts{ (C)0 };
	grammar_inspector<C, T> gi(g);
	// convert name to upper case and append __PARSER_GEN_H__ as a guard at the begining
	std::string guard = name;
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
	auto gen_opts = [&decoder, &encoder]() {
		std::stringstream os;
		if (decoder.size()) os << "\t\to.chars_to_terminals = "
			<< decoder << ";\n";
		if (encoder.size()) os << "\t\to.terminals_to_chars = "
			<< encoder << ";\n";
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
	const auto ps = gen_prods();
	os <<	"// This file is generated from a file " <<
					strip_pwd(tgf_filename)<<" by\n"
		"//       https://github.com/IDNI/parser/tools/tgf\n"
		"//\n"
		"#ifndef __" <<guard<< "_H__\n"
		"#define __" <<guard<< "_H__\n"
		"#include <string.h>\n"
		"#include \"parser.h\"\n"
		"struct " <<name<< " {\n"
		"	" <<name<< "() :\n"
		"		nts(load_nonterminals()), cc(load_cc()),\n"
		"		g(nts, load_prods(), nt(" <<gi.start().n()<<
				"), cc), p(g, load_opts()) {}\n"
		"	std::unique_ptr<typename idni::parser<" <<tn<< ">"
							"::pforest> parse(\n"
		"		const "<<char_type<<"* data, size_t size = 0, "
						"size_t max_l = 0,\n"
		"		" <<char_type<< " eof = std::char_traits<"
						<<char_type<<">::eof())\n"
		"		\t{ return p.parse(data, size, max_l, eof); }\n"
		"	std::unique_ptr<typename idni::parser<" <<tn<< ">"
							"::pforest> parse(\n"
		"		std::basic_istream<" <<char_type<< ">& is, "
							"size_t max_l = 0,\n"
		"		" <<char_type<< " eof = std::char_traits<"
						<<char_type<< ">::eof())\n"
		"			{ return p.parse(is, max_l, eof); }\n"
		"	std::unique_ptr<typename idni::parser<" <<tn<< ">"
							"::pforest> parse(\n"
		"	\tstd::string fn, mmap_mode m, size_t max_l = 0,\n"
		"		" <<char_type<< " eof = std::char_traits<"
						<<char_type<< ">::eof())\n"
		"		\t{ return p.parse(fn, m, max_l, eof); }\n"
		"#ifndef WIN32\n"
		"	std::unique_ptr<typename idni::parser<" <<tn<< ">"
							"::pforest> parse(\n"
		"		int fd, size_t max_l = 0,\n"
		"		" <<char_type<< " eof = std::char_traits<"
						<<char_type<< ">::eof())\n"
		"			{ return p.parse(fd, max_l, eof); }\n"
		"#endif //WIN32\n"
		"	bool found() { return p.found(); }\n"
		"	typename idni::parser<" <<tn<< ">::error "
								"get_error()\n"
		"		{ return p.get_error(); }\n"
		"	enum nonterminal {" << gen_nts_enum_cte() <<
								"	};\n"
		"	size_t id(const std::basic_string<"<<char_type<<">& "
					"name) { return nts.get(name); }\n"
		"private:\n"
		"	std::vector<" <<terminal_type<< "> ts{\n" <<
				gen_ts() <<
		"	};\n"
		"	idni::nonterminals<" <<tn<< "> nts{};\n"
		"	idni::char_class_fns<" <<terminal_type<< "> cc;\n"
		"	idni::grammar<" <<tn<< "> g;\n"
		"	idni::parser<" <<tn<< "> p;\n"
		"	idni::prods<" <<tn<< "> t(size_t tid) {\n"
		"		return idni::prods<" <<tn<< ">(ts[tid]);\n"
		"	}\n"
		"	idni::prods<" <<tn<<"> nt(size_t ntid) {\n"
		"		return idni::prods<"<<tn<< ">("
					"idni::lit<"<<tn<<">(ntid, &nts));\n"
		"	}\n"
		"	idni::nonterminals<" <<tn<< "> load_nonterminals() "
								"const {\n"
		"		idni::nonterminals<" <<tn<< "> nts{};\n"
		"		for (const auto& nt : {" <<
					gen_nts() <<
		"		}) nts.get(nt);\n"
		"		return nts;\n"
		"	}\n"
		"	idni::char_class_fns<"<<terminal_type<<"> load_cc() {\n"
		"		return idni::predefined_char_classes<" <<tn<<
								">({\n" <<
					gen_cc_fns() <<
		"		}, nts);\n"
		"	}\n"
		"	idni::parser<" <<tn<< ">::options load_opts() {\n"
		"		idni::parser<" <<tn<< ">::options o;\n" <<
					gen_opts() <<
		"		return o;\n"
		"	}\n"
		"	idni::prods<" <<tn<< "> load_prods() {\n"
		"		idni::prods<" <<tn<< "> q, nul(idni::lit<" <<
					tn<< ">{});\n" << ps <<
		"		return q;\n"
		"	}\n"
		"};\n"
		"#endif // __" << guard << "_H__\n";
	return os;
}

template <typename C = char, typename T = C>
std::ostream& generate_parser_cpp_from_string(std::ostream& os,
	const std::string& name,
	const std::basic_string<C>& grammar_tgf,
	const std::basic_string<C>& start_nt = from_cstr<C>("start"),
	const std::string& char_type = "char",
	const std::string& terminal_type = "",
	const std::string& decoder = "",
	const std::string& encoder = "")
{
	nonterminals<C, T> nts;
	return generate_parser_cpp(os, name,
		tgf<C, T>::from_string(nts, grammar_tgf, start_nt),
		char_type, terminal_type, decoder, encoder);
}

template <typename C = char, typename T = C>
void generate_parser_cpp_from_file(std::ostream& os, const std::string& name,
	const std::string& tgf_filename,
	const std::basic_string<C>& start_nt = from_cstr<C>("start"),
	const std::string& char_type = "char",
	const std::string& terminal_type = "",
	const std::string& decoder = "",
	const std::string& encoder = "")
{
	nonterminals<C, T> nts;
	generate_parser_cpp(os, name, tgf_filename,
		tgf<C, T>::from_file(nts, tgf_filename, start_nt),
		char_type, terminal_type, decoder, encoder);
}

} // idni namespace
#endif //__IDNI__PARSER__PARSER_GEN_H__
