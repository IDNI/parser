// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__REPL_FTXUI_TMPL_H__
#define __IDNI__PARSER__UTILITY__REPL_FTXUI_TMPL_H__

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <concepts>
#include <iostream>
#include <mutex>
#include <sstream>
#include <streambuf>
#include <string>
#include <thread>
#include <utility>
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
	thread_routing_streambuf(std::streambuf* passthrough,
		std::string& buffer, std::mutex& mutex)
		: passthrough_(passthrough), buffer_(buffer), mutex_(mutex) {}

	void set_routed_thread(std::thread::id id) {
		routed_thread_.store(id, std::memory_order_release);
	}

protected:
	int_type overflow(int_type c) override {
		if (traits_type::eq_int_type(c, traits_type::eof())) return c;
		char ch = traits_type::to_char_type(c);
		if (is_routed()) {
			std::lock_guard<std::mutex> lock(mutex_);
			buffer_.push_back(ch);
		} else return passthrough_->sputc(ch);
		return c;
	}

	std::streamsize xsputn(const char* s, std::streamsize n) override {
		if (is_routed()) {
			std::lock_guard<std::mutex> lock(mutex_);
			buffer_.append(s, static_cast<size_t>(n));
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

	std::streambuf* passthrough_;
	std::atomic<std::thread::id> routed_thread_{};
	std::string& buffer_;
	std::mutex& mutex_;
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

// --- repl_ftxui ------------------------------------------------------------

template <typename evaluator_t>
repl_ftxui<evaluator_t>::repl_ftxui(evaluator_t& re, std::string prompt,
                                    std::string history_file)
        : re_(re),
          eval_widget_(re),
          prompt_(std::move(prompt)),
          history_(std::move(history_file)) {
	re_.r_ftx = this; // link evaluator to this repl
}

template <typename evaluator_t>
void repl_ftxui<evaluator_t>::store_history(const std::string& s) {
	history_.store(s);
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
int repl_ftxui<evaluator_t>::run() {
#ifdef __EMSCRIPTEN__
	return run_interactive();
#else
	return isatty(STDIN_FILENO) ? run_interactive() : run_pipe();
#endif
}

template <typename evaluator_t>
int repl_ftxui<evaluator_t>::run_pipe() {
	using namespace std;
	re_.reprompt(); // populate prompt_
	string acc, line;
	cout << prompt_;
	cout.flush();
	while (true) {
		if (!getline(cin, line)) {
			cout << "\n";
			break;
		}
		cout << line << "\n";
		string full = acc.empty() ? line : acc + "\n" + line;
		if (full.empty()) continue;
		int ret = re_.eval(full).value_or(0);
		if (ret == 2) {
			acc = std::move(full);
			continue;
		} // incomplete
		store_history(full); // store on ok and quit (matches repl<>)
		acc.clear();
		if (ret == 1) break;
		cout << prompt_;
		cout.flush();
	}
	return 0;
}

template <typename evaluator_t>
int repl_ftxui<evaluator_t>::run_interactive() {
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

	// Prompt rendered as a left gutter; continuation lines indent beneath it.
	// While an evaluation runs, the submitted text stays visible and the
	// spinner row is drawn below it.
	Component line = Renderer(input, [this, input] {
		std::string prompt;
		{
			std::lock_guard<std::mutex> lock(prompt_mutex());
			prompt = prompt_;
		}
		Element editor = input_text_.empty()
			? (text(" ") | focusCursorBarBlinking)
			: input->Render();
		Element row = hbox({ ansi_to_element(prompt), editor });
		return eval_widget_.active()
			? vbox({ row, eval_widget_.render() }) : row;
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
					start_eval(act.line, false, false);
					return true;
				}
			}
		}
		// Enter starts evaluation and leaves the UI loop responsive.
		if (e == Event::Return) {
			if (input_text_.empty()) return true;
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
			int cp = std::min(cursor_pos_,
			                  (int)input_text_.size());
			if (cp == 0) return true;
			int end = cp;
			// skip trailing non-alnum
			while (cp > 0 && !std::isalnum(
			        (unsigned char)input_text_[cp - 1]))
				--cp;
			// skip alnum word
			while (cp > 0 && std::isalnum(
			        (unsigned char)input_text_[cp - 1]))
				--cp;
			input_text_.erase(cp, end - cp);
			cursor_pos_ = cp;
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
	return 0;
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
	spinner_frame_ = 0;
	started_ = std::chrono::steady_clock::now();
	done_.store(false, std::memory_order_relaxed);
	active_.store(true, std::memory_order_relaxed);
	{
		std::lock_guard<std::mutex> lock(out_mutex());
		out_buf_.clear();
	}
	{
		std::lock_guard<std::mutex> lock(err_mutex());
		err_buf_.clear();
	}
	{
		std::lock_guard<std::mutex> lock(out_mutex());
		out_buf_ += text_;
		out_buf_ += '\n';
	}

	stream_lock_ = std::unique_lock<std::mutex>(repl_stream_mutex());
	saved_cout_buf_ = std::cout.rdbuf();
	saved_cerr_buf_ = std::cerr.rdbuf();
	out_tsb_ = std::make_unique<thread_routing_streambuf>(
		saved_cout_buf_, out_buf_, out_mutex());
	err_tsb_ = std::make_unique<thread_routing_streambuf>(
		saved_cerr_buf_, err_buf_, err_mutex());
	std::cout.rdbuf(out_tsb_.get());
	std::cerr.rdbuf(err_tsb_.get());

	ticker_running_.store(true, std::memory_order_relaxed);
	ticker_ = std::thread([this] {
		while (ticker_running_.load(std::memory_order_relaxed)) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			if (ticker_running_.load(std::memory_order_relaxed) && screen_)
				screen_->PostEvent(ftxui::Event::Custom);
		}
	});
	worker_ = std::thread([this] {
		out_tsb_->set_routed_thread(std::this_thread::get_id());
		err_tsb_->set_routed_thread(std::this_thread::get_id());
		result_ = evaluator_.eval(text_).value_or(0);
		done_.store(true, std::memory_order_release);
		if (screen_) screen_->PostEvent(ftxui::Event::Custom);
	});
}

template <typename evaluator_t>
void repl_eval_widget<evaluator_t>::handle_custom_event() {
	if (!active()) return;
	++spinner_frame_;
	if (clear_requested_.exchange(false))
		screen_->WithRestoredIO([] { term::clear(); })();
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
	active_.store(false, std::memory_order_relaxed);
	if (completion_)
		completion_(result_, text_, cursor_, store_, allow_incomplete_);
}

template <typename evaluator_t>
void repl_eval_widget<evaluator_t>::drain_output(bool flush_partial) {
	auto cutoff = [&](const std::string& buffer) -> size_t {
		if (flush_partial) return buffer.size();
		size_t newline = buffer.rfind('\n');
		return newline == std::string::npos ? 0 : newline + 1;
	};
	std::string out_ready;
	std::string err_ready;
	{
		std::lock_guard<std::mutex> lock(out_mutex());
		size_t count = cutoff(out_buf_);
		out_ready = out_buf_.substr(0, count);
		out_buf_.erase(0, count);
	}
	{
		std::lock_guard<std::mutex> lock(err_mutex());
		size_t count = cutoff(err_buf_);
		err_ready = err_buf_.substr(0, count);
		err_buf_.erase(0, count);
	}
	if (out_ready.empty() && err_ready.empty()) return;
	if (flush_partial && !out_ready.empty() && out_ready.back() != '\n')
		out_ready += '\n';
	if (flush_partial && !err_ready.empty() && err_ready.back() != '\n')
		err_ready += '\n';
	screen_->WithRestoredIO([&] {
		term::clear_line();
		std::cout << out_ready;
		std::cerr << err_ready;
		std::cout.flush();
		std::cerr.flush();
	})();
}

template <typename evaluator_t>
void repl_eval_widget<evaluator_t>::request_clear() {
	if (!active()) return;
	clear_requested_.store(true, std::memory_order_relaxed);
	if (screen_) screen_->PostEvent(ftxui::Event::Custom);
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
		input_text_.insert(input_text_.begin() + cp, '\n');
		cursor_pos_ = static_cast<int>(cp) + 1;
	} else {
		if (store) store_history(text);
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
	if (screen_) screen_->PostEvent(ftxui::Event::Custom); // request redraw
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

} // namespace idni

#endif // __IDNI__PARSER__UTILITY__REPL_FTXUI_TMPL_H__
