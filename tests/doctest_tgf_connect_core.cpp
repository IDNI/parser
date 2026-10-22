// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "tgf/tgf_repl_evaluator_base.h"

#include <sstream>
#include <string>

using namespace idni;
using jt = format::json::tree;

namespace {

static tgf_repl_options text_options() {
	tgf_repl_options options;
	options.colors = false;
	return options;
}

struct renderer_evaluator : tgf_repl_evaluator_base {
	renderer_evaluator() : tgf_repl_evaluator_base(text_options()) {}
	std::ostringstream output;

	tref state() const override {
		return jt::build_object({});
	}

	cmd_result run_statement(const trv& statement) override {
		cmd_result result;
		result.cmd = name_of(statement | trv::nonterminal);
		result.data = jt::geth(jt::build_object({ jt::build_pair(
			tgf_protocol::version, jt::build_string("client-core-version")) }));
		render_text(result, output);
		return result;
	}
};

} // namespace

TEST_CASE("the client core parses and renders without a grammar loader") {
	options_repository repo;
	options_scope scope(repo);
	renderer_evaluator evaluator;
	auto result = evaluator.eval("version");
	REQUIRE(result.has_value());
	CHECK_FALSE(result.has_error());
	CHECK(result.value() == 0);
	CHECK(evaluator.output.str() == "client-core-version\n");
}

TEST_CASE("the client core builds a session-free error response") {
	bintree<format::json::node>::gc_pause pause;
	diagnostics::report report;
	report.error(diagnostics::code::invalid_argument, "bad request");
	const jt& response = jt::get(tgf_repl_evaluator_base::error_response(
		jt::build_number(7), report));
	CHECK(response.find("id") == jt::build_number(7));
	CHECK(response.find("state") == nullptr);
	CHECK(response.find("cmd") == nullptr);
	CHECK(format::json::report_of(response.find("report")).has_error());
}
