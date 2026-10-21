// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "parser_strings.h"
#include "tgf_repl_evaluator_base.h"
#include "utility/escapes.h"
// last: a second include of the color macros undefines them
#include "parser_term_color_macros.h"

namespace idni {

using namespace std;
using tp = tgf_protocol;
using jt = format::json::tree;
using jtt = format::json::tt;

tgf_repl_evaluator_base::tgf_repl_evaluator_base(options o)
	: repl_evaluator_base(o, repl_history_policy{ ".tgf_history" }) {}

std::string tgf_repl_evaluator_base::prompt_text() const {
	bintree<format::json::node>::gc_pause pause;
	const tref st = state();
	const string* grammar = format::json::string_member(st, tp::grammar);
	const string* start = format::json::string_member(st, tp::start);
	stringstream ss;
	if (opt.status) {
		ss << TC_STATUS << "[ ";
		if (grammar)
			ss << TC_STATUS_FILE << "\"" << *grammar << "\""
				<< TC.CLEAR() << TC_STATUS << " "
				<< TC_STATUS_START << (start ? *start : string())
				<< TC.CLEAR() << TC_STATUS;
		else ss << parser_strings::messages::prompt_no_grammar;
		ss << " ]" << TC.CLEAR() << " ";
	}
	ss << TC_PROMPT << "tgf>" << TC.CLEAR() << " ";
	return ss.str();
}

string tgf_repl_evaluator_base::option_name(const trv& s) {
	auto n = s | tgf_repl_parser::option_name;
	return n.has_value() ? n | trv::terminals : string();
}

// The grammar trims the whitespace between the words of a value, so one
// space joins them again.
string tgf_repl_evaluator_base::option_text(const trv& s) {
	string text;
	for (const auto& w : (s | tgf_repl_parser::option_value
		|| tgf_repl_parser::option_word)())
			text += (text.empty() ? "" : " ")
				+ string(w | trv::terminals);
	return text;
}

string tgf_repl_evaluator_base::unquote(const string& s,
	diagnostics::report& rep) {
	if (s.size() < 2 || s[0] != '"' || s[s.size() - 1] != '"') return s;
	auto dec = escapes::decode(
		s.substr(1, s.size() - 2), escapes::tgf_string);
	if (!dec.has_value()) {
		rep.append(std::move(dec.report()));
		return {};
	}
	return std::move(dec).value();
}

// Text form of an option value: on/off, a number, a text, or a list
// joined with ", ".
static std::string option_value_text(const jtt& v) {
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

// A terminal in the quotes and escapes of the grammar text.
static std::string quoted_terminal(const std::string& text) {
	return "'" + escapes::encode(to_u32string(text), escapes::tgf_char)
		+ "'";
}

// The symbol of an AST JSON node, or its quoted terminal text.
static std::string node_label(tref node) {
	if (auto s = format::json::string_member(node, tp::symbol);
			s && !s->empty())
		return *s;
	if (auto t = format::json::string_member(node, tp::text); t)
		return quoted_terminal(*t);
	return {};
}

// The range of an AST JSON node: "[<begin><sep><end>]".
static void print_range(std::ostream& os, tref node, const char* sep) {
	os << "[";
	auto r = jtt(node) | jtt::member(tp::range);
	if (r.is(json_parser::arr) && r.value_tree().children_size() >= 2)
		os << static_cast<long long>(r | jtt::first | jtt::number)
			<< sep << static_cast<long long>(
				r | jtt::second | jtt::number);
	os << "]";
}

// One ambiguous node of a parse, from its data only.
static void render_data_ambiguous_node(std::ostream& os, tref node) {
	os << "\t `" << node_label(node) << "` ";
	print_range(os, node, ",");
	os << "\n";
	size_t d = 0;
	auto alts = jtt(node) | jtt::member(tp::alternatives);
	if (alts.is(json_parser::arr))
		for (const auto& alt : (alts | jtt::children)()) {
			os << "\t\t " << d++ << "\t";
			auto ch = alt | jtt::member(tp::children);
			if (ch.is(json_parser::arr))
				for (tref c : (ch | jtt::children | jtt::refs)) {
					os << " `" << node_label(c) << "`";
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
			|| t.get_string() == tp::children;
	};
	auto enter = [&](tref node) {
		if (!jt::get(node).is_object()) return;
		for (size_t i = 0; i != l; ++i) os << "\t";
		++l;
		const std::string label = node_label(node);
		if (auto id = jtt(node) | jtt::member(tp::id);
				id.is(json_parser::number))
			os << TC_NT << label << TC.CLEAR() << TC_NT_ID << "("
				<< static_cast<long long>(id | jtt::number)
				<< ")" << TC.CLEAR();
		else os << TC_T << label << TC.CLEAR();
		os << TC_RANGE;
		print_range(os, node, ", ");
		os << TC.CLEAR() << "\n";
	};
	auto leave = [&l](tref node) { if (jt::get(node).is_object()) --l; };
	pre_order<format::json::node>(root).visit(enter, into, leave);
}

// The parse text from the response data.
static void render_data_parse(const cmd_result& r, std::ostream& os,
	const term::colors& TC)
{
	using format::json::string_member;
	const jtt data(r.data);
	const tref d = r.data->get();
	if (auto in = string_member(d, tp::input); in)
		os << "input: \"" << *in << "\"\n";
	if (auto amb = data | jtt::member(tp::ambiguous);
			amb.is(json_parser::object)) {
		double n = amb | jtt::member(tp::trees) | jtt::number;
		// The text printer stays silent for an unambiguous parse.
		if (n > 1) {
			os << "# n trees: " << static_cast<long long>(n)
				<< "\n# ambiguous nodes:\n";
			if (auto nodes = amb | jtt::member(tp::nodes);
					nodes.is(json_parser::arr))
				for (tref node : nodes | jtt::children
						| jtt::refs)
					render_data_ambiguous_node(os, node);
		}
	}
	if (auto t = string_member(d, tp::terminals); t)
		os << "parsed terminals: " << TC_T << *t
			<< TC_CLEARED_DEFAULT << "\n";
	if (auto tml = string_member(d, tp::tml_rules); tml)
		os << *tml;
	if (auto tml = string_member(d, tp::tml_facts); tml)
		os << *tml;
	if (auto tree = data | jtt::member(tp::tree);
			tree.is(json_parser::object)) {
		os << "parsed graph:\n";
		render_data_tree(os, tree.value(), 1, TC);
	}
}

// The grammar source text from the response data.
static void render_data_grammar(const cmd_result& r, std::ostream& os) {
	print_grammar_source(os, jtt(r.data) | jtt::member(tp::source)
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
		if (auto i = pid | jtt::member(tp::index);
				i.is(json_parser::number))
			index = static_cast<size_t>(i | jtt::number);
		if (auto h = pid | jtt::member(tp::head);
				h.is(json_parser::number)) {
			head_id = static_cast<long long>(h | jtt::number);
			have_head = true;
		}
		if (auto b = pid | jtt::member(tp::body);
				b.is(json_parser::arr))
			body_ids = b;
		conjunctive = pid | jtt::member(tp::conjunctive)
			| jtt::boolean;
		guard = pid | jtt::member(tp::guard) | jtt::string;
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
	auto prods = data | jtt::member(tp::productions);
	auto pids = data | jtt::member(tp::production_ids);
	const bool unreachable = r.cmd == "unreachable";
	std::string s = data | jtt::member(unreachable ? tp::symbol : tp::start)
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
		if (auto v = data | jtt::member(tp::version); v)
			os << (v | jtt::string) << "\n";
		return;
	}
	if (r.cmd == "license") {
		if (auto v = data | jtt::member(tp::license); v)
			os << (v | jtt::string) << "\n";
		return;
	}
	if (r.cmd == "help") {
		if (auto v = data | jtt::member(tp::text); v)
			os << (v | jtt::string);
		return;
	}
	if (r.cmd == "start") {
		auto s = data | jtt::member(tp::start);
		auto ch = data | jtt::member(tp::changed);
		if (!s || !ch) return;
		if (ch | jtt::boolean) os << "start symbol set: " << TC_NT
			<< (s | jtt::string) << TC_DEFAULT << "\n";
		else os << "start symbol: " << TC_NT << (s | jtt::string)
			<< TC_DEFAULT << "\n";
		return;
	}
	if (r.cmd == "load" || r.cmd == "reload") {
		auto l = data | jtt::member(tp::loaded);
		if (!l) return;   // fixed grammar: only the warning
		auto name = data | jtt::member(tp::grammar) | jtt::string;
		if (l | jtt::boolean) os << "loaded: " << name << "\n";
		else os << "reload failed: " << name << "\n";
		return;
	}
	auto label = [](const std::string& name) {
		std::string l = name + ":";
		l.resize(std::max<size_t>(l.size(), 24), ' ');
		return l;
	};
	if (auto o = data | jtt::member(tp::option); o) {
		os << label(o | jtt::string)
			<< option_value_text(data | jtt::member(tp::value))
			<< "\n";
		return;
	}
	if (auto opts = data | jtt::member(tp::options); opts) {
		for (tref kv : ((opts || json_parser::object_pair) | jtt::refs))
			os << label(jt::get(kv).get_string())
				<< option_value_text(jtt(kv) | jtt::only_child)
				<< "\n";
		return;
	}
}

void tgf_repl_evaluator_base::render_text(const cmd_result& res,
	std::ostream& os) const
{
	// A character class nonterminal takes its own color.
	std::set<size_t> char_classes;
	if (res.cmd == "internal-grammar" || res.cmd == "unreachable") {
		auto cc = jtt(state()) | jtt::member(tp::char_classes);
		if (cc.is(json_parser::arr))
			for (tref n : (cc | jtt::children | jtt::refs))
				if (jt::get(n).is_number()) char_classes.insert(
					static_cast<size_t>(
						jt::get(n).get_number()));
	}
	render_command_text(res, os, TC, &char_classes);
}
#ifdef _MSC_VER
#	pragma warning(pop)
#endif

} // namespace idni
