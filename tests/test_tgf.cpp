// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <filesystem>
#include <fstream>
#include "parser.h"
#include "tgf/parser_gen.h"
#include "testing.h"

using namespace std;
using namespace idni;
using testing::run_test_tgf;

int main(int argc, char **argv) {

	testing::process_args(argc, argv);

	testing::test_options o;
	o.ambiguity_fails = false;

	// test parsing of a TGF having a comment but no statements
	nonterminals<char> nts;
	auto gr = tgf<char>::from_string(nts,
		"	# TGF only with ws and ws_comment \n");
	if (!gr.print_and_ok(cerr << "comment-only TGF should load\n")) return 1;

	TEST("basic", "char terminals")
	run_test_tgf(" start => 'a'. ", "a");
	run_test_tgf(" start => \"a\". ", "a");
	run_test_tgf(" start => 'a' 'b'. ", "ab");
	run_test_tgf(" start => \"a\" \"b\". ", "ab");
	run_test_tgf(" start => \"ab\". ", "ab");

	TEST("basic", "literals rule")
	run_test_tgf(
	"	start  => \"hi\". \n"
	, "hi");

	TEST("basic", "comment tabs")
	run_test_tgf(
	"	#	comment	with	tabs	\n"
	"	start  => '1'. \n"
	, "1");

	TEST("basic", "ambiguous with nulls")
	run_test_tgf(
	"	start => A B & D C. \n"
	"	A  => 'a' A     | null. \n"
	"	B  => 'b' B 'c' | null. \n"
	"	C  => 'c' C     | null. \n"
	"	D  => 'a' D 'b' | null. \n"
	, "abc", o);

	TEST("boolean", "conjunction and negation")
	run_test_tgf(
	"	start => X & ~'b'. \n"
	"	X  => 'a' | 'b'. \n"
	, "a");

	TEST("ebnf", "optional")
	run_test_tgf(
	"	start  => binary [ two ]. \n"
	"	binary => '0' | '1'. \n"
	"	two    => '2'. \n"
	, "02");
	run_test_tgf(
	"	start  => '1' [ '0' ] '1'. \n"
	, "11");
	run_test_tgf(
	"	start  => [ '0' ]. \n"
	, "");


	TEST("ebnf", "zero or any")
	run_test_tgf(
	"	start  => { binary } | { two }. \n"
	"	binary => '0' | '1'. \n"
	"	two    => '2'. \n"
	, "0101010100001001111");
	run_test_tgf(
	"	start  => '1' { '0' } '1'. \n"
	, "1000001");

	TEST("ebnf", "plus")
	run_test_tgf(
	"	@use char classes digit. \n"
	"	start  => digit+. \n"
	, "1382746358690");
	run_test_tgf(
	"	start  => 'a' 'b'+ 'c'. \n"
	, "abbbc");

	TEST("ebnf", "asterisk")
	run_test_tgf(
	"	@use char classes digit. \n"
	"	start  => digit*. \n"
	, "1382746358690");

	TEST("ebnf", "group")
	run_test_tgf(
	"	start  => '1' ( '0' ) '1'. \n"
	, "101");

	TEST("ebnf", "group plus")
	run_test_tgf(
	"	start  => '1' ( '0' '0' )+ '1'. \n"
	, "100001");

	TEST("ebnf", "group asterisk")
	run_test_tgf(
	"	start  => '1' ( '0' '0' )* '1'. \n"
	, "11");

	TEST("directives", "start directive")
	run_test_tgf(
	"	@start S. \n"
	"	S  => S S S | S S | '1'. \n"
	, "111");

/*******************************************************************************
*       DYNAMIC
*******************************************************************************/

	// run_test_tgf() builds its grammar internally and never hands it back,
	// so a test calling add_dynamic() has to build the grammar itself.
	auto check = [](bool cond, const char* name) {
		if (cond) cout << "\n\t# OK " << name;
		else cout << "\n\t# FAILED " << name, testing::failed = true;
	};
	auto parse_ok = [](parser<char>& p, const string& in) {
		return p.parse(in.c_str(), in.size()).found;
	};
	auto warns_about = [](diagnostics::report& rep, const string& needle) {
		for (size_t i = 0; i != rep.nodes().size(); ++i)
			if (diagnostics::is_warning(rep.nodes()[i].tag)
			&& rep.format_message(i).find(needle) != string::npos)
				return true;
		return false;
	};

	TEST("dynamic", "declares no alternatives")
	{
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string(nts,
			"@dynamic type_name.\n"
			"start => type_name.\n");
		check(!gr.report().has_error(), "loads with no error");
		check(!warns_about(gr.report(), "type_name"),
			"no warning about type_name");
		grammar<char> g = std::move(gr).value();
		parser<char> p(g);
		check(!parse_ok(p, "u8"), "type_name matches nothing yet");
	}

	TEST("dynamic", "host fills the values")
	{
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string(nts,
			"@dynamic type_name.\n"
			"start => type_name.\n");
		check(gr.print_and_ok(cerr), "grammar loads");
		grammar<char> g = std::move(gr).value();
		parser<char> p(g);
		g.add_dynamic("type_name", { "u8", "u16" });
		check(parse_ok(p, "u8"),  "u8 accepted after add_dynamic");
		check(parse_ok(p, "u16"), "u16 accepted after add_dynamic");
		check(!parse_ok(p, "i32"), "i32 still rejected");
	}

	TEST("dynamic", "cast ambiguity resolved by add_dynamic")
	{
		const char* src =
		"@use char classes alpha, alnum, digit.\n"
		"@dynamic type_name.\n"
		"start       => expr.\n"
		"expr        => ('(' expr ')')                        :paren\n"
		"             | ('(' type '[' num ']' ')' operand)    :cast\n"
		"             | (name '[' num ']')                    :indexed_var\n"
		"             | name.\n"
		"type        => type_name.\n"
		"operand     => name.\n"
		"name        => alpha (alnum | '_')*.\n"
		"num         => digit+.\n";
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string(nts, src);
		check(gr.print_and_ok(cerr), "cast grammar loads");
		grammar<char> g = std::move(gr).value();
		parser<char> p(g);
		check(!parse_ok(p, "(u8[4])x"),
			"cast does not parse before add_dynamic");
		check(parse_ok(p, "(x[4])"),
			"indexed variable parses before add_dynamic");
		g.add_dynamic("type_name", { "u8" });
		check(parse_ok(p, "(u8[4])x"),
			"cast parses after add_dynamic");
		check(parse_ok(p, "(x[4])"),
			"indexed variable still parses after add_dynamic");
	}

	TEST("dynamic", "defaults from the grammar file")
	{
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string(nts,
			"@dynamic type_name defaults u8, u16.\n"
			"start => type_name.\n");
		check(gr.print_and_ok(cerr), "grammar loads");
		grammar<char> g = std::move(gr).value();
		parser<char> p(g);
		check(parse_ok(p, "u8"),  "u8 from defaults parses");
		check(parse_ok(p, "u16"), "u16 from defaults parses");
		check(!parse_ok(p, "i32"), "i32 not in defaults rejected");
	}

	TEST("dynamic", "host adds to the grammar file defaults")
	{
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string(nts,
			"@dynamic type_name defaults u8, u16.\n"
			"start => type_name.\n");
		check(gr.print_and_ok(cerr), "grammar loads");
		grammar<char> g = std::move(gr).value();
		parser<char> p(g);
		g.add_dynamic("type_name", { "i32" });
		check(parse_ok(p, "u8"),  "u8 from defaults still parses");
		check(parse_ok(p, "u16"), "u16 from defaults still parses");
		check(parse_ok(p, "i32"), "i32 added by host parses");
	}

	TEST("dynamic", "quoted default value")
	{
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string(nts,
			"@dynamic type_name defaults \"a b\", u8.\n"
			"start => type_name.\n");
		check(gr.print_and_ok(cerr), "grammar loads");
		grammar<char> g = std::move(gr).value();
		parser<char> p(g);
		check(parse_ok(p, "a b"), "quoted default with a space parses");
		check(parse_ok(p, "u8"),  "plain default still parses");
	}

	TEST("dynamic", "separate decls take no defaults")
	{
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string(nts,
			"@dynamic a; b.\n"
			"start => a | b.\n");
		check(gr.print_and_ok(cerr), "grammar loads");
		grammar<char> g = std::move(gr).value();
		parser<char> p(g);
		check(!parse_ok(p, "x"), "separate decls declare no values");
		g.add_dynamic("a", { "x" });
		check(parse_ok(p, "x"), "x parses after add_dynamic");
	}

	TEST("dynamic", "generated header carries names and default values")
	{
		namespace fs = std::filesystem;
		fs::path dir = fs::temp_directory_path();
		parser_gen_options gopt;
		gopt.output_dir = dir.string() + "/";
		gopt.output = "test_dynamic_gen.generated.h";
		gopt.name = "test_dynamic_gen_parser";
		// "extra" plays the command line value, appended after the
		// .tgf defaults; unknown_nt has no @dynamic in the grammar
		gopt.dynamic["ba_type"] = { "extra" };
		gopt.dynamic["unknown_nt"] = { "z" };
		auto gr = generate_parser_cpp_from_string<char>(
			"test_dynamic_gen.tgf",
			string(
			"@dynamic ba_type defaults sbf, tau.\n"
			"@dynamic type_name.\n"
			"start => ba_type | type_name.\n"),
			gopt);
		check(gr.print_and_ok(cerr), "generator runs");
		fs::path out = dir / gopt.output;
		ifstream in(out);
		stringstream ss; ss << in.rdbuf();
		string text = ss.str();
		auto dpos = text.find(".dynamic = {");
		check(dpos != string::npos, "grammar_options has a .dynamic block");
		auto dend = text.find("};\n", dpos);
		string block = text.substr(dpos, dend - dpos);
		auto sbf = block.find("\"sbf\"");
		auto tau = block.find("\"tau\"");
		auto extra = block.find("\"extra\"");
		check(sbf != string::npos && tau != string::npos
			&& extra != string::npos,
			"sbf, tau and extra all appear in the block");
		check(sbf < tau && tau < extra,
			"order is: .tgf defaults, then command line values");
		check(block.find("\"type_name\", {}") != string::npos,
			"type_name has an empty list");
		check(block.find("unknown_nt") == string::npos,
			"a name not declared in the grammar has no entry");
		fs::remove(out);
	}

	TEST("dynamic", "one decl with defaults, another added by the host")
	{
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string(nts,
			"@dynamic ba_type defaults tau, sbf; a.\n"
			"start => ba_type | a.\n");
		check(gr.print_and_ok(cerr), "grammar loads");
		grammar<char> g = std::move(gr).value();
		parser<char> p(g);
		check(parse_ok(p, "tau"), "tau from defaults parses");
		check(parse_ok(p, "sbf"), "sbf from defaults parses");
		check(!parse_ok(p, "x"), "x not declared anywhere rejected");
		g.add_dynamic("a", { "x" });
		check(parse_ok(p, "x"), "x parses through a after add_dynamic");
	}

	TEST("dynamic", "comma list of names fails to load")
	{
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string(nts,
			"@dynamic a, b.\n"
			"start => a | b.\n");
		check(gr.report().has_error(),
			"comma separated names is a load error");
	}

	TEST("dynamic", "escape in a production string")
	{
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string(nts,
			"start => \"a\\/b\".\n");
		check(gr.print_and_ok(cerr), "grammar loads");
		grammar<char> g = std::move(gr).value();
		parser<char> p(g);
		check(parse_ok(p, "a/b"), "escaped slash in a production parses");
	}

	TEST("dynamic", "escape in a default value")
	{
		nonterminals<char> nts;
		auto gr = tgf<char>::from_string(nts,
			"@dynamic t defaults \"a\\/b\".\n"
			"start => t.\n");
		check(gr.print_and_ok(cerr), "grammar loads");
		grammar<char> g = std::move(gr).value();
		parser<char> p(g);
		check(parse_ok(p, "a/b"), "escaped slash in a default value parses");
	}

	if (testing::stress) {
		TEST("stress", "basic arithmetic")
		run_test_tgf(
		"	@use char class eof, digit, space, printable. \n"

		"	ws_comment   => '#' eol | '#' printable_chars eol. \n"
		"	ws_required  => space ws | ws_comment ws. \n"
		"	ws           => ws_required | null. \n"
		"	eol          => '\\r' | '\\n' | eof. \n"
		"	printable_chars => printable printable_chars1. \n"
		"	printable_chars1=> printable printable_chars1 | null. \n"

		"	sign         => '+' | '-'. \n"
		"	integer      => digit integer_rest. \n"
		"	integer_rest => digit integer_rest | null. \n"

		"	expr         => expr_op | term. \n"
		"	term         => term_op | factor. \n"
		"	expr_op      => expr ws addsub ws term. \n"
		"	term_op      => term ws muldiv ws factor. \n"
		"	addsub       => '+' | '-'. \n"
		"	muldiv       => '*' | '/'. \n"
		"	factor       => '(' ws expr ws ')' | sign ws factor | integer. \n"

		"	statement    => ws expr. \n"
		"	statements   => statement statements1. \n"
		"	statements1  => eol statement statements1 | null. \n"
		"	start        => statements ws | null. \n"
		, "(1+2)*3/2");

	}

	if (testing::failed) cout << "FAILED\n";
	return testing::failed ? 1 : 0;
}
