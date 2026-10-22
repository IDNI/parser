// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include "tgf_connect.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "format/json/json.h"
#include "parser_strings.h"
#include "utility/diagnostics.h"
#include "utility/repl_run.h"

namespace idni {

diagnostics::result<connect_target> parse_connect_iri(std::string_view iri)
{
	return parse_connect_address(iri, tgf_address_rules());
}

diagnostics::result<int> tgf_connect_run(const tgf_connect_options& opt) {
	diagnostics::result<int> res;
	tgf_repl_options ro;
	ro.colors = opt.client.colors;
	ro.status = opt.status;
	ro.json = opt.client.json;
	repl_client_evaluator re(std::cout, ro);
	// A lost connection is an error of the report, so it holds no code.
	auto finish = [&res, &re](int code) {
		res.report().append(re.take_report());
		if (!res.report().has_error()) res.emplace(code);
		return std::move(res);
	};
	if (!re.open_session(opt.client)) {
		res.report().append(re.take_report());
		return res;
	}
	if (!opt.client.json) {
		std::cout << "session " << re.session_id() << "\n";
		std::cout.flush();
	}
	// The answer of a failed load already printed its report.
	if (!opt.grammar_file.empty()
			&& re.load_grammar(opt.grammar_file) != cmd_status::ok)
		return finish(1);
	if (opt.client.json) return finish(re.run_json(std::cin));
	auto run = run_repl(re, opt.client.legacy_repl, re.history);
	res.report().append(std::move(run).report());
	return finish(run.has_value() ? run.value() : 1);
}

int tgf_connect_command(const parsed_args& args,
	const std::string& grammar_file)
{
	auto& opts = idni::options();
	const std::vector<std::string>& inputs = args.inputs;
	tgf_connect_options opt;
	// Only argv counts as given: a default or an environment value of
	// --host or --port does not conflict with an address argument.
	auto given = [&args](std::string_view name) {
		return std::ranges::any_of(args.values,
			[name](const auto& v) { return v.first == name; });
	};
	if (!inputs.empty()) {
		if (given("host") || given("port"))
			return command_line_error("give the server either as an "
				"address argument or with --host and --port");
		auto t = parse_connect_iri(inputs.front());
		if (!t.has_value()) {
			tgf_repl_evaluator_base::print_report(t.report(), false,
				tgf_report_colors());
			return 1;
		}
		opt.client.target = std::move(t).value();
	} else {
		size_t port = opts.get<size_t>("port");
		if (port < 1 || port > 65535) return command_line_error(
			"--port must be between 1 and 65535");
		opt.client.target.kind = line_transport_kind::tcp;
		opt.client.target.host = opts.get<std::string>("host");
		opt.client.target.port = static_cast<uint16_t>(port);
	}
	if (const auto session = opts.get<std::string>("session");
			!session.empty())
		opt.client.open_fields = format::json::tree::geth(
			format::json::tree::build_object({
				format::json::tree::build_pair(tgf_protocol::session,
					format::json::tree::build_string(session)) }));
	opt.client.legacy_repl = opts.get<bool>("legacy-repl");
	opt.client.limits.max_line = opts.get<size_t>("max-answer");
	// the client prints answers on stdout and errors on stderr with one set
	const auto colors = tgf_report_colors();
	opt.client.colors = colors.error.enabled && colors.info.enabled;
	opt.client.json = opts.get<bool>("json");
	opt.grammar_file = grammar_file;
	opt.status = opts.get<bool>("status");
	auto r = tgf_connect_run(opt);
	tgf_repl_evaluator_base::print_report(r.report(), opt.client.json,
		tgf_report_colors());
	return r.has_value() ? r.value() : 1;
}

} // namespace idni
