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
#include <fstream>
#include <streambuf>

#include "parser_instance.h"
#include "tgf_parser.generated.h"
#include "devhelpers.h"

namespace idni {

template <typename C = char, typename T = C>
struct tgf {
	struct builder {
		prods<C, T> ps{}, nul{ lit<C, T>{} };
		lit<C, T> p_head{};
		std::vector<prods<C, T>> p{};
		std::set<std::string> cc_names{};
		bool head = true;       // first sym in rule is head
		bool in_directive = false;
		bool in_boolean_directive = false;
		bool in_nodisambig_directive = false;
		bool boolean_directive_enable = true;
		grammar<C, T>::options gopts;
		nonterminals<C, T>& nts;
		char_class_fns<C> cc;
		builder(nonterminals<C, T>& nts) : nts(nts) {}
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
		void directive() {
			in_directive = in_boolean_directive =
				in_nodisambig_directive = false;
		}
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
				predefined_char_classes(std::vector<
					std::string>(cc_names.begin(),
						cc_names.end()), nts),
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
			std::stringstream ss;
			ss << "_R" <<p_head.to_std_string()<< "_"<<id++;
			//std::cout << "new name: " << id << " " << to_std_string(ss.str()) << "\n";
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
		int transform(
			std::unique_ptr<typename parser<C, T>::pforest> f,
			tgf_parser::parse_options po)
		{
			auto& p = parser_instance<tgf_parser>();
			auto cb_enter = [this, &f](const auto& n) {
				const auto& l = n.first;
				if (!l.nt()) return;
				auto s = l.n();
				//DBG(std::cout << "//entering: `" << l.to_std_string() << "`\n";)
				if (s == tgf_parser::sym) add_nonterminal(from_str<C>(
					terminals_to_str<C, T>(*f, n)));
				else if (s == tgf_parser::directive) in_directive = true;
				else if (s == tgf_parser::nodisambig_directive)
					in_nodisambig_directive = true;
				else if (s == tgf_parser::sym) {
					if (in_nodisambig_directive)
						gopts.nodisambig_list.insert(
							to_std_string(from_str<C>(
								terminals_to_str<C, T>(*f, n))));
				}
				else if (s == tgf_parser::boolean_directive)
					boolean_directive_enable =
						in_boolean_directive = true;
				else if (s == tgf_parser::disable_sym) {
					if (in_boolean_directive)
						boolean_directive_enable = false;
				}
				else if (s == tgf_parser::disambiguation_sym)
					gopts.auto_disambiguate =
						boolean_directive_enable;
				else if (s == tgf_parser::char_class_name) {
					auto name = to_std_string(from_str<C>(
						terminals_to_str<C, T>(*f, n)));
					cc_names.insert(name);
				}
				else if (s == tgf_parser::literals) push();
				else if (s == tgf_parser::group
					|| s == tgf_parser::optional
					|| s == tgf_parser::repeat) pop();
				else if (s == tgf_parser::production) new_production();
				else if (s == tgf_parser::terminal_string
					|| s == tgf_parser::terminal_char) {
					auto str = terminals_to_str<C, T>(*f, n);
					str.erase(str.begin()), str.erase(str.end()-1);
					//DBG(std::cout << "quoted_char?: " << (l == quoted_char)
					//	<< " str.size(): " << str.size()
					//	<< " str: `" << to_std_string(str)
					//	<< "`" << std::endl;)
					if (s == tgf_parser::terminal_char) {
						if (str.size() > 1) switch (str[1]) {
							case 'r':  str = from_cstr<C>("\r"); break;
							case 'n':  str = from_cstr<C>("\n"); break;
							case 't':  str = from_cstr<C>("\t"); break;
							default: str = str[1];
						}
						add_terminal(str[0]);
					} else add_terminals(str);
				}
			};
			auto cb_exit = [this](const auto& n, const auto&) {
				const auto& l = n.first;
				if (!l.nt()) return;
				auto s = l.n();
				//DBG(std::cout << "\t// leaving: `" << l.to_std_string() << "`\n";)
				if      (s == tgf_parser::production)  production();
				else if (s == tgf_parser::directive)   directive();
				else if (s == tgf_parser::negation)    negate();
				else if (s == tgf_parser::conjunction) conjunction();
				else if (s == tgf_parser::disjunction) disjunction();
				else if (s == tgf_parser::multi)       multi();
				else if (s == tgf_parser::group)       group();
				else if (s == tgf_parser::optional)    optional();
				else if (s == tgf_parser::plus
					|| s == tgf_parser::repeat)    repeat();
			};
	#ifdef DEBUG
			auto cb_ambig = [](const auto& n, auto& ambset) {
				const auto& l = n.first;
				std::cout << "\t\tAMBIG: `"<< l.to_std_string()<< "`\n";
				return ambset;
			};
	#endif
			bool found = p.found(po.start);
			if (!found) {
				std::cerr << "There is an error in the grammar."
				" Cannot recognize TGF.\n" << p.get_error().to_str(
					parser<C, T>::error::info_lvl::INFO_ROOT_CAUSE);
				//parser<C, T>::error::info_lvl::INFO_DETAILED);
				//DBG(p.p.print_data(std::cout << "PARSER DATA:\n") << "\n";)
				//DBG(p.g.print_internal_grammar(std::cout << "TGF productions:\n") << std::endl;)
			} else {
				//DBG(cout << "TRAVERSE parsed TGF source and "
				//			"generate productions\n";)
				//auto c = f->has_single_parse_tree();
				size_t c = f->count_trees();
				//std::cout << "trees: " << c << std::endl;
				if (c > 1) {
					size_t i = 0;
					std::cerr << "ambiguity. number of trees: " <<
									c << std::endl;
					auto cb_next_graph = [&i](
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
					, NO_REVISIT, cb_ambig
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
			//std::cout << "found: " << found << std::endl;
			return found ? 0 : 1;
		}
		int parse(const char* s, size_t l) {
			auto& p = parser_instance<tgf_parser>();
			static tgf_parser::parse_options po{
				.start = tgf_parser::start_statement };
			return transform(p.parse(s, l, po), po);
		}
		grammar<C, T> g(const std::basic_string<C>& start_nt =
							from_cstr<C>("start"))
		{
			if (cc_names.size())
				cc = predefined_char_classes(std::vector<
					std::string>(cc_names.begin(),
						cc_names.end()), nts);
			return grammar<C, T>(
				nts, ps, prods<C, T>(nt(start_nt)), cc, gopts);
		}
	};
	static grammar<C, T> from_string(nonterminals<C, T>& nts_,
		const std::basic_string<C>& s,
		const std::basic_string<C>& start_nt = from_cstr<C>("start"))
	{
		builder b(nts_);
		auto c = s.c_str();
		size_t last = 0;
		auto l = s.size();
		bool in_string = false;
		bool in_comment = false;
		bool in_char = false;
		bool in_escape = false;
		auto escape = [&in_escape](char c) {
			bool ret = in_escape;
			if (in_escape) in_escape = false;
			if (ret) return ret;
			return in_escape = (c == '\\');
		};
		for (size_t i = 0; i != l; ++i) {
			//std::cout << "i: " << i << " c[i]: '" << c[i]
			//	<< "' string: " << in_string
			//	<< " comment: " << in_comment
			//	<< " char: " << in_char
			//	<< " escape: " << in_escape << std::endl;
			if (in_comment) {
				if (c[i] == '\n') in_comment = false;
			}
			else if (in_string) {
				if (escape(c[i])) continue;
				if (c[i] == '"') in_string = false;
			}
			else if (in_char) {
				if (escape(c[i])) continue;
				if (c[i] == '\'') in_char = false;
			}
			else if (c[i] == '\'') in_char = true;
			else if (c[i] == '"') in_string = true;
			else if (c[i] == '#') in_comment = true;
			else if (c[i] == '.') {
				const char* nc = c + last;
				//std::cout << "last: " << last << " " << c[last] << std::endl;
				//std::cout << "i:    " << i << " " << c[i] << std::endl;
				size_t nl = i - last + 1;
				//std::cout << "nl:   " << nl << std::endl;
				//std::cout << "nc:   {";
				//for (size_t j = 0; j < nl; ++j) {
				//	std::cout << nc[j];
				//}
				//std::cout << "}\n";
				last = i + 1;
				int ret = b.parse(nc, nl);
				//std::cout << "ret:   " << ret << std::endl;
				if (ret == 1) break;
			}
		}
		return b.g(start_nt);
	}
	static grammar<C, T> from_file(nonterminals<C, T>& nts_,
		const std::string& filename,
		const std::basic_string<C>& start_nt = from_cstr<C>("start"))
	{
		std::ifstream ifs(filename);
		if (!ifs) {
			std::cerr << "cannot open file: " << filename << std::endl;
			return grammar<C, T>(nts_);
		}
		return from_string(nts_, std::string(
				std::istreambuf_iterator<C>(ifs),
				std::istreambuf_iterator<C>()), start_nt);
	}
};

} // idni namespace
#endif // __IDNI__PARSER__TGF_H__
