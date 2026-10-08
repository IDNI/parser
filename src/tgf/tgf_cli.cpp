// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <map>
#include <optional>
#include <ranges>
#include <sstream>
#include <string_view>

#include "parser.h"
#include "parser_gen.h"
#include "parser_strings.h"
#include "parser_term_color_macros.h"
#include "recoders.h"
#include "format/tgf/tgf.h"
#include "tgf_cli.h"
#ifdef TAU_PARSER_BUILD_SERVE
#include "tgf_serve.h"
#include "tgf_connect.h"
#endif
#include "format/tgf.test/tgf_test.h"
#ifndef DEBUG
#include "utility/devhelpers.h"
#endif
#include "defs.h"
#include "format/json/json.h"
#include "format/ast.json/ast_json.h"

namespace idni {

using namespace std;

using tt = tgf_repl_parser::tree::traverser;

static format::json::value range_value(const std::array<size_t, 2>& range) {
	using value = format::json::value;
	value v = value::array();
	v.push_back(value::number(static_cast<double>(range[0])));
	v.push_back(value::number(static_cast<double>(range[1])));
	return v;
}

// Text form of an option value: on/off, a number, a text, or a list
// joined with ", ".
static std::string option_text(const format::json::value& v) {
	if (v.is_bool()) return v.as_bool() ? "on" : "off";
	if (v.is_number()) return std::to_string(
		static_cast<size_t>(v.as_number()));
	if (v.is_string()) return v.as_string();
	if (!v.is_array()) return {};
	if (v.size() == 0) return "(empty)";
	std::stringstream ss;
	bool first = true;
	for (const auto& e : v)
		ss << (first ? first = false, "" : ", ") << e.as_string();
	return ss.str();
}

// Long command name for a command nonterminal.
static const char* command_name(size_t nt) {
	using p = tgf_repl_parser;
	switch (nt) {
	case p::parse_cmd:       return "parse";
	case p::parse_file_cmd:  return "parse file";
	case p::grammar_cmd:     return "grammar";
	case p::igrammar_cmd:    return "internal-grammar";
	case p::start_cmd:       return "start";
	case p::unreachable_cmd: return "unreachable";
	case p::reload_cmd:      return "reload";
	case p::load_cmd:        return "load";
	case p::help_cmd:        return "help";
	case p::version_cmd:     return "version";
	case p::license_cmd:     return "license";
	case p::quit_cmd:        return "quit";
	case p::clear_cmd:       return "clear";
	case p::get_cmd:         return "get";
	case p::set_cmd:         return "set";
	case p::toggle_cmd:      return "toggle";
	case p::enable_cmd:      return "enable";
	case p::disable_cmd:     return "disable";
	case p::add_cmd:         return "add";
	case p::del_cmd:         return "delete";
	default:                 return "";
	}
}

// Long command name for a help argument nonterminal.
static const char* help_arg_name(size_t nt) {
	using p = tgf_repl_parser;
	switch (nt) {
	case p::grammar_sym:     return "grammar";
	case p::igrammar_sym:    return "internal-grammar";
	case p::unreachable_sym: return "unreachable";
	case p::start_sym:       return "start";
	case p::parse_sym:       return "parse";
	case p::parse_file_sym:  return "parse file";
	case p::load_sym:        return "load";
	case p::reload_sym:      return "reload";
	case p::clear_sym:       return "clear";
	case p::help_sym:        return "help";
	case p::quit_sym:        return "quit";
	case p::version_sym:     return "version";
	case p::license_sym:     return "license";
	case p::get_sym:         return "get";
	case p::set_sym:         return "set";
	case p::add_sym:         return "add";
	case p::del_sym:         return "delete";
	case p::toggle_sym:      return "toggle";
	case p::enable_sym:      return "enable";
	case p::disable_sym:     return "disable";
	default:                 return "help";
	}
}

const char* cmd_status_name(cmd_status s) {
	switch (s) {
	case cmd_status::ok:         return "ok";
	case cmd_status::error:      return "error";
	case cmd_status::incomplete: return "incomplete";
	case cmd_status::quit:       return "quit";
	}
	return "ok";
}

void print_diagnostics_report(const idni::diagnostics::report& report,
	bool json, bool print_names)
{
	if (report.nodes().empty()) return;
	if (json) format::json::print(report, std::cout, print_names) << '\n';
	else report.print();
}

// bintree<pnode>::M() and its hit/miss/geth counters are process-wide
// statics (shared by grammar loading, the target parse and post-parse
// tree shaping), so they are read here once, not folded into per-parse
// diagnostics::report counters.
static void print_bintree_process_totals() {
	using tree_t = tgf_repl_evaluator::parser_type::tree;
	size_t buckets = 0, entries = 0, max_chain = 0;
	double load_factor = 0.0, mean_chain = 0.0;
	std::array<size_t, 8> chain_len_histogram{};
	tree_t::bucket_stats(buckets, entries, load_factor, max_chain,
		mean_chain, &chain_len_histogram);
	cout << "bintree lookups (whole process): get hits: "
		<< tree_t::get_hits() << ", get misses: "
		<< tree_t::get_misses() << ", geth calls: "
		<< tree_t::geth_calls() << "\n";
	cout << "bintree M() buckets: " << buckets
		<< ", entries: " << entries
		<< ", load factor: " << load_factor
		<< ", max chain: " << max_chain
		<< ", mean chain: " << mean_chain << "\n";
	static constexpr const char* bin_names[8] = {
		"0", "1", "2", "3", "4-7", "8-15", "16-63", "64+" };
	cout << "bintree M() chain length histogram:";
	for (size_t i = 0; i != 8; ++i)
		cout << " [" << bin_names[i] << "]=" << chain_len_histogram[i];
	cout << "\n";
	size_t distinct_hashes = 0, largest_group_size = 0;
	std::array<size_t, 5> top_group_sizes{}, top_group_triples{};
	size_t distinct_values = 0, distinct_child_pairs = 0, leaf_count = 0;
	std::vector<tree_t::hash_group_sample> samples;
	size_t stale_hash_count = 0, largest_group_stale_count = 0;
	std::uint64_t largest_group_hash = 0;
	tree_t::hash_group_stats(distinct_hashes, largest_group_size,
		top_group_sizes, top_group_triples, distinct_values,
		distinct_child_pairs, leaf_count, samples,
		stale_hash_count, largest_group_hash,
		largest_group_stale_count);
	cout << "bintree M() hash groups: distinct hashes: " << distinct_hashes
		<< ", largest group size: " << largest_group_size << "\n";
	cout << "bintree M() top 5 hash groups (size/distinct (value,l,r) triples):";
	for (size_t i = 0; i != 5; ++i)
		cout << " [" << top_group_sizes[i] << "/"
			<< top_group_triples[i] << "]";
	cout << "\n";
	cout << "bintree M() largest hash group: distinct values: "
		<< distinct_values << ", distinct child pairs: "
		<< distinct_child_pairs << ", leaf count: " << leaf_count
		<< "\n";
	cout << "bintree M() largest hash group sample values:";
	for (const auto& s : samples)
		cout << " [" << s.value
			<< " (l=" << (s.left == nullptr ? "null" : "set")
			<< ", r=" << (s.right == nullptr ? "null" : "set")
			<< ") stored hash=" << s.stored_hash
			<< " recomputed hash=" << s.recomputed_hash << "]";
	cout << "\n";
	cout << "bintree M() stale hash check: stale_hash_count (whole map): "
		<< stale_hash_count << ", largest_group_hash: "
		<< largest_group_hash << ", largest_group_stale_count: "
		<< largest_group_stale_count << "\n";
}

// The same bintree counters as print_bintree_process_totals(), as JSON
// numbers for the --json data under bintree_totals.
static format::json::value bintree_process_totals_value() {
	using tree_t = tgf_repl_evaluator::parser_type::tree;
	using value = format::json::value;
	size_t buckets = 0, entries = 0, max_chain = 0;
	double load_factor = 0.0, mean_chain = 0.0;
	std::array<size_t, 8> chain_len_histogram{};
	tree_t::bucket_stats(buckets, entries, load_factor, max_chain,
		mean_chain, &chain_len_histogram);
	size_t distinct_hashes = 0, largest_group_size = 0;
	std::array<size_t, 5> top_group_sizes{}, top_group_triples{};
	size_t distinct_values = 0, distinct_child_pairs = 0, leaf_count = 0;
	std::vector<tree_t::hash_group_sample> samples;
	size_t stale_hash_count = 0, largest_group_stale_count = 0;
	std::uint64_t largest_group_hash = 0;
	tree_t::hash_group_stats(distinct_hashes, largest_group_size,
		top_group_sizes, top_group_triples, distinct_values,
		distinct_child_pairs, leaf_count, samples,
		stale_hash_count, largest_group_hash,
		largest_group_stale_count);
	auto n = [](size_t x) { return value::number(
		static_cast<double>(x)); };
	// A 64-bit hash exceeds the exact JSON number range, so it is text.
	auto h = [](std::uint64_t x) {
		return value::string(std::to_string(x)); };
	static constexpr const char* bin_names[8] = {
		"0", "1", "2", "3", "4-7", "8-15", "16-63", "64+" };
	auto histogram = value::object();
	for (size_t i = 0; i != 8; ++i)
		histogram.set(bin_names[i], n(chain_len_histogram[i]));
	auto top_groups = value::array();
	for (size_t i = 0; i != 5; ++i)
		top_groups.push_back(value::object()
			.set("size", n(top_group_sizes[i]))
			.set("triples", n(top_group_triples[i])));
	auto sample_arr = value::array();
	for (const auto& s : samples) {
		auto e = value::object();
		e.set("value", value::string(s.value.first.to_std_string()));
		auto range = value::array();
		range.push_back(n(s.value.second[0]));
		range.push_back(n(s.value.second[1]));
		e.set("range", std::move(range));
		e.set("left", s.left == nullptr ? value::null()
			: h(tree_t::get(s.left).hash));
		e.set("right", s.right == nullptr ? value::null()
			: h(tree_t::get(s.right).hash));
		e.set("stored_hash", h(s.stored_hash));
		e.set("recomputed_hash", h(s.recomputed_hash));
		sample_arr.push_back(std::move(e));
	}
	value v = value::object();
	v.set("get_hits", n(tree_t::get_hits()))
	 .set("get_misses", n(tree_t::get_misses()))
	 .set("geth_calls", n(tree_t::geth_calls()))
	 .set("buckets", n(buckets))
	 .set("entries", n(entries))
	 .set("load_factor", value::number(load_factor))
	 .set("max_chain", n(max_chain))
	 .set("mean_chain", value::number(mean_chain))
	 .set("distinct_hashes", n(distinct_hashes))
	 .set("largest_group_size", n(largest_group_size))
	 .set("distinct_values", n(distinct_values))
	 .set("distinct_child_pairs", n(distinct_child_pairs))
	 .set("leaf_count", n(leaf_count))
	 .set("stale_hash_count", n(stale_hash_count))
	 .set("largest_group_hash", h(largest_group_hash))
	 .set("largest_group_stale_count", n(largest_group_stale_count))
	 .set("chain_length_histogram", std::move(histogram))
	 .set("top_hash_groups", std::move(top_groups))
	 .set("largest_group_samples", std::move(sample_arr));
	return v;
}

static void print_version() {
	std::cout << tauparser::full_version << "\n";
}

static void print_license() {
	std::cout << tauparser::license << "\n";
}

static string statement_option_name(const tt& s) {
	auto n = s | tgf_repl_parser::option_name;
	return n.has_value() ? n | tt::terminals : string();
}

// The grammar trims the whitespace between the words of a value, so one
// space joins them again.
static string statement_option_text(const tt& s) {
	string text;
	for (const auto& w : (s | tgf_repl_parser::option_value
		|| tgf_repl_parser::option_word)())
			text += (text.empty() ? "" : " ")
				+ string(w | tt::terminals);
	return text;
}

static string unquote(const string& s, diagnostics::report& report) {
	if (s.size() < 2 || s[0] != '"' || s[s.size() - 1] != '"') return s;
	auto dec = escapes::decode(
		s.substr(1, s.size() - 2), escapes::tgf_string);
	if (!dec.has_value()) {
		report.append(std::move(dec.report()));
		return {};
	}
	return std::move(dec).value();
}

const std::string& tgf_repl_evaluator::filename() const noexcept {
	return tgf_filename;
}

const std::string& tgf_repl_evaluator::source() const noexcept {
	return grammar_source;
}

std::string tgf_repl_evaluator::grammar_text() const {
	if (!grammar_loaded) return {};
	return grammar_source;
}

void tgf_repl_evaluator::make_grammar_source_backed() {
	// A built-in grammar cannot reload; a grammar that already came from
	// text needs no change.
	if (!grammar_loaded || fixed_grammar || grammar_from_source) return;
	grammar_from_source = true;
}

bool tgf_repl_evaluator::load_file_source(const std::string& filename) {
	ifstream f(filename, ios::binary);
	if (!f) {
		report.error(diagnostics::code::io_error,
			parser_strings::messages::cannot_open_file);
		return false;
	}
	std::string text((istreambuf_iterator<char>(f)),
		istreambuf_iterator<char>());
	return load_source(filename, text);
}

std::string tgf_repl_evaluator::start_symbol() const {
	if (!has_grammar()) return {};
	auto start = idni::options().get_text("start");
	return start.has_value() ? start.value() : std::string();
}

bool tgf_repl_evaluator::has_fixed_grammar() const noexcept {
	return fixed_grammar;
}

bool tgf_repl_evaluator::require_grammar() {
	if (grammar_loaded) return true;
	report.error(diagnostics::code::no_grammar,
		parser_strings::messages::no_grammar_loaded);
	return false;
}

bool tgf_repl_evaluator::paths_allowed() {
	if (!opt.no_server_paths) return true;
	report.error(diagnostics::code::server_path,
		parser_strings::messages::server_path_forbidden);
	return false;
}

void tgf_repl_evaluator::begin_server_session(const std::string& id) {
	session_id = id;
	opt.no_server_paths = true;
	opt.json = true;
	opt.colors = false;
	TC.set(false);
}

diagnostics::report tgf_repl_evaluator::take_report() {
	diagnostics::report r = std::move(report);
	report.clear();
	return r;
}

// A text stream adds the CR of a CRLF back on Windows, so drop it here.
static void print_text_source(ostream& os, const string& src) {
	for (size_t i = 0; i != src.size(); ++i) {
		if (src[i] == '\r' && i + 1 != src.size()
				&& src[i + 1] == '\n')
			continue;
		os << src[i];
	}
}

// The text of the grammar command, with one blank line after the last line.
// The local REPL and the client of `tgf serve` both print it.
static void print_grammar_source(ostream& os, const string& src) {
	os << "grammar:\n";
	std::ostringstream norm;
	print_text_source(norm, src);
	std::string s = norm.str();
	if (s.empty()) { os << "\n"; return; }
	if (s.back() == '\n') s.pop_back();
	os << s << "\n\n";
}

void tgf_repl_evaluator::print_source(ostream& os) const {
	print_grammar_source(os, grammar_source);
}

void tgf_repl_evaluator::reprompt() {
	stringstream ss;
	if (opt.status) {
		ss << TC_STATUS << "[ ";
		if (has_grammar())
			ss << TC_STATUS_FILE << "\"" << tgf_filename
				<< "\"" << TC.CLEAR() << TC_STATUS << " ";
		if (has_grammar())
			ss << TC_STATUS_START << start_symbol()
				<< TC.CLEAR() << TC_STATUS << " ]"
				<< TC.CLEAR() << " ";
		else
			ss << "]" << TC.CLEAR() << " ";
	}
	ss << TC_PROMPT << "tgf>" << TC.CLEAR() << " ";
	if (r) r->set_prompt(ss.str());
#ifdef TAU_PARSER_HAS_FTXUI
	if (r_ftx) r_ftx->set_prompt(ss.str());
#endif
}

ostream& tgf_repl_evaluator::pretty_print(ostream& os, tref n,
	set<size_t> skip, bool nulls, size_t l)
{
	const auto& t = tree::get(n);
	if (skip.size() && t.value.first.nt() &&
		skip.find(t.value.first.n()) != skip.end())
			return os;
	if (!nulls && t.value.first.is_null()) return os;
	for (size_t t = 0; t < l; t++) os << "\t";
	if (t.value.first.nt())
		os << TC_NT << t.value.first << TC.CLEAR() << TC_NT_ID
			<< "(" << t.value.first.n() << ")" << TC.CLEAR();
	else if (t.value.first.is_null())
		os << TC_NULL << "null" << TC.CLEAR();
	else os << TC_T << t.value.first << TC.CLEAR();
	os << TC_RANGE << "[" << t.value.second[0] << ", "
		<< t.value.second[1] << "]" << TC.CLEAR() << "\n";
	for (tref d : t.children()) pretty_print(os, d, skip, nulls, l + 1);
	return os;
}

void tgf_repl_evaluator::flush_report() {
	print_diagnostics_report(report, opt.json);
	report.clear();
}

// The text stays, so a session and a source-backed reload use the bytes of
// this load. A file-backed reload reads the file again.
bool tgf_repl_evaluator::load_file(const std::string& filename) {
	ifstream f(filename, ios::binary);
	if (!f) {
		report.error(diagnostics::code::io_error,
			parser_strings::messages::cannot_open_file,
			{ { parser_strings::label::path, filename } });
		return false;
	}
	std::string text((istreambuf_iterator<char>(f)),
		istreambuf_iterator<char>());
	auto next_nts = make_unique<nonterminals_type>();
	auto gr = tgf<char_type, terminal_type>::from_string(*next_nts, text,
		idni::options().get<bool>("measure"));
	if (!gr.has_value()) {
		report.append(std::move(gr).report());
		return false;
	}
	auto next_grammar = make_unique<grammar_type>(std::move(gr).value());
	auto next_parser = make_unique<parser_type>(*next_grammar,
		default_parser_options<char_type, terminal_type>());
	report.append(std::move(gr).report());

	// the unbind of the old parser and grammar reads the old nonterminals
	owned_p       = std::move(next_parser);
	owned_g       = std::move(next_grammar);
	owned_nts     = std::move(next_nts);
	p_            = owned_p.get();
	bind_grammar_options();
	report.append(p().bind_options().report());
	write_argv_values();
	tgf_filename = filename;
	grammar_source = std::move(text);
	grammar_from_source = false;
	grammar_loaded = true;
	return true;
}

bool tgf_repl_evaluator::load_source(const std::string& name,
	const std::string& text)
{
	auto next_nts = make_unique<nonterminals_type>();
	auto gr = tgf<char_type, terminal_type>::from_string(*next_nts, text,
		idni::options().get<bool>("measure"));
	if (!gr.has_value()) {
		report.append(std::move(gr).report());
		return false;
	}
	auto next_grammar = make_unique<grammar_type>(std::move(gr).value());
	auto next_parser = make_unique<parser_type>(*next_grammar,
		default_parser_options<char_type, terminal_type>());
	report.append(std::move(gr).report());

	// the unbind of the old parser and grammar reads the old nonterminals
	owned_p             = std::move(next_parser);
	owned_g             = std::move(next_grammar);
	owned_nts           = std::move(next_nts);
	p_                  = owned_p.get();
	bind_grammar_options();
	report.append(p().bind_options().report());
	write_argv_values();
	tgf_filename        = name;
	grammar_source      = text;
	grammar_from_source = true;
	grammar_loaded      = true;
	return true;
}

std::string tgf_repl_evaluator::nt_name(size_t id) const {
	return g().nt(id).to_std_string();
}

void tgf_repl_evaluator::bind_grammar_options() {
	// the old grammar left its values in the repository when it unbound
	idni::options().reset("grammar");
	report.append(g().bind_options().report());
}

// A load resets only the owner grammar, so only its options lose a flag.
void tgf_repl_evaluator::write_argv_values() {
	auto& opts = idni::options();
	for (const auto& [name, v] : argv_values)
		if (auto spec = opts.find(name); spec && spec->owner == "grammar")
			report.append(opts.set(name, v).report());
}

void tgf_repl_evaluator::set_argv_values(
	std::vector<std::pair<std::string, option_value>> values)
{
	argv_values = std::move(values);
}

void tgf_repl_evaluator::set_grammar_option(std::string_view name,
	idni::option_value v)
{
	report.append(idni::options().set(name, std::move(v)).report());
}

void tgf_repl_evaluator::bind_repl_options() {
	auto& opts = idni::options();
	auto declared = opts.declare(tgf_repl_option_set);
	const bool ok = !declared.has_error();
	report.append(declared.report());
	if (!ok) return;
	repl_options_bound = true;
	// a constructor input is the initial value, so no value that an
	// evaluator before this one left in the repository reaches the field
	for (const auto& owner : { "repl", "ui", "cli" }) opts.reset(owner);
	auto bind = [&](auto r) { report.append(r.report()); };
	bind(opts.bind("print-terminals", opt.print_terminals));
	bind(opts.bind("status", opt.status));
	bind(opts.bind("print-ambiguity", opt.print_ambiguity));
	bind(opts.bind("print-graphs", opt.print_graphs));
	bind(opts.bind("print-tml-rules", opt.tml_rules));
	bind(opts.bind("print-tml-facts", opt.tml_facts));
	bind(opts.bind("print-input", opt.print_input));
	auto set_colors = [this] { TC.set(!opt.json && opt.colors); };
	bind(opts.bind("colors", opt.colors, set_colors));
	bind(opts.bind("json", opt.json, set_colors));
}

void tgf_repl_evaluator::reset_parse_options() {
	auto& opts = idni::options();
	report.append(opts.declare(parser_option_set).report());
	// a load keeps the parse options, a new evaluator starts from the
	// defaults
	opts.reset("parse");
}

void tgf_repl_evaluator::set_repl(repl<tgf_repl_evaluator>& r_) {
	r = &r_;
	reprompt();
}

format::json::value tgf_repl_evaluator::parsed(parser_type::result& r,
	std::string& text)
{
	using value = format::json::value;
	if (!r.good() || !r.found) {
		report.append(std::move(r.report()));
		return value::object();
	}
	auto f = r.get_forest();
	using c_t = parser_type::char_type;
	using t_t = parser_type::terminal_type;
	if (!opt.json) {
		stringstream ss;
		if (opt.print_input) ss << "input: \"" << r.get_input() << "\"\n";
		if (opt.print_ambiguity) r.print_ambiguous_nodes(ss);
		if (opt.print_terminals) ss << "parsed terminals: "
			<< TC_T << to_std_string(r.get_terminals())
			<< TC_CLEARED_DEFAULT << "\n";
		auto cb_next_g = [&r, &ss, this](parser_type::pgraph& g) {
			r.inline_grammar_transformations(g);
			if (opt.tml_rules) to_tml_rules<c_t, t_t,
				parser_type::pgraph>(ss << "TML rules:\n", g),
				ss << "\n";
			return true;
		};
		if (opt.tml_rules) f->extract_graphs(f->root(), cb_next_g);
		if (opt.tml_facts) to_tml_facts<c_t, t_t>(
			ss << "TML facts:\n", r);
		if (opt.print_graphs) pretty_print(ss << "parsed graph:\n",
			r.get_shaped_tree2(shaping()), {}, false, 1);
		text = ss.str();
		report.append(std::move(r.report()));
		return value::object();
	}
	auto v = value::object();
	if (opt.print_input) v.set("input", value::string(r.get_input()));
	if (opt.print_ambiguity) {
		auto amb = value::object();
		amb.set("trees", value::number(
			static_cast<double>(r.count_trees())));
		auto nodes = value::array();
		for (const auto& [n, alts] : r.ambiguous_nodes()) {
			auto a = value::object();
			a.set("symbol", value::string(
				n.first.to_std_string()))
			 .set("id", value::number(
				static_cast<double>(n.first.n())))
			 .set("range", range_value(n.second));
			auto alts_arr = value::array();
			for (const auto& ns : alts) {
				auto alt = value::object();
				auto children = value::array();
				for (const auto& child : ns) {
					tref t = r.get_tree2(child);
					if (t) children.push_back(
						format::ast_json::
							node_to_value<tree>(t));
				}
				alt.set("children", std::move(children));
				alts_arr.push_back(std::move(alt));
			}
			a.set("alternatives", std::move(alts_arr));
			nodes.push_back(std::move(a));
		}
		amb.set("nodes", std::move(nodes));
		v.set("ambiguous", std::move(amb));
	}
	if (opt.print_terminals) v.set("terminals",
		value::string(to_std_string(r.get_terminals())));
	if (opt.tml_rules) {
		stringstream ss;
		auto cb_next_g = [&r, &ss](parser_type::pgraph& g) {
			r.inline_grammar_transformations(g);
			to_tml_rules<c_t, t_t, parser_type::pgraph>(
				ss << "TML rules:\n", g), ss << "\n";
			return true;
		};
		f->extract_graphs(f->root(), cb_next_g);
		v.set("tml_rules", value::string(ss.str()));
	}
	if (opt.tml_facts) {
		stringstream ss;
		to_tml_facts<c_t, t_t>(ss << "TML facts:\n", r);
		v.set("tml_facts", value::string(ss.str()));
	}
	if (opt.print_graphs) v.set("tree",
		format::ast_json::node_to_value<tree>(
			r.get_shaped_tree2(shaping())));
	report.append(std::move(r.report()));
	return v;
}

format::json::value tgf_repl_evaluator::parse(const char* input, size_t size,
	std::string& text)
{
	if (!require_grammar()) return format::json::value::object();
	if (!good()) return format::json::value::object();
	auto r = p().parse(input, size);
	return parsed(r, text);
}

format::json::value tgf_repl_evaluator::parse(istream& instream,
	std::string& text)
{
	if (!require_grammar()) return format::json::value::object();
	if (!good()) return format::json::value::object();
	auto r = p().parse(instream);
	return parsed(r, text);
}

format::json::value tgf_repl_evaluator::parse(const string& infile,
	std::string& text)
{
	if (!require_grammar()) return format::json::value::object();
	if (!good()) return format::json::value::object();
	auto r = p().parse(infile);
	return parsed(r, text);
}

shaping_options tgf_repl_evaluator::shaping() {
	return g().opt().shaping;
}

std::string tgf_repl_evaluator::production_string(size_t p) const {
	std::string s = g()(p).to_std_string();
	s += " =>";
	size_t j = 0;
	for (const auto& c : g()[p]) {
		if (j++ != 0) s += " &";
		if (c.neg) s += " ~(";
		for (const auto& l : c) {
			s += " ";
			if (l.nt()) s += l.to_std_string();
			else if (l.is_null()) s += "null";
			else s += l.to_std_string();
		}
		if (c.neg) s += " )";
	}
	s += ".";
	return s;
}

format::json::value tgf_repl_evaluator::production_id_entry(
	size_t p) const
{
	using value = format::json::value;
	auto v = value::object();
	v.set("index", value::number(static_cast<double>(p)));
	v.set("head", value::number(
		static_cast<double>(g()(p).n())));
	auto body = value::array();
	for (const auto& c : g()[p]) {
		auto conj = value::array();
		for (const auto& l : c) {
			if (l.nt()) conj.push_back(value::number(
				static_cast<double>(l.n())));
			else conj.push_back(value::null());
		}
		body.push_back(std::move(conj));
	}
	v.set("body", std::move(body));
	const std::string* guard = g().production_guard(p);
	v.set("guard", guard ? value::string(*guard) : value::null());
	v.set("conjunctive", value::boolean(g().conjunctive(p)));
	return v;
}

bool tgf_repl_evaluator::reload(const string& new_tgf_file) {
	if (fixed_grammar) {
		report.warning(parser_strings::messages::loading_grammars_unavailable);
		return false;
	}

	// A grammar loaded from text has no file to read; reload the stored
	// text instead. @p new_tgf_file is ignored in that case.
	if (grammar_from_source) {
		std::string name = tgf_filename;
		std::string text = grammar_source;
		if (!load_source(name, text)) {
			report.error(diagnostics::code::io_error,
				parser_strings::messages::reload_failed,
				{{parser_strings::label::path, name}});
			return false;
		}
		report.info(parser_strings::messages::reload_succeeded,
			{{parser_strings::label::path, tgf_filename}});
		return true;
	}
	if (!load_file(new_tgf_file)) {
		report.error(diagnostics::code::io_error,
			parser_strings::messages::reload_failed,
			{{parser_strings::label::path, new_tgf_file}});
		return false;
	}
	report.info(parser_strings::messages::reload_succeeded,
		{{parser_strings::label::path, tgf_filename}});
	return true;
}

// Data of the load and reload commands. @p new_tgf_file is the file to
// load; on success tgf_filename names it. The caller checked fixed_grammar.
format::json::value tgf_repl_evaluator::reload_data(
	const std::string& new_tgf_file)
{
	using value = format::json::value;
	auto v = value::object();
	if (!load_file(new_tgf_file))
		return v.set("grammar", value::string(new_tgf_file))
			.set("loaded", value::boolean(false));
	return v.set("grammar", value::string(tgf_filename))
		.set("loaded", value::boolean(true));
}

format::json::value tgf_repl_evaluator::load_source_data(
	const std::string& name, const std::string& text)
{
	using value = format::json::value;
	auto v = value::object();
	if (fixed_grammar) {
		report.warning(parser_strings::messages::loading_grammars_unavailable);
		return v.set("grammar", value::string(name))
			.set("loaded", value::boolean(false));
	}
	if (!load_source(name, text))
		return v.set("grammar", value::string(name))
			.set("loaded", value::boolean(false));
	return v.set("grammar", value::string(tgf_filename))
		.set("loaded", value::boolean(true));
}

format::json::value tgf_repl_evaluator::reload_source_data() {
	using value = format::json::value;
	auto v = value::object();
	std::string name = tgf_filename;
	std::string text = grammar_source;
	if (!load_source(name, text))
		return v.set("grammar", value::string(name))
			.set("loaded", value::boolean(false));
	return v.set("grammar", value::string(tgf_filename))
		.set("loaded", value::boolean(true));
}

format::json::value tgf_repl_evaluator::get_cmd(std::string_view name) {
	namespace ps = parser_strings;
	using value = format::json::value;
	if (name.empty()) {
		auto v = value::object();
		v.set("options", option_values());
		return v;
	}
	if (!idni::options().find(name)) {
		report.error(diagnostics::code::not_found,
			ps::messages::option_not_found,
			{ { ps::label::name, name } });
		return value::object();
	}
	auto v = value::object();
	v.set("option", value::string(std::string(name)))
	 .set("value", format::json::to_json(idni::options().value(name)));
	return v;
}

format::json::value tgf_repl_evaluator::option_values() const {
	using value = format::json::value;
	const auto& opts = idni::options();
	auto v = value::object();
	for (const auto& name : opts.names())
		v.set(name, format::json::to_json(opts.value(name)));
	return v;
}

format::json::value tgf_repl_evaluator::set_cmd(std::string_view name,
	std::string_view text)
{
	auto r = idni::options().set_text(name, text);
	const bool ok = !r.has_error();
	report.append(r.report());
	if (!ok) return format::json::value::object();
	return get_cmd(name);
}

format::json::value tgf_repl_evaluator::set_option(std::string_view name,
	idni::option_value v)
{
	auto r = idni::options().set(name, std::move(v));
	const bool ok = !r.has_error();
	report.append(r.report());
	if (!ok) return format::json::value::object();
	return get_cmd(name);
}

// An unknown name or a spec of another kind is an error in @p report.
static bool option_of_kind(std::string_view name, option_kind kind,
	diagnostics::report& report)
{
	namespace ps = parser_strings;
	auto spec = idni::options().find(name);
	if (!spec) {
		report.error(diagnostics::code::not_found,
			ps::messages::option_not_found,
			{ { ps::label::name, name } });
		return false;
	}
	if (kind_of(*spec) == kind) return true;
	report.error(diagnostics::code::type_error,
		ps::messages::option_value_kind,
		{ { ps::label::name, name } });
	return false;
}

format::json::value tgf_repl_evaluator::list_cmd(std::string_view name,
	std::string_view text, bool add)
{
	auto entries = idni::detail::parse_value(option_kind::list, text,
		name);
	if (!entries.has_value()) {
		report.append(entries.report());
		return format::json::value::object();
	}
	return edit_option_list(name,
		std::get<option_list>(entries.value()), add);
}

format::json::value tgf_repl_evaluator::edit_option_list(
	std::string_view name, const option_list& entries, bool add)
{
	if (!option_of_kind(name, option_kind::list, report))
		return format::json::value::object();
	auto list = idni::options().get<option_list>(name);
	for (const auto& e : entries) {
		auto it = std::find(list.begin(), list.end(), e);
		if (add && it == list.end()) list.push_back(e);
		else if (!add && it != list.end()) list.erase(it);
	}
	return set_option(name, std::move(list));
}

format::json::value tgf_repl_evaluator::flag_cmd(std::string_view name,
	const function<bool(bool)>& update)
{
	if (!option_of_kind(name, option_kind::flag, report))
		return format::json::value::object();
	auto& opts = idni::options();
	opts.set(name, update(opts.get<bool>(name)));
	return get_cmd(name);
}

// TODO (LOW) write proper help messages
static std::string help_text(size_t nt, bool show_load_reload) {
	using p = tgf_repl_parser;
	std::ostringstream os;
	// the declared options of a kind, or all of them
	auto available = [](std::optional<option_kind> kind) {
		std::ostringstream os;
		os << "Available options:\n";
		const auto& opts = idni::options();
		for (const auto& name : opts.names()) {
			auto spec = opts.find(name);
			if (kind && kind_of(*spec) != *kind) continue;
			std::string col = "  " + name;
			col.resize(std::max<size_t>(col.size() + 1, 25), ' ');
			os << col << spec->help << "\n";
		}
		return os.str();
	};
	const string all_available_options = available(std::nullopt);
	const string bool_available_options = available(option_kind::flag);
	const string list_available_options = available(option_kind::list);
	switch (nt) {
	case p::help_sym: os
		<< "tgf commands:\n"
		<< "  help or h                    print this help\n"
		<< "  help <command>               print help for a command\n"
		<< "  quit, q, exit or e           exit the repl\n"
		<< "  version or v                 print version\n"
		<< "  clear or cls                 clears the screen\n"
		<< "\n"
		<< "settings commands:\n"
		<< "  get                          get options' values\n"
		<< "  set                          set option's value\n"
		<< "  toggle                       toggle option's value\n"
		<< "  enable                       set option's value to on\n"
		<< "  disable                      set option's value to off\n"
		<< "  add                          add value to the list\n"
		<< "  delete                       remove value from the list\n"
		<< "\n"
		<< "grammar commands:\n"
		<< "  grammar or g                 show TGF grammar\n"
		<< "  internal-grammar or ig or i  show TGF grammar\n"
		<< "  start or s                   show or change start symbol\n"
		<< "  unreachable or u             show unreachable productions\n"
		<< "\n"
		<< (show_load_reload
			? "  load or l                   load a TGF file\n"
			  "  reload or r                 reload current TGF file\n"
			: "")
		<< "parsing commands:\n"
		<< "  parse or p                   parse input\n"
		<< "  parse file or pf or f        parse input file\n"
		<< "\n";
		break;
	case p::version_sym: os
		<< "version or v prints out current TGF commit id\n";
		break;
	case p::quit_sym: os
		<< "command: quit or exit\n"
		<< "short: q or e\n"
		<< "\texits the repl\n";
		break;
	case p::clear_sym: os
		<< "command: clear\n"
		<< "short: cls\n"
		<< "\tclears the screen\n";
		break;
	case p::get_sym: os
		<< "command: get [<option>]\n"
		<< "\tprints the value of the given option\n"
		<< "\tprints all option values if no option provided\n"
		<< "\n"
		<< all_available_options;
		break;
	case p::set_sym: os
		<< "command: set <option> [=] <value>\n"
		<< "\tsets value of the given option\n"
		<< "\n"
		<< all_available_options;
		break;
	case p::toggle_sym: os
		<< "command: toggle <option>\n"
		<< "short: tog\n"
		<< "\t toggles value between on/off of the given option\n"
		<< "\n"
		<< bool_available_options;
		break;
	case p::enable_sym: os
		<< "command: enable <option>\n"
		<< "short: en\n"
		<< "\tsets the value of the given option to on\n"
		<< "\n"
		<< bool_available_options;
		break;
	case p::disable_sym: os
		<< "command: disable <option>\n"
		<< "short: dis\n"
		<< "\tsets the value of the given option to off\n"
		<< "\n"
		<< bool_available_options;
		break;
	case p::add_sym: os
		<< "command: add <option> <value>\n"
		<< "\tadds the value to the given option list\n"
		<< "\n"
		<< list_available_options;
		break;
	case p::del_sym: os
		<< "command: delete <option> <value>\n"
		<< "or: del, remove, rem or rm\n"
		<< "\tremoves the value from the given option list\n"
		<< "\n"
		<< list_available_options;
		break;
	case p::load_sym: os
		<< "command: load \"TGF filepath\"\n"
		<< "short: l\n"
		<< "\tload a TGF file from drive\n";
		break;
	case p::start_sym: os
		<< "command: start [<start symbol>]\n"
		<< "short: s\n"
		<< "\tset a new start symbol for parsing"
		<< "\tprint the current start symbol if no argument\n";
		break;
	case p::grammar_sym: os
		<< "command: grammar\n"
		<< "short: g\n"
		<< "\tprints the actual TGF file\n";
		break;
	case p::igrammar_sym: os
		<< "command: internal-grammar [<start symbol>]\n"
		<< "short: ig or i\n"
		<< "\tprints the internal grammar\n"
		<< "\tif start symbol provided prints the internal sub-grammar\n";
		break;
	case p::unreachable_sym: os
		<< "command: unreachable [<symbol>]\n"
		<< "short: u\n"
		<< "\tprints unreachable production rules for provided symbol\n"
		<< "\tif no symbol provided prints unreachable rules for start symbol\n";
		break;
	case p::parse_sym: os
		<< "command: parse <input>\n"
		<< "short: p\n"
		<< "\tparse the given input\n";
		break;
	case p::parse_file_sym: os
		<< "command: parse file \"<input file>\"\n"
		<< "short: pf or f\n"
		<< "\tparse the given input file\n";
		break;
	}
	return os.str();
}

cmd_result tgf_repl_evaluator::run(const trv& s) {
	using p = tgf_repl_parser;
	using value = format::json::value;
	const auto nt = s | tt::nonterminal;
	cmd_result res;
	res.cmd = command_name(nt);
	{
		auto _ = report.open_if(
			idni::options().get<bool>("measure") || nt == p::parse_cmd,
			tgf_repl_parser::instance().name(nt),
			idni::diagnostics::code::info_micros);
		switch (nt) {
		case p::quit_cmd:
			res.status = cmd_status::quit;
			break;
		case p::clear_cmd:
			if (r) r->clear();
#ifdef TAU_PARSER_HAS_FTXUI
			else if (r_ftx) r_ftx->clear();
#endif
			break;
		case p::help_cmd: {
			auto optarg = s | p::help_arg
					| tt::only_child | tt::nonterminal;
			size_t target = optarg
				? static_cast<size_t>(optarg)
				: static_cast<size_t>(p::help_sym);
			auto v = value::object();
			v.set("command", value::string(help_arg_name(target)))
			 .set("text", value::string(
				help_text(target, !fixed_grammar)));
			res.data = std::move(v);
			break;
		}
		case p::version_cmd: {
			auto v = value::object();
			v.set("version", value::string(tauparser::full_version));
			res.data = std::move(v);
			break;
		}
		case p::license_cmd: {
			auto v = value::object();
			v.set("license", value::string(tauparser::license));
			res.data = std::move(v);
			break;
		}
		case p::get_cmd:
			res.data = get_cmd(statement_option_name(s));
			break;
		case p::set_cmd:
			res.data = set_cmd(statement_option_name(s),
				statement_option_text(s));
			break;
		case p::toggle_cmd:
			res.data = flag_cmd(statement_option_name(s),
				[](bool b) { return !b; });
			break;
		case p::enable_cmd:
			res.data = flag_cmd(statement_option_name(s),
				[](bool) { return true; });
			break;
		case p::disable_cmd:
			res.data = flag_cmd(statement_option_name(s),
				[](bool) { return false; });
			break;
		case p::add_cmd:
			res.data = list_cmd(statement_option_name(s),
				statement_option_text(s), true);
			break;
		case p::del_cmd:
			res.data = list_cmd(statement_option_name(s),
				statement_option_text(s), false);
			break;
		case p::reload_cmd:
			if (fixed_grammar) {
				report.warning(parser_strings::messages::
					loading_grammars_unavailable);
				break;
			}
			if (!require_grammar()) break;
			// A file-backed grammar needs a path; a source-backed one
			// reloads from the text it was loaded from.
			if (!grammar_from_source && !paths_allowed()) break;
			res.data = grammar_from_source
				? reload_source_data() : reload_data(tgf_filename);
			break;
		case p::load_cmd: {
			if (fixed_grammar) {
				report.warning(parser_strings::messages::
					loading_grammars_unavailable);
				break;
			}
			if (!paths_allowed()) break;
			auto n = s | p::filename;
			auto filename = unquote(n | tt::terminals, report);
			if (report.has_error()) break;
			res.data = reload_data(filename);
			break;
		}
		case p::start_cmd: {
			auto n = s | p::symbol;
			bool changed = false;
			if (n.has_value()) {
				auto r = idni::options().set_text("start",
					n | tt::terminals);
				changed = !r.has_error();
				report.append(r.report());
			}
			const string start = start_symbol();
			auto v = value::object();
			v.set("start", start.empty() ? value::null()
				: value::string(start))
			 .set("changed", value::boolean(changed));
			res.data = std::move(v);
			break;
		}
		case p::igrammar_cmd: {
			if (!require_grammar()) break;
			auto n = s | p::symbol;
			string start = n.has_value()
				? n | tt::terminals : start_symbol();
			auto v = value::object();
			v.set("start", value::string(start));
			auto prods = value::array();
			auto pids = value::array();
			for (size_t p : g().reachable_productions(g().nt(start))) {
				prods.push_back(value::string(production_string(p)));
				pids.push_back(production_id_entry(p));
			}
			v.set("productions", std::move(prods));
			v.set("production_ids", std::move(pids));
			res.data = std::move(v);
			if (!opt.json) {
				ostringstream ts;
				g().print_internal_grammar_for(ts
					<< "\ninternal grammar for symbol "
					<< TC_NT << start << TC_DEFAULT << ":\n",
					start, "  ", true, TC);
				res.text = ts.str();
			}
			break;
		}
		case p::grammar_cmd: {
			if (!require_grammar()) break;
			auto v = value::object();
			v.set("file", value::string(tgf_filename));
			v.set("source", value::string(grammar_source));
			res.data = std::move(v);
			break;
		}
		case p::unreachable_cmd: {
			if (!require_grammar()) break;
			auto n = s | p::symbol;
			string start = n.has_value()
				? n | tt::terminals : start_symbol();
			auto unreachable = g().unreachable_productions(
				g().nt(start));
			auto v = value::object();
			v.set("symbol", value::string(start));
			auto prods = value::array();
			auto pids = value::array();
			for (size_t p : unreachable) {
				prods.push_back(value::string(production_string(p)));
				pids.push_back(production_id_entry(p));
			}
			v.set("productions", std::move(prods));
			v.set("production_ids", std::move(pids));
			res.data = std::move(v);
			if (!opt.json) {
				ostringstream ts;
				if (unreachable.size()) {
					ts << "unreachable production rules for symbol: "
						<< TC_NT << start << TC_DEFAULT << "\n";
					for (auto& p : unreachable) g().print_production(
						ts << "  ", p, true, TC) << "\n";
				}
				else ts << "all production rules reachable for "
					"symbol: " << TC_NT << start
					<< TC_DEFAULT << "\n";
				res.text = ts.str();
			}
			break;
		}
		case p::parse_cmd: {
			string input{};
			auto i = s | p::parse_input;
			if (auto seq = i | p::parse_input_char_seq;
				seq.has_value()) input = seq | tt::terminals;
			else if (auto qstr = i | p::quoted_string;
				qstr.has_value()) input = unquote(
					qstr | tt::terminals, report);
			res.data = parse(input.c_str(), input.size(),
				res.text);
			break;
		}
		case p::parse_file_cmd: {
			if (!paths_allowed()) break;
			auto n = s | p::filename;
			auto filename = unquote(n | tt::terminals, report);
			if (report.has_error()) break;
			res.data = parse(filename, res.text);
			break;
		}
		default:
			report.error(diagnostics::code::invalid_argument,
				parser_strings::messages::unknown_command);
			break;
		}
	}
	res.report = std::move(report);
	report.clear();
	if (res.report.has_error()) res.status = cmd_status::error;
	return res;
}

eval_result tgf_repl_evaluator::run(const std::string& src,
	const std::function<void(cmd_result&)>& each)
{
	eval_result er;
	static tgf_repl_parser rp;
	auto r = rp.parse(src.c_str(), src.size());
	if (!r.found) {
		if (opt.continue_on_eof && r.parse_error.at_eof()) {
			er.status = cmd_status::incomplete;
			r.report().demote_errors_to_warnings();
			er.report.append(std::move(r.report()));
			return er;
		}
		er.status = cmd_status::error;
		er.report.append(std::move(r.report()));
		return er;
	}
	if (!opt.json) {
		r.print_ambiguous_nodes(cout);
		if (idni::options().get<bool>("debug"))
			pretty_print(cout << "input command graph:\n",
				r.get_shaped_tree2(), {}, false, 1);
	}
	tref ref = r.get_shaped_tree2();
	auto t = tt(ref);
	auto statements = t || tgf_repl_parser::statement;
	for (const auto& statement : statements()) {
		er.results.push_back(run(statement | tt::only_child));
		cmd_result& cr = er.results.back();
		if (each) each(cr);
		if (cr.status == cmd_status::quit) {
			er.status = cmd_status::quit;
			break;
		}
		if (cr.status == cmd_status::error)
			er.status = cmd_status::error;
	}
	return er;
}

// One ambiguous node of a parse, from its data only.
static void render_data_ambiguous_node(std::ostream& os,
	const format::json::value& node)
{
	auto sym = node.find("symbol");
	std::string symbol = sym && sym->is_string() ? sym->as_string() : "";
	auto range = node.find("range");
	os << "\t `" << symbol << "` [";
	if (range && range->is_array() && range->size() >= 2)
		os << static_cast<long long>((*range)[0].as_number()) << ","
			<< static_cast<long long>((*range)[1].as_number());
	os << "]\n";
	size_t d = 0;
	if (auto alts = node.find("alternatives"); alts && alts->is_array())
		for (const auto& alt : *alts) {
			os << "\t\t " << d++ << "\t";
			if (auto ch = alt.find("children");
					ch && ch->is_array())
				for (const auto& c : *ch) {
					auto s = c.find("symbol");
					auto rg = c.find("range");
					os << " `" << (s && s->is_string()
						? s->as_string() : "") << "`[";
					if (rg && rg->is_array()
							&& rg->size() >= 2)
						os << static_cast<long long>(
							(*rg)[0].as_number()) << ","
							<< static_cast<long long>(
							(*rg)[1].as_number());
					os << "] ";
				}
			os << "\n";
		}
}

// One AST JSON node in the shape of the text tree printer.
static void render_data_tree(std::ostream& os,
	const format::json::value& node, size_t l,
	const term::colors& TC)
{
	for (size_t i = 0; i != l; ++i) os << "\t";
	auto sym = node.find("symbol");
	std::string symbol = sym && sym->is_string() ? sym->as_string() : "";
	auto id = node.find("id");
	auto range = node.find("range");
	if (!symbol.empty()) {
		os << TC_NT << symbol << TC.CLEAR() << TC_NT_ID << "(";
		if (id && id->is_number())
			os << static_cast<long long>(id->as_number());
		os << ")" << TC.CLEAR();
	} else if (auto text = node.find("text");
			text && text->is_string())
		os << TC_T << text->as_string() << TC.CLEAR();
	os << TC_RANGE << "[";
	if (range && range->is_array() && range->size() >= 2)
		os << static_cast<long long>((*range)[0].as_number()) << ", "
			<< static_cast<long long>((*range)[1].as_number());
	os << "]" << TC.CLEAR() << "\n";
	if (auto ch = node.find("children"); ch && ch->is_array())
		for (const auto& c : *ch) render_data_tree(os, c, l + 1, TC);
}

// The parse text from the response data.
static void render_data_parse(const cmd_result& r, std::ostream& os,
	const term::colors& TC)
{
	if (auto in = r.data.find("input"); in && in->is_string())
		os << "input: \"" << in->as_string() << "\"\n";
	if (auto amb = r.data.find("ambiguous"); amb && amb->is_object()) {
		auto trees = amb->find("trees");
		double n = trees && trees->is_number()
			? trees->as_number() : 0;
		// The text printer stays silent for an unambiguous parse.
		if (n > 1) {
			os << "# n trees: " << static_cast<long long>(n)
				<< "\n# ambiguous nodes:\n";
			if (auto nodes = amb->find("nodes");
					nodes && nodes->is_array())
				for (const auto& node : *nodes)
					render_data_ambiguous_node(os, node);
		}
	}
	if (auto t = r.data.find("terminals"); t && t->is_string())
		os << "parsed terminals: " << TC_T << t->as_string()
			<< TC_CLEARED_DEFAULT << "\n";
	if (auto tml = r.data.find("tml_rules");
			tml && tml->is_string()) os << tml->as_string();
	if (auto tml = r.data.find("tml_facts");
			tml && tml->is_string()) os << tml->as_string();
	if (auto tree = r.data.find("tree"); tree && tree->is_object()) {
		os << "parsed graph:\n";
		render_data_tree(os, *tree, 1, TC);
	}
}

// The grammar source text from the response data.
static void render_data_grammar(const cmd_result& r, std::ostream& os) {
	std::string src;
	if (auto v = r.data.find("source"); v && v->is_string())
		src = v->as_string();
	print_grammar_source(os, src);
}

// The production list of internal-grammar and unreachable, from data.
// Split a production body on spaces. The internal grammar writes one
// literal per token and every literal is a name or a quoted character, so
// no literal carries a space; '&', '~(' and ')' are the only other tokens.
static std::vector<std::string> split_production_body(const std::string& s) {
	std::vector<std::string> out;
	size_t i = 0;
	while (i != s.size()) {
		while (i != s.size() && s[i] == ' ') ++i;
		size_t b = i;
		while (i != s.size() && s[i] != ' ') ++i;
		if (i != b) out.push_back(s.substr(b, i - b));
	}
	return out;
}

// One production in the text form of grammar::print_production, from the
// production string and its production_ids entry. The literal names come
// from the string and the nonterminal ids from production_ids.
static void render_data_production(std::ostream& os,
	const std::string& pstr, const format::json::value* pid,
	const term::colors& TC, const std::set<size_t>* char_classes)
{
	using value = format::json::value;
	const size_t id_width = 6;
	const size_t head_width = 20;
	size_t arrow = pstr.find(" =>");
	std::string head = arrow == std::string::npos
		? pstr : pstr.substr(0, arrow);
	std::string body = arrow == std::string::npos
		? std::string() : pstr.substr(arrow + 3);
	if (!body.empty() && body.back() == '.') body.pop_back();

	size_t index = 0;
	long long head_id = 0;
	bool have_head = false;
	const value* body_ids = nullptr;
	bool conjunctive = false;
	std::string guard;
	if (pid && pid->is_object()) {
		if (auto i = pid->find("index"); i && i->is_number())
			index = static_cast<size_t>(i->as_number());
		if (auto h = pid->find("head"); h && h->is_number()) {
			head_id = static_cast<long long>(h->as_number());
			have_head = true;
		}
		if (auto b = pid->find("body"); b && b->is_array())
			body_ids = b;
		if (auto c = pid->find("conjunctive"); c && c->is_bool())
			conjunctive = c->as_bool();
		if (auto g = pid->find("guard"); g && g->is_string())
			guard = g->as_string();
	}
	// "G<p>:" padded to id_width, then the head with its id padded to
	// head_width, then " =>".
	std::string id = "G" + std::to_string(index) + ":";
	os << id;
	for (size_t i = id.size(); i < id_width; ++i) os << " ";
	// A character class nonterminal takes the character class color; the
	// rest are plain nonterminals.
	auto nt_begin = [&](long long nt_id) -> std::string {
		return (char_classes && char_classes->count(
			static_cast<size_t>(nt_id))) ? TC_CC : TC_NT;
	};
	std::string head_id_s = have_head
		? "(" + std::to_string(head_id) + ")" : std::string();
	os << (have_head ? nt_begin(head_id) : TC_NT) << head
		<< TC_DEFAULT << TC_NT_ID << head_id_s << TC_DEFAULT;
	for (size_t i = head.size() + head_id_s.size(); i < head_width; ++i)
		os << " ";
	os << " =>";
	// The tokens and the production_ids body describe the same literals in
	// the same order, so they walk in parallel.
	std::vector<std::string> tok = split_production_body(body);
	size_t t = 0;
	size_t nconj = body_ids ? body_ids->size() : 0;
	if (!body_ids && !body.empty()) os << " " << body;
	for (size_t j = 0; j != nconj; ++j) {
		if (j != 0) {
			os << " &";
			if (t != tok.size() && tok[t] == "&") ++t;
		}
		bool neg = t != tok.size() && tok[t] == "~(";
		if (neg) { os << " " << TC_NEG << "~("; ++t; }
		const value& conj = (*body_ids)[j];
		for (size_t k = 0; k != conj.size() && t != tok.size(); ++k) {
			os << " ";
			const std::string& name = tok[t++];
			if (conj[k].is_number())
				os << nt_begin(static_cast<long long>(
					conj[k].as_number())) << name
					<< TC_DEFAULT << TC_NT_ID
					<< "(" << static_cast<long long>(
						conj[k].as_number()) << ")"
					<< TC_DEFAULT;
			else if (name == "null")
				os << TC_NULL << "null" << TC_DEFAULT;
			else {
				os << TC_T << name << TC.CLEAR();
				if (neg) os << TC_NEG;
			}
		}
		if (neg) {
			os << TC_NEG << " )" << TC.CLEAR();
			if (t != tok.size() && tok[t] == ")") ++t;
		}
	}
	os << ".";
	if (conjunctive)
		os << "\t " << TC_NULL << "# conjunctive" << TC_DEFAULT;
	if (!guard.empty())
		os << "\t " << TC_NULL << "# guarded: " << guard
			<< TC_DEFAULT;
}

static void render_data_productions(const cmd_result& r, std::ostream& os,
	const term::colors& TC, const std::set<size_t>* char_classes)
{
	auto prods = r.data.find("productions");
	auto pids = r.data.find("production_ids");
	auto sym = r.data.find("symbol");
	auto start = r.data.find("start");
	auto name = r.cmd == "unreachable" ? sym : start;
	std::string s = name && name->is_string() ? name->as_string() : "";
	if (r.cmd == "unreachable") {
		bool any = prods && prods->is_array()
			&& prods->size() != 0;
		if (any)
			os << "unreachable production rules for symbol: "
				<< TC_NT << s << TC_DEFAULT << "\n";
		else
			os << "all production rules reachable for "
				"symbol: " << TC_NT << s << TC_DEFAULT
				<< "\n";
	} else {
		os << "\ninternal grammar for symbol " << TC_NT << s
			<< TC_DEFAULT << ":\n";
	}
	if (!prods || !prods->is_array()) return;
	for (size_t i = 0; i != prods->size(); ++i) {
		const format::json::value* pid = nullptr;
		if (pids && pids->is_array() && i < pids->size())
			pid = &(*pids)[i];
		os << "  ";
		render_data_production(os, (*prods)[i].as_string(), pid, TC,
			char_classes);
		os << "\n";
	}
}

void render_command_text(const cmd_result& r, std::ostream& os,
	const term::colors& TC, const std::set<size_t>* char_classes)
{
	// A text result the evaluator built from live objects wins; the data
	// renderer covers the client, which holds only the response data.
	if (!r.text.empty()) { os << r.text; return; }
	if (r.cmd == "parse" || r.cmd == "parse file") {
		render_data_parse(r, os, TC);
		return;
	}
	if (r.cmd == "grammar") { render_data_grammar(r, os); return; }
	if (r.cmd == "internal-grammar" || r.cmd == "unreachable") {
		render_data_productions(r, os, TC, char_classes);
		return;
	}
	if (r.cmd == "quit") { os << "Quit.\n"; return; }
	if (r.cmd == "clear") return;
	if (r.cmd == "version") {
		if (auto v = r.data.find("version"); v)
			os << v->as_string() << "\n";
		return;
	}
	if (r.cmd == "license") {
		if (auto v = r.data.find("license"); v)
			os << v->as_string() << "\n";
		return;
	}
	if (r.cmd == "help") {
		if (auto v = r.data.find("text"); v) os << v->as_string();
		return;
	}
	if (r.cmd == "start") {
		auto s = r.data.find("start");
		auto ch = r.data.find("changed");
		if (!s || !ch) return;
		if (ch->as_bool()) os << "start symbol set: " << TC_NT
			<< s->as_string() << TC_DEFAULT << "\n";
		else os << "start symbol: " << TC_NT << s->as_string()
			<< TC_DEFAULT << "\n";
		return;
	}
	if (r.cmd == "load" || r.cmd == "reload") {
		auto l = r.data.find("loaded");
		if (!l) return;   // fixed grammar: only the warning
		auto name = r.data.find("grammar");
		if (l->as_bool()) os << "loaded: " << name->as_string() << "\n";
		else os << "reload failed: " << name->as_string() << "\n";
		return;
	}
	auto label = [](const std::string& name) {
		std::string l = name + ":";
		l.resize(std::max<size_t>(l.size(), 24), ' ');
		return l;
	};
	if (auto o = r.data.find("option"); o) {
		os << label(o->as_string())
			<< option_text(*r.data.find("value")) << "\n";
		return;
	}
	if (auto opts = r.data.find("options"); opts) {
		for (const auto& kv : opts->members())
			os << label(kv.first) << option_text(kv.second) << "\n";
		return;
	}
}

void tgf_repl_evaluator::render_text(const cmd_result& r,
	std::ostream& os) const
{
	// The live evaluator prints the loaded source, so its text matches
	// the REPL text. A client with no evaluator renders the grammar from
	// data only.
	if (r.cmd == "grammar") { print_source(os); return; }
	render_command_text(r, os, TC);
}

format::json::value statement_request(const tgf_repl_evaluator::trv& n) {
	using p = tgf_repl_parser;
	using format::json::value;
	const auto nt = n | tt::nonterminal;
	value req = value::object();
	req.set("cmd", value::string(command_name(nt)));
	switch (nt) {
	case p::parse_cmd: {
		auto i = n | p::parse_input;
		std::string input;
		if (auto seq = i | p::parse_input_char_seq; seq.has_value())
			input = seq | tt::terminals;
		else if (auto qs = i | p::quoted_string; qs.has_value()) {
			diagnostics::report rep;
			input = unquote(qs | tt::terminals, rep);
		}
		req.set("input", value::string(std::move(input)));
		break;
	}
	case p::parse_file_cmd:
	case p::load_cmd: {
		auto fn = n | p::filename;
		diagnostics::report rep;
		req.set("file", value::string(
			unquote(fn | tt::terminals, rep)));
		break;
	}
	case p::start_cmd:
	case p::igrammar_cmd:
	case p::unreachable_cmd: {
		auto sym = n | p::symbol;
		if (sym.has_value())
			req.set("symbol", value::string(
				sym | tt::terminals));
		break;
	}
	case p::help_cmd: {
		auto h = n | p::help_arg;
		if (h.has_value())
			req.set("command", value::string(help_arg_name(
				h | tt::only_child | tt::nonterminal)));
		break;
	}
	case p::get_cmd:
		if (auto name = statement_option_name(n); !name.empty())
			req.set("option", value::string(std::move(name)));
		break;
	// The client knows no option kind, so the server reads the text.
	case p::set_cmd:
	case p::add_cmd:
	case p::del_cmd:
		req.set("option", value::string(statement_option_name(n)));
		req.set("text", value::string(statement_option_text(n)));
		break;
	case p::toggle_cmd:
	case p::enable_cmd:
	case p::disable_cmd:
		req.set("option", value::string(statement_option_name(n)));
		break;
	default:
		// grammar, reload, version, license, quit and clear carry no
		// field beyond cmd.
		break;
	}
	return req;
}

idni::diagnostics::result<int> tgf_repl_evaluator::eval(const string& src) {
	auto er = run(src, [this](cmd_result& r) {
		render_text(r, cout);
		report.append(std::move(r.report));
		flush_report();
	});
	if (er.status == cmd_status::incomplete) {
		idni::diagnostics::result<int> res(2);
		res.append(std::move(er.report));
		return res;
	}
	if (er.status == cmd_status::quit) {
		return idni::diagnostics::result<int>(1);
	}
	if (er.report.nodes().size()) {
		report.append(std::move(er.report));
		flush_report();
	}
	cout << endl;
	reprompt();
	return idni::diagnostics::result<int>(0);
}

tgf_repl_evaluator::tgf_repl_evaluator(std::string tgf_file)
	: tgf_repl_evaluator(std::move(tgf_file), options{})
{
}

tgf_repl_evaluator::tgf_repl_evaluator(std::string tgf_file, options opt)
	: opt(opt), tgf_filename(std::move(tgf_file))
{
	TC.set(!opt.json && opt.colors);
	reset_parse_options();
	load_file(tgf_filename);
	bind_repl_options();
}

tgf_repl_evaluator::tgf_repl_evaluator(options opt)
	: opt(opt)
{
	TC.set(!opt.json && opt.colors);
	reset_parse_options();
	owned_nts = make_unique<nonterminals_type>();
	owned_g = make_unique<grammar_type>(*owned_nts);
	owned_p = make_unique<parser_type>(*owned_g,
		default_parser_options<char_type, terminal_type>());
	p_ = owned_p.get();
	bind_grammar_options();
	report.append(p().bind_options().report());
	bind_repl_options();
}

tgf_repl_evaluator::tgf_repl_evaluator(
	parser_type& parser,
	std::string display_name,
	std::string grammar_source)
	: tgf_repl_evaluator(parser, std::move(display_name),
		std::move(grammar_source), options{})
{
}

tgf_repl_evaluator::tgf_repl_evaluator(
	parser_type& parser,
	std::string display_name,
	std::string grammar_source,
	options opt)
	: opt(opt),
	  fixed_grammar(true),
	  grammar_loaded(true),
	  tgf_filename(std::move(display_name)),
	  grammar_source(std::move(grammar_source)),
	  p_(&parser)
{
	TC.set(!opt.json && opt.colors);
	reset_parse_options();
	bind_grammar_options();
	report.append(p().bind_options().report());
	bind_repl_options();
}

// A fixed grammar usually outlives the evaluator, and the repository of
// the thread can end before it.
tgf_repl_evaluator::~tgf_repl_evaluator() {
	if (p_) {
		g().unbind_options();
		p().unbind_options();
	}
	if (repl_options_bound)
		for (const auto& o : tgf_repl_option_set.options)
			idni::options().unbind(o.name);
}

static int run_tests(
	tgf_repl_evaluator::parser_type& p,
	const vector<string>& files)
{
	using char_t = tgf_repl_evaluator::char_type;
	using term_t = tgf_repl_evaluator::terminal_type;
	tgf_test<char_t, term_t> t;
	int ret = 0;
	size_t total_passed = 0, total_failed = 0;
	for (const auto& file : files) {
		cout << "running test: " << file << endl;
		auto r = t.run_from_file(p, file);
		if (r.ret) ret = r.ret;
		total_passed += r.passed;
		total_failed += r.failed;
	}
	if (files.size() > 1)
		cout << "total: " << total_passed << " passed, "
			<< total_failed << " failed" << endl;
	return ret;
}

// The structs of the generator and of the server, with their fields bound
// to the options of one run of the tool.
struct tgf_tool {
	parser_gen_options gen;
#ifdef TAU_PARSER_BUILD_SERVE
	serve_options serve;
#endif
	diagnostics::report report;

	tgf_tool();
	~tgf_tool();
	tgf_tool(const tgf_tool&) = delete;
	tgf_tool& operator=(const tgf_tool&) = delete;

private:
	vector<string_view> bound;
};

tgf_tool::tgf_tool() {
	// a run starts from the defaults, also after a run on the same thread
	for (const auto& owner : { "global", "generator", "serve" })
		idni::options().reset(owner);
	auto bind = [this](string_view name, auto& field) {
		report.append(idni::options().bind(name, field).report());
		bound.push_back(name);
	};
	bind("char-type", gen.char_type);
	bind("terminal-type", gen.terminal_type);
	bind("utf8", gen.utf8);
	bind("name", gen.name);
	bind("output-dir", gen.output_dir);
	bind("output", gen.output);
	bind("namespace", gen.ns);
	bind("header-only", gen.header_only);
	bind("treemr", gen.treemr);
	bind("decoder", gen.decoder);
	bind("encoder", gen.encoder);
#ifdef TAU_PARSER_BUILD_SERVE
	bind("port", serve.port);
	bind("max-sessions", serve.max_sessions);
	bind("idle-timeout", serve.idle_timeout_minutes);
	bind("session-memory", serve.session_memory_mb);
	bind("max-line", serve.max_line);
	bind("write-timeout", serve.write_timeout_seconds);
	bind("log-dir", serve.log_dir);
	bind("no-log", serve.no_log);
#endif
}

tgf_tool::~tgf_tool() {
	for (const auto& name : bound) idni::options().unbind(name);
}

static int command_line_error(string_view msg) {
	cerr << msg << "\n";
	return 1;
}

static int gen_command(tgf_tool& tool, tgf_repl_evaluator& re) {
	using format::json::value;
	const bool json = idni::options().get<bool>("json");
	auto fail = [&](const diagnostics::report& rep) {
		if (json) json_write_line(cout, json_result_response(
			cmd_status::error, "gen", value::object(),
			state_value(re), rep));
		else print_diagnostics_report(rep, false);
		return 1;
	};
	if (!re.require_grammar()) return fail(re.take_report());
	// the warnings of the load stay in the evaluator until here
	diagnostics::report rep = re.take_report();
	// the generator needs char terminals, and the evaluator grammar has
	// char32_t ones, so gen loads the source again
	nonterminals<char> nts;
	auto lg = tgf<char>::from_string(nts, re.source());
	// a loaded file reports the same diagnostics the second time
	if (re.has_fixed_grammar() || !lg.has_value()) rep.append(lg.report());
	if (!lg.has_value()) return fail(rep);
	diagnostics::result<vector<string>> gr;
	{
		options_repository own;
		options_scope scope(own);
		grammar<char> g = std::move(lg).value();
		if (g.size() == 0)
			rep.warning(parser_strings::messages::empty_grammar);
		rep.append(g.bind_options().report());
		re.write_argv_values();
		rep.append(re.take_report());
		if (!rep.has_error())
			gr = generate_parser_cpp(re.filename(), g, tool.gen);
	}
	rep.append(gr.report());
	if (json) {
		auto data = value::object();
		auto files = value::array();
		if (gr.has_value())
			for (const auto& f : gr.value())
				files.push_back(value::string(f));
		data.set("files", std::move(files));
		cmd_status st = gr.has_value()
			? cmd_status::ok : cmd_status::error;
		json_write_line(cout, json_result_response(st, "gen", data,
			state_value(re), rep));
		return st == cmd_status::error ? 1 : 0;
	}
	print_diagnostics_report(rep, false);
	return gr.has_value() ? 0 : 1;
}

// Print the derived character class report of the loaded grammar, one line
// per rule, grouped by state.
static void print_char_class_report(tgf_repl_evaluator& re) {
	using state = cc_rule_info<
		tgf_repl_evaluator::terminal_type>::state;
	auto& g = re.g();
	auto& nts = g.get_nts();
	auto report = g.derive_char_classes_report();
	auto print_group = [&](const char* title, state st) {
		cout << title << ":\n";
		for (const auto& e : report) {
			if (e.st != st) continue;
			cout << "\t" << nts.get(e.nt);
			if (st == state::rejected) cout << " " << e.reason;
			else {
				cout << " guards=";
				bool first = true;
				for (const auto& gd : e.guards)
					cout << (first ? "" : ",") << gd,
						first = false;
			}
			cout << "\n";
		}
	};
	print_group("derived", state::derived);
	print_group("inner", state::inner);
	print_group("unused", state::unused);
	print_group("rejected", state::rejected);
}

static int show_command(tgf_repl_evaluator& re) {
	using format::json::value;
	auto& opts = idni::options();
	const bool json = opts.get<bool>("json");
	if (!re.require_grammar()) {
		if (json) {
			json_write_line(cout, json_result_response(
				cmd_status::error, "grammar", value::object(),
				state_value(re), re.take_report()));
			return 1;
		}
		re.flush_report();
		return 1;
	}
	const string start = re.start_symbol();
	if (json) {
		auto v = value::object();
		v.set("start", value::string(start));
		auto prods = value::array();
		auto pids = value::array();
		for (size_t p : re.g().reachable_productions(re.g().nt(start))) {
			prods.push_back(value::string(
				re.production_string(p)));
			pids.push_back(re.production_id_entry(p));
		}
		v.set("productions", std::move(prods));
		v.set("production_ids", std::move(pids));
		if (opts.get<bool>("nullable")) {
			auto nl = value::array();
			for (const auto& [head, p] :
					re.g().nullable_recursive_productions()) {
				auto e = value::object();
				e.set("symbol", value::string(
					head.to_std_string()))
				 .set("id", value::number(
					static_cast<double>(head.n())))
				 .set("index", value::number(
					static_cast<double>(p)))
				 .set("production", value::string(
					re.production_string(p)));
				nl.push_back(std::move(e));
			}
			v.set("nullable", std::move(nl));
		}
		json_write_line(cout, json_result_response(cmd_status::ok,
			"grammar", v, state_value(re), re.take_report()));
		return 0;
	}
	if (opts.get<bool>("grammar")) re.g().print_internal_grammar_for(
		cout, start, {}, true);
	if (opts.get<bool>("nullable"))
		re.g().check_nullable_recursive_production(cout);
	if (opts.get<bool>("char-class-report")) print_char_class_report(re);
	re.flush_report();
	return 0;
}

static int serve_command(tgf_tool& tool, tgf_repl_evaluator& re) {
#ifndef TAU_PARSER_BUILD_SERVE
	(void)tool;
	(void)re;
	return command_line_error("tgf serve was not built: configure with "
		"-DTAU_PARSER_BUILD_SERVE=ON");
#else
	serve_options& opt = tool.serve;
	if (opt.port > 65535)
		return command_line_error("--port must be between 0 and 65535");
	if (opt.max_sessions < 1)
		return command_line_error("--max-sessions must be at least 1");
	if (opt.max_line < 1)
		return command_line_error("--max-line must be at least 1");
	opt.evaluator = &re;
	auto r = tgf_serve_run(opt);
	// A warning (for example the Windows session memory note) is not an
	// error, so print the report even on a clean stop.
	print_diagnostics_report(r.report(), false);
	return r.has_value() ? r.value() : 1;
#endif
}

#ifdef TAU_PARSER_BUILD_SERVE
// Parse "<host>:<port>". IPv6 is out of scope: the server binds an IPv4
// loopback address only.
static bool connect_parse_host_port(const string& s, string& host,
	uint16_t& port)
{
	auto pos = s.rfind(':');
	if (pos == string::npos || pos == 0 || pos + 1 >= s.size())
		return false;
	host = s.substr(0, pos);
	unsigned long p = 0;
	for (size_t i = pos + 1; i != s.size(); ++i) {
		char c = s[i];
		if (c < '0' || c > '9') return false;
		p = p * 10 + static_cast<unsigned long>(c - '0');
		if (p > 65535) return false;
	}
	port = static_cast<uint16_t>(p);
	return true;
}
#endif

static int connect_command(const vector<string>& inputs) {
#ifndef TAU_PARSER_BUILD_SERVE
	(void)inputs;
	return command_line_error("tgf connect was not built: configure with "
		"-DTAU_PARSER_BUILD_SERVE=ON");
#else
	if (inputs.size() != 1) return command_line_error(
		"connect needs one <host>:<port> argument");
	connect_options opt;
	if (!connect_parse_host_port(inputs.front(), opt.host, opt.port))
		return command_line_error("invalid <host>:<port> argument");
	opt.session = idni::options().get<string>("session");
	opt.legacy_repl = idni::options().get<bool>("legacy-repl");
	auto r = tgf_connect_run(opt);
	if (!r.has_value()) {
		print_diagnostics_report(r.report(), false);
		return 1;
	}
	return r.value();
#endif
}

// @p args holds the inputs without the grammar file.
static int run_command(const parsed_args& args, tgf_tool& tool,
	tgf_repl_evaluator& re)
{
	auto& opts = idni::options();
	const string& command = args.command;
	// A session child runs the JSON API for `tgf serve`, so it sets
	// no_server_paths itself and refuses a path a client sends.
	if (string sid = opts.get<string>("session"); !sid.empty())
		re.begin_server_session(sid);

	if (command == "grammar") return show_command(re);

	if (command == "serve") return serve_command(tool, re);

	if (command == "connect") return connect_command(args.inputs);

	if (command == "gen") return gen_command(tool, re);

	if (command == "test") {
		if (args.inputs.empty()) return command_line_error(
			"test command needs at least one file as an argument");
		int ret = run_tests(re.p(), args.inputs);
		re.flush_report();
		return ret;
	}

	if (command == "repl") {
		const bool json = opts.get<bool>("json");
		const string eval = opts.get<string>("evaluate");
		if (json) {
			if (eval.size()) {
				auto er = re.run(eval);
				json_write_line(cout, json_eval_response(
					format::json::value::null(), er,
					state_value(re)));
				return er.status == cmd_status::error ? 1 : 0;
			}
			return tgf_json_loop(re, cin, cout, tgf_json_max_line,
				opts.get<bool>("init-stdin"));
		}
		auto with_report_flush = [&](auto run) {
			re.flush_report();
			int ret = run();
			re.flush_report();
			return ret;
		};
		auto run_legacy = [&] {
			repl<tgf_repl_evaluator> r(re, "tgf> ", ".tgf_history");
			re.set_repl(r);
			return r.run();
		};
		auto run_default = [&] {
#ifdef TAU_PARSER_HAS_FTXUI
			repl_ftxui<tgf_repl_evaluator> rftx(
				re, "tgf> ", ".tgf_history");
			return rftx.run(); // ctor sets re.r_ftx
#else
			// FTXUI not available: silently fall back to legacy REPL
			return run_legacy();
#endif
		};

		if (eval.size())
			return with_report_flush([&] {
				return re.eval(eval).value_or(0);
			});
		if (opts.get<bool>("legacy-repl"))
			return with_report_flush(run_legacy);
		return with_report_flush(run_default);
	}

	if (command == "parse") {
		using format::json::value;
		const bool json = opts.get<bool>("json");
		string infile = opts.get<string>("input");
		string inexp  = opts.get<string>("input-expression");
		if (infile.size() && inexp.size())
			return command_line_error("multiple inputs specified, "
				"use either --input or --input-expression, not "
				"both");
		if (json) {
			value data = value::object();
			string text;
			if (infile.size())
				if (infile == "-")
					data = re.parse(cin, text);
				else
					data = re.parse(infile, text);
			else
				data = re.parse(inexp.c_str(), inexp.size(),
					text);
			if (opts.get<bool>("grammar")) {
				string start = re.start_symbol();
				auto ig = value::object();
				ig.set("start", value::string(start));
				auto prods = value::array();
				auto pids = value::array();
				for (size_t p : re.g().reachable_productions(
						re.g().nt(start))) {
					prods.push_back(value::string(
						re.production_string(p)));
					pids.push_back(re.production_id_entry(p));
				}
				ig.set("productions", std::move(prods));
				ig.set("production_ids", std::move(pids));
				data.set("internal_grammar", std::move(ig));
			}
			if (opts.get<bool>("measure"))
				data.set("bintree_totals",
					bintree_process_totals_value());
			auto rep = re.take_report();
			cmd_status st = rep.has_error()
				? cmd_status::error : cmd_status::ok;
			json_write_line(cout,
				json_result_response(st, "parse", data,
					state_value(re), rep));
			return st == cmd_status::error ? 1 : 0;
		}
		if (opts.get<bool>("grammar")) re.eval("i");
		string text;
		if (infile.size())
			if (infile == "-")
				re.parse(cin, text);
			else
				re.parse(infile, text);
		else
			re.parse(inexp.c_str(), inexp.size(), text);
		cout << text;
		re.flush_report();
		if (opts.get<bool>("measure")) print_bintree_process_totals();
		return re.has_grammar() ? 0 : 1;
	}
	return 0;
}

// Declare the options of the tool, the grammar, the parser and the
// evaluator, and put each option on its commands.
static diagnostics::result<void> declare_tgf_options(
	options_repository& repo)
{
	diagnostics::result<void> r;
	for (const auto* set : { &tgf_option_set, &grammar_option_set,
		&parser_option_set, &tgf_repl_option_set })
			if (!r.merge_ok(repo.declare(*set))) return r;
	for (const auto& [name, surface] : tgf_surfaces)
		r.merge(repo.add_surface(name, surface));
	return r;
}

// Write the environment and the command line through the options, then
// run the command. The grammar is loaded already, so the command line
// writes after its directives. With @p grammar_input the first input is
// the grammar file.
static int run_tool(int argc, char** argv, const args_config& cfg,
	tgf_tool& tool, tgf_repl_evaluator& re, bool grammar_input)
{
	if (!re.good()) return re.flush_report(), 1;
	auto& opts = idni::options();
	diagnostics::report rep;
	auto env = opts.load_env("TGF");
	const bool env_ok = !env.has_error();
	rep.append(env.report());
	auto parsed = env_ok ? opts.parse_args(argc, argv, cfg)
		: diagnostics::result<parsed_args>{};
	if (env_ok) rep.append(parsed.report());
	if (!rep.nodes().empty()) print_diagnostics_report(rep, false);
	if (!env_ok || !parsed.has_value()) return 1;
	parsed_args args = std::move(parsed).value();

	bool quit = false;
	if (opts.get<bool>("version")) quit = true, print_version();
	if (opts.get<bool>("license")) quit = true, print_license();
	if (opts.get<bool>("help")) quit = true, opts.help(cout,
		args.command_given ? args.command : string());
	if (quit) return 0;

	re.set_argv_values(args.values);
	if (grammar_input && !args.inputs.empty())
		args.inputs.erase(args.inputs.begin());
	// A server keeps the bytes it loaded, so a session and a reload see
	// the bytes of the load, not a later file change.
	if (args.command == "serve") re.make_grammar_source_backed();
	return run_command(args, tool, re);
}

int tgf_specialized_run(int argc, char** argv,
	tgf_repl_evaluator::parser_type& parser,
	const char* display_name,
	const char* grammar_source_c)
{
	const string grammar_source = grammar_source_c ? grammar_source_c : "";
	const string tgf_label = display_name ? display_name : "<compiled>";
	if (auto r = declare_tgf_options(idni::options()); r.has_error())
		return print_diagnostics_report(r.report(), false), 1;
	tgf_tool tool;
	if (!tool.report.nodes().empty())
		print_diagnostics_report(tool.report, false);
	if (tool.report.has_error()) return 1;

	args_config cfg = tgf_args_config;
	cfg.name = tgf_label;
	cfg.help_header = tgf_label;
	cfg.description = "Tau Grammar Format (TGF) specialized grammar tool";

	tgf_repl_evaluator re(parser, tgf_label, grammar_source);
	return run_tool(argc, argv, cfg, tool, re, false);
}

int tgf_run(int argc, char** argv) {
	if (auto r = declare_tgf_options(idni::options()); r.has_error())
		return print_diagnostics_report(r.report(), false), 1;
	tgf_tool tool;
	if (!tool.report.nodes().empty())
		print_diagnostics_report(tool.report, false);
	if (tool.report.has_error()) return 1;

	// The first argument names the grammar file unless it is a command
	// or an option.
	string tgf_file;
	if (argc > 1 && argv[1][0] != '-' && ranges::none_of(
		tgf_option_set.commands, [&](const command_spec& c) {
			return c.name == argv[1]; }))
				tgf_file = argv[1];
	if (tgf_file.empty()) {
		tgf_repl_evaluator re(tgf_repl_evaluator::options{});
		return run_tool(argc, argv, tgf_args_config, tool, re, false);
	}
	if (!ifstream(tgf_file).good())
		return command_line_error("TGF file does not exist");
	tgf_repl_evaluator re(tgf_file);
	return run_tool(argc, argv, tgf_args_config, tool, re, true);
}

#ifdef TAU_PARSER_HAS_FTXUI
template struct repl_ftxui<tgf_repl_evaluator>;
#endif

} // namespace idni
