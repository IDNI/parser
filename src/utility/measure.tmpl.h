// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include "measure.h"

namespace idni::measures {

// ---- current_rss_kb -------------------------------------------------------

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

// ---- peak_rss_kb ----------------------------------------------------------

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

// ---- detail clock tags ----------------------------------------------------

inline auto cpu_clock_tag::now() -> clock_t {
	return clock();
}

inline auto cpu_clock_tag::ms_between(clock_t a, clock_t b) -> double {
	return (double(b - a) / CLOCKS_PER_SEC) * 1000;
}

inline auto steady_clock_tag::now() -> std::chrono::steady_clock::time_point {
	return std::chrono::steady_clock::now();
}

inline auto steady_clock_tag::ms_between(
	std::chrono::steady_clock::time_point a,
	std::chrono::steady_clock::time_point b) -> double
{
	return std::chrono::duration<double, std::milli>(b - a).count();
}

// ---- basic_timer ----------------------------------------------------------

template <typename ClockTag>
basic_timer<ClockTag>::basic_timer(bool silent) : silent_(silent) { }

template <typename ClockTag>
void basic_timer<ClockTag>::start() {
	if (!started_) started_ = true, start_time_ = ClockTag::now();
}

template <typename ClockTag>
double basic_timer<ClockTag>::restart() {
	if (!started_) return start(), 0;
	double ms = pause();
	start_time_ = ClockTag::now();
	return ms;
}

template <typename ClockTag>
double basic_timer<ClockTag>::pause() {
	if (!started_) return 0;
	double ms = ClockTag::ms_between(start_time_, ClockTag::now());
	ms_ += ms;
	return ms;
}

template <typename ClockTag>
double basic_timer<ClockTag>::unpause() {
	if (!started_) return start(), 0;
	return start_time_ = ClockTag::now(), 0;
}

template <typename ClockTag>
double basic_timer<ClockTag>::stop() {
	if (!started_) return 0;
	pause();
	return started_ = false, ms_;
}

template <typename ClockTag>
double basic_timer<ClockTag>::get() {
	if (!started_) return 0;
	return pause();
}

template <typename ClockTag>
basic_timer<ClockTag>::~basic_timer() { stop(); }

// ---- timer helpers --------------------------------------------------------

[[maybe_unused]]
static void start_timer(const std::string& name, bool silent) {
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

// ---- counter helpers ------------------------------------------------------

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

// ---- rule counter helpers -------------------------------------------------

template<typename node_t>
[[maybe_unused]]
static size_t increase_rule_counter(const rule<node_t>& r) {
	if (rule_counters<node_t>.find(r) == rule_counters<node_t>.end()) rule_counters<node_t>[r] = 0;
	return ++rule_counters<node_t>[r];
}

template<typename node_t>
[[maybe_unused]]
static size_t get_rule_counter(const rule<node_t>& r) {
	return (rule_counters<node_t>.find(r) == rule_counters<node_t>.end())
		? rule_counters<node_t>[r] = 0 : rule_counters<node_t>[r];
}

template<typename node_t>
[[maybe_unused]]
static void remove_rule_counter(const rule<node_t>& r) {
	if (rule_counters<node_t>.find(r) != rule_counters<node_t>.end()) rule_counters<node_t>.erase(r);
}

template<typename node_t>
[[maybe_unused]]
static void remove_all_rule_counters() { rule_counters<node_t>.clear(); }

// ---- rule hit helpers -----------------------------------------------------

template<typename node_t>
[[maybe_unused]]
static size_t increase_rule_hit(const rule<node_t>& r) {
	if (rule_hits<node_t>.find(r) == rule_hits<node_t>.end()) rule_hits<node_t>[r] = 0;
	return ++rule_hits<node_t>[r];
}

template<typename node_t>
[[maybe_unused]]
static size_t get_rule_hit(const rule<node_t>& r) {
	return (rule_hits<node_t>.find(r) == rule_hits<node_t>.end())
		? rule_hits<node_t>[r] = 0 : rule_hits<node_t>[r];
}

template<typename node_t>
[[maybe_unused]]
static void remove_rule_hit(const rule<node_t>& r) {
	if (rule_hits<node_t>.find(r) != rule_hits<node_t>.end()) rule_hits<node_t>.erase(r);
}

template<typename node_t>
[[maybe_unused]]
static void remove_all_rule_hits() { rule_hits<node_t>.clear(); }

// ---- remove_all -----------------------------------------------------------

template<typename node_t>
[[maybe_unused]]
static void remove_all() {
	remove_all_timers();
	remove_all_counters();
	remove_all_rule_counters<node_t>();
	remove_all_rule_hits<node_t>();
}

} // namespace idni::measures
