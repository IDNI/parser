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
inline long current_rss_kb() {
#ifdef __linux__
	std::ifstream f("/proc/self/statm");
	if (!f.is_open()) return -1;
	long total_pages = 0, rss_pages = 0;
	if (!(f >> total_pages >> rss_pages)) return -1;
	static long page_size_kb = sysconf(_SC_PAGESIZE) / 1024;
	return rss_pages * page_size_kb;
#elif defined(__APPLE__)
	task_vm_info_data_t vm_info;
	mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
	if (task_info(mach_task_self(), TASK_VM_INFO,
			(task_info_t)&vm_info, &count) != KERN_SUCCESS)
		return -1;
	return static_cast<long>(vm_info.phys_footprint / 1024);
#elif defined(_WIN32)
	HANDLE hProcess = GetCurrentProcess();
	PROCESS_MEMORY_COUNTERS pmc;
	pmc.cb = sizeof(pmc);
	if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
		return static_cast<long>(pmc.WorkingSetSize / 1024);
	return -1;
#else
	return -1;
#endif
}

/// peak resident set size since process start, in KB
[[maybe_unused]]
inline long peak_rss_kb() {
#if defined(__linux__)
	struct rusage ru;
	if (getrusage(RUSAGE_SELF, &ru) != 0) return -1;
	return ru.ru_maxrss;
#elif defined(__APPLE__)
	struct rusage ru;
	if (getrusage(RUSAGE_SELF, &ru) != 0) return -1;
	return ru.ru_maxrss / 1024;
#elif defined(_WIN32)
	HANDLE hProcess = GetCurrentProcess();
	PROCESS_MEMORY_COUNTERS pmc;
	pmc.cb = sizeof(pmc);
	if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
		return static_cast<long>(pmc.PeakWorkingSetSize / 1024);
	return -1;
#else
	return -1;
#endif
}

namespace detail {

struct cpu_clock_tag {
	using time_point = clock_t;
	static time_point now() { return clock(); }
	static double ms_between(time_point a, time_point b) {
		return (double(b - a) / CLOCKS_PER_SEC) * 1000;
	}
};

struct steady_clock_tag {
	using time_point = std::chrono::steady_clock::time_point;
	static time_point now() { return std::chrono::steady_clock::now(); }
	static double ms_between(time_point a, time_point b) {
		return std::chrono::duration<double, std::milli>(b - a).count();
	}
};

} // namespace detail

/// Elapsed-time accumulator.
///  `basic_timer<cpu_clock_tag>` measures process CPU time via clock();
///  `basic_timer<steady_clock_tag>` measures wall time via std::chrono::steady_clock.
/// The two are interchangeable at the call site: start/pause/unpause/restart/stop/get all return milliseconds.
template <typename ClockTag = detail::cpu_clock_tag>
struct basic_timer {

	using time_point = typename ClockTag::time_point;

	time_point start_time_{};
	bool started_ = false;
	bool silent_ = false;
	double ms_ = 0;

	basic_timer(bool silent = false) : silent_(silent) { }

	void start() {
		if (!started_) started_ = true, start_time_ = ClockTag::now();
	}

	double restart() {
		if (!started_) return start(), 0;
		double ms = pause();
		start_time_ = ClockTag::now();
		return ms;
	}

	double pause() {
		if (!started_) return 0;
		double ms = ClockTag::ms_between(start_time_, ClockTag::now());
		ms_ += ms;
		return ms;
	}

	double unpause() {
		if (!started_) return start(), 0;
		return start_time_ = ClockTag::now(), 0;
	}

	double stop() {
		if (!started_) return 0;
		pause();
		return started_ = false, ms_;
	}

	double get() {
		if (!started_) return 0;
		return pause();
	}

	~basic_timer() { stop(); }
};

using timer        = basic_timer<detail::cpu_clock_tag>;
using steady_timer = basic_timer<detail::steady_clock_tag>;

static std::map<std::string, timer> timers;
static std::map<std::string, size_t> counters;

template<typename node_t>
using rule = std::pair<node_t, node_t>;

template<typename node_t>
static std::map<rule<node_t>, size_t> rule_counters;

template<typename node_t>
static std::map<rule<node_t>, size_t> rule_hits;

[[maybe_unused]]
static void start_timer(const std::string& name, bool silent = false) {
	if (timers.find(name) == timers.end()) timers[name] = timer(silent);
	timers[name].start();
}

[[maybe_unused]]
static void restart_timer(const std::string& name) {
	if (timers.find(name) != timers.end()) timers[name].restart();
}

[[maybe_unused]]
static void pause_timer(const std::string& name) {
	if (timers.find(name) != timers.end()) timers[name].pause();
}

[[maybe_unused]]
static void unpause_timer(const std::string& name) {
	if (timers.find(name) != timers.end()) timers[name].unpause();
}

[[maybe_unused]]
static double get_timer(const std::string& name) {
	return (timers.find(name) != timers.end()) ? timers[name].get() : 0;
}

[[maybe_unused]]
static void stop_timer(const std::string& name) {
	if (timers.find(name) != timers.end()) timers[name].stop();
}

[[maybe_unused]]
static void print_timer(const std::string& name) {
	if (timers.find(name) != timers.end() && !timers[name].silent_)
		std::cout << std::fixed << std::setprecision(2)
			<< name << " time: " << timers[name].get() << " ms\n";
}

[[maybe_unused]]
static void remove_timer(const std::string& name) {
	if (timers.find(name) != timers.end()) timers.erase(name);
}

[[maybe_unused]]
static void remove_all_timers() { timers.clear(); }

[[maybe_unused]]
static size_t increase_counter(const std::string& name) {
	if (counters.find(name) == counters.end()) counters[name] = 0;
	return ++counters[name];
}

[[maybe_unused]]
static size_t get_counter(const std::string& name) {
	return (counters.find(name) == counters.end()) ? counters[name] = 0 : counters[name];
}

[[maybe_unused]]
static void remove_counter(const std::string& name) {
	if (counters.find(name) != counters.end()) counters.erase(name);
}

[[maybe_unused]]
static void remove_all_counters() { counters.clear(); }

template<typename node_t>
static size_t increase_rule_counter(const rule<node_t>& r) {
	if (rule_counters<node_t>.find(r) == rule_counters<node_t>.end()) rule_counters<node_t>[r] = 0;
	return ++rule_counters<node_t>[r];
}

template<typename node_t>
static size_t get_rule_counter(const rule<node_t>& r) {
	return (rule_counters<node_t>.find(r) == rule_counters<node_t>.end())
		? rule_counters<node_t>[r] = 0 : rule_counters<node_t>[r];
}

template<typename node_t>
static void remove_rule_counter(const rule<node_t>& r) {
	if (rule_counters<node_t>.find(r) != rule_counters<node_t>.end()) rule_counters<node_t>.erase(r);
}

template<typename node_t>
static void remove_all_rule_counters() { rule_counters<node_t>.clear(); }

template<typename node_t>
static size_t increase_rule_hit(const rule<node_t>& r) {
	if (rule_hits<node_t>.find(r) == rule_hits<node_t>.end()) rule_hits<node_t>[r] = 0;
	return ++rule_hits<node_t>[r];
}

template<typename node_t>
static size_t get_rule_hit(const rule<node_t>& r) {
	return (rule_hits<node_t>.find(r) == rule_hits<node_t>.end())
		? rule_hits<node_t>[r] = 0 : rule_hits<node_t>[r];
}

template<typename node_t>
static void remove_rule_hit(const rule<node_t>& r) {
	if (rule_hits<node_t>.find(r) != rule_hits<node_t>.end()) rule_hits<node_t>.erase(r);
}

template<typename node_t>
static void remove_all_rule_hits() { rule_hits<node_t>.clear(); }


template<typename node_t>
static void remove_all() {
	remove_all_timers();
	remove_all_counters();
	remove_all_rule_counters<node_t>();
	remove_all_rule_hits<node_t>();
}

} // namespace idni::measures
#endif // __IDNI__PARSER__UTILITY__MEASURE_H__
