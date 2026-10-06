// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__OPTIONS_H__
#define __IDNI__PARSER__UTILITY__OPTIONS_H__

#include <cstddef>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "diagnostics.h"

namespace idni {

using option_list  = std::vector<std::string>;
/// The index of the alternative is the kind of the option.
using option_value = std::variant<bool, std::size_t, std::string, option_list>;

/// Where an option shows on the command line: the command, a short letter,
/// and an optional default of that command. The default has the kind of the
/// option.
struct option_surface {
	std::string command;
	char short_name = '\0';
	std::optional<option_value> fallback = std::nullopt;
};

/// One option. The type of `fallback` gives the kind.
struct option_spec {
	std::string name;
	std::string owner;
	option_value fallback;
	std::string help;
	std::vector<option_surface> surfaces;
	bool from_env = true;
};

struct command_spec {
	std::string name;
	std::string help;
};

/// Everything one program declares.
struct option_set {
	std::vector<option_spec> options = {};
	std::vector<command_spec> commands = {};
};

namespace detail {
struct options_snapshot;
struct repository_state;
}

/// A consistent read of all values: the shared snapshot and the values of
/// the calling thread, taken at one moment.
struct options_view {
	template <typename T> T get(std::string_view name) const;

private:
	friend struct options_repository;
	std::shared_ptr<const detail::options_snapshot> snap_;
	std::unordered_map<std::size_t, option_value> local_;
};

/// What one command line gave.
struct parsed_args {
	std::string command;
	std::vector<std::string> files;
	bool help = false;
};

/// The program data that parse_args() and help() need.
struct args_config {
	std::string name;
	std::string help_header;
	std::string description;
	std::string default_command;
	std::string default_command_when_files;
	/// The surface of the options given before the command.
	std::string global_surface;
};

struct options_repository {
	options_repository();
	~options_repository();
	options_repository(const options_repository&) = delete;
	options_repository& operator=(const options_repository&) = delete;

	/// Register a source and return its id. The id is the precedence: a
	/// higher id wins.
	std::size_t source(std::string_view name);

	/// Every name of the set is new, or the call fails and declares nothing.
	[[nodiscard]] diagnostics::result<void> declare(const option_set& set);

	/// Typed access by name. A wrong name or type is a programming error,
	/// caught by a DBG assert only.
	template <typename T> T get(std::string_view name) const;
	template <typename T> void set(std::string_view name, T value,
		std::size_t source);
	void clear(std::string_view name, std::size_t source);
	bool is_set(std::string_view name, std::size_t source) const;

	/// The source of the calling thread. It is above every other source,
	/// and no other thread sees its values.
	template <typename T> void set_local(std::string_view name, T value);
	void clear_local(std::string_view name);

	/// One consistent read for one operation, for example one solve.
	options_view view() const;

	/// Text access for a REPL, an api or a binding.
	[[nodiscard]] diagnostics::result<void> set_text(std::string_view name,
		std::string_view text, std::size_t source);
	[[nodiscard]] diagnostics::result<std::string> get_text(
		std::string_view name) const;
	std::vector<std::string> names() const;

	/// The prefix and the name in upper case, with '-' as '_'. A bad value
	/// is a warning.
	[[nodiscard]] diagnostics::result<void> load_env(std::string_view prefix,
		std::size_t source);

	/// Store the values of argv at `source`. Nothing is stored on an error.
	[[nodiscard]] diagnostics::result<parsed_args> parse_args(int argc,
		char** argv, std::size_t source, const args_config& cfg = {});
	/// The help of `command`, or of the program when it is empty. It uses
	/// the config of the last parse_args() call.
	void help(std::ostream& os, std::string_view command = {}) const;

	/// A number that grows with each write that changes a value: the
	/// shared writes and the set_local() writes of the calling thread.
	std::size_t version() const;

private:
	// The values and the thread records, out of the public header.
	std::unique_ptr<detail::repository_state> state_;
};

} // namespace idni

#include "options.tmpl.h"
#endif // __IDNI__PARSER__UTILITY__OPTIONS_H__
