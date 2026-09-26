// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <algorithm>
#include <array>
#include <cassert>
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

// The REPL option table, keyed by option nonterminal so iteration runs in
// the ascending nonterminal order that get prints. The label is the text
// that get prints before each value.
static const map<size_t, option_desc> option_table = {
{ tgf_repl_parser::error_verbosity_opt, {
	tgf_repl_parser::error_verbosity_opt,
	"error-verbosity", "error-verbosity:        ",
	option_kind::string_value } },
{ tgf_repl_parser::status_opt, {
	tgf_repl_parser::status_opt,
	"status", "show status:            ",
	option_kind::boolean } },
{ tgf_repl_parser::colors_opt, {
	tgf_repl_parser::colors_opt,
	"colors", "colors:                 ",
	option_kind::boolean } },
{ tgf_repl_parser::print_ambiguity_opt, {
	tgf_repl_parser::print_ambiguity_opt,
	"print-ambiguity", "print-ambiguity:        ",
	option_kind::boolean } },
{ tgf_repl_parser::print_graphs_opt, {
	tgf_repl_parser::print_graphs_opt,
	"print-graphs", "print-graphs:           ",
	option_kind::boolean } },
{ tgf_repl_parser::print_rules_opt, {
	tgf_repl_parser::print_rules_opt,
	"print-rules", "print-rules:            ",
	option_kind::boolean } },
{ tgf_repl_parser::print_facts_opt, {
	tgf_repl_parser::print_facts_opt,
	"print-facts", "print-facts:            ",
	option_kind::boolean } },
{ tgf_repl_parser::print_terminals_opt, {
	tgf_repl_parser::print_terminals_opt,
	"print-terminals", "print-terminals:        ",
	option_kind::boolean } },
{ tgf_repl_parser::measure_parsing_opt, {
	tgf_repl_parser::measure_parsing_opt,
	"measure-parsing", "measure-parsing:        ",
	option_kind::boolean } },
{ tgf_repl_parser::measure_each_pos_opt, {
	tgf_repl_parser::measure_each_pos_opt,
	"measure-each-pos", "measure-each:           ",
	option_kind::boolean } },
{ tgf_repl_parser::measure_forest_opt, {
	tgf_repl_parser::measure_forest_opt,
	"measure-forest", "measure-forest:         ",
	option_kind::boolean } },
{ tgf_repl_parser::measure_preprocess_opt, {
	tgf_repl_parser::measure_preprocess_opt,
	"measure-preprocess", "measure-preprocess:     ",
	option_kind::boolean } },
{ tgf_repl_parser::gc_opt, {
	tgf_repl_parser::gc_opt,
	"gc", "gc:                     ",
	option_kind::boolean } },
{ tgf_repl_parser::debug_opt, {
	tgf_repl_parser::debug_opt,
	"debug", "show debug:             ",
	option_kind::boolean } },
{ tgf_repl_parser::auto_disambiguate_opt, {
	tgf_repl_parser::auto_disambiguate_opt,
	"auto-disambiguate", "auto-disambiguate:      ",
	option_kind::boolean } },
{ tgf_repl_parser::trim_terminals_opt, {
	tgf_repl_parser::trim_terminals_opt,
	"trim-terminals", "trim-terminals:         ",
	option_kind::boolean } },
{ tgf_repl_parser::inline_cc_opt, {
	tgf_repl_parser::inline_cc_opt,
	"inline-char-classes", "inline-char-classes:    ",
	option_kind::boolean } },
{ tgf_repl_parser::derive_char_classes_opt, {
	tgf_repl_parser::derive_char_classes_opt,
	"derive-char-classes", "derive-char-classes:    ",
	option_kind::boolean } },
{ tgf_repl_parser::nodisambig_list_opt, {
	tgf_repl_parser::nodisambig_list_opt,
	"nodisambig-list", "nodisambig-list:        ",
	option_kind::list } },
{ tgf_repl_parser::enabled_prods_opt, {
	tgf_repl_parser::enabled_prods_opt,
	"enabled-productions", "enabled_productions:    ",
	option_kind::list } },
{ tgf_repl_parser::trim_opt, {
	tgf_repl_parser::trim_opt,
	"trim", "trim:                   ",
	option_kind::list } },
{ tgf_repl_parser::trim_children_opt, {
	tgf_repl_parser::trim_children_opt,
	"trim-children", "trim-children:          ",
	option_kind::list } },
{ tgf_repl_parser::trim_children_terminals_opt, {
	tgf_repl_parser::trim_children_terminals_opt,
	"trim-children-terminals", "trim-children-terminals:",
	option_kind::list } },
{ tgf_repl_parser::inline_opt, {
	tgf_repl_parser::inline_opt,
	"inline", "inline:                 ",
	option_kind::treepaths } },
{ tgf_repl_parser::start_opt, {
	tgf_repl_parser::start_opt,
	"start", "start:                  ",
	option_kind::symbol_value } },
};

static const option_desc* find_option(size_t nt) {
	auto it = option_table.find(nt);
	return it == option_table.end() ? nullptr : &it->second;
}

const option_desc* option_desc_by_name(std::string_view name) {
	for (const auto& [_, d] : option_table)
		if (name == d.name) return &d;
	return nullptr;
}

static format::json::value range_value(const std::array<size_t, 2>& range) {
	using value = format::json::value;
	value v = value::array();
	v.push_back(value::number(static_cast<double>(range[0])));
	v.push_back(value::number(static_cast<double>(range[1])));
	return v;
}

// Text form of an option value: on/off, a list joined with ", ", tree
// paths joined with " > ", or the error-verbosity name.
static std::string option_text(const format::json::value& v) {
	if (v.is_bool()) return v.as_bool() ? "on" : "off";
	if (v.is_string()) return v.as_string();
	if (!v.is_array()) return {};
	if (v.size() == 0) return "(empty)";
	std::stringstream ss;
	bool first = true;
	for (const auto& e : v) {
		ss << (first ? first = false, "" : ", ");
		if (e.is_array()) {
			bool first_s = true;
			for (const auto& s : e)
				ss << (first_s ? first_s = false, "" : " > ")
					<< s.as_string();
		} else ss << e.as_string();
	}
	return ss.str();
}

static format::json::value strings_value(const std::set<std::string>& l) {
	using value = format::json::value;
	value v = value::array();
	for (const auto& s : l) v.push_back(value::string(s));
	return v;
}

static format::json::value treepaths_value(
	const std::set<std::vector<std::string>>& l)
{
	using value = format::json::value;
	value v = value::array();
	for (const auto& tp : l) {
		value p = value::array();
		for (const auto& s : tp) p.push_back(value::string(s));
		v.push_back(std::move(p));
	}
	return v;
}

static const char* verbosity_name(
	tgf_repl_evaluator::parser_type::error::info_lvl v)
{
	using lvl = tgf_repl_evaluator::parser_type::error::info_lvl;
	switch (v) {
	case lvl::INFO_BASIC:      return "basic";
	case lvl::INFO_DETAILED:   return "detailed";
	case lvl::INFO_ROOT_CAUSE: return "root-cause";
	default:                   return "unknown";
	}
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

static void print_diagnostics_report(
	const idni::diagnostics::report& report,
	bool json, bool print_names = true)
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

static pair<tgf_repl_parser::nonterminal, tt> get_opt(const tt& t) {
	using p = tgf_repl_parser;
	static const map<p::nonterminal, p::nonterminal> ov{
		{ p::bool_option,      p::bool_value },
		{ p::list_option,      p::symbol_list },
		{ p::treepaths_option, p::treepath_list },
		{ p::enum_ev_option,   p::error_verbosity },
		{ p::symbol_option,    p::symbol }
	};
	for (auto it = ov.begin(); it != ov.end(); ++it)
		if (auto x = t | it->first; x.has_value())
			return { static_cast<p::nonterminal>(
					x | tt::only_child | tt::nonterminal),
				t | it->second };
	return { p::nul, t };
}

static bool get_bool_value(const tt& t) {
	return (t | tt::only_child | tt::nonterminal)
		== tgf_repl_parser::true_value;
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

const std::string& tgf_repl_evaluator::start_symbol() const noexcept {
	return opt.start;
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

void tgf_repl_evaluator::print_source(ostream& os) const {
	os << "grammar:\n";
	if (!grammar_source.empty()) {
		print_text_source(os, grammar_source);
		os << "\n\n";
		return;
	}

	ifstream f(tgf_filename);
	if (!f) {
		os << "error: could not open file: " << tgf_filename << "\n";
		return;
	}
	string line;
	while (getline(f, line)) print_text_source(os, line + "\n");
	os << "\n";
}

void tgf_repl_evaluator::reprompt() {
	stringstream ss;
	if (opt.status) {
		ss << TC_STATUS << "[ ";
		if (has_grammar())
			ss << TC_STATUS_FILE << "\"" << tgf_filename
				<< "\"" << TC.CLEAR() << TC_STATUS << " ";
		if (has_grammar() || !opt.start.empty())
			ss << TC_STATUS_START << opt.start
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
	print_diagnostics_report(report, opt.print_json);
	report.clear();
}

bool tgf_repl_evaluator::load_file(const std::string& filename) {
	auto next_nts = make_unique<nonterminals_type>();
	auto gr = tgf<char_type, terminal_type>::from_file(*next_nts, filename,
		opt.measure);
	if (!gr.has_value()) {
		report.append(std::move(gr).report());
		return false;
	}
	auto next_grammar = make_unique<grammar_type>(std::move(gr).value());
	auto next_parser = make_unique<parser_type>(*next_grammar,
		default_parser_options<char_type, terminal_type>());
	report.append(std::move(gr).report());

	owned_nts     = std::move(next_nts);
	owned_g       = std::move(next_grammar);
	owned_p       = std::move(next_parser);
	p_            = owned_p.get();
	tgf_filename = filename;
	grammar_source.clear();
	grammar_loaded = true;
	return true;
}

size_t tgf_repl_evaluator::nt_id(const std::string& s) {
	return g().nt(from_str<char_type>(s)).n();
}

std::string tgf_repl_evaluator::nt_name(size_t id) const {
	return g().nt(id).to_std_string();
}

void tgf_repl_evaluator::update_opts_by_grammar_opts() {
	auto ntids2strs = [this] (const set<size_t>& ntids) {
		set<string> r;
		for (const auto& id : ntids) r.insert(nt_name(id));
		return r;
	};
	opt.to_trim          = ntids2strs(g().opt.shaping.to_trim);
	opt.to_trim_children = ntids2strs(g().opt.shaping.to_trim_children);
	opt.dont_trim_terminals_of =
		ntids2strs(g().opt.shaping.dont_trim_terminals_of);
	opt.to_trim_children_terminals =
		ntids2strs(g().opt.shaping.to_trim_children_terminals);
	opt.to_inline.clear();
	for (const auto& tp : g().opt.shaping.to_inline) {
		vector<string> v;
		for (const auto& s : tp) v.push_back(nt_name(s));
		opt.to_inline.insert(std::move(v));
	}
	if (opt.start.size() == 0)
		opt.start = g().start_literal().to_std_string();
}

// TODO (HIGH) replace by generic option origin tracking: every option
// resolves default < grammar directive < CLI, not one flag per field.
void tgf_repl_evaluator::apply_auto_disambiguate() {
	if (opt.auto_disambiguate_user_set)
		g().opt.auto_disambiguate = opt.auto_disambiguate;
	else if (!g().opt.auto_disambiguate_set_by_grammar)
		g().opt.auto_disambiguate = opt.auto_disambiguate;
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
	if (!opt.json_api) {
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

tgf_repl_evaluator::parser_type::parse_options
	tgf_repl_evaluator::get_parse_options()
{
	parser_type::parse_options po{
		.start              = nt_id(opt.start),
		.measure            = opt.measure,
		.measure_each_pos   = opt.measure_each_pos,
		.measure_forest     = opt.measure_forest,
		.measure_preprocess = opt.measure_preprocess,
		.debug              = opt.debug,
		.error_verbosity    = opt.error_verbosity,
		.tree_path          = opt.tree_path,
		.enable_gc          = opt.gc
	};
	if (opt.measure) { /// `opt.measure` is a master ENABLE for now
		po.measure_scopes       = true;
		po.measure_counters     = true;
		po.measure_forest       = true;
		po.measure_preprocess   = true;
	}
	return po;
}

format::json::value tgf_repl_evaluator::parse(const char* input, size_t size,
	std::string& text)
{
	if (!require_grammar()) return format::json::value::object();
	if (!good()) return format::json::value::object();
	auto po = get_parse_options();
	auto r = p().parse(input, size, po);
	return parsed(r, text);
}

format::json::value tgf_repl_evaluator::parse(istream& instream,
	std::string& text)
{
	if (!require_grammar()) return format::json::value::object();
	if (!good()) return format::json::value::object();
	auto po = get_parse_options();
	auto r = p().parse(instream, po);
	return parsed(r, text);
}

format::json::value tgf_repl_evaluator::parse(const string& infile,
	std::string& text)
{
	if (!require_grammar()) return format::json::value::object();
	if (!good()) return format::json::value::object();
	auto po = get_parse_options();
	auto r = p().parse(infile, po);
	return parsed(r, text);
}

shaping_options tgf_repl_evaluator::shaping() {
	auto str2ntids = [this](const set<string>& list) {
		set<size_t> r;
		for (const auto& s : list) r.insert(nt_id(s));
		return r;
	};
	shaping_options sopt;
	sopt.trim_terminals = g().opt.shaping.trim_terminals;
	sopt.inline_char_classes = g().opt.shaping.inline_char_classes;
	sopt.to_trim = str2ntids(opt.to_trim);
	sopt.to_trim_children = str2ntids(opt.to_trim_children);
	sopt.dont_trim_terminals_of = str2ntids(opt.dont_trim_terminals_of);
	sopt.to_trim_children_terminals =
		str2ntids(opt.to_trim_children_terminals);
	for (const auto& tp : opt.to_inline) {
		vector<size_t> v;
		for (const auto& s : tp) v.push_back(nt_id(s));
		sopt.to_inline.insert(v);
	}
	return sopt;
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

bool tgf_repl_evaluator::load_grammar(const std::string& new_tgf_file) {
	if (!load_file(new_tgf_file)) return false;
	update_opts_by_grammar_opts();
	apply_auto_disambiguate();
	return true;
}

bool tgf_repl_evaluator::reload(const string& new_tgf_file) {
	if (fixed_grammar) {
		report.warning(parser_strings::messages::loading_grammars_unavailable);
		return false;
	}
	if (!load_grammar(new_tgf_file)) {
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
	if (!load_grammar(new_tgf_file))
		return v.set("grammar", value::string(new_tgf_file))
			.set("loaded", value::boolean(false));
	return v.set("grammar", value::string(tgf_filename))
		.set("loaded", value::boolean(true));
}

format::json::value tgf_repl_evaluator::get_cmd(const tt& n) {
	using value = format::json::value;
	if (!n) {
		auto v = value::object();
		v.set("options", option_values());
		return v;
	}
	auto [o, _] = get_opt(n);
	const option_desc* d = find_option(o);
	if (!d) {
		report.error(diagnostics::code::invalid_argument,
			parser_strings::messages::unknown_option);
		return value::object();
	}
	auto v = value::object();
	v.set("option", value::string(d->name))
	 .set("value", option_value(o));
	return v;
}

format::json::value tgf_repl_evaluator::option_value(size_t o) const {
	using value = format::json::value;
	switch (o) {
	case tgf_repl_parser::status_opt:
		return value::boolean(opt.status);
	case tgf_repl_parser::colors_opt:
		return value::boolean(opt.colors);
	case tgf_repl_parser::print_ambiguity_opt:
		return value::boolean(opt.print_ambiguity);
	case tgf_repl_parser::print_graphs_opt:
		return value::boolean(opt.print_graphs);
	case tgf_repl_parser::print_rules_opt:
		return value::boolean(opt.tml_rules);
	case tgf_repl_parser::print_facts_opt:
		return value::boolean(opt.tml_facts);
	case tgf_repl_parser::print_terminals_opt:
		return value::boolean(opt.print_terminals);
	case tgf_repl_parser::measure_parsing_opt:
		return value::boolean(opt.measure);
	case tgf_repl_parser::measure_each_pos_opt:
		return value::boolean(opt.measure_each_pos);
	case tgf_repl_parser::measure_forest_opt:
		return value::boolean(opt.measure_forest);
	case tgf_repl_parser::measure_preprocess_opt:
		return value::boolean(opt.measure_preprocess);
	case tgf_repl_parser::gc_opt:
		return value::boolean(opt.gc);
	case tgf_repl_parser::debug_opt:
		return value::boolean(opt.debug);
	case tgf_repl_parser::auto_disambiguate_opt:
		return value::boolean(g().opt.auto_disambiguate);
	case tgf_repl_parser::trim_terminals_opt:
		return value::boolean(g().opt.shaping.trim_terminals);
	case tgf_repl_parser::inline_cc_opt:
		return value::boolean(g().opt.shaping.inline_char_classes);
	case tgf_repl_parser::derive_char_classes_opt:
		return value::boolean(g().opt.derive_char_classes);
	case tgf_repl_parser::nodisambig_list_opt: {
		value v = value::array();
		for (size_t id : g().opt.nodisambig_list)
			v.push_back(value::string(nt_name(id)));
		return v;
	}
	case tgf_repl_parser::enabled_prods_opt:
		return strings_value(g().opt.enabled_guards);
	case tgf_repl_parser::trim_opt:
		return strings_value(opt.to_trim);
	case tgf_repl_parser::trim_children_opt:
		return strings_value(opt.to_trim_children);
	case tgf_repl_parser::trim_children_terminals_opt:
		return strings_value(opt.to_trim_children_terminals);
	case tgf_repl_parser::inline_opt:
		return treepaths_value(opt.to_inline);
	case tgf_repl_parser::error_verbosity_opt:
		return value::string(verbosity_name(opt.error_verbosity));
	case tgf_repl_parser::start_opt:
		return opt.start.empty() ? value::null()
			: value::string(opt.start);
	default: return value::null();
	}
}

format::json::value tgf_repl_evaluator::option_values() const {
	using value = format::json::value;
	auto v = value::object();
	for (const auto& [nt, d] : option_table)
		v.set(d.name, option_value(nt));
	return v;
}

vector<string> tgf_repl_evaluator::treepath(const tt& tp) const {
	vector<string> v;
	for (const auto& s : (tp || tgf_repl_parser::symbol)())
		v.push_back(s | tt::terminals);
	return v;
}

format::json::value tgf_repl_evaluator::set_cmd(const tt& n) {
	using p = tgf_repl_parser;
	using value = format::json::value;
	auto [o, v] = get_opt(n);
	switch (o) {
	case p::debug_opt:
		opt.debug = get_bool_value(v); break;
	case p::status_opt:
		opt.status = get_bool_value(v); break;
	case p::colors_opt: {
		opt.colors = get_bool_value(v);
		if (!opt.json_api) TC.set(opt.colors);
		break;
	}
	case p::print_terminals_opt:
		opt.print_terminals = get_bool_value(v); break;
	case p::print_graphs_opt:
		opt.print_graphs = get_bool_value(v); break;
	case p::print_ambiguity_opt:
		opt.print_ambiguity = get_bool_value(v); break;
	case p::print_rules_opt:
		opt.tml_rules = get_bool_value(v); break;
	case p::print_facts_opt:
		opt.tml_facts = get_bool_value(v); break;
	case p::measure_parsing_opt:
		opt.measure = get_bool_value(v); break;
	case p::measure_each_pos_opt:
		opt.measure_each_pos = get_bool_value(v); break;
	case p::measure_forest_opt:
		opt.measure_forest = get_bool_value(v); break;
	case p::measure_preprocess_opt:
		opt.measure_preprocess = get_bool_value(v); break;
	case p::gc_opt: opt.gc = get_bool_value(v); break;
	case p::trim_terminals_opt:
		g().opt.shaping.trim_terminals = get_bool_value(v); break;
	case p::inline_cc_opt:
		g().opt.shaping.inline_char_classes = get_bool_value(v); break;
	case p::trim_opt:
		opt.to_trim.clear();
		for (const auto& s : (v || p::symbol)())
			opt.to_trim.insert(s | tt::terminals);
		break;
	case p::enabled_prods_opt: {
		std::set<std::string> grds;
		for (const auto& s : (v || p::symbol)())
			grds.insert(s | tt::terminals);
		if (grds != g().opt.enabled_guards)
			g().set_enabled_productions(grds);
		break;
	}
	case p::trim_children_opt:
		opt.to_trim_children.clear();
		for (const auto& s : (v || p::symbol)())
			opt.to_trim_children.insert(s | tt::terminals);
		break;
	case p::trim_children_terminals_opt:
		opt.to_trim_children_terminals.clear();
		for (const auto& s : (v || p::symbol)())
			opt.to_trim_children_terminals.insert(s | tt::terminals);
		break;
	case p::inline_opt:
		opt.to_inline.clear();
		for (const auto& tp : (v || p::treepath)())
			opt.to_inline.insert(treepath(tp));
		break;
	case p::auto_disambiguate_opt:
		g().opt.auto_disambiguate = get_bool_value(v); break;
	case p::derive_char_classes_opt:
		g().derive_char_classes(get_bool_value(v)); break;
	case p::start_opt:
		opt.start = v | tt::terminals;
		break;
	case p::nodisambig_list_opt:
		g().opt.nodisambig_list.clear();
		for (const auto& s : (v || p::symbol)())
			g().opt.nodisambig_list.insert(nt_id(s | tt::terminals));
		break;
	case p::error_verbosity_opt: {
		auto vrb = v;
		if (!vrb.has_value()) {
			report.error(diagnostics::code::invalid_argument,
				parser_strings::messages::invalid_error_verbosity);
			return value::object();
		}
		auto vrb_type = vrb | tt::only_child | tt::nonterminal;
		using lvl = parser_type::error::info_lvl;
		switch (vrb_type) {
		case p::basic_sym:
			opt.error_verbosity = lvl::INFO_BASIC; break;
		case p::detailed_sym:
			opt.error_verbosity = lvl::INFO_DETAILED; break;
		case p::root_cause_sym:
			opt.error_verbosity = lvl::INFO_ROOT_CAUSE; break;
		default:
			report.error(diagnostics::code::invalid_argument,
				parser_strings::messages::invalid_error_verbosity);
			return value::object();
		}
		break;
	}
	default:
		report.error(diagnostics::code::invalid_argument,
			parser_strings::messages::unknown_option);
		return value::object();
	};
	return get_cmd(n);
}

format::json::value tgf_repl_evaluator::add_cmd(const tt& n) {
	using p = tgf_repl_parser;
	using value = format::json::value;
	auto [o, v] = get_opt(n);
	if (o == p::inline_opt) {
		for (const auto& tp : (v || p::treepath)())
			opt.to_inline.insert(treepath(tp));
		return get_cmd(n);
	}
	if (o == p::enabled_prods_opt) {
		for (const auto& s : (v || p::symbol)())
			g().opt.enabled_guards.insert(s | tt::terminals);
		g().set_enabled_productions(g().opt.enabled_guards);
		return get_cmd(n);
	}
	if (o == p::nodisambig_list_opt) {
		for (const auto& s : (v || p::symbol)())
			g().opt.nodisambig_list.insert(nt_id(s | tt::terminals));
		return get_cmd(n);
	}
	if (o == p::trim_opt) {
		for (const auto& s : (v || p::symbol)())
			opt.to_trim.insert(s | tt::terminals);
		return get_cmd(n);
	}
	if (o == p::trim_children_opt) {
		for (const auto& s : (v || p::symbol)())
			opt.to_trim_children.insert(s | tt::terminals);
		return get_cmd(n);
	}
	if (o == p::trim_children_terminals_opt) {
		for (const auto& s : (v || p::symbol)())
			opt.to_trim_children_terminals.insert(s | tt::terminals);
		return get_cmd(n);
	}
	report.error(diagnostics::code::invalid_argument,
		parser_strings::messages::unknown_option);
	return value::object();
}

format::json::value tgf_repl_evaluator::del_cmd(const tt& n) {
	using p = tgf_repl_parser;
	using value = format::json::value;
	auto [o, v] = get_opt(n);
	if (o == p::inline_opt) {
		for (const auto& tp : (v || p::treepath)())
			opt.to_inline.erase(treepath(tp));
		return get_cmd(n);
	}
	if (o == p::enabled_prods_opt) {
		for (const auto& s : (v || p::symbol)())
			g().opt.enabled_guards.erase(s | tt::terminals);
		g().set_enabled_productions(g().opt.enabled_guards);
		return get_cmd(n);
	}
	if (o == p::nodisambig_list_opt) {
		for (const auto& s : (v || p::symbol)())
			g().opt.nodisambig_list.erase(nt_id(s | tt::terminals));
		return get_cmd(n);
	}
	if (o == p::trim_opt) {
		for (const auto& s : (v || p::symbol)())
			opt.to_trim.erase(s | tt::terminals);
		return get_cmd(n);
	}
	if (o == p::trim_children_opt) {
		for (const auto& s : (v || p::symbol)())
			opt.to_trim_children.erase(s | tt::terminals);
		return get_cmd(n);
	}
	if (o == p::trim_children_terminals_opt) {
		for (const auto& s : (v || p::symbol)())
			opt.to_trim_children_terminals.erase(s | tt::terminals);
		return get_cmd(n);
	}
	report.error(diagnostics::code::invalid_argument,
		parser_strings::messages::unknown_option);
	return value::object();
}

format::json::value tgf_repl_evaluator::update_bool_opt_cmd(
	const tt& n,
	const function<bool(bool&)>& update_fn)
{
	using p = tgf_repl_parser;
	using value = format::json::value;
	auto option_type = n | tgf_repl_parser::bool_option
		| tt::only_child | tt::nonterminal;
	switch (option_type) {
	case p::debug_opt:             update_fn(opt.debug); break;
	case p::status_opt:            update_fn(opt.status); break;
	case p::colors_opt: {
		bool b = update_fn(opt.colors);
		if (!opt.json_api) TC.set(b);
		break;
	}
	case p::print_terminals_opt:   update_fn(opt.print_terminals); break;
	case p::print_graphs_opt:      update_fn(opt.print_graphs); break;
	case p::print_ambiguity_opt:   update_fn(opt.print_ambiguity); break;
	case p::print_rules_opt:       update_fn(opt.tml_rules); break;
	case p::print_facts_opt:       update_fn(opt.tml_facts); break;
	case p::measure_parsing_opt:   update_fn(opt.measure); break;
	case p::measure_each_pos_opt:  update_fn(opt.measure_each_pos); break;
	case p::measure_forest_opt:    update_fn(opt.measure_forest); break;
	case p::measure_preprocess_opt:update_fn(opt.measure_preprocess); break;
	case p::gc_opt:                update_fn(opt.gc); break;
	case p::auto_disambiguate_opt: update_fn(g().opt.auto_disambiguate); break;
	case p::derive_char_classes_opt: {
		bool on = update_fn(g().opt.derive_char_classes);
		g().derive_char_classes(on);
		break;
	}
	case p::trim_terminals_opt:    update_fn(g().opt.shaping.trim_terminals); break;
	case p::inline_cc_opt:         update_fn(g().opt.shaping.inline_char_classes); break;
	default:
		report.error(diagnostics::code::invalid_argument,
			parser_strings::messages::unknown_bool_option);
		return value::object();
	}
	return get_cmd(n);
}

// TODO (LOW) write proper help messages
static std::string help_text(size_t nt, bool show_load_reload) {
	using p = tgf_repl_parser;
	std::ostringstream os;
	static const string bool_options =
		"  status                 show status                        on/off\n"
		"  colors                 use term colors                    on/off\n"
		"  print-ambiguity        prints ambiguous nodes             on/off\n"
		"  print-terminals        prints parsed terminals            on/off\n"
		"  print-graphs           prints parsed graphs               on/off\n"
		"  print-rules            prints parsed forest as TML rules  on/off\n"
		"  print-facts            prints parsed forest as TML facts  on/off\n"
		"  measure-parsing        measures parsing time              on/off\n"
		"  measure-each-pos       measures parsing time of each pos  on/off\n"
		"  measure-forest         measures forest building time      on/off\n"
		"  measure-preprocess     measures forest preprocess time    on/off\n"
		"  gc                     Earley chart garbage collection    on/off\n"
		"  trim-terminals         trim terminals                     on/off\n"
		"  inline-char-classes    inline character classes           on/off\n"
		"  derive-char-classes    derived character classes         on/off\n";
	static const string list_options =
		"  nodisambig-list        list of nodes to keep ambiguous    symbol1, symbol2...\n"
		"  trim                   list of nodes to trim              symbol1, symbol2...\n"
		"  trim-children          list of nodes to trim children     symbol1, symbol2...\n";
	static const string treepaths_options =
		"  inline                 list of tree paths to inline       symbol1 > ch1 > ch2, symbol2...\n";
	static const string enum_ev_option =
		"  error-verbosity        parse errors verbosity             basic/detailed/root-cause\n";
	static const string symbol_options =
		"  start                  get or set the start symbol        symbol\n";
	static const string all_available_options = string{} +
		"Available options:\n" + bool_options + list_options
			+ treepaths_options + symbol_options + enum_ev_option;
	static const string bool_available_options = string{} +
		"Available options:\n" + bool_options;
	static const string list_and_treepaths_available_options =
		string{} +
		"Available options:\n" + list_options + treepaths_options;
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
		<< list_and_treepaths_available_options;
		break;
	case p::del_sym: os
		<< "command: delete <option> <value>\n"
		<< "or: del, remove, rem or rm\n"
		<< "\tremoves the value from the given option list\n"
		<< "\n"
		<< list_and_treepaths_available_options;
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
		auto _ = report.open_if(opt.measure || nt == p::parse_cmd,
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
			res.data = get_cmd(s | p::option);
			break;
		case p::set_cmd:
			res.data = set_cmd(s);
			break;
		case p::toggle_cmd:
			res.data = update_bool_opt_cmd(s,
				[](bool& b){ return b = !b; });
			break;
		case p::enable_cmd:
			res.data = update_bool_opt_cmd(s,
				[](bool& b){ return b = true; });
			break;
		case p::disable_cmd:
			res.data = update_bool_opt_cmd(s,
				[](bool& b){ return b = false; });
			break;
		case p::add_cmd:
			res.data = add_cmd(s);
			break;
		case p::del_cmd:
			res.data = del_cmd(s);
			break;
		case p::reload_cmd:
			if (fixed_grammar) {
				report.warning(parser_strings::messages::
					loading_grammars_unavailable);
				break;
			}
			if (!require_grammar()) break;
			res.data = reload_data(tgf_filename);
			break;
		case p::load_cmd: {
			if (fixed_grammar) {
				report.warning(parser_strings::messages::
					loading_grammars_unavailable);
				break;
			}
			auto n = s | p::filename;
			auto filename = unquote(n | tt::terminals, report);
			if (report.has_error()) break;
			res.data = reload_data(filename);
			break;
		}
		case p::start_cmd: {
			auto n = s | p::symbol;
			string start;
			bool changed = false;
			if (n.has_value()
				&& (start = n | tt::terminals).size()) {
				opt.start = start;
				changed = true;
			}
			auto v = value::object();
			v.set("start", opt.start.empty() ? value::null()
				: value::string(opt.start))
			 .set("changed", value::boolean(changed));
			res.data = std::move(v);
			break;
		}
		case p::igrammar_cmd: {
			if (!require_grammar()) break;
			auto n = s | p::symbol;
			string start = n.has_value()
				? n | tt::terminals : opt.start;
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
			if (!opt.json_api) {
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
			string src = grammar_source;
			if (src.empty()) {
				// The exact bytes of the file, so no line translation.
				ifstream f(tgf_filename, ios::binary);
				src.assign(istreambuf_iterator<char>(f),
					istreambuf_iterator<char>());
			}
			v.set("source", value::string(std::move(src)));
			res.data = std::move(v);
			break;
		}
		case p::unreachable_cmd: {
			if (!require_grammar()) break;
			auto n = s | p::symbol;
			string start = n.has_value()
				? n | tt::terminals : opt.start;
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
			if (!opt.json_api) {
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
	if (!opt.json_api) {
		r.print_ambiguous_nodes(cout);
		if (opt.debug) pretty_print(cout << "input command graph:\n",
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

void tgf_repl_evaluator::render_text(const cmd_result& r,
	std::ostream& os) const
{
	if (!r.text.empty()) { os << r.text; return; }
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
	if (r.cmd == "grammar") { print_source(os); return; }
	if (r.cmd == "load" || r.cmd == "reload") {
		auto l = r.data.find("loaded");
		if (!l) return;   // fixed grammar: only the warning
		auto name = r.data.find("grammar");
		if (l->as_bool()) os << "loaded: " << name->as_string() << "\n";
		else os << "reload failed: " << name->as_string() << "\n";
		return;
	}
	if (auto o = r.data.find("option"); o) {
		const option_desc* d = option_desc_by_name(o->as_string());
		os << (d ? d->label : "")
			<< option_text(*r.data.find("value")) << "\n";
		return;
	}
	if (auto opts = r.data.find("options"); opts) {
		for (const auto& kv : opts->members()) {
			const option_desc* d = option_desc_by_name(kv.first);
			os << (d ? d->label : "")
				<< option_text(kv.second) << "\n";
		}
		return;
	}
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
	TC.set(opt.json_api ? false : opt.colors);
	if (!load_file(tgf_filename)) return;
	update_opts_by_grammar_opts();
	apply_auto_disambiguate();
}

tgf_repl_evaluator::tgf_repl_evaluator(options opt)
	: opt(opt)
{
	TC.set(opt.json_api ? false : opt.colors);
	owned_nts = make_unique<nonterminals_type>();
	owned_g = make_unique<grammar_type>(*owned_nts);
	owned_p = make_unique<parser_type>(*owned_g,
		default_parser_options<char_type, terminal_type>());
	p_ = owned_p.get();
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
	TC.set(opt.json_api ? false : opt.colors);
	update_opts_by_grammar_opts();
	apply_auto_disambiguate();
}

static tgf_repl_evaluator::parser_type::error::info_lvl
str2error_verbosity(const string& str)
{
	using lvl = tgf_repl_evaluator::parser_type::error::info_lvl;
	if (str == "detailed")   return lvl::INFO_DETAILED;
	if (str == "root-cause") return lvl::INFO_ROOT_CAUSE;
	if (str != "basic") cerr << "error: invalid error-verbosity: "
				"\"" << str << "\". setting to \"basic\"\n";
	return lvl::INFO_BASIC;
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

// cli::command::has() reports whether a command's schema declares an
// option, not whether argv actually carried it (every command declares
// auto-disambiguate with a default). Scan the raw args for the long
// flag to tell an explicit user override from that default.
static bool user_passed_long_flag(const vector<string>& argv,
	const string& name)
{
	string flag = "--" + name;
	for (auto& a : argv)
		if (a == flag || a.rfind(flag + "=", 0) == 0) return true;
	return false;
}

static tgf_repl_evaluator::options
	repl_options_from_cmd(const cli::command& cmd, const vector<string>& argv)
{
	tgf_repl_evaluator::options tgf_repl_opt;
	if (cmd.has("status"))                  tgf_repl_opt.status =
		cmd.get<bool>("status");
	if (cmd.has("colors"))                  tgf_repl_opt.colors =
		cmd.get<bool>("colors");
	if (cmd.has("measure"))                 tgf_repl_opt.measure =
		cmd.get<bool>("measure");
	if (cmd.has("json")) {
		tgf_repl_opt.print_json = cmd.get<bool>("json");
		tgf_repl_opt.json_api  = tgf_repl_opt.print_json;
	}
	if (cmd.has("print-input"))             tgf_repl_opt.print_input =
		cmd.get<bool>("print-input");
	if (cmd.has("print-ambiguity"))         tgf_repl_opt.print_ambiguity =
		cmd.get<bool>("print-ambiguity");
	if (cmd.has("print-graphs"))            tgf_repl_opt.print_graphs =
		cmd.get<bool>("print-graphs");
	if (cmd.has("tml-rules"))               tgf_repl_opt.tml_rules =
		cmd.get<bool>("tml-rules");
	if (cmd.has("tml-facts"))               tgf_repl_opt.tml_facts =
		cmd.get<bool>("tml-facts");
	if (cmd.has("error-verbosity"))         tgf_repl_opt.error_verbosity =
		str2error_verbosity(cmd.get<string>("error-verbosity"));
	if (cmd.has("start"))
		tgf_repl_opt.start = cmd.get<string>("start");
	if (cmd.has("tree-path"))
		tgf_repl_opt.tree_path =
			cmd.get<string>("tree-path") == "forest"
				? parse_tree_path::forest_path
				: parse_tree_path::bintree_path;
	if (user_passed_long_flag(argv, "auto-disambiguate")) {
		tgf_repl_opt.auto_disambiguate =
			cmd.get<bool>("auto-disambiguate");
		tgf_repl_opt.auto_disambiguate_user_set = true;
	}
	return tgf_repl_opt;
}

static parser_gen_options gen_options_from_cmd(const cli::command& cmd) {
	vector<string> nodisambig_list;
	if (cmd.has("nodisambig-list"))
		for (auto&& s : cmd.get<string>("nodisambig-list")
			| views::split(',')) nodisambig_list
				.emplace_back(s.begin(), s.end());
	auto get_str = [&](const char* n) {
		return cmd.has(n) ? cmd.get<string>(n) : "";
	};
	auto get_bool = [&](const char* n) {
		return cmd.has(n) && cmd.get<bool>(n);
	};
	// several @dynamic nonterminals separated by ';', values by ','
	map<string, vector<string>> dynamic;
	for (auto&& e : get_str("dynamic") | views::split(';')) {
		string entry(e.begin(), e.end());
		if (entry.empty()) continue;
		auto eq = entry.find('=');
		if (eq == string::npos) {
			cerr << "invalid --dynamic entry (missing '='): "
				<< entry << '\n';
			continue;
		}
		vector<string> values;
		for (auto&& v : string_view(entry).substr(eq + 1)
			| views::split(',')) values
					.emplace_back(v.begin(), v.end());
		dynamic[entry.substr(0, eq)] = values;
	}
	string char_type     = get_str("char-type");
	string terminal_type = get_str("terminal-type");
	string decoder       = get_str("decoder");
	string encoder       = get_str("encoder");
	if (get_bool("utf8")) {
		char_type     = "char";
		terminal_type = "char32_t";
		if (decoder.empty()) decoder = "idni::utf8_to_u32_conv";
		if (encoder.empty()) encoder = "idni::u32_to_utf8_conv";
	}
	return parser_gen_options{
		.output_dir          = get_str("output-dir"),
		.output              = get_str("output"),
		.name                = get_str("name"),
		.ns                  = get_str("namespace"),
		.char_type           = char_type,
		.terminal_type       = terminal_type,
		.decoder             = decoder,
		.encoder             = encoder,
		.auto_disambiguate   = get_bool("auto-disambiguate"),
		.nodisambig_list     = nodisambig_list,
		.dynamic             = dynamic,
		.header_only         = get_bool("header-only"),
		.treemr              = get_bool("treemr")
	};
}

static int gen_command(const cli::command& cmd,
	tgf_repl_evaluator& re)
{
	using format::json::value;
	const bool print_json = cmd.has("json") && cmd.get<bool>("json");
	if (!re.require_grammar()) {
		if (print_json) {
			json_write_line(cout, json_result_response(
				cmd_status::error, "gen", value::object(),
				state_value(re), re.take_report()));
			return 1;
		}
		re.flush_report();
		return 1;
	}
	auto gen_opt = gen_options_from_cmd(cmd);
	auto gr = !re.has_fixed_grammar()
		? generate_parser_cpp_from_file<char>(re.filename(), gen_opt,
			false)
		: generate_parser_cpp_from_string<char>(re.filename(),
			re.source(), gen_opt, false);
	if (print_json) {
		auto data = value::object();
		auto files = value::array();
		if (gr.has_value())
			for (const auto& f : gr.value())
				files.push_back(value::string(f));
		data.set("files", std::move(files));
		cmd_status st = gr.has_value()
			? cmd_status::ok : cmd_status::error;
		json_write_line(cout, json_result_response(st, "gen", data,
			state_value(re), gr.report()));
		return st == cmd_status::error ? 1 : 0;
	}
	print_diagnostics_report(gr.report(), false);
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

static int show_command(const cli::command& cmd,
	tgf_repl_evaluator& re)
{
	using format::json::value;
	const bool json = cmd.has("json") && cmd.get<bool>("json");
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
	string start = cmd.get<string>("start");
	if (start.empty()) start = re.g().start_literal().to_std_string();
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
		if (cmd.get<bool>("nullable")) {
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
	if (cmd.get<bool>("grammar")) re.g().print_internal_grammar_for(
		cout, start, {}, true);
	if (cmd.get<bool>("nullable"))
		re.g().check_nullable_recursive_production(cout);
	if (cmd.has("char-class-report")
		&& cmd.get<bool>("char-class-report"))
			print_char_class_report(re);
	re.flush_report();
	return 0;
}

static int run_command(cli& cl, const cli::command& cmd,
	tgf_repl_evaluator& re)
{
	// apply --productions from command line
	if (string prods = cmd.has("productions")
			? cmd.get<string>("productions") : string{};
		!prods.empty())
	{
		std::set<std::string> grds;
		for (auto&& g : prods | views::split(','))
			grds.emplace(g.begin(), g.end());
		re.g().set_enabled_productions(grds);
	}

	if (cmd.name() == "grammar") return show_command(cmd, re);

	if (cmd.name() == "gen") return gen_command(cmd, re);

	if (cmd.name() == "test") {
		auto files = cl.get_files();
		if (!files.size()) return cl.error(
			"test command needs at least one file as an argument");
		int ret = run_tests(re.p(), files);
		re.flush_report();
		return ret;
	}

	if (cmd.name() == "repl") {
		const bool json = cmd.has("json") && cmd.get<bool>("json");
		if (json) {
			if (auto ev = cmd.get<string>("evaluate"); ev.size()) {
				auto er = re.run(ev);
				json_write_line(cout, json_eval_response(
					format::json::value::null(), er,
					state_value(re)));
				return er.status == cmd_status::error ? 1 : 0;
			}
			return tgf_json_loop(re, cin, cout);
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

		if (auto eval = cmd.get<string>("evaluate"); eval.size())
			return with_report_flush([&] {
				return re.eval(eval).value_or(0);
			});
		if (cmd.get<bool>("legacy-repl"))
			return with_report_flush(run_legacy);
		return with_report_flush(run_default);
	}

	if (cmd.name() == "parse") {
		// the classes are on by default; the option can turn them off
		if (cmd.has("derive-char-classes")
			&& !cmd.get<bool>("derive-char-classes"))
				re.g().derive_char_classes(false);
		using format::json::value;
		const bool json = cmd.has("json") && cmd.get<bool>("json");
		string infile = cmd.get<string>("input");
		string inexp  = cmd.get<string>("input-expression");
		if (infile.size() && inexp.size())
			return cl.error("multiple inputs specified, use ei"
				"ther --input or --input-expression, not both");
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
			if (cmd.get<bool>("grammar")) {
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
			if (cmd.get<bool>("measure"))
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
		if (cmd.get<bool>("grammar")) re.eval("i");
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
		if (cmd.get<bool>("measure")) print_bintree_process_totals();
		return re.has_grammar() ? 0 : 1;
	}
	return 0;
}

// Run universal CLI flags (version/license/help, cmd.ok, cmd help).
// On exit-without-running returns the exit code; otherwise returns nullopt
// and sets @p out_cmd to the parsed command.
static optional<int> cli_universal(cli& cl, cli::command& out_cmd) {
	if (cl.process_args() != 0) return cl.status();
	auto opts = cl.get_processed_options();
	bool quit = false;
	if (opts["version"].get<bool>()) { quit = true; print_version(); }
	if (opts["license"].get<bool>()) { quit = true; print_license(); }
	if (opts["help"]   .get<bool>()) { quit = true; cl.help(); }
	if (quit) return 0;
	out_cmd = cl.get_processed_command();
	if (!out_cmd.ok()) return cl.error("invalid command", true);
	if (out_cmd.get<bool>("help")) return cl.help(out_cmd), 0;
	return nullopt;
}

int tgf_specialized_run(int argc, char** argv,
	tgf_repl_evaluator::parser_type& parser,
	const char* display_name,
	const char* grammar_source_c)
{
	const string grammar_source = grammar_source_c ? grammar_source_c : "";
	const string tgf_label = display_name ? display_name : "<compiled>";
	auto cmds = tgf_commands();
	auto options = tgf_options();

	vector<string> args;
	for (int i = 0; i != argc; ++i) args.push_back(argv[i]);
	if (args.empty()) args.push_back(tgf_label);

	cli cl(args[0], args, cmds, "repl", options);
	cl.set_description("Tau Grammar Format (TGF) specialized grammar tool");
	cl.set_help_header(tgf_label);

	cli::command cmd;
	if (auto code = cli_universal(cl, cmd)) return *code;

	tgf_repl_evaluator re(parser, tgf_label, grammar_source,
		repl_options_from_cmd(cmd, args));
	if (!re.good()) return re.flush_report(), 1;

	return run_command(cl, cmd, re);
}

int tgf_run(int argc, char** argv) {
	auto cmds = tgf_commands();
	auto options = tgf_options();

	vector<string> args;
	string tgf_file{};
	bool provided = false;
	bool exists = false;
	for (int i = 0; i != argc; ++i)
		if (i == 1 && argv[i][0] != '-') {
			if (cmds.find(argv[i]) != cmds.end())
				args.push_back(argv[i]);
			else {
				provided = true, tgf_file = argv[i];
				if (std::ifstream f(tgf_file); f.good())
					exists = true;
			}
		} else args.push_back(argv[i]);

	cli cl("tgf", args, cmds, "repl", options);
	cl.set_description("Tau Grammar Format (TGF) tool");
	cl.set_help_header("tgf <TGF file>");

	cli::command cmd;
	if (auto code = cli_universal(cl, cmd)) return *code;

	if (!provided) {
		tgf_repl_evaluator re(repl_options_from_cmd(cmd, args));
		if (!re.good()) return re.flush_report(), 1;
		return run_command(cl, cmd, re);
	}
	if (!exists) return cl.error("TGF file does not exist ", true);

	tgf_repl_evaluator re(tgf_file, repl_options_from_cmd(cmd, args));
	if (!re.good()) return re.flush_report(), 1;

	return run_command(cl, cmd, re);
}

#ifdef TAU_PARSER_HAS_FTXUI
template struct repl_ftxui<tgf_repl_evaluator>;
#endif

} // namespace idni
