// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__REPL_FTXUI_H__
#define __IDNI__PARSER__UTILITY__REPL_FTXUI_H__

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <streambuf>
#include <string>
#include <thread>

#include "repl_history.h"

namespace ftxui { class ScreenInteractive; }

namespace idni {

class thread_routing_streambuf;

template <typename evaluator_t>
struct repl_eval_widget {
	using completion_fn = std::function<void(int, const std::string&, int,
		bool, bool)>;

	explicit repl_eval_widget(evaluator_t& evaluator);

	bool active() const;
	auto render() const;
	void start(ftxui::ScreenInteractive* screen, std::string text,
		int cursor, bool store, bool allow_incomplete,
		completion_fn completion);
	void handle_custom_event();
	void request_clear();
	void shutdown();

private:
	void finish();
	void drain_output(bool flush_partial);
	std::mutex& out_mutex() { return out_mutex_; }
	std::mutex& err_mutex() { return err_mutex_; }

	evaluator_t& evaluator_;
	ftxui::ScreenInteractive* screen_ = nullptr;
	completion_fn completion_;
	std::atomic<bool> active_{false};
	std::atomic<bool> done_{false};
	std::atomic<bool> ticker_running_{false};
	std::atomic<bool> clear_requested_{false};
	std::thread worker_;
	std::thread ticker_;
	int result_ = 0;
	std::string text_;
	int cursor_ = 0;
	bool store_ = false;
	bool allow_incomplete_ = false;
	size_t spinner_frame_ = 0;
	std::chrono::steady_clock::time_point started_{};
	std::streambuf* saved_cout_buf_ = nullptr;
	std::streambuf* saved_cerr_buf_ = nullptr;
	std::string out_buf_;
	std::string err_buf_;
	std::mutex out_mutex_;
	std::mutex err_mutex_;
	std::unique_ptr<thread_routing_streambuf> out_tsb_;
	std::unique_ptr<thread_routing_streambuf> err_tsb_;
	std::unique_lock<std::mutex> stream_lock_;
};

/// What an evaluator's optional `on_repl_key(key)` hook asks the REPL to do
/// with a single keypress: pass it to normal editing, swallow it, or submit
/// `line` through eval() as if typed (used for single-key prompts).
struct repl_key_action {
	enum kind_t { pass_through, consume, submit } kind = pass_through;
	std::string line{};
};

/// FTXUI-based REPL.
///
/// Same api as `repl<evaluator_t>` (see repl.h):
/// evaluator_t must provide `idni::diagnostics::result<int> eval(const std::string&)`,
/// with a status code always carried as a value (0 = ok, 1 = exit,
/// 2 = incomplete) and a valueless result meaning the command failed; and a
/// `repl_ftxui<evaluator_t>* r_ftx` back-pointer (declare this struct a
/// friend if it is private).
///
/// Optionally, evaluator_t may provide
/// `repl_key_action on_repl_key(const std::string& key)`: when present it is
/// consulted for each keypress before normal editing (key is "enter",
/// "ctrl-c", "escape", or the character), enabling single-key prompts.
///
/// Should be in parity with `repl<>`: same key bindings, history
/// behaviour and file format, multiline continuation and pipe-mode fallback.
/// Interactive evaluation runs on a worker thread so the FTXUI loop can show
/// progress and live output. Pipe mode remains synchronous.
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
	std::mutex& prompt_mutex() { return prompt_mutex_; }
	void apply_eval_result(int result, const std::string& text,
		int cursor, bool store, bool allow_incomplete);

	evaluator_t& re_;
	repl_eval_widget<evaluator_t> eval_widget_;
	std::string  prompt_;
	mutable std::mutex prompt_mutex_;
	repl_history history_;

	std::string input_text_;     ///< current edit buffer (may be multiline)
	int         cursor_pos_ = 0; ///< cursor index within input_text_

	ftxui::ScreenInteractive* screen_ = nullptr;
};

} // namespace idni

#include "repl_ftxui.tmpl.h"

#endif // __IDNI__PARSER__UTILITY__REPL_FTXUI_H__
