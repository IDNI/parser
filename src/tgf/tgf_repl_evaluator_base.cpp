// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <sstream>
#include <string>

#include "parser_strings.h"
#include "tgf_repl_evaluator_base.h"
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

} // namespace idni
