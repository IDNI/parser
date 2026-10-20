// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "format/json/json.h"
#include "format/ast.json/ast_json.h"

#include <bit>
#include <sstream>
#include <string>
#include <vector>

using namespace idni;
using namespace idni::format::json;

using jt = idni::format::json::tree;

// The tree of a JSON text that must parse.
static tref parsed(std::string_view text) {
	auto r = jt::get(text);
	REQUIRE(r.has_value());
	return r.value();
}

TEST_SUITE("json tree: round trip") {
	// The texts the tests of doctest_json read.
	const std::vector<std::string> texts{
		"null", "true", "false", "42", "-1.5e2", "\"hello\"",
		"[1,2,3]", "{\"a\":1}", "[10,20,30]",
		R"({"a":[1,{"b":true},null],"c":{"d":[]}})",
		"\"line\\nbreak\"", R"("q\"b\\")",
		"{\r\"a\"\r:\r[\r1\r,\r2\r]\r,\r\"b\"\r:\rtrue\r}\r",
		"\r\r1", "[]", "{}" };

	TEST_CASE("the tree prints the expected text") {
		const std::vector<std::string> printed{
			"null", "true", "false", "42", "-150", "\"hello\"",
			"[1,2,3]", "{\"a\":1}", "[10,20,30]",
			R"({"a":[1,{"b":true},null],"c":{"d":[]}})",
			"\"line\\nbreak\"", R"("q\"b\\")",
			R"({"a":[1,2],"b":true})", "1", "[]", "{}" };
		REQUIRE(printed.size() == texts.size());
		for (size_t i = 0; i != texts.size(); ++i) {
			CAPTURE(texts[i]);
			CHECK(jt::get(parsed(texts[i])).to_str() == printed[i]);
		}
	}

	TEST_CASE("the printed text parses back to the same node") {
		for (const auto& s : texts) {
			CAPTURE(s);
			tref t = parsed(s);
			CHECK(parsed(jt::get(t).to_str()) == t);
		}
	}

	TEST_CASE("malformed input reports an error and no value") {
		for (const char* s : { "", "[1,2,]", "{\"a\":1,}", "\"abc",
				"\"\\uD800\"", "{\"\\uD800\":1}" })
		{
			CAPTURE(s);
			auto r = jt::get(std::string_view(s));
			CHECK_FALSE(r.has_value());
			CHECK(r.has_error());
		}
	}
}

TEST_SUITE("json tree: values") {
	TEST_CASE("a repeated key keeps its first position and its last value") {
		tref t = parsed(R"({"a":1,"b":2,"a":3})");
		CHECK(jt::get(t).to_str() == R"({"a":3,"b":2})");
	}

	TEST_CASE("-0 is 0") {
		CHECK(jt::build_number(-0.0) == jt::build_number(0.0));
		CHECK(jt::get(parsed("-0")).to_str() == "0");
		CHECK(jt::get(parsed("[-0,0,-0.0]")).to_str() == "[0,0,0]");
		CHECK(parsed("-0") == parsed("0"));
	}

	TEST_CASE("escapes read back decoded and print escaped") {
		tref t = parsed(R"("a\nb\"c\\d\/e\t")");
		CHECK(jt::get(t).get_string() == "a\nb\"c\\d/e\t");
		CHECK(jt::get(t).to_str() == R"("a\nb\"c\\d/e\t")");
	}

	TEST_CASE("unicode escapes and raw UTF-8 decode to UTF-8") {
		tref t = parsed(R"("\u00e9\ud83d\ude00")");
		CHECK(jt::get(t).get_string() == "\xc3\xa9\xf0\x9f\x98\x80");
		CHECK(parsed("\"\xc3\xa9\xf0\x9f\x98\x80\"") == t);
		CHECK(parsed(jt::get(t).to_str()) == t);
	}

	TEST_CASE("an element printed alone has no trailing comma") {
		tref t = parsed("[1,[2,3],4]");
		CHECK(jt::get(jt::get(t).first()).to_str() == "1");
		CHECK(jt::get(jt::get(t).second()).to_str() == "[2,3]");
	}

	TEST_CASE("builders, queries and the traverser") {
		tref state = jt::build_object({
			jt::build_pair("grammar", jt::build_string("g.tgf")),
			jt::build_pair("start", jt::_null()) });
		tref t = jt::build_object({
			jt::build_pair("id", jt::build_number(7)),
			jt::build_pair("ok", jt::build_bool(true)),
			jt::build_pair("list", jt::build_array({
				jt::build_string("x"), jt::build_number(1.5) })),
			jt::build_pair("state", state) });
		CHECK(jt::get(t).to_str() == R"({"id":7,"ok":true,)"
			R"("list":["x",1.5],"state":{"grammar":"g.tgf",)"
			R"("start":null}})");
		CHECK((tt(t) | tt::member("id") | tt::number) == 7);
		CHECK((tt(t) | tt::member("ok") | tt::boolean) == true);
		CHECK((tt(t) | tt::member("state") | tt::member("grammar")
			| tt::string) == "g.tgf");
		CHECK_FALSE((tt(t) | tt::member("missing")).has_value());
		CHECK_FALSE((tt(t) | tt::member("missing") | tt::member("x"))
			.has_value());
		CHECK((tt(t) | tt::member("list") | tt::children).size() == 2);
		CHECK((tt(t) || json_parser::object_pair).size() == 4);
		CHECK((tt(t) | tt::member("list") | json_parser::number
			| tt::number) == 1.5);
		CHECK(*string_member(state, "grammar") == "g.tgf");
		CHECK(string_member(state, "start") == nullptr);
		CHECK(format::json::is(t, json_parser::object));
		CHECK(format::json::is(json_parser::object)(t));
		CHECK(format::json::is(t,
			{ json_parser::arr, json_parser::object }));
		CHECK(jt::get(t).find("state") == state);
		CHECK(jt::get(t).find("nope") == nullptr);
		tref u = jt::set_member(t, "id", jt::build_number(8));
		CHECK(jt::get(u).to_str().starts_with(R"({"id":8,"ok":true)"));
		tref w = jt::set_member(state, "start", jt::build_string("s"));
		CHECK(jt::get(w).to_str() == R"({"grammar":"g.tgf","start":"s"})");
	}
}

TEST_SUITE("json tree: integers") {
	TEST_CASE("an integer above 2^53 prints as it was read") {
		for (const char* s : { "9007199254740993",
				"9223372036854775807", "-9223372036854775808" })
		{
			CAPTURE(s);
			tref t = parsed(s);
			CHECK(jt::get(t).to_str() == s);
			REQUIRE(jt::get(t).get_integer().has_value());
			CHECK(std::to_string(*jt::get(t).get_integer()) == s);
		}
		CHECK(*jt::get(parsed("9007199254740993")).get_integer()
			== 9007199254740993);
	}

	TEST_CASE("-0 prints 0 and has the integer 0") {
		CHECK(jt::get(parsed("-0")).to_str() == "0");
		CHECK(jt::get(parsed("-0")).get_integer() == 0);
	}

	TEST_CASE("a fraction has no integer") {
		CHECK_FALSE(jt::get(parsed("1.5")).get_integer().has_value());
		CHECK_FALSE(jt::get(parsed("2.5e-1")).get_integer().has_value());
		CHECK_FALSE(jt::get(jt::build_number(1.5)).get_integer()
			.has_value());
	}

	TEST_CASE("an exponent form with an integer value is that integer") {
		CHECK(jt::get(parsed("1e3")).get_integer() == 1000);
		CHECK(jt::get(parsed("1e3")).to_str() == "1000");
		CHECK(parsed("1e3") == parsed("1000"));
		CHECK(parsed("-1.5e2") == jt::build_integer(-150));
		CHECK(parsed("2.0") == parsed("2"));
		CHECK(parsed("-0.0") == parsed("0"));
	}

	TEST_CASE("an integer outside int64 prints its text and has no integer") {
		tref t = parsed("18446744073709551616");
		CHECK(jt::get(t).to_str() == "18446744073709551616");
		CHECK_FALSE(jt::get(t).get_integer().has_value());
		CHECK(jt::get(t).get_number() == 0x1p64);
	}

	TEST_CASE("the builders and the parse of an integer give one node") {
		CHECK(parsed("1") == jt::build_number(1.0));
		CHECK(parsed("1") == jt::build_integer(1));
		CHECK(parsed("9007199254740993")
			== jt::build_integer(9007199254740993));
		CHECK(jt::get(jt::build_integer(-7)).to_str() == "-7");
		CHECK(jt::get(jt::build_integer(-7)).get_integer() == -7);
		CHECK(jt::get(jt::build_number(0x1p63)).to_str()
			== "9223372036854775808");
		CHECK_FALSE(jt::get(jt::build_number(0x1p63)).get_integer()
			.has_value());
		CHECK(jt::get(jt::build_string("12")).get_integer()
			== std::nullopt);
	}
}

TEST_SUITE("json tree: AST JSON") {
	using p_t = idni::parser<char, char32_t>;
	using lit_t = idni::lit<char, char32_t>;
	using tree_t = p_t::tree;
	using pnode_t = p_t::pnode;

	static idni::nonterminals<char, char32_t> nts({"", "start", "digit"});

	static tref term_node(char32_t c, size_t b, size_t e) {
		return tree_t::get(pnode_t(lit_t(c), {b, e}));
	}

	// The shaped parse tree of a JSON text that must parse.
	static tref parse_tree_of(const std::string& text) {
		json_parser::parse_options po{ .start = json_parser::start };
		auto r = json_parser::instance().parse(text.data(),
			text.size(), po);
		REQUIRE(r.found);
		return r.get_shaped_tree2();
	}

	TEST_CASE("the tree form prints the expected text") {
		tref term = term_node(char32_t('1'), 0, 1);
		tref nul = tree_t::get(pnode_t(lit_t(), {1, 1}));
		tref dig = tree_t::get(pnode_t(nts("digit"), {0, 1}),
			trefs{term, nul});
		tref root = tree_t::get(pnode_t(nts("start"), {0, 1}),
			trefs{dig});
		const std::string start_id = std::to_string(nts("start").n());
		const std::string digit_id = std::to_string(nts("digit").n());
		const std::string text = R"({"format":"ast","version":"1",)"
			R"("start":"start","input":"1","ast":{"symbol":"start",)"
			R"("id":)" + start_id + R"(,"range":[0,1],"children":[)"
			R"({"symbol":"digit","id":)" + digit_id
			+ R"(,"range":[0,1],"children":[{"symbol":"",)"
			R"("range":[0,1],"text":"1"}]}]}})";
		namespace aj = idni::format::ast_json;
		CHECK(jt::get(aj::to_tree<tree_t>(root, "start", "1")).to_str()
			== text);

		tref x = term_node(char32_t('x'), 2, 3);
		tref r = tree_t::get(pnode_t(nts("start"), {2, 3}), trefs{x});
		CHECK(jt::get(aj::node_to_tree<tree_t>(r)).to_str()
			== R"({"symbol":"start","id":)" + start_id
			+ R"(,"range":[2,3],"children":[{"symbol":"",)"
			R"("range":[2,3],"text":"x"}]})");
	}

	TEST_CASE("the AST JSON of a JSON parse tree") {
		namespace aj = idni::format::ast_json;
		for (const char* s : { "1", "[1,2,3]", "[[],[]]",
			R"({"a":[1,{"b":true},null],"c":{"d":[]}})" })
		{
			CAPTURE(s);
			tref p = parse_tree_of(s);
			tref t = aj::node_to_tree<parse_tree>(p);
			CHECK(parsed(jt::get(t).to_str()) == t);
			CHECK((tt(t) | tt::member("symbol") | tt::string)
				== "start");
		}
	}
}

TEST_SUITE("json tree: deep documents and gc") {
	constexpr size_t deep = 1000000;
	// The deepest parse that stays under 10 s in a release build.
	constexpr size_t deep_parse = 150000;

	TEST_CASE("a document 1000000 levels deep builds, prints and is freed") {
		jt::gc();
		const size_t before = jt::m_size();
		{
			tref t = jt::_null();
			for (size_t i = 0; i != deep; ++i)
				t = i % 2 ? jt::build_array({ t })
					: jt::build_object({ jt::build_pair("k", t) });
			htref h = jt::geth(t);
			std::string open, close;
			for (size_t i = deep; i-- != 0; ) {
				open += i % 2 ? "[" : "{\"k\":";
				close += (deep - 1 - i) % 2 ? ']' : '}';
			}
			const std::string text = open + "null" + close;
			CHECK(jt::get(h).to_str() == text);
			jt::gc();
			CHECK(jt::get(h).to_str() == text);
			CHECK(jt::m_size() > deep);
		}
		jt::gc();
		CHECK(jt::m_size() == before);
	}

	TEST_CASE("parse and print deep arrays") {
		const std::string text = std::string(deep_parse, '[')
			+ std::string(deep_parse, ']');
		json_parser::parse_options po{ .start = json_parser::start };
		auto r = json_parser::instance().parse(text.data(),
			text.size(), po);
		REQUIRE(r.found);
		tref p = r.get_shaped_tree2();
		auto t = jt::get(parse_tree::get(p));
		REQUIRE(t.has_value());
		CHECK(jt::get(t.value()).to_str() == text);
		// the AST JSON of the same parse tree
		const std::string ast = jt::get(format::ast_json::
			node_to_tree<parse_tree>(p)).to_str();
		const std::string arr = R"("symbol":"arr")";
		size_t arrays = 0;
		for (size_t at = ast.find(arr); at != std::string::npos;
				at = ast.find(arr, at + arr.size()))
			++arrays;
		CHECK(arrays == deep_parse);
	}
}

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

TEST_SUITE("json tree: parse") {
	TEST_CASE("a parse gives the tree of each kind") {
		tref t = parsed(R"({"a":[1.5,{"b":true},null],"c":"x","d":false})");
		CHECK(jt::get(t).is_object());
		CHECK((tt(t) | tt::member("a") | tt::first | tt::number) == 1.5);
		CHECK((tt(t) | tt::member("a") | tt::second | tt::member("b")
			| tt::boolean));
		CHECK((tt(t) | tt::member("a") | tt::children).size() == 3);
		CHECK((tt(t) | tt::member("a") | json_parser::null_sym)
			.has_value());
		CHECK(*string_member(t, "c") == "x");
		CHECK_FALSE((tt(t) | tt::member("d") | tt::boolean));
		CHECK(parsed(" \r\n[ 1 , 2 ]\t") == parsed("[1,2]"));
		CHECK(parsed(R"({"a":[1.5,{"b":true},null],"c":"x","d":false})")
			== t);
	}

	TEST_CASE("a parse and the builders give one node") {
		CHECK(parsed("-0") == parsed("0"));
		CHECK(parsed("-0") == jt::build_number(0));
		CHECK(parsed("-1.5e2") == jt::build_number(-150));
		CHECK(parsed(R"({"a":1,"b":2,"a":3})") == jt::build_object({
			jt::build_pair("a", jt::build_number(3)),
			jt::build_pair("b", jt::build_number(2)) }));
		CHECK(parsed(R"("a\nb\"c\\d\/e\t")")
			== jt::build_string("a\nb\"c\\d/e\t"));
		CHECK(parsed(R"("\u00e9\ud83d\ude00")")
			== jt::build_string("\xc3\xa9\xf0\x9f\x98\x80"));
	}

	TEST_CASE("the tree of a parse result and of a parse tree") {
		json_parser::parse_options po{ .start = json_parser::start };
		const std::string text = R"([true,"s"])";
		auto r = json_parser::instance().parse(text.data(), text.size(),
			po);
		REQUIRE(r.found);
		auto from_tree = jt::get(parse_tree::get(r.get_shaped_tree2()));
		REQUIRE(from_tree.has_value());
		CHECK(from_tree.value() == parsed(text));
		auto from_result = jt::get(r);
		REQUIRE(from_result.has_value());
		CHECK(from_result.value() == parsed(text));
		const std::string bad = "[true,";
		auto f = json_parser::instance().parse(bad.data(), bad.size(), po);
		auto failed = jt::get(f);
		CHECK_FALSE(failed.has_value());
		CHECK(failed.report().first_error_code()
			== idni::diagnostics::code::parse_error);
	}
}

TEST_SUITE("json tree: printers") {
	TEST_CASE("the stream operators, write_line and the node dump") {
		tref t = parsed(R"({"a":[1.5,"x"]})");
		std::ostringstream os;
		os << jt::get(t);
		CHECK(os.str() == jt::get(t).to_str());
		std::ostringstream line;
		write_line(t, line);
		CHECK(line.str() == R"({"a":[1.5,"x"]})" "\n");
		std::ostringstream nodes;
		nodes << node(json_parser::number, std::bit_cast<uint64_t>(1.5))
			<< '|' << node(json_parser::str, 0, "x\"y")
			<< '|' << node(json_parser::null_sym);
		CHECK(nodes.str() == R"(number 1.5|str "x\"y"|null_sym)");
		std::ostringstream two;
		two << (tt(t) | tt::member("a") | tt::children);
		CHECK(two.str() == "1.5\n\"x\"");
		std::ostringstream none;
		none << tt();
		CHECK(none.str() == "(no value)");
		CHECK_FALSE(jt::get(t).dump_to_str().empty());
		std::ostringstream tree_text;
		jt::get(t).print_tree(tree_text);
		CHECK_FALSE(tree_text.str().empty());
		CHECK((tt() | tt::dump).empty());
	}
}
