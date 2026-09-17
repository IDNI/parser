// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "utility/diagnostics.h"
#include "utility/repl_ftxui.h"

using namespace std::chrono_literals;

struct slow_evaluator {
	idni::repl_ftxui<slow_evaluator>* r_ftx = nullptr;

	void reprompt() { r_ftx->set_prompt("test> "); }

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

int main() {
	slow_evaluator evaluator;
	idni::repl_ftxui repl(evaluator, "test> ", "");
	return repl.run();
}
