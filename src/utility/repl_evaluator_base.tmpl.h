// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <array>
#include <iostream>
#include <utility>

#include "repl_evaluator_base.h"

namespace idni {

inline repl_front_link::repl_front_link(repl_history_policy h)
	: history(std::move(h)) {}

inline void repl_front_link::reprompt() {
	if (front) front->set_prompt(prompt_text());
}

template <typename repl_parser_t, size_t statement_nt, typename options_t>
repl_evaluator_base<repl_parser_t, statement_nt, options_t>::repl_evaluator_base(
	options_t o, repl_history_policy h)
	: repl_front_link(std::move(h)), opt(o)
{
	set_colors();
}

template <typename repl_parser_t, size_t statement_nt, typename options_t>
void repl_evaluator_base<repl_parser_t, statement_nt, options_t>::set_colors() {
	using diagnostics::color_mode;
	report_colors = diagnostics::output_colors(opt.json || !opt.colors
		? color_mode::off
		: idni::options().given("colors") ? color_mode::always
		: color_mode::terminal);
	TC = report_colors.info;
}

template <typename repl_parser_t, size_t statement_nt, typename options_t>
void repl_evaluator_base<repl_parser_t, statement_nt, options_t>::on_parsed(
	typename repl_parser_t::result&) {}

template <typename repl_parser_t, size_t statement_nt, typename options_t>
void repl_evaluator_base<repl_parser_t, statement_nt, options_t>::end_answer() {}

template <typename repl_parser_t, size_t statement_nt, typename options_t>
diagnostics::report
	repl_evaluator_base<repl_parser_t, statement_nt, options_t>::take_report()
{
	diagnostics::report res = std::move(report);
	report.clear();
	return res;
}

template <typename repl_parser_t, size_t statement_nt, typename options_t>
void repl_evaluator_base<repl_parser_t, statement_nt, options_t>::flush_report() {
	print_report(report);
	report.clear();
}

template <typename repl_parser_t, size_t statement_nt, typename options_t>
void repl_evaluator_base<repl_parser_t, statement_nt, options_t>::print_report(
	const diagnostics::report& rep) const
{
	print_report(rep, opt.json, report_colors);
}

template <typename repl_parser_t, size_t statement_nt, typename options_t>
void repl_evaluator_base<repl_parser_t, statement_nt, options_t>::print_report(
	const diagnostics::report& rep, bool json, const term::colors& tc,
	bool print_names)
{
	print_report(rep, json, diagnostics::sink_colors{ tc, tc, tc },
		print_names);
}

template <typename repl_parser_t, size_t statement_nt, typename options_t>
void repl_evaluator_base<repl_parser_t, statement_nt, options_t>::print_report(
	const diagnostics::report& rep, bool json,
	const diagnostics::sink_colors& tc, bool print_names)
{
	if (rep.nodes().empty()) return;
	if (json) {
		bintree<format::json::node>::gc_pause pause;
		format::json::write_line(
			format::json::tree::build_report(rep, print_names),
			std::cout);
		return;
	}
	rep.print({ .error = &std::cerr, .warning = &std::cerr,
		.info = &std::cout }, tc);
}

template <typename repl_parser_t, size_t statement_nt, typename options_t>
const char* repl_evaluator_base<repl_parser_t, statement_nt, options_t>::status_name(
	cmd_status s)
{
	return cmd_status_name(s);
}

template <typename repl_parser_t, size_t statement_nt, typename options_t>
cmd_status repl_evaluator_base<repl_parser_t, statement_nt, options_t>::status_of(
	std::string_view name)
{
	for (const auto& [st, n] : detail::cmd_status_names)
		if (name == n) return st;
	return cmd_status::error;
}

template <typename repl_parser_t, size_t statement_nt, typename options_t>
cmd_status repl_evaluator_base<repl_parser_t, statement_nt, options_t>::report_status(
	const diagnostics::report& rep)
{
	return rep.has_error() ? cmd_status::error : cmd_status::ok;
}

template <typename repl_parser_t, size_t statement_nt, typename options_t>
eval_result repl_evaluator_base<repl_parser_t, statement_nt, options_t>::run_statements(
	const std::string& src,
	const std::function<cmd_result(const trv&)>& step)
{
	eval_result er;
	auto res = repl_parser_t::instance().parse(src.c_str(), src.size());
	if (!res.found) {
		if (opt.continue_on_eof && res.parse_error.at_eof()) {
			er.status = cmd_status::incomplete;
			// The text goes on in the next line, so its cut part is no error.
			er.report.info(parser_strings::messages::incomplete_input);
			return er;
		}
		er.status = cmd_status::error;
		er.report.append(std::move(res.report()));
		return er;
	}
	on_parsed(res);
	tref ref = res.get_shaped_tree2();
	auto t = trv(ref);
	auto statements = t || statement_nt;
	for (const auto& statement : statements()) {
		er.results.push_back(step(statement | trv::only_child));
		const cmd_status st = er.results.back().status;
		if (st == cmd_status::quit || st == cmd_status::incomplete) {
			er.status = st;
			break;
		}
		if (st == cmd_status::error) er.status = cmd_status::error;
	}
	return er;
}

template <typename repl_parser_t, size_t statement_nt, typename options_t>
diagnostics::result<int> repl_evaluator_base<repl_parser_t, statement_nt, options_t>
	::eval(const std::string& src)
{
	eval_result er;
	{
		bintree<format::json::node>::gc_pause pause;
		er = run_statements(src,
			[this](const trv& s) { return run_statement(s); });
	}
	// The answers are printed, and only the handles in er hold data.
	format::json::tree::gc_if_grown();
	if (er.status == cmd_status::incomplete) {
		diagnostics::result<int> res(2);
		res.append(std::move(er.report));
		return res;
	}
	if (er.status == cmd_status::quit) return diagnostics::result<int>(1);
	if (er.report.nodes().size()) {
		report.append(std::move(er.report));
		flush_report();
	}
	end_answer();
	reprompt();
	return diagnostics::result<int>(0);
}

} // namespace idni
