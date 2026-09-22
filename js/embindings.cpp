// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "parser.h"
#include "utility/charclasses.h"
#include "tgf/tgf_cli.h"

namespace em = emscripten;

// =========================================================================
// Type aliases
// =========================================================================

using nts_t      = idni::nonterminals<char, char>;
using lit_t      = idni::lit<char, char>;
using prods_t    = idni::prods<char, char>;
using grammar_t  = idni::grammar<char, char>;
using parser_t   = idni::parser<char, char>;
using result_t   = parser_t::result;
using tree_t     = parser_t::tree;
using trv_t      = tree_t::traverser;
using pnode_t    = idni::pnode_type<char, char>;
using pforest_t  = parser_t::pforest;
using tref_t     = idni::tref;

// =========================================================================
// lit_wrap -- a read-only literal (terminal, nonterminal, or null)
// =========================================================================

struct lit_wrap {
	lit_t l;

	lit_wrap() : l{} {}
	explicit lit_wrap(lit_t l_) : l(l_) {}
	explicit lit_wrap(const prods_t& p) : l(p.to_lit()) {}

	bool   is_nt()       const { return l.nt(); }
	bool   is_terminal() const { return !l.nt() && !l.is_null(); }
	bool   is_null()     const { return l.is_null(); }
	size_t nt()          const { return l.n(); }
	char   t()           const { return l.t(); }
	std::string to_string() const { return l.to_std_string(); }
};

// =========================================================================
// prods_wrap -- a production body
// =========================================================================

struct prods_wrap {
	prods_t p;

	prods_wrap() = default;
	explicit prods_wrap(prods_t p_) : p(p_) {}

	static prods_wrap from_lit(const lit_wrap& lt) {
		return prods_wrap(prods_t(lt.l));
	}
	static prods_wrap from_char(char c) {
		return prods_wrap(prods_t(lit_t(c)));
	}
	static prods_wrap from_string(const std::string& s) {
		return prods_wrap(prods_t(idni::from_str<char>(s)));
	}

	lit_wrap to_lit() const { return lit_wrap(p.to_lit()); }
};

static prods_wrap prods_or(const prods_wrap& a, const prods_wrap& b) {
	return prods_wrap(a.p | b.p);
}
static prods_wrap prods_and(const prods_wrap& a, const prods_wrap& b) {
	return prods_wrap(a.p & b.p);
}
static prods_wrap prods_concat(const prods_wrap& a, const prods_wrap& b) {
	return prods_wrap(a.p + b.p);
}
static prods_wrap prods_not(const prods_wrap& p) {
	return prods_wrap(~p.p);
}

// =========================================================================
// grammar_wrap -- owns nonterminals + grammar, creates parsers
// =========================================================================

struct grammar_wrap {
	std::unique_ptr<nts_t>     nts;
	std::unique_ptr<grammar_t> g;
	bool auto_disambiguate_ = true;
	bool auto_disambiguate_set_ = false;
	std::set<size_t> nodisambig_;

	grammar_wrap() : nts(std::make_unique<nts_t>()) {}

	// an explicit setter call always wins, whether it came before the
	// grammar existed or after a TGF directive already set its own value
	bool get_auto_disambiguate() const {
		return g ? g->opt.auto_disambiguate : auto_disambiguate_;
	}
	void set_auto_disambiguate(bool v) {
		auto_disambiguate_ = v;
		auto_disambiguate_set_ = true;
		if (g) g->opt.auto_disambiguate = v;
	}
	void add_nodisambig(const std::string& name) {
		size_t id = nts->get(idni::from_str<char>(name));
		nodisambig_.insert(id);
		if (g) g->opt.nodisambig_list.insert(id);
	}

	bool from_tgf_string(const std::string& src) {
		auto local_nts = std::make_unique<nts_t>();
		auto r = idni::tgf<char, char>::from_string(*local_nts, src);
		if (!r.has_value()) {
			g.reset();
			nts = std::make_unique<nts_t>();
			return false;
		}
		nts = std::move(local_nts);
		g = std::make_unique<grammar_t>(std::move(r).value());
		apply_pending_options();
		return true;
	}
	bool from_tgf_file(const std::string& filename) {
		auto local_nts = std::make_unique<nts_t>();
		auto r = idni::tgf<char, char>::from_file(*local_nts, filename);
		if (!r.has_value()) {
			g.reset();
			nts = std::make_unique<nts_t>();
			return false;
		}
		nts = std::move(local_nts);
		g = std::make_unique<grammar_t>(std::move(r).value());
		apply_pending_options();
		return true;
	}

	bool good() const { return g != nullptr; }

	size_t nt(const std::string& name) {
		return nts->get(idni::from_str<char>(name));
	}
	std::string nt_name(size_t id) const {
		return idni::to_std_string(nts->get(id));
	}
	lit_wrap nt_lit(const std::string& name) {
		return lit_wrap((*nts)(idni::from_str<char>(name)));
	}
	lit_wrap nt_lit_by_id(size_t id) {
		return lit_wrap((*nts)(id));
	}
	prods_wrap lit(const std::string& name) {
		return prods_wrap(prods_t(
			(*nts)(idni::from_str<char>(name))));
	}

	void add_rule(const std::string& head_name, const prods_wrap& body) {
		size_t hid = nts->get(idni::from_str<char>(head_name));
		prods_t head = prods_t(lit_t(hid, nts.get()));
		ps(head, body.p);
	}
	bool build(const std::string& start_symbol) {
		idni::char_class_fns<char> cc{};
		auto start = prods_t(
			(*nts)(idni::from_str<char>(start_symbol)));
		g = std::make_unique<grammar_t>(*nts, ps, start, cc);
		apply_pending_options();
		return true;
	}

	size_t size() const { return g ? g->size() : 0; }

	lit_wrap start_literal() const {
		if (!g) return lit_wrap();
		return lit_wrap(g->start_literal());
	}

	std::string production_to_string(size_t idx) const {
		if (!g) return "";
		std::ostringstream os;
		g->print_production(os, idx, false);
		return os.str();
	}

	std::string internal_grammar() const {
		if (!g) return "";
		std::ostringstream os;
		g->print_internal_grammar(os, "", false);
		return os.str();
	}

	std::vector<size_t> unreachable(const std::string& sym) const {
		if (!g) return {};
		auto s = g->unreachable_productions(
			g->nt(idni::from_str<char>(sym)));
		return {s.begin(), s.end()};
	}

	const grammar_t& grammar() const { return *g; }
	const nts_t& nonterminals() const { return *nts; }

private:
	prods_t ps;

	void apply_pending_options() {
		if (auto_disambiguate_set_) g->opt.auto_disambiguate = auto_disambiguate_;
		for (size_t id : nodisambig_) g->opt.nodisambig_list.insert(id);
	}
};

// =========================================================================
// shaping_options_wrap (must precede parse_options_wrap and parse_result_wrap)
// =========================================================================

struct shaping_options_wrap {
	idni::shaping_options opts;

	bool get_trim_terminals()     const { return opts.trim_terminals; }
	void set_trim_terminals(bool v)     { opts.trim_terminals = v; }
	bool get_inline_char_classes()const { return opts.inline_char_classes; }
	void set_inline_char_classes(bool v){ opts.inline_char_classes = v; }

	std::vector<size_t> get_to_trim() const {
		return {opts.to_trim.begin(), opts.to_trim.end()};
	}
	void add_to_trim(size_t nt)   { opts.to_trim.insert(nt); }
	void remove_to_trim(size_t nt){ opts.to_trim.erase(nt); }
};

// =========================================================================
// parse_options_wrap
// =========================================================================

struct parse_options_wrap {
	parser_t::parse_options po;
	std::optional<shaping_options_wrap> shaping_opt;

	shaping_options_wrap get_shaping() const {
		return shaping_opt.value_or(shaping_options_wrap());
	}
	void set_shaping(const shaping_options_wrap& s) { shaping_opt = s; }

	size_t get_start()        const { return po.start; }
	void   set_start(size_t s)      { po.start = s; }
	bool   get_measure()      const { return po.measure; }
	void   set_measure(bool m)      { po.measure = m; }
	bool   get_debug()        const { return po.debug; }
	void   set_debug(bool d)        { po.debug = d; }

	std::string get_tree_path() const {
		return po.tree_path == idni::parse_tree_path::forest_path
			? "forest" : "bintree";
	}
	void set_tree_path(const std::string& tp) {
		po.tree_path = tp == "forest"
			? idni::parse_tree_path::forest_path
			: idni::parse_tree_path::bintree_path;
	}

	std::string get_error_verbosity() const {
		switch (po.error_verbosity) {
		case parser_t::error::INFO_BASIC:      return "basic";
		case parser_t::error::INFO_DETAILED:   return "detailed";
		case parser_t::error::INFO_ROOT_CAUSE: return "root-cause";
		}
		return "basic";
	}
	void set_error_verbosity(const std::string& ev) {
		if (ev == "detailed")
			po.error_verbosity = parser_t::error::INFO_DETAILED;
		else if (ev == "root-cause")
			po.error_verbosity = parser_t::error::INFO_ROOT_CAUSE;
		else
			po.error_verbosity = parser_t::error::INFO_BASIC;
	}
};

// =========================================================================
// tree_wrap -- wraps parser::tree::traverser (must precede parse_result_wrap)
// =========================================================================

struct tree_wrap {
	trv_t t;

	tree_wrap() = default;
	explicit tree_wrap(trv_t t_) : t(t_) {}
	explicit tree_wrap(tref_t ref) : t(trv_t(ref)) {}

	bool has_value() const { return t.has_value(); }

	tree_wrap find(size_t nt) const {
		return tree_wrap(t | nt | trv_t::only_child);
	}
	tree_wrap only_child() const {
		return tree_wrap(t | trv_t::only_child);
	}
	tree_wrap first() const {
		return tree_wrap(t | trv_t::first | trv_t::only_child);
	}
	tree_wrap second() const {
		return tree_wrap(t | trv_t::second | trv_t::only_child);
	}

	std::vector<tree_wrap> find_all(size_t nt) const {
		std::vector<tree_wrap> r;
		if (!t.has_value()) return r;
		auto matches = (t || nt)();
		for (const auto& m : matches)
			r.emplace_back(m | trv_t::only_child);
		return r;
	}

	std::vector<tree_wrap> children() const {
		std::vector<tree_wrap> r;
		if (!t.has_value()) return r;
		auto ch = (t | trv_t::children)();
		for (const auto& c : ch)
			r.emplace_back(c);
		return r;
	}

	std::string text() const { return t | trv_t::terminals; }
	size_t nt()  const { return t | trv_t::nonterminal; }

	bool is_nt() const {
		if (!t.has_value()) return false;
		return t.value_tree().value.first.nt();
	}
	bool is_terminal() const {
		if (!t.has_value()) return false;
		const auto& v = t.value_tree().value.first;
		return !v.nt() && !v.is_null();
	}
	bool is_null() const {
		if (!t.has_value()) return false;
		return t.value_tree().value.first.is_null();
	}
	std::vector<size_t> location() const {
		if (!t.has_value()) return {};
		auto loc = t.value_tree().value.second;
		return {loc[0], loc[1]};
	}
};

// =========================================================================
// parse_result_wrap
// =========================================================================

struct parse_result_wrap {
	std::shared_ptr<result_t> r;
	std::string               input_;
	std::optional<idni::shaping_options> shaping_override;

	parse_result_wrap() = default;
	explicit parse_result_wrap(std::shared_ptr<result_t> r_,
		std::string input__)
		: r(std::move(r_)), input_(std::move(input__)) {}
	explicit parse_result_wrap(std::shared_ptr<result_t> r_,
		std::string input__, idni::shaping_options shaping_)
		: r(std::move(r_)), input_(std::move(input__)),
			shaping_override(shaping_) {}

	bool good()                 const { return r && r->good(); }
	bool found()                const { return r && r->found; }
	bool is_ambiguous()         const { return r && r->is_ambiguous(); }
	bool has_single_parse_tree()const { return r && r->has_single_parse_tree(); }

	std::string get_input()     const { return r ? r->get_input() : ""; }
	std::string get_terminals() const { return r ? r->get_terminals() : ""; }
	std::string error_string()  const {
		if (!r || r->found) return "";
		return r->parse_error.to_str();
	}

	tree_wrap get_bintree() const {
		if (!r || !r->found) return tree_wrap();
		return tree_wrap(r->get_bintree());
	}
	tree_wrap get_shaped_bintree() const {
		if (!r || !r->found) return tree_wrap();
		return tree_wrap(shaping_override
			? r->get_shaped_bintree(*shaping_override)
			: r->get_shaped_bintree());
	}

	std::string report_string() const {
		if (!r) return "";
		std::ostringstream os;
		r->report().print(os);
		return os.str();
	}
	bool has_diagnostics() const {
		return r && r->report().has_error();
	}
};

// =========================================================================
// parser_wrap -- owns a grammar + parser
// =========================================================================

struct parser_wrap {
	std::unique_ptr<nts_t>     owned_nts;
	std::unique_ptr<grammar_t> owned_g;
	std::unique_ptr<parser_t>  owned_p;

	parser_wrap() = default;

	bool init(grammar_wrap& gram) {
		if (!gram.good()) return false;
		owned_nts = std::move(gram.nts);
		owned_g   = std::move(gram.g);
		owned_p   = std::make_unique<parser_t>(*owned_g,
			idni::default_parser_options<char, char>());
		return true;
	}

	bool from_tgf_string(const std::string& src) {
		auto local_nts = std::make_unique<nts_t>();
		auto r = idni::tgf<char, char>::from_string(*local_nts, src);
		if (!r.has_value()) {
			owned_p.reset();
			owned_g.reset();
			owned_nts.reset();
			return false;
		}
		owned_nts = std::move(local_nts);
		owned_g = std::make_unique<grammar_t>(std::move(r).value());
		owned_p = std::make_unique<parser_t>(*owned_g,
			idni::default_parser_options<char, char>());
		return true;
	}

	bool from_tgf_file(const std::string& filename) {
		auto local_nts = std::make_unique<nts_t>();
		auto r = idni::tgf<char, char>::from_file(*local_nts, filename);
		if (!r.has_value()) {
			owned_p.reset();
			owned_g.reset();
			owned_nts.reset();
			return false;
		}
		owned_nts = std::move(local_nts);
		owned_g = std::make_unique<grammar_t>(std::move(r).value());
		owned_p = std::make_unique<parser_t>(*owned_g,
			idni::default_parser_options<char, char>());
		return true;
	}

	bool good() const { return owned_p != nullptr; }

	parse_result_wrap parse(const std::string& input) {
		if (!owned_p) return parse_result_wrap();
		auto r = owned_p->parse(input.c_str(), input.size());
		return parse_result_wrap(
			std::make_shared<result_t>(std::move(r)), input);
	}

	parse_result_wrap parse_with_options(const std::string& input,
		const parse_options_wrap& popts)
	{
		if (!owned_p) return parse_result_wrap();
		auto r = owned_p->parse(input.c_str(), input.size(), popts.po);
		auto rr = std::make_shared<result_t>(std::move(r));
		if (popts.shaping_opt)
			return parse_result_wrap(rr, input, popts.shaping_opt->opts);
		return parse_result_wrap(rr, input);
	}

	const grammar_t& grammar() const { return *owned_g; }
};

// =========================================================================
// tgf_repl_wrap -- full TGF REPL evaluator
// =========================================================================

struct tgf_repl_wrap {
	idni::tgf_repl_evaluator* re = nullptr;
	std::string load_diag;

	tgf_repl_wrap(const std::string& grammar_path) {
		std::ostringstream cap;
		auto* old_out = std::cout.rdbuf(cap.rdbuf());
		auto* old_err = std::cerr.rdbuf(cap.rdbuf());
		re = new idni::tgf_repl_evaluator(grammar_path);
		re->flush_report();
		std::cout.rdbuf(old_out);
		std::cerr.rdbuf(old_err);
		load_diag = cap.str();
	}
	~tgf_repl_wrap() { delete re; }

	bool good() const { return re && re->good(); }
	const std::string& diagnostics() const { return load_diag; }

	int eval(const std::string& command) {
		return re->eval(command).value_or(0);
	}

	std::string eval_capture(const std::string& command) {
		std::ostringstream cap;
		auto* old = std::cout.rdbuf(cap.rdbuf());
		re->eval(command);
		std::cout.rdbuf(old);
		return cap.str();
	}

	void reprompt() { re->reprompt(); }
	const std::string& filename() const { return re->filename(); }
	bool reload(const std::string& new_tgf_file) {
		return re->reload(new_tgf_file);
	}
};

// =========================================================================
// Legacy API
// =========================================================================

static std::string run_commands(const std::string& grammar_path,
                                const em::val& commands)
{
	tgf_repl_wrap node(grammar_path);
	if (!node.good())
		return "ERROR: failed to load grammar " + grammar_path;
	std::ostringstream all;
	const int n = commands["length"].as<int>();
	for (int i = 0; i < n; ++i) {
		std::string cmd = commands[i].as<std::string>();
		all << node.eval_capture(cmd);
	}
	return all.str();
}

// =========================================================================
// Embind registrations
// =========================================================================

EMSCRIPTEN_BINDINGS(tauparser) {

	// -- lit_wrap ------------------------------------------------------

	emscripten::class_<lit_wrap>("lit")
		.constructor<>()
		.function("is_nt",       &lit_wrap::is_nt)
		.function("is_terminal", &lit_wrap::is_terminal)
		.function("is_null",     &lit_wrap::is_null)
		.function("nt",          &lit_wrap::nt)
		.function("t",           &lit_wrap::t)
		.function("to_string",   &lit_wrap::to_string)
		;

	// -- prods_wrap ----------------------------------------------------

	emscripten::class_<prods_wrap>("prods")
		.constructor<>()
		.class_function("from_lit",    &prods_wrap::from_lit)
		.class_function("from_char",   &prods_wrap::from_char)
		.class_function("from_string", &prods_wrap::from_string)
		.function("to_lit",            &prods_wrap::to_lit)
		;

	emscripten::function("prods_or",     &prods_or);
	emscripten::function("prods_and",    &prods_and);
	emscripten::function("prods_concat", &prods_concat);
	emscripten::function("prods_not",    &prods_not);

	// -- grammar_wrap --------------------------------------------------

	emscripten::class_<grammar_wrap>("grammar")
		.constructor<>()
		.function("from_tgf_string",     &grammar_wrap::from_tgf_string)
		.function("from_tgf_file",       &grammar_wrap::from_tgf_file)
		.function("good",                &grammar_wrap::good)
		.function("nt",                  &grammar_wrap::nt)
		.function("nt_name",             &grammar_wrap::nt_name)
		.function("nt_lit",              &grammar_wrap::nt_lit)
		.function("nt_lit_by_id",        &grammar_wrap::nt_lit_by_id)
		.function("lit",                 &grammar_wrap::lit)
		.function("add_rule",            &grammar_wrap::add_rule)
		.function("build",               &grammar_wrap::build)
		.function("size",                &grammar_wrap::size)
		.function("start_literal",       &grammar_wrap::start_literal)
		.function("production_to_string",&grammar_wrap::production_to_string)
		.function("internal_grammar",    &grammar_wrap::internal_grammar)
		.function("unreachable",         &grammar_wrap::unreachable)
		.property("auto_disambiguate",   &grammar_wrap::get_auto_disambiguate,
		                                 &grammar_wrap::set_auto_disambiguate)
		.function("add_nodisambig",      &grammar_wrap::add_nodisambig)
		;

	// -- parse_options_wrap --------------------------------------------

	emscripten::class_<parse_options_wrap>("parse_options")
		.constructor<>()
		.property("start", &parse_options_wrap::get_start,
		                   &parse_options_wrap::set_start)
		.property("measure",&parse_options_wrap::get_measure,
		                   &parse_options_wrap::set_measure)
		.property("debug", &parse_options_wrap::get_debug,
		                   &parse_options_wrap::set_debug)
		.property("tree_path", &parse_options_wrap::get_tree_path,
		                       &parse_options_wrap::set_tree_path)
		.property("error_verbosity",
			&parse_options_wrap::get_error_verbosity,
			&parse_options_wrap::set_error_verbosity)
		.property("shaping", &parse_options_wrap::get_shaping,
		                    &parse_options_wrap::set_shaping)
		;

	// -- tree_wrap -----------------------------------------------------

	emscripten::class_<tree_wrap>("tree")
		.constructor<>()
		.function("has_value",   &tree_wrap::has_value)
		.function("find",        &tree_wrap::find)
		.function("find_all",    &tree_wrap::find_all)
		.function("only_child",  &tree_wrap::only_child)
		.function("first",       &tree_wrap::first)
		.function("second",      &tree_wrap::second)
		.function("children",    &tree_wrap::children)
		.function("text",        &tree_wrap::text)
		.function("nt",          &tree_wrap::nt)
		.function("is_nt",       &tree_wrap::is_nt)
		.function("is_terminal", &tree_wrap::is_terminal)
		.function("is_null",     &tree_wrap::is_null)
		.function("location",    &tree_wrap::location)
		;

	// -- parse_result_wrap ---------------------------------------------

	emscripten::class_<parse_result_wrap>("parse_result")
		.constructor<>()
		.function("good",              &parse_result_wrap::good)
		.function("found",             &parse_result_wrap::found)
		.function("is_ambiguous",      &parse_result_wrap::is_ambiguous)
		.function("has_single_parse_tree",
			&parse_result_wrap::has_single_parse_tree)
		.function("get_input",         &parse_result_wrap::get_input)
		.function("get_terminals",     &parse_result_wrap::get_terminals)
		.function("error_string",      &parse_result_wrap::error_string)
		.function("get_bintree",       &parse_result_wrap::get_bintree)
		.function("get_shaped_bintree",&parse_result_wrap::get_shaped_bintree)
		.function("report_string",     &parse_result_wrap::report_string)
		.function("has_diagnostics",   &parse_result_wrap::has_diagnostics)
		;

	// -- shaping_options_wrap ------------------------------------------

	emscripten::class_<shaping_options_wrap>("shaping_options")
		.constructor<>()
		.property("trim_terminals",
			&shaping_options_wrap::get_trim_terminals,
			&shaping_options_wrap::set_trim_terminals)
		.property("inline_char_classes",
			&shaping_options_wrap::get_inline_char_classes,
			&shaping_options_wrap::set_inline_char_classes)
		.function("add_to_trim",    &shaping_options_wrap::add_to_trim)
		.function("remove_to_trim", &shaping_options_wrap::remove_to_trim)
		.function("get_to_trim",    &shaping_options_wrap::get_to_trim)
		;

	// -- parser_wrap ---------------------------------------------------

	emscripten::class_<parser_wrap>("parser")
		.constructor<>()
		.function("init",              &parser_wrap::init)
		.function("from_tgf_string",   &parser_wrap::from_tgf_string)
		.function("from_tgf_file",     &parser_wrap::from_tgf_file)
		.function("good",              &parser_wrap::good)
		.function("parse",             &parser_wrap::parse)
		.function("parse_with_options",&parser_wrap::parse_with_options)
		;

	// -- tgf_repl_wrap -------------------------------------------------

	emscripten::class_<tgf_repl_wrap>("tgf_repl")
		.constructor<const std::string&>()
		.function("good",         &tgf_repl_wrap::good)
		.function("diagnostics",  &tgf_repl_wrap::diagnostics)
		.function("eval",         &tgf_repl_wrap::eval)
		.function("eval_capture", &tgf_repl_wrap::eval_capture)
		.function("reprompt",     &tgf_repl_wrap::reprompt)
		.function("filename",     &tgf_repl_wrap::filename)
		.function("reload",       &tgf_repl_wrap::reload)
		;

	// -- legacy API ----------------------------------------------------

	emscripten::function("runCommands", &run_commands);

	emscripten::function("version",
		emscripten::optional_override([]() -> std::string {
		return std::string("tauparser-js "
			+ std::string(idni::tauparser::version));
	}));

	emscripten::function("test_isprint",
		emscripten::optional_override([](int c) -> bool {
		return idni::charclasses::isprint<char>(static_cast<char>(c));
	}));

	emscripten::function("load_grammar",
		emscripten::optional_override([](const std::string& path)
			-> std::string
	{
		idni::tgf_repl_evaluator re(path);
		std::ostringstream cap;
		auto* old_out = std::cout.rdbuf(cap.rdbuf());
		auto* old_err = std::cerr.rdbuf(cap.rdbuf());
		re.flush_report();
		std::cout.rdbuf(old_out);
		std::cerr.rdbuf(old_err);
		return std::to_string(re.good()) + "\n" + cap.str();
	}));

	emscripten::function("parse_grammar_str",
		emscripten::optional_override([](const std::string& src)
			-> std::string
	{
		nts_t nts;
		auto r = idni::tgf<char, char>::from_string(nts, src);
		std::ostringstream cap;
		auto* old_out = std::cout.rdbuf(cap.rdbuf());
		auto* old_err = std::cerr.rdbuf(cap.rdbuf());
		r.report().print();
		std::cout.rdbuf(old_out);
		std::cerr.rdbuf(old_err);
		return std::to_string(r.has_value()) + "\n" + cap.str();
	}));

	// -- vector registrations ------------------------------------------

	emscripten::register_vector<std::string>("vector<string>");
	emscripten::register_vector<size_t>("vector<size_t>");
	emscripten::register_vector<tree_wrap>("vector<tree>");
	emscripten::register_vector<lit_wrap>("vector<lit>");
}
