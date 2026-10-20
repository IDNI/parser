// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Unit tests for read_program_args.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"

#include <functional>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
#	define PROGRAM_ARGS_TEST_THREADS 1
#	include <thread>
#endif

#include "../src/utility/program_args.h"
#include "test_env.h"

using namespace std;
using namespace idni;

namespace {

option_set demo_set() {
	return option_set{
		.options = {
			{ "help", "cli", false, "show the help",
				{ { "", 'h' } } },
			{ "version", "cli", false, "print version and exit",
				{ { "" } } },
			{ "license", "cli", false, "print license and exit",
				{ { "" } } },
			{ "count", "run", size_t{ 1 }, "how many times",
				{ { "run", 'n' } } },
		},
		.commands = { { "run", "run the tool" } },
	};
}

args_config demo_config() {
	args_config cfg;
	cfg.name = "demo";
	cfg.description = "a test program";
	return cfg;
}

const program_identity demo{ "demo", "demo 4.5.6", "demo license\n" };

// What one read_program_args call gave on a thread of its own.
struct outcome {
	bool declared = false;
	bool has_value = false;
	bool has_error = false;
	info_flag info = info_flag::none;
	string command;
	size_t count = 0;
	/// What read_program_args wrote on std::cout.
	string read_out;
	/// What print_info wrote on std::cout.
	string info_out;
};

#ifdef PROGRAM_ARGS_TEST_THREADS
// A new thread starts with an empty options repository.
outcome read_on_thread(initializer_list<const char*> args,
	const program_identity& who = demo)
{
	outcome o;
	vector<string> store(args.begin(), args.end());
	thread t([&o, &store, &who] {
		auto& opts = idni::options();
		o.declared = opts.declare(demo_set()).has_value();
		vector<char*> argv;
		for (auto& s : store) argv.push_back(s.data());
		ostringstream read_out, info_out;
		streambuf* old_out = cout.rdbuf(read_out.rdbuf());
		auto r = read_program_args(static_cast<int>(argv.size()),
			argv.data(), who, demo_config());
		cout.rdbuf(info_out.rdbuf());
		if (r.has_value()) print_info(who, r.value());
		cout.rdbuf(old_out);
		o.read_out = read_out.str();
		o.info_out = info_out.str();
		o.has_value = r.has_value();
		o.has_error = r.report().has_error();
		if (r.has_value()) {
			o.info = r.value().info;
			o.command = r.value().args.command;
		}
		o.count = opts.get<size_t>("count");
	});
	t.join();
	return o;
}
#endif

} // namespace

#ifdef PROGRAM_ARGS_TEST_THREADS
TEST_SUITE("read_program_args") {

	TEST_CASE("plain arguments give the parsed command and no info") {
		auto o = read_on_thread({ "demo", "run", "-n", "3" });
		CHECK(o.declared);
		CHECK(o.has_value);
		CHECK(o.info == info_flag::none);
		CHECK(o.command == "run");
		CHECK(o.count == 3u);
		CHECK(o.read_out.empty());
		CHECK(o.info_out.empty());
	}

	TEST_CASE("the environment prefix of the program reads its variable") {
		test_setenv("DEMO_COUNT", "7");
		test_setenv("TGF_COUNT", "9");
		auto o = read_on_thread({ "demo" });
		test_unsetenv("DEMO_COUNT");
		test_unsetenv("TGF_COUNT");
		CHECK(o.has_value);
		CHECK(o.count == 7u);
	}

	TEST_CASE("a name with a dash reads its variables with an underscore") {
		const program_identity dashed{ "my-tool", "my-tool 1.0", "" };
		test_setenv("MY_TOOL_COUNT", "5");
		test_setenv("MY-TOOL_COUNT", "6");
		auto o = read_on_thread({ "demo" }, dashed);
		test_unsetenv("MY_TOOL_COUNT");
		test_unsetenv("MY-TOOL_COUNT");
		CHECK(o.has_value);
		CHECK(o.count == 5u);
	}

	TEST_CASE("read_program_args writes nothing for an info flag") {
		for (const char* flag : { "--help", "--version", "--license" }) {
			CAPTURE(flag);
			auto o = read_on_thread({ "demo", flag });
			CHECK(o.has_value);
			CHECK(o.read_out.empty());
		}
	}

	TEST_CASE("--version: print_info prints the version text") {
		auto o = read_on_thread({ "demo", "--version" });
		CHECK(o.info == info_flag::version);
		CHECK(o.info_out == "demo 4.5.6\n");
	}

	TEST_CASE("--license: a text with a final line end prints as it is") {
		auto o = read_on_thread({ "demo", "--license" });
		CHECK(o.info == info_flag::license);
		CHECK(o.info_out == "demo license\n");
	}

	TEST_CASE("--license: a text without a final line end gets one") {
		const program_identity bare{ "demo", "demo 4.5.6",
			"demo license" };
		auto o = read_on_thread({ "demo", "--license" }, bare);
		CHECK(o.info_out == "demo license\n");
	}

	TEST_CASE("--help: print_info prints the help") {
		auto o = read_on_thread({ "demo", "--help" });
		CHECK(o.info == info_flag::help);
		CHECK(o.info_out.find("a test program") != string::npos);
	}

	TEST_CASE("--version --help: info names the help") {
		auto o = read_on_thread({ "demo", "--version", "--help" });
		CHECK(o.info == info_flag::help);
		CHECK(o.info_out.find("a test program") != string::npos);
		CHECK(o.info_out.find("4.5.6") == string::npos);
	}

	TEST_CASE("--license --help: info names the help") {
		auto o = read_on_thread({ "demo", "--license", "--help" });
		CHECK(o.info == info_flag::help);
		CHECK(o.info_out.find("a test program") != string::npos);
		CHECK(o.info_out.find("demo license") == string::npos);
	}

	TEST_CASE("--license --version: info names the version") {
		auto o = read_on_thread({ "demo", "--license", "--version" });
		CHECK(o.info == info_flag::version);
		CHECK(o.info_out == "demo 4.5.6\n");
	}

	TEST_CASE("a bad value on the command line is an error") {
		auto o = read_on_thread({ "demo", "run", "-n", "many" });
		CHECK_FALSE(o.has_value);
		CHECK(o.has_error);
		CHECK(o.read_out.empty());
	}

	TEST_CASE("a bad value in the environment is an error") {
		test_setenv("DEMO_COUNT", "many");
		auto o = read_on_thread({ "demo", "--version" });
		test_unsetenv("DEMO_COUNT");
		CHECK_FALSE(o.has_value);
		CHECK(o.has_error);
		CHECK(o.read_out.empty());
		CHECK(o.info_out.empty());
	}
}
#endif
