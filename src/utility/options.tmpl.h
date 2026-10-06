// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__OPTIONS_TMPL_H__
#define __IDNI__PARSER__UTILITY__OPTIONS_TMPL_H__

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <system_error>
#include <type_traits>
#include <unordered_map>
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

/// Parse @p text as a value of @p kind. A flag takes 1 t true on yes y and
/// 0 f false off no n in any case. A list is comma separated.
inline diagnostics::result<option_value> parse_option_value(option_kind kind,
	std::string_view text, std::string_view name = {})
{
	diagnostics::result<option_value> r;
	auto v = parse_value(kind, text);
	if (!v) return r.with_error(diagnostics::code::invalid_argument,
		parser_strings::messages::option_bad_value,
		{ { parser_strings::label::name, name },
		  { parser_strings::label::value, text } });
	return r.with_value(std::move(*v));
}

/// The text form of @p v, which parse_option_value() reads back.
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

/// The shared values. A snapshot never changes after it is published.
struct options_snapshot {
	std::shared_ptr<const options_catalog> cat;
	/// One row per option, one cell per source id.
	std::vector<std::vector<std::optional<option_value>>> values;
	/// The command of the last parse_args(), which selects the surface
	/// defaults.
	std::string command;
};

using local_values = std::unordered_map<std::size_t, option_value>;

struct thread_slots;

/// The state of one thread for one repository. Only that thread reads or
/// writes its values. The repository owns it.
struct thread_record {
	std::shared_ptr<const options_snapshot> snap;
	/// The shared version that `snap` belongs to.
	std::size_t snap_version = 0;
	/// Grows with each set_local() or clear_local() that changes a value.
	std::size_t local_version = 0;
	local_values local;
	/// The index of the thread, and its slots.
	std::size_t index = 0;
	thread_slots* owner = nullptr;
};

/// A repository finds the record of a thread with no lock for this many
/// threads at once. Further threads take the lock of the repository.
inline constexpr std::size_t record_page_size = 32;
inline constexpr std::size_t record_page_count = 256;

struct record_page {
	std::array<std::atomic<thread_record*>, record_page_size> slot{};
};

/// The links between the threads and the repositories. Its mutex is taken
/// before the mutex of a repository, never after it.
struct options_links {
	std::mutex mutex;
	std::vector<thread_slots*> threads;
	/// The indices of ended threads, which a new thread takes first.
	std::vector<std::size_t> free_indices;
	std::size_t next_index = 0;
	std::size_t records = 0;
};

// A repository and a thread_slots touch it in their constructors, so it is
// destroyed after the last of them.
inline options_links& links() {
	static options_links l;
	return l;
}

/// The part of one thread that the repositories see: its index and the
/// repositories that hold a record of it.
struct thread_slots {
	std::size_t index = 0;
	std::vector<repository_state*> repos;
	thread_slots();
	~thread_slots();
	thread_slots(const thread_slots&) = delete;
	thread_slots& operator=(const thread_slots&) = delete;
};

inline thread_slots& this_thread_slots() {
	thread_local thread_slots me;
	return me;
}

/// The numbers that a test checks after the threads and the repositories
/// end.
struct options_link_counts {
	std::size_t threads = 0;
	std::size_t entries = 0;
	std::size_t records = 0;
};

inline options_link_counts link_counts() {
	options_links& l = links();
	std::lock_guard<std::mutex> lock(l.mutex);
	options_link_counts c;
	c.threads = l.threads.size();
	for (const thread_slots* t : l.threads) c.entries += t->repos.size();
	c.records = l.records;
	return c;
}

/// The value of option @p i: the thread source, the sources from high to
/// low, the default of @p command, the default of the option.
inline const option_value& effective(const options_snapshot& s,
	const local_values& local, std::size_t i, std::string_view command)
{
	if (auto it = local.find(i); it != local.end()) return it->second;
	const auto& row = s.values[i];
	for (auto it = row.rbegin(); it != row.rend(); ++it)
		if (*it) return **it;
	const option_spec& o = s.cat->specs[i];
	if (!command.empty())
		for (const auto& sf : o.surfaces)
			if (sf.command == command && sf.fallback)
				return *sf.fallback;
	return o.fallback;
}

inline const option_value& effective(const options_snapshot& s,
	const local_values& local, std::size_t i)
{
	return effective(s, local, i, s.command);
}

template <typename T>
T typed(const option_value& v) {
	if (const T* p = std::get_if<T>(&v)) return *p;
	DBG(assert(false && "an option is read with another type");)
	return T{};
}

/// Store @p v at @p source. False when the cell holds it already.
inline bool set_cell(options_snapshot& s, std::size_t i, std::size_t source,
	option_value v)
{
	auto& row = s.values[i];
	if (row.size() <= source) row.resize(source + 1);
	auto& cell = row[source];
	if (cell && *cell == v) return false;
	cell = std::move(v);
	return true;
}

inline bool clear_cell(options_snapshot& s, std::size_t i, std::size_t source)
{
	auto& row = s.values[i];
	if (source >= row.size() || !row[source]) return false;
	row[source].reset();
	return true;
}

/// The hidden part of an options_repository.
struct repository_state {
	std::mutex mutex;
	std::shared_ptr<const options_snapshot> snap;
	std::atomic<std::size_t> version{0};
	std::vector<std::string> sources;
	args_config cfg;
	/// One record per thread that uses the repository. A thread that ends
	/// removes its record, and the repository frees the rest.
	std::vector<std::unique_ptr<thread_record>> records;
	/// The record of each thread by its index, read with no lock.
	std::array<std::atomic<record_page*>, record_page_count> pages{};
	/// The records of the threads whose index is above the pages.
	std::unordered_map<std::size_t, thread_record*> overflow;

	repository_state();
	~repository_state();
	repository_state(const repository_state&) = delete;
	repository_state& operator=(const repository_state&) = delete;

	thread_record& record();
	thread_record& add_record(thread_slots& me);
	void drop_record(std::size_t index);
	const options_snapshot& current();
	template <typename F> void modify(F&& f);
};

} // namespace detail

template <typename T>
T options_view::get(std::string_view name) const {
	static_assert(detail::is_option_type_v<T>,
		"an option value is bool, size_t, string or option_list");
	if (!snap_) {
		DBG(assert(false && "a view is read before view() gave it");)
		return T{};
	}
	const auto i = snap_->cat->find(name);
	if (!i) {
		DBG(assert(false && "an unknown option is read");)
		return T{};
	}
	return detail::typed<T>(detail::effective(*snap_, local_, *i));
}

namespace detail {

inline thread_slots::thread_slots() {
	options_links& l = links();
	std::lock_guard<std::mutex> lock(l.mutex);
	if (l.free_indices.empty()) index = l.next_index++;
	else {
		index = l.free_indices.back();
		l.free_indices.pop_back();
	}
	l.threads.push_back(this);
}

// A thread that ends leaves every repository that holds a record of it, so
// a new thread with the same index starts with no values.
inline thread_slots::~thread_slots() {
	options_links& l = links();
	std::lock_guard<std::mutex> lock(l.mutex);
	for (repository_state* r : repos) r->drop_record(index);
	l.free_indices.push_back(index);
	l.threads.erase(std::remove(l.threads.begin(), l.threads.end(), this),
		l.threads.end());
	if (l.threads.empty()) {
		std::vector<thread_slots*>().swap(l.threads);
		std::vector<std::size_t>().swap(l.free_indices);
		l.next_index = 0;
	}
}


inline repository_state::repository_state()
	: snap(std::make_shared<const options_snapshot>(options_snapshot{
		std::make_shared<const options_catalog>(), {}, {} }))
{
	links();
}

// Every thread that holds a record of this repository forgets it here, so
// nothing of the repository stays in any thread.
inline repository_state::~repository_state() {
	options_links& l = links();
	std::lock_guard<std::mutex> links_lock(l.mutex);
	std::lock_guard<std::mutex> lock(mutex);
	for (const auto& rec : records) {
		auto& repos = rec->owner->repos;
		repos.erase(std::remove(repos.begin(), repos.end(), this),
			repos.end());
		if (repos.empty()) std::vector<repository_state*>().swap(repos);
		--l.records;
	}
	records.clear();
	overflow.clear();
	for (auto& page : pages) delete page.exchange(nullptr);
}

// The common case: the record of this thread exists, and no lock is taken.
inline thread_record& repository_state::record() {
	thread_slots& me = this_thread_slots();
	const std::size_t i = me.index;
	if (i < record_page_size * record_page_count) {
		if (const record_page* page = pages[
			i / record_page_size].load(
				std::memory_order_acquire))
			if (thread_record* r = page->slot[
				i % record_page_size].load(
					std::memory_order_acquire))
				return *r;
	} else {
		std::lock_guard<std::mutex> lock(mutex);
		if (auto it = overflow.find(i); it != overflow.end())
			return *it->second;
	}
	return add_record(me);
}

inline thread_record& repository_state::add_record(thread_slots& me) {
	options_links& l = links();
	std::lock_guard<std::mutex> links_lock(l.mutex);
	std::lock_guard<std::mutex> lock(mutex);
	auto rec = std::make_unique<thread_record>();
	rec->index = me.index;
	rec->owner = &me;
	thread_record* p = rec.get();
	records.push_back(std::move(rec));
	const std::size_t i = me.index;
	if (i < record_page_size * record_page_count) {
		auto& page = pages[i / record_page_size];
		record_page* pg = page.load(std::memory_order_relaxed);
		if (!pg) {
			pg = new record_page{};
			page.store(pg, std::memory_order_release);
		}
		pg->slot[i % record_page_size].store(p,
			std::memory_order_release);
	} else overflow[i] = p;
	me.repos.push_back(this);
	++l.records;
	return *p;
}

// The thread of @p index ends. The caller holds the links mutex.
inline void repository_state::drop_record(std::size_t index) {
	std::lock_guard<std::mutex> lock(mutex);
	if (index < record_page_size * record_page_count) {
		if (record_page* page = pages[
			index / record_page_size].load(
				std::memory_order_relaxed))
			page->slot[index % record_page_size].store(
				nullptr, std::memory_order_release);
	} else overflow.erase(index);
	records.erase(std::remove_if(records.begin(), records.end(),
		[&](const auto& rec) { return rec->index == index; }),
		records.end());
	if (records.empty())
		std::vector<std::unique_ptr<thread_record>>().swap(records);
	--links().records;
}

// A reader locks only when the shared version moved since its last read.
inline const options_snapshot& repository_state::current() {
	thread_record& t = record();
	const std::size_t v = version.load(std::memory_order_acquire);
	if (!t.snap || t.snap_version != v) {
		std::lock_guard<std::mutex> lock(mutex);
		t.snap = snap;
		t.snap_version = version.load(std::memory_order_relaxed);
	}
	return *t.snap;
}

// @p f edits a copy and returns true when it changed a value. Only then the
// copy is published, so a reader never sees a half-done write.
template <typename F>
void repository_state::modify(F&& f) {
	std::lock_guard<std::mutex> lock(mutex);
	auto next = std::make_shared<options_snapshot>(*snap);
	if (!f(*next)) return;
	snap = std::move(next);
	version.fetch_add(1, std::memory_order_release);
}

} // namespace detail

inline options_repository::options_repository()
	: state_(std::make_unique<detail::repository_state>())
{}

// The state frees the records of every thread that used the repository.
inline options_repository::~options_repository() = default;

inline std::size_t options_repository::source(std::string_view name) {
	std::lock_guard<std::mutex> lock(state_->mutex);
	for (std::size_t i = 0; i < state_->sources.size(); ++i)
		if (state_->sources[i] == name) return i;
	state_->sources.emplace_back(name);
	return state_->sources.size() - 1;
}

inline diagnostics::result<void> options_repository::declare(
	const option_set& set)
{
	namespace ps = parser_strings;
	using diagnostics::code;
	diagnostics::result<void> r;
	state_->modify([&](detail::options_snapshot& s) {
		auto cat = std::make_shared<detail::options_catalog>(*s.cat);
		std::map<std::pair<std::string, char>, std::string> shorts;
		for (const auto& o : cat->specs)
			for (const auto& sf : o.surfaces) if (sf.short_name)
				shorts.emplace(std::make_pair(sf.command,
					sf.short_name), o.name);
		for (const auto& c : set.commands) {
			if (cat->command(c.name)) {
				r.error(code::invalid_argument,
					ps::messages::command_already_exists,
					{ { ps::label::name, c.name } });
				continue;
			}
			cat->commands.push_back(c);
		}
		for (const auto& o : set.options) {
			if (cat->find(o.name)) {
				r.error(code::invalid_argument,
					ps::messages::option_already_exists,
					{ { ps::label::name, o.name } });
				continue;
			}
			for (const auto& sf : o.surfaces) {
				if (sf.fallback && sf.fallback->index()
					!= o.fallback.index())
					r.error(code::type_error, ps::messages::
						option_command_default_kind,
						{ { ps::label::name, o.name },
						  { ps::label::value, sf.command } });
				if (!sf.short_name) continue;
				auto [it, fresh] = shorts.emplace(std::make_pair(
					sf.command, sf.short_name), o.name);
				if (!fresh) r.error(code::invalid_argument,
					ps::messages::option_short_name_taken,
					{ { ps::label::name, it->second },
					  { ps::label::name, o.name },
					  { ps::label::value,
						std::string(1, sf.short_name) } });
			}
			cat->index.emplace(o.name, cat->specs.size());
			cat->specs.push_back(o);
		}
		if (r.has_error()) return false;
		s.values.resize(cat->specs.size());
		s.cat = std::move(cat);
		return true;
	});
	return r;
}

template <typename T>
T options_repository::get(std::string_view name) const {
	static_assert(detail::is_option_type_v<T>,
		"an option value is bool, size_t, string or option_list");
	const detail::options_snapshot& s = state_->current();
	const auto i = s.cat->find(name);
	if (!i) {
		DBG(assert(false && "an unknown option is read");)
		return T{};
	}
	return detail::typed<T>(detail::effective(s,
		state_->record().local, *i));
}

template <typename T>
void options_repository::set(std::string_view name, T value,
	std::size_t source)
{
	option_value v = detail::to_option_value(std::move(value));
	state_->modify([&](detail::options_snapshot& s) {
		DBG(assert(source < state_->sources.size() && "an unknown source");)
		const auto i = s.cat->find(name);
		if (!i) {
			DBG(assert(false && "an unknown option is set");)
			return false;
		}
		if (s.cat->specs[*i].fallback.index() != v.index()) {
			DBG(assert(false && "an option is set with another type");)
			return false;
		}
		return detail::set_cell(s, *i, source, std::move(v));
	});
}

inline void options_repository::clear(std::string_view name,
	std::size_t source)
{
	state_->modify([&](detail::options_snapshot& s) {
		const auto i = s.cat->find(name);
		if (!i) {
			DBG(assert(false && "an unknown option is cleared");)
			return false;
		}
		return detail::clear_cell(s, *i, source);
	});
}

inline bool options_repository::is_set(std::string_view name,
	std::size_t source) const
{
	const detail::options_snapshot& s = state_->current();
	const auto i = s.cat->find(name);
	if (!i) {
		DBG(assert(false && "an unknown option is read");)
		return false;
	}
	const auto& row = s.values[*i];
	return source < row.size() && row[source].has_value();
}

template <typename T>
void options_repository::set_local(std::string_view name, T value) {
	option_value v = detail::to_option_value(std::move(value));
	const detail::options_snapshot& s = state_->current();
	const auto i = s.cat->find(name);
	if (!i) {
		DBG(assert(false && "an unknown option is set");)
		return;
	}
	if (s.cat->specs[*i].fallback.index() != v.index()) {
		DBG(assert(false && "an option is set with another type");)
		return;
	}
	detail::thread_record& t = state_->record();
	if (auto it = t.local.find(*i); it != t.local.end() && it->second == v)
		return;
	t.local[*i] = std::move(v);
	++t.local_version;
}

inline void options_repository::clear_local(std::string_view name) {
	const detail::options_snapshot& s = state_->current();
	const auto i = s.cat->find(name);
	if (!i) {
		DBG(assert(false && "an unknown option is cleared");)
		return;
	}
	detail::thread_record& t = state_->record();
	if (t.local.erase(*i)) ++t.local_version;
}

inline options_view options_repository::view() const {
	state_->current();
	const auto& t = state_->record();
	options_view v;
	v.snap_ = t.snap;
	v.local_ = t.local;
	return v;
}

inline diagnostics::result<void> options_repository::set_text(
	std::string_view name, std::string_view text, std::size_t source)
{
	namespace ps = parser_strings;
	diagnostics::result<void> r;
	state_->modify([&](detail::options_snapshot& s) {
		const auto i = s.cat->find(name);
		if (!i) {
			r.error(diagnostics::code::not_found,
				ps::messages::option_not_found,
				{ { ps::label::name, name } });
			return false;
		}
		auto v = r.merge_take(detail::parse_option_value(
			detail::kind_of(s.cat->specs[*i].fallback), text, name));
		if (!v) return false;
		return detail::set_cell(s, *i, source, std::move(*v));
	});
	return r;
}

inline diagnostics::result<std::string> options_repository::get_text(
	std::string_view name) const
{
	namespace ps = parser_strings;
	diagnostics::result<std::string> r;
	const detail::options_snapshot& s = state_->current();
	const auto i = s.cat->find(name);
	if (!i) return r.with_error(diagnostics::code::not_found,
		ps::messages::option_not_found, { { ps::label::name, name } });
	auto text = r.merge_take(detail::format_option_value(detail::effective(
		s, state_->record().local, *i)));
	if (!text) return r;
	return r.with_value(std::move(*text));
}

inline std::vector<std::string> options_repository::names() const {
	std::vector<std::string> out;
	for (const auto& o : state_->current().cat->specs) out.push_back(o.name);
	return out;
}

inline diagnostics::result<void> options_repository::load_env(
	std::string_view prefix, std::size_t source)
{
	namespace ps = parser_strings;
	diagnostics::result<void> r;
	state_->modify([&](detail::options_snapshot& s) {
		bool changed = false;
		for (std::size_t i = 0; i < s.cat->specs.size(); ++i) {
			const option_spec& o = s.cat->specs[i];
			if (!o.from_env) continue;
			const std::string var = detail::option_env_name(prefix,
				o.name);
			const char* text = std::getenv(var.c_str());
			if (!text) continue;
			auto v = detail::parse_value(detail::kind_of(o.fallback),
				text);
			if (!v) {
				r.warning(ps::messages::option_env_bad_value,
					{ { ps::label::name, var },
					  { ps::label::value, text } });
				continue;
			}
			changed = detail::set_cell(s, i, source, std::move(*v))
				|| changed;
		}
		return changed;
	});
	return r;
}

inline diagnostics::result<parsed_args> options_repository::parse_args(
	int argc, char** argv, std::size_t source, const args_config& cfg)
{
	namespace ps = parser_strings;
	using diagnostics::code;
	using diagnostics::attr_in;
	{
		std::lock_guard<std::mutex> lock(state_->mutex);
		state_->cfg = cfg;
	}
	diagnostics::result<parsed_args> r;
	state_->current();
	// A copy, so the help() of an error cannot drop the snapshot we read.
	const auto snap = state_->record().snap;
	const detail::options_catalog& cat = *snap->cat;
	std::vector<std::string> args;
	for (int i = 0; i < argc; ++i) args.emplace_back(argv[i]);
	parsed_args out;
	// Stored only when the whole command line is valid.
	std::map<std::size_t, option_value> pending;

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
		return detail::kind_of(cat.specs[i].fallback)
			== detail::option_kind::flag;
	};
	// A list given again appends. Every other kind keeps the last value.
	auto store = [&](std::size_t i, std::string_view text) {
		const option_spec& o = cat.specs[i];
		auto v = r.merge_take(detail::parse_option_value(
			detail::kind_of(o.fallback), text, o.name));
		if (!v) return false;
		if (const auto* add = std::get_if<option_list>(&*v))
			if (auto it = pending.find(i); it != pending.end()) {
				auto& have = std::get<option_list>(it->second);
				have.insert(have.end(), add->begin(), add->end());
				return true;
			}
		pending[i] = std::move(*v);
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
				if (is_flag(*cur)) pending[*cur] = true;
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
			if (has_value) return store(*cur, value) ? 0 : 1;
			bool b = true;
			if (arg < args.size())
				if (auto v = detail::parse_value(
					detail::option_kind::flag, args[arg]))
				{
					b = std::get<bool>(*v);
					++arg;
				}
			pending[*cur] = b;
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
		return store(*cur, value) ? 0 : 1;
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
	if (const auto h = cat.find("help"))
		if (auto it = pending.find(*h); it != pending.end())
			if (const bool* b = std::get_if<bool>(&it->second))
				out.help = *b;
	state_->modify([&](detail::options_snapshot& s) {
		bool changed = s.command != out.command;
		s.command = out.command;
		for (auto& [i, v] : pending)
			changed = detail::set_cell(s, i, source, std::move(v))
				|| changed;
		return changed;
	});
	return r.with_value(std::move(out));
}

inline void options_repository::help(std::ostream& os,
	std::string_view command) const
{
	args_config cfg;
	{
		std::lock_guard<std::mutex> lock(state_->mutex);
		cfg = state_->cfg;
	}
	state_->current();
	const auto& t = state_->record();
	const auto snap = t.snap;
	const detail::options_catalog& cat = *snap->cat;
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
	// A row shows the value that the command gets: a source value, else
	// the default of the command, else the default of the option.
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
				detail::effective(*snap, t.local, i, cmd));
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
		print_rows(rows, snap->command);
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

inline std::size_t options_repository::version() const {
	return state_->version.load(std::memory_order_acquire) + state_->record().local_version;
}

} // namespace idni

#endif // __IDNI__PARSER__UTILITY__OPTIONS_TMPL_H__
