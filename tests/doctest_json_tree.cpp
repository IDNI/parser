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

TEST_SUITE("json tree: traverser") {
	// {"a":[1.5,"x"],"b":true,"s":"t"} from nodes only
	static tref sample() {
		tref num = jt::get(node(json_parser::number,
			std::bit_cast<uint64_t>(1.5)));
		tref x = jt::get(node(json_parser::str, 0, "x"));
		tref arr = jt::get(json_parser::arr, { num, x });
		return jt::get(json_parser::object, {
			jt::get(node(json_parser::object_pair, 0, "a"), arr),
			jt::get(node(json_parser::object_pair, 0, "b"),
				jt::get(json_parser::true_sym)),
			jt::get(node(json_parser::object_pair, 0, "s"),
				jt::get(node(json_parser::str, 0, "t"))) });
	}

	TEST_CASE("a traverser holds values and drops a nullptr") {
		tref obj = sample();
		CHECK_FALSE(tt().has_value());
		CHECK(tt().empty());
		CHECK_FALSE(tt(nullptr));
		CHECK(tt(trefs{ obj, nullptr, obj }).size() == 2);
		CHECK(tt(jt::geth(obj)).value() == obj);
		CHECK(tt(jt::get(obj)).value() == obj);
		CHECK(jt::get(obj)().value() == obj);
		CHECK(tt(obj).is(json_parser::object));
		CHECK(tt(obj).value_tree().is_object());
		CHECK(tt(obj)[2].get_string() == "s");
		CHECK(tt(trefs{ obj, obj }).traversers().size() == 2);
		CHECK(tt(trefs{ obj, obj })().size() == 2);
	}

	TEST_CASE("| and || select children, and the extractors read them") {
		tref obj = sample();
		CHECK((tt(obj) | json_parser::object_pair).value_tree()
			.get_string() == "a");
		CHECK((tt(obj) || json_parser::object_pair).size() == 3);
		CHECK_FALSE((tt(obj) | json_parser::arr).has_value());
		CHECK_FALSE((tt() | json_parser::arr).has_value());
		CHECK((tt(obj) | tt::ref) == obj);
		CHECK((tt() | tt::ref) == nullptr);
		CHECK((tt(obj) | tt::refs).size() == 1);
		CHECK((tt(obj) | tt::handle)->get() == obj);
		CHECK((tt(obj) | tt::Tree).is_object());
		CHECK((tt(obj) | tt::nt) == json_parser::object);
		CHECK((tt(obj) | tt::member("s") | tt::string) == "t");
		CHECK((tt(obj) | tt::member("b") | tt::string).empty());
		CHECK((tt(obj) | tt::member("a") | json_parser::number
			| tt::number) == 1.5);
		CHECK((tt(obj) | tt::member("s") | tt::number) == 0);
		CHECK((tt(obj) | tt::member("b") | tt::boolean));
		CHECK_FALSE((tt(obj) | tt::member("missing")).has_value());
		CHECK_FALSE((tt(obj) | tt::member("missing")
			| tt::member("x")).has_value());
		CHECK((tt(obj) | tt::member("a") | tt::children).size() == 2);
		CHECK((tt(obj) | tt::member("a") | tt::first | tt::number) == 1.5);
		CHECK((tt(obj) | tt::member("a") | tt::second | tt::string)
			== "x");
		CHECK((tt(obj) | json_parser::object_pair | tt::only_child)
			.is(json_parser::arr));
		size_t n = 0;
		for (tref c : tt(obj) | tt::children_range) n += c != nullptr;
		CHECK(n == 3);
		n = 0;
		for (const jt& c : tt(obj) | tt::children_trees_range)
			n += c.is(json_parser::object_pair);
		CHECK(n == 3);
		auto strings = tt::f([](tref p) {
			tref v = jt::get(p).first();
			return jt::get(v).is_string() ? v : nullptr;
		});
		CHECK((tt(obj) | tt::children | strings).size() == 1);
	}
}

TEST_SUITE("json tree: free queries") {
	TEST_CASE("is tests the kind of a node, and nullptr is no kind") {
		tref obj = jt::get(json_parser::object, trefs{});
		CHECK(format::json::is(obj, json_parser::object));
		CHECK_FALSE(format::json::is(obj, json_parser::arr));
		CHECK_FALSE(format::json::is(nullptr, json_parser::object));
		CHECK(format::json::is(json_parser::object)(obj));
		CHECK_FALSE(format::json::is(json_parser::arr)(obj));
		CHECK(format::json::is(obj,
			{ json_parser::arr, json_parser::object }));
		CHECK_FALSE(format::json::is(obj,
			{ json_parser::arr, json_parser::str }));
		auto either = format::json::is(
			{ json_parser::arr, json_parser::object });
		CHECK(either(obj));
		CHECK_FALSE(either(nullptr));
	}

	TEST_CASE("string_member gives the text of a string member only") {
		tref obj = jt::get(json_parser::object, {
			jt::get(node(json_parser::object_pair, 0, "g"),
				jt::get(node(json_parser::str, 0, "g.tgf"))),
			jt::get(node(json_parser::object_pair, 0, "n"),
				jt::get(json_parser::null_sym)) });
		REQUIRE(string_member(obj, "g") != nullptr);
		CHECK(*string_member(obj, "g") == "g.tgf");
		CHECK(string_member(obj, "n") == nullptr);
		CHECK(string_member(obj, "missing") == nullptr);
		CHECK(string_member(nullptr, "g") == nullptr);
	}
}

TEST_SUITE("json tree: builders") {
	TEST_CASE("each builder gives the node of its kind") {
		CHECK(jt::_null() == jt::get(json_parser::null_sym));
		CHECK(jt::build_bool(true) == jt::_true());
		CHECK(jt::build_bool(false) == jt::_false());
		CHECK(jt::get(jt::build_number(1.5)).get_number() == 1.5);
		CHECK(jt::build_number(1.5) == jt::build_number(1.5));
		CHECK(jt::build_number(-0.0) == jt::build_number(0.0));
		CHECK(jt::get(jt::build_string("x")).get_string() == "x");
		tref arr = jt::build_array({ jt::build_string("x"),
			jt::build_number(1.5) });
		CHECK((tt(arr) | tt::first | tt::string) == "x");
		CHECK((tt(arr) | tt::second | tt::number) == 1.5);
		tref pair = jt::build_pair("k", jt::_true());
		CHECK(jt::get(pair).is(json_parser::object_pair));
		CHECK(jt::get(pair).get_string() == "k");
		CHECK(jt::get(pair).value_tree().get() == jt::_true());
	}

	TEST_CASE("a repeated key keeps its first position and its last value") {
		tref obj = jt::build_object({
			jt::build_pair("a", jt::build_number(1)),
			jt::build_pair("b", jt::build_number(2)),
			jt::build_pair("a", jt::build_number(3)) });
		CHECK((tt(obj) || json_parser::object_pair).size() == 2);
		CHECK(jt::get(obj).first_tree().get_string() == "a");
		CHECK((tt(obj) | tt::member("a") | tt::number) == 3);
		CHECK((tt(obj) | tt::member("b") | tt::number) == 2);
	}

	TEST_CASE("set_member changes a member or adds it at the end") {
		tref obj = jt::build_object({
			jt::build_pair("id", jt::build_number(7)),
			jt::build_pair("ok", jt::_true()) });
		tref changed = jt::set_member(obj, "id", jt::build_number(8));
		CHECK((tt(changed) | tt::member("id") | tt::number) == 8);
		CHECK(jt::get(changed).first_tree().get_string() == "id");
		CHECK((tt(obj) | tt::member("id") | tt::number) == 7);
		tref added = jt::set_member(obj, "new", jt::_null());
		CHECK((tt(added) || json_parser::object_pair).size() == 3);
		CHECK(jt::get(added)[2].get_string() == "new");
		CHECK(jt::set_member(obj, "ok", jt::_true()) == obj);
	}
}
