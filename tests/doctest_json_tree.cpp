// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "format/json/json.h"

#include <bit>
#include <string>

using namespace idni;
using namespace idni::format::json;

using jt = idni::format::json::tree;

TEST_SUITE("json tree: nodes") {
	TEST_CASE("equal nodes give one tree, and each field tells nodes apart") {
		tref a = jt::get(node(json_parser::str, 0, "x"));
		CHECK(jt::get(node(json_parser::str, 0, "x")) == a);
		CHECK(jt::get(node(json_parser::str, 0, "y")) != a);
		CHECK(jt::get(node(json_parser::str, 1, "x")) != a);
		CHECK(jt::get(node(json_parser::object_pair, 0, "x")) != a);
		CHECK(jt::get(json_parser::null_sym)
			== jt::get(node(json_parser::null_sym)));
		CHECK(jt::m_size() > 0);
		CHECK(node::name(json_parser::str) == "str");
		CHECK(jt::get(a).value.name() == "str");
	}

	TEST_CASE("the children and the fast access of each kind") {
		tref one = jt::get(node(json_parser::number,
			std::bit_cast<uint64_t>(1.5)));
		tref yes = jt::get(json_parser::true_sym);
		tref arr = jt::get(json_parser::arr, { one, yes });
		const jt& a = jt::get(arr);
		CHECK(a.is_array());
		CHECK(a.get_type() == json_parser::arr);
		size_t n = 0;
		for (const jt& c : a.children_trees())
			n += c.is_number() || c.is_bool();
		CHECK(n == 2);
		// a child with a right sibling is another tree than its value alone
		CHECK(a.first_tree().value == jt::get(one).value);
		CHECK(a.second_tree().get() == yes);
		CHECK(a[1].get() == yes);
		CHECK(a.child_tree(0).is_number());
		CHECK(a.child_tree(0).get_number() == 1.5);
		CHECK(a.second_tree().is_bool());
		CHECK(a.second_tree().get_bool());
		CHECK(jt::get(jt::get(json_parser::false_sym)).is_bool());
		CHECK_FALSE(jt::get(jt::get(json_parser::false_sym)).get_bool());
		CHECK(jt::get(jt::get(json_parser::null_sym)).is_null());

		tref text = jt::get(node(json_parser::str, 0, "hello"));
		tref pair = jt::get(node(json_parser::object_pair, 0, "a"), text);
		tref obj = jt::get(json_parser::object, { pair });
		const jt& o = jt::get(obj);
		CHECK(o.is_object());
		CHECK(o.only_child_tree().get() == pair);
		CHECK(o.only_child_tree().get_string() == "a");
		CHECK(o.only_child_tree().value_tree().get() == text);
		CHECK(jt::get(text).is_string());
		CHECK(jt::get(text).get_string() == "hello");
		CHECK(o.find("a") == text);
		CHECK(o.find("b") == nullptr);
		CHECK(a.find("a") == nullptr);
		CHECK(jt::get(jt::geth(obj)).get() == obj);
	}
}
