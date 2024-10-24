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
#include <sys/ioctl.h>
#include <stdio.h>
#include <termios.h>

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
		if (!isatty(STDIN_FILENO) // if not tty (stdin)
			|| tcgetattr(STDIN_FILENO, &orig_attrs_) == -1) return;
		// save terminal state and set terminal to raw mode
		raw_attrs_ = orig_attrs_;
		raw_attrs_.c_lflag &= ~(ECHO | ICANON | ISIG);
		raw_attrs_.c_iflag &= ~(IXON);
		raw_attrs_.c_cc[VMIN] = 0;
		raw_attrs_.c_cc[VTIME] = 1;
		if (tcsetattr(STDIN_FILENO, TCSANOW, &raw_attrs_) == -1) return;
		reset_terminal = true;
		re.r = this;
		// load history from file if exists
		if (!std::filesystem::exists(history_file_)) return;
		std::ifstream hf(history_file_);
		std::string s;
		while (std::getline(hf, s)) if (s.size()) history_.push_back(s);
		hpos_ = history_.size();
	}
	~repl() {
		// return terminal to its original state
		if (reset_terminal)
			tcsetattr(STDIN_FILENO, TCSANOW, &orig_attrs_);
	}
	int run() {
		refresh_input();
		while (1) {
			std::string s;
			char ch;
			while (in(ch) == 1) {
				//DBG(std::cerr << "pressed '" << ch << "' = " << (int)ch << "\n";)
				// control characters
				if (ch == 3 || ch == 4) return 0; // ctrl-c or d
				if      (ch == 1) home();
				else if (ch == 5) end();
				else if (ch == 8 || ch == 127) backspace();
				else if (ch == 27) { // escape seq
					char c;
					if (in(c) != 1 || c != 91 || in(c) != 1)
						continue;
					if (c == 49) { // ctrl
						if (in(c) != 1 || c != 59 ||
							in(c) != 1 || c != 53 ||
							in(c) != 1) continue;
						if      (c == 65) ctrl_up();
						else if (c == 66) ctrl_down();
						else if (c == 67) ctrl_right();
						else if (c == 68) ctrl_left();
					}
					else if (c == 51) {
						if (in(c)==1 && c==126) del();
					}
					else if (c == 65) up();
					else if (c == 66) down();
					else if (c == 67) right();
					else if (c == 68) left();
					else if (c == 70) end();
					else if (c == 72) home();
					//#ifdef DEBUG
					//else std::cerr << "unknown escape seq: "
					//	<< (int)c << "\n";
					//#endif // DEBUG
				}
				else if (ch == 10) { // enter
					if (input_.size() == 0) continue;
					s = store();
					out("\n", 1);
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
	void clear() { [[maybe_unused]] int result = std::system(
#ifdef _WIN32
		"cls"
#else
		"clear"
#endif
		);
	}
	// returns the current prompt
	std::string prompt() const { return prompt_; }
private:
	// reads a character from stdin
	int in(char& c) const { return read(STDIN_FILENO, &c, 1); }
	// writes data to stdout
	void out(const char* data, size_t size) const {
		size_t written = write(STDOUT_FILENO, data, size);
		// TODO (HIGH) handle write errors
		if (written != size) std::cerr << "write error\n";
	}
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
		while (pos_<input_.size() && !std::isalnum(input_[pos_-1]))pos_++;
		while (pos_<input_.size() &&  std::isalnum(input_[pos_]))pos_++;
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
	std::pair<unsigned short, unsigned short> get_termsize() const {
		if (!isatty(STDOUT_FILENO)) return { 1, USHRT_MAX };
		struct winsize w;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
		return { w.ws_row, w.ws_col };
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
		auto [_, cols] = get_termsize();
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
// 		auto [rows, cols] = get_termsize();
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
			//std::cerr << "clear line and move up\n",
			out("\r\033[K\033[A", 8); // clear line and move up
		out("\r\033[K", 4); // clear input
		if (down) {
			for (size_t i = 0; i < down; i++)
				out("\033[B", 3); // move down
		}
//#ifdef DEBUG
//		std::stringstream ss;
//		ss << "[";
//		ss << "hpos: " << hpos_ << ", ";
//		ss << "history: " << history_.size() << ", ";
//		ss << "pos: " << pos_ << ", ";
//		ss << "size: " << input_.size() << ", ";
//		ss << "end: " << (pos_ >= input_.size() ? "yes" : "no");
//		ss << "] ";
//		out(ss.str().c_str(), ss.str().size());
//#endif // DEBUG
		out(prompt_.c_str(), prompt_.size()); // print prompt
		// print input
		if (input_.size()) out(input_.data(), input_.size());
		if (pos_ < input_.size()) { // move cursor to its current pos
			std::stringstream ss;
			ss << "\033[" << input_.size() - pos_ << "D";
			out(ss.str().c_str(), ss.str().size());
		}
	}
	evaluator_t& re_;
	std::string prompt_;
	std::string history_file_;
	std::vector<char> input_;
	std::vector<std::string> history_;
	size_t pos_ = 0;
	size_t hpos_ = 0;
	struct termios orig_attrs_, raw_attrs_;
	bool reset_terminal = false;
};

} // idni namespace
#endif // __IDNI_REPL_H__