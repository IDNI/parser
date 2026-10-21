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
#include "utility/win_args.h"
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
using jt = format::json::tree;
using jtt = format::json::tt;

static tref range_value(const std::array<size_t, 2>& range) {
	return jt::build_array({
		jt::build_number(static_cast<double>(range[0])),
		jt::build_number(static_cast<double>(range[1])) });
}

// Text form of an option value: on/off, a number, a text, or a list
// joined with ", ".
static std::string option_text(const jtt& v) {
	if (!v) return {};
	const jt& t = v.value_tree();
	if (t.is_bool()) return t.get_bool() ? "on" : "off";
	if (t.is_number()) return std::to_string(
		static_cast<size_t>(t.get_number()));
	if (t.is_string()) return t.get_string();
	if (!t.is_array()) return {};
	if (t.children_size() == 0) return "(empty)";
	std::stringstream ss;
	bool first = true;
	for (const auto& e : (v | jtt::children)())
		ss << (first ? first = false, "" : ", ") << (e | jtt::string);
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

void print_diagnostics_report(const idni::diagnostics::report& report,
	bool json, bool print_names)
{
	if (report.nodes().empty()) return;
	if (json) {
		bintree<format::json::node>::gc_pause pause;
		format::json::print(report, std::cout, print_names) << '\n';
	} else report.print();
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
static tref bintree_process_totals_value() {
	using tree_t = tgf_repl_evaluator::parser_type::tree;
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
	auto n = [](size_t x) { return jt::build_number(
		static_cast<double>(x)); };
	// A 64-bit hash exceeds the exact JSON number range, so it is text.
	auto h = [](std::uint64_t x) {
		return jt::build_string(std::to_string(x)); };
	static constexpr const char* bin_names[8] = {
		"0", "1", "2", "3", "4-7", "8-15", "16-63", "64+" };
	trefs histogram;
	for (size_t i = 0; i != 8; ++i) histogram.push_back(
		jt::build_pair(bin_names[i], n(chain_len_histogram[i])));
	trefs top_groups;
	for (size_t i = 0; i != 5; ++i)
		top_groups.push_back(jt::build_object({
			jt::build_pair("size", n(top_group_sizes[i])),
			jt::build_pair("triples", n(top_group_triples[i])) }));
	trefs sample_arr;
	for (const auto& s : samples)
		sample_arr.push_back(jt::build_object({
			jt::build_pair("value", jt::build_string(
				s.value.first.to_std_string())),
			jt::build_pair("range", jt::build_array({
				n(s.value.second[0]), n(s.value.second[1]) })),
			jt::build_pair("left", s.left == nullptr ? jt::_null()
				: h(tree_t::get(s.left).hash)),
			jt::build_pair("right", s.right == nullptr
				? jt::_null() : h(tree_t::get(s.right).hash)),
			jt::build_pair("stored_hash", h(s.stored_hash)),
			jt::build_pair("recomputed_hash",
				h(s.recomputed_hash)) }));
	return jt::build_object({
		jt::build_pair("get_hits", n(tree_t::get_hits())),
		jt::build_pair("get_misses", n(tree_t::get_misses())),
		jt::build_pair("geth_calls", n(tree_t::geth_calls())),
		jt::build_pair("buckets", n(buckets)),
		jt::build_pair("entries", n(entries)),
		jt::build_pair("load_factor", jt::build_number(load_factor)),
		jt::build_pair("max_chain", n(max_chain)),
		jt::build_pair("mean_chain", jt::build_number(mean_chain)),
		jt::build_pair("distinct_hashes", n(distinct_hashes)),
		jt::build_pair("largest_group_size", n(largest_group_size)),
		jt::build_pair("distinct_values", n(distinct_values)),
		jt::build_pair("distinct_child_pairs",
			n(distinct_child_pairs)),
		jt::build_pair("leaf_count", n(leaf_count)),
		jt::build_pair("stale_hash_count", n(stale_hash_count)),
		jt::build_pair("largest_group_hash", h(largest_group_hash)),
		jt::build_pair("largest_group_stale_count",
			n(largest_group_stale_count)),
		jt::build_pair("chain_length_histogram",
			jt::build_object(histogram)),
		jt::build_pair("top_hash_groups", jt::build_array(top_groups)),
		jt::build_pair("largest_group_samples",
			jt::build_array(sample_arr)) });
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
	diagnostics::report res = std::move(report);
	report.clear();
	return res;
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
	for (size_t i = 0; i < l; i++) os << "\t";
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
	report.append(opts.declare(parser_option_set()).report());
	// a load keeps the parse options, a new evaluator starts from the
	// defaults
	opts.reset("parse");
}

void tgf_repl_evaluator::set_repl(repl<tgf_repl_evaluator>& r_) {
	r = &r_;
	reprompt();
}

tref tgf_repl_evaluator::parsed(parser_type::result& res) {
	if (!res.good() || !res.found) {
		report.append(std::move(res.report()));
		return jt::build_object({});
	}
	auto f = res.get_forest();
	using c_t = parser_type::char_type;
	using t_t = parser_type::terminal_type;
	trefs v;
	if (opt.print_input) v.push_back(jt::build_pair("input",
		jt::build_string(res.get_input())));
	if (opt.print_ambiguity) {
		trefs nodes;
		for (const auto& [n, alts] : res.ambiguous_nodes()) {
			trefs alts_arr;
			for (const auto& ns : alts) {
				trefs children;
				for (const auto& child : ns) {
					tref t = res.get_tree2(child);
					if (t) children.push_back(
						format::ast_json::
							node_to_tree<tree>(t));
				}
				alts_arr.push_back(jt::build_object({
					jt::build_pair("children",
						jt::build_array(children)) }));
			}
			nodes.push_back(jt::build_object({
				jt::build_pair("symbol", jt::build_string(
					n.first.to_std_string())),
				jt::build_pair("id", jt::build_number(
					static_cast<double>(n.first.n()))),
				jt::build_pair("range",
					range_value(n.second)),
				jt::build_pair("alternatives",
					jt::build_array(alts_arr)) }));
		}
		v.push_back(jt::build_pair("ambiguous", jt::build_object({
			jt::build_pair("trees", jt::build_number(
				static_cast<double>(res.count_trees()))),
			jt::build_pair("nodes", jt::build_array(nodes)) })));
	}
	if (opt.print_terminals) v.push_back(jt::build_pair("terminals",
		jt::build_string(to_std_string(res.get_terminals()))));
	if (opt.tml_rules) {
		stringstream ss;
		auto cb_next_g = [&res, &ss](parser_type::pgraph& g) {
			res.inline_grammar_transformations(g);
			to_tml_rules<c_t, t_t, parser_type::pgraph>(
				ss << "TML rules:\n", g), ss << "\n";
			return true;
		};
		f->extract_graphs(f->root(), cb_next_g);
		v.push_back(jt::build_pair("tml_rules",
			jt::build_string(ss.str())));
	}
	if (opt.tml_facts) {
		stringstream ss;
		to_tml_facts<c_t, t_t>(ss << "TML facts:\n", res);
		v.push_back(jt::build_pair("tml_facts",
			jt::build_string(ss.str())));
	}
	if (opt.print_graphs) v.push_back(jt::build_pair("tree",
		format::ast_json::node_to_tree<tree>(
			res.get_shaped_tree2(shaping()))));
	report.append(std::move(res.report()));
	return jt::build_object(v);
}

tref tgf_repl_evaluator::parse(const char* input, size_t size) {
	if (!require_grammar()) return jt::build_object({});
	if (!good()) return jt::build_object({});
	auto res = p().parse(input, size);
	return parsed(res);
}

tref tgf_repl_evaluator::parse(istream& instream) {
	if (!require_grammar()) return jt::build_object({});
	if (!good()) return jt::build_object({});
	auto res = p().parse(instream);
	return parsed(res);
}

tref tgf_repl_evaluator::parse(const std::string& infile) {
	if (!require_grammar()) return jt::build_object({});
	if (!good()) return jt::build_object({});
	auto res = p().parse(infile);
	return parsed(res);
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

tref tgf_repl_evaluator::production_id_entry(size_t p) const {
	trefs body;
	for (const auto& c : g()[p]) {
		trefs conj;
		for (const auto& l : c)
			conj.push_back(l.nt() ? jt::build_number(
				static_cast<double>(l.n())) : jt::_null());
		body.push_back(jt::build_array(conj));
	}
	const std::string* guard = g().production_guard(p);
	return jt::build_object({
		jt::build_pair("index",
			jt::build_number(static_cast<double>(p))),
		jt::build_pair("head", jt::build_number(
			static_cast<double>(g()(p).n()))),
		jt::build_pair("body", jt::build_array(body)),
		jt::build_pair("guard",
			guard ? jt::build_string(*guard) : jt::_null()),
		jt::build_pair("conjunctive",
			jt::build_bool(g().conjunctive(p))) });
}

tref tgf_repl_evaluator::productions_value(const char* key,
	const std::string& symbol, const std::set<size_t>& ps) const
{
	trefs prods, pids;
	for (size_t p : ps) {
		prods.push_back(jt::build_string(production_string(p)));
		pids.push_back(production_id_entry(p));
	}
	return jt::build_object({
		jt::build_pair(key, jt::build_string(symbol)),
		jt::build_pair("productions", jt::build_array(prods)),
		jt::build_pair("production_ids", jt::build_array(pids)) });
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

tref tgf_repl_evaluator::load_answer(bool loaded,
	const std::string& name) const
{
	return jt::build_object({
		jt::build_pair("grammar", jt::build_string(name)),
		jt::build_pair("loaded", jt::build_bool(loaded)) });
}

// Data of the load and reload commands. @p new_tgf_file is the file to
// load; on success tgf_filename names it. The caller checked fixed_grammar.
tref tgf_repl_evaluator::reload_data(const std::string& new_tgf_file) {
	if (!load_file(new_tgf_file)) return load_answer(false, new_tgf_file);
	return load_answer(true, tgf_filename);
}

tref tgf_repl_evaluator::load_source_data(const std::string& name,
	const std::string& text)
{
	if (fixed_grammar) {
		report.warning(parser_strings::messages::loading_grammars_unavailable);
		return load_answer(false, name);
	}
	if (!load_source(name, text)) return load_answer(false, name);
	return load_answer(true, tgf_filename);
}

tref tgf_repl_evaluator::reload_source_data() {
	std::string name = tgf_filename;
	std::string text = grammar_source;
	if (!load_source(name, text)) return load_answer(false, name);
	return load_answer(true, tgf_filename);
}

tref tgf_repl_evaluator::get_cmd(std::string_view name) {
	namespace ps = parser_strings;
	if (name.empty()) return jt::build_object({
		jt::build_pair("options", option_values()) });
	if (!session_option(name)) {
		report.error(diagnostics::code::not_found,
			ps::messages::option_not_found,
			{ { ps::label::name, name } });
		return jt::build_object({});
	}
	return jt::build_object({
		jt::build_pair("option", jt::build_string(name)),
		jt::build_pair("value",
			build_option(idni::options().value(name))) });
}

tref tgf_repl_evaluator::option_values() const {
	const auto& opts = idni::options();
	trefs v;
	for (const auto& name : opts.names())
		if (session_option(name)) v.push_back(
			jt::build_pair(name, build_option(opts.value(name))));
	return jt::build_object(v);
}

bool tgf_repl_evaluator::session_option(std::string_view name) const {
	auto spec = idni::options().find(name);
	return spec && std::ranges::find(tgf_session_owners, spec->owner)
		!= tgf_session_owners.end();
}

tref tgf_repl_evaluator::set_cmd(std::string_view name,
	std::string_view text)
{
	if (!option_of_kind(name, std::nullopt)) return jt::build_object({});
	auto res = idni::options().set_text(name, text);
	const bool ok = !res.has_error();
	report.append(res.report());
	if (!ok) return jt::build_object({});
	return get_cmd(name);
}

tref tgf_repl_evaluator::set_option(std::string_view name,
	idni::option_value v)
{
	if (!option_of_kind(name, std::nullopt)) return jt::build_object({});
	auto res = idni::options().set(name, std::move(v));
	const bool ok = !res.has_error();
	report.append(res.report());
	if (!ok) return jt::build_object({});
	return get_cmd(name);
}

bool tgf_repl_evaluator::option_of_kind(std::string_view name,
	std::optional<option_kind> kind)
{
	namespace ps = parser_strings;
	if (!session_option(name)) {
		report.error(diagnostics::code::not_found,
			ps::messages::option_not_found,
			{ { ps::label::name, name } });
		return false;
	}
	if (!kind || kind_of(*idni::options().find(name)) == *kind)
		return true;
	report.error(diagnostics::code::type_error,
		ps::messages::option_value_kind,
		{ { ps::label::name, name } });
	return false;
}

tref tgf_repl_evaluator::list_cmd(std::string_view name,
	std::string_view text, bool add)
{
	if (!option_of_kind(name, option_kind::list))
		return jt::build_object({});
	auto entries = idni::detail::parse_value(option_kind::list, text,
		name);
	if (!entries.has_value()) {
		report.append(entries.report());
		return jt::build_object({});
	}
	return edit_option_list(name,
		std::get<option_list>(entries.value()), add);
}

tref tgf_repl_evaluator::edit_option_list(std::string_view name,
	const option_list& entries, bool add)
{
	if (!option_of_kind(name, option_kind::list))
		return jt::build_object({});
	auto list = idni::options().get<option_list>(name);
	for (const auto& e : entries) {
		auto it = std::find(list.begin(), list.end(), e);
		if (add && it == list.end()) list.push_back(e);
		else if (!add && it != list.end()) list.erase(it);
	}
	return set_option(name, std::move(list));
}

tref tgf_repl_evaluator::flag_cmd(std::string_view name,
	const function<bool(bool)>& update)
{
	if (!option_of_kind(name, option_kind::flag))
		return jt::build_object({});
	auto& opts = idni::options();
	opts.set(name, update(opts.get<bool>(name)));
	return get_cmd(name);
}

// TODO (LOW) write proper help messages
static std::string help_text(const tgf_repl_evaluator& re, size_t nt,
	bool show_load_reload)
{
	using p = tgf_repl_parser;
	std::ostringstream os;
	// the session options of a kind, or all of them
	auto available = [&re](std::optional<option_kind> kind) {
		std::ostringstream os;
		os << "Available options:\n";
		const auto& opts = idni::options();
		for (const auto& name : opts.names()) {
			if (!re.session_option(name)) continue;
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
	const auto nt = s | tt::nonterminal;
	cmd_result res;
	res.cmd = command_name(nt);
	tref data = nullptr;
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
			data = jt::build_object({
				jt::build_pair("command",
					jt::build_string(help_arg_name(target))),
				jt::build_pair("text", jt::build_string(
					help_text(*this, target,
						!fixed_grammar))) });
			break;
		}
		case p::version_cmd: {
			data = jt::build_object({ jt::build_pair("version",
				jt::build_string(tauparser::full_version)) });
			break;
		}
		case p::license_cmd: {
			data = jt::build_object({ jt::build_pair("license",
				jt::build_string(tauparser::license)) });
			break;
		}
		case p::get_cmd:
			data = get_cmd(statement_option_name(s));
			break;
		case p::set_cmd:
			data = set_cmd(statement_option_name(s),
				statement_option_text(s));
			break;
		case p::toggle_cmd:
			data = flag_cmd(statement_option_name(s),
				[](bool b) { return !b; });
			break;
		case p::enable_cmd:
			data = flag_cmd(statement_option_name(s),
				[](bool) { return true; });
			break;
		case p::disable_cmd:
			data = flag_cmd(statement_option_name(s),
				[](bool) { return false; });
			break;
		case p::add_cmd:
			data = list_cmd(statement_option_name(s),
				statement_option_text(s), true);
			break;
		case p::del_cmd:
			data = list_cmd(statement_option_name(s),
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
			data = grammar_from_source
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
			data = reload_data(filename);
			break;
		}
		case p::start_cmd: {
			auto n = s | p::symbol;
			bool changed = false;
			if (n.has_value()) {
				auto written = idni::options().set_text("start",
					n | tt::terminals);
				changed = !written.has_error();
				report.append(written.report());
			}
			const string start = start_symbol();
			data = jt::build_object({
				jt::build_pair("start", start.empty()
					? jt::_null() : jt::build_string(start)),
				jt::build_pair("changed",
					jt::build_bool(changed)) });
			break;
		}
		case p::igrammar_cmd: {
			if (!require_grammar()) break;
			auto n = s | p::symbol;
			string start = n.has_value()
				? n | tt::terminals : start_symbol();
			data = productions_value("start", start,
				g().reachable_productions(g().nt(start)));
			break;
		}
		case p::grammar_cmd: {
			if (!require_grammar()) break;
			data = jt::build_object({
				jt::build_pair("file",
					jt::build_string(tgf_filename)),
				jt::build_pair("source",
					jt::build_string(grammar_source)) });
			break;
		}
		case p::unreachable_cmd: {
			if (!require_grammar()) break;
			auto n = s | p::symbol;
			string start = n.has_value()
				? n | tt::terminals : start_symbol();
			auto unreachable = g().unreachable_productions(
				g().nt(start));
			data = productions_value("symbol", start, unreachable);
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
			data = parse(input.c_str(), input.size());
			break;
		}
		case p::parse_file_cmd: {
			if (!paths_allowed()) break;
			auto n = s | p::filename;
			auto filename = unquote(n | tt::terminals, report);
			if (report.has_error()) break;
			data = parse(filename);
			break;
		}
		default:
			report.error(diagnostics::code::invalid_argument,
				parser_strings::messages::unknown_command);
			break;
		}
	}
	if (data) res.data = jt::geth(data);
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
	auto res = rp.parse(src.c_str(), src.size());
	if (!res.found) {
		if (opt.continue_on_eof && res.parse_error.at_eof()) {
			er.status = cmd_status::incomplete;
			res.report().demote_errors_to_warnings();
			er.report.append(std::move(res.report()));
			return er;
		}
		er.status = cmd_status::error;
		er.report.append(std::move(res.report()));
		return er;
	}
	if (!opt.json) {
		res.print_ambiguous_nodes(cout);
		if (idni::options().get<bool>("debug"))
			pretty_print(cout << "input command graph:\n",
				res.get_shaped_tree2(), {}, false, 1);
	}
	tref ref = res.get_shaped_tree2();
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

// A terminal in the quotes and escapes of the grammar text.
static std::string quoted_terminal(const std::string& text) {
	return "'" + escapes::encode(to_u32string(text), escapes::tgf_char)
		+ "'";
}

// The symbol of an AST JSON node, or its quoted terminal text.
static std::string node_label(tref node) {
	if (auto s = format::json::string_member(node, "symbol");
			s && !s->empty())
		return *s;
	if (auto t = format::json::string_member(node, "text"); t)
		return quoted_terminal(*t);
	return {};
}

// The range of an AST JSON node: "[<begin><sep><end>]".
static void print_range(std::ostream& os, const jtt& node, const char* sep) {
	os << "[";
	auto r = node | jtt::member("range");
	if (r.is(json_parser::arr) && r.value_tree().children_size() >= 2)
		os << static_cast<long long>(r | jtt::first | jtt::number)
			<< sep << static_cast<long long>(
				r | jtt::second | jtt::number);
	os << "]";
}

// One ambiguous node of a parse, from its data only.
static void render_data_ambiguous_node(std::ostream& os, const jtt& node) {
	os << "\t `" << node_label(node.value()) << "` ";
	print_range(os, node, ",");
	os << "\n";
	size_t d = 0;
	auto alts = node | jtt::member("alternatives");
	if (alts.is(json_parser::arr))
		for (const auto& alt : (alts | jtt::children)()) {
			os << "\t\t " << d++ << "\t";
			auto ch = alt | jtt::member("children");
			if (ch.is(json_parser::arr))
				for (const auto& c : (ch | jtt::children)()) {
					os << " `" << node_label(c.value()) << "`";
					print_range(os, c, ",");
					os << " ";
				}
			os << "\n";
		}
}

#ifdef _MSC_VER
// the color macros need the parameter TC, which hides the global idni::TC
#	pragma warning(push)
#	pragma warning(disable : 4459)
#endif
// An AST JSON tree in the shape of the text tree printer, @p l tabs deep
// at its root.
static void render_data_tree(std::ostream& os, tref root, size_t l,
	const term::colors& TC)
{
	// Only the children members are entered, so every object on the way
	// is an AST node.
	auto into = [](tref n) {
		const jt& t = jt::get(n);
		return !t.is(json_parser::object_pair)
			|| t.get_string() == "children";
	};
	auto enter = [&](tref n) {
		if (!jt::get(n).is_object()) return;
		const jtt node(n);
		for (size_t i = 0; i != l; ++i) os << "\t";
		++l;
		const std::string label = node_label(n);
		if (auto id = node | jtt::member("id");
				id.is(json_parser::number))
			os << TC_NT << label << TC.CLEAR() << TC_NT_ID << "("
				<< static_cast<long long>(id | jtt::number)
				<< ")" << TC.CLEAR();
		else os << TC_T << label << TC.CLEAR();
		os << TC_RANGE;
		print_range(os, node, ", ");
		os << TC.CLEAR() << "\n";
	};
	auto leave = [&l](tref n) { if (jt::get(n).is_object()) --l; };
	pre_order<format::json::node>(root).visit(enter, into, leave);
}

// The parse text from the response data.
static void render_data_parse(const cmd_result& r, std::ostream& os,
	const term::colors& TC)
{
	using format::json::string_member;
	const jtt data(r.data);
	const tref d = r.data->get();
	if (auto in = string_member(d, "input"); in)
		os << "input: \"" << *in << "\"\n";
	if (auto amb = data | jtt::member("ambiguous");
			amb.is(json_parser::object)) {
		double n = amb | jtt::member("trees") | jtt::number;
		// The text printer stays silent for an unambiguous parse.
		if (n > 1) {
			os << "# n trees: " << static_cast<long long>(n)
				<< "\n# ambiguous nodes:\n";
			if (auto nodes = amb | jtt::member("nodes");
					nodes.is(json_parser::arr))
				for (const auto& node : (nodes
						| jtt::children)())
					render_data_ambiguous_node(os, node);
		}
	}
	if (auto t = string_member(d, "terminals"); t)
		os << "parsed terminals: " << TC_T << *t
			<< TC_CLEARED_DEFAULT << "\n";
	if (auto tml = string_member(d, "tml_rules"); tml)
		os << *tml;
	if (auto tml = string_member(d, "tml_facts"); tml)
		os << *tml;
	if (auto tree = data | jtt::member("tree");
			tree.is(json_parser::object)) {
		os << "parsed graph:\n";
		render_data_tree(os, tree.value(), 1, TC);
	}
}

// The grammar source text from the response data.
static void render_data_grammar(const cmd_result& r, std::ostream& os) {
	print_grammar_source(os, jtt(r.data) | jtt::member("source")
		| jtt::string);
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
	const std::string& pstr, const jtt& pid,
	const term::colors& TC, const std::set<size_t>* char_classes)
{
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
	jtt body_ids;
	bool conjunctive = false;
	std::string guard;
	if (pid.is(json_parser::object)) {
		if (auto i = pid | jtt::member("index");
				i.is(json_parser::number))
			index = static_cast<size_t>(i | jtt::number);
		if (auto h = pid | jtt::member("head");
				h.is(json_parser::number)) {
			head_id = static_cast<long long>(h | jtt::number);
			have_head = true;
		}
		if (auto b = pid | jtt::member("body");
				b.is(json_parser::arr))
			body_ids = b;
		conjunctive = pid | jtt::member("conjunctive")
			| jtt::boolean;
		guard = pid | jtt::member("guard") | jtt::string;
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
	const trefs conjs = (body_ids | jtt::children | jtt::refs);
	const size_t nconj = conjs.size();
	if (!body_ids && !body.empty()) os << " " << body;
	for (size_t j = 0; j != nconj; ++j) {
		if (j != 0) {
			os << " &";
			if (t != tok.size() && tok[t] == "&") ++t;
		}
		bool neg = t != tok.size() && tok[t] == "~(";
		if (neg) { os << " " << TC_NEG << "~("; ++t; }
		const trefs lits = (jtt(conjs[j]) | jtt::children).values();
		for (size_t k = 0; k != lits.size() && t != tok.size(); ++k) {
			os << " ";
			const std::string& name = tok[t++];
			if (jt::get(lits[k]).is_number())
				os << nt_begin(static_cast<long long>(
					jt::get(lits[k]).get_number())) << name
					<< TC_DEFAULT << TC_NT_ID
					<< "(" << static_cast<long long>(
						jt::get(lits[k]).get_number())
					<< ")" << TC_DEFAULT;
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
	const jtt data(r.data);
	auto prods = data | jtt::member("productions");
	auto pids = data | jtt::member("production_ids");
	const bool unreachable = r.cmd == "unreachable";
	std::string s = data | jtt::member(unreachable ? "symbol" : "start")
		| jtt::string;
	const trefs prod_list = prods.is(json_parser::arr)
		? (prods | jtt::children | jtt::refs) : trefs{};
	const trefs pid_list = pids.is(json_parser::arr)
		? (pids | jtt::children | jtt::refs) : trefs{};
	if (unreachable) {
		bool any = !prod_list.empty();
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
	if (!prods.is(json_parser::arr)) return;
	for (size_t i = 0; i != prod_list.size(); ++i) {
		jtt pid;
		if (i < pid_list.size()) pid = jtt(pid_list[i]);
		os << "  ";
		render_data_production(os, jtt(prod_list[i]) | jtt::string,
			pid, TC, char_classes);
		os << "\n";
	}
}

void render_command_text(const cmd_result& r, std::ostream& os,
	const term::colors& TC, const std::set<size_t>* char_classes)
{
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
	const jtt data(r.data);
	if (r.cmd == "version") {
		if (auto v = data | jtt::member("version"); v)
			os << (v | jtt::string) << "\n";
		return;
	}
	if (r.cmd == "license") {
		if (auto v = data | jtt::member("license"); v)
			os << (v | jtt::string) << "\n";
		return;
	}
	if (r.cmd == "help") {
		if (auto v = data | jtt::member("text"); v)
			os << (v | jtt::string);
		return;
	}
	if (r.cmd == "start") {
		auto s = data | jtt::member("start");
		auto ch = data | jtt::member("changed");
		if (!s || !ch) return;
		if (ch | jtt::boolean) os << "start symbol set: " << TC_NT
			<< (s | jtt::string) << TC_DEFAULT << "\n";
		else os << "start symbol: " << TC_NT << (s | jtt::string)
			<< TC_DEFAULT << "\n";
		return;
	}
	if (r.cmd == "load" || r.cmd == "reload") {
		auto l = data | jtt::member("loaded");
		if (!l) return;   // fixed grammar: only the warning
		auto name = data | jtt::member("grammar") | jtt::string;
		if (l | jtt::boolean) os << "loaded: " << name << "\n";
		else os << "reload failed: " << name << "\n";
		return;
	}
	auto label = [](const std::string& name) {
		std::string l = name + ":";
		l.resize(std::max<size_t>(l.size(), 24), ' ');
		return l;
	};
	if (auto o = data | jtt::member("option"); o) {
		os << label(o | jtt::string)
			<< option_text(data | jtt::member("value")) << "\n";
		return;
	}
	if (auto opts = data | jtt::member("options"); opts) {
		for (tref kv : ((opts || json_parser::object_pair) | jtt::refs))
			os << label(jt::get(kv).get_string())
				<< option_text(jtt(kv) | jtt::only_child)
				<< "\n";
		return;
	}
}
#ifdef _MSC_VER
#	pragma warning(pop)
#endif

void tgf_repl_evaluator::render_text(const cmd_result& res,
	std::ostream& os) const
{
	// A character class nonterminal takes its own color.
	std::set<size_t> char_classes;
	if (res.cmd == "internal-grammar" || res.cmd == "unreachable") {
		auto cc = jtt(state_value(*this)) | jtt::member("char_classes");
		if (cc.is(json_parser::arr))
			for (tref n : (cc | jtt::children | jtt::refs))
				if (jt::get(n).is_number()) char_classes.insert(
					static_cast<size_t>(
						jt::get(n).get_number()));
	}
	render_command_text(res, os, TC, &char_classes);
}

tref statement_request(const tgf_repl_evaluator::trv& n) {
	using p = tgf_repl_parser;
	const auto nt = n | tt::nonterminal;
	trefs req{ jt::build_pair("cmd", jt::build_string(command_name(nt))) };
	auto add = [&req](const char* key, std::string_view v) {
		req.push_back(jt::build_pair(key, jt::build_string(v)));
	};
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
		add("input", input);
		break;
	}
	case p::parse_file_cmd:
	case p::load_cmd: {
		auto fn = n | p::filename;
		diagnostics::report rep;
		add("file", unquote(fn | tt::terminals, rep));
		break;
	}
	case p::start_cmd:
	case p::igrammar_cmd:
	case p::unreachable_cmd: {
		auto sym = n | p::symbol;
		if (sym.has_value()) add("symbol", sym | tt::terminals);
		break;
	}
	case p::help_cmd: {
		auto h = n | p::help_arg;
		if (h.has_value()) add("command", help_arg_name(
			h | tt::only_child | tt::nonterminal));
		break;
	}
	case p::get_cmd:
		if (auto name = statement_option_name(n); !name.empty())
			add("option", name);
		break;
	// The client knows no option kind, so the server reads the text.
	case p::set_cmd:
	case p::add_cmd:
	case p::del_cmd:
		add("option", statement_option_name(n));
		add("text", statement_option_text(n));
		break;
	case p::toggle_cmd:
	case p::enable_cmd:
	case p::disable_cmd:
		add("option", statement_option_name(n));
		break;
	default:
		// grammar, reload, version, license, quit and clear carry no
		// field beyond cmd.
		break;
	}
	return jt::build_object(req);
}

idni::diagnostics::result<int> tgf_repl_evaluator::eval(const string& src) {
	eval_result er;
	{
		bintree<format::json::node>::gc_pause pause;
		er = run(src, [this](cmd_result& r) {
			render_text(r, cout);
			report.append(std::move(r.report));
			flush_report();
		});
	}
	// The answers are printed, and only the handles in er hold data.
	jt::gc_if_grown();
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
	bintree<format::json::node>::gc_pause pause;
	const bool json = idni::options().get<bool>("json");
	auto fail = [&](const diagnostics::report& rep) {
		if (json) format::json::write_line(json_result_response(
			cmd_status::error,
			tgf_command_spec(tgf_command::gen).name,
			jt::build_object({}), state_value(re), rep), cout);
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
		trefs files;
		if (gr.has_value())
			for (const auto& f : gr.value())
				files.push_back(jt::build_string(f));
		tref data = jt::build_object({
			jt::build_pair("files", jt::build_array(files)) });
		cmd_status st = gr.has_value()
			? cmd_status::ok : cmd_status::error;
		format::json::write_line(json_result_response(st,
			tgf_command_spec(tgf_command::gen).name, data,
			state_value(re), rep), cout);
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
	bintree<format::json::node>::gc_pause pause;
	auto& opts = idni::options();
	const bool json = opts.get<bool>("json");
	if (!re.require_grammar()) {
		if (json) {
			format::json::write_line(json_result_response(
				cmd_status::error,
				tgf_command_spec(tgf_command::grammar).name,
				jt::build_object({}),
				state_value(re), re.take_report()), cout);
			return 1;
		}
		re.flush_report();
		return 1;
	}
	const string start = re.start_symbol();
	if (json) {
		tref v = re.productions_value("start", start,
			re.g().reachable_productions(re.g().nt(start)));
		if (opts.get<bool>("nullable")) {
			trefs nl;
			for (const auto& [head, p] :
					re.g().nullable_recursive_productions())
				nl.push_back(jt::build_object({
					jt::build_pair("symbol", jt::build_string(
						head.to_std_string())),
					jt::build_pair("id", jt::build_number(
						static_cast<double>(head.n()))),
					jt::build_pair("index", jt::build_number(
						static_cast<double>(p))),
					jt::build_pair("production",
						jt::build_string(
						re.production_string(p))) }));
			v = jt::set_member(v, "nullable", jt::build_array(nl));
		}
		format::json::write_line(json_result_response(cmd_status::ok,
			tgf_command_spec(tgf_command::grammar).name, v,
			state_value(re), re.take_report()), cout);
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
	auto is = [&args](tgf_command c) {
		return args.command == tgf_command_spec(c).name;
	};
	// A session child runs the JSON API for `tgf serve`, so it sets
	// no_server_paths itself and refuses a path a client sends.
	if (string sid = opts.get<string>("session"); !sid.empty())
		re.begin_server_session(sid);

	if (is(tgf_command::grammar)) return show_command(re);

	if (is(tgf_command::serve)) return serve_command(tool, re);

	if (is(tgf_command::connect)) return connect_command(args.inputs);

	if (is(tgf_command::gen)) return gen_command(tool, re);

	if (is(tgf_command::test)) {
		if (args.inputs.empty()) return command_line_error(
			"test command needs at least one file as an argument");
		int ret = run_tests(re.p(), args.inputs);
		re.flush_report();
		return ret;
	}

	if (is(tgf_command::repl)) {
		const bool json = opts.get<bool>("json");
		const string eval = opts.get<string>("evaluate");
		if (json) {
			if (eval.size()) {
				bintree<format::json::node>::gc_pause pause;
				auto er = re.run(eval);
				format::json::write_line(json_eval_response(
					jt::_null(), er, state_value(re)), cout);
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
		auto exit_code = [](const diagnostics::result<int>& run) {
			print_diagnostics_report(run.report(), false);
			return run.has_value() ? run.value() : 1;
		};
		auto run_legacy = [&] {
			repl<tgf_repl_evaluator> r(re, "tgf> ", { ".tgf_history" });
			re.set_repl(r);
			return exit_code(r.run());
		};
		auto run_default = [&] {
#ifdef TAU_PARSER_HAS_FTXUI
			repl_ftxui<tgf_repl_evaluator> rftx(
				re, "tgf> ", { ".tgf_history" });
			return exit_code(rftx.run()); // ctor sets re.r_ftx
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

	if (is(tgf_command::parse)) {
		bintree<format::json::node>::gc_pause pause;
		const bool json = opts.get<bool>("json");
		string infile = opts.get<string>("input");
		string inexp  = opts.get<string>("input-expression");
		if (infile.size() && inexp.size())
			return command_line_error("multiple inputs specified, "
				"use either --input or --input-expression, not "
				"both");
		if (json) {
			tref data = nullptr;
			if (infile == "-") data = re.parse(cin);
			else if (infile.size()) data = re.parse(infile);
			else data = re.parse(inexp.c_str(), inexp.size());
			if (opts.get<bool>("grammar")) {
				string start = re.start_symbol();
				auto prods = re.g().reachable_productions(
					re.g().nt(start));
				data = jt::set_member(data, "internal_grammar",
					re.productions_value("start", start, prods));
			}
			if (opts.get<bool>("measure"))
				data = jt::set_member(data, "bintree_totals",
					bintree_process_totals_value());
			auto rep = re.take_report();
			cmd_status st = rep.has_error()
				? cmd_status::error : cmd_status::ok;
			format::json::write_line(json_result_response(st,
				tgf_command_spec(tgf_command::parse).name, data,
				state_value(re), rep), cout);
			return st == cmd_status::error ? 1 : 0;
		}
		if (opts.get<bool>("grammar")) re.eval("i");
		cmd_result parse_result;
		parse_result.cmd = command_name(tgf_repl_parser::parse_cmd);
		tref data = nullptr;
		if (infile == "-") data = re.parse(cin);
		else if (infile.size()) data = re.parse(infile);
		else data = re.parse(inexp.c_str(), inexp.size());
		parse_result.data = jt::geth(data);
		re.render_text(parse_result, cout);
		// A failed parse leaves its error in the report, as in --json.
		auto rep = re.take_report();
		print_diagnostics_report(rep, false);
		if (opts.get<bool>("measure")) print_bintree_process_totals();
		return rep.has_error() || !re.has_grammar() ? 1 : 0;
	}
	return 0;
}

// Declare the options of the tool, the grammar, the parser and the
// evaluator, and put each option on its commands.
static diagnostics::result<void> declare_tgf_options(
	options_repository& repo)
{
	diagnostics::result<void> r;
	for (const auto* set : { &tgf_option_set, &grammar_option_set(),
		&parser_option_set(), &tgf_repl_option_set })
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
	auto env = opts.load_env("TGF_");
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
	if (args.command == tgf_command_spec(tgf_command::serve).name)
		re.make_grammar_source_backed();
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

int tgf_main(int argc, char** argv) {
#ifdef _WIN32
	return win_utf8_main(argc, argv, tgf_run);
#else
	return tgf_run(argc, argv);
#endif
}

#ifdef TAU_PARSER_HAS_FTXUI
template struct repl_ftxui<tgf_repl_evaluator>;
#endif

} // namespace idni
