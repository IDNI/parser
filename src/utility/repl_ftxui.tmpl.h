// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__REPL_FTXUI_TMPL_H__
#define __IDNI__PARSER__UTILITY__REPL_FTXUI_TMPL_H__

#include <algorithm>
#include <cctype>
#include <concepts>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <unistd.h>
#include <utility>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "repl_ftxui.h"
#include "term.h"

namespace idni {
namespace repl_ftxui_detail {

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

struct scoped_stream_redirect {
	scoped_stream_redirect(std::ostream& os, std::streambuf* next)
	        : os_(os), previous_(os.rdbuf(next)) {}
	scoped_stream_redirect(const scoped_stream_redirect&) = delete;
	scoped_stream_redirect&
	operator=(const scoped_stream_redirect&) = delete;
	~scoped_stream_redirect() { os_.rdbuf(previous_); }

private:
	std::ostream&   os_;
	std::streambuf* previous_;
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

} // namespace repl_ftxui_detail

// --- repl_ftxui ------------------------------------------------------------

template <typename evaluator_t>
repl_ftxui<evaluator_t>::repl_ftxui(evaluator_t& re, std::string prompt,
                                    std::string history_file)
        : re_(re),
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
		int ret = re_.eval(full);
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
	Component line = Renderer(input, [this, input] {
		Element editor = input_text_.empty()
		                       ? (text(" ") | focusCursorBarBlinking)
		                       : input->Render();
		return hbox({ repl_ftxui_detail::ansi_to_element(prompt_),
		              editor });
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

	// Runs `text` through eval() with stdout/stderr captured, then emits it
	// cleanly. Stores history when `store`; exits on ret==1. Returns eval()'s
	// code so the caller can handle ret==2 (incomplete) as it sees fit.
	auto run_line = [&](const std::string& text, bool store) -> int {
		std::ostringstream out_cap, err_cap;
		int                ret = 0;
		{
			repl_ftxui_detail::scoped_stream_redirect
				out_redirect(std::cout, out_cap.rdbuf());
			repl_ftxui_detail::scoped_stream_redirect
				err_redirect(std::cerr, err_cap.rdbuf());
			ret = re_.eval(text);
		}
		if (ret == 2) return ret; // caller decides
		screen_->WithRestoredIO([&] {
			std::cout << "\n" << out_cap.str();
			std::cerr << err_cap.str();
			std::cout.flush();
			std::cerr.flush();
		})();
		if (store) store_history(text);
		if (ret == 1) screen_->Exit();
		return ret;
	};

	Component root = CatchEvent(line, [&](Event e) {
		// Single-key hook: let the evaluator claim a keypress (e.g. a
		// single-key continue/quit prompt) before normal editing sees it.
		if constexpr (repl_ftxui_detail::has_on_repl_key<evaluator_t>) {
			if (std::string k = repl_ftxui_detail::key_token(e);
				!k.empty())
			{
				auto act = re_.on_repl_key(k);
				if (act.kind == repl_key_action::consume)
					return true;
				if (act.kind == repl_key_action::submit) {
					if (run_line(act.line, false) != 1)
						clear_input();
					return true;
				}
			}
		}
		// Enter: evaluate. On incomplete input (eval == 2) break the line and
		// keep editing; otherwise emit output and commit (or exit).
		if (e == Event::Return) {
			if (input_text_.empty())
				return true; // ignore empty enter
			int ret = run_line(input_text_, true);
			// incomplete: insert newline at cursor, continue
			if (ret == 2) {
				size_t cp = std::min((size_t) cursor_pos_,
							input_text_.size());
				input_text_.insert(input_text_.begin() + cp,
							'\n');
				cursor_pos_ = (int) cp + 1;
				return true;
			}
			if (ret != 1) clear_input();
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
	screen_ = nullptr;
	return 0;
}

template <typename evaluator_t>
void repl_ftxui<evaluator_t>::set_prompt(const std::string& p) {
	prompt_ = p;
	if (screen_) screen_->PostEvent(ftxui::Event::Custom); // request redraw
}

template <typename evaluator_t>
void repl_ftxui<evaluator_t>::clear() {
	if (!screen_) return;
	screen_->WithRestoredIO([]{ term::clear(); })();
}

} // namespace idni

#endif // __IDNI__PARSER__UTILITY__REPL_FTXUI_TMPL_H__
