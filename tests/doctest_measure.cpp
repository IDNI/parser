// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Unit tests for the measurement facility (namespace idni::measures).
//
// Tests only the timer / counter / rule-counter / rule-hit registries and
// the RSS helpers. Each TU including measure.h gets its own static 
// `timers`/`counters` maps; tests reset the relevant registries on entry
// so case order is irrelevant.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "utility/measure.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>

using namespace idni::measures;

// Deterministic clock for testing basic_timer's state machine without
// depending on OS scheduling or wall-clock sleeps.
struct fake_clock_tag {
	using time_point = double;

	static inline double now_ms = 0.0;

	static time_point now() { return now_ms; }
	static double ms_between(time_point a, time_point b) { return b - a; }
};

using fake_timer = basic_timer<fake_clock_tag>;

void reset_fake_clock(double now = 0.0) { fake_clock_tag::now_ms = now; }
void advance_fake_clock(double delta_ms) { fake_clock_tag::now_ms += delta_ms; }

void reset_all_local() {
	remove_all_timers();
	remove_all_counters();
	remove_all_rule_counters<int>();
	remove_all_rule_hits<int>();
}

// Busy-loop helpers for the cpu_clock_tag-backed registry. These tests assert
// positive CPU time, so the loop runs until clock() itself has advanced enough
// to avoid depending on a particular platform's clock resolution.
std::atomic_size_t cpu_burn_sink{0};

void burn_cpu_slice() {
	for (size_t i = 0; i < 100'000; ++i)
		cpu_burn_sink.fetch_add(i, std::memory_order_relaxed);
}

// Assumes clock() advances (CPU clock available); spins until it does.
void burn_cpu_for_at_least(double min_ms = 3.0) {
	auto start = cpu_clock_tag::now();
	do burn_cpu_slice();
	while (cpu_clock_tag::ms_between(start, cpu_clock_tag::now()) < min_ms);
}

// ---------------------------------------------------------------------------
// basic_timer state machine (deterministic fake clock)
// ---------------------------------------------------------------------------

TEST_SUITE("measure: basic_timer state machine") {

	TEST_CASE("default-constructed: get() returns 0") {
		reset_fake_clock();
		fake_timer t(true);
		CHECK(t.get() == doctest::Approx(0.0));
	}

	TEST_CASE("start + advancing clock: get() returns elapsed ms") {
		reset_fake_clock();
		fake_timer t(true);
		t.start();
		advance_fake_clock(20.0);
		CHECK(t.get() == doctest::Approx(20.0));
	}

	TEST_CASE("double-start is a no-op (start_time_ preserved)") {
		reset_fake_clock();
		fake_timer t(true);
		t.start();
		advance_fake_clock(20.0);
		t.start();
		advance_fake_clock(5.0);
		CHECK(t.get() == doctest::Approx(25.0));
	}

	TEST_CASE("stop returns total; subsequent get()/stop() return same total") {
		reset_fake_clock();
		fake_timer t(true);
		t.start();
		advance_fake_clock(12.0);
		double total = t.stop();
		CHECK(total == doctest::Approx(12.0));
		advance_fake_clock(100.0);
		CHECK(t.get() == doctest::Approx(total));
		CHECK(t.stop() == doctest::Approx(total));
	}

	TEST_CASE("restart on never-started timer starts a fresh run and returns 0") {
		reset_fake_clock();
		fake_timer t(true);
		CHECK(t.restart() == doctest::Approx(0.0));
		advance_fake_clock(7.0);
		CHECK(t.get() == doctest::Approx(7.0));
	}

	TEST_CASE("restart on previously-stopped timer returns frozen total and resets") {
		reset_fake_clock();
		fake_timer t(true);
		t.start();
		advance_fake_clock(10.0);
		CHECK(t.stop() == doctest::Approx(10.0));
		advance_fake_clock(50.0);
		CHECK(t.restart() == doctest::Approx(10.0));
		CHECK(t.get() == doctest::Approx(0.0));
		advance_fake_clock(5.0);
		CHECK(t.stop() == doctest::Approx(5.0));
	}

	TEST_CASE("restart on running timer returns previous total and resets") {
		reset_fake_clock();
		fake_timer t(true);
		t.start();
		advance_fake_clock(20.0);
		CHECK(t.restart() == doctest::Approx(20.0));
		CHECK(t.get() == doctest::Approx(0.0));
		advance_fake_clock(4.0);
		CHECK(t.stop() == doctest::Approx(4.0));
	}

	TEST_CASE("restart on paused timer returns paused total and resets") {
		reset_fake_clock();
		fake_timer t(true);
		t.start();
		advance_fake_clock(8.0);
		CHECK(t.pause() == doctest::Approx(8.0));
		advance_fake_clock(50.0);
		CHECK(t.restart() == doctest::Approx(8.0));
		CHECK(t.get() == doctest::Approx(0.0));
		advance_fake_clock(5.0);
		CHECK(t.stop() == doctest::Approx(5.0));
	}

	TEST_CASE("unpause on unstarted timer starts the timer") {
		reset_fake_clock();
		fake_timer t(true);
		t.unpause();
		advance_fake_clock(9.0);
		CHECK(t.get() == doctest::Approx(9.0));
	}

	TEST_CASE("pause on unstarted timer returns 0 and does not start") {
		reset_fake_clock();
		fake_timer t(true);
		CHECK(t.pause() == doctest::Approx(0.0));
		advance_fake_clock(9.0);
		CHECK(t.get() == doctest::Approx(0.0));
	}

	TEST_CASE("pause is idempotent; second pause does not advance ms_") {
		reset_fake_clock();
		fake_timer t(true);
		t.start();
		advance_fake_clock(10.0);
		double p1 = t.pause();
		advance_fake_clock(50.0);
		double p2 = t.pause();
		CHECK(p1 == doctest::Approx(10.0));
		CHECK(p2 == doctest::Approx(p1));
		CHECK(t.get() == doctest::Approx(p1));
	}

	TEST_CASE("pause / unpause cycle accumulates disjoint intervals") {
		reset_fake_clock();
		fake_timer t(true);
		t.start();
		advance_fake_clock(10.0);
		CHECK(t.pause() == doctest::Approx(10.0));
		advance_fake_clock(100.0);
		CHECK(t.unpause() == doctest::Approx(0.0));
		advance_fake_clock(15.0);
		CHECK(t.stop() == doctest::Approx(25.0));
	}

	TEST_CASE("stop after pause returns the paused total (no overcount)") {
		reset_fake_clock();
		fake_timer t(true);
		t.start();
		advance_fake_clock(10.0);
		double p = t.pause();
		advance_fake_clock(50.0);
		CHECK(t.stop() == doctest::Approx(p));
		CHECK(t.get() == doctest::Approx(p));
	}

	TEST_CASE("get() is non-mutating; repeated reads do not double-count") {
		reset_fake_clock();
		fake_timer t(true);
		t.start();
		advance_fake_clock(10.0);
		double g1 = t.get();
		double g2 = t.get();
		advance_fake_clock(5.0);
		double g3 = t.get();
		CHECK(g1 == doctest::Approx(10.0));
		CHECK(g2 == doctest::Approx(10.0));
		CHECK(g3 == doctest::Approx(15.0));
		CHECK(t.stop() == doctest::Approx(15.0));
	}

	TEST_CASE("unpause while running is a no-op") {
		reset_fake_clock();
		fake_timer t(true);
		t.start();
		advance_fake_clock(10.0);
		CHECK(t.unpause() == doctest::Approx(0.0));
		advance_fake_clock(5.0);
		CHECK(t.stop() == doctest::Approx(15.0));
	}

	TEST_CASE("start after stop resets accumulation") {
		reset_fake_clock();
		fake_timer t(true);
		t.start();
		advance_fake_clock(10.0);
		CHECK(t.stop() == doctest::Approx(10.0));
		advance_fake_clock(50.0);
		t.start();
		CHECK(t.get() == doctest::Approx(0.0));
		advance_fake_clock(4.0);
		CHECK(t.stop() == doctest::Approx(4.0));
	}

	TEST_CASE("stop on unstarted timer returns 0 and does not start") {
		reset_fake_clock();
		fake_timer t(true);
		CHECK(t.stop() == doctest::Approx(0.0));
		advance_fake_clock(10.0);
		CHECK(t.get() == doctest::Approx(0.0));
	}

	TEST_CASE("start() while paused is a no-op; does not reset ms_") {
		reset_fake_clock();
		fake_timer t(true);
		t.start();
		advance_fake_clock(12.0);
		t.pause();
		double paused_total = t.get();
		advance_fake_clock(50.0);
		t.start();                      // should be a no-op
		CHECK(t.get() == doctest::Approx(paused_total));
		// unpause and verify we continue from the paused total
		t.unpause();
		advance_fake_clock(8.0);
		CHECK(t.stop() == doctest::Approx(paused_total + 8.0));
	}

	TEST_CASE("pause() after stop() returns frozen total") {
		reset_fake_clock();
		fake_timer t(true);
		t.start();
		advance_fake_clock(7.0);
		double total = t.stop();
		advance_fake_clock(100.0);
		CHECK(t.pause() == doctest::Approx(total));
		CHECK(t.get()   == doctest::Approx(total));
	}

	TEST_CASE("unpause() after stop() starts fresh, losing previous ms_") {
		reset_fake_clock();
		fake_timer t(true);
		t.start();
		advance_fake_clock(10.0);
		CHECK(t.stop() == doctest::Approx(10.0));
		advance_fake_clock(50.0);
		t.unpause();                    // calls start(), resets ms_
		CHECK(t.get() == doctest::Approx(0.0));
		advance_fake_clock(4.0);
		CHECK(t.stop() == doctest::Approx(4.0));
	}

	TEST_CASE("multiple pause/unpause cycles accumulate disjoint intervals") {
		reset_fake_clock();
		fake_timer t(true);
		t.start();
		advance_fake_clock(10.0);
		CHECK(t.pause()   == doctest::Approx(10.0));
		t.unpause();
		advance_fake_clock(15.0);
		CHECK(t.pause()   == doctest::Approx(25.0));
		t.unpause();
		advance_fake_clock(5.0);
		CHECK(t.pause()   == doctest::Approx(30.0));
		t.unpause();
		advance_fake_clock(20.0);
		CHECK(t.stop()    == doctest::Approx(50.0));
	}

	TEST_CASE("chained restart() → restart() without clock advance returns 0") {
		reset_fake_clock();
		fake_timer t(true);
		t.start();
		advance_fake_clock(10.0);
		CHECK(t.restart() == doctest::Approx(10.0));
		// no clock advance; second restart should return 0
		CHECK(t.restart() == doctest::Approx(0.0));
		CHECK(t.get()     == doctest::Approx(0.0));
		advance_fake_clock(3.0);
		CHECK(t.stop()    == doctest::Approx(3.0));
	}
}

// ---------------------------------------------------------------------------
// basic_timer<cpu_clock_tag>
// ---------------------------------------------------------------------------

TEST_SUITE("measure: basic_timer<cpu>") {

	TEST_CASE("cpu time advances across busy work") {
		timer t(true);
		t.start();
		burn_cpu_for_at_least();
		CHECK(t.get() > 0.0);
	}

	TEST_CASE("pause excludes CPU work until unpause") {
		timer t(true);
		t.start();
		burn_cpu_for_at_least();
		double paused = t.pause();
		CHECK(paused > 0.0);
		burn_cpu_for_at_least();
		CHECK(t.get() == doctest::Approx(paused));
		t.unpause();
		burn_cpu_for_at_least();
		CHECK(t.stop() > paused);
	}
}

// ---------------------------------------------------------------------------
// basic_timer<steady_clock_tag>
// ---------------------------------------------------------------------------

// Sanity-checks the std::chrono::steady_clock bridge. The fake-clock suite
// already proves the state machine; these tests only verify that the steady
// clock tag itself measures forward-moving wall time.
TEST_SUITE("measure: basic_timer<steady>") {

	TEST_CASE("steady clock advances across sleep") {
		steady_timer t(true);
		t.start();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
		// sleep_for is at-least; allow generous slack for scheduling jitter.
		CHECK(t.get() >= 4.0);
	}

	TEST_CASE("pause excludes wall time until unpause") {
		steady_timer t(true);
		t.start();
		std::this_thread::sleep_for(std::chrono::milliseconds(3));
		double paused = t.pause();
		CHECK(paused >= 2.0);
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		CHECK(t.get() == doctest::Approx(paused));
		t.unpause();
		std::this_thread::sleep_for(std::chrono::milliseconds(3));
		double total = t.stop();
		CHECK(total > paused);
	}
}

// ---------------------------------------------------------------------------
// named timer registry
// ---------------------------------------------------------------------------

// Named-timer registry uses basic_timer<cpu_clock_tag>; tests burn enough CPU
// to assert positive elapsed time without relying on wall-clock sleeps.
TEST_SUITE("measure: named timers") {

	TEST_CASE("get_timer on absent name returns 0 without creating a timer") {
		reset_all_local();
		CHECK(get_timer("missing") == doctest::Approx(0.0));
		CHECK(timers.empty());
	}

	TEST_CASE("start_timer + get_timer returns positive elapsed CPU time") {
		reset_all_local();
		start_timer("a", true);
		burn_cpu_for_at_least();
		CHECK(get_timer("a") > 0.0);
	}

	TEST_CASE("start_timer on existing name does not reset the timer") {
		reset_all_local();
		start_timer("a", true);
		burn_cpu_for_at_least();
		double before = get_timer("a");
		CHECK(before > 0.0);
		start_timer("a", true);
		CHECK(get_timer("a") >= before);
	}

	TEST_CASE("stop_timer freezes get_timer at the final value") {
		reset_all_local();
		start_timer("a", true);
		burn_cpu_for_at_least();
		stop_timer("a");
		double frozen = get_timer("a");
		CHECK(frozen > 0.0);
		burn_cpu_for_at_least();
		CHECK(get_timer("a") == doctest::Approx(frozen));
	}

	TEST_CASE("pause_timer freezes elapsed time until unpause_timer") {
		reset_all_local();
		start_timer("a", true);
		burn_cpu_for_at_least();
		pause_timer("a");
		double paused = get_timer("a");
		CHECK(paused > 0.0);
		burn_cpu_for_at_least();
		CHECK(get_timer("a") == doctest::Approx(paused));
		unpause_timer("a");
		burn_cpu_for_at_least();
		CHECK(get_timer("a") > paused);
	}

	TEST_CASE("named timers are independent") {
		reset_all_local();
		start_timer("a", true);
		burn_cpu_for_at_least();
		start_timer("b", true);
		burn_cpu_for_at_least();
		double a = get_timer("a");
		double b = get_timer("b");
		CHECK(a > 0.0);
		CHECK(b > 0.0);
		CHECK(a >= b);
	}

	TEST_CASE("remove_timer clears that timer only") {
		reset_all_local();
		start_timer("a", true);
		start_timer("b", true);
		burn_cpu_for_at_least();
		pause_timer("b");
		double b = get_timer("b");
		CHECK(b > 0.0);
		remove_timer("a");
		CHECK(get_timer("a") == doctest::Approx(0.0));
		CHECK(get_timer("b") == doctest::Approx(b));
	}

	TEST_CASE("remove_all_timers clears every named timer") {
		reset_all_local();
		start_timer("a", true);
		start_timer("b", true);
		burn_cpu_for_at_least();
		remove_all_timers();
		CHECK(get_timer("a") == doctest::Approx(0.0));
		CHECK(get_timer("b") == doctest::Approx(0.0));
	}

	TEST_CASE("restart_timer resets elapsed time and keeps timer running") {
		reset_all_local();
		start_timer("a", true);
		burn_cpu_for_at_least(10.0);
		double before = get_timer("a");
		CHECK(before > 0.0);
		restart_timer("a");
		double after_restart = get_timer("a");
		CHECK(after_restart < before);
		burn_cpu_for_at_least();
		CHECK(get_timer("a") > after_restart);
	}

	TEST_CASE("timer operations on absent names do not create timers") {
		reset_all_local();
		pause_timer("missing");
		unpause_timer("missing");
		restart_timer("missing");
		stop_timer("missing");
		remove_timer("missing");
		CHECK(timers.empty());
		CHECK(get_timer("missing") == doctest::Approx(0.0));
		CHECK(timers.empty());
	}
}

// ---------------------------------------------------------------------------
// counter registry
// ---------------------------------------------------------------------------

TEST_SUITE("measure: counters") {

	TEST_CASE("increase_counter returns running total") {
		reset_all_local();
		CHECK(increase_counter("x") == 1u);
		CHECK(increase_counter("x") == 2u);
		CHECK(increase_counter("x") == 3u);
	}

	TEST_CASE("get_counter on absent returns 0 without creating an entry") {
		reset_all_local();
		CHECK(get_counter("missing") == 0u);
		CHECK(counters.empty());
	}

	TEST_CASE("get_counter reflects current value") {
		reset_all_local();
		increase_counter("x");
		increase_counter("x");
		CHECK(get_counter("x") == 2u);
	}

	TEST_CASE("counters are independent") {
		reset_all_local();
		increase_counter("x");
		increase_counter("y");
		increase_counter("y");
		CHECK(get_counter("x") == 1u);
		CHECK(get_counter("y") == 2u);
	}

	TEST_CASE("remove_counter clears one") {
		reset_all_local();
		increase_counter("x");
		increase_counter("y");
		remove_counter("x");
		CHECK(get_counter("x") == 0u);
		CHECK(get_counter("y") == 1u);
	}

	TEST_CASE("remove_all_counters clears every counter") {
		reset_all_local();
		increase_counter("x");
		increase_counter("y");
		remove_all_counters();
		CHECK(get_counter("x") == 0u);
		CHECK(get_counter("y") == 0u);
	}
}

// ---------------------------------------------------------------------------
// rule_counter<node_t>
// ---------------------------------------------------------------------------

TEST_SUITE("measure: rule_counter<int>") {

	using rint = rule<int>;

	TEST_CASE("increase + get") {
		remove_all_rule_counters<int>();
		rint r{1, 2};
		CHECK(increase_rule_counter(r) == 1u);
		CHECK(increase_rule_counter(r) == 2u);
		CHECK(get_rule_counter(r) == 2u);
	}

	TEST_CASE("get on absent returns 0 without creating an entry") {
		remove_all_rule_counters<int>();
		CHECK(get_rule_counter(rint{99, 100}) == 0u);
		CHECK(rule_counters<int>.empty());
	}

	TEST_CASE("different rules are independent") {
		remove_all_rule_counters<int>();
		increase_rule_counter(rint{1, 2});
		increase_rule_counter(rint{3, 4});
		increase_rule_counter(rint{3, 4});
		CHECK(get_rule_counter(rint{1, 2}) == 1u);
		CHECK(get_rule_counter(rint{3, 4}) == 2u);
	}

	TEST_CASE("remove_rule_counter clears one") {
		remove_all_rule_counters<int>();
		increase_rule_counter(rint{1, 2});
		increase_rule_counter(rint{3, 4});
		remove_rule_counter(rint{1, 2});
		CHECK(get_rule_counter(rint{1, 2}) == 0u);
		CHECK(get_rule_counter(rint{3, 4}) == 1u);
	}

	TEST_CASE("remove_all_rule_counters clears every entry") {
		remove_all_rule_counters<int>();
		increase_rule_counter(rint{1, 2});
		increase_rule_counter(rint{3, 4});
		remove_all_rule_counters<int>();
		CHECK(get_rule_counter(rint{1, 2}) == 0u);
		CHECK(get_rule_counter(rint{3, 4}) == 0u);
	}
}

// ---------------------------------------------------------------------------
// rule_hit<node_t>
// ---------------------------------------------------------------------------

TEST_SUITE("measure: rule_hit<int>") {

	using rint = rule<int>;

	TEST_CASE("increase + get") {
		remove_all_rule_hits<int>();
		rint r{1, 2};
		CHECK(increase_rule_hit(r) == 1u);
		CHECK(increase_rule_hit(r) == 2u);
		CHECK(get_rule_hit(r) == 2u);
	}

	TEST_CASE("get on absent returns 0 without creating an entry") {
		remove_all_rule_hits<int>();
		CHECK(get_rule_hit(rint{99, 100}) == 0u);
		CHECK(rule_hits<int>.empty());
	}

	TEST_CASE("rule_hit registry is separate from rule_counter") {
		remove_all_rule_counters<int>();
		remove_all_rule_hits<int>();
		rint r{1, 2};
		increase_rule_counter(r);
		increase_rule_counter(r);
		increase_rule_hit(r);
		CHECK(get_rule_counter(r) == 2u);
		CHECK(get_rule_hit(r) == 1u);
	}

	TEST_CASE("remove_rule_hit clears one") {
		remove_all_rule_hits<int>();
		increase_rule_hit(rint{1, 2});
		increase_rule_hit(rint{3, 4});
		remove_rule_hit(rint{1, 2});
		CHECK(get_rule_hit(rint{1, 2}) == 0u);
		CHECK(get_rule_hit(rint{3, 4}) == 1u);
	}

	TEST_CASE("remove_all_rule_hits clears every entry") {
		remove_all_rule_hits<int>();
		increase_rule_hit(rint{1, 2});
		increase_rule_hit(rint{3, 4});
		remove_all_rule_hits<int>();
		CHECK(get_rule_hit(rint{1, 2}) == 0u);
		CHECK(get_rule_hit(rint{3, 4}) == 0u);
	}
}

// ---------------------------------------------------------------------------
// remove_all<node_t>
// ---------------------------------------------------------------------------

TEST_SUITE("measure: remove_all<int>") {

	TEST_CASE("clears timers, counters, rule_counters, rule_hits") {
		reset_all_local();
		start_timer("t", true);
		increase_counter("c");
		increase_rule_counter(rule<int>{1, 2});
		increase_rule_hit(rule<int>{1, 2});
		remove_all<int>();
		CHECK(get_timer("t") == doctest::Approx(0.0));
		CHECK(get_counter("c") == 0u);
		CHECK(get_rule_counter(rule<int>{1, 2}) == 0u);
		CHECK(get_rule_hit(rule<int>{1, 2}) == 0u);
	}
}

// ---------------------------------------------------------------------------
// RSS readers
// ---------------------------------------------------------------------------

TEST_SUITE("measure: RSS") {

	TEST_CASE("current_rss_kb reports supported/unsupported platforms honestly") {
		size_t cur = current_rss_kb();
#if defined(__linux__) || defined(__APPLE__) || defined(_WIN32)
		CHECK(cur != static_cast<size_t>(-1));
		CHECK(cur > 0u);
#else
		CHECK(cur == static_cast<size_t>(-1));
#endif
	}

	TEST_CASE("peak_rss_kb reports supported/unsupported platforms honestly") {
		size_t peak = peak_rss_kb();
#if defined(__linux__) || defined(__APPLE__) || defined(_WIN32)
		CHECK(peak != static_cast<size_t>(-1));
		CHECK(peak > 0u);
#else
		CHECK(peak == static_cast<size_t>(-1));
#endif
	}
}
