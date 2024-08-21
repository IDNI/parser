// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.
#ifndef __IDNI__PARSER__MEASURE_H__
#define __IDNI__PARSER__MEASURE_H__
#include <string>
#include <iostream>
#include <iomanip>

namespace idni::measures {

struct timer {

	clock_t start_time_;
	bool started_ = false;
	bool silent_ = false;
	double ms_ = 0;

	timer(bool silent = false) : silent_(silent) { }

	void start() { if (!started_) started_ = true, start_time_ = clock(); }

	double restart() {
		if (!started_) return start(), 0;
		double ms = pause();

		return ms;
	}

	double pause() {
		if (!started_) return 0;
		double ms = ((double(clock() - start_time_)
			/ CLOCKS_PER_SEC) * 1000);
		ms_ += ms;
		return ms;
	}

	double unpause() {
		if (!started_) return start(), 0;
		return start_time_ = clock(), 0;
	}

	double stop() {
		if (!started_) return 0;
		pause();
		return started_ = false, ms_;
	}

	double get() { return pause(); }

	~timer() { stop(); }
};

static std::map<std::string, timer> timers;
static std::map<std::string, size_t> counters;

static void start_timer(const std::string& name, bool silent = false) {
	if (timers.find(name) == timers.end()) timers[name] = timer(silent);
	else timers[name].start();
}

static void restart_timer(const std::string& name) {
	if (timers.find(name) == timers.end()) return;
	timers[name].restart();
}

static void pause_timer(const std::string& name) {
	if (timers.find(name) == timers.end()) return;
	timers[name].pause();
}

static void unpause_timer(const std::string& name) {
	if (timers.find(name) == timers.end()) return;
	timers[name].unpause();
}

static double get_timer(const std::string& name) {
	if (timers.find(name) == timers.end()) return 0;
	return timers[name].get();
}

static void stop_timer(const std::string& name) {
	if (timers.find(name) == timers.end()) return;
	timers[name].stop();
}

static void print_timer(const std::string& name) {
	if (timers.find(name) == timers.end()) return;
	if (!timers[name].silent_) return;
	std::cout << std::fixed << std::setprecision(2)
		<< name << " time: " << timers[name].get() << " ms\n";
}

static void remove_timer(const std::string& name) {
	if (timers.find(name) == timers.end()) return;
	timers.erase(name);
}

static void remove_all_timers() { timers.clear(); }

static size_t increase_counter(const std::string& name) {
	if (counters.find(name) == counters.end()) counters[name] = 0;
	return ++counters[name];
}

static size_t get_counter(const std::string& name) {
	if (counters.find(name) == counters.end()) counters[name] = 0;
	return counters[name];
}

static void remove_counter(const std::string& name) {
	if (counters.find(name) == counters.end()) return;
	counters.erase(name);
}

static void remove_all_counters() { counters.clear(); }

} // namespace idni::measures
#endif // __IDNI__PARSER__MEASURE_H__