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
#ifndef __IDNI__TESTING_H__
#define __IDNI__TESTING_H__
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

namespace idni { namespace testing {

// current group, name, id of a test and previous test name
std::string test_group = "";
std::string test_name_prev = "";
std::string test_name = "";
std::string test_file = "";
size_t test_line = 0;
size_t test_id = 0;
// group, tests and ids white/black lists
std::vector<std::string> tests_wl, tests_bl, groups_wl, groups_bl;
std::vector<size_t> ids_wl;

// sets group and name of following run_test calls
#define TEST(group, name) { testing::test_file = __FILE__, \
				testing::test_line = __LINE__, \
				testing::test_name = (name), \
				testing::test_group = (group); }

bool failed = false;
bool print_all = false;
bool print_lines = false;
size_t verbosity = 0;
bool run = false;

// selects group/test/id for testing or disables group/test from running
void group(const std::string& s) { groups_wl.push_back(s); }
void disable_group(const std::string& s) { groups_bl.push_back(s); }
void test(const std::string& s) { tests_wl.push_back(s); }
void disable_test(const std::string& s) { tests_bl.push_back(s); }
void id(size_t i) { ids_wl.push_back(i); }

bool check() {
	// check if we have another test_id (prev test name differs to actual)
	if (test_name == test_name_prev) test_id++;
	else test_name_prev = test_name, test_id = 0;
	// check white lists and black lists and decide if we run or skip
	bool g_wl = groups_wl.size(),
		t_wl = tests_wl.size(),
		i_wl = ids_wl.size(),
		in_g_wl = std::find(groups_wl.begin(), groups_wl.end(),
						test_group) != groups_wl.end(),
		in_g_bl = std::find(groups_bl.begin(), groups_bl.end(),
						test_group) != groups_bl.end(),
		in_t_wl = std::find(tests_wl.begin(), tests_wl.end(),
						test_name) != tests_wl.end(),
		in_t_bl = std::find(tests_bl.begin(), tests_bl.end(),
						test_name) != tests_bl.end(),
		in_i_wl = std::find(ids_wl.begin(), ids_wl.end(),
						test_id) != ids_wl.end();
	return run = (!g_wl || in_g_wl) && (!t_wl || in_t_wl) &&
		(!i_wl || in_i_wl) && (!in_g_bl && !in_t_bl);
}

// prints test info (group / name / id) and decides if the test is skipped
bool info(std::ostream& os) {
	if (!run && !print_all) return false;
	os << "\nTEST " << test_group << " / " << test_name << " / "
		<< test_id << " " << (run ? "" : "skipped");
	if (print_lines) os << " (" << test_file << ":" << test_line << ") ";
	return run;
}

} } // namespace idni::testing
#endif // __IDNI__TESTING_H__
