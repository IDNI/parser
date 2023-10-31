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
#include <fcntl.h>
#include <cstring>
#include "parser.h"
#include "devhelpers.h"

namespace idni {

template <typename C = char, typename T = C>
struct tgf {
	typedef typename parser<C, T>::pforest forest_t;
	tgf() :
		cc(predefined_char_classes<C, T>({ "eof", "alpha", "alnum",
			"digit", "printable", "space" }, nts)),
		alnum(nt("alnum")),
		alpha(nt("alpha")),
		char_(nt("char")),
		char0(nt("char0")),
		chars(nt("chars")),
		chars1(nt("chars1")),
		conjunction(nt("conjunction")),
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
		statement(nt("statement")),
		statements(nt("statements")),
		statements1(nt("statements1")),
		string_(nt("string")),
		string_char(nt("string_char")),
		string_chars(nt("string_chars")),
		string_chars1(nt("string_chars1")),
		sym(nt("sym")),
		terminal(nt("terminal")),
		ws(nt("ws")),
		ws_comment(nt("ws_comment")),
		ws_required(nt("ws_required")),
		g(nts, setup_productions(), start, cc), p(g)
	{
#if DEBUG && DEBUG_PARSING
		p.debug = false;
#endif
	}
	static grammar<C, T> from_string(nonterminals<C, T>& nts_,
		const std::basic_string<C>& s,
		const std::basic_string<C>& start_nt = from_cstr<C>("start"))
	{
		tgf<C, T> f;
		//std::cout << "parsing: " << to_std_string(s) << std::endl;
		//DBG(f.g.print_data(std::cout << "\n>>>\n\n") << "\n<<<" << std::endl;)
		//return {nts_};
		return f.transform(
			f.p.parse(s.c_str(), s.size()), nts_, start_nt);
	}
	static grammar<C, T> from_file(nonterminals<C, T>& nts_,
		const std::string& filename,
		const std::basic_string<C>& start_nt = from_cstr<C>("start"))
	{
		tgf<C, T> f;
		int fd;
		//std::cout << "parsing file: " << filename << std::endl;
		if ((fd = ::open(filename.c_str(), O_RDONLY)) == -1)
			return std::cerr << "Failed to open file '" << filename
					<< "':" << strerror(errno) << std::endl,
				grammar<C, T>(nts_);
		//std::cout << "fd: " << fd << " l: " << l << std::endl;
		return f.transform(f.p.parse(fd), nts_, start_nt);
	}
private:
	grammar<C, T> transform(
		std::unique_ptr<typename parser<C, T>::pforest> f,
		nonterminals<C, T>& nts_,
		const std::basic_string<C>& start_nt = from_cstr<C>("start"))
	{
		struct context {
			prods<C, T> ps{}, nul{ lit<C, T>{} };
			lit<C, T> p_head{};
			std::vector<prods<C, T>> p{};
			std::vector<std::string> cc_names{};
			bool head = true;       // first sym in rule is head
			bool in_directive = false;
			nonterminals<C, T>& nts;
			char_class_fns<C> cc;
			context(nonterminals<C, T>& nts) : nts(nts) {}
			size_t id = 0;          // id of the new term
			lit<C, T> nt(const std::basic_string<C>& s) {
				return lit<C, T>{ nts.get(s), &nts };
			};
			void add_literal(const lit<C, T>& l) {
				//DBG(std::cout << "add_literal: " << l.to_std_string() << std::endl;)
				if (head) p_head = l, p = {}, head = false;
				else p.back() = p.back() + prods<C, T>{ l };
			}
			void add_nonterminal(const std::basic_string<C>& s) {
				if (in_directive) return;
				//DBG(std::cout << "add_nonterminal: " << to_std_string(s) << std::endl;)
				if (s == from_cstr<C>("null")) add_literal({});
				else add_literal(lit<C, T>{ nt(s) });
			}
			void add_terminal(const T& ch) {
				//DBG(std::cout << "add_terminal: " << to_std_string(ch) << std::endl;)
				add_literal(lit<C, T>{ ch });
			}
			void add_terminals(const std::basic_string<C>& s) {
				//DBG(std::cout << "add_terminals: " << to_std_string(s) << std::endl;)
				for (const auto& ch : s) add_terminal(ch);
			}
			void directive() { in_directive = false; }
			void push() {
				//DBG(std::cout << "push" << "\n";)
				p.push_back({});
			}
			void pop() {
				//DBG(std::cout << "pop" << "\n";)
				if (p.back().size() == 0) p.pop_back();
			}
			void new_production() {
				if (cc_names.size()) cc =
					predefined_char_classes(cc_names, nts),
					cc_names.clear();
			}
			void disjunction() {
				//DBG(std::cout << "disjunction" << std::endl;)
				p[p.size() - 2] = p[p.size() - 2] | p.back();
				p.pop_back();
			}
			void conjunction() {
				//DBG(std::cout << "conjunction" << std::endl;)
				p[p.size() - 2] = p[p.size() - 2] & p.back();
				p.pop_back();
			}
			void production() {
				//DBG(std::cout << "production: " << std::endl;)
				auto p_body = p[0];
				for (size_t i = 1; i != p.size(); ++i)
					p_body = p_body + p[i];
				ps(p_head, p_body);
				p_head = {}, p = {}, head = true;
			}
			void negate() {
				//DBG(std::cout << "negation: " << std::endl;)
				p.back() = ~p.back();
			}
			void group() {
				//DBG(std::cout << "group\n";)
				auto nn = prods<C, T>(nt(get_new_name()));
				ps(nn, p.back()), p.back() = nn;
			}
			void optional() {
				//DBG(std::cout << "optional\n";)
				auto nn = prods<C, T>(nt(get_new_name()));
				ps(nn, p.back() | nul), p.back() = nn;
			}
			void repeat() {
				auto nn = prods<C, T>(nt(get_new_name()));
				prods<C, T> t = p.back();
				p.back() = prods<C, T>{ };
				auto it = t.back().second.begin()->begin();
				for (size_t i = 0; i != it->size() - 1; ++i)
					p.back()=p.back()+prods<C, T>((*it)[i]);
				p.back() = p.back() + nn;
				auto last = prods<C, T>((*it)[it->size() - 1]);
				ps(nn, (last + nn) | last);
			}
			void multi() {
				auto nn = prods<C, T>(nt(get_new_name()));
				//std::cout << "p.back(): " << p.back() << std::endl;
				ps(nn, (p.back() + nn) | nul), p.back() = nn;
			}
			std::basic_string<C> get_new_name() {
				std::stringstream ss("_R");
				ss << p_head.to_std_string() << "_" << id++;
				//cout << "new name: " << id << " " << to_std_string(ss.str()) << "\n";
				return from_str<C>(ss.str());
			}
#ifdef DEBUG
			void print_data() const {
				std::cout << "\tp.size(): " << p.size() << "\n";
				size_t i = 0;
				for (const auto& t : p) std::cout << "\tp[" <<
						i++ << "]:\t " << t << "\n";
			}
#endif
		} x(nts_);
		auto cb_enter = [&f, &x, this](const auto& n) {
			const auto& l = n.first;
			if (!l.nt()) return;
			//DBG(std::cout << "//entering: `" << l.to_std_string() << "`\n";)
			if (l == sym) x.add_nonterminal(from_str<C>(
				terminals_to_str<C, T>(*f, n)));
			else if (l == directive) x.in_directive = true;
			else if (l == directive_param)
				x.cc_names.push_back(to_std_string(from_str<C>(
					terminals_to_str<C, T>(*f, n))));
			else if (l == literals) x.push();
			else if (l == group || l == optional || l == repeat)
						x.pop();
			else if (l == production_) x.new_production();
			else if (l == string_ || l == quoted_char) {
				auto str = terminals_to_str<C, T>(*f, n);
				str.erase(str.begin()), str.erase(str.end()-1);
				//DBG(std::cout << "quoted_char?: " << (l == quoted_char)
				//	<< " str.size(): " << str.size()
				//	<< " str: `" << to_std_string(str)
				//	<< "`" << std::endl;)
				if (l == quoted_char) {
					if (str.size() > 1) switch (str[1]) {
						case 'r':  str = U('\r'); break;
						case 'n':  str = U('\n'); break;
						case 't':  str = U('\t'); break;
						default: str = str[1];
					}
					x.add_terminal(str[0]);
				} else x.add_terminals(str);
			}
		};
		auto cb_exit = [&x, this](const auto& n, const auto&) {
			const auto& l = n.first;
			if (!l.nt()) return;
			//DBG(std::cout << "\t// leaving: `" << l.to_std_string() << "`\n";)
			if      (l == production_)         x.production();
			else if (l == directive)           x.directive();
			else if (l == negation)            x.negate();
			else if (l == conjunction)         x.conjunction();
			else if (l == disjunction)         x.disjunction();
			else if (l == multi)               x.multi();
			else if (l == group)               x.group();
			else if (l == optional)            x.optional();
			else if (l == plus || l == repeat) x.repeat();
		};
#ifdef DEBUG
		auto cb_ambig = [](const auto& n, auto& ambset) {
			const auto& l = n.first;
			std::cout << "\t\tAMBIG: `"<< l.to_std_string()<< "`\n";
			return ambset;
		};
#endif
		if (!p.found()) std::cerr << "There is an error in the grammar."
			" Cannot recognize TGF.\n" << p.get_error().to_str(
				parser<C, T>::error::info_lvl::INFO_DETAILED);
			//parser<C, T>::error::info_lvl::INFO_DETAILED);
			//DBG(p.print_data(std::cout << "PARSER DATA:\n") << "\n";)
			//DBG(g.print_internal_grammar(cout << "TGF productions:\n") << endl;)
		else {
			//DBG(cout << "TRAVERSE parsed TGF source and "
			//			"generate productions\n";)
			//auto c = f->has_single_parse_tree();
			size_t c = f->count_trees();
			//std::cout << "trees: " << c << std::endl;
			if (c > 1) {
				size_t i = 0;
				std::cerr << "ambiguity. number of trees: " <<
								c << std::endl;
				auto cb_next_graph = [&i, &f](
					typename parser<C, T>::pgraph& g)
				{
					std::stringstream ptd;
					std::stringstream ssf;
					//f->detect_cycle(g);
					std::cerr << "tree " << i << std::endl;
					ssf << "parse_rules_" << i << ".tml";
					std::ofstream filet(ssf.str());
					to_tml_rules<C, T>(ptd, g);
					filet << ptd.str();
					filet.close();
					i++;
					return true;
				};
				std::cerr << "ambiguous nodes:\n";
				for (auto& n : f->ambiguous_nodes()) {
					std::cerr << "\t `" << n.first.first <<
						"` [" << n.first.second[0] <<","
						<< n.first.second[1] << "]\n";
					size_t d = 0;
					for (auto ns : n.second) {
						std::cerr<<"\t\t "<< d++ <<"\t";
						for (auto nt : ns) std::cerr <<
							" `" << nt.first << "`["
							<< nt.second[0] << ","
							<< nt.second[1] << "] ";
						std::cerr << "\n";
					}
					f->extract_graphs(n.first,
							cb_next_graph, false);
				}
			}
			f->traverse(cb_enter, cb_exit
#ifdef DEBUG
				, f->no_revisit, cb_ambig
#endif
				);
			//if (!c) {
			//	std::cerr <<
			//		"Forest contains more than one tree\n";
					//<< "\nDumping 1 tree..." << std::endl;
				//size_t n = 0;
				//auto next_g = [&n](parser<C, T>
				//			::pforest::graph& fg)
				//{
				//	fg.extract_trees()->to_print(std::cout
				//		<< ">>> tree " << ++n << "\n")
				//		<< "\n<<<\n\n";
				//	return false;
				//};
				//f->extract_graphs(f->root(), next_g);
			//}
		}
		return grammar<C, T>(
			nts_, x.ps, prods<C, T>(x.nt(start_nt)), x.cc);
	}
	nonterminals<C, T> nts{};
	char_class_fns<C> cc;
	prods<C, T> nul{ lit<C, T>{} },
		alnum, alpha, char_, char0, chars, chars1, conjunction,
		directive, directive_param,
		directive_params, disjunction, eof, eol, expr1, expr2, expr3,
		group, literal, literals, literals1, multi, nonliteral,
		nonterminal, negation, optional, plus, printable,
		printable_chars, printable_chars1, production_, production_sep,
		quoted_char, quoted_char_esc, repeat, space, start,
		statement, statements, statements1,
		string_,
		string_char, string_chars, string_chars1, sym, terminal, ws,
		ws_comment, ws_required;
	grammar<C, T> g;
	parser<C, T> p;
	forest<T> f;
	std::basic_string<C> U(const char* s) { return from_cstr<C>(s); }
	std::basic_string<C> U(const char  c) {
		return from_str<C>(std::string{ c }); }
	lit<C, T> nt(const std::string& s) {
		return lit<C, T>{ nts.get(from_str<C>(s)), &nts }; };
	prods<C, T> setup_productions() {
		prods<C, T> q, sep{U("=>")}, underscore{'_'},
			lparen{'('}, lbracket{'['}, lbrace('{'), tilde{'~'},
			hash{'#'}, cr{'\r'}, nl{'\n'}, quotes{'"'}, at{'@'},
			apostrophe{'\''};
		q(start,            (ws + statements + ws) | nul);
		q(statements,       statement + statements1);
		q(statements1,      (ws + statement + statements1) | nul);
		q(statement,        directive | production_);
		q(directive,        at + U("use_char_class") + ws_required +
					directive_params + ws + U('.'));
		q(directive_params, directive_param |
					(directive_param + ws + U(',') + ws +
					directive_params));
		q(directive_param,  prods<C, T>(U("eof")) | U("alnum") |
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
		q(literals1,        (ws_required + literal + literals1) | nul);
		q(literal,          terminal | nonterminal);
		q(terminal,         quoted_char | string_);
		q(nonterminal,      sym | nonliteral);
		q(nonliteral,       group | optional | repeat | plus | multi);
		q(sym,              chars);
		q(chars,            (alpha + chars1) | (underscore + chars1));
		q(chars1,           (alnum + chars1) | (underscore + chars1) |
					nul);
		q(printable_chars,  printable + printable_chars1);
		q(printable_chars1, (printable + printable_chars1) | nul);
		q(ws_comment,       (hash + eol) |
					(hash + printable_chars + eol));
		q(ws_required,      (space + ws) | (ws_comment + ws));
		q(ws,               ws_required | nul);
		q(eol,              cr | nl | eof);
		q(string_,          quotes + string_chars + quotes);
		q(string_chars,     string_char + string_chars1);
		q(string_chars1,    (string_char + string_chars1) | nul);
		q(string_char,      char0 | apostrophe | U("\\\""));
		q(quoted_char,      (apostrophe + char_ + apostrophe) |
					quoted_char_esc);
		q(quoted_char_esc,  apostrophe + U('\\') +
					char_ + apostrophe);
		q(char_,            char0 | U("\\'") | U('"') | U("`"));
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
