// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__OPTIONS_TMPL_H__
#define __IDNI__PARSER__UTILITY__OPTIONS_TMPL_H__

#include <algorithm>
#include <cassert>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <system_error>
#include <type_traits>
#include <utility>

#include "../parser_strings.h"

namespace idni {

namespace detail {

template <typename T>
constexpr bool is_option_type_v = std::is_same_v<T, bool>
	|| std::is_same_v<T, std::size_t> || std::is_same_v<T, std::string>
	|| std::is_same_v<T, option_list>;

// set() takes a literal or an int too, so a caller needs no cast.
template <typename T, typename D = std::decay_t<T>>
using option_store_t = std::conditional_t<std::is_same_v<D, bool>, bool,
	std::conditional_t<std::is_integral_v<D>, std::size_t,
	std::conditional_t<std::is_convertible_v<const D&, std::string_view>,
		std::string, D>>>;

template <typename T>
option_value to_option_value(T&& v) {
	using D = std::decay_t<T>;
	using S = option_store_t<T>;
	static_assert(is_option_type_v<S>,
		"an option value is bool, size_t, string or option_list");
	if constexpr (std::is_integral_v<D> && !std::is_same_v<D, bool>) {
		if constexpr (std::is_signed_v<D>) { DBG(assert(v >= 0);) }
		return option_value(static_cast<std::size_t>(v));
	} else return option_value(S(std::forward<T>(v)));
}

/// The text form of @p v, which parse_value() reads back.
inline diagnostics::result<std::string> format_option_value(
	const option_value& v)
{
	diagnostics::result<std::string> r;
	return r.with_value(format_value(v));
}

inline std::string option_env_name(std::string_view prefix,
	std::string_view name)
{
	std::string var(prefix);
	for (char c : name)
		var += c == '-' ? '_' : static_cast<char>(std::toupper(
			static_cast<unsigned char>(c)));
	return var;
}

// A numeric argument such as -5 is a value, not a short option.
inline bool starts_with_int(std::string_view s) {
	std::size_t i = 0;
	if (i < s.size() && (s[i] == '-' || s[i] == '+')) ++i;
	return i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]));
}

/// `<scheme>://<rest>`, or `<host>:<port>` with a numeric port.
inline bool is_url(std::string_view s) {
	auto alnum = [](char c) {
		return std::isalnum(static_cast<unsigned char>(c)) != 0;
	};
	if (auto sep = s.find("://"); sep != std::string_view::npos) {
		if (sep == 0 || sep + 3 == s.size()
			|| !std::isalpha(static_cast<unsigned char>(s[0])))
				return false;
		for (char c : s.substr(0, sep))
			if (!alnum(c) && c != '+' && c != '-' && c != '.')
				return false;
		return true;
	}
	const auto colon = s.rfind(':');
	if (colon == std::string_view::npos || colon == 0
		|| colon + 1 == s.size()) return false;
	for (char c : s.substr(0, colon))
		if (!alnum(c) && c != '.' && c != '-' && c != '_') return false;
	for (char c : s.substr(colon + 1))
		if (!std::isdigit(static_cast<unsigned char>(c))) return false;
	return true;
}

inline bool is_option_prefix(std::string_view s) {
	return s.size() >= 2 && s[0] == '-' && !starts_with_int(s);
}

/// Strip the dashes of @p opt. False when it is no option.
inline bool option_prefix(std::string& opt, bool& is_short) {
	is_short = false;
	if (!is_option_prefix(opt)) return false;
	if (opt[1] != '-') is_short = true, opt = opt.substr(1);
	else opt = opt.substr(2);
	return true;
}

/// The declarations. A declare() replaces the catalog as a whole.
struct options_catalog {
	std::vector<option_spec> specs;
	std::map<std::string, std::size_t, std::less<>> index;
	std::vector<command_spec> commands;

	std::optional<std::size_t> find(std::string_view name) const {
		auto it = index.find(name);
		if (it == index.end()) return std::nullopt;
		return it->second;
	}
	const command_spec* command(std::string_view name) const {
		for (const auto& c : commands) if (c.name == name) return &c;
		return nullptr;
	}
};

// The surfaces are left out: a tool adds them after a library declared
// the option, and the library may declare the option again.
inline bool same_spec_without_surfaces(const option_spec& a,
	const option_spec& b)
{
	return a.name == b.name && a.owner == b.owner
		&& a.fallback == b.fallback && a.help == b.help
		&& a.from_env == b.from_env;
}

/// Report in @p r when @p sf does not fit the option @p o of @p cat: a
/// default of another kind, or a short letter that another option uses on
/// the same command.
inline void check_surface(const options_catalog& cat, const option_spec& o,
	const option_surface& sf, diagnostics::result<void>& r)
{
	namespace ps = parser_strings;
	using diagnostics::code;
	if (sf.fallback && sf.fallback->index() != o.fallback.index())
		r.error(code::type_error, ps::messages::option_command_default_kind,
			{ { ps::label::name, o.name },
			  { ps::label::value, sf.command } });
	if (!sf.short_name) return;
	for (const auto& other : cat.specs)
		for (const auto& have : other.surfaces)
			if (have.command == sf.command
				&& have.short_name == sf.short_name)
			{
				r.error(code::invalid_argument,
					ps::messages::option_short_name_taken,
					{ { ps::label::name, other.name },
					  { ps::label::name, o.name },
					  { ps::label::value,
						std::string(1, sf.short_name) } });
				return;
			}
}

template <typename T>
T typed(const option_value& v) {
	if (const T* p = std::get_if<T>(&v)) return *p;
	DBG(assert(false && "an option is read with another type");)
	return T{};
}

} // namespace detail

inline options_repository::options_repository()
	: cat_(std::make_shared<const detail::options_catalog>())
{}

inline option_value options_repository::effective(std::size_t i,
	std::string_view command) const
{
	if (bindings_[i]) return bindings_[i]->read();
	if (values_[i]) return *values_[i];
	const option_spec& o = cat_->specs[i];
	if (!command.empty())
		for (const auto& sf : o.surfaces)
			if (sf.command == command && sf.fallback)
				return *sf.fallback;
	return o.fallback;
}

inline option_value options_repository::effective(std::size_t i) const {
	return effective(i, command_);
}

#ifdef DEBUG
inline void options_repository::assert_owner_thread() const {
	assert(std::this_thread::get_id() == thread_
		&& "an option is written on another thread");
}
#endif

inline diagnostics::result<void> options_repository::store(std::size_t i,
	option_value v)
{
	DBG(assert_owner_thread();)
	diagnostics::result<void> r;
	if (!bindings_[i]) {
		values_[i] = std::move(v);
		return r;
	}
	if (r.merge_ok(bindings_[i]->write(v, true)) && bindings_[i]->after)
		bindings_[i]->after();
	return r;
}

inline diagnostics::result<void> options_repository::declare(
	const option_set& set)
{
	namespace ps = parser_strings;
	using diagnostics::code;
	diagnostics::result<void> r;
	auto cat = std::make_shared<detail::options_catalog>(*cat_);
	for (const auto& c : set.commands) {
		if (const command_spec* have = cat->command(c.name)) {
			if (*have == c) continue;
			r.error(code::invalid_argument,
				ps::messages::command_already_exists,
				{ { ps::label::name, c.name } });
			continue;
		}
		cat->commands.push_back(c);
	}
	for (const auto& o : set.options) {
		if (const auto have = cat->find(o.name)) {
			if (detail::same_spec_without_surfaces(
				cat->specs[*have], o)) continue;
			r.error(code::invalid_argument,
				ps::messages::option_already_exists,
				{ { ps::label::name, o.name } });
			continue;
		}
		for (const auto& sf : o.surfaces)
			detail::check_surface(*cat, o, sf, r);
		cat->index.emplace(o.name, cat->specs.size());
		cat->specs.push_back(o);
	}
	if (r.has_error()) return r;
	values_.resize(cat->specs.size());
	bindings_.resize(cat->specs.size());
	cat_ = std::move(cat);
	return r;
}

inline diagnostics::result<void> options_repository::add_surface(
	std::string_view name, option_surface surface)
{
	namespace ps = parser_strings;
	diagnostics::result<void> r;
	const auto i = cat_->find(name);
	if (!i) return r.with_error(diagnostics::code::not_found,
		ps::messages::option_not_found, { { ps::label::name, name } });
	auto cat = std::make_shared<detail::options_catalog>(*cat_);
	detail::check_surface(*cat, cat->specs[*i], surface, r);
	if (r.has_error()) return r;
	cat->specs[*i].surfaces.push_back(std::move(surface));
	cat_ = std::move(cat);
	return r;
}

template <typename T>
T options_repository::get(std::string_view name) const {
	static_assert(detail::is_option_type_v<T>,
		"an option value is bool, size_t, string or option_list");
	const auto i = cat_->find(name);
	if (!i) {
		DBG(assert(false && "an unknown option is read");)
		return T{};
	}
	return detail::typed<T>(effective(*i));
}

inline option_value options_repository::value(std::string_view name) const
{
	const auto i = cat_->find(name);
	if (!i) {
		DBG(assert(false && "an unknown option is read");)
		return option_value{};
	}
	return effective(*i);
}

template <typename T>
void options_repository::set(std::string_view name, T value) {
	option_value v = detail::to_option_value(std::move(value));
	const auto i = cat_->find(name);
	if (!i) {
		DBG(assert(false && "an unknown option is set");)
		return;
	}
	if (cat_->specs[*i].fallback.index() != v.index()) {
		DBG(assert(false && "an option is set with another type");)
		return;
	}
	// a typed write comes from the program, so a codec error is a bug
	auto r = store(*i, std::move(v));
	DBG(assert(!r.has_error() && "a typed write does not fit the field");)
}

inline diagnostics::result<void> options_repository::set(
	std::string_view name, option_value value)
{
	namespace ps = parser_strings;
	diagnostics::result<void> r;
	const auto i = cat_->find(name);
	if (!i) return r.with_error(diagnostics::code::not_found,
		ps::messages::option_not_found, { { ps::label::name, name } });
	if (kind_of(cat_->specs[*i]) != kind_of(value))
		return r.with_error(diagnostics::code::type_error,
			ps::messages::option_value_kind,
			{ { ps::label::name, name } });
	r.merge(store(*i, std::move(value)));
	return r;
}

template <typename T>
diagnostics::result<void> options_repository::bind(std::string_view name,
	T& field, option_hook after)
{
	static_assert(detail::is_option_type_v<T>,
		"a field without a codec has the type of its option");
	T* f = &field;
	return bind_at(name, detail::option_binding{
		[f] { return option_value(*f); },
		[f](const option_value& v, bool assign) {
			diagnostics::result<void> r;
			const T* x = std::get_if<T>(&v);
			DBG(assert(x
				&& "a bound option is written with another type");)
			if (x && assign) *f = *x;
			return r;
		},
		std::move(after) });
}

template <typename T, typename Codec>
	requires option_codec<Codec, T>
diagnostics::result<void> options_repository::bind(std::string_view name,
	T& field, Codec codec, option_hook after)
{
	T* f = &field;
	return bind_at(name, detail::option_binding{
		[f, codec] { return option_value(codec.to_value(*f)); },
		[f, codec](const option_value& v, bool assign) {
			diagnostics::result<void> r;
			auto x = r.merge_take(codec.from_value(v));
			if (x && assign) *f = std::move(*x);
			return r;
		},
		std::move(after) });
}

inline diagnostics::result<void> options_repository::bind_at(
	std::string_view name, detail::option_binding b)
{
	DBG(assert_owner_thread();)
	diagnostics::result<void> r;
	const auto i = cat_->find(name);
	if (!i) {
		DBG(assert(false && "an unknown option is bound");)
		return r;
	}
	if (b.read().index() != cat_->specs[*i].fallback.index()) {
		DBG(assert(false
			&& "an option is bound to a field of another type");)
		return r;
	}
	DBG(assert(!bindings_[*i] && "an option is bound twice");)
	bindings_[*i] = std::move(b);
	if (auto v = std::exchange(values_[*i], std::nullopt))
		r.merge(store(*i, std::move(*v)));
	return r;
}

inline void options_repository::unbind(std::string_view name) {
	DBG(assert_owner_thread();)
	const auto i = cat_->find(name);
	if (!i || !bindings_[*i]) {
		DBG(assert(false && "an option is unbound that is not bound");)
		return;
	}
	values_[*i] = bindings_[*i]->read();
	bindings_[*i].reset();
}

inline void options_repository::reset(std::string_view owner) {
	for (std::size_t i = 0; i < cat_->specs.size(); ++i) {
		const option_spec& o = cat_->specs[i];
		if (o.owner != owner) continue;
		if (!bindings_[i]) {
			DBG(assert_owner_thread();)
			values_[i].reset();
			continue;
		}
		auto r = store(i, o.fallback);
		DBG(assert(!r.has_error()
			&& "the default of an option does not fit its field");)
	}
}

inline diagnostics::result<void> options_repository::set_text(
	std::string_view name, std::string_view text)
{
	namespace ps = parser_strings;
	diagnostics::result<void> r;
	const auto i = cat_->find(name);
	if (!i) return r.with_error(diagnostics::code::not_found,
		ps::messages::option_not_found, { { ps::label::name, name } });
	auto v = r.merge_take(detail::parse_value(
		kind_of(cat_->specs[*i]), text, name));
	if (!v) return r;
	r.merge(store(*i, std::move(*v)));
	return r;
}

inline diagnostics::result<std::string> options_repository::get_text(
	std::string_view name) const
{
	namespace ps = parser_strings;
	diagnostics::result<std::string> r;
	const auto i = cat_->find(name);
	if (!i) return r.with_error(diagnostics::code::not_found,
		ps::messages::option_not_found, { { ps::label::name, name } });
	auto text = r.merge_take(detail::format_option_value(effective(*i)));
	if (!text) return r;
	return r.with_value(std::move(*text));
}

inline std::vector<std::string> options_repository::names() const {
	std::vector<std::string> out;
	for (const auto& o : cat_->specs) out.push_back(o.name);
	return out;
}

inline std::shared_ptr<const option_spec> options_repository::find(
	std::string_view name) const
{
	const auto i = cat_->find(name);
	if (!i) return nullptr;
	// shares the catalog, which a later declare() replaces
	return std::shared_ptr<const option_spec>(cat_, &cat_->specs[*i]);
}

inline diagnostics::result<void> options_repository::load_env(
	std::string_view prefix)
{
	namespace ps = parser_strings;
	diagnostics::result<void> r;
	for (std::size_t i = 0; i < cat_->specs.size(); ++i) {
		const option_spec& o = cat_->specs[i];
		if (!o.from_env) continue;
		const std::string var = detail::option_env_name(prefix, o.name);
		const char* text = std::getenv(var.c_str());
		if (!text) continue;
		auto v = detail::parse_value(kind_of(o), text, o.name);
		if (!v.has_value()) {
			r.error(diagnostics::code::invalid_argument,
				ps::messages::option_env_bad_value,
				{ { ps::label::name, var },
				  { ps::label::value, text } });
			r.merge(std::move(v));
			continue;
		}
		r.merge(store(i, std::move(v.value())));
	}
	return r;
}

inline diagnostics::result<parsed_args> options_repository::parse_args(
	int argc, char** argv, const args_config& cfg)
{
	namespace ps = parser_strings;
	using diagnostics::code;
	using diagnostics::attr_in;
	cfg_ = cfg;
	diagnostics::result<parsed_args> r;
	const detail::options_catalog& cat = *cat_;
	std::vector<std::string> args;
	for (int i = 0; i < argc; ++i) args.emplace_back(argv[i]);
	parsed_args out;
	// Stored only when the whole command line is valid. One entry per
	// option, in the order of its first use in argv.
	std::vector<std::pair<std::size_t, option_value>> pending;
	auto pending_of = [&](std::size_t i) -> option_value* {
		for (auto& [j, v] : pending) if (j == i) return &v;
		return nullptr;
	};
	auto put = [&](std::size_t i, option_value v) {
		if (option_value* have = pending_of(i)) *have = std::move(v);
		else pending.emplace_back(i, std::move(v));
	};

	auto fail_with_help = [&](std::string_view msg,
		std::initializer_list<attr_in> attrs)
	{
		r.error(code::invalid_argument, msg, attrs);
		std::ostringstream os;
		help(os);
		r.info(os.str());
	};
	auto find_long = [&](std::string_view surface, std::string_view name)
		-> std::optional<std::size_t>
	{
		const auto i = cat.find(name);
		if (!i) return std::nullopt;
		for (const auto& sf : cat.specs[*i].surfaces)
			if (sf.command == surface) return i;
		return std::nullopt;
	};
	auto find_short = [&](std::string_view surface, char c)
		-> std::optional<std::size_t>
	{
		for (std::size_t i = 0; i < cat.specs.size(); ++i)
			for (const auto& sf : cat.specs[i].surfaces)
				if (sf.command == surface && sf.short_name == c)
					return i;
		return std::nullopt;
	};
	auto is_flag = [&](std::size_t i) {
		return kind_of(cat.specs[i]) == option_kind::flag;
	};
	// A list given again appends. Every other kind keeps the last value.
	auto add_text = [&](std::size_t i, std::string_view text) {
		const option_spec& o = cat.specs[i];
		auto v = r.merge_take(detail::parse_value(
			kind_of(o), text, o.name));
		if (!v) return false;
		if (const auto* add = std::get_if<option_list>(&*v))
			if (option_value* have = pending_of(i)) {
				auto& list = std::get<option_list>(*have);
				list.insert(list.end(), add->begin(), add->end());
				return true;
			}
		put(i, std::move(*v));
		return true;
	};
	// 0: done, 1: error, 2: a command, 3: an unknown option before the
	// command, which the default command can still take.
	auto process = [&](std::size_t& arg, bool has_cmd,
		std::string_view surface)
	{
		std::string opt = args[arg];
		if (cat.command(opt)) return 2;
		bool is_short = false;
		if (!detail::option_prefix(opt, is_short)) {
			if (opt == "-" || std::ifstream(opt).good()
				|| detail::is_url(opt))
			{
				out.inputs.push_back(opt);
				++arg;
				return 0;
			}
			fail_with_help(ps::messages::option_no_such_input,
				{ { ps::label::value, opt } });
			return 1;
		}
		std::string value;
		bool has_value = false;
		if (!is_short) {
			if (auto eq = opt.find('='); eq != std::string::npos) {
				value = opt.substr(eq + 1);
				opt = opt.substr(0, eq);
				has_value = true;
			}
		}
		std::optional<std::size_t> cur;
		if (is_short) {
			for (std::size_t k = 0; k != opt.size(); ++k) {
				cur = find_short(surface, opt[k]);
				if (!cur) {
					if (!has_cmd) return 3;
					fail_with_help(
						ps::messages::option_not_found,
						{ { ps::label::name, "-"
							+ std::string(1, opt[k]) } });
					return 1;
				}
				if (is_flag(*cur)) put(*cur, true);
				else if (k + 1 < opt.size()) {
					fail_with_help(
					    ps::messages::option_missing_argument,
					    { { ps::label::name,
						cat.specs[*cur].name } });
					return 1;
				}
			}
		} else {
			cur = find_long(surface, opt);
			if (!cur) {
				if (!has_cmd) return 3;
				fail_with_help(ps::messages::option_not_found,
					{ { ps::label::name, opt } });
				return 1;
			}
		}
		++arg;
		if (is_flag(*cur)) {
			if (has_value) return add_text(*cur, value) ? 0 : 1;
			bool b = true;
			// a word that is no flag text is the next argument
			if (arg < args.size())
				if (auto v = detail::parse_value(
					option_kind::flag, args[arg]); v.has_value())
				{
					b = std::get<bool>(v.value());
					++arg;
				}
			put(*cur, b);
			return 0;
		}
		if (!has_value) {
			if (arg >= args.size()
				|| detail::is_option_prefix(args[arg]))
			{
				r.error(code::invalid_argument,
					ps::messages::option_missing_argument,
					{ { ps::label::name,
						cat.specs[*cur].name } });
				return 1;
			}
			value = args[arg++];
		}
		return add_text(*cur, value) ? 0 : 1;
	};

	std::size_t arg = 1;
	int status = 0;
	while (arg < args.size())
		if ((status = process(arg, false, cfg.global_surface)) != 0)
			break;
	if (status == 1) return r;
	if (status == 3 && (cfg.default_command.empty()
		|| (!out.inputs.empty() && cfg.default_command_when_inputs.empty())))
	{
		fail_with_help(ps::messages::option_not_found,
			{ { ps::label::name, args[arg] } });
		return r;
	}
	const std::string cmd = status == 2 ? args[arg++]
		: out.inputs.empty() ? cfg.default_command
		: cfg.default_command_when_inputs;
	if (!cmd.empty()) {
		if (!cat.command(cmd)) {
			fail_with_help(ps::messages::option_unknown_command,
				{ { ps::label::name, cmd } });
			return r;
		}
		while (arg < args.size()) {
			status = process(arg, true, cmd);
			if (status == 2) {
				fail_with_help(ps::messages::option_second_command,
					{ { ps::label::name, args[arg] } });
				return r;
			}
			if (status != 0) return r;
		}
		out.command = cmd;
	}
	// a field takes no value until every value of argv fits its field
	for (const auto& [i, v] : pending)
		if (bindings_[i]) r.merge(bindings_[i]->write(v, false));
	if (r.has_error()) return r;
	command_ = out.command;
	for (auto& [i, v] : pending) {
		r.merge(store(i, v));
		out.values.emplace_back(cat.specs[i].name, std::move(v));
	}
	return r.with_value(std::move(out));
}

inline void options_repository::help(std::ostream& os,
	std::string_view command) const
{
	const args_config& cfg = cfg_;
	const detail::options_catalog& cat = *cat_;
	const std::string& header = cfg.help_header.empty() ? cfg.name
		: cfg.help_header;

	using row_t = std::pair<std::size_t, const option_surface*>;
	auto rows_of = [&](std::string_view surface) {
		std::vector<row_t> rows;
		for (std::size_t i = 0; i < cat.specs.size(); ++i)
			for (const auto& sf : cat.specs[i].surfaces)
				if (sf.command == surface) {
					rows.emplace_back(i, &sf);
					break;
				}
		std::sort(rows.begin(), rows.end(),
			[&](const row_t& a, const row_t& b) {
				return cat.specs[a.first].name
					< cat.specs[b.first].name;
			});
		return rows;
	};
	auto print_rows = [&](const std::vector<row_t>& rows,
		std::string_view cmd)
	{
		for (const auto& [i, sf] : rows) {
			const option_spec& o = cat.specs[i];
			os << "\t--" << o.name
				<< (o.name.length() < 6 ? "\t\t" : "\t");
			if (sf->short_name) os << '-' << sf->short_name;
			os << "\t" << o.help;
			const std::string def = detail::format_value(
				effective(i, cmd));
			if (!def.empty()) os << " (default: " << def << ")";
			os << "\n";
		}
	};

	if (const command_spec* c = command.empty() ? nullptr
		: cat.command(command))
	{
		const auto rows = rows_of(c->name);
		os << header << " " << c->name;
		if (!rows.empty()) os << " [ options ]";
		os << "\n";
		if (!c->help.empty()) os << c->help << "\n\n";
		os << "Command options:\n";
		print_rows(rows, c->name);
		os << "\n";
		return;
	}
	const auto rows = rows_of(cfg.global_surface);
	os << header;
	if (!rows.empty()) os << " [ options ] ";
	if (!cat.commands.empty()) {
		if (!cfg.default_command.empty()) os << "[ ";
		os << "<command> [ <command options> ] ";
		if (!cfg.default_command.empty()) os << "]";
	}
	os << "\n";
	if (!cfg.description.empty()) os << cfg.description << "\n";
	if (!rows.empty()) {
		os << "\nOptions:\n";
		print_rows(rows, command_);
	}
	if (!cat.commands.empty()) {
		std::vector<const command_spec*> cmds;
		for (const auto& c : cat.commands) cmds.push_back(&c);
		std::sort(cmds.begin(), cmds.end(),
			[](const command_spec* a, const command_spec* b) {
				return a->name < b->name;
			});
		os << "Commands:\n";
		for (const auto* c : cmds)
			os << "\t" << c->name << "\t\t\t" << c->help << "\n";
	}
	os << "\n";
}

namespace detail {

// null until the first options() call or scope of the thread
inline options_repository*& current_options() {
	thread_local options_repository* current = nullptr;
	return current;
}

} // namespace detail

inline options_repository& options() {
	options_repository*& current = detail::current_options();
	if (!current) {
		thread_local options_repository own;
		current = &own;
	}
	return *current;
}

inline options_scope::options_scope(options_repository& r)
	: previous_(detail::current_options())
{
	DBG(current_ = &r;)
	detail::current_options() = &r;
}

inline options_scope::~options_scope() {
	DBG(assert(detail::current_options() == current_
		&& "an options scope ends out of order");)
	detail::current_options() = previous_;
}

} // namespace idni

#endif // __IDNI__PARSER__UTILITY__OPTIONS_TMPL_H__
