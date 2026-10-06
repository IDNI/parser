// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__OPTIONS_H__
#define __IDNI__PARSER__UTILITY__OPTIONS_H__

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "diagnostics.h"

namespace idni {

using option_list  = std::vector<std::string>;
/// The index of the alternative is the kind of the option.
using option_value = std::variant<bool, std::size_t, std::string, option_list>;

/// One option. The type of `fallback` gives the kind.
struct option_spec {
	std::string name;
	std::string owner;
	option_value fallback;
	std::string help;
	bool from_env = true;
};

/// Everything one program declares.
struct option_set {
	std::vector<option_spec> options = {};
};

namespace detail { struct options_snapshot; }

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

	/// A number that grows with each write that changes a value.
	std::size_t version() const;

private:
	const detail::options_snapshot& current() const;
	template <typename F> void modify(F&& f);

	std::unique_ptr<detail::options_snapshot> snap_;
	std::size_t version_ = 0;
	std::vector<std::string> sources_;
};

} // namespace idni

#include "options.tmpl.h"
#endif // __IDNI__PARSER__UTILITY__OPTIONS_H__
