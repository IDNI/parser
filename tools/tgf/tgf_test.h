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
#ifndef __IDNI__PARSER__TGF_TEST_H__
#define __IDNI__PARSER__TGF_TEST_H__
#include <fstream>
#include <streambuf>

#include "traverser.h"
#include "tgf_test_parser.generated.h"

namespace idni {

template <typename C = char, typename T = C>
struct tgf_test {
	using parse_symbol_t = tgf_test_parser::node_type;
	using symbol_t       = tgf_test_parser::symbol_type;
	using node_variant_t = std::variant<symbol_t>;
	using node_t         = idni::rewriter::sp_node<node_variant_t>;
	using traverser_t    = traverser<node_variant_t, tgf_test_parser>;

	static constexpr const auto& get_only_child =
		traverser_t::get_only_child_extractor();
	static constexpr const auto& get_terminals =
		traverser_t::get_terminal_extractor();
	static constexpr const auto& get_nonterminal =
		traverser_t::get_nonterminal_extractor();

	static std::string get_quoted_string(traverser_t t) {
		std::stringstream ss;
		auto s = t || tgf_test_parser::quoted_string_char;
		for (auto& c : s()) {
			auto x = c | get_only_child;
			auto nt = x | get_nonterminal;
			// std::cout << "quoted_string_char nt: " << nt << "\n";
			if (nt == tgf_test_parser::unescaped_s)
				ss << (x | get_terminals);
			else {
				auto esc = x | get_terminals;
				switch (esc[1]) {
				case '"': ss << '"'; break;
				case '\\': ss << '\\'; break;
				case 'n': ss << '\n'; break;
				case 'r': ss << '\r'; break;
				case 't': ss << '\t'; break;
				case 'b': ss << '\b'; break;
				case 'f': ss << '\f'; break;
				case '/': ss << '/'; break;
				}
			}
		}
		return ss.str();
	}

	static int run_tests(auto p, const traverser_t& tests) {
		int ret = 0;
		for (auto& test : tests()) {
			auto s = test | tgf_test_parser::symbol | get_terminals;
			auto ts = test || tgf_test_parser::test_string;
			std::cout << "testing symbol: " << s << "\n";
			tgf_test_parser::parse_options po{
				.start = p->get_grammar().nt(s).n()
			};
			// std::cout << "test strings size: " << ts().size() << std::endl;
			for (auto& t : ts()) {
				auto x = t | get_only_child;
				auto nt = x | get_nonterminal;
				// std::cout << "test string nt: " << nt << "\n";
				auto in = nt == tgf_test_parser::string
					? x | get_terminals
					: get_quoted_string(x);
				std::cout << "\t\"" << in << "\"\t\t";
				auto r = p->parse(in.c_str(), in.size(), po);
				if (!r.found) {
					std::cout << "FAIL\n";
					std::cerr << "parse error: "
						<< r.parse_error << "\n";
					std::cout << "\n";
					ret = 1;
				} else std::cout << "OK\n";
			}
			std::cout << "\n";
		}
		return ret;
	}

	static int run_from_string(auto p, const std::basic_string<C>& s) {
		using namespace rewriter;
		auto& tp = tgf_test_parser::instance();
		auto r = tp.parse(s.c_str(), s.size());
		if (!r.found) return std::cerr << "TGF test: " <<
			r.parse_error.to_str(tgf_test_parser::error::
				info_lvl::INFO_BASIC) << "\n", 1;
		auto root = make_node_from_tree<tgf_test_parser,
			drop_location_t<parse_symbol_t, node_variant_t>,
								node_variant_t>(
				drop_location<parse_symbol_t, node_variant_t>,
				r.get_shaped_tree());
		auto tests = traverser_t(root) || tgf_test_parser::test;
		std::cout << "tests.size(): " << tests().size() << std::endl;
		return run_tests(p, tests);
	}

	static int run_from_file(auto p, const std::string& filename) {
		std::cout << "opening file: " << filename << std::endl;
		std::ifstream ifs(filename);
		if (!ifs) {
			std::cerr << "cannot open file: "
						<< filename << std::endl;
			return 1;
		}
		return run_from_string(p, std::string(
				std::istreambuf_iterator<C>(ifs),
				std::istreambuf_iterator<C>()));
	}
};

} // idni namespace
#endif // __IDNI__PARSER__TGF_TEST_H__
