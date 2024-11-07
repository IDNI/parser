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
#include <iostream>
#include <sstream>
#ifndef _WIN32
#	include <unistd.h>
#	include <stdio.h>
#	include <termios.h>
#	include <sys/ioctl.h>
#else
#	include <map>
#	include <deque>
#	include <vector>
#	include <windows.h>
#endif

#include "term.h"

namespace idni::term {

bool opened = false;

#ifndef _WIN32
struct termios orig_attrs, raw_attrs;
#else
DWORD orig_in_mode = 0,  raw_in_mode = 0;
DWORD orig_out_mode = 0, raw_out_mode = 0;
HANDLE hIn = INVALID_HANDLE_VALUE;
HANDLE hOut = INVALID_HANDLE_VALUE;
bool init() {
	if (hIn == INVALID_HANDLE_VALUE)  hIn  = GetStdHandle(STD_INPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE) hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hIn == INVALID_HANDLE_VALUE || hOut == INVALID_HANDLE_VALUE) {
		std::cerr << "Error getting standard handles\n";
		return false;
	}
	return true;
}
#endif

bool open() {
	if (opened)    return true;
	if (!is_tty()) return false;
#ifndef _WIN32
	tcgetattr(STDIN_FILENO, &orig_attrs);
	raw_attrs = orig_attrs;
	raw_attrs.c_lflag &= ~(ECHO | ICANON | ISIG);
	raw_attrs.c_iflag &= ~(IXON);
	raw_attrs.c_cc[VMIN] = 0;
	raw_attrs.c_cc[VTIME] = 1;
	tcsetattr(STDIN_FILENO, TCSANOW, &raw_attrs);
#else
	// Get original console input mode
	if (!GetConsoleMode(hIn, &orig_in_mode)) {
		std::cerr << "Error getting console input mode\n";
		return false;
	}
	// Disable echo input, line input, and enable window/mouse input
	raw_in_mode = orig_in_mode;
	raw_in_mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
	raw_in_mode |= ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;

	if (!SetConsoleMode(hIn, raw_in_mode)) {
		std::cerr << "Error setting console input mode\n";
		return false;
	}
	// Get original console output mode
	if (!GetConsoleMode(hOut, &orig_out_mode)) {
		std::cerr << "Error getting console output mode\n";
		return false;
	}
	// Enable virtual terminal processing to support ANSI escape codes
	raw_out_mode = orig_out_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, raw_out_mode)) {
		std::cerr << "Error setting console output mode\n";
		return false;
	}
#endif

	return opened = true;
}

void close() {
	if (!opened) return;

#ifndef _WIN32
	tcsetattr(STDIN_FILENO, TCSANOW, &orig_attrs);
#else
	SetConsoleMode(hIn,  orig_in_mode);
	SetConsoleMode(hOut, orig_out_mode);
#endif

	opened = false;
}

void enable_getline_mode() {
	if (!opened || !open()) return;
#ifndef _WIN32
	termios attrs;
	tcgetattr(STDIN_FILENO, &attrs);
	attrs.c_lflag |= ICANON;
	attrs.c_lflag |= ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &attrs);
#else
	DWORD in_mode;
	GetConsoleMode(hIn, &in_mode);
	in_mode |= ENABLE_LINE_INPUT;
	in_mode |= ENABLE_ECHO_INPUT;
	SetConsoleMode(hIn, in_mode);
#endif
}

void disable_getline_mode() {
	if (!opened) return;
#ifndef _WIN32
	tcsetattr(STDIN_FILENO, TCSANOW, &raw_attrs);
#else
	SetConsoleMode(hIn, raw_in_mode);
#endif
}

void clear() { [[maybe_unused]] int result = std::system(
#ifndef _WIN32
	"clear"
#else
	"cls"
#endif
); }

#ifdef _WIN32
std::deque<char> input_buffer;
std::vector<char> translate_virtual_key(WORD vk, DWORD controlState) {
	static std::map<WORD, std::vector<char>> vk_map = {
		{ VK_HOME,       { 1 } },
		{ VK_END,        { 5 } },
		{ VK_BACK,       { 8 } },
		{ VK_RETURN,     { 10 } },
		{ VK_DELETE,     { 27, 91, 51, 126 } },
		{ VK_UP,         { 27, 91, 65 } },
		{ VK_DOWN,       { 27, 91, 66 } },
		{ VK_RIGHT,      { 27, 91, 67 } },
		{ VK_LEFT,       { 27, 91, 68 } }
	};
	static std::map<WORD, std::vector<char>> ctrl_vk_map = {
		{ VK_UP,         { 27, 91, 49, 59, 53, 65 } },
		{ VK_DOWN,       { 27, 91, 49, 59, 53, 66 } },
		{ VK_RIGHT,      { 27, 91, 49, 59, 53, 67 } },
		{ VK_LEFT,       { 27, 91, 49, 59, 53, 68 } }
	};
	if ((controlState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0) {
		if (auto it = ctrl_vk_map.find(vk); it != ctrl_vk_map.end())
			return it->second;
	} else if (auto it = vk_map.find(vk); it != vk_map.end())
		return it->second;
	return {};
}
#endif

int in(char& c) {
#ifndef _WIN32
	return read(STDIN_FILENO, &c, 1);
#else
	if (!input_buffer.empty()) {
		c = input_buffer.front();
		input_buffer.pop_front();
		return 1;
	}
	if (!init()) return 0;
	DWORD read;
	if (is_tty()) while (true) {
		INPUT_RECORD record;
		if (!ReadConsoleInput(hIn, &record, 1, &read) || read == 0)
			return 0;
		if (record.EventType == KEY_EVENT
			&& record.Event.KeyEvent.bKeyDown)
		{
			c = record.Event.KeyEvent.uChar.AsciiChar;
			if (c) return 1;
			// Special key without an ASCII representation
			std::vector<char> seq = translate_virtual_key(
					record.Event.KeyEvent.wVirtualKeyCode,
					record.Event.KeyEvent.dwControlKeyState);
			if (!seq.empty()) {
				input_buffer.insert(input_buffer.end(),
					seq.begin(), seq.end());
				c = input_buffer.front();
				input_buffer.pop_front();
				return 1;
			}
		}
	} else { // pipe or file input
		char buffer[1024];
		if (!ReadFile(hIn, buffer, sizeof(buffer) - 1, &read, NULL)
			|| read == 0) return 0;
		input_buffer.insert(input_buffer.end(), buffer, buffer + read);
		c = input_buffer.front();
		input_buffer.pop_front();
		return 1;
	}
#endif
}

int in(char* s, size_t l) {
#ifndef _WIN32
	return read(STDIN_FILENO, &s, l);
#else
	size_t count = 0;
	while (count < l) {
		char c;
		if (in(c) != 1) break;
		s[count++] = c;
	}
	return static_cast<int>(count);
#endif
}

void out(const char* data, size_t size) {
	// TODO (HIGH) handle write errors
#ifndef _WIN32
	size_t written = write(STDOUT_FILENO, data, size);
	if (written != size)
#else
	DWORD written;
	if (!init()) return;
	if (!WriteFile(hOut, data, static_cast<DWORD>(size), &written, NULL)
		|| written != size)
#endif
		std::cerr << "write error\n";
}

void out(const std::string& str) {
	out(str.c_str(), str.size());
}

void clear_line() { out("\r\033[K", 4); }

void cursor_up(int n) {
	std::stringstream ss;
	bool down = n < 0; n = abs(n);
	ss << "\033[" << (n ? n : 1) << (down ? "B" : "A");
	out(ss.str().c_str(), ss.str().size());
}

void cursor_down(int n) { cursor_up(-n); }

void cursor_right(int n) {
	std::stringstream ss;
	bool left = n < 0; n = abs(n);
	ss << "\033[" << (n ? n : 1) << (left ? "C" : "D");
	out(ss.str().c_str(), ss.str().size());
}

void cursor_left(int n) { cursor_right(-n); }

std::pair<unsigned short, unsigned short> get_termsize() {
#ifndef _WIN32
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	return { w.ws_row, w.ws_col };
#else
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!init()) return { 0, 0 };
	if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return { 0, 0 };
	unsigned short rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	unsigned short cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	return { rows, cols };
#endif
}

bool is_tty() {
#ifndef _WIN32
	return isatty(STDIN_FILENO);
#else
	if (!init()) return false;
	return GetFileType(hIn) == FILE_TYPE_CHAR;
#endif
}

} // namespace idni::term
