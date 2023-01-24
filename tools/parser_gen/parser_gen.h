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
#include "parser.h"
namespace idni {

template <typename CharT>
struct grammar_inspector { // provides access to private grammar members
	grammar_inspector(const grammar<CharT>& g) : g(g) {}
	const grammar<CharT>& g;
	lit<CharT> start() { return g.start; }
	nonterminals<CharT>& nts() { return g.nts; }
	const std::vector<std::pair<lit<CharT>, std::vector<lits<CharT>>>>& G(){
		return g.G; }
	const char_class_fns<CharT>& cc_fns() { return g.cc_fns; }
};

template <typename CharT> std::string char_type();
template<> std::string char_type<char>()     { return "char"; }
template<> std::string char_type<char32_t>() { return "char32_t"; }

template <typename CharT>
std::ostream& generate_parser_cpp(std::ostream& os, const std::string& name,
	const grammar<CharT>& g)
{
	if (g.size() == 0) return os;
	const std::string cht = char_type<CharT>();
	std::string U = cht == "char32_t" ? "U" : "";
	std::vector<CharT> ts{ (CharT)0 };
	grammar_inspector<CharT> gi(g);
	auto gen_ts = [&ts, &U]() {
		std::stringstream os;
		os << "\t\t";
		size_t n = 0;
		for (const CharT ch : ts) {
			if (++n == 10) n = 0, os << "\n\t\t";
			os << U <<"'" <<
				( ch == (CharT)    0 ? "\\0"
				: ch == (CharT) '\r' ? "\\r"
				: ch == (CharT) '\n' ? "\\n"
				: ch == (CharT) '\t' ? "\\t"
				: ch == (CharT) '\\' ? "\\\\"
				: ch == (CharT) '\'' ? "\\'"
				: to_std_string(ch)) << "', ";
		}
		os << "\n";
		return os.str();
	};
	auto gen_nts = [&gi, &U]() {
		std::stringstream os;
		auto& x = gi.nts();
		for (size_t n = 0; n != x.size(); ++n)
			os << (n % 10 == 0 ? "\n\t\t\t" : "") <<
				U << "\"" << to_std_string(x[n]) << "\", ";
		os << "\n";
		return os.str();
	};
	auto gen_cc_fns = [&gi, &U]() {
		std::stringstream os;
		auto& x = gi.nts();
		for (const auto& fn : gi.cc_fns().fns)
			if (x[fn.first].size()) os<<"\t\t\t"<<U<<"\""
				<< to_std_string(x[fn.first]) << "\",\n";
		return os.str();
	};
	auto gen_prods = [&gi, &ts]() {
		std::stringstream os;
		auto terminal = [&ts](const lit<CharT>& l) {
			for (size_t n = 0; n != ts.size(); ++n)
				if (ts[n] == l.c()) return n;
			ts.push_back(l.c());
			return ts.size()-1;
		};
		for (const auto& p : gi.G()) {
			os << "\t\tq(nt(" << p.first.n() << "), ";
			bool first_c = true;
			for (const auto& c : p.second) {
				if (!first_c) os << " & "; else first_c = false;
				os << '(';
				bool first_l = true;
				for (const auto& l : c) {
					if (!first_l) os << "+";
					else first_l = false;
					if (l.nt()) os << "nt(" << l.n() << ")";
					else os << "t(" << terminal(l) << ")";
				}
				os << ')';
			}
			os << ");\n";
		}
		return os.str();
	};
	const auto ps = gen_prods();
	os <<	"// This file is generated by IDNI/parser/tools/parser_gen\n" <<
		"#include <string.h>\n" <<
		"#include \"parser.h\"\n" <<
		"struct " << name << " {\n" <<
		"	" << name << "() :\n" <<
		"		nts(load_nonterminals()), cc(load_cc()),\n" <<
		"		g(nts, load_prods(), nt(" << gi.start().n() <<
					"), cc), p(g) { }\n"
		"	std::unique_ptr<typename idni::parser<" <<cht<< ">"
							"::pforest> parse(\n"
		"		const " <<cht<< "* data, size_t size = 0,\n"
		"		" <<cht<< " eof = std::char_traits<" <<cht<< ">"
								"::eof())\n"
		"			{ return p.parse(data, size, eof); }\n"
		"	std::unique_ptr<typename idni::parser<" <<cht<< ">"
							"::pforest> parse(\n"
		"		int filedescriptor, size_t size = 0,\n"
		"		" <<cht<< " eof = std::char_traits<" <<cht<< ">"
								"::eof())\n"
		"			{ return p.parse(filedescriptor, size, "
								"eof); }\n"
		"	std::unique_ptr<typename idni::parser<" <<cht<< ">"
							"::pforest> parse(\n"
		"		std::basic_istream<" <<cht<< ">& is,\n"
		"		size_t size = 0,\n"
		"		" <<cht<< " eof = std::char_traits<" <<cht<< ">"
								"::eof())\n"
		"			{ return p.parse(is, size, eof); }\n"
		"	bool found() { return p.found(); }\n"
		"	typename idni::parser<" <<cht<< ">::perror_t "
								"get_error()\n"
		"		{ return p.get_error(); }\n"
		"private:\n"
		"	std::vector<" <<cht<< "> ts{\n" << 
				gen_ts() <<
		"	};\n"
		"	idni::nonterminals<" <<cht<< "> nts{};\n"
		"	idni::char_class_fns<" <<cht<< "> cc;\n"
		"	idni::grammar<" <<cht<< "> g;\n"
		"	idni::parser<" <<cht<< "> p;\n"
		"	idni::prods<" <<cht<< "> t(size_t tid) {\n"
		"		return idni::prods<" <<cht<< ">(ts[tid]);\n"
		"	}\n"
		"	idni::prods<" <<cht<<"> nt(size_t ntid) {\n"
		"		return idni::prods<"<<cht<< ">("
					"idni::lit<"<<cht<<">(ntid, &nts));\n"
		"	}\n"
		"	idni::nonterminals<" <<cht<< "> load_nonterminals() "
								"const {\n"
		"		idni::nonterminals<" <<cht<< "> nts{};\n"
		"		for (const auto& nt : {" << 
					gen_nts() <<
		"		}) nts.get(nt);\n"
		"		return nts;\n"
		"	}\n"
		"	idni::char_class_fns<" <<cht<< "> load_cc() {\n"
		"		return idni::predefined_char_classes<" <<cht<<
								">({\n" <<
					gen_cc_fns() <<
		"		}, nts);\n"
		"	}\n"
		"	idni::prods<" <<cht<< "> load_prods() {\n"
		"		idni::prods<" <<cht<< "> q;\n" <<
					gen_prods() <<
		"		return q;\n"
		"	}\n"
		"};\n";	
	return os;
}

template <typename CharT>
std::ostream& generate_parser_cpp(std::ostream& os,
	const std::string& name,
	const std::basic_string<CharT>& grammar_tgf,
	const std::basic_string<CharT>& start_nt = from_cstr<CharT>("start"))
{
	nonterminals<CharT> nts;
	return generate_parser_cpp(os, name,
		tgf<CharT>::from_string(nts, grammar_tgf, start_nt));
}

template <typename CharT>
void generate_parser_cpp(std::ostream& os, const std::string& name,
	const std::string& tgf_filename,
	const std::basic_string<CharT>& start_nt = from_cstr<CharT>("start"))
{
	nonterminals<CharT> nts;
	generate_parser_cpp(os, name,
		tgf<CharT>::from_file(nts, tgf_filename, start_nt));
}

} // idni namespace
#endif //__IDNI__PARSER__PARSER_GEN_H__