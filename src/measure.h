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
struct measure {
	std::string label_;
	bool silent_ = false;
	clock_t start_time_;
	bool started_ = false;
	double ms_ = 0;
	measure(const std::string& label = "", bool start_measure = false,
		bool silent = false) : label_(label), silent_(silent)
	{
		if (start_measure) start();
	}
	void start() { if (!started_) started_ = true, start_time_ = clock(); }
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
		if (!silent_) std::cout << std::fixed << std::setprecision(2)
			<< label_ << (label_.size() ? "::" : "") << "time: "
			<< ms_ << " ms\n";
		return started_ = false, ms_;
	}
	~measure() { stop(); }
};
#endif // __IDNI__PARSER__MEASURE_H__