// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__OPTIONS_TYPES_H__
#define __IDNI__PARSER__UTILITY__OPTIONS_TYPES_H__

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "diagnostics.h"

// The plain option types and their text form, without the repository, so a
// grammar reader can name options without the repository code.
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
	/// The default applies only to an option that no field binds.
	std::optional<option_value> fallback = std::nullopt;
	bool operator==(const option_surface&) const = default;
};

/// One option. The type of `fallback` gives the kind.
struct option_spec {
	std::string name;
	std::string owner;
	option_value fallback;
	std::string help;
	std::vector<option_surface> surfaces = {};
	bool from_env = true;
	/// The text as given: no quotes and no trim.
	bool raw = false;
	bool operator==(const option_spec&) const = default;
};

struct command_spec {
	std::string name;
	std::string help;
	bool operator==(const command_spec&) const = default;
};

/// Everything one program declares.
struct option_set {
	std::vector<option_spec> options = {};
	std::vector<command_spec> commands = {};
};

/// The kind of an option is the index of its alternative in option_value.
enum class option_kind : std::size_t { flag, numeric, text, list };

option_kind kind_of(const option_value& v);

/// The kind of the fallback of @p s.
option_kind kind_of(const option_spec& s);

namespace detail {

std::string option_trim(std::string_view s);

/// The position after the closing quote of the quoted string that starts
/// at @p s[i], or npos when it has none. A `\` skips the next character.
std::size_t option_quote_end(std::string_view s, std::size_t i);

/// The value of @p quoted, a TGF string with its quotes.
std::optional<std::string> option_unquote(std::string_view quoted,
	diagnostics::report& rep);

/// The entries of the list @p s, split at the commas outside a quoted
/// entry. A bare entry is trimmed and dropped when it is empty.
std::optional<option_list> option_list_entries(std::string_view s,
	diagnostics::report& rep);

/// The value of @p text without the spaces around it. A text or a list
/// entry may be a TGF string in double quotes. @p name labels an error.
diagnostics::result<option_value> parse_value(option_kind kind,
	std::string_view text, std::string_view name = {});

/// True when @p v prints bare and reads back as itself.
bool option_reads_bare(std::string_view v, bool in_list);

std::string option_quote(std::string_view v, bool in_list);

/// The text that parse_value() reads back as @p v. A text or a list entry
/// prints in double quotes when its bare form reads back differently.
std::string format_value(const option_value& v);

} // namespace detail

} // namespace idni

#include "options_types.tmpl.h"

#endif // __IDNI__PARSER__UTILITY__OPTIONS_TYPES_H__
