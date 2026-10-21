// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__REPL_FTXUI_TMPL_H__
#define __IDNI__PARSER__UTILITY__REPL_FTXUI_TMPL_H__

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <concepts>
#include <functional>
#include <future>
#include <iostream>
#include <iterator>
#include <mutex>
#include <sstream>
#include <streambuf>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#ifdef _WIN32
#include <io.h>
#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif
#define isatty _isatty
#else
#include <unistd.h>
#endif

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "options.h"
#include "repl_ftxui.h"
#include "term.h"

namespace idni {

inline std::mutex& repl_stream_mutex() {
	static std::mutex value;
	return value;
}

// True when evaluator_t opts into single-key handling (see repl_key_action).
template <typename E>
concept has_on_repl_key = requires(E& e, const std::string& k) {
	{ e.on_repl_key(k) } -> std::convertible_to<repl_key_action>;
};

// Neutral key token for on_repl_key: named for specials, else the character.
static inline std::string key_token(const ftxui::Event& e) {
	using namespace ftxui;
	if (e == Event::Return) return "enter";
	if (e == Event::Escape) return "escape";
	if (e == Event::CtrlC)  return "ctrl-c";
	if (e == Event::CtrlD)  return "ctrl-d";
	if (e.is_character())   return e.character();
	return {};
}

class thread_routing_streambuf : public std::streambuf {
public:
	using written_t = std::vector<std::pair<bool, std::string>>;

	thread_routing_streambuf(std::streambuf* passthrough,
		written_t& written, std::mutex& mutex, bool is_err)
		: passthrough_(passthrough), written_(written), mutex_(mutex),
			is_err_(is_err) {}

	void set_routed_thread(std::thread::id id) {
		routed_thread_.store(id, std::memory_order_release);
	}

protected:
	int_type overflow(int_type c) override {
		if (traits_type::eq_int_type(c, traits_type::eof())) return c;
		char ch = traits_type::to_char_type(c);
		if (is_routed()) {
			std::lock_guard<std::mutex> lock(mutex_);
			run().push_back(ch);
		} else return passthrough_->sputc(ch);
		return c;
	}

	std::streamsize xsputn(const char* s, std::streamsize n) override {
		if (is_routed()) {
			std::lock_guard<std::mutex> lock(mutex_);
			run().append(s, static_cast<size_t>(n));
			return n;
		}
		return passthrough_->sputn(s, n);
	}

	int sync() override {
		if (is_routed()) return 0;
		return passthrough_->pubsync();
	}

private:
	bool is_routed() const {
		return std::this_thread::get_id()
			== routed_thread_.load(std::memory_order_acquire);
	}

	// The run this stream appends to; the caller holds mutex_.
	std::string& run() {
		if (written_.empty() || written_.back().first != is_err_)
			written_.emplace_back(is_err_, std::string{});
		return written_.back().second;
	}

	std::streambuf* passthrough_;
	std::atomic<std::thread::id> routed_thread_{};
	written_t& written_;
	std::mutex& mutex_;
	bool is_err_;
};

// Renders a string carrying ANSI SGR sequences (as produced by term::colors)
// into a styled FTXUI element. Only the finite palette term::colors emits is
// handled; unrecognized codes are ignored.
static inline ftxui::Element ansi_to_element(const std::string& s) {
	using namespace ftxui;
	auto base_color = [](int idx, bool bright) -> Color {
		switch (idx) {
		case 0: return bright ? Color::GrayDark : Color::Black;
		case 1: return bright ? Color::RedLight : Color::Red;
		case 2: return bright ? Color::GreenLight : Color::Green;
		case 3: return bright ? Color::YellowLight : Color::Yellow;
		case 4: return bright ? Color::BlueLight : Color::Blue;
		case 5: return bright ? Color::MagentaLight : Color::Magenta;
		case 6: return bright ? Color::CyanLight : Color::Cyan;
		case 7: return bright ? Color::White : Color::GrayLight;
		}
		return Color::Default;
	};
	Color       fg = Color::Default, bg = Color::Default;
	bool        has_fg = false, has_bg = false;
	bool        bold = false, dim = false, ul = false, inv = false;
	Elements    spans;
	std::string buf;
	auto        flush = [&] {
		if (buf.empty()) return;
		Element e = text(buf);
		if (has_fg) e = e | color(fg);
		if (has_bg) e = e | bgcolor(bg);
		if (bold) e = e | ftxui::bold;
		if (dim) e = e | ftxui::dim;
		if (ul) e = e | underlined;
		if (inv) e = e | inverted;
		spans.push_back(e);
		buf.clear();
	};
	auto apply = [&](int p) {
		if (p == 0) {
			fg = bg = Color::Default;
			has_fg = has_bg = false;
			bold = dim = ul = inv = false;
		} else if (p == 1)           bold       = true;
		else if (p == 2)             dim        = true;
		else if (p == 4)             ul         = true;
		else if (p == 7)             inv        = true;
		else if (p == 21 || p == 22) bold = dim = false;
		else if (p == 24)            ul         = false;
		else if (p == 27)            inv        = false;
		else if (p >= 30 && p <= 37) {
				fg = base_color(p - 30, false), has_fg = true;
		} else if (p == 39) {
				fg = Color::Default,            has_fg = false;
		} else if (p >= 90 && p <= 97) {
				fg = base_color(p - 90, true),  has_fg = true;
		} else if (p >= 40 && p <= 47) {
				bg = base_color(p - 40, false), has_bg = true;
		} else if (p == 49) {
				bg = Color::Default,            has_bg = false;
		} else if (p >= 100 && p <= 107) {
				bg = base_color(p - 100, true), has_bg = true;
		}
		// other codes (blink, hidden, ...) are ignored
	};
	for (size_t i = 0; i < s.size();) {
		if (s[i] == '\033' && i + 1 < s.size() && s[i + 1] == '[') {
			flush(), i += 2;
			int cur = 0;
			while (i < s.size() && s[i] != 'm') {
				char c = s[i++];
				if (c >= '0' && c <= '9')
					cur = cur * 10 + (c - '0');
				else if (c == ';') apply(cur), cur = 0;
			}
			if (i < s.size() && s[i] == 'm') apply(cur), i++;
		} else buf += s[i++];
	}
	flush();
	return spans.empty() ? text("") : hbox(spans);
}

// A line count feeds size_t positions, and std::count returns a signed
// difference, so count newlines in size_t directly.
static inline size_t count_newlines(const char* s, size_t n) {
	size_t c = 0;
	for (size_t i = 0; i != n; ++i) if (s[i] == '\n') ++c;
	return c;
}

// Prints newlines so real rows exist below the cursor, then moves back up.
static inline void pad_scroll_rows(size_t newlines, size_t moveup) {
	if (newlines > 0)
		std::cout << std::string(newlines, '\n');
	if (moveup > 0) std::cout << "\033[" << moveup << "A";
}

// Character count of s with ANSI SGR sequences (as ansi_to_element parses) excluded.
static inline size_t visible_width(const std::string& s) {
	size_t n = 0;
	for (size_t i = 0; i < s.size();) {
		if (s[i] == '\033' && i + 1 < s.size() && s[i + 1] == '[') {
			i += 2;
			while (i < s.size() && s[i] != 'm') ++i;
			if (i < s.size()) ++i;
		} else ++n, ++i;
	}
	return n;
}

// --- repl_ftxui ------------------------------------------------------------

template <typename evaluator_t>
repl_ftxui<evaluator_t>::repl_ftxui(evaluator_t& re, std::string prompt,
                                    const repl_history_policy& history)
        : re_(re),
          eval_widget_(re),
          prompt_(std::move(prompt)),
          history_(history) {
	link_front(re_, this, [this](auto& e) { e.r_ftx = this; });
}

template <typename evaluator_t>
diagnostics::result<void> repl_ftxui<evaluator_t>::store_history(
	const std::string& s)
{
	return history_.store(s);
}

template <typename evaluator_t>
void repl_ftxui<evaluator_t>::set_input(const std::string& s) {
	input_text_ = s;
	cursor_pos_ = static_cast<int>(s.size());
}

template <typename evaluator_t>
void repl_ftxui<evaluator_t>::clear_input() {
	input_text_.clear();
	cursor_pos_ = 0;
}

// History navigation — mirrors repl<>::up/down/ctrl_up/ctrl_down exactly,
// including pushing dirty current input when navigating away from the end.
template <typename evaluator_t>
void repl_ftxui<evaluator_t>::history_up() {
	if (auto t = history_.prev(input_text_)) set_input(*t);
}

template <typename evaluator_t>
void repl_ftxui<evaluator_t>::history_down() {
	if (auto t = history_.next()) set_input(*t);
}

template <typename evaluator_t>
void repl_ftxui<evaluator_t>::history_first() {
	if (auto t = history_.first()) set_input(*t);
}

template <typename evaluator_t>
void repl_ftxui<evaluator_t>::history_last() {
	if (auto t = history_.past_end()) set_input(*t);
}

template <typename evaluator_t>
diagnostics::result<int> repl_ftxui<evaluator_t>::run() {
#ifdef __EMSCRIPTEN__
	return run_interactive();
#else
	return isatty(STDIN_FILENO) ? run_interactive() : run_pipe();
#endif
}

template <typename evaluator_t>
diagnostics::result<int> repl_ftxui<evaluator_t>::run_pipe() {
	using namespace std;
	diagnostics::result<int> res;
	re_.reprompt(); // populate prompt_
	string acc, line;
	bool first_line = true;
	cout << prompt_;
	cout.flush();
	while (true) {
		if (!getline(cin, line)) {
			cout << "\n";
			break;
		}
		// only the first line starts the input; acc keeps it without the mark
		if (first_line) term::drop_utf8_bom(line), first_line = false;
		cout << line << "\n";
		string full = acc.empty() ? line : acc + "\n" + line;
		if (full.empty()) continue;
		int ret = 0;
		{
			repl_stop_signal::scope stop;
			ret = re_.eval(full).value_or(0);
		}
		if (ret == 2) {
			acc = std::move(full);
			cout << continuation_prompt_;
			cout.flush();
			continue;
		} // incomplete
		// store on ok and quit (matches repl<>)
		res.merge(store_history(full));
		acc.clear();
		if (ret == 1) break;
		cout << prompt_;
		cout.flush();
	}
	return res.with_value(0);
}

template <typename evaluator_t>
diagnostics::result<int> repl_ftxui<evaluator_t>::run_interactive() {
	using namespace ftxui;

	auto screen = ScreenInteractive::TerminalOutput();
	screen.TrackMouse(false);
	// Take control of Ctrl+C; handle it in the CatchEvent below.
	screen.ForceHandleCtrlC(false);
	screen_ = &screen;
	re_.reprompt();

	InputOption opt;
	opt.content         = &input_text_;
	opt.cursor_position = &cursor_pos_;
	opt.multiline       = true;
	opt.transform       = [](InputState state) { return state.element; };
	Component input     = Input(opt);

	// Prompt rendered as a left gutter, one row per input line: the prompt,
	// then the continuation prompt. While an evaluation runs, the spinner row
	// replaces the prompt row.
	Component line = Renderer(input, [this, input] {
		if (eval_widget_.active()) return eval_widget_.render();
		Elements rows;
		size_t h = count_newlines(input_text_.data(),
			input_text_.size()) + 1;
		for (size_t i = 0; i != h; ++i)
			rows.push_back(ansi_to_element(gutter(i)));
		Element editor = input_text_.empty()
			? (text(" ") | focusCursorBarBlinking)
			: input->Render();
		return hbox({ vbox(std::move(rows)), editor });
	});

	// Is the cursor on the first / last line of the (possibly multiline) buffer?
	auto on_first_line = [this] {
		size_t cp = std::min((size_t) cursor_pos_, input_text_.size());
		return input_text_.find('\n') >= cp; // no '\n' before cursor
	};
	auto on_last_line = [this] {
		size_t cp = std::min((size_t) cursor_pos_, input_text_.size());
		return input_text_.find('\n', cp) == std::string::npos;
	};

	// Prints the prompt and submitted line into terminal history before the
	// spinner frame covers them. Keys and Enter in one batch get no draw in
	// between, so the current line is reprinted from the buffer.
	auto commit_input_rows = [this] {
		screen_->WithRestoredIO([this] {
			size_t cp = std::min(static_cast<size_t>(cursor_pos_),
				input_text_.size());
			size_t cy = count_newlines(input_text_.data(), cp);
			size_t h = count_newlines(input_text_.data(),
				input_text_.size()) + 1;
			size_t line_start = cy == 0 ? 0
				: input_text_.rfind('\n', cp > 0 ? cp - 1 : 0) + 1;
			size_t line_end = input_text_.find('\n', cp);
			if (line_end == std::string::npos) line_end = input_text_.size();
			std::cout << "\r\033[K" << gutter(cy);
			std::cout << input_text_.substr(line_start,
				line_end - line_start);
			size_t down = h - 1 - cy;
			if (down > 0) std::cout << "\033[" << down << "B";
			std::cout << '\n';
			pad_scroll_rows(h - 1, down);
			std::cout.flush();
		})();
	};

	auto start_eval = [this](std::string text, bool store,
		bool allow_incomplete) {
		int cursor = cursor_pos_;
		eval_widget_.start(screen_, std::move(text), cursor, store,
			allow_incomplete,
			[this](int result, const std::string& saved_text,
				int saved_cursor, bool save_history,
				bool restore_incomplete) {
				apply_eval_result(result, saved_text, saved_cursor,
					save_history, restore_incomplete);
			});
	};

	Component root = CatchEvent(line, [&](Event e) {
		if (eval_widget_.active()) {
			if (e == Event::Custom) eval_widget_.handle_custom_event();
			else if (e == Event::CtrlC) eval_widget_.request_stop();
			return true;
		}
		// Single-key hook: let the evaluator claim a keypress (e.g. a
		// single-key continue/quit prompt) before normal editing sees it.
		if constexpr (has_on_repl_key<evaluator_t>) {
			if (std::string k = key_token(e);
				!k.empty())
			{
				auto act = re_.on_repl_key(k);
				if (act.kind == repl_key_action::consume)
					return true;
				if (act.kind == repl_key_action::submit) {
					commit_input_rows();
					start_eval(act.line, false, false);
					return true;
				}
			}
		}
		// Enter starts evaluation and leaves the UI loop responsive.
		if (e == Event::Return) {
			if (input_text_.empty()) return true;
			commit_input_rows();
			start_eval(input_text_, true, true);
			return true;
		}
		// Up/Down: history at the buffer's first/last line, otherwise let the
		// multiline Input move the cursor between lines.
		if (e == Event::ArrowUp) {
			if (on_first_line()) return history_up(), true;
			return false;
		}
		if (e == Event::ArrowDown) {
			if (on_last_line()) return history_down(), true;
			return false;
		}
		if (e == Event::ArrowUpCtrl) return history_first(), true;
		if (e == Event::ArrowDownCtrl) return history_last(), true;
		// Ctrl+A / Ctrl+E: Home / End
		if (e == Event::CtrlA) return cursor_pos_ = 0, true;
		if (e == Event::CtrlE)
			return cursor_pos_ = (int)input_text_.size(), true;
		// Ctrl+K: kill to end of line.
		if (e == Event::CtrlK) {
			size_t cp = std::min((size_t)cursor_pos_,
			                     input_text_.size());
			size_t nl = input_text_.find('\n', cp);
			if (nl == std::string::npos)
				nl = input_text_.size();
			input_text_.erase(cp, nl - cp);
			// cursor_pos_ stays at cp (text after it was removed)
			return true;
		}
		// Ctrl+U: kill to start of line.
		if (e == Event::CtrlU) {
			size_t cp = std::min((size_t)cursor_pos_,
			                     input_text_.size());
			size_t sol = input_text_.rfind('\n',
			        cp > 0 ? cp - 1 : std::string::npos);
			size_t from = sol == std::string::npos ? 0 : sol + 1;
			input_text_.erase(from, cp - from);
			cursor_pos_ = (int)from;
			return true;
		}
		// Ctrl+W: delete word backward (alphanumeric word boundaries).
		if (e == Event::CtrlW) {
			size_t cp = std::min(static_cast<size_t>(cursor_pos_),
			                  input_text_.size());
			if (cp == 0) return true;
			size_t end = cp;
			// skip trailing non-alnum
			while (cp > 0 && !std::isalnum(
			        (unsigned char)input_text_[cp - 1]))
				--cp;
			// skip alnum word
			while (cp > 0 && std::isalnum(
			        (unsigned char)input_text_[cp - 1]))
				--cp;
			input_text_.erase(cp, end - cp);
			cursor_pos_ = static_cast<int>(cp);
			return true;
		}
		// Ctrl+C: cancel a non-empty buffer in place (FTXUI redraws a clean
		// empty prompt); exit when the buffer is already empty.
		if (e == Event::CtrlC) {
			if (input_text_.empty()) return screen_->Exit(), true;
			return clear_input(), true;
		}
		// Ctrl+D: always exit.
		if (e == Event::CtrlD) return screen_->Exit(), true;
		return false;
	});

	screen.Loop(root);
	eval_widget_.shutdown();
	screen_ = nullptr;
	diagnostics::result<int> res;
	res.report().append(std::move(history_report_));
	history_report_ = {};
	return res.with_value(0);
}

template <typename evaluator_t>
repl_eval_widget<evaluator_t>::repl_eval_widget(evaluator_t& evaluator)
	: evaluator_(evaluator) {}

template <typename evaluator_t>
bool repl_eval_widget<evaluator_t>::active() const {
	return active_.load(std::memory_order_relaxed);
}

template <typename evaluator_t>
auto repl_eval_widget<evaluator_t>::render() const {
	using namespace ftxui;
	using namespace std::chrono;
	int64_t micros = duration_cast<microseconds>(
		steady_clock::now() - started_).count();
	return hbox({ spinner(15, spinner_frame_), text("  evaluating  "),
		text(diagnostics::report::format_time(micros)) });
}

template <typename evaluator_t>
void repl_eval_widget<evaluator_t>::start(ftxui::ScreenInteractive* screen,
	std::string text, int cursor, bool store, bool allow_incomplete,
	completion_fn completion)
{
	screen_ = screen;
	text_ = std::move(text);
	cursor_ = cursor;
	store_ = store;
	allow_incomplete_ = allow_incomplete;
	completion_ = std::move(completion);
	result_ = 0;
	stop_requested_.store(false, std::memory_order_relaxed);
	spinner_frame_ = 0;
	started_ = std::chrono::steady_clock::now();
	done_.store(false, std::memory_order_relaxed);
	active_.store(true, std::memory_order_relaxed);
	{
		std::lock_guard<std::mutex> lock(written_mutex_);
		written_.clear();
	}

	stream_lock_ = std::unique_lock<std::mutex>(repl_stream_mutex());
	saved_cout_buf_ = std::cout.rdbuf();
	saved_cerr_buf_ = std::cerr.rdbuf();
	out_tsb_ = std::make_unique<thread_routing_streambuf>(
		saved_cout_buf_, written_, written_mutex_, false);
	err_tsb_ = std::make_unique<thread_routing_streambuf>(
		saved_cerr_buf_, written_, written_mutex_, true);
	std::cout.rdbuf(out_tsb_.get());
	std::cerr.rdbuf(err_tsb_.get());

	ticker_running_.store(true, std::memory_order_relaxed);
	ticker_ = std::thread([this] {
		while (ticker_running_.load(std::memory_order_relaxed)) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			post_event(screen_, true);
		}
	});
	// the evaluator reads and sets the options of the thread that runs
	// the REPL, which waits for the worker and leaves them alone meanwhile
	worker_ = std::thread([this, repo = &idni::options()] {
		options_scope scope(*repo);
		out_tsb_->set_routed_thread(std::this_thread::get_id());
		err_tsb_->set_routed_thread(std::this_thread::get_id());
		result_ = evaluator_.eval(text_).value_or(0);
		done_.store(true, std::memory_order_release);
		post_event(screen_);
	});
}

template <typename evaluator_t>
void repl_eval_widget<evaluator_t>::handle_custom_event() {
	if (!active()) return;
	++spinner_frame_;
	if (clear_requested_.exchange(false)) {
		std::lock_guard<std::mutex> lock(screen_io_mutex_);
		screen_->WithRestoredIO([] { term::clear(); })();
	}
	if (done_.load(std::memory_order_acquire)) finish();
}

template <typename evaluator_t>
void repl_eval_widget<evaluator_t>::finish() {
	ticker_running_.store(false, std::memory_order_relaxed);
	if (ticker_.joinable()) ticker_.join();
	if (worker_.joinable()) worker_.join();
	drain_output(true);
	std::cout.rdbuf(saved_cout_buf_);
	std::cerr.rdbuf(saved_cerr_buf_);
	saved_cout_buf_ = saved_cerr_buf_ = nullptr;
	out_tsb_.reset();
	err_tsb_.reset();
	stream_lock_.unlock();
	stop_requested_.store(false, std::memory_order_relaxed);
	active_.store(false, std::memory_order_relaxed);
	if (completion_)
		completion_(result_, text_, cursor_, store_, allow_incomplete_);
}

template <typename evaluator_t>
void repl_eval_widget<evaluator_t>::drain_output(bool flush_partial) {
	// Complete lines only, unless flush_partial: every run up to the last
	// newline written.
	std::vector<std::pair<bool, std::string>> ready;
	{
		std::lock_guard<std::mutex> lock(written_mutex_);
		if (flush_partial) ready.swap(written_);
		else for (auto it = written_.rbegin(); it != written_.rend(); ++it) {
			size_t newline = it->second.rfind('\n');
			if (newline == std::string::npos) continue;
			auto last = std::prev(it.base());
			ready.assign(written_.begin(), last);
			ready.emplace_back(last->first,
				last->second.substr(0, newline + 1));
			last->second.erase(0, newline + 1);
			written_.erase(written_.begin(), last);
			break;
		}
	}
	std::erase_if(ready, [](const auto& run) { return run.second.empty(); });
	if (ready.empty()) return;
	if (flush_partial && ready.back().second.back() != '\n')
		ready.back().second += '\n';
	std::lock_guard<std::mutex> lock(screen_io_mutex_);
	screen_->WithRestoredIO([&] {
		// Clears rows the previous frame drew below the cursor, not just the current line.
		std::cout << "\r\033[J";
		for (const auto& [is_err, run] : ready)
			(is_err ? std::cerr : std::cout) << run << std::flush;
		// Scrolls past the next frame's stale reset-cursor move so it lands below this text, not on it.
		const int dimy = screen_->dimy();
		const size_t dy = dimy > 0 ? static_cast<size_t>(dimy - 1) : 0;
		pad_scroll_rows(dy, dy);
		std::cout.flush();
	})();
}

template <typename evaluator_t>
void repl_eval_widget<evaluator_t>::post_and_wait(
	const std::function<void()>& task)
{
	std::promise<void> done;
	std::future<void> finished = done.get_future();
	{
		std::lock_guard<std::mutex> lock(screen_io_mutex_);
		ticker_paused_.store(true, std::memory_order_relaxed);
		// The FTXUI senders and its exit stack are not thread safe, so
		// only the loop thread may uninstall and install the screen.
		screen_->Post([&task, &done] {
			task();
			done.set_value();
		});
	}
	finished.wait();
	ticker_paused_.store(false, std::memory_order_relaxed);
}

template <typename evaluator_t>
void repl_eval_widget<evaluator_t>::run_on_loop(
	const std::function<void()>& fn)
{
	post_and_wait([this, &fn] {
		drain_output(true);
		fn();
	});
}

template <typename evaluator_t>
void repl_eval_widget<evaluator_t>::post_event(
	ftxui::ScreenInteractive* screen, bool from_ticker)
{
	// The loop thread resets the sender that PostEvent reads in each restore.
	std::lock_guard<std::mutex> lock(screen_io_mutex_);
	if (from_ticker && (!ticker_running_.load(std::memory_order_relaxed)
		|| ticker_paused_.load(std::memory_order_relaxed))) return;
	if (screen) screen->PostEvent(ftxui::Event::Custom);
}

template <typename evaluator_t>
void repl_eval_widget<evaluator_t>::request_redraw(
	ftxui::ScreenInteractive* screen)
{
	post_event(screen);
}

template <typename evaluator_t>
void repl_eval_widget<evaluator_t>::request_stop() {
	if (active()) stop_requested_.store(true, std::memory_order_relaxed);
}

template <typename evaluator_t>
bool repl_eval_widget<evaluator_t>::stop_requested() const {
	return stop_requested_.load(std::memory_order_relaxed);
}

template <typename evaluator_t>
void repl_eval_widget<evaluator_t>::request_clear() {
	if (!active()) return;
	clear_requested_.store(true, std::memory_order_relaxed);
	post_event(screen_);
}

template <typename evaluator_t>
void repl_eval_widget<evaluator_t>::shutdown() {
	if (!worker_.joinable() && !ticker_.joinable()) return;
	ticker_running_.store(false, std::memory_order_relaxed);
	if (ticker_.joinable()) ticker_.join();
	if (worker_.joinable()) worker_.join();
	drain_output(true);
	if (saved_cout_buf_) std::cout.rdbuf(saved_cout_buf_);
	if (saved_cerr_buf_) std::cerr.rdbuf(saved_cerr_buf_);
	out_tsb_.reset();
	err_tsb_.reset();
	if (stream_lock_.owns_lock()) stream_lock_.unlock();
	active_.store(false, std::memory_order_relaxed);
	screen_ = nullptr;
}

template <typename evaluator_t>
void repl_ftxui<evaluator_t>::apply_eval_result(int result,
	const std::string& text, int cursor, bool store, bool allow_incomplete)
{
	if (result == 2 && allow_incomplete) {
		input_text_ = text;
		size_t cp = std::min(static_cast<size_t>(cursor), input_text_.size());
		input_text_.insert(cp, 1, '\n');
		cursor_pos_ = static_cast<int>(cp) + 1;
	} else {
		if (store)
			history_report_.append(std::move(store_history(text)).report());
		clear_input();
	}
	if (result == 1) screen_->Exit();
	else if (screen_) screen_->PostEvent(ftxui::Event::Custom);
}

template <typename evaluator_t>
void repl_ftxui<evaluator_t>::set_prompt(const std::string& p) {
	{
		std::lock_guard<std::mutex> lock(prompt_mutex());
		prompt_ = p;
	}
	eval_widget_.request_redraw(screen_);
}

template <typename evaluator_t>
void repl_ftxui<evaluator_t>::set_continuation_prompt(const std::string& p) {
	{
		std::lock_guard<std::mutex> lock(prompt_mutex());
		continuation_prompt_ = p;
	}
	eval_widget_.request_redraw(screen_);
}

template <typename evaluator_t>
std::string repl_ftxui<evaluator_t>::gutter(size_t row) {
	std::lock_guard<std::mutex> lock(prompt_mutex());
	size_t pw = visible_width(prompt_);
	size_t cw = visible_width(continuation_prompt_);
	size_t w = std::max(pw, cw);
	// the prompt keeps its column, the continuation prompt ends where it ends
	if (row == 0) return prompt_ + std::string(w - pw, ' ');
	return std::string(w - cw, ' ') + continuation_prompt_;
}

template <typename evaluator_t>
void repl_ftxui<evaluator_t>::clear() {
	if (!screen_) return;
	if (eval_widget_.active()) {
		eval_widget_.request_clear();
		return;
	}
	screen_->WithRestoredIO([] { term::clear(); })();
}

// A modal dialog below the last output: @p content draws the inside of
// the box, @p key takes every key and returns true to close. The box needs
// no alternate screen, which some terminals lack, and its rows clear at the
// end, so the prompt comes back to its row.
inline void run_screen_dialog(const std::function<ftxui::Element()>& content,
	const std::function<bool(const ftxui::Event&)>& key)
{
	using namespace ftxui;
	auto screen = ScreenInteractive::TerminalOutput();
	screen.TrackMouse(false);
	screen.ForceHandleCtrlC(false);
	Component box = Renderer([&] { return content() | border | center; });
	Component root = CatchEvent(box, [&](Event e) {
		if (key(e)) screen.Exit();
		return true;
	});
	screen.Loop(root);
}

template <typename evaluator_t>
bool repl_ftxui<evaluator_t>::stop_requested() {
	if (!screen_) return repl_stop_signal::requested();
	return eval_widget_.stop_requested();
}

// The dialog of ask_yes_no.
inline diagnostics::result<bool> ask_yes_no_in_dialog(
	const std::string& prompt)
{
	using namespace ftxui;
	diagnostics::result<bool> r;
	std::optional<bool> answer;
	bool yes_selected = false;
	auto button = [](const char* label, bool selected) {
		Element b = text(label);
		return selected ? b | inverted : b;
	};
	run_screen_dialog([&] {
		return vbox({ text(prompt),
			hbox({ button("[ Yes ]", yes_selected), text(" "),
				button("[ No ]", !yes_selected) }) });
	}, [&](const Event& e) {
		if (e == Event::Character('y') || e == Event::Character('Y'))
			return answer = true, true;
		if (e == Event::Character('n') || e == Event::Character('N'))
			return answer = false, true;
		if (e == Event::Return) return answer = yes_selected, true;
		if (e == Event::Escape || e == Event::CtrlC) return true;
		if (e == Event::ArrowLeft || e == Event::ArrowRight
				|| e == Event::Tab || e == Event::TabReverse)
			yes_selected = !yes_selected;
		return false;
	});
	if (!answer) return r.with_error(diagnostics::code::io_error,
		parser_strings::messages::answer_cancelled);
	return r.with_value(*answer);
}

template <typename evaluator_t>
diagnostics::result<bool> repl_ftxui<evaluator_t>::ask_yes_no(
	const std::string& prompt)
{
	if (!screen_) return read_yes_no(prompt);
	diagnostics::result<bool> r;
	auto ask = [&] { r = ask_yes_no_in_dialog(prompt); };
	// The nested loop of the dialog suspends this screen itself, so the
	// terminal must not be restored around it.
	if (eval_widget_.active()) eval_widget_.run_on_loop(ask);
	else ask();
	return r;
}

} // namespace idni

#endif // __IDNI__PARSER__UTILITY__REPL_FTXUI_TMPL_H__
