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
#ifndef __IDNI__PARSER__GLANGUAGE_H__
#define __IDNI__PARSER__GLANGUAGE_H__
#include <fstream>
#include "parser.h"
namespace idni { namespace glanguage {

#ifndef WITH_CHARCLASSES
#error glanguage.h requires WITH_CHARCLASSES to be defined
#endif

// template declarations

// returns parser of g language
template <typename CharT> parser<CharT> g_parser();
// returns grammar of g language
template <typename CharT> typename parser<CharT>::grammar g_grammar();
// transforms parsed forest of a g language into a grammar structure
template <typename CharT> typename parser<CharT>::grammar
	transform_parsed_g_to_grammar(parser<CharT>& p);
// transforms provided grammar to a cpp code, stream interface
// if basic_char is true, exports char parsers, otherwise for char32_t parsers
template <typename CharT> ostream_t& grammar_to_cpp(ostream_t& ss,
	const typename parser<CharT>::grammar& g, bool basic_char = false);
// transforms provided grammar `g` to a cpp code, returns string
// if basic_char is true, exports char parsers, otherwise for char32_t parsers
template <typename CharT> std::string grammar_to_cpp(
	const typename parser<CharT>::grammar& g, bool basic_char = false);
// loads a grammar into `g` of a language defined in a g language in `filename` 
// returns true if success and false if loading failed
template <typename CharT> bool load_g(const std::string& filename,
	typename parser<CharT>::grammar& g);

// template definitions

// convert const char* string into parser's string<CharT> at compile time
template <typename CharT>
constexpr typename parser<CharT>::string S(const char* s) {
	return from_cstr<CharT>(s);
}

template <typename CharT>
parser<CharT> g_parser() {
	typename parser<CharT>::parser_options o;
	o.cc_fns = { "eof", "alpha", "alnum", "printable", "space" };
	return parser<CharT>(g_grammar<CharT>(), o);
}
template <>
parser<char32_t>::grammar g_grammar<char32_t>() {
	typename parser<char32_t>::grammar g
	#include "glanguage/g.grammar.inc.h"
	return g;
}
template <>
parser<char>::grammar g_grammar<char>() {
	typename parser<char>::grammar g
	#include "glanguage/g.grammar_char.inc.h"
	return g;
}
template <typename CharT>
typename parser<CharT>::grammar transform_parsed_g_to_grammar(parser<CharT>& p){
	typedef typename parser<CharT>::grammar grammar;
	typedef typename parser<CharT>::grammar_rule grammar_rule;
	typedef typename parser<CharT>::string string;
	typedef typename parser<CharT>::strings strings;
	struct {
		enum ftype { SINGLE, GROUP, OPTIONAL, ATLEASTONE };//factor type
		enum utype { NONE, PLUS, MULTI } ut = NONE; // type of unot
		grammar g;                  // resulting grammar
		grammar_rule r;             // current rule transformed 
		std::vector<strings> f;     // factors (with nesting) 
		std::vector<ftype> ft;      // types of factors
		bool head = true;           // first sym in rule is head
		bool in_factor = false;     // are we in a factor
		bool is_constraint = false; // are we in a constraint
		size_t id = 0;              // id of the new term
		void add_element(string e) {
			if (e == S<CharT>("null")) e = S<CharT>("\0");
			if (head) r.first = e, head = false;
			else if (in_factor) f.back().push_back(e);
			else r.second.back().push_back(e);
		}
		void alt_sep() { r.second.emplace_back(); }
		void new_rule() { head = true, r = { {}, { {} } }; }
		void new_factor() { in_factor = true,
			f.emplace_back(), ft.emplace_back(SINGLE); }
		void add_new_factor(const string& nn) {
			(f.size() ? f.back() : r.second.back())
				.push_back(nn); }
		void save_factor() {
			auto lf =  f.back();  f.pop_back();
			auto lt = ft.back(); ft.pop_back();
			strings nll = { S<CharT>("") };
			in_factor = !f.size();
			if (lt == SINGLE && ut != PLUS && ut != MULTI) {
				add_new_factor(lf[0]);
				return;
			}
			auto nn = get_new_name();
			if (lt == GROUP && ut == PLUS) lt = ATLEASTONE;
			if (lt == SINGLE) {
				if (ut == PLUS)	g.push_back(
					{ nn, { { lf[0], nn }, { lf[0] } } });
				else if (ut == MULTI) g.push_back(
					{ nn, { { lf[0], nn }, nll } });
			} else if (lt == OPTIONAL) {
				g.push_back({ nn, { lf, nll } });
			} else if (lt == GROUP) {
				if (ut == MULTI) {
					auto lf_nn = lf; lf_nn.push_back(nn);
					g.push_back({ nn, { lf_nn, nll } });
				} else g.push_back({ nn, { lf } });
			} else if (lt == ATLEASTONE) {
				auto lf_nn = lf; lf_nn.push_back(nn);
				g.push_back({ nn, { lf_nn, lf } });
			}
			add_new_factor(nn);
		}
		void save_rule() { g.push_back(r); }
		void group() {    ft.back() = GROUP; }
		void optional() { ft.back() = OPTIONAL; }
		void plus() {     ft.back() = ATLEASTONE; }
		void unot(string c) { ut = c == S<CharT>("+") ? PLUS
					: c == S<CharT>("*") ? MULTI : NONE; };
		string get_new_name() {
			std::stringstream ss;
			ss << "_R" << to_std_string(r.first) << "_" << id++;
			return from_str<CharT>(ss.str());
		}
	} x;
	auto cb_enter = [&p, &x](const auto& n) {
		if (!n.first.nt()) return;
		const auto& s = p.dict(n.first.n());
		if      (s == S<CharT>("production"))      x.new_rule();
		else if (s == S<CharT>("alt_separator"))   x.alt_sep();
		else if (s == S<CharT>("sym"))      x.add_element(p.flatten(n));
		else if (s == S<CharT>("unot"))            x.unot(p.flatten(n));
		else if (s == S<CharT>("factor_group"))    x.group();
		else if (s == S<CharT>("factor_optional")) x.optional();
		else if (s == S<CharT>("factor_plus"))     x.plus();
		else if (s == S<CharT>("string") ||
			 s == S<CharT>("quoted_char"))
		{
			auto str = p.flatten(n);
			str.erase(str.begin()), str.erase(str.end() - 1);
			if (s == S<CharT>("quoted_char") && str.size() > 1) {
				switch (str[1]) {
					case 'r':  str = S<CharT>("\r"); break;
					case 'n':  str = S<CharT>("\n"); break;
					case 't':  str = S<CharT>("\t"); break;
					default: str = str[1];
				}
			}
			x.add_element(str);
		}
		else if (s == S<CharT>("constraints")) x.is_constraint = true;
		else if (s == S<CharT>("t_t_factor") ||
			 s == S<CharT>("nt_t_factor") ||
			 s == S<CharT>("nt_nt_factor")) x.new_factor();
	};
	auto cb_exit = [&p, &x](const auto& n, const auto&) {
		if (!n.first.nt()) return;
		const auto& s = p.dict(n.first.n());
		if      (s == S<CharT>("production"))      x.save_rule();
		else if (s == S<CharT>("constraints"))  x.is_constraint = false;
		else if (s == S<CharT>("t_t_factor") ||
			 s == S<CharT>("nt_t_factor") ||
			 s == S<CharT>("nt_nt_factor"))    x.save_factor();
	};
	p.parsed_forest().traverse(p.root(), cb_enter, cb_exit);
	return x.g;
}

template <typename CharT>
std::string grammar_to_cpp(const typename parser<CharT>::grammar& g,
	bool basic_char)
{
	std::stringstream ss;
	g_to_cpp(ss, g, basic_char);
	return ss.str();
}
template <typename CharT>
ostream_t& grammar_to_cpp(ostream_t& ss,
	const typename parser<CharT>::grammar& g, bool basic_char)
{
	std::string u = basic_char ? "" : "U";
	auto escape = [](char s) -> std::string {
		std::string r({ s });
		return    s == '\r' ? "\\r"
			: s == '\n' ? "\\n"
			: s == '\t' ? "\\t"
			: s == '"' ?  "\\\""
			: s == '\\' ? "\\\\" : r.c_str();
	};
	ss << "{";
	size_t i = 0;
	for (auto& r : g) {
		if (i != 0) ss << ","; else i++;
		ss << "\n\t{ " << u <<"\"" << to_std_string(r.first) << "\", {";
		size_t j = 0;
		for (auto& a : r.second) {
			if (j != 0) ss << ","; else j++;
			ss << "\n\t\t{ ";
			size_t k = 0;
			for (auto& t : a) {
				if (k != 0) ss << ", "; else k++;
				ss << u << "\"";
				if (t.size() == 1)
					ss << escape(to_std_string(t)[0]);
				else  if (t != S<CharT>("null"))
					ss << to_std_string(t);
				ss << "\"";
			}
			ss << " }";
		}
		ss << "\n\t} }";
	}
	return ss << "\n};\n";
}
template <typename CharT>
bool load_g(const std::string& filename, typename parser<CharT>::grammar& g) {
	std::ifstream s(filename);
	if (!s.is_open()) return std::cerr << "failed to open grammar from: `"
		<< filename << "`\n", false;
	std::stringstream ss; ss << s.rdbuf();
	auto gp = g_parser<char32_t>();
	if (!gp.recognize(from_str<CharT>(ss.str()))) return
		std::cerr<<"grammar: `"<<filename<<"` not recognized", false;
	g = transform_parsed_g_to_grammar(gp);
	return true;
}

}} // idni::glanguage namespace
#endif // __IDNI__PARSER__GLANGUAGE_H__
