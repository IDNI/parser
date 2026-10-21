// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__REPL_H__
#define __IDNI__PARSER__UTILITY__REPL_H__

#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <climits>
#include <stdio.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "term.h"
#include "repl_history.h"

namespace idni {

/// The calls an evaluator makes on the front end that runs it, so one
/// pointer serves repl<> and repl_ftxui<>.
struct repl_front {
	virtual ~repl_front() = default;
	virtual void set_prompt(const std::string& p) = 0;
	virtual void clear() = 0;
};

/// Link the evaluator @p re to the front end @p f through its member
/// front, or call @p fallback with @p re when it has no such member.
template <typename evaluator_t, typename fallback_t>
void link_front(evaluator_t& re, repl_front* f, fallback_t fallback) {
	if constexpr (requires { re.front; }) re.front = f;
	else fallback(re);
}

/// evaluator_t must be a class with:
/// - a method idni::diagnostics::result<int> eval(const std::string&)
///         which allows to plug own evaluation logic.
///         A status code is always a value: 0 for normal operation, 1 if
///         the repl should exit, 2 if the input was incomplete. A
///         valueless result means the command failed.
/// - a member repl_front* front = nullptr, or a member
///         repl<evaluator_t>* r = 0; accessible to this struct
///         if it is private, add friend struct repl<evaluator_t>;
template <typename evaluator_t>
struct repl : repl_front {
	repl(evaluator_t& re, std::string prompt = "> ",
		const repl_history_policy& history = {})
		: re_(re), prompt_(prompt), history_(history)
	{
		link_front(re, this, [this](auto& e) { e.r = this; });
		term::open();
	}
	~repl() override { term::close(); }
	bool echo = true;
	/// The report holds every failed write of the history file.
	diagnostics::result<int> run() {
		diagnostics::result<int> res;
		clear_input();
		reset_input();
		while (1) {
			std::string s;
			char ch;
			std::ptrdiff_t read;
			while ((read = term::in(ch)) == 1) {
				TDBG(std::cerr << "\n";) TDBG(print_debug();)
				TDBG(std::cerr << " <pressed '" << ch << "' = "
						<< static_cast<int>(ch) << ">";)
				// control characters
				if (ch == 3 || ch == 4) // ctrl-c or d
					return res.with_value(0);
				if      (ch == 1) home();
				else if (ch == 5) end();
				else if (ch == 8 || ch == 127) backspace();
				else if (ch == 27) { // escape seq
					char c;
					if (term::in(c) != 1 || c != 91
						|| term::in(c) != 1) continue;
					if (c == 49) { // ctrl
						if (term::in(c)!=1 || c != 59 ||
						    term::in(c)!=1 || c != 53 ||
						    term::in(c)!=1) continue;
						if      (c == 65) ctrl_up();
						else if (c == 66) ctrl_down();
						else if (c == 67) ctrl_right();
						else if (c == 68) ctrl_left();
					}
					else if (c == 65) up();
					else if (c == 66) down();
					else if (c == 67) right();
					else if (c == 68) left();
					else if (c == 70) end();
					else if (c == 72) home();
					else if (c == 51)
						if (term::in(c) == 1
							&& c == 126) del();
					//#ifdef DEBUG
					//else std::cerr << "unknown escape seq: "
					//	<< static_cast<int>(c) << "\n";
					//#endif // DEBUG
				}
				else {  // not a control char or ENTER
					if (input_.empty()
						&& (ch == 10 || ch == 13))
							continue;
					if (echo) term::out(std::string{ ch });
					pos_++;
					// add to input
					if (ch == 10 || ch == 13) {
						s = enter();
						break; // exit input loop to evaluate
					} else write(ch);
				}
				TDBG(std::cerr << "\n";) TDBG(print_debug();)
			}
			if (!s.empty()) { // evaluate input
				//TDBG(std::cerr << " <input_size(): " << input_.size() << " post: " << (pos_-1) << ">";)
				//if (pos_ - 1 < input_.size()) split_line();
				//else {
					auto ret = evaluate(s, res);
					if (ret == 1) break;
					if (ret == 2) continue;
				//}
			}
			if (read == 0 && is_pipe_) break;
		}
		return res.with_value(0);
	}
	void split_line() {
		if (is_pipe_) return;
		size_t r = r_;
		clear_input();
		input_.insert(it_at(pos_ - 1), '\n');
		print_input(), go(r, 0);
	}
	/// Merges the report of the history store into @p res.
	int evaluate(const std::string& line, diagnostics::result<int>& res) {
		std::string s = line;
		// only the first command, maybe over several lines, starts the input
		if (!input_started_) term::drop_utf8_bom(s);
		auto ret = re_.eval(s).value_or(0);
		if (ret != 2) input_started_ = true;
		TDBG(std::cerr << " <EVAL: " << ret << ">";)
		if (ret == 2) { // Unexpected end of file, continue
			input_.insert(it_at(pos_ - 1), '\n');
			// the interactive editor starts continuation rows at
			// column 0 (see update_widths), so only a pipe shows it
			if (is_pipe_) term::out(continuation_prompt_.c_str(),
				continuation_prompt_.size());
			return ret;
		}
		res.merge(history_.store(s));
		if (ret == 1) return ret; // exit loop if 1
		reset_input();
		return 0;
	}
	/// Sets the prompt
	void prompt(const std::string& p) {
		TDBG(std::cerr << " <PROMPT: `" << p << "`>";)
		update_widths(), clear_input();
		prompt_ = p, print_input();
		TDBG(std::cerr << " <PROMPT PRINTED>";)
	}
	std::string enter() {
		TDBG(std::cerr << " <ENTER>";)
		auto s = get();
		new_line();
		TDBG(std::cerr << "\n";) TDBG(print_debug();)
		return s;
	}
	void write(char ch) {
		TDBG(std::cerr << " <WRITE: '" << ch << "' = "
			<< static_cast<int>(ch) << ">";)
		size_t r = r_, c = c_;
		TDBG(print_debug();)
		clear_input();
		input_.insert(it_at(pos_ - 1), ch);
		if (is_pipe_) return;
		print_input();
		go(r, c);
		TDBG(print_debug();)
		if (c_ >= lws_[r_]) next_line();
		else c_++, term::cursor_right();
		TDBG(print_debug();)
	}
	void set_prompt(const std::string& p) override { prompt_ = p; }
	/// Sets the prompt of the lines after the first while the input is
	/// incomplete (pipe mode only); empty by default
	void set_continuation_prompt(const std::string& p) {
		continuation_prompt_ = p;
	}
	void clear() override { if (!is_pipe_) term::clear(); }
	/// Returns the current prompt
	std::string prompt() const { return prompt_; }
private:
	/// Iterator at index i: an iterator moves by a signed difference while the
	/// index is a size, so the single conversion lives here.
	std::vector<char>::iterator it_at(size_t i) {
		return input_.begin() + static_cast<std::ptrdiff_t>(i);
	}
	/// Returns the current input as a string
	std::string get() const {
		return std::string(input_.begin(), input_.end());
	}
	/// Sets the current input from a string
	void set(const std::string& s) {
		clear_input();
		input_.assign(s.begin(), s.end()), pos_ = input_.size();
		print_input();
	}
	/// Clears the input line (set to empty)
	void set() { clear_input(), input_.clear(), pos_ = 0, print_input(); }
#ifdef TERM_DEBUG
	void print_debug() {
		std::cerr << " <POS: " << pos_ << ", IN.SIZE: " << input_.size()
			<< ", R: " << r_ << ", C: " << c_ << " LWS:";
		for (size_t i = 0; i != lws_.size(); ++i)
			std::cerr << " " << lws_[i];
		std::cerr << " >";
	}
#endif // TERM_DEBUG
	void prev_line() {
		TDBG(std::cerr << " <PREV LINE: [" << r_ << ", " << c_ << "]>";)
		if (r_) {
			TDBG(print_debug();) TDBG(std::cerr << " <PREV LINE: ["
				<< r_ << ", " << c_ << "] -> ";)
			c_ = lws_[--r_];
			TDBG(std::cerr << "[" << r_ << ", " << c_ << "] >";)
			term::cursor_up();
			term::cursor_right(c_);
		}
	}
	void next_line() {
		TDBG(std::cerr << " <NEXT LINE: [" << r_ << ", " << c_ << "]>";)
		term::cursor_left(c_);
		term::cursor_down();
		new_line();
	}
	void new_line() {
		if (is_pipe_) return;
		lws_[r_++] = c_, c_ = 0;
		TDBG(std::cerr << " <NEW LINE: [" << r_ << ", " << c_ << "]>";)
		while (lws_.size() <= r_) lws_.push_back(0);
	}
	void go(size_t r, size_t c) {
		if (is_pipe_) return;
		TDBG(std::cerr << " <GO: [" << r << ", " << c << "] -> "
			<< "relative [up:" << (r_ - r) << ", left: "
			<< (c_ - c) << "]";)
		term::cursor_up(r_ - r);
		term::cursor_left(c_ - c);
		r_ = r, c_ = c;
		TDBG(std::cerr << " >";)
	}
	/// Delete character before the cursor
	void backspace() {
		if (pos_ == 0) return;
		size_t r = r_, c = c_;
		if (c) c--;
		else if (r) c = lws_[--r];
		clear_input();
		input_.erase(it_at(--pos_));
		print_input();
		go(r, c);
	}
	/// Delete character after the cursor
	void del() {
		if (pos_ >= input_.size()) return;
		size_t r = r_, c = c_;
		clear_input();
		input_.erase(it_at(pos_));
		print_input();
		go(r, c);
	}
	/// Move cursor left
	void left() {
		if (pos_) {
			pos_--;
			if (c_) c_--, term::cursor_left();
			else if (r_) prev_line();
		}
	}
	/// Move cursor right
	void right() {
		if (pos_ < input_.size()) {
			pos_++;
			if (c_ >= lws_[r_]) next_line();
			else c_++, term::cursor_right();
		}
	}
	/// Move cursor word left
	void ctrl_left() {
		if (pos_ == 0) return;
		left();
		while (pos_ > 0 && !std::isalnum(input_[pos_]))     left();
		while (pos_ > 0 &&  std::isalnum(input_[pos_ - 1])) left();
	}
	/// Move cursor word right
	void ctrl_right() {
		if (pos_ == input_.size()) return;
		right();
		while (pos_ < input_.size()
			&& !std::isalnum(input_[pos_-1])) right();
		while (pos_ < input_.size()
			&&  std::isalnum(input_[pos_]))   right();
	}
	/// Move cursor to the beginning of the line
	void home() {
		while (pos_) left();
	}
	/// Move cursor to the end of the line
	void end() {
		while (pos_ < input_.size()) right();
	}
	/// Previous history input
	void up() {
		if (auto t = history_.prev(get())) set(*t);
	}
	/// Next history input
	void down() {
		if (auto t = history_.next()) set(*t);
	}
	/// Go to first input in history
	void ctrl_up() {
		if (auto t = history_.first()) set(*t);
	}
	/// Go beyond the end of history into a new input
	void ctrl_down() {
		if (auto t = history_.past_end()) set(*t);
	}
	/// Store the string (mostly the current input) in history
	diagnostics::result<void> store(const std::string& s) {
		return history_.store(s);
	}
	size_t printed_size(const std::string& s) const {
		size_t size = 0;
		bool in_escape = false;
		// count characters skip escapes
		for (auto c : s) if (in_escape) {
				if (c == 'm') in_escape = false;
			} else if (c == '\033') in_escape = true;
			else size++;
		return size;
	}
	void reset_input() {
		TDBG(std::cerr << " <RESET INPUT>";)
		input_.clear(), pos_ = 0, print_input();
		if (is_pipe_) term::out(prompt_.c_str(), prompt_.size());
	}
	void update_widths() {
		if (is_pipe_) return;
		auto [h, w] = term::get_termsize();
		term_h_ = h, term_w_ = w, r_ = c_ = 0;
		size_t ps = printed_size(prompt_);
		if (ps > 0 && term_w_) r_ = (ps - 1) / term_w_, c_ = ps % term_w_;
		else r_ = 0, c_ = 0;
		lws_.clear(), lws_.reserve(r_);
		if (r_ > 1) for (size_t i = 0; i != r_ - 1; ++i)
				lws_.push_back(term_w_ - 1);
		lws_.push_back(c_);
		size_t x = 0;
		TDBG(std::cerr << " <PROMPT SIZE: " << ps
			<< ", R: " << r_ << ", C: " << c_ << ">";)
		while (x <= input_.size()) {
			TDBG(std::cerr << " <POS: " << pos_
				<< ", x: " << x << ">";)
			if (x == input_.size()) break;
			if (input_[x++] == '\n') new_line();
			else {
				if (c_ + 1 >= term_w_) new_line();
				else lws_[r_] = ++c_;
			}
		}
		TDBG(print_debug();)
	}
	void clear_input() {
		if (is_pipe_) return;
		TDBG(std::cerr << " <CLEAR INPUT";)
		if (r_ < lws_.size() - 1)
			term::cursor_down(lws_.size() - 1 - r_);
		r_ = lws_.size() - 1;
		while (r_) r_--, term::clear_line(), term::cursor_up();
		c_ = 0, term::clear_line();
		TDBG(std::cerr << ">";)
	}
	void print_input() { /// refresh input line
		if (is_pipe_) return;
		TDBG(std::cerr << " <REFRESH INPUT ";)
		auto [h, w] = term::get_termsize();
		term_h_ = h, term_w_ = w;
		size_t ps = printed_size(prompt_);
		size_t l = input_.size() + ps;
		TDBG(std::cerr << "POS: " << pos_ << ", DIM[" << w << "," << h
			<< "]" << ", SIZE: " << l << "(" << ps << ")" << "\n`"
			<< get() << "`\n";)
		TDBG(std::cerr << " <PRINTING " << printed_size(prompt_)
			<< " + " << input_.size() << ">";)
		term::out(prompt_.c_str(), prompt_.size());
		term::out(input_.data(), input_.size());
		if (term_w_ && l % term_w_ == 0) term::out("\n");
		update_widths();
// #ifdef TERM_DEBUG
// 		std::stringstream ss;
// 		ss << "[";
// 		ss << "hpos: " << hpos_ << ", ";
// 		ss << "history: " << history_.size() << ", ";
// 		ss << "pos: " << pos_ << ", ";
// 		ss << "size: " << input_.size() << ", ";
// 		ss << "end: " << (pos_ >= input_.size() ? "yes" : "no");
// 		ss << "] ";
// 		std::cerr << ss.str() << "\n";
// #endif // TERM_DEBUG
		TDBG(std::cerr << " <INPUT REFRESHED>\n\n----------\n\n";)
	}
	evaluator_t& re_;
	std::string prompt_;
	std::string continuation_prompt_;
	repl_history history_;
	std::vector<char> input_;
	size_t pos_ = 0; /// cursor position in input
	size_t term_h_ = 0, term_w_ = 0; /// term height and width
	size_t r_ = 0, c_ = 0; /// current cursor's row and column
	std::vector<size_t> lws_{ 0 }; /// input line widths
	bool is_pipe_ = !term::is_tty(stdin);
	bool input_started_ = false;
};

} // idni namespace
#endif // __IDNI__PARSER__UTILITY__REPL_H__