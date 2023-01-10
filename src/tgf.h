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
#ifndef __IDNI__PARSER__TGF_H__
#define __IDNI__PARSER__TGF_H__
#include "parser.h"
namespace idni {

template <typename CharT>
struct tgf {
	typedef CharT char_t;
	typedef std::basic_string<CharT> string;
	typedef std::vector<string> strings;
	typedef typename parser<CharT>::pforest forest_t;
	tgf() :
		cc(predefined_char_classes({ U("eof"), U("alpha"), U("alnum"),
			U("digit"), U("printable"), U("space") }, nts)),
		alnum(nt("alnum")),
		alpha(nt("alpha")),
		char_(nt("char")),
		char0(nt("char0")),
		chars(nt("chars")),
		chars1(nt("chars1")),
		conjunction(nt("conjunction")),
		command(nt("command")),
		commands(nt("commands")),
		commands_rest(nt("commands_rest")),
		directive(nt("directive")),
		directive_param(nt("directive_param")),
		directive_params(nt("directive_params")),
		disjunction(nt("disjunction")),
		eof(nt("eof")),
		eol(nt("eol")),
		expr1(nt("expr1")),
		expr2(nt("expr2")),
		expr3(nt("expr3")),
		group(nt("group")),
		literal(nt("literal")),
		literals(nt("literals")),
		literals1(nt("literals1")),
		multi(nt("multi")),
		nonliteral(nt("nonliteral")),
		nonterminal(nt("nonterminal")),
		negation(nt("negation")),
		optional(nt("optional")),
		plus(nt("plus")),
		printable(nt("printable")),
		printable_chars(nt("printable_chars")),
		printable_chars1(nt("printable_chars1")),
		production_(nt("production")),
		production_sep(nt("production_sep")),
		quoted_char(nt("quoted_char")),
		quoted_char_esc(nt("quoted_char_esc")),
		repeat(nt("repeat")),
		space(nt("space")),
		start(nt("start")),
		string_(nt("string")),
		string_char(nt("string_char")),
		string_chars(nt("string_chars")),
		string_chars1(nt("string_chars1")),
		sym(nt("sym")),
		terminal(nt("terminal")),
		ws(nt("ws")),
		ws_comment(nt("ws_comment")),
		ws_required(nt("ws_required")),
		g(nts, setup_productions(), start, cc), p(g) { }
	static grammar<CharT> from_string(nonterminals<CharT>& nts_,
		const string& s, const string& start_nt = "start")
	{
		tgf<CharT> f;
		//DBG(f.g.print_data(cout << "\n>>>\n\n") << "\n<<<" << std::endl;)
		return f.parse(nts_, s, start_nt);
	}
	grammar<CharT> parse(nonterminals<CharT>& nts_, const string& s,
		const string& start_nt = "start")
	{
		struct context {
			enum ftype { SIMPLE, GROUP, OPTIONAL, REPEAT };
			enum utype { NONE, PLUS, MULTI } ut = NONE; // type of unot
			prods<CharT> ps{}, nll{'\0'};
			lit<CharT> p_head{};
			std::vector<prods<CharT>> p{};
			std::vector<string> cc_names{};
			bool head = true;       // first sym in rule is head
			bool in_directive = false;
			nonterminals<CharT>& nts;
			char_class_fns<CharT> cc;
			std::vector<alt<CharT>> f; // factors (with nesting) 
			std::vector<ftype> ft;  // types of factors
			context(nonterminals<CharT>& nts) : nts(nts) {}
			size_t id = 0;          // id of the new term
			lit<CharT> nt(const string& s) {
				return lit<CharT>{
					nts.get(from_str<CharT>(s)), &nts };
			};
			void add_literal(const lit<CharT>& l) {
				//DBG(print_data();)
				//DBG(std::cout << "add_literal: " << l.to_std_string() << std::endl;)
				if (head) p_head = l, p = {}, head = false;
				else p.back() = p.back() + prods<CharT>{ l };
				//DBG(print_data();)
			}
			void add_nonterminal(const string& s) {
				if (in_directive) return;
				//DBG(std::cout << "add_nonterminal: " << to_std_string(s) << std::endl;)
				if (s == from_cstr<CharT>("null"))
					add_literal(lit<CharT>{'\0'});
				else add_literal(lit<CharT>{nt(s)});
			}
			void add_terminal(const CharT& ch) {
				//DBG(std::cout << "add_terminal: " << to_std_string(ch) << std::endl;)
				add_literal(lit<CharT>{ch});
			}
			void directive() {
				in_directive = false;
			}
			
			void push() {
				//DBG(cout << "push" << "\n";)
				p.push_back({});
				//DBG(print_data();)
			}
			void pop() {
				//DBG(cout << "pop" << "\n";)
				if (p.back().size() == 0) p.pop_back();
				//DBG(print_data();)
			}
			void new_production() {
				if (cc_names.size()) cc =
					predefined_char_classes(cc_names, nts),
					cc_names.clear();
			}
			void disjunction() {
				//DBG(print_data();)
				//DBG(std::cout << "disjunction" << std::endl;)
				p[p.size()-2] = p[p.size()-2] | p.back();
				p.pop_back();
				//DBG(print_data();)
			}
			void conjunction() {
				//DBG(print_data();)
				//DBG(std::cout << "conjunction" << std::endl;)
				p[p.size()-2] = p[p.size()-2] & p.back();
				p.pop_back();
				//DBG(print_data();)
			}
			void production() {
				//DBG(std::cout << "production: " << std::endl;)
				//DBG(print_data();)
				auto p_body = p[0];
				for (size_t i = 1; i != p.size(); ++i)
					p_body = p_body + p[i];
				ps(p_head, p_body);
				p_head = {}, p = {}, head = true;
			}
			void negate() {
				//DBG(std::cout << "negation: " << std::endl;)
				//DBG(print_data();)
				p.back() = ~ p.back();
			}
			void group() {
				//DBG(std::cout << "group\n";)
				//DBG(print_data();)
				auto nn = prods<CharT>(nt(get_new_name()));
				ps(nn, p.back()), p.back() = nn;				
				//DBG(print_data();)
			}
			void optional() {
				//DBG(std::cout << "optional\n";)
				//DBG(print_data();)
				auto nn = prods<CharT>(nt(get_new_name()));
				ps(nn, p.back() | nll), p.back() = nn;
				//DBG(print_data();)
			}
			void repeat() {
				//DBG(std::cout << "repeat\n";)
				//DBG(print_data();)
				auto nn = prods<CharT>(nt(get_new_name()));
				ps(nn, (p.back()+nn) | p.back()), p.back() = nn;
				//DBG(print_data();)
			}
			void multi() {
				auto nn = prods<CharT>(nt(get_new_name()));
				ps(nn, (p.back() + nn) | nll), p.back() = nn;
			}
			string get_new_name() {
				std::basic_stringstream<CharT> s;
				s << from_cstr<CharT>("_R") <<
					p_head.to_string() <<
					from_cstr<CharT>("_") << id++;
				return s.str();
			}
#ifdef DEBUG
			void print_data() const {
				std::cout << "\tp.size(): " << p.size() << "\n";
				size_t i = 0;
				for (const auto& t : p) std::cout << "\tp[" << i++
					<< "]:\t " << t << "\n";
			}
#endif
		} x(nts_);
		//DBG(std::cout << "parsing: " << s << std::endl;)
		auto f = p.parse(s.c_str(), s.size());
		bool found = p.found();
#ifdef DEBUG
		//if (!found) 
		//	p.print_data(cout << "PARSER DATA:\n") << endl;
#endif
		auto cb_enter = [&f, &x, this](const auto& n) {
			const auto& l = n.first;
			if (!l.nt()) return;
			//DBG(cout << "entering: `" << l.to_std_string() << "`\n";)
			if (l == sym) x.add_nonterminal(flatten<CharT>(*f, n));
			else if (l == directive) x.in_directive = true;
			else if (l == directive_param)
				x.cc_names.push_back(flatten<CharT>(*f, n));
			else if (l == literals) x.push();
			else if (l == group || l == optional || l == repeat)
						x.pop();
			else if (l == production_) x.new_production();
			else if (l == string_ || l == quoted_char) {
				auto str = flatten<CharT>(*f, n);
				str.erase(str.begin()), str.erase(str.end() - 1);
				// DBG(cout << "quoted_char?: " << (l == quoted_char)
				// 	<< " str.size(): " << str.size()
				// 	<< " str: `" << to_std_string(str)
				// 	<< "`" << endl;)
				if (l == quoted_char) {
					if (str.size() > 1) switch (str[1]) {
						case 'r':  str = U('\r'); break;
						case 'n':  str = U('\n'); break;
						case 't':  str = U('\t'); break;
						default: str = str[1];
					}
					x.add_terminal(str[0]);
				} else x.add_nonterminal(str);
			}
		};
		auto cb_exit = [&x, this](const auto& n, const auto&) {
			const auto&l = n.first;
			if (!l.nt()) return;
			//DBG(cout << "\t// leaving: `" << l.to_std_string() << "`\n";)
			if      (l == production_) x.production();
			else if (l == directive)   x.directive();
			else if (l == negation)    x.negate();
			else if (l == conjunction) x.conjunction();
			else if (l == disjunction) x.disjunction();
			else if (l == multi)       x.multi();
			else if (l == group)       x.group();
			else if (l == optional)    x.optional();
			else if (l == plus || l == repeat) x.repeat();
		};
		if (!found) {
			cout << "There is an error in the grammar. "
					"Cannot recognize TGF.\n";
			//DBG(g.print_internal_grammar(cout << "TGF productions:\n") << endl;)
			auto error = p.get_error();
			cerr << error.to_str();
		} else {
			//DBG(cout << "TRAVERSE parsed TGF source and "
			//			"generate productions\n";)
			auto c = f->count_trees();
			if (c > 1) {
				cerr << "Forest contains more than one tree: "
					<< c << "\nDumping trees...\n";
				static bool opt_edge = true;
				size_t n = 0;
				auto next_g = [&n](
					parser<char_t>::pforest::graph &fg)
				{
					fg.extract_trees()->to_print(cout
						<< ">>> tree " << ++n << "\n")
						<< "\n<<<\n\n";
					return true;
				};
				f->extract_graphs(f->root(), next_g, opt_edge);
			} else f->traverse(cb_enter, cb_exit);
		}
		return grammar<CharT>(nts_, x.ps, prods<CharT>(x.nt(start_nt)),
			x.cc);
	}
private:
	nonterminals<CharT> nts{};
	char_class_fns<CharT> cc;
	prods<CharT> nll{'\0'},
		alnum, alpha, char_, char0, chars, chars1, conjunction, command,
		commands, commands_rest, directive, directive_param,
		directive_params, disjunction, eof, eol, expr1, expr2, expr3,
		group, literal, literals, literals1, multi, nonliteral,
		nonterminal, negation, optional, plus, printable,
		printable_chars, printable_chars1, production_, production_sep,
		quoted_char, quoted_char_esc, repeat, space, start, string_,
		string_char, string_chars, string_chars1, sym, terminal, ws,
		ws_comment, ws_required;
	grammar<CharT> g;
	parser<CharT> p;
	string U(const char* s) { return from_cstr<CharT>(s); }
	string U(const char  c) { return from_str<CharT>({ c }); }
	lit<CharT> nt(const std::string& s) {
		return lit<CharT>{ nts.get(from_str<CharT>(s)), &nts }; };
	prods<CharT> setup_productions() {
		prods<CharT> q, sep{U("=>")}, underscore{'_'},
			lparen{'('}, lbracket{'['}, lbrace('{'), tilde{'~'},
			hash{'#'}, cr{'\r'}, nl{'\n'}, quotes{'"'}, at{'@'},
			apostrophe{'\''};
		q(start,            commands + ws);
		q(commands,         ws + command + commands_rest);
		q(commands_rest,    (ws + command + commands_rest) | nll);
		q(command,          directive | production_);
		q(directive,        at + U("use_char_class") + ws_required +
					directive_params + ws + U('.'));
		q(directive_params, directive_param |
					(directive_param + ws + U(',') + ws +
					directive_params));
		q(directive_param,  prods<CharT>(U("eof")) | U("alnum") |
					U("alpha") | U("blank") | U("cntrl") |
					U("digit") | U("graph") | U("lower") |
					U("printable") | U("punct") |
					U("space") | U("upper") | U("xdigit"));
		q(production_,      sym + ws + production_sep + ws + expr1 +
					ws + U('.'));
		q(production_sep,   sep);
		q(expr1,            disjunction | expr2);
		q(expr2,            conjunction | expr3);
		q(expr3,            negation | literals);
		q(disjunction,      expr1 + ws + U('|') + ws + expr2);
		q(conjunction,      expr2 + ws + U('&') + ws + expr3);
		q(negation,         tilde + ws + expr3);
		q(group,            lparen   + ws + expr1 + ws + U(')'));
		q(optional,         lbracket + ws + expr1 + ws + U(']'));
		q(repeat,           lbrace   + ws + expr1 + ws + U('}'));
		q(plus,             literal + ws + U('+'));
		q(multi,            literal + ws + U('*'));
		q(literals,         literal + literals1);
		q(literals1,        (ws_required + literal + literals1) | nll);
		q(literal,          terminal | nonterminal);
		q(terminal,         quoted_char | string_);
		q(nonterminal,      sym | nonliteral);
		q(nonliteral,       group | optional | repeat | plus | multi);
		q(sym,              chars);
		q(chars,            (alpha + chars1) | (underscore + chars1));
		q(chars1,           (alnum + chars1) | (underscore + chars1) |
					nll);
		q(printable_chars,  printable + printable_chars1);
		q(printable_chars1, (printable + printable_chars1) | nll);
		q(ws_comment,       (hash + eol) |
					(hash + printable_chars + eol));
		q(ws_required,      (space + ws) | (ws_comment + ws));
		q(ws,               ws_required | nll);
		q(eol,              cr | nl | eof);
		q(string_,          quotes + string_chars + quotes);
		q(string_chars,     string_char + string_chars1);
		q(string_chars1,    (string_char + string_chars1) | nll);
		q(string_char,      char0 | apostrophe | U("\\\""));
		q(quoted_char,      (apostrophe + char_ + apostrophe) |
					quoted_char_esc);
		q(quoted_char_esc,  apostrophe + U('\\') +
					printable + apostrophe);
		q(char_,            char0 | U("\\'") | U('"'));
		q(char0,            alnum | space | U('!') | U('#') | U('$') |
					U('%') | U('&') | U('(') | U(')') |
					U('*') | U('+') | U(',') | U('-') |
					U('.') | U('/') | U(':') | U(';') |
					U('<') | U('=') | U('>') | U('?') |
					U('@') | U('[') | U('\\')| U(']') |
					U('^') | U('_') | U('{') | U('|') |
					U('}') | U('~'));
		return q;
	}
};

} // idni namespace
#endif // __IDNI__PARSER__TGF_H__
