// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// The grammar owns its nonterminal symbol table. A caller never creates or
// passes a `nonterminals` for the common path. A saved `nts_ptr()` keeps the
// table, and any lit that points into it, alive after the grammar is gone.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"

#include <memory>
#include <string>

#include "format/tgf/tgf.h"

using namespace std;
using namespace idni;

static const string source =
	"@use char classes digit.\n"
	"start => num.\n"
	"num => digit+.\n";

TEST_SUITE("grammar ownership") {

TEST_CASE("tgf creates and owns the symbol table") {
	auto gr = tgf<char>::from_string(source);
	REQUIRE(gr.has_value());
	grammar<char> g = std::move(gr).value();
	CHECK(g.nts_ptr() != nullptr);
	parser<char> p(g);
	auto r = p.parse("12", 2);
	REQUIRE(r.found);
}

TEST_CASE("a saved table handle keeps a lit valid after the grammar") {
	shared_ptr<nonterminals<char>> keep;
	lit<char> start;
	{
		auto gr = tgf<char>::from_string(source);
		REQUIRE(gr.has_value());
		grammar<char> g = std::move(gr).value();
		keep = g.nts_ptr();
		start = g.nt("start");
		// the grammar and `keep` both hold the table
		CHECK(keep.use_count() >= 2);
	}
	REQUIRE(keep != nullptr);
	// the grammar is gone. The lit still resolves through the table.
	CHECK(start.to_std_string() == "start");
	CHECK(keep->find(string("num")).has_value());
	CHECK(!keep->find(string("nope")).has_value());
}

TEST_CASE("a null table handle makes the grammar create one") {
	auto gr = tgf<char>::from_string(shared_ptr<nonterminals<char>>{},
		source);
	REQUIRE(gr.has_value());
	grammar<char> g = std::move(gr).value();
	CHECK(g.nts_ptr() != nullptr);
	CHECK(g.nts_ptr().use_count() >= 1);
	grammar<char> empty(shared_ptr<nonterminals<char>>{});
	CHECK(empty.nts_ptr() != nullptr);
}

TEST_CASE("a caller-provided table still works") {
	nonterminals<char> nts;
	auto gr = tgf<char>::from_string(nts, source);
	REQUIRE(gr.has_value());
	grammar<char> g = std::move(gr).value();
	// the grammar aliases the caller's table, so it does not own it
	CHECK(g.nts_ptr().use_count() == 0);
	CHECK(&g.get_nts() == &nts);
}

TEST_CASE("a parser can own the grammar") {
	auto gr = tgf<char>::from_string(source);
	REQUIRE(gr.has_value());
	auto gp = std::make_shared<grammar<char>>(std::move(gr).value());
	parser<char> p(gp);
	auto r = p.parse("12", 2);
	REQUIRE(r.found);
	// the parser keeps the grammar, and its table, alive
	gp.reset();
	CHECK(p.get_grammar().nts_ptr() != nullptr);
	CHECK(r.get_shaped_tree2() != nullptr);
}

} // TEST_SUITE
