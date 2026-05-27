// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__MEASURE_H__
#define __IDNI__PARSER__UTILITY__MEASURE_H__

#include <chrono>
#include <ctime>
#include <string>
#include <iostream>
#include <iomanip>
#include <map>
#include <fstream>

#if defined(__linux__) || defined(__APPLE__)
#	include <unistd.h>
#	include <sys/resource.h>
#	ifdef __APPLE__
#		include <mach/mach.h>
#	endif
#elif defined(_WIN32)
#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#	include <windows.h>
#	include <psapi.h>
#	ifdef _MSC_VER
#		pragma comment(lib, "psapi.lib")
#	endif
#endif

namespace idni::measures {

/// current resident set size in KB
[[maybe_unused]]
inline size_t current_rss_kb();

/// peak resident set size since process start, in KB
[[maybe_unused]]
inline size_t peak_rss_kb();

struct cpu_clock_tag {
	using time_point = clock_t;
	static time_point now();
	static double ms_between(time_point a, time_point b);
};

struct steady_clock_tag {
	using time_point = std::chrono::steady_clock::time_point;
	static time_point now();
	static double ms_between(time_point a, time_point b);
};

/// Elapsed-time accumulator with real pause/resume semantics.
///  `basic_timer<cpu_clock_tag>` measures process CPU time via clock();
///  `basic_timer<steady_clock_tag>` measures wall time via std::chrono::steady_clock.
///
/// State machine: { stopped, running, paused }. Transitions:
///   stopped --start--> running   (ms_ reset to 0)
///   stopped --unpause-> running  (ms_ reset to 0; returns 0)
///   stopped --restart-> running  (returns frozen ms_, initially 0; resets ms_)
///   running --pause--> paused    (accumulates [start_time_, now] into ms_)
///   paused  --unpause-> running  (start_time_ = now)
///   running --stop--> stopped    (flushes active interval)
///   paused  --stop--> stopped    (total unchanged)
///   running/paused --restart--> running (returns previous total; resets ms_)
/// All non-transitioning calls are idempotent (e.g. pause-when-paused is a no-op).
/// get() is non-mutating: it samples the current total without changing state.
template <typename ClockTag = cpu_clock_tag>
struct basic_timer {

	using time_point = typename ClockTag::time_point;

	time_point start_time_{};
	bool started_ = false;
	bool paused_  = false;
	bool silent_  = false;
	double ms_    = 0;

	basic_timer(bool silent = false);

	void start();
	double restart();
	double pause();
	double unpause();
	double stop();
	double get() const;

	~basic_timer();
};

using timer        = basic_timer<cpu_clock_tag>;
using steady_timer = basic_timer<steady_clock_tag>;

static std::map<std::string, timer> timers;
static std::map<std::string, size_t> counters;

template<typename node_t>
using rule = std::pair<node_t, node_t>;

template<typename node_t>
static std::map<rule<node_t>, size_t> rule_counters;

template<typename node_t>
static std::map<rule<node_t>, size_t> rule_hits;

[[maybe_unused]]
static void start_timer(const std::string& name, bool silent = false);

[[maybe_unused]]
static void restart_timer(const std::string& name);

[[maybe_unused]]
static void pause_timer(const std::string& name);

[[maybe_unused]]
static void unpause_timer(const std::string& name);

[[maybe_unused]]
static double get_timer(const std::string& name);

[[maybe_unused]]
static void stop_timer(const std::string& name);

[[maybe_unused]]
static void print_timer(const std::string& name);

[[maybe_unused]]
static void remove_timer(const std::string& name);

[[maybe_unused]]
static void remove_all_timers();

[[maybe_unused]]
static size_t increase_counter(const std::string& name);

[[maybe_unused]]
static size_t get_counter(const std::string& name);

[[maybe_unused]]
static void remove_counter(const std::string& name);

[[maybe_unused]]
static void remove_all_counters();

template<typename node_t>
static size_t increase_rule_counter(const rule<node_t>& r);

template<typename node_t>
static size_t get_rule_counter(const rule<node_t>& r);

template<typename node_t>
static void remove_rule_counter(const rule<node_t>& r);

template<typename node_t>
static void remove_all_rule_counters();

template<typename node_t>
static size_t increase_rule_hit(const rule<node_t>& r);

template<typename node_t>
static size_t get_rule_hit(const rule<node_t>& r);

template<typename node_t>
static void remove_rule_hit(const rule<node_t>& r);

template<typename node_t>
static void remove_all_rule_hits();

template<typename node_t>
static void remove_all();

} // namespace idni::measures

#include "measure.tmpl.h"
#endif // __IDNI__PARSER__UTILITY__MEASURE_H__
