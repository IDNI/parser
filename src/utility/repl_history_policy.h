// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__REPL_HISTORY_POLICY_H__
#define __IDNI__PARSER__UTILITY__REPL_HISTORY_POLICY_H__

#include <functional>
#include <string>
#include <string_view>

namespace idni {

/// Where a REPL front end keeps its history and which lines it keeps.
struct repl_history_policy {
	/// Path of the history file. Empty keeps the history in memory only.
	std::string file{};
	/// Called before each store. False drops the line from the memory
	/// history and from the file. Empty keeps every line.
	std::function<bool(std::string_view line)> keep{};
};

} // namespace idni

#endif // __IDNI__PARSER__UTILITY__REPL_HISTORY_POLICY_H__
