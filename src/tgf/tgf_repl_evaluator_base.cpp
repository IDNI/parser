// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <sstream>
#include <string>
#include <utility>

#include "parser_strings.h"
#include "tgf_repl_evaluator_base.h"
#include "utility/escapes.h"
// last: a second include of the color macros undefines them
#include "parser_term_color_macros.h"

namespace idni {

using namespace std;
using tp = tgf_protocol;

tgf_repl_evaluator_base::tgf_repl_evaluator_base(options o)
	: repl_evaluator_base(o, repl_history_policy{ ".tgf_history" }) {}

std::string tgf_repl_evaluator_base::prompt_text() const {
	bintree<format::json::node>::gc_pause pause;
	const tref st = state();
	const string* grammar = format::json::string_member(st, tp::grammar);
	const string* start = format::json::string_member(st, tp::start);
	stringstream ss;
	if (opt.status) {
		ss << TC_STATUS << "[ ";
		if (grammar)
			ss << TC_STATUS_FILE << "\"" << *grammar << "\""
				<< TC.CLEAR() << TC_STATUS << " "
				<< TC_STATUS_START << (start ? *start : string())
				<< TC.CLEAR() << TC_STATUS;
		else ss << parser_strings::messages::prompt_no_grammar;
		ss << " ]" << TC.CLEAR() << " ";
	}
	ss << TC_PROMPT << "tgf>" << TC.CLEAR() << " ";
	return ss.str();
}

string tgf_repl_evaluator_base::option_name(const trv& s) {
	auto n = s | tgf_repl_parser::option_name;
	return n.has_value() ? n | trv::terminals : string();
}

// The grammar trims the whitespace between the words of a value, so one
// space joins them again.
string tgf_repl_evaluator_base::option_text(const trv& s) {
	string text;
	for (const auto& w : (s | tgf_repl_parser::option_value
		|| tgf_repl_parser::option_word)())
			text += (text.empty() ? "" : " ")
				+ string(w | trv::terminals);
	return text;
}

string tgf_repl_evaluator_base::unquote(const string& s,
	diagnostics::report& rep) {
	if (s.size() < 2 || s[0] != '"' || s[s.size() - 1] != '"') return s;
	auto dec = escapes::decode(
		s.substr(1, s.size() - 2), escapes::tgf_string);
	if (!dec.has_value()) {
		rep.append(std::move(dec.report()));
		return {};
	}
	return std::move(dec).value();
}

} // namespace idni
