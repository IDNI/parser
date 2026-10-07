// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__OPTIONS_TYPES_H__
#define __IDNI__PARSER__UTILITY__OPTIONS_TYPES_H__

#include <cctype>
#include <charconv>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <variant>
#include <vector>

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
	std::vector<option_surface> surfaces;
	bool from_env = true;
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

namespace detail {

/// The kind of an option is the index of its alternative in option_value.
enum class option_kind : std::size_t { flag, numeric, text, list };

inline option_kind kind_of(const option_value& v) {
	return static_cast<option_kind>(v.index());
}

inline std::string option_trim(std::string_view s) {
	std::size_t b = 0, e = s.size();
	while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
	while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
	return std::string(s.substr(b, e - b));
}

inline std::vector<std::string> option_split(std::string_view s, char sep) {
	std::vector<std::string> out;
	for (std::size_t start = 0;;) {
		std::size_t pos = s.find(sep, start);
		out.push_back(option_trim(s.substr(start, pos - start)));
		if (pos == std::string_view::npos) return out;
		start = pos + 1;
	}
}

/// The value of @p text, or nothing when the kind does not take it.
inline std::optional<option_value> parse_value(option_kind kind,
	std::string_view text)
{
	switch (kind) {
	case option_kind::flag: {
		std::string w(text);
		for (auto& c : w) c = static_cast<char>(
			std::tolower(static_cast<unsigned char>(c)));
		if (w == "true" || w == "on" || w == "1" || w == "yes"
			|| w == "y" || w == "t") return option_value(true);
		if (w == "false" || w == "off" || w == "0" || w == "no"
			|| w == "n" || w == "f") return option_value(false);
		return std::nullopt;
	}
	case option_kind::numeric: {
		std::size_t v = 0;
		auto r = std::from_chars(text.data(), text.data() + text.size(), v);
		if (text.empty() || r.ec != std::errc{}
			|| r.ptr != text.data() + text.size()) return std::nullopt;
		return option_value(v);
	}
	case option_kind::text: return option_value(std::string(text));
	case option_kind::list: {
		option_list out;
		for (auto& part : option_split(text, ','))
			if (!part.empty()) out.push_back(std::move(part));
		return option_value(std::move(out));
	}
	}
	return std::nullopt;
}

inline std::string format_value(const option_value& v) {
	switch (kind_of(v)) {
	case option_kind::flag: return std::get<bool>(v) ? "true" : "false";
	case option_kind::numeric:
		return std::to_string(std::get<std::size_t>(v));
	case option_kind::text: return std::get<std::string>(v);
	case option_kind::list: {
		std::string out;
		for (const auto& s : std::get<option_list>(v)) {
			if (!out.empty()) out += ", ";
			out += s;
		}
		return out;
	}
	}
	return {};
}

} // namespace detail

} // namespace idni

#endif // __IDNI__PARSER__UTILITY__OPTIONS_TYPES_H__
