// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Unit tests for the generic REPL evaluator base, on the tgf REPL parser.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../src/utility/repl_evaluator_base.h"
#include "../src/tgf/tgf_repl_parser.generated.h"

using namespace std;
using namespace idni;

namespace {

using base_t = repl_evaluator_base<tgf_repl_parser, tgf_repl_parser::statement>;

// An evaluator that answers each statement with the name of its command.
struct test_evaluator : base_t {
	using base_t::trv;
	vector<string> seen;
	size_t answers = 0;

	explicit test_evaluator(repl_base_options o = { .colors = false })
		: base_t(o, repl_history_policy{ ".test_history" }) {}

	cmd_result run_statement(const trv& s) override {
		cmd_result r;
		const size_t nt = s | trv::nonterminal;
		r.cmd = string(tgf_repl_parser::instance().name(nt));
		seen.push_back(r.cmd);
		if (nt == tgf_repl_parser::quit_cmd) r.status = cmd_status::quit;
		return r;
	}
	void end_answer() override { ++answers; }
	string prompt_text() const override { return "test> "; }
};

// A front end that keeps the last prompt.
struct test_front : repl_front {
	string prompt;
	bool cleared = false;
	void set_prompt(const string& p) override { prompt = p; }
	void clear() override { cleared = true; }
};

// Run @p src with the error stream captured.
diagnostics::result<int> eval_quiet(test_evaluator& e, const string& src,
	string& err)
{
	ostringstream os;
	streambuf* old_err = cerr.rdbuf(os.rdbuf());
	auto r = e.eval(src);
	cerr.rdbuf(old_err);
	err = os.str();
	return r;
}

} // namespace

TEST_SUITE("repl_evaluator_base") {

	TEST_CASE("one statement runs and the answer ends") {
		test_evaluator e;
		auto r = e.eval("version");
		REQUIRE(r.has_value());
		CHECK(r.value() == 0);
		CHECK(e.seen == vector<string>{ "version_cmd" });
		CHECK(e.answers == 1u);
	}

	TEST_CASE("two statements run in their order") {
		test_evaluator e;
		auto r = e.eval("version . license");
		REQUIRE(r.has_value());
		CHECK(r.value() == 0);
		CHECK(e.seen == vector<string>{ "version_cmd", "license_cmd" });
	}

	TEST_CASE("a text cut off at its end is incomplete") {
		test_evaluator e;
		auto r = e.eval("version .");
		REQUIRE(r.has_value());
		CHECK(r.value() == 2);
		CHECK_FALSE(r.report().has_error());
		ostringstream os;
		r.report().print(os, term::colors(false));
		CHECK(os.str().find(parser_strings::messages::incomplete_input)
			!= string::npos);
		CHECK(e.seen.empty());
		CHECK(e.answers == 0u);
	}

	TEST_CASE("without continue_on_eof a cut text is an error") {
		test_evaluator e({ .continue_on_eof = false, .colors = false });
		string err;
		auto r = eval_quiet(e, "version .", err);
		REQUIRE(r.has_value());
		CHECK(r.value() == 0);
		CHECK_FALSE(err.empty());
		CHECK(e.seen.empty());
	}

	TEST_CASE("a parse error prints an error report") {
		test_evaluator e;
		string err;
		auto r = eval_quiet(e, "no such command", err);
		REQUIRE(r.has_value());
		CHECK(r.value() == 0);
		CHECK_FALSE(err.empty());
		CHECK(e.seen.empty());
		CHECK(e.take_report().nodes().empty());
	}

	TEST_CASE("the run_statements report holds the parse error") {
		struct probe : test_evaluator {
			eval_result run(const string& src) {
				return run_statements(src, [this](const trv& s) {
					return run_statement(s); });
			}
		} e;
		auto er = e.run("no such command");
		CHECK(er.status == cmd_status::error);
		CHECK(er.report.has_error());
		CHECK(er.results.empty());
	}

	TEST_CASE("the report of a cut text holds the incomplete info") {
		struct probe : test_evaluator {
			eval_result run(const string& src) {
				return run_statements(src, [this](const trv& s) {
					return run_statement(s); });
			}
		} e;
		auto er = e.run("version .");
		CHECK(er.status == cmd_status::incomplete);
		CHECK_FALSE(er.report.has_error());
		REQUIRE(er.report.nodes().size() == 1);
		ostringstream os;
		er.report.print(os, term::colors(false));
		CHECK(os.str().find(parser_strings::messages::incomplete_input)
			!= string::npos);
		CHECK(er.results.empty());
	}

	TEST_CASE("a quit ends the loop") {
		test_evaluator e;
		auto r = e.eval("quit . version");
		REQUIRE(r.has_value());
		CHECK(r.value() == 1);
		CHECK(e.seen == vector<string>{ "quit_cmd" });
		CHECK(e.answers == 0u);
	}

	TEST_CASE("reprompt sets the prompt of the front end") {
		test_evaluator e;
		test_front f;
		link_front(e, &f, [](auto&) {});
		CHECK(e.front == &f);
		e.reprompt();
		CHECK(f.prompt == "test> ");
		f.prompt.clear();
		REQUIRE(e.eval("version").has_value());
		CHECK(f.prompt == "test> ");
	}

	TEST_CASE("reprompt without a front end does nothing") {
		test_evaluator e;
		e.reprompt();
		CHECK(e.front == nullptr);
	}

	TEST_CASE("the history policy and the status names") {
		test_evaluator e;
		CHECK(e.history.file == ".test_history");
		CHECK_FALSE(e.history.keep);
		for (auto s : { cmd_status::ok, cmd_status::error,
				cmd_status::incomplete, cmd_status::quit })
			CHECK(base_t::status_of(base_t::status_name(s)) == s);
		CHECK(base_t::status_of("nonsense") == cmd_status::error);
	}
}

TEST_SUITE("repl_evaluator_base: colors") {

	TEST_CASE("a report has colors only on a terminal") {
		test_evaluator e({ .colors = true });
		string err;
		REQUIRE(eval_quiet(e, "no such command", err).has_value());
		REQUIRE_FALSE(err.empty());
		CHECK((err.find('\x1b') != string::npos) == term::is_tty(stderr));
	}

	TEST_CASE("without the colors option a report has no colors") {
		test_evaluator e;
		string err;
		REQUIRE(eval_quiet(e, "no such command", err).has_value());
		REQUIRE_FALSE(err.empty());
		CHECK(err.find('\x1b') == string::npos);
	}

	// This case writes the global options, so it stays the last one.
	TEST_CASE("a colors option of the user gives colors in a pipe") {
		REQUIRE(idni::options().declare(option_set{ .options = {
			{ "colors", "cli", true, "colored output" } } }).has_value());
		REQUIRE(idni::options().set_text("colors", "true").has_value());
		test_evaluator e({ .colors = true });
		string err;
		REQUIRE(eval_quiet(e, "no such command", err).has_value());
		CHECK(err.find('\x1b') != string::npos);
	}
}
