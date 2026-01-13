// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__TGF__TGF_TEST_H__
#define __IDNI__PARSER__TGF__TGF_TEST_H__
#include <fstream>
#include <streambuf>

#include "tgf_test_parser.generated.h"

namespace idni {

template <typename C = char, typename T = C>
struct tgf_test {
	using tree           = tgf_test_parser::tree;
	using trv            = tree::traverser;

	static std::string get_quoted_string(const trv& t) {
		std::stringstream ss;
		auto s = t || tgf_test_parser::quoted_string_char;
		for (auto& c : s()) {
			auto x = c | trv::only_child;
			auto nt = x | trv::nonterminal;
			// std::cout << "quoted_string_char nt: " << nt << "\n";
			if (nt == tgf_test_parser::unescaped_s)
				ss << (x | trv::terminals);
			else {
				auto esc = x | trv::terminals;
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

	static int run_tests(auto p, const trv& tests) {
		int ret = 0;
		for (auto& test : tests()) {
			auto s = test | tgf_test_parser::symbol | trv::terminals;
			auto ts = test || tgf_test_parser::test_string;
			std::cout << "testing symbol: " << s << "\n";
			tgf_test_parser::parse_options po{
				.start = p->get_grammar().nt(s).n()
			};
			// std::cout << "test strings size: " << ts().size() << std::endl;
			for (auto& t : ts()) {
				auto x = t | trv::only_child;
				auto nt = x | trv::nonterminal;
				// std::cout << "test string nt: " << nt << "\n";
				auto in = nt == tgf_test_parser::string
					? x | trv::terminals
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
		auto& tp = tgf_test_parser::instance();
		auto r = tp.parse(s.c_str(), s.size());
		if (!r.found) return std::cerr << "TGF test: " <<
			r.parse_error.to_str(tgf_test_parser::error::
				info_lvl::INFO_BASIC) << "\n", 1;
		tref root = r.get_shaped_tree2();
		auto tests = trv(root) || tgf_test_parser::test;
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
#endif // __IDNI__PARSER__TGF__TGF_TEST_H__
