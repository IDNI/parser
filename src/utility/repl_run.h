// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__REPL_RUN_H__
#define __IDNI__PARSER__UTILITY__REPL_RUN_H__

#include "repl.h"
#include "repl_history_policy.h"
#ifdef TAU_PARSER_HAS_FTXUI
#include "repl_ftxui.h"
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten/em_asm.h>
#endif

namespace idni {

#ifdef __EMSCRIPTEN__
/// True under node when stdin is no terminal, for node or for the module
/// (a stdin pre-js gives the module a non-terminal stdin). Always false in
/// a browser page, also when the page sets Module["stdin"].
inline bool repl_stdin_piped_under_node() {
	const int node_tty = MAIN_THREAD_EM_ASM_INT({
		if (typeof ENVIRONMENT_IS_NODE === 'undefined'
			|| !ENVIRONMENT_IS_NODE || typeof require === 'undefined')
				return -1;
		return require('tty').isatty(0) ? 1 : 0;
	});
	if (node_tty < 0) return false;
	return node_tty == 0 || !isatty(STDIN_FILENO);
}
#endif

/// Run the FTXUI REPL on @p re, or the legacy REPL with @p legacy or in a
/// build without FTXUI. The evaluator sets the prompt in its reprompt().
/// The report holds every failed write of the history file.
template <typename evaluator_t>
diagnostics::result<int> run_repl(evaluator_t& re, bool legacy,
	const repl_history_policy& history)
{
#ifdef TAU_PARSER_HAS_FTXUI
#ifdef __EMSCRIPTEN__
	// repl_ftxui runs interactive under Emscripten whatever stdin is, and
	// waits forever after the first line of a piped stdin.
	if (repl_stdin_piped_under_node()) legacy = true;
#endif
	if (!legacy) {
		repl_ftxui<evaluator_t> front(re, "", history);
		return front.run();
	}
#else
	(void)legacy;
#endif
	repl<evaluator_t> front(re, "", history);
	re.reprompt();
	return front.run();
}

} // namespace idni

#endif // __IDNI__PARSER__UTILITY__REPL_RUN_H__
