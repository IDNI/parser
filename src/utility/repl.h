// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.txt

#ifndef __IDNI__PARSER__UTILITY__REPL_H__
#define __IDNI__PARSER__UTILITY__REPL_H__

#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <climits>
#include <unistd.h>
#include <stdio.h>

#include "term.h"

namespace idni {

/// evaluator_t must be a class with:
/// - a method int eval(const std::string&)
///         which allows to plug own evaluation logic.
///         The method should return 0 for normal operation
///         1 if the repl should exit and 2 if the input was incomplete
/// - a member repl<evaluator_t>* r = 0; accessible to this struct
///         if r is private, add friend struct repl<evaluator_t>;
template <typename evaluator_t>
struct repl {
	repl(evaluator_t& re, std::string prompt = "> ",
		std::string history_file = ".history")
		: re_(re), prompt_(prompt), history_file_(history_file)
	{
		auto unescape = [](const std::string& s) {
			std::ostringstream oss;
			for (size_t i = 0; i < s.length(); ++i) {
				if (s[i] == '\\' && i + 1 < s.length()) {
					char next = s[++i];
					oss << (next == 'n' ? '\n'
						: next == '\\' ? '\\' : next);
				} else oss << s[i];
			}
			return oss.str();
		};
		// load history from file if exists
		if (std::filesystem::exists(history_file_)) {
			std::ifstream hf(history_file_);
			std::string s;
			while (std::getline(hf, s))
				if (s.size()) history_.push_back(unescape(s));
			hpos_ = history_.size();
		}
		re.r = this; // link evaluator to this repl
		term::open();
	}
	~repl() { term::close(); }
	bool echo = true;
	int run() {
		clear_input();
		reset_input();
		while (1) {
			std::string s;
			char ch;
			int read;
			while ((read = term::in(ch)) == 1) {
				TDBG(std::cerr << "\n";) TDBG(print_debug();)
				TDBG(std::cerr << " <pressed '" << ch << "' = "
						<< static_cast<int>(ch) << ">";)
				// control characters
				if (ch == 3 || ch == 4) return 0; // ctrl-c or d
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
					auto ret = evaluate(s);
					if (ret == 1) break;
					if (ret == 2) continue;
				//}
			}
			if (read == 0 && is_pipe_) break;
		}
		return 0;
	}
	void split_line() {
		if (is_pipe_) return;
		size_t r = r_;
		clear_input();
		input_.insert(input_.begin() + pos_ - 1, '\n');
		print_input(), go(r, 0);
	}
	int evaluate(const std::string& s) {
		auto ret = re_.eval(s);
		TDBG(std::cerr << " <EVAL: " << ret << ">";)
		if (ret == 2) { // Unexpected end of file, continue
			input_.insert(input_.begin() + pos_ - 1, '\n');
			return ret;
		}
		store(s);
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
		input_.insert(input_.begin() + pos_ - 1, ch);
		if (is_pipe_) return;
		print_input();
		go(r, c);
		TDBG(print_debug();)
		if (c_ >= lws_[r_]) next_line();
		else c_++, term::cursor_right();
		TDBG(print_debug();)
	}
	void set_prompt(const std::string& p) { prompt_ = p; }
	void clear() { if (!is_pipe_) term::clear(); }
	/// Returns the current prompt
	std::string prompt() const { return prompt_; }
private:
	/// Returns the current input as a string
	std::string get() const {
		std::stringstream ss;
		return ss.write(input_.data(), input_.size()), ss.str();
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
		int up   = static_cast<int>(r_) - static_cast<int>(r);
		int left = static_cast<int>(c_) - static_cast<int>(c);
		TDBG(std::cerr << " <GO: [" << r << ", " << c << "] -> "
			<< "relative [up:" << up << ", left: " << left << "]";)
		term::cursor_up(r_ - r);
		term::cursor_left(c_ - c);
		r_ -= up, c_ -= left;
		TDBG(std::cerr << " >";)
	}
	/// Delete character before the cursor
	void backspace() {
		if (pos_ == 0) return;
		size_t r = r_, c = c_;
		if (c) c--;
		else if (r) c = lws_[--r];
		clear_input();
		input_.erase(input_.begin() + --pos_);
		print_input();
		go(r, c);
	}
	/// Delete character after the cursor
	void del() {
		if (pos_ >= input_.size()) return;
		size_t r = r_, c = c_;
		clear_input();
		input_.erase(input_.begin() + pos_);
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
		if (history_.size() == 0 || hpos_ == 0) return;
		// push current input into history if we are at the end
		if (hpos_ == history_.size() && input_.size())
			history_.push_back(get());
		set(history_[--hpos_]);
	}
	/// Next history input
	void down() {
		if (hpos_ == history_.size()) return;
		if (++hpos_ == history_.size()) set();
		else set(history_[hpos_]);
	}
	/// Go to first input in history
	void ctrl_up() {
		if (history_.size() == 0 || hpos_ == 0) return;
		set(history_[hpos_ = 0]);
	}
	/// Go beyond the end of history into a new input
	void ctrl_down() {
		if (hpos_ == history_.size()) return;
		hpos_ = history_.size(), set();
	}
	/// Store a string into history and return the string
	const std::string& store(const std::string& s) {
		auto escape = [](const std::string& s) {
			std::ostringstream oss;
			for (char c : s) oss << (c == '\\' ? "\\\\"
						: c == '\n' ? "\\n"
						: std::string(1, c));
			return oss.str();
		};
		if (history_.size() && history_.back() == s)
			return hpos_ = history_.size(), history_.back();
		history_.push_back(s), hpos_ = history_.size();
		std::ofstream file(history_file_, std::ios::app);
		if (file) file << escape(history_[hpos_ - 1]) << '\n';
		return history_[hpos_ - 1];
	}
	/// Store current input into history and return the input as a string
	const std::string& store() { return store(get()); }
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
		if (r_ < lws_.size() - 1) term::cursor_down(lws_.size() - 1 - r_);
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
	std::string history_file_;
	std::vector<char> input_;
	std::vector<std::string> history_;
	size_t pos_ = 0; /// cursor position in input
	size_t hpos_ = 0; /// history position
	size_t term_h_ = 0, term_w_ = 0; /// term height and width
	size_t r_ = 0, c_ = 0; /// current cursor's row and column
	std::vector<size_t> lws_{ 0 }; /// input line widths
	bool is_pipe_ = !term::is_tty();
};

} // idni namespace
#endif // __IDNI__PARSER__UTILITY__REPL_H__