// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "utility/diagnostics.h"
#include "utility/repl_ftxui.h"

using namespace std::chrono_literals;

struct slow_evaluator {
	idni::repl_ftxui<slow_evaluator>* r_ftx = nullptr;

	void reprompt() {
		r_ftx->set_prompt("test> ");
		r_ftx->set_continuation_prompt("...> ");
	}

	idni::diagnostics::result<int> eval(const std::string& text) {
		idni::diagnostics::result<int> result;
		if (text == "slow") {
			std::cout << "complete line\n";
			std::this_thread::sleep_for(400ms);
			std::cout << "partial output";
			std::this_thread::sleep_for(200ms);
			result = 0;
		} else if (text == "incomplete") result = 2;
		else if (text == "quit") {
			std::cout << "Quit.\n";
			result = 1;
		} else {
			std::cout << "evaluated: " << text << "\n";
			result = 0;
		}
		return result;
	}
};

// Prints every byte of stdin as two hex digits, so a test sees what a pipe
// delivers, a byte order mark included.
int print_stdin_hex() {
#ifdef _WIN32
	// text mode would turn CR LF into LF and stop at a Ctrl-Z byte
	_setmode(_fileno(stdin), _O_BINARY);
#endif
	const char* digits = "0123456789abcdef";
	std::string out;
	for (int c; (c = std::getchar()) != EOF; ) {
		if (!out.empty()) out += ' ';
		out += digits[(c >> 4) & 0xf];
		out += digits[c & 0xf];
	}
	std::cout << out << "\n";
	return 0;
}

int main(int argc, char** argv) {
	if (argc > 1 && std::string(argv[1]) == "--hex")
		return print_stdin_hex();
	slow_evaluator evaluator;
	idni::repl_ftxui repl(evaluator, "test> ", "");
	return repl.run();
}
