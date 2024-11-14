// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.
#ifndef __IDNI_REPL_H__
#define __IDNI_REPL_H__

#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <climits>
#include <unistd.h>
#include <stdio.h>

#include "term.h"

// TODO (MEDIUM) support multiline input (containing '\n') - escape new lines?
// TODO (MEDIUM) windows support (+ test on mac since it should work on POSIX)
// TODO (LOW) implement clear directly (not by calling system(clear/cls) and
//            make evaluators call it instead of clearing the screen themselves

namespace idni {

// evaluator_t must be a class with:
// - a method int eval(const std::string&)
//         which allows to plug own evaluation logic.
//         The method should return 0 for normal operation
//         1 if the repl should exit and 2 if the input was incomplete
// - a member repl<evaluator_t>* r = 0; accessible to this struct
//         if r is private, add friend struct repl<evaluator_t>;
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
	int run() {
		clear_input();
		print_input();
		bool is_pipe = !term::is_tty();
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
					term::out(std::string{ ch });
					// add to input
					if (ch == 10 || ch == 13) { // enter
						TDBG(std::cerr << " <ENTER>";)
						if (input_.empty()) continue;
						s = get();
						new_line();
						TDBG(std::cerr << "\n";)
						TDBG(print_debug();)
						break; // exit input loop to evaluate
					} else {
						TDBG(std::cerr << " <CH: "
								<< ch << ">";)
						if (c_ >= term_w_) new_line();
						else lws_[r_] = ++c_;
						input_.insert(input_.begin()
								+ pos_++, ch);
					}
				}
				TDBG(std::cerr << "\n";) TDBG(print_debug();)
			}
			if (!s.empty()) {
				auto ret = re_.eval(s);
				TDBG(std::cerr << " <EVAL: " << ret << ">";)
				if (ret == 2) { // Unexpected end of file, continue
					input_.insert(input_.begin()
								+ pos_++, '\n');
					continue;
				}
				store(s);
				if (ret == 1) break; // exit loop if 1
				reset_input();
			}
			if (read == 0 && is_pipe) break;
		}
		return 0;
	}
	// sets the prompt
	void prompt(const std::string& p) {
		TDBG(std::cerr << " <PROMPT: `" << p << "`>";)
		update_widths(), clear_input();
		prompt_ = p, print_input();
		TDBG(std::cerr << " <PROMPT PRINTED>";)
	}
	void set_prompt(const std::string& p) { prompt_ = p; }
	void clear() { term::clear(); }
	// returns the current prompt
	std::string prompt() const { return prompt_; }
private:
	// returns the current input as a string
	std::string get() const {
		std::stringstream ss;
		return ss.write(input_.data(), input_.size()), ss.str();
	}
	// sets the current input from a string
	void set(const std::string& s) {
		clear_input();
		input_.assign(s.begin(), s.end()), pos_ = input_.size();
		print_input();
	}
	// clears the input line (set to empty)
	void set() { clear_input(), input_.clear(), pos_ = 0, print_input(); }
#ifdef TERM_DEBUG
	void print_debug() {
		std::cerr << " <POS: " << pos_
			<< " LWS:";
		for (size_t i = 0; i != lws_.size(); ++i)
			std::cerr << " " << lws_[i];
		std::cerr << " >";
	}
#endif // TERM_DEBUG	
	void prev_line() {
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
		term::cursor_left(c_);
		term::cursor_down();
		new_line();
	}
	void new_line() {
		lws_[r_++] = c_, c_ = 0;
		while (lws_.size() <= r_) lws_.push_back(0);
	}
	void go(size_t r, size_t c) {
		int up   = static_cast<int>(r_) - static_cast<int>(r);
		int left = static_cast<int>(c_) - static_cast<int>(c);
		TDBG(std::cerr << " <GO: [" << r << ", " << c << "] -> "
			<< "relative [up:" << up << ", left: " << left << "]";)
		term::cursor_up(r_ - r);
		term::cursor_left(c_ - c);
		r_ -= up, c_ -= left;
		TDBG(std::cerr << " >";)
	}
	void backspace() { // delete character before the cursor
		if (pos_ == 0) return;
		size_t r = r_, c = c_;
		clear_input();
		input_.erase(input_.begin() + --pos_);
		print_input();
		if (c) c--;
		else if (r) c = lws_[--r];
		go(r, c);
	}
	void del() { // delete character after the cursor
		if (pos_ >= input_.size()) return;
		size_t r = r_, c = c_;
		clear_input();
		input_.erase(input_.begin() + pos_);
		print_input();
		go(r, c);
	}
	void left() { // move cursor left
		if (pos_) {
			pos_--;
			if (c_) c_--, term::cursor_left();
			else if (r_) prev_line();
		}
	}
	void right() { // move cursor right
		if (pos_ < input_.size()) {
			pos_++;
			if (c_ >= lws_[r_]) next_line();
			else c_++, term::cursor_right();
		}
	}
	void ctrl_left() { // move cursor word left
		if (pos_ == 0) return;
		left();
		while (pos_ > 0 && !std::isalnum(input_[pos_]))     left();
		while (pos_ > 0 &&  std::isalnum(input_[pos_ - 1])) left();
	}
	void ctrl_right() { // move cursor word right
		if (pos_ == input_.size()) return;
		right();
		while (pos_ < input_.size()
			&& !std::isalnum(input_[pos_-1])) right();
		while (pos_ < input_.size()
			&&  std::isalnum(input_[pos_]))   right();
	}
	void home() { // move cursor to the beginning of the line
		while (pos_) left();
	}
	void end() { // move cursor to the end of the line
		while (pos_ < input_.size()) right();
	}
	void up() { // previous history input
		if (history_.size() == 0 || hpos_ == 0) return;
		// push current input into history if we are at the end
		if (hpos_ == history_.size() && input_.size())
			history_.push_back(get());
		set(history_[--hpos_]);
	}
	void down() { // next history input
		if (hpos_ == history_.size()) return;
		if (++hpos_ == history_.size()) set();
		else set(history_[hpos_]);
	}
	void ctrl_up() { // go to first input in history
		if (history_.size() == 0 || hpos_ == 0) return;
		set(history_[hpos_ = 0]);
	}
	void ctrl_down() { // go beyond the end of history into a new input
		if (hpos_ == history_.size()) return;
		hpos_ = history_.size(), set();
	}
	// store a string into history and return the string
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
	// store current input into history and return the input as a string
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
	}
	void update_widths() {
		auto [h, w] = term::get_termsize();
		term_h_ = h, term_w_ = w, r_ = c_ = 0;
		size_t ps = printed_size(prompt_);
		if (ps > 0) r_ = (ps - 1) / term_w_, c_ = ps % term_w_;
		lws_.reserve(r_);
		if (r_ > 1) for (size_t i = 0; i != r_ - 1; ++i)
				lws_.push_back(term_w_);
		size_t x = 0;
		TDBG(std::cerr << " <PROMPT SIZE: " << ps
			<< ", R: " << r_ << ", C: " << c_ << ">";)
		lws_.clear(), lws_.push_back(c_);
		while (x <= input_.size()) {
			TDBG(std::cerr << " <POS: " << pos_
				<< ", x: " << x << ">";)
			if (x == input_.size()) break;
			if (input_[x++] == '\n') new_line();
			else {
				if (c_ >= term_w_) new_line();
				else c_++, lws_[r_] = c_;
			}
		}
	}
	void clear_input() {
		TDBG(std::cerr << " <CLEAR INPUT";)
		if (r_ < lws_.size() - 1) term::cursor_down(lws_.size() - 1 - r_);
		r_ = lws_.size() - 1;
		while (r_) r_--, term::clear_line(), term::cursor_up();
		c_ = 0, term::clear_line();
		TDBG(std::cerr << ">";)
	}
	void print_input() { // refresh input line
		TDBG(std::cerr << " <REFRESH INPUT ";)
		auto [h, w] = term::get_termsize();
		term_h_ = h, term_w_ = w;
#ifdef TERM_DEBUG
		size_t ps = printed_size(prompt_);
		size_t l = input_.size() + ps;
		std::cerr << "POS: " << pos_ << ", DIM[" << w << "," << h << "]"
			<< ", SIZE: " << l << "(" << ps << ")" << "\n`"
			<< get() << "`\n";
#endif // TERM_DEBUG
		// last_n_ = n;
		TDBG(std::cerr << " <PRINTING " << prompt_.size() << " + "
			<< input_.size() << ">";)
		term::out(prompt_.c_str(), prompt_.size());
		term::out(input_.data(), input_.size());
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
	size_t pos_ = 0; // cursor position in input
	size_t hpos_ = 0; // history position
	size_t term_h_ = 0, term_w_ = 0; // term height and width
	size_t r_ = 0, c_ = 0; // current cursor's row and column
	std::vector<size_t> lws_{ 0 }; // input line widths
};

} // idni namespace
#endif // __IDNI_REPL_H__