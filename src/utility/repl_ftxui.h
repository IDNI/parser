// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__REPL_FTXUI_H__
#define __IDNI__PARSER__UTILITY__REPL_FTXUI_H__

#include <string>

#include "repl_history.h"

namespace ftxui { class ScreenInteractive; }

namespace idni {

/// FTXUI-based REPL.
///
/// Same api as `repl<evaluator_t>` (see repl.h):
/// evaluator_t must provide `int eval(const std::string&)` returning
/// 0 = ok, 1 = exit, 2 = incomplete, and a
/// `repl_ftxui<evaluator_t>* r_ftx` back-pointer (declare this struct a
/// friend if it is private).
///
/// Should be in parity with `repl<>`: same key bindings, history
/// behaviour and file format, multiline continuation and pipe-mode fallback.
/// `eval()` output is captured into a stringstream, then emitted via
/// `WithRestoredIO` so it reaches the terminal cleanly after each command.
template <typename evaluator_t>
struct repl_ftxui {
	repl_ftxui(evaluator_t& re, std::string prompt = "> ",
	           std::string history_file = ".history");

	/// Runs the REPL.  Interactive on a TTY, plain getline loop otherwise.
	int run();

	/// Sets the prompt (used by the evaluator's reprompt()).
	void set_prompt(const std::string& p);

	/// Clears the terminal while the interactive screen is temporarily restored.
	void clear();

private:
	int run_interactive();
	int run_pipe();

	void history_up();
	void history_down();
	void history_first();
	void history_last();

	void set_input(const std::string& s); ///< replace input, cursor to end
	void clear_input();                   ///< empty input, cursor to 0

	void store_history(const std::string& s);

	evaluator_t& re_;
	std::string  prompt_;
	repl_history history_;

	std::string input_text_;     ///< current edit buffer (may be multiline)
	int         cursor_pos_ = 0; ///< cursor index within input_text_

	ftxui::ScreenInteractive* screen_ = nullptr;
};

} // namespace idni

#include "repl_ftxui.tmpl.h"

#endif // __IDNI__PARSER__UTILITY__REPL_FTXUI_H__
