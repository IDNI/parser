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
#include <utility>
#include <vector>

#include "diagnostics.h"
#include "options_types.h"

namespace idni {

namespace detail {
struct options_catalog;
}

/// What one command line gave.
struct parsed_args {
	std::string command;
	/// The positional arguments: `-`, the files and the URLs.
	std::vector<std::string> inputs;
	bool help = false;
	/// The options argv gave, with their values, in argv order. A program
	/// writes them again after it loads a grammar.
	std::vector<std::pair<std::string, option_value>> values;
};

/// The program data that parse_args() and help() need.
struct args_config {
	std::string name;
	std::string help_header;
	std::string description;
	std::string default_command;
	/// The command when the arguments name inputs: files or URLs.
	std::string default_command_when_inputs;
	/// The surface of the options given before the command.
	std::string global_surface;
};

/// One value per option. A later write replaces the value.
struct options_repository {
	options_repository();
	options_repository(const options_repository&) = delete;
	options_repository& operator=(const options_repository&) = delete;

	/// Every name of the set is new, or the call fails and declares nothing.
	[[nodiscard]] diagnostics::result<void> declare(const option_set& set);

	/// Typed access by name. A wrong name or type is a programming error,
	/// caught by a DBG assert only.
	template <typename T> T get(std::string_view name) const;
	template <typename T> void set(std::string_view name, T value);

	/// Text access for a REPL, an api or a binding.
	[[nodiscard]] diagnostics::result<void> set_text(std::string_view name,
		std::string_view text);
	[[nodiscard]] diagnostics::result<std::string> get_text(
		std::string_view name) const;
	std::vector<std::string> names() const;
	/// The declared option @p name, or null when no option has the name.
	/// The spec stays valid after a later declare().
	std::shared_ptr<const option_spec> find(std::string_view name) const;

	/// The prefix and the name in upper case, with '-' as '_'. A bad value
	/// is a warning.
	[[nodiscard]] diagnostics::result<void> load_env(std::string_view prefix);

	/// Store the values of argv. Nothing is stored on an error.
	[[nodiscard]] diagnostics::result<parsed_args> parse_args(int argc,
		char** argv, const args_config& cfg = {});
	/// The help of `command`, or of the program when it is empty. It uses
	/// the config of the last parse_args() call.
	void help(std::ostream& os, std::string_view command = {}) const;

private:
	/// The stored value, else the default of @p command, else the default
	/// of the option.
	const option_value& effective(std::size_t i, std::string_view command)
		const;
	const option_value& effective(std::size_t i) const;

	// find() shares it, so declare() replaces it and never edits it.
	std::shared_ptr<const detail::options_catalog> cat_;
	std::vector<std::optional<option_value>> values_;
	/// The command of the last parse_args(), which selects the surface
	/// defaults.
	std::string command_;
	args_config cfg_;
};

} // namespace idni

#include "options.tmpl.h"
#endif // __IDNI__PARSER__UTILITY__OPTIONS_H__
