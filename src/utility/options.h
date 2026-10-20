// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__OPTIONS_H__
#define __IDNI__PARSER__UTILITY__OPTIONS_H__

#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#ifdef DEBUG
#	include <thread>
#endif

#include "diagnostics.h"
#include "options_types.h"

namespace idni {

/// Runs after each write of a bound option.
using option_hook = std::function<void()>;

/// Converts between a field and the value of its option. from_value
/// reports a bad value, for example a name that the grammar does not have.
template <typename C, typename T>
concept option_codec = requires(const C& c, const T& field,
	const option_value& v)
{
	{ c.to_value(field) } -> std::convertible_to<option_value>;
	{ c.from_value(v) } -> std::same_as<diagnostics::result<T>>;
};

namespace detail {
struct options_catalog;

/// One bound option: its field, seen through the codec.
struct option_binding {
	std::function<option_value()> read;
	/// Converts @p v for the field. It assigns the field only when
	/// @p assign is true, so a caller can check a value first.
	std::function<diagnostics::result<void>(const option_value& v,
		bool assign)> write;
	option_hook after;
};
}

/// What one command line gave that is no option.
struct parsed_args {
	std::string command;
	/// True when argv named the command.
	bool command_given = false;
	/// The positional arguments: `-`, the files and the URLs.
	std::vector<std::string> inputs;
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
	/// True takes each positional argument before the command as it is,
	/// also one that names no file and looks like no URL.
	bool free_inputs = false;
};

/// One value per option. A later write replaces the value.
struct options_repository {
	options_repository();
	options_repository(const options_repository&) = delete;
	options_repository& operator=(const options_repository&) = delete;
	/// Runs every detach callback, so no bound owner keeps a pointer here.
	~options_repository();

	/// A name that is new or declared with an equal spec is fine. The
	/// surfaces are not compared, and the declared ones stay. A different
	/// spec under a declared name fails, and nothing is declared.
	[[nodiscard]] diagnostics::result<void> declare(const option_set& set);
	/// Add a surface to a declared option. An unknown name is an error.
	/// A short letter already used on that command is an error.
	/// An equal surface that the option has already changes nothing.
	[[nodiscard]] diagnostics::result<void> add_surface(
		std::string_view name, option_surface surface);

	/// Typed access by name. A wrong name or type is a programming error,
	/// caught by a DBG assert only.
	template <typename T> T get(std::string_view name) const;
	/// The value of @p name, of the option's own kind. A wrong name is a
	/// programming error, caught by a DBG assert only.
	option_value value(std::string_view name) const;
	template <typename T> void set(std::string_view name, T value);
	/// Store a value of the option's own kind. An unknown name, a value of
	/// another kind and a codec failure are errors, and nothing changes.
	[[nodiscard]] diagnostics::result<void> set(std::string_view name,
		option_value value);

	/// Bind a declared option to its field. The field type is the type
	/// of the option. The hook runs after each write. A stored value goes
	/// into the field. The owner unbinds before the field ends.
	template <typename T>
	[[nodiscard]] diagnostics::result<void> bind(std::string_view name,
		T& field, option_hook after = {});
	/// Bind with a codec, when the field type is not the option type.
	template <typename T, typename Codec>
		requires option_codec<Codec, T>
	[[nodiscard]] diagnostics::result<void> bind(std::string_view name,
		T& field, Codec codec, option_hook after = {});
	/// The owner calls it in its destructor. The last value of the field
	/// stays in the repository.
	void unbind(std::string_view name);
	/// Register @p detach of a bound @p owner. The destructor runs it.
	void add_detach(const void* owner, std::function<void()> detach);
	/// Forget the detach callback of @p owner.
	void remove_detach(const void* owner);
	/// Give every option of @p owner its default value: the fallback of
	/// the spec. A bound field gets it written, an unbound slot is cleared.
	void reset(std::string_view owner);
	/// Give the option @p name its default, as reset() does for each option
	/// of an owner. An unknown name is an error.
	[[nodiscard]] diagnostics::result<void> reset_option(
		std::string_view name);

	/// Text access for a REPL, an api or a binding.
	[[nodiscard]] diagnostics::result<void> set_text(std::string_view name,
		std::string_view text);
	[[nodiscard]] diagnostics::result<std::string> get_text(
		std::string_view name) const;
	std::vector<std::string> names() const;
	/// The declared option @p name, or null when no option has the name.
	/// The spec stays valid after a later declare().
	std::shared_ptr<const option_spec> find(std::string_view name) const;

	/// The variable is the prefix as given plus the name in upper case,
	/// with '-' as '_', so a caller passes a prefix such as `TGF_`. A bad
	/// value is an error.
	[[nodiscard]] diagnostics::result<void> load_env(std::string_view prefix);

	/// Store the values of argv. Nothing is stored on an error.
	/// A program reads get<bool>("help") after it and calls help() itself.
	[[nodiscard]] diagnostics::result<parsed_args> parse_args(int argc,
		char** argv, const args_config& cfg = {});
	/// The help of `command`, or of the program when it is empty. It uses
	/// the config of the last parse_args() call.
	void help(std::ostream& os, std::string_view command = {}) const;

private:
	/// The field of a bound option, else the stored value, else the
	/// default of @p command, else the default of the option.
	option_value effective(std::size_t i, std::string_view command) const;
	option_value effective(std::size_t i) const;
	/// Write @p v into the field and run the hook, or store it when the
	/// option is not bound.
	[[nodiscard]] diagnostics::result<void> store(std::size_t i,
		option_value v);
	[[nodiscard]] diagnostics::result<void> bind_at(std::string_view name,
		detail::option_binding b);
	/// Write the fallback of the spec into a bound field, or clear the
	/// stored value of an unbound option.
	void reset_at(std::size_t i);
#ifdef DEBUG
	void assert_owner_thread() const;
#endif

	// find() shares it, so declare() replaces it and never edits it.
	std::shared_ptr<const detail::options_catalog> cat_;
	std::vector<std::optional<option_value>> values_;
	std::vector<std::optional<detail::option_binding>> bindings_;
	// an owner can outlive the repository, as a static one does a thread local
	std::vector<std::pair<const void*, std::function<void()>>> detach_;
	/// The command of the last parse_args(), which selects the surface
	/// defaults.
	std::string command_;
	args_config cfg_;
#ifdef DEBUG
	// A repository and the fields it binds belong to one thread; a scope
	// on another thread takes it over until the scope ends.
	std::thread::id thread_ = std::this_thread::get_id();
	friend struct options_scope;
#endif
};

/// The current repository of the calling thread. The first call on a
/// thread creates one that lives as long as the thread.
options_repository& options();

/// Makes a repository current for a scope. Restores the previous one.
/// A scope opened on another thread hands the repository to that thread:
/// the thread that owned it must not touch it until the scope ends.
struct options_scope {
	explicit options_scope(options_repository& r);
	~options_scope();
	options_scope(const options_scope&) = delete;
	options_scope& operator=(const options_scope&) = delete;

private:
	options_repository* previous_;
#ifdef DEBUG
	options_repository* current_;
	std::thread::id owner_;
#endif
};

} // namespace idni

#include "options.tmpl.h"
#endif // __IDNI__PARSER__UTILITY__OPTIONS_H__
