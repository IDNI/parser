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
//         The method should return non-zero if the repl should exit.
// - a member repl<evaluator_t>* r = 0; accessible to this struct
//         if r is private, add friend struct repl<evaluator_t>;
template <typename evaluator_t>
struct repl {
	repl(evaluator_t& re, std::string prompt = "> ",
		std::string history_file = ".history")
		: re_(re), prompt_(prompt), history_file_(history_file)
	{
		// load history from file if exists
		if (std::filesystem::exists(history_file_)) {
			std::ifstream hf(history_file_);
			std::string s;
			while (std::getline(hf, s))
				if (s.size()) history_.push_back(s);
			hpos_ = history_.size();
		}
		re.r = this; // link evaluator to this repl
		term::open();
	}
	~repl() { term::close(); }
	int run() {
		refresh_input();
		while (1) {
			std::string s;
			char ch;
			while (term::in(ch) == 1) {
				//DBG(std::cerr << "pressed '" << ch << "' = " << (int)ch << "\n";)
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
					//	<< (int)c << "\n";
					//#endif // DEBUG
				}
				else if (ch == 10 || ch == 13) { // enter
					if (input_.size() == 0) continue;
					s = store();
					term::out("\n", 1);
					break; // exit input loop to evaluate
				} else  // not a control ch. add to input
					input_.insert(
						input_.begin() + pos_++, ch),
					refresh_input();
			}
			if (s.empty()) continue;
			if (re_.eval(s)) break; // exit loop if nonzero
			clear_input();
		}
		return 0;
	}
	// sets the prompt
	void prompt(const std::string& p) { prompt_ = p, refresh_input(); }
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
		input_.assign(s.begin(), s.end()), pos_ = input_.size();
	}
	// clears the input line (set to empty)
	void set() { input_.clear(), pos_ = 0; }
	void clear_input() { // clears the input line
		input_.clear(), pos_ = 0, refresh_input();
	}
	void backspace() { // delete character before the cursor
		if (pos_) input_.erase(input_.begin() + --pos_),refresh_input();
	}
	void del() { // delete character after the cursor
		if (pos_ < input_.size())
			input_.erase(input_.begin() + pos_), refresh_input();
	}
	void left() { // move cursor left
		if (pos_) pos_--, refresh_input();
	}
	void right() { // move cursor right
		if (pos_ < input_.size()) pos_++, refresh_input();
	}
	void ctrl_left() { // move cursor word left
		if (pos_ == 0) return;
		pos_--;
		while (pos_ > 0 && !std::isalnum(input_[pos_]))     pos_--;
		while (pos_ > 0 &&  std::isalnum(input_[pos_ - 1])) pos_--;
		refresh_input();
	}
	void ctrl_right() { // move cursor word right
		if (pos_ == input_.size()) return;
		pos_++;
		while (pos_ < input_.size()
			&& !std::isalnum(input_[pos_-1])) pos_++;
		while (pos_ < input_.size()
			&&  std::isalnum(input_[pos_]))   pos_++;
		refresh_input();
	}
	void home() { // move cursor to the beginning of the line
		if (pos_) pos_ = 0, refresh_input();
	}
	void end() { // move cursor to the end of the line
		if (pos_ < input_.size()) pos_ = input_.size(), refresh_input();
	}
	void up() { // previous history input
		if (history_.size() == 0 || hpos_ == 0) return;
		// push current input into history if we are at the end
		if (hpos_ == history_.size() && input_.size())
			history_.push_back(get());
		set(history_[--hpos_]), refresh_input();
	}
	void down() { // next history input
		if (hpos_ == history_.size()) return;
		if (++hpos_ == history_.size()) set();
		else set(history_[hpos_]);
		refresh_input();
	}
	void ctrl_up() { // go to first input in history
		if (history_.size() == 0 || hpos_ == 0) return;
		set(history_[hpos_ = 0]), refresh_input();
	}
	void ctrl_down() { // go beyond the end of history into a new input
		if (hpos_ == history_.size()) return;
		hpos_ = history_.size(), set(), refresh_input();
	}
	// store current input into history and return the input as a string
	const std::string& store() {
		auto s = get();
		if (history_.size() && history_.back() == s)
			return hpos_ = history_.size(), history_.back();
		history_.push_back(s), hpos_ = history_.size();
		std::ofstream file(history_file_, std::ios::app);
		if (file) file << history_[hpos_ - 1] << '\n';
		return history_[hpos_ - 1];
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
	size_t get_number_of_lines() const {
		auto [_, cols] = term::get_termsize();
		if (cols == 0) return 1;
		size_t prompt_size = printed_size(prompt_);
		size_t l = prompt_size + input_.size();
		if (l > 0) l--;
		return l / cols + 1;
	}
	void refresh_input() const { // refresh input line
		static size_t last_n = 1;
		size_t n = get_number_of_lines();
		size_t down = last_n > n ? last_n - n : 0;
		if (n >= last_n) last_n = n;
		else
			//std::cerr << "n " << n << " < last_n " << last_n << "\n",
			std::swap(n, last_n);
// #ifdef DEBUG
// 		auto [rows, cols] = term::get_termsize();
// 		size_t prompt_size = printed_size(prompt_);
// 		size_t l = input_.size() + prompt_size;
// 		if (l > 0) l--;
// 		std::cerr << pos_;
// 		std::cerr << "[" << cols << "," << rows << "]";
// 		std::cerr << prompt_size << "+";
// 		std::cerr << input_.size() << "=";
// 		std::cerr << l << "/" << n << "%" << last_n << "\n";
// #endif // DEBUG
		for (size_t i = 0; i < n-1; i++)
			term::clear_line(), term::cursor_up();
		term::clear_line();
		if (down) for (size_t i = 0; i < down; i++)
			term::cursor_down();
// #ifdef DEBUG
// 		std::stringstream ss;
// 		ss << "[";
// 		ss << "hpos: " << hpos_ << ", ";
// 		ss << "history: " << history_.size() << ", ";
// 		ss << "pos: " << pos_ << ", ";
// 		ss << "size: " << input_.size() << ", ";
// 		ss << "end: " << (pos_ >= input_.size() ? "yes" : "no");
// 		ss << "] ";
// 		std::cerr << ss.str() << "\n";
// #endif // DEBUG
		term::out(prompt_.c_str(), prompt_.size()); // print prompt
		// print input
		if (input_.size()) term::out(input_.data(), input_.size());
		if (pos_ < input_.size())
			term::cursor_right(input_.size() - pos_);
	}
	evaluator_t& re_;
	std::string prompt_;
	std::string history_file_;
	std::vector<char> input_;
	std::vector<std::string> history_;
	size_t pos_ = 0;
	size_t hpos_ = 0;
};

} // idni namespace
#endif // __IDNI_REPL_H__