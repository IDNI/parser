// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Tests for growing a @dynamic nonterminal mid-parse via
// grammar::add_dynamic_production_from and parser::options::on_dynamic_grow.
//
// The hook fires when a PARENT (the first of a dynamic_grow_nts pair)
// advances to a strictly greater input position after consuming a
// registered CHILD (the pair's second id), passing the child's own span.
// Neither the child's own completion nor the parent's own completion is
// the trigger: a child like a left-recursive run of letters completes at
// every prefix length, and firing there would make every prefix of a
// declared name live for the rest of the parse. Firing on progress past
// the child means a prefix that a delimiter rejects never grows at all,
// while a name followed by its delimiter grows the moment that delimiter
// is consumed - before the parent's own body, so a name is usable inside
// the very declaration that introduces it. A parent's own completion
// still confirms exactly the child spans it grew; parser::parse() commits
// confirmed values on success and retires everything pending on failure.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "parser.h"

using namespace std;
using namespace idni;

namespace {
	// Builds: start => def ; def => "t " chars "=" "X" ;
	// chars => alpha | chars alpha. chars is left-recursive, so it
	// completes at every prefix length of a run of letters, but def can
	// only complete over the full run: whatever chars matched must be
	// followed immediately by "=". type_name has no static production
	// of its own: the only way it ever matches anything is through
	// add_dynamic_production_from, called from on_dynamic_grow with
	// chars' own span.
	struct def_fixture {
		nonterminals<char> nts;
		prods<char> ps, start, def, chars, alpha;
		char_class_fns<char> cc;
		grammar<char> g;
		lit<char> type_name_l;
		size_t def_id, chars_id;
		size_t hook_calls = 0;
		parser<char>::options popt;

		def_fixture() :
			start(nts("start")), def(nts("def")), chars(nts("chars")),
			alpha(nts("alpha")),
			cc(predefined_char_classes<char>({ "alpha" }, nts)),
			g(nts, build_ps(), start, cc)
		{
			def_id = g.nt("def").n();
			chars_id = g.nt("chars").n();
			type_name_l = g.nt("type_name");
			popt.dynamic_grow_nts = { { def_id, chars_id } };
			popt.on_dynamic_grow = [this](parser<char>::input& in,
				size_t, size_t from, size_t to)
			{
				++hook_calls;
				g.add_dynamic_production_from(type_name_l,
					in.get_terminals(from, to));
			};
		}

		prods<char> build_ps() {
			ps(start, def);
			ps(def, prods<char>("t ") + chars + "=" + "X");
			ps(chars, alpha);
			ps(chars, chars + alpha);
			return ps;
		}

		// probes type_name alone, bypassing def so the hook never
		// fires from the probe itself
		bool probes_ok(parser<char>& p, const string& text) {
			parser<char>::parse_options o;
			o.start = type_name_l.n();
			return p.parse(text.c_str(), text.size(), o).found;
		}
	};
}

TEST_SUITE("dynamic grow: commit and rollback") {

	TEST_CASE("a value the parent confirms stays live after the parse, "
		"and a second parse can use it")
	{
		def_fixture f;
		parser<char> p(f.g, f.popt);

		CHECK_FALSE(f.probes_ok(p, "Point"));

		string in = "t Point=X";
		auto r = p.parse(in.c_str(), in.size());
		REQUIRE(r.found);
		CHECK(f.hook_calls >= 1);

		// a later, separate parse sees the committed value
		CHECK(f.probes_ok(p, "Point"));
	}

	TEST_CASE("a parse that fails leaves the container exactly as it was")
	{
		nonterminals<char> nts;
		prods<char> ps, start(nts("start")), def(nts("def")),
			chars(nts("chars")), alpha(nts("alpha")), tail(nts("tail"));
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "alpha" }, nts);
		ps(start, def + " " + tail);
		ps(def, prods<char>("t ") + chars + "=" + "X");
		ps(chars, alpha);
		ps(chars, chars + alpha);
		ps(tail, prods<char>("Y"));
		grammar<char> g(nts, ps, start, cc);

		lit<char> type_name_l = g.nt("type_name");
		size_t def_id = g.nt("def").n(), chars_id = g.nt("chars").n();

		parser<char>::options popt;
		popt.dynamic_grow_nts = { { def_id, chars_id } };
		popt.on_dynamic_grow = [&](parser<char>::input& in, size_t,
			size_t from, size_t to)
		{
			g.add_dynamic_production_from(type_name_l,
				in.get_terminals(from, to));
		};
		parser<char> p(g, popt);

		parser<char>::parse_options probe;
		probe.start = type_name_l.n();

		// def completes and confirms "Point" well before the parse as
		// a whole fails (tail expects "Y", gets "Z")
		string in = "t Point=X Z";
		auto r = p.parse(in.c_str(), in.size());
		CHECK_FALSE(r.found);
		CHECK_FALSE(p.parse("Point", 5, probe).found);

		// the next, successful parse still registers Point itself
		string ok_input = "t Point=X Y";
		auto r2 = p.parse(ok_input.c_str(), ok_input.size());
		REQUIRE(r2.found);
		CHECK(p.parse("Point", 5, probe).found);
	}
}

TEST_SUITE("dynamic grow: prefix immunity") {

	TEST_CASE("a strict prefix used elsewhere in the same parse never "
		"enters the grammar, in a single attempt")
	{
		// chars is left-recursive on itself (chars => alpha | chars
		// alpha), so it completes at every prefix length of a run of
		// letters, and its own recursive predictor sits in the same
		// completion cache slot as def's. A parenthesised term with an
		// alternative that explicitly excludes a type, a declaration
		// of Point, and a use of (P) later in the same input complete
		// the mirror. Only the registered pair's parent (def) may
		// read a completed chars' span: chars' own left-recursive
		// predictor is not that parent, so growing a prefix on its
		// account would be the exact defect this test pins. "P" a
		// strict prefix of Point would otherwise grow, making (P)
		// ambiguous between type_name and not_type.
		nonterminals<char> nts;
		prods<char> ps, start(nts("start")), def(nts("def")),
			chars(nts("chars")), alpha(nts("alpha")), use(nts("use")),
			inner(nts("inner")), not_type(nts("not_type")),
			type_name(nts("type_name"));
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "alpha" }, nts);
		ps(start, def + " " + use);
		ps(def, prods<char>("t ") + chars + "=" + "X");
		ps(chars, alpha);
		ps(chars, chars + alpha);
		ps(use, prods<char>("(") + inner + ")");
		ps(inner, type_name);
		ps(inner, not_type);
		ps(not_type, prods<char>("P"));
		// type_name has no static production of its own
		grammar<char> g(nts, ps, start, cc);

		lit<char> type_name_l = g.nt("type_name");
		size_t def_id = g.nt("def").n(), chars_id = g.nt("chars").n();
		size_t hook_calls = 0;

		parser<char>::options popt;
		popt.dynamic_grow_nts = { { def_id, chars_id } };
		popt.on_dynamic_grow = [&](parser<char>::input& in, size_t,
			size_t from, size_t to)
		{
			++hook_calls;
			g.add_dynamic_production_from(type_name_l,
				in.get_terminals(from, to));
		};
		parser<char> p(g, popt);

		string in = "t Point=X (P)";
		auto r = p.parse(in.c_str(), in.size());
		REQUIRE(r.found);
		CHECK_FALSE(r.is_ambiguous());
		// a single fire, for Point, in a single parsing attempt: P,
		// Po, Poi and Poin never progress past their own chars span
		CHECK(hook_calls == 1);

		auto t = r.get_shaped_forest_tree();
		REQUIRE(t);
		ostringstream ss;
		t->to_print(ss);
		// present only if (P) resolved through not_type, ie. type_name
		// never carried a "P" alternative during the parse
		CHECK(ss.str().find("not_type") != string::npos);

		parser<char>::parse_options probe;
		probe.start = type_name_l.n();
		CHECK_FALSE(p.parse("P", 1, probe).found);
		CHECK_FALSE(p.parse("Po", 2, probe).found);
		CHECK_FALSE(p.parse("Poi", 3, probe).found);
		CHECK_FALSE(p.parse("Poin", 4, probe).found);
		CHECK(p.parse("Point", 5, probe).found);
	}

	TEST_CASE("declaring Point registers Point, and none of its "
		"prefixes")
	{
		def_fixture f;
		parser<char> p(f.g, f.popt);

		string in = "t Point=X";
		auto r = p.parse(in.c_str(), in.size());
		REQUIRE(r.found);

		CHECK(f.probes_ok(p, "Point"));
		CHECK_FALSE(f.probes_ok(p, "P"));
		CHECK_FALSE(f.probes_ok(p, "Po"));
		CHECK_FALSE(f.probes_ok(p, "Poi"));
		CHECK_FALSE(f.probes_ok(p, "Poin"));
	}
}

TEST_SUITE("dynamic grow: relaxation enables self-reference") {

	TEST_CASE("a name is usable inside the very declaration that "
		"declares it, in a single attempt")
	{
		// type_def => "type " chars "=" "{" "next" ":" type_name
		// "}" "." : the self-reference inside the body needs
		// type_name to already have "Node" by the time it is
		// reached, well before type_def itself completes. Firing on
		// progress past chars means the grow happens the moment "="
		// is consumed, right after chars, which is before the body
		// (and the self-reference inside it) is parsed at all. chars
		// is left-recursive on itself (chars => alpha | chars alpha),
		// so its own recursive predictor shares the completion cache
		// slot with type_def's; only the registered pair's parent
		// (type_def) may read a completed chars' span.
		nonterminals<char> nts;
		prods<char> ps, start(nts("start")), type_def(nts("type_def")),
			chars(nts("chars")), alpha(nts("alpha")),
			type_name(nts("type_name"));
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "alpha" }, nts);
		ps(start, type_def);
		ps(type_def, prods<char>("type ") + chars + "=" + "{" +
			"next" + ":" + type_name + "}" + ".");
		ps(chars, alpha);
		ps(chars, chars + alpha);
		grammar<char> g(nts, ps, start, cc);

		lit<char> type_name_l = g.nt("type_name");
		size_t type_def_id = g.nt("type_def").n();
		size_t chars_id = g.nt("chars").n();
		size_t hook_calls = 0;

		parser<char>::options popt;
		popt.dynamic_grow_nts = { { type_def_id, chars_id } };
		popt.on_dynamic_grow = [&](parser<char>::input& in, size_t,
			size_t from, size_t to)
		{
			++hook_calls;
			g.add_dynamic_production_from(type_name_l,
				in.get_terminals(from, to));
		};
		parser<char> p(g, popt);

		string in = "type Node={next:Node}.";
		auto r = p.parse(in.c_str(), in.size());
		REQUIRE(r.found);
		// one fire, for the full "Node" span, the moment "=" is
		// consumed: N, No and Nod never progress past their own span
		CHECK(hook_calls == 1);
	}
}

TEST_SUITE("dynamic grow: nullable group between child and delimiter") {

	TEST_CASE("a self-reference reached through one intervening optional "
		"group still grows before that group is parsed")
	{
		// type_def => "type " chars grp "=" "{" "next" ":" type_name
		// "}" "." ; grp => "(" type_name ")" | null. grp sits between
		// chars and the parent's first literal, and grp itself can
		// derive empty, so the item pointing at grp is nullable: add()
		// takes its own shortcut past grp the moment that item is
		// inserted, alongside the ordinary predict() route into grp's
		// alternatives. This input takes the non-empty alternative, so
		// the successful derivation runs through predict()/scan(), not
		// through that shortcut; the case proves the annotation still
		// reaches a name used one group away from chars.
		nonterminals<char> nts;
		prods<char> ps, start(nts("start")), type_def(nts("type_def")),
			chars(nts("chars")), alpha(nts("alpha")),
			type_name(nts("type_name")), grp(nts("grp")),
			nll(lit<char>{});
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "alpha" }, nts);
		ps(start, type_def);
		ps(type_def, prods<char>("type ") + chars + grp + "=" + "{" +
			"next" + ":" + type_name + "}" + ".");
		ps(chars, alpha);
		ps(chars, chars + alpha);
		ps(grp, prods<char>("(") + type_name + ")");
		ps(grp, nll);
		grammar<char> g(nts, ps, start, cc);

		lit<char> type_name_l = g.nt("type_name");
		size_t type_def_id = g.nt("type_def").n();
		size_t chars_id = g.nt("chars").n();
		size_t hook_calls = 0;

		parser<char>::options popt;
		popt.dynamic_grow_nts = { { type_def_id, chars_id } };
		popt.on_dynamic_grow = [&](parser<char>::input& in, size_t,
			size_t from, size_t to)
		{
			++hook_calls;
			g.add_dynamic_production_from(type_name_l,
				in.get_terminals(from, to));
		};
		parser<char> p(g, popt);

		string in = "type Node(Node)={next:Node}.";
		auto r = p.parse(in.c_str(), in.size());
		REQUIRE(r.found);
		CHECK(hook_calls >= 1);
	}

	TEST_CASE("a self-reference nested the way the generated tau grammar "
		"nests it still grows before the parents group is parsed")
	{
		// Mirrors type_def's real shape: an outer nullable group (og,
		// like __E_type_def_13) whose non-empty alternative opens with
		// an inner group (ig, like __E___E_type_def_13_14) followed by
		// a parents group holding the self-reference, then a mandatory
		// delimiter group (dg, like __E_type_def_15) that is itself a
		// group rather than a bare terminal, ahead of the body. This is
		// the shape "type Y of (Y) is {m: bool}." needs: the grow must
		// land before parents is parsed, and dg being a group rather
		// than a literal means the parent's next advance also runs
		// through predict(), not a direct scan() of type_def's own
		// production.
		nonterminals<char> nts;
		prods<char> ps, start(nts("start")), type_def(nts("type_def")),
			chars(nts("chars")), alpha(nts("alpha")),
			type_name(nts("type_name")), og(nts("og")), ig(nts("ig")),
			parents(nts("parents")), dg(nts("dg")), nll(lit<char>{});
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "alpha" }, nts);
		ps(start, type_def);
		ps(type_def, prods<char>("type ") + chars + og + dg + "{" +
			"next" + ":" + type_name + "}" + ".");
		ps(chars, alpha);
		ps(chars, chars + alpha);
		ps(og, ig + parents);
		ps(og, nll);
		ps(ig, prods<char>(" of "));
		ps(parents, prods<char>("(") + type_name + ")");
		ps(dg, prods<char>(" is "));
		grammar<char> g(nts, ps, start, cc);

		lit<char> type_name_l = g.nt("type_name");
		size_t type_def_id = g.nt("type_def").n();
		size_t chars_id = g.nt("chars").n();
		size_t hook_calls = 0;

		parser<char>::options popt;
		popt.dynamic_grow_nts = { { type_def_id, chars_id } };
		popt.on_dynamic_grow = [&](parser<char>::input& in, size_t,
			size_t from, size_t to)
		{
			++hook_calls;
			g.add_dynamic_production_from(type_name_l,
				in.get_terminals(from, to));
		};
		parser<char> p(g, popt);

		string in = "type Node of (Node) is {next:Node}.";
		auto r = p.parse(in.c_str(), in.size());
		REQUIRE(r.found);
		CHECK(hook_calls >= 1);
	}

	TEST_CASE("the outer group's empty alternative still parses when the "
		"self-reference sits in the body, the case that already worked")
	{
		// Same grammar as the previous case, with input that takes og's
		// null alternative instead of ig + parents: chars is followed
		// immediately by dg, a mandatory group rather than a bare
		// terminal. add()'s nullable shortcut past og and the ordinary
		// predict()/complete() route through og's own null production
		// both produce the same advanced item here, so this proves
		// carrying the annotation through that shortcut does not cost
		// the path that already worked without it.
		nonterminals<char> nts;
		prods<char> ps, start(nts("start")), type_def(nts("type_def")),
			chars(nts("chars")), alpha(nts("alpha")),
			type_name(nts("type_name")), og(nts("og")), ig(nts("ig")),
			parents(nts("parents")), dg(nts("dg")), nll(lit<char>{});
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "alpha" }, nts);
		ps(start, type_def);
		ps(type_def, prods<char>("type ") + chars + og + dg + "{" +
			"next" + ":" + type_name + "}" + ".");
		ps(chars, alpha);
		ps(chars, chars + alpha);
		ps(og, ig + parents);
		ps(og, nll);
		ps(ig, prods<char>(" of "));
		ps(parents, prods<char>("(") + type_name + ")");
		ps(dg, prods<char>(" is "));
		grammar<char> g(nts, ps, start, cc);

		lit<char> type_name_l = g.nt("type_name");
		size_t type_def_id = g.nt("type_def").n();
		size_t chars_id = g.nt("chars").n();
		size_t hook_calls = 0;

		parser<char>::options popt;
		popt.dynamic_grow_nts = { { type_def_id, chars_id } };
		popt.on_dynamic_grow = [&](parser<char>::input& in, size_t,
			size_t from, size_t to)
		{
			++hook_calls;
			g.add_dynamic_production_from(type_name_l,
				in.get_terminals(from, to));
		};
		parser<char> p(g, popt);

		string in = "type Node is {next:Node}.";
		auto r = p.parse(in.c_str(), in.size());
		REQUIRE(r.found);
		CHECK(hook_calls >= 1);
	}

	TEST_CASE("a bare terminal right after the nullable group carries the "
		"annotation through add()'s own shortcut, with no later group "
		"for complete() to re-derive it from")
	{
		// Same grammar as the first case in this suite, grp taking its
		// null alternative this time: chars is followed by grp, then
		// directly by "=", a bare terminal in type_def's own
		// production rather than another group. complete() only
		// re-derives the annotation when a later group completes; here
		// there is none between grp and "=", so add()'s nullable-path
		// carry is the only site that can still reach the "=" item.
		nonterminals<char> nts;
		prods<char> ps, start(nts("start")), type_def(nts("type_def")),
			chars(nts("chars")), alpha(nts("alpha")),
			type_name(nts("type_name")), grp(nts("grp")),
			nll(lit<char>{});
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "alpha" }, nts);
		ps(start, type_def);
		ps(type_def, prods<char>("type ") + chars + grp + "=" + "{" +
			"next" + ":" + type_name + "}" + ".");
		ps(chars, alpha);
		ps(chars, chars + alpha);
		ps(grp, prods<char>("(") + type_name + ")");
		ps(grp, nll);
		grammar<char> g(nts, ps, start, cc);

		lit<char> type_name_l = g.nt("type_name");
		size_t type_def_id = g.nt("type_def").n();
		size_t chars_id = g.nt("chars").n();
		size_t hook_calls = 0;

		parser<char>::options popt;
		popt.dynamic_grow_nts = { { type_def_id, chars_id } };
		popt.on_dynamic_grow = [&](parser<char>::input& in, size_t,
			size_t from, size_t to)
		{
			++hook_calls;
			g.add_dynamic_production_from(type_name_l,
				in.get_terminals(from, to));
		};
		parser<char> p(g, popt);

		string in = "type Node={next:Node}.";
		auto r = p.parse(in.c_str(), in.size());
		REQUIRE(r.found);
		CHECK(hook_calls >= 1);
	}
}

TEST_SUITE("dynamic grow: multiple pairs") {

	TEST_CASE("one parent with two registered children confirms both")
	{
		nonterminals<char> nts;
		prods<char> ps, start(nts("start")), def(nts("def")),
			chars_a(nts("chars_a")), chars_b(nts("chars_b")),
			alpha(nts("alpha"));
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "alpha" }, nts);
		ps(start, def);
		ps(def, prods<char>("t ") + chars_a + ":" + chars_b + "=" +
			"X");
		ps(chars_a, alpha);
		ps(chars_a, chars_a + alpha);
		ps(chars_b, alpha);
		ps(chars_b, chars_b + alpha);
		grammar<char> g(nts, ps, start, cc);

		lit<char> type_name_a_l = g.nt("type_name_a");
		lit<char> type_name_b_l = g.nt("type_name_b");
		size_t def_id = g.nt("def").n();
		size_t chars_a_id = g.nt("chars_a").n();
		size_t chars_b_id = g.nt("chars_b").n();

		parser<char>::options popt;
		popt.dynamic_grow_nts = { { def_id, chars_a_id },
			{ def_id, chars_b_id } };
		popt.on_dynamic_grow = [&](parser<char>::input& in,
			size_t child_id, size_t from, size_t to)
		{
			const auto& l = child_id == chars_a_id ? type_name_a_l
								: type_name_b_l;
			g.add_dynamic_production_from(l,
				in.get_terminals(from, to));
		};
		parser<char> p(g, popt);

		string in = "t Point:Circle=X";
		auto r = p.parse(in.c_str(), in.size());
		REQUIRE(r.found);

		parser<char>::parse_options oa;
		oa.start = type_name_a_l.n();
		CHECK(p.parse("Point", 5, oa).found);
		parser<char>::parse_options ob;
		ob.start = type_name_b_l.n();
		CHECK(p.parse("Circle", 6, ob).found);
	}

	TEST_CASE("two parents sharing one registered child both confirm")
	{
		nonterminals<char> nts;
		prods<char> ps, start(nts("start")), def_a(nts("def_a")),
			def_b(nts("def_b")), chars(nts("chars")),
			alpha(nts("alpha"));
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "alpha" }, nts);
		ps(start, def_a + " " + def_b);
		ps(def_a, prods<char>("t ") + chars + "=" + "X");
		ps(def_b, prods<char>("u ") + chars + "=" + "Y");
		ps(chars, alpha);
		ps(chars, chars + alpha);
		grammar<char> g(nts, ps, start, cc);

		lit<char> type_name_l = g.nt("type_name");
		size_t def_a_id = g.nt("def_a").n();
		size_t def_b_id = g.nt("def_b").n();
		size_t chars_id = g.nt("chars").n();

		parser<char>::options popt;
		popt.dynamic_grow_nts = { { def_a_id, chars_id },
			{ def_b_id, chars_id } };
		popt.on_dynamic_grow = [&](parser<char>::input& in, size_t,
			size_t from, size_t to)
		{
			g.add_dynamic_production_from(type_name_l,
				in.get_terminals(from, to));
		};
		parser<char> p(g, popt);

		string in = "t Point=X u Circle=Y";
		auto r = p.parse(in.c_str(), in.size());
		REQUIRE(r.found);

		parser<char>::parse_options o;
		o.start = type_name_l.n();
		CHECK(p.parse("Point", 5, o).found);
		CHECK(p.parse("Circle", 6, o).found);
	}
}

TEST_SUITE("dynamic grow: parent equals child is rejected") {

	TEST_CASE("a pair whose parent equals its child is dropped, not "
		"thrown")
	{
		nonterminals<char> nts;
		prods<char> ps, start(nts("start"));
		ps(start, prods<char>("x"));
		char_class_fns<char> cc{};
		grammar<char> g(nts, ps, start, cc);
		size_t start_id = g.nt("start").n();

		size_t hook_calls = 0;
		parser<char>::options popt;
		popt.dynamic_grow_nts = { { start_id, start_id } };
		popt.on_dynamic_grow = [&](parser<char>::input&, size_t, size_t,
			size_t) { ++hook_calls; };
		// the invalid pair is dropped, so the parser builds fine and the
		// hook never fires: dynamic_grow_nts is empty by the time parse
		// runs
		parser<char> p(g, popt);
		string in = "x";
		auto r = p.parse(in.c_str(), in.size());
		CHECK(r.found);
		CHECK(hook_calls == 0);

		parser<char> p2(g);
		size_t hook_calls2 = 0;
		CHECK_FALSE(p2.set_dynamic_grow(
			[&](parser<char>::input&, size_t, size_t, size_t) {
				++hook_calls2;
			}, { { start_id, start_id } }));

		// options are unchanged: still no hook registered
		auto r2 = p2.parse(in.c_str(), in.size());
		CHECK(r2.found);
		CHECK(hook_calls2 == 0);
	}
}

TEST_SUITE("dynamic grow: dynamic_ctx") {

	TEST_CASE("a host-supplied container is used when given, and two "
		"parsers with separate containers do not see each other's "
		"values")
	{
		nonterminals<char> nts;
		prods<char> ps, start(nts("start")), def(nts("def")),
			chars(nts("chars")), alpha(nts("alpha"));
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "alpha" }, nts);
		ps(start, def);
		ps(def, prods<char>("t ") + chars + "=" + "X");
		ps(chars, alpha);
		ps(chars, chars + alpha);
		grammar<char> g(nts, ps, start, cc);

		lit<char> type_name_l = g.nt("type_name");
		size_t def_id = g.nt("def").n(), chars_id = g.nt("chars").n();

		parser<char>::options popt;
		popt.dynamic_grow_nts = { { def_id, chars_id } };
		popt.on_dynamic_grow = [&](parser<char>::input& in, size_t,
			size_t from, size_t to)
		{
			g.add_dynamic_production_from(type_name_l,
				in.get_terminals(from, to));
		};

		dynamic_context<char> ctx1, ctx2;
		parser<char> p1(g, popt), p2(g, popt);

		parser<char>::parse_options po1;
		po1.dynamic_ctx = &ctx1;
		string in1 = "t Point=X";
		REQUIRE(p1.parse(in1.c_str(), in1.size(), po1).found);

		parser<char>::parse_options probe1;
		probe1.start = type_name_l.n();
		probe1.dynamic_ctx = &ctx1;
		CHECK(p1.parse("Point", 5, probe1).found);

		parser<char>::parse_options probe2;
		probe2.start = type_name_l.n();
		probe2.dynamic_ctx = &ctx2;
		CHECK_FALSE(p2.parse("Point", 5, probe2).found);

		// the internal container (dynamic_ctx left null) is
		// likewise separate per parser instance
		parser<char> p3(g, popt), p4(g, popt);
		string in3 = "t Square=X";
		REQUIRE(p3.parse(in3.c_str(), in3.size()).found);

		parser<char>::parse_options probe3;
		probe3.start = type_name_l.n();
		CHECK(p3.parse("Square", 6, probe3).found);

		parser<char>::parse_options probe4;
		probe4.start = type_name_l.n();
		CHECK_FALSE(p4.parse("Square", 6, probe4).found);
	}
}

TEST_SUITE("dynamic grow: option off") {

	TEST_CASE("an empty dynamic_grow_nts fires no hook") {
		def_fixture f;
		f.popt.dynamic_grow_nts.clear();
		parser<char> p(f.g, f.popt);

		// def itself parses fine either way: type_name is not part
		// of this grammar's start production, only reachable via a
		// probe
		string in = "t Point=X";
		auto r = p.parse(in.c_str(), in.size());
		REQUIRE(r.found);
		CHECK(f.hook_calls == 0);
		CHECK_FALSE(f.probes_ok(p, "Point")); // type_name never grew
	}
}

TEST_SUITE("dynamic grow: ordering under disambiguation") {

	// A grown alternative always has a larger production index than one
	// declared at grammar construction, and disambiguation picks the
	// smallest production id among completions of equal span. So a
	// dynamically grown alternative never outranks a statically declared
	// one that matches the same text.
	TEST_CASE("a statically declared alternative outranks a "
		"dynamically grown one for the same text")
	{
		nonterminals<char> nts;
		prods<char> ps, start(nts("start")), def(nts("def")),
			chars(nts("chars")), alpha(nts("alpha")),
			type_name(nts("type_name")), static_word(nts("static_word"));
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "alpha" }, nts);
		ps(start, def + " " + type_name);
		ps(def, prods<char>("t ") + chars + "=" + "X");
		ps(chars, alpha);
		ps(chars, chars + alpha);
		ps(type_name, static_word);       // declared first: low prod id
		ps(static_word, prods<char>("Point"));
		grammar<char> g(nts, ps, start, cc);

		lit<char> type_name_l = g.nt("type_name");
		size_t def_id = g.nt("def").n(), chars_id = g.nt("chars").n();

		// the hook grows type_name mid-parse, so both alternatives are
		// live when disambiguation runs over the second "Point"
		parser<char>::options popt;
		popt.dynamic_grow_nts = { { def_id, chars_id } };
		popt.on_dynamic_grow = [&](parser<char>::input& in, size_t,
			size_t from, size_t to)
		{
			g.add_dynamic_production_from(type_name_l,
				in.get_terminals(from, to));
		};
		parser<char> p(g, popt);

		string in = "t Point=X Point";
		auto r = p.parse(in.c_str(), in.size());
		REQUIRE(r.found);
		CHECK_FALSE(r.is_ambiguous());

		auto t = r.get_shaped_forest_tree();
		REQUIRE(t);
		ostringstream ss;
		t->to_print(ss);
		// present only if type_name's child is static_word, ie. the
		// statically declared alternative was the one picked
		CHECK(ss.str().find("static_word") != string::npos);
	}
}

TEST_SUITE("dynamic grow: set_dynamic_grow") {

	TEST_CASE("set_dynamic_grow on an already-built parser wires the "
		"parent/child pair")
	{
		nonterminals<char> nts;
		prods<char> ps, start(nts("start")), def(nts("def")),
			chars(nts("chars")), alpha(nts("alpha"));
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "alpha" }, nts);
		ps(start, def);
		ps(def, prods<char>("t ") + chars + "=" + "X");
		ps(chars, alpha);
		ps(chars, chars + alpha);
		grammar<char> g(nts, ps, start, cc);

		lit<char> type_name_l = g.nt("type_name");
		size_t def_id = g.nt("def").n(), chars_id = g.nt("chars").n();

		// default options: the hook is unset until set_dynamic_grow runs
		parser<char> p(g);
		size_t hook_calls = 0;
		p.set_dynamic_grow(
			[&](parser<char>::input& in, size_t, size_t from,
				size_t to)
			{
				++hook_calls;
				g.add_dynamic_production_from(type_name_l,
					in.get_terminals(from, to));
			},
			{ { def_id, chars_id } });

		string in = "t Point=X";
		auto r = p.parse(in.c_str(), in.size());
		CHECK(r.found);
		CHECK(hook_calls >= 1);

		parser<char>::parse_options probe_opts;
		probe_opts.start = type_name_l.n();
		string probe = "Point";
		CHECK(p.parse(probe.c_str(), probe.size(), probe_opts).found);
	}
}

TEST_SUITE("dynamic grow: opt.dynamic host values survive sync") {

	TEST_CASE("a value given through the grammar constructor's "
		"opt.dynamic is found even with an explicit empty "
		"dynamic_context")
	{
		nonterminals<char> nts;
		prods<char> ps, start(nts("start")), def(nts("def")),
			chars(nts("chars")), alpha(nts("alpha"));
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "alpha" }, nts);
		ps(start, def);
		ps(def, prods<char>("t ") + chars + "=" + "X");
		ps(chars, alpha);
		ps(chars, chars + alpha);
		grammar<char>::options gopt;
		gopt.dynamic = { { "type_name", { "tau" } } };
		grammar<char> g(nts, ps, start, cc, gopt);

		lit<char> type_name_l = g.nt("type_name");
		size_t def_id = g.nt("def").n(), chars_id = g.nt("chars").n();

		// a registered pair is what makes _parse() call
		// sync_dynamic_context at all
		parser<char>::options popt;
		popt.dynamic_grow_nts = { { def_id, chars_id } };
		popt.on_dynamic_grow = [&](parser<char>::input& in, size_t,
			size_t from, size_t to)
		{
			g.add_dynamic_production_from(type_name_l,
				in.get_terminals(from, to));
		};
		parser<char> p(g, popt);

		dynamic_context<char> ctx; // deliberately empty
		parser<char>::parse_options po;
		po.dynamic_ctx = &ctx;
		po.start = type_name_l.n();
		CHECK(p.parse("tau", 3, po).found);
	}
}

TEST_SUITE("dynamic grow: add_dynamic dedupe") {

	TEST_CASE("a duplicated value in one add_dynamic call puts one "
		"copy in opt.dynamic")
	{
		nonterminals<char> nts;
		prods<char> ps, start(nts("start")), type_name(nts("type_name"));
		char_class_fns<char> cc{};
		ps(start, type_name);
		grammar<char> g(nts, ps, start, cc);

		g.add_dynamic("type_name", { "Point", "Point" });
		auto it = g.opt.dynamic.find("type_name");
		REQUIRE(it != g.opt.dynamic.end());
		CHECK(it->second.size() == 1);
		CHECK(it->second.front() == "Point");
	}
}

TEST_SUITE("dynamic grow: add_dynamic_production_from return value") {

	TEST_CASE("nullopt for an empty value, an index otherwise, the same "
		"index for a repeat still pending, and nullopt once committed")
	{
		def_fixture f;

		auto empty = f.g.add_dynamic_production_from(f.type_name_l, "");
		CHECK_FALSE(empty.has_value());

		auto first = f.g.add_dynamic_production_from(f.type_name_l,
			"Point");
		REQUIRE(first.has_value());

		// still pending (no parse has decided it yet): a repeat joins
		// the same hook instead of adding a second production
		auto repeat = f.g.add_dynamic_production_from(f.type_name_l,
			"Point");
		REQUIRE(repeat.has_value());
		CHECK(*repeat == *first);

		parser<char> p(f.g, f.popt);
		string in = "t Point=X";
		auto r = p.parse(in.c_str(), in.size());
		REQUIRE(r.found);

		// committed now: add_dynamic_production_from has nothing left
		// to add for the same (l, value) pair
		auto after_commit = f.g.add_dynamic_production_from(
			f.type_name_l, "Point");
		CHECK_FALSE(after_commit.has_value());
	}
}

TEST_SUITE("dynamic grow: pending hooks are always decided") {

	TEST_CASE("a value added directly, with no registered parent able "
		"to confirm it, is retired by the time the next parse returns")
	{
		// dynamic_grow_nts empty means on_dynamic_grow never fires
		// through the ordinary hook path; the direct call below mimics
		// application code adding a value outside any hook firing, as
		// grammar.md documents. Nothing can ever confirm it, so it
		// must not linger once a parse has run.
		def_fixture f;
		f.popt.dynamic_grow_nts.clear();
		parser<char> p(f.g, f.popt);

		f.g.add_dynamic_production_from(f.type_name_l, "Point");
		CHECK(f.g.has_pending_dynamic());

		string in = "t Point=X";
		auto r = p.parse(in.c_str(), in.size());
		REQUIRE(r.found);

		CHECK_FALSE(f.g.has_pending_dynamic());
		CHECK_FALSE(f.probes_ok(p, "Point"));
	}

	TEST_CASE("a direct add with no parse at all leaves the container "
		"pending, and the next parse call still decides it")
	{
		def_fixture f;
		f.popt.dynamic_grow_nts.clear();
		parser<char> p(f.g, f.popt);

		f.g.add_dynamic_production_from(f.type_name_l, "Point");
		CHECK(f.g.has_pending_dynamic());

		string in = "t Point=X";
		auto r = p.parse(in.c_str(), in.size());
		REQUIRE(r.found);
		CHECK_FALSE(f.g.has_pending_dynamic());
	}
}

TEST_SUITE("dynamic grow: a pending value gains a second child") {

	TEST_CASE("a value grown by one child and confirmed only through a "
		"different child's completed parent still survives")
	{
		// junk has two alternatives at the same input position: a
		// literal matching the whole "t Point=Y" text outright, and
		// attempt, which reaches the very same "Point" span through
		// chars_a but then requires "=Z" where the input has "=Y".
		// attempt's own derivation dies right after that mismatch, but
		// growing chars_a's span already happened the moment "=" was
		// scanned, strictly past chars_a's own span: the grow does not
		// need attempt to ever complete. def_b, later in the input,
		// reaches its own "Point" through chars_b and does complete,
		// so it is the only registered parent that ever confirms this
		// value. Before child_nts, the first hook for "Point" would be
		// tagged to chars_a alone, and def_b's confirmation would miss
		// it entirely.
		nonterminals<char> nts;
		prods<char> ps, start(nts("start")), junk(nts("junk")),
			attempt(nts("attempt")), chars_a(nts("chars_a")),
			def_b(nts("def_b")), chars_b(nts("chars_b")),
			alpha(nts("alpha"));
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "alpha" }, nts);
		ps(start, junk + " " + def_b);
		ps(junk, prods<char>("t Point=Y"));
		ps(junk, attempt);
		ps(attempt, prods<char>("t ") + chars_a + "=" + "Z");
		ps(chars_a, alpha);
		ps(chars_a, chars_a + alpha);
		ps(def_b, prods<char>("u ") + chars_b + "=" + "Y");
		ps(chars_b, alpha);
		ps(chars_b, chars_b + alpha);
		grammar<char> g(nts, ps, start, cc);

		lit<char> type_name_l = g.nt("type_name");
		size_t attempt_id = g.nt("attempt").n();
		size_t chars_a_id = g.nt("chars_a").n();
		size_t def_b_id = g.nt("def_b").n();
		size_t chars_b_id = g.nt("chars_b").n();

		parser<char>::options popt;
		popt.dynamic_grow_nts = { { attempt_id, chars_a_id },
			{ def_b_id, chars_b_id } };
		popt.on_dynamic_grow = [&](parser<char>::input& in, size_t,
			size_t from, size_t to)
		{
			g.add_dynamic_production_from(type_name_l,
				in.get_terminals(from, to));
		};
		parser<char> p(g, popt);

		string in = "t Point=Y u Point=Y";
		auto r = p.parse(in.c_str(), in.size());
		REQUIRE(r.found);

		parser<char>::parse_options probe;
		probe.start = type_name_l.n();
		CHECK(p.parse("Point", 5, probe).found);
	}
}

TEST_SUITE("dynamic grow: several splits reach one shared item") {

	TEST_CASE("five ambiguous pre/chars splits all confirm, only "
		"possible when their entries merge instead of overwrite")
	{
		// pre is nullable and left-recursive on alpha, so it can match
		// any prefix length of the run of letters between "t " and
		// chars. For "Point", pre/chars can split five ways: ""/"Point",
		// "P"/"oint", "Po"/"int", "Poi"/"nt", "Poin"/"t". Every split
		// advances def's own dot past chars to the identical item:
		// same production, same from, same dot, and the same set,
		// since chars always ends right before "=". So all five
		// completions of chars write their own entry onto that one
		// shared item. With an overwrite, only the last one survives to
		// fire and confirm; only a merge keeps all five.
		nonterminals<char> nts;
		prods<char> ps, start(nts("start")), def(nts("def")),
			pre(nts("pre")), chars(nts("chars")), alpha(nts("alpha")),
			nll(lit<char>{});
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "alpha" }, nts);
		ps(start, def);
		ps(def, prods<char>("t ") + pre + chars + "=" + "X");
		ps(pre, nll);
		ps(pre, pre + alpha);
		ps(chars, alpha);
		ps(chars, chars + alpha);
		grammar<char> g(nts, ps, start, cc);

		lit<char> type_name_l = g.nt("type_name");
		size_t def_id = g.nt("def").n(), chars_id = g.nt("chars").n();

		parser<char>::options popt;
		popt.dynamic_grow_nts = { { def_id, chars_id } };
		popt.on_dynamic_grow = [&](parser<char>::input& in, size_t,
			size_t from, size_t to)
		{
			g.add_dynamic_production_from(type_name_l,
				in.get_terminals(from, to));
		};
		parser<char> p(g, popt);

		string in = "t Point=X";
		auto r = p.parse(in.c_str(), in.size());
		REQUIRE(r.found);

		parser<char>::parse_options probe;
		probe.start = type_name_l.n();
		CHECK(p.parse("Point", 5, probe).found);
		CHECK(p.parse("oint", 4, probe).found);
		CHECK(p.parse("int", 3, probe).found);
		CHECK(p.parse("nt", 2, probe).found);
		CHECK(p.parse("t", 1, probe).found);
	}
}

TEST_SUITE("dynamic grow: conjunctive parent rejected by resolution") {

	TEST_CASE("a conjunctive parent that resolution rejects never "
		"confirms, even though the whole parse succeeds through "
		"another alternative")
	{
		// def's positive conjunct grows chars' span the same way any
		// other parent does, but the negated conjunct rules def's own
		// completion out once resolve_conjunctions() runs; the parse
		// still succeeds through alt, matching the same text outright.
		nonterminals<char> nts;
		prods<char> ps, start(nts("start")), def(nts("def")),
			alt(nts("alt")), chars(nts("chars")), alpha(nts("alpha"));
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "alpha" }, nts);
		ps(start, def | alt);
		ps(alt, prods<char>("t Point=X"));
		ps(def, (prods<char>("t ") + chars + "=" + "X") &
			~(prods<char>("t Point=X")));
		ps(chars, alpha);
		ps(chars, chars + alpha);
		grammar<char> g(nts, ps, start, cc);

		lit<char> type_name_l = g.nt("type_name");
		size_t def_id = g.nt("def").n(), chars_id = g.nt("chars").n();

		parser<char>::options popt;
		popt.dynamic_grow_nts = { { def_id, chars_id } };
		popt.on_dynamic_grow = [&](parser<char>::input& in, size_t,
			size_t from, size_t to)
		{
			g.add_dynamic_production_from(type_name_l,
				in.get_terminals(from, to));
		};
		parser<char> p(g, popt);

		string in = "t Point=X";
		auto r = p.parse(in.c_str(), in.size());
		REQUIRE(r.found);

		parser<char>::parse_options probe;
		probe.start = type_name_l.n();
		CHECK_FALSE(p.parse("Point", 5, probe).found);
	}
}

TEST_SUITE("dynamic grow: confirmation is tagged by parent, not just "
	"child")
{
	TEST_CASE("two parents sharing one registered child: only the one "
		"that actually completes over a value confirms it")
	{
		// def_a and def_b both register the same chars as their child,
		// and both grow into the same type_name. def_a's own item
		// grows "Point" the moment it scans "=" past chars, but the
		// next literal it needs is "Z"; the input has "Y" there, so
		// def_a's own item dies right after growing and never
		// completes. def_b, later in the input, grows and completes
		// over "Circle" through the same chars. Before grown_by
		// carried the parent half of the pair, confirm_dynamic matched
		// a hook by child id and text alone: since the two grown
		// values have different text, this case does not itself
		// distinguish the two schemes, but it pins the intended
		// behavior of the parent-tagged design going forward.
		nonterminals<char> nts;
		prods<char> ps, start(nts("start")), junk(nts("junk")),
			def_a(nts("def_a")), def_b(nts("def_b")),
			chars(nts("chars")), alpha(nts("alpha"));
		char_class_fns<char> cc = predefined_char_classes<char>(
			{ "alpha" }, nts);
		ps(start, junk + " " + def_b);
		ps(junk, prods<char>("t Point=Y"));
		ps(junk, def_a);
		ps(def_a, prods<char>("t ") + chars + "=" + "Z");
		ps(def_b, prods<char>("u ") + chars + "=" + "Y");
		ps(chars, alpha);
		ps(chars, chars + alpha);
		grammar<char> g(nts, ps, start, cc);

		lit<char> type_name_l = g.nt("type_name");
		size_t def_a_id = g.nt("def_a").n();
		size_t def_b_id = g.nt("def_b").n();
		size_t chars_id = g.nt("chars").n();

		parser<char>::options popt;
		popt.dynamic_grow_nts = { { def_a_id, chars_id },
			{ def_b_id, chars_id } };
		popt.on_dynamic_grow = [&](parser<char>::input& in, size_t,
			size_t from, size_t to)
		{
			g.add_dynamic_production_from(type_name_l,
				in.get_terminals(from, to));
		};
		parser<char> p(g, popt);

		string in = "t Point=Y u Circle=Y";
		auto r = p.parse(in.c_str(), in.size());
		REQUIRE(r.found);

		parser<char>::parse_options probe;
		probe.start = type_name_l.n();
		CHECK_FALSE(p.parse("Point", 5, probe).found);
		CHECK(p.parse("Circle", 6, probe).found);
	}
}

TEST_SUITE("dynamic grow: derive_all skips retired productions") {

	TEST_CASE("a retired one-character value no longer derives through "
		"derive_all")
	{
		// a single-character dynamic value is a unit rule (one
		// terminal literal as the whole body), which is exactly the
		// shape derive_all's own unit-rule scan matches against.
		nonterminals<char> nts;
		prods<char> ps, start(nts("start")), type_name(nts("type_name"));
		char_class_fns<char> cc{};
		ps(start, type_name);
		grammar<char> g(nts, ps, start, cc);

		lit<char> type_name_l = g.nt("type_name");
		auto idx = g.add_dynamic_production_from(type_name_l, "X");
		REQUIRE(idx.has_value());
		g.rollback_dynamic(); // retires it: G keeps the production

		lit<char> x_lit('X');
		auto derived = g.derive_all({ { x_lit, 0 } });

		bool found = false;
		for (auto& [l, sp] : derived)
			if (l == type_name_l && sp[0] == 0 && sp[1] == 1)
				found = true;
		CHECK_FALSE(found);
	}
}
