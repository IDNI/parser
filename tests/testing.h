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
#ifndef __IDNI__TESTING_H__
#define __IDNI__TESTING_H__
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <utility>
#include "parser.h"

namespace idni::testing {

struct test_options {
	std::string contains = "";
	std::string error_expected = "";
	bool dump = false;
	bool ambiguity_fails = true;
	size_t start = SIZE_MAX;
};

template <typename T = char>
typename parser<T>::options options;
template <typename T = char>
typename grammar<T>::options grammar_options;

static bool opt_edge = true;
static size_t c = 0;
static size_t stop_after = SIZE_MAX;
bool print_only_failed = false;
// if true enables stress tests
bool stress = false;
bool measure = false;

// current group, name, id of a test and previous test name
std::string test_group = "";
std::string test_name_prev = "";
std::string test_name = "";
std::string test_file = "";
size_t test_line = 0;
size_t test_id = 0;
// group, tests and ids white/black lists
std::vector<std::string> tests_wl, tests_bl, groups_wl, groups_bl;
std::vector<size_t> ids_wl;

// sets group and name of following run_test calls
#define TEST(group, name) { testing::test_file = __FILE__, \
				testing::test_line = __LINE__, \
				testing::test_name = (name), \
				testing::test_group = (group); }

bool failed = false;
bool print_all = false;
bool print_lines = false;
size_t verbosity = 0;
bool run = false;

// selects group/test/id for testing or disables group/test from running
void group(const std::string& s) { groups_wl.push_back(s); }
void disable_group(const std::string& s) { groups_bl.push_back(s); }
void test(const std::string& s) { tests_wl.push_back(s); }
void disable_test(const std::string& s) { tests_bl.push_back(s); }
void id(size_t i) { ids_wl.push_back(i); }

bool check() {
	// check if we have another test_id (prev test name differs to actual)
	if (test_name == test_name_prev) test_id++;
	else test_name_prev = test_name, test_id = 0;
	// check white lists and black lists and decide if we run or skip
	bool g_wl = groups_wl.size(),
		t_wl = tests_wl.size(),
		i_wl = ids_wl.size(),
		in_g_wl = std::find(groups_wl.begin(), groups_wl.end(),
						test_group) != groups_wl.end(),
		in_g_bl = std::find(groups_bl.begin(), groups_bl.end(),
						test_group) != groups_bl.end(),
		in_t_wl = std::find(tests_wl.begin(), tests_wl.end(),
						test_name) != tests_wl.end(),
		in_t_bl = std::find(tests_bl.begin(), tests_bl.end(),
						test_name) != tests_bl.end(),
		in_i_wl = std::find(ids_wl.begin(), ids_wl.end(),
						test_id) != ids_wl.end();
	run = (!g_wl || in_g_wl) && (!t_wl || in_t_wl) &&
		(!i_wl || in_i_wl) && (!in_g_bl && !in_t_bl);
	//DBG(std::cout << "run: " << run << " " << test_group << " " <<
	//	test_name << " " << test_id << "\n";)
	return run;
}

// prints test info (group / name / id) and decides if the test is skipped
bool info(std::ostream& os) {
	if (!run && !print_all) return false;
	os << "\nTEST " << test_group << " / " << test_name << " / "
		<< test_id << " " << (run ? "" : "skipped");
	if (print_lines) os << " (" << test_file << ":" << test_line << ") ";
	return run;
}

// saves resulting forest and graphs into files with various formats
template <typename T>
int test_out(int c, const grammar<T>& g, const std::basic_string<T>& inputstr,
	typename parser<T>::pforest& f)
{
	std::stringstream ptd;
	std::stringstream ssf;

	g.print_internal_grammar(ssf, "\\l", true);
	std::string s = ssf.str();
	ssf.str({});

	ssf << "forest" << c << ".dot";
	std::ofstream file(ssf.str());
	to_dot<T>(ptd, std::as_const(f), to_std_string(inputstr), s);
	file << ptd.str();
	file.close();
	ssf.str({});
	ptd.str({});

	ssf << "forest_facts" << c << ".tml";
	std::ofstream file1(ssf.str());
	to_tml_facts<T>(ptd, as_const(f));
	file1 << ptd.str();
	file1.close();
	ssf.str({});
	ptd.str({});

	ssf << "forest_grammar_rules" << c << ".tml";
	std::ofstream file2(ssf.str());
	to_tml_rules<T>(ptd, as_const(f));
	file2 << ptd.str();
	file2.close();

	size_t i = 0;
	auto dump_files = [&](typename parser<T>::pgraph& g,
		std::string suffix="")
	{
		ssf.str({});
		ptd.str({});
		ssf << "graph" << c << "_" << i << suffix << ".dot";
		std::ofstream filet(ssf.str());
		to_dot<T, T, typename parser<T>::pgraph>(ptd, g,
			to_std_string(inputstr), s);
		filet << ptd.str();
		filet.close();
		ssf.str({});
		ptd.str({});
		ssf << "graph_grammar_rules" << c << "_" << i <<suffix<< ".tml";
		filet.open(ssf.str());
		to_tml_rules<T>(ptd, g);
		filet << ptd.str();
		filet.close();

		typename parser<T>::psptree tr= g.extract_trees();
		ssf.str({});
		ptd.str({});
		ssf << "tree" << c << "_" << i << suffix << ".dot";
		filet.open(ssf.str());
		to_dot<T>(ptd, tr, to_std_string(inputstr), s);
		filet << ptd.str();
		filet.close();
	};
	auto cb_next_graph = [&](typename parser<T>::pgraph& g){
		f.detect_cycle(g);
		dump_files(g);
		if (options<T>.binarize && f.remove_binarization(g))
			dump_files(g, "rembin");
		i++;

		return i < stop_after ? true : false;
	};

	f.extract_graphs(f.root(), cb_next_graph, opt_edge);
	return 1;
}

template <typename T>
void print_ambig_nodes(std::ostream& os, typename parser<T>::pforest& f) {
	if (!f.is_ambiguous()) return;
	os << "\t# ambiguous nodes:\n";
	for (auto& n : f.ambiguous_nodes()) {
		os << "\t `" << n.first.first << "` [" << n.first.second[0] <<
			"," << n.first.second[1] << "]\n";
		size_t d = 0;
		for (auto ns : n.second) {
			os << "\t\t " << d++ << "\t";
			for (auto nt : ns) os << " `" << nt.first << "`[" <<
				nt.second[0] << "," << nt.second[1] << "] ";
			os << "\n";
		}
	}
}

// runs a test
template <typename T>
bool run_test_(grammar<T>& g, parser<T>& p, const std::basic_string<T>& input,
	test_options opts = {})
{
	std::stringstream ss;
	if (!info(ss)) return true;
	std::stringstream ssna;
	g.check_nullable_ambiguity(ssna);
	if (ssna.tellp())
		ss << "\npossible nullable ambiguity...\n" << ssna.str() <<"\n";
	bool expect_fail = opts.error_expected.size() > 0;
	if (verbosity == 0) opts.dump = false;
	else {
		ss <<"\n\n\t# parsing input " << quoted(to_std_string(input)) <<
			" [" << input.size() << "]\n\n";
		if (verbosity > 1) g.print_internal_grammar(
			ss << "\t# grammar productions:\n", "\t# ") <<std::endl;
	}
	emeasure_time_start(start_p, end_p);
	auto f = p.parse(input.c_str(), input.size(), {
		.start = opts.start,
		.debug = false
	});
	if (measure) {
		ss << "\nelapsed parsing: ";
		emeasure_time_end_to(start_p, end_p, ss) << "\t";
	}
	bool found = p.found(opts.start);
	bool found_orig = found;
	std::string msg{};
	if (!found) msg = p.get_error()
			.to_str(parser<T>::error::info_lvl::INFO_BASIC);
	//if (!found and opts.dump) p.print_S(ss << "\t# S:\n") << "\n";
	bool ambiguity = found && f->is_ambiguous();
	if (ambiguity && opts.ambiguity_fails) {
		expect_fail = found = false;
		msg = "Input with provided grammar is providing ambiguous "
							"result.";
	}
	bool contained = false;
	if (found && (opts.dump || opts.contains.size())) {
		auto cb_enter = [&opts, &contained, &ss] (const auto& n) {
			if (opts.contains == n.first.to_std_string())
				contained = true;
			if (opts.dump) ss << "\t#\t entering: \"" <<
				n.first.to_std_string() << "\"\n";
		};
		auto cb_exit = [&opts, &ss](const auto& n, const auto&) {
			if (opts.dump) ss << "\t#\t exiting: \"" <<
				n.first.to_std_string() << "\"\n";
		};
		f->traverse(cb_enter, cb_exit);
	}
	if (verbosity > 0) {
		ss << "\t# found: " << found_orig << " " << (found ? "yes => SUCCESS" : "no => FAIL");
		if (ambiguity) ss << " because of ambiguity";
		if (expect_fail)
			ss << " but expected FAIL => " <<
				(found ? "FAIL" : "SUCCESS") << "\n\n",
			ss << "\t# error_expected \"" <<
					opts.error_expected <<"\"\n";
		ss << "\n";
	}
	if (found) {
		if (verbosity > 0)
			ss << "\t# is_ambiguous " << (f->is_ambiguous()
				? "yes" : "no") << "\n",
			ss << "\t# count_trees " << f->count_trees() << "\n";
	}
	if (found && verbosity > 1) {
		struct {
			bool graphs = false, facts = false, rules = false;
		} print;
		auto cb_next_g = [&f, &print, &ss](parser<T>::pgraph& g) {
			f->remove_binarization(g);
			f->remove_recursive_nodes(g);
			if (print.graphs) {
				static size_t c = 1;
				ss << "\n\t# parsed graph";
				if (c++ > 1) ss << " " << c;
				ss << ":\n\n";
				g.extract_trees()->to_print(ss);
				ss << "\n\n";
			}
			if (print.rules) to_tml_rules<T, T, typename
				parser<T>::pgraph>(ss << "\n\t# TML rules:\n\n", g),
				ss << "\n";
			return true;
		};
		if (print.rules || print.graphs)
			f->extract_graphs(f->root(), cb_next_g);
		if (print.facts) to_tml_facts<T,T>(ss << "\n\t# TML facts:\n\n",
			*f), ss << "\n";
		ss << "\t# terminals parsed: `" << to_string(
			terminals_to_str<T>(*f, f->root())) << "`\n";
		if (verbosity > 2) test_out<T>(c, g, input.size() > 100
						? input.substr(0, 100) : input, *f);
	}


	if (ambiguity && verbosity > 0)
		print_ambig_nodes<T>(ss, *f);
	if (expect_fail) found = msg.find(opts.error_expected) ==
					decltype(opts.error_expected)::npos;
	bool fail = false;
	if ((found == expect_fail) ||
		(found && opts.contains.size() && !contained))
			fail = failed = true;
	ss << (verbosity == 0 ? "\t" : "\n\t#") << " " <<
		(fail ? "FAILED" : "OK") << "\n";
	if (msg.size() && (!expect_fail || verbosity > 0))
		ss << (verbosity == 0 ? "" : "\n\t# error received \"")
			<< msg << (verbosity == 0 ? "" : "\"") << "\n";
	c++;
	if (!print_only_failed || fail) std::cout << ss.str();
	return !fail;
}

// runs test from tgf
template <typename T = char>
bool run_test_tgf(const char* g_tgf, const std::basic_string<T>& input,
	test_options opts = {})
{
	std::stringstream ss;
	if (!check()) return info(std::cout), true;
	emeasure_time_start(start_tgf, end_tgf);
	nonterminals<T> nts;
	grammar<T> g = tgf<T>::from_string(nts, g_tgf);
	if (measure) {
		ss << "elapsed TGF: ";
		emeasure_time_end_to(start_tgf, end_tgf, ss) << "\n";
	}
	if (g.size() == 0)
		return std::cout << "\t# FAILED\n", failed = true, false;
	parser<T> p(g, options<T>);
	bool success = run_test_<T>(g, p, input, opts);
	if (!print_only_failed || !success) std::cout << ss.str();
	return success;
}
template <typename T = char>
bool run_test_tgf(const char* g_tgf, const T* input, test_options opts = {}) {
	return run_test_tgf(g_tgf, std::basic_string<T>(input), opts);
}

// runs test from prods and cc
template <typename T = char>
bool run_test(const prods<T>& ps, nonterminals<T>& nts,
	const prods<T>& start, const std::basic_string<T>& input,
	char_class_fns<T> cc = {}, test_options opts = {})
{
	if (!check()) return info(std::cout), true;
	grammar<T> g(nts, ps, start, cc, grammar_options<T>);
	parser<T> p(g, options<T>);
	return run_test_<T>(g, p, input, opts);
}

void help(const std::string& opt) {
	if (opt.size())	std::cout << "Invalid option: " << opt << "\n";
	std::cout <<
"Valid options:\n" <<
"	-help        -h         help (this text)\n" <<
"	-print_all   -p         prints names of all tests (includes skipped)\n"<<
"	-print_lines -l         prints file and line of tests (if not skipped)\n"<<
"	-only_failed -f         prints info only for failed tests\n" <<
"	-measure     -m         measure parsing time and TGF parsing time\n" <<
"	-stress      -s         run also stress tests\n" <<
"	-verbose     -v         increase verbosity\n" <<
"	-vv                     increase verbosity +2\n" <<
"	-vvv                    increase verbosity +3\n" <<
"	-[enable|disable]_incrgen\n" <<
"	              enable/disable incremental generation of forest\n" <<
"	-[enable|disable]_binarization\n" <<
"\t              enables binarization and leftright optimization of forest\n" <<
"	-[enable|disable]_gc	enables garbage collection\n" <<
"	-[enable|disable]_autodisambg enable/disable auto disambiguation\n" <<
"	-[no_disambg nt1,nt2..ntn] disables disambiguation for specified non-terminals\n"
"	-unique_node\n" <<
"	              retrieves graphs from forest based on nodes, not edges\n"
"	-stop_after_[1|5]\n" <<
"		      stop retrieving further graphs after 1 or 5 count\n" <<
"Enabling/disabling tests (can be combined and used more than once):\n" <<
"	-group <group>          run <group> of tests\n" <<
"	-disable_group <group>  disable <group> of tests\n" <<
"	-test <test>            run <test>\n" <<
"	-disable_test <test>    disable running of <test>\n" <<
"	-id <id>                run N-th subtest where <id> is N\n" <<
	std::endl;
}

void process_args(int argc, char **argv) {
	bool binarize = false;
	bool incr_gen = false;
	bool enable_gc = false;
	bool auto_disambg = false;

	std::vector<std::string> args(argv + 1, argv + argc);

	auto missing = [](const std::string& opt) {
		std::cerr << "Missing argument for options: " << opt << "\n";
		exit(1);
	};

	for (auto it = args.begin(); it != args.end(); ++it) {
		auto opt = *it;
		if (opt == "-enable_binarization")
			binarize = true;
		else if (opt == "-enable_incrgen")
			incr_gen = true;
		else if (opt == "-disable_binarization")
			binarize = false;
		else if (opt == "-disable_incrgen")
			incr_gen = false;
		else if (opt == "-enable_gc")
			enable_gc = true;
		else if (opt == "-disable_gc")
			enable_gc = false;
		else if (opt == "-unique_node")
			opt_edge = false;
		else if (opt == "-stop_after_1")
			stop_after = 1;
		else if (opt == "-stop_after_5")
			stop_after = 5;
		else if (opt == "-stress" || opt == "-s")
			stress = true;
		else if (opt == "-measure" || opt == "-m")
			measure = true;
		else if (opt == "-print_all" || opt == "-p")
			print_all = true;
		else if (opt == "-print_lines" || opt == "-l")
			print_lines = true;
		else if (opt == "-only_failed" || opt == "-f")
			print_only_failed = true;
		else if (opt == "-verbose" || opt == "-v")
			verbosity++;
		else if (opt == "-vv") verbosity += 2;
		else if (opt == "-vvv") verbosity += 3;
		else if (opt == "-group") {
			it++;
			if (it == args.end()) missing(opt);
			group(*it);
		}
		else if (opt == "-test") {
			it++;
			if (it == args.end()) missing(opt);
			test(*it);
		}
		else if (opt == "-disable_group") {
			it++;
			if (it == args.end()) missing(opt);
			disable_group(*it);
		}
		else if (opt == "-disable_test") {
			it++;
			if (it == args.end()) missing(opt);
			disable_test(*it);
		}
		else if (opt == "-id") {
			it++;
			if (it == args.end()) missing(opt);
			std::stringstream is(*it);
			int_t result = 0;
			if (!(is >> result) || result < 0) {
				std::cerr << "'" << *it <<
					"' is not a positive number.\n";
				exit(1);
			}
			id(result);
		}
		else if (opt == "-enable_autodisambg")  auto_disambg = true;
		else if (opt == "-disable_autodisambg") auto_disambg = false;
		else if (opt == "-no_disambg") {
			++it;
			if( it == args.end()) missing(opt);
			std::stringstream ss(*it);
			std::string nt;
			while(getline(ss, nt, ',')) {
				grammar_options<char>.nodisambig_list
					.insert(nt),
				grammar_options<char32_t>.nodisambig_list
					.insert(nt);
			}
		}
		else if (opt == "-help" || opt == "-h") help(""), exit(0);
		else help(opt), exit(1);
	}
	if (verbosity > 0) {
		std::cout << "Running with options: ";
		for_each(args.begin(), args.end(),
			[](std::string& a) { std::cout << a << " "; });
		std::cout << std::endl;
	}

	options<char>.binarize =
		options<char32_t>.binarize = binarize;
	options<char>.incr_gen_forest =
		options<char32_t>.incr_gen_forest = incr_gen;
	options<char>.enable_gc =
		options<char32_t>.enable_gc = enable_gc;
	grammar_options<char>.auto_disambiguate =
		grammar_options<char32_t>.auto_disambiguate = auto_disambg;
}

} // namespace idni::testing
#endif // __IDNI__TESTING_H__
