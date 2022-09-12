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

// template declarations

template <typename CharT>
parser<CharT> g_parser();
template <typename CharT>
typename parser<CharT>::grammar g_grammar();
template <typename CharT>
typename parser<CharT>::grammar transform_parsed_g_to_grammar(parser<CharT>& p);
template <typename CharT>
ostream_t& grammar_to_cpp(ostream_t& ss,
	const typename parser<CharT>::grammar& g, bool basic_char = false);
template <typename CharT>
std::string grammar_to_cpp(const typename parser<CharT>::grammar& g,
	bool basic_char = false);
template <typename CharT>
bool load_g(const std::string& filename, typename parser<CharT>::grammar& g);

// template definitions

template <typename CharT>
constexpr typename parser<CharT>::string S(const char* s) {
	return from_cstr<CharT>(s);
}

//#define S(x) (from_cstr<CharT>(x))

template <typename CharT>
parser<CharT> g_parser() {
	typename parser<CharT>::parser_options o;
	o.cc_fns = { "eof", "alpha", "alnum", "printable", "space" };
	auto g = g_grammar<CharT>();
	auto p = parser<CharT>(g, o);
	//DBG(p.print_grammar(std::cout << "g grammar: ", g) << "\n\n";)
	return p;
	//return parser<CharT>(g_grammar<CharT>(), o);
}
template <>
parser<char32_t>::grammar g_grammar<char32_t>() {
	typename parser<char32_t>::grammar g
#include "g.grammar.inc.h"
	return g;
}
template <>
parser<char>::grammar g_grammar<char>() {
	typename parser<char>::grammar g
#include "g.grammar_char.inc.h"
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
			//DBG(std::cout << "  add_element: " << to_std_string(e)
			//	<< " head: " << head << " in_factor: "
			//	<< in_factor << "\n";)
			if (e == S<CharT>("null")) e = S<CharT>("\0");
			if (head) r.first = e, head = false;
			else if (in_factor) f.back().push_back(e);
			else r.second.back().push_back(e);
		}
		void alt_sep() { r.second.emplace_back();
			//DBG(std::cout << "  alt_sep\n";)
		}
		void new_rule() { head = true, r = { {}, { {} } }; }
			//DBG(std::cout << "new_rule\n";) }
		void new_factor() { in_factor = true,
			f.emplace_back(), ft.emplace_back(SINGLE); }
			//DBG(std::cout << "  new_factor\n";) }
		void add_new_factor(const string& nn) {
			(f.size() ? f.back() : r.second.back())
				.push_back(nn);
		}
		void save_factor() {
			//DBG(std::cout << "  save_factor\n";)
			auto lf =  f.back();  f.pop_back();
			auto lt = ft.back(); ft.pop_back();
			strings nll = { S<CharT>("") };
			in_factor = !f.size();
//#ifdef DEBUG
//			std::cout << "\tR: " << to_std_string(r.first)<<" lf: ";
//			for (auto& t : lf)
//				std::cout << '"' << to_std_string(t) << "\" ";
//			std::cout << "| unot: " << (int_t) ut
//				<< " lt: " << (int_t) lt << "\n";
//#endif
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
		void save_rule() {
			//DBG(std::cout << "save_rule\n";)
			g.push_back(r);
		}
		void group() {
			ft.back() = GROUP;
			//DBG(std::cout << "  ft: " << (int_t) ft.back()<<"\n";)
		}
		void optional() {
			ft.back() = OPTIONAL;
			//DBG(std::cout << "  ft: " << (int_t) ft.back()<<"\n";)
		}
		void plus() {
			ft.back() = ATLEASTONE;
			//DBG(std::cout << "  ft: " << (int_t) ft.back()<<"\n";)
		}
		void unot(string c) {
			//DBG(std::cout << "  unot: " << (int_t) ut << "\n";)
			ut = c == S<CharT>("+") ? PLUS : c == S<CharT>("*") ? MULTI : NONE;
		};
		string get_new_name() {
			std::stringstream ss;
			ss << "_R" << to_std_string(r.first) << "_" << id++;
			//DBG(std::cout << "  new_name: " << ss.str() << "\n";)
			return from_str<CharT>(ss.str());
		}
	} x;
	auto cb_enter = [&p, &x](const auto& n) { // entering a node
		if (!n.first.nt()) return;
		//DBG(std::cout << "entering " << (n.first.nt() ? "NT" : " T")
		//	<< ": `" << to_std_string(p.to_string(n.first))<< "`\n";)
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
			str.erase(str.begin());
			str.erase(str.end() - 1);
			x.add_element(str);
		}
		else if (s == S<CharT>("constraints"))   x.is_constraint = true;
		else if (s == S<CharT>("t_t_factor") ||
			 s == S<CharT>("nt_t_factor") ||
			 s == S<CharT>("nt_nt_factor"))    x.new_factor();
	};
	auto cb_exit = [&p, &x](const auto& n, const auto&) {
		if (!n.first.nt()) return;
		//DBG(std::cout << "leaving " << (n.first.nt() ? "NT" : " T") <<
		//	": `" << to_std_string(p.to_string(n.first))<< "`\n";)
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
	ss << "{";
	for (auto& r : g) {
		ss << "\n\t{ " << u <<"\"" << to_std_string(r.first) << "\", {";
		for (auto& a : r.second) {
			ss << "\n\t\t{ ";
			for (auto& t : a) {
				ss << u << "\"";
				if (t != S<CharT>("null"))
					ss << to_std_string(t);
				ss << "\"";
				/*if (t != a.back())*/ ss << ", ";
			}
			ss << " }";
			/*if (a != r.second.back())*/ ss << ",";
		}
		ss << "\n\t} }";
		/*if (r != g.back())*/ ss << ",";
	}
	return ss << "\n};\n";
}
template <typename CharT>
bool load_g(const std::string& filename, typename parser<CharT>::grammar& g) {
	std::ifstream s(filename);
	if (!s.is_open()) return std::cerr << "failed to open grammar from: `"
		<< filename << "`\n", false;
	std::stringstream ss; ss << s.rdbuf();
	//std::cout << "grammar: `" << ss.str() << "`\n";
	auto gp = g_parser<char32_t>();
	if (!gp.recognize(from_str<CharT>(ss.str()))) return
		std::cerr<<"grammar: `"<<filename<<"` not recognized", false;
	g = transform_parsed_g_to_grammar(gp);
	//DBG(gp.print_grammar(std::cout << "grammar: ", g) << "\n\n";)
	return true;
}

}} // idni::glanguage namespace
#endif // __IDNI__PARSER__GLANGUAGE_H__
