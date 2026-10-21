// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__REPL_HISTORY_H__
#define __IDNI__PARSER__UTILITY__REPL_HISTORY_H__

#include <string>
#include <vector>
#include <optional>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "repl_history_policy.h"

namespace idni {

// REPL command history with line-delimited file persistence. Each entry is
// stored on one physical line; only the record separator '\n' and the escape
// char '\\' are escaped (an unknown "\c" decodes to "c"). Owns the entry list
// and the navigation cursor; navigation methods report what the editor should
// display via std::optional (nullopt = no change, value = set input, empty
// value = clear input).
struct repl_history {
	explicit repl_history(repl_history_policy policy = {})
		: policy_(std::move(policy)) { load(); }

	bool   empty() const { return entries_.empty(); }
	size_t size()  const { return entries_.size(); }

	// Append `s` (deduplicated against the most recent entry) and persist
	// it to the file, unless the policy drops it. Resets the navigation
	// cursor to the end.
	void store(const std::string& s) {
		stash_.reset();
		if ((policy_.keep && !policy_.keep(s))
			|| (!entries_.empty() && entries_.back() == s))
		{
			hpos_ = entries_.size();
			return;
		}
		entries_.push_back(s);
		hpos_ = entries_.size();
		if (policy_.file.empty()) return;
		std::ofstream f(policy_.file, std::ios::app);
		if (f) f << escape(s) << '\n';
	}

	// Previous entry (↑). `current` is the live edit buffer; stepping up
	// from the end stashes it (not as a real entry) so ↓ can restore it.
	std::optional<std::string> prev(const std::string& current) {
		if (entries_.empty() || hpos_ == 0) return std::nullopt;
		if (hpos_ == entries_.size() && !current.empty())
			stash_ = current;
		return entries_[--hpos_];
	}
	// Next entry (↓). Returns the stashed live buffer (or empty) when
	// stepping past the newest entry back to it.
	std::optional<std::string> next() {
		if (hpos_ == entries_.size()) return std::nullopt;
		if (++hpos_ == entries_.size()) {
			auto s = stash_.value_or(std::string{});
			stash_.reset();
			return s;
		}
		return entries_[hpos_];
	}
	// First entry (Ctrl-↑).
	std::optional<std::string> first() {
		if (entries_.empty() || hpos_ == 0) return std::nullopt;
		hpos_ = 0;
		return entries_[hpos_];
	}
	// Past the newest entry (Ctrl-↓) — clears the input.
	std::optional<std::string> past_end() {
		if (hpos_ == entries_.size()) return std::nullopt;
		hpos_ = entries_.size();
		return std::string{};
	}

private:
	void load() {
		if (policy_.file.empty()) return;
		if (!std::filesystem::exists(policy_.file)) return;
		std::ifstream f(policy_.file);
		std::string line;
		while (std::getline(f, line))
			if (line.size()) entries_.push_back(unescape(line));
		hpos_ = entries_.size();
	}
	static std::string escape(const std::string& s) {
		std::ostringstream oss;
		for (char c : s) oss << (c == '\\' ? "\\\\"
					: c == '\n' ? "\\n"
					: std::string(1, c));
		return oss.str();
	}
	static std::string unescape(const std::string& s) {
		std::ostringstream oss;
		for (size_t i = 0; i < s.length(); ++i) {
			if (s[i] == '\\' && i + 1 < s.length()) {
				char next = s[++i];
				oss << (next == 'n' ? '\n'
					: next == '\\' ? '\\' : next);
			} else oss << s[i];
		}
		return oss.str();
	}

	repl_history_policy        policy_;
	std::vector<std::string>   entries_;
	size_t                     hpos_ = 0; // navigation cursor, 0..size()
	std::optional<std::string> stash_; // live edit buffer parked by prev()
};

} // namespace idni

#endif // __IDNI__PARSER__UTILITY__REPL_HISTORY_H__
