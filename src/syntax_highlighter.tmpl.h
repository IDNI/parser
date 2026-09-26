// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__SYNTAX_HIGHLIGHTER_TMPL_H__
#define __IDNI__PARSER__SYNTAX_HIGHLIGHTER_TMPL_H__

#include <sstream>
#include <cstring>
#include <cctype>
#include <algorithm>

#include "grammar_inspector.h"
#include "utility/charclasses.h"

namespace idni {

// --- token_classifier ---

inline const std::vector<std::string>& token_classifier::token_type_names() {
	static const std::vector<std::string> names(
		highlight_token_types.begin(), highlight_token_types.end());
	return names;
}

inline const std::vector<std::string>&
	token_classifier::token_modifier_names()
{
	static const std::vector<std::string> names = {
		"declaration",
		"readonly",
		"defaultLibrary",
	};
	return names;
}

inline uint32_t token_classifier::type_index(const std::string& name) {
	const auto& names = token_type_names();
	for (size_t i = 0; i < names.size(); ++i)
		if (names[i] == name) return static_cast<uint32_t>(i);
	return no_type;
}

inline uint32_t token_classifier::default_type() {
	// Neutral default: unclassified nonterminals map to "variable".
	return type_index("variable");
}

inline uint32_t token_classifier::classify(size_t nt_id) const {
	auto it = nt_to_type_.find(nt_id);
	return it != nt_to_type_.end() ? it->second : no_type;
}

inline bool token_classifier::is_char_class_nt(size_t nt_id) const {
	return char_class_nts_.count(nt_id) != 0;
}

inline bool token_classifier::is_transparent_nt(size_t nt_id) const {
	return transparent_nts_.count(nt_id) != 0;
}

// Anchors the first and last literal segments of pat unless they border a '*'.
inline bool token_classifier::glob_match(const std::string& pat,
	const std::string& name)
{
	if (pat.find('*') == std::string::npos) return pat == name;
	std::vector<std::string> segs;
	size_t start = 0;
	for (size_t i = 0; i <= pat.size(); ++i) {
		if (i == pat.size() || pat[i] == '*') {
			segs.push_back(pat.substr(start, i - start));
			start = i + 1;
		}
	}
	bool starts_star = pat.front() == '*';
	bool ends_star   = pat.back() == '*';
	size_t pos = 0;
	for (size_t i = 0; i < segs.size(); ++i) {
		const std::string& seg = segs[i];
		if (seg.empty()) continue;
		bool is_first = i == 0, is_last = i + 1 == segs.size();
		if (is_first && !starts_star) {
			if (name.compare(0, seg.size(), seg) != 0) return false;
			pos = seg.size();
		} else if (is_last && !ends_star) {
			if (name.size() < seg.size()
				|| name.size() - seg.size() < pos)
				return false;
			if (name.compare(name.size() - seg.size(),
				seg.size(), seg) != 0) return false;
		} else {
			size_t found = name.find(seg, pos);
			if (found == std::string::npos) return false;
			pos = found + seg.size();
		}
	}
	return true;
}

inline bool token_classifier::produces_digit_class(
	const grammar<char, char>& g, size_t nt_id,
	std::set<size_t>& visited) const
{
	if (!visited.insert(nt_id).second) return false;
	grammar_inspector<char, char> gi(g);
	const auto& prods = gi.G();
	for (const auto& pr : prods) {
		if (!pr.first.nt() || pr.first.n() != nt_id) continue;
		for (const auto& c : pr.second) {
			if (c.neg) continue;
			for (const auto& l : c) {
				if (!l.nt()) continue;
				if (digit_class_nts_.count(l.n())) return true;
				// Only descend through transparent EBNF helpers, never real rules.
				if (transparent_nts_.count(l.n())
					&& produces_digit_class(g, l.n(), visited))
					return true;
			}
		}
	}
	return false;
}

inline char token_classifier::boundary_quote(
	const grammar<char, char>& g, const lit<char, char>& l) const
{
	if (!l.nt()) {
		if (l.is_null()) return 0;
		char c = static_cast<char>(l.t());
		return (c == '"' || c == '\'') ? c : 0;
	}
	// A nonterminal is a quote boundary only if every production is a
	// single quote terminal.
	grammar_inspector<char, char> gi(g);
	const auto& prods = gi.G();
	char found = 0;
	bool any = false;
	for (const auto& pr : prods) {
		if (!pr.first.nt() || pr.first.n() != l.n()) continue;
		if (pr.second.size() != 1 || pr.second[0].neg) return 0;
		const auto& seq = pr.second[0];
		if (seq.size() != 1) return 0;
		const auto& t = seq[0];
		if (t.nt() || t.is_null()) return 0;
		char c = static_cast<char>(t.t());
		if (c != '"' && c != '\'') return 0;
		if (any && c != found) return 0;
		found = c;
		any = true;
	}
	return any ? found : 0;
}

// Classifies a nonterminal by its productions: quoted bodies, literal
// words or punctuation, digit classes.
inline uint32_t token_classifier::classify_by_content(
	const grammar<char, char>& g, size_t nt_id) const
{
	grammar_inspector<char, char> gi(g);
	const auto& prods = gi.G();

	bool has_alt = false;
	bool string_wrapped = false;
	bool all_alts_literal = true;   // every alternative is a pure literal
	bool any_literal_alt = false;
	// type suggested by the first pure-literal alternative that suggests one
	uint32_t first_lit_type = no_type;

	for (const auto& pr : prods) {
		if (!pr.first.nt() || pr.first.n() != nt_id) continue;
		has_alt = true;
		const auto& body = pr.second;   // vector<lits>, one per conjunct

		// literal / quote detection on a single positive conjunct
		if (body.size() == 1 && !body[0].neg) {
			const auto& seq = body[0];
			// A quote-delimited body's boundaries may be terminals or quote-only nts.
			if (seq.size() >= 2) {
				char fc = boundary_quote(g, seq.front());
				char bc = boundary_quote(g, seq.back());
				if (fc && fc == bc) string_wrapped = true;
			}
			bool pure = !seq.empty();
			std::string chars;
			for (const auto& l : seq) {
				if (l.nt() || l.is_null()) { pure = false; break; }
				chars += static_cast<char>(l.t());
			}
			if (pure) {
				any_literal_alt = true;
				bool alt_all_punct = true, alt_all_word = true,
					alt_all_quote = true, alt_any_alpha = false;
				for (char c : chars) {
					const unsigned char ch =
						static_cast<unsigned char>(c);
					bool word = std::isalnum(ch) || ch == '_';
					if (!word) alt_all_word = false;
					else alt_all_punct = false;
					if (std::isalpha(ch)) alt_any_alpha = true;
					if (ch != '"' && ch != '\'')
						alt_all_quote = false;
				}
				uint32_t alt_type = no_type;
				if (alt_all_quote) alt_type = type_index("string");
				else if (alt_all_punct) alt_type = type_index("operator");
				else if (alt_any_alpha && alt_all_word
					&& chars.size() >= 2)
					alt_type = type_index("keyword");
				if (first_lit_type == no_type)
					first_lit_type = alt_type;
			} else all_alts_literal = false;
		} else all_alts_literal = false;
	}

	if (!has_alt) return no_type;

	if (string_wrapped) return type_index("string");

	if (all_alts_literal && any_literal_alt && first_lit_type != no_type)
		return first_lit_type;

	std::set<size_t> visited;
	if (produces_digit_class(g, nt_id, visited))
		return type_index("number");

	return no_type;
}

// Matches a nonterminal name against known heuristic patterns, or no_type.
inline uint32_t token_classifier::classify_by_name(
	const std::string& name) const
{
	// Suffix matches (checked first -- more specific).
	struct { const char* suffix; const char* type; } suffixes[] = {
		{"_kw",      "keyword"},
		{"keyword",  "keyword"},
		{"_op",      "operator"},
		{"operator", "operator"},
		{"_comment", "comment"},
		{"comment",  "comment"},
		{"_string",  "string"},
		{"_str",     "string"},
		{"string",   "string"},
		{"_num",     "number"},
		{"number",   "number"},
		{"integer",  "number"},
		{"decimal",  "number"},
		{"_digit",   "number"},
		{"digits",   "number"},
		{"_int",     "number"},
		{"_type",    "type"},
		{"type",     "type"},
		{"_char",    "string"},
		{"_literal", "keyword"},
		{"_const",   "keyword"},
		{"_sym",     "keyword"},
		{"_dir",     "keyword"},
		{"_cmd",     "keyword"},
		{"_delim",   "keyword"},  // delimiter-like but not operator
		{"_sep",     "keyword"},
		{"_paren",   "keyword"},
	};
	for (auto& s : suffixes) {
		size_t slen = std::strlen(s.suffix);
		if (name.size() >= slen && name.compare(
			name.size() - slen, slen, s.suffix) == 0)
			return type_index(s.type);
	}

	// Prefix matches.
	struct { const char* prefix; const char* type; } prefixes[] = {
		{"kw_",       "keyword"},
		{"keyword_",  "keyword"},
		{"op_",       "operator"},
		{"operator_", "operator"},
		{"comment_",  "comment"},
		{"string_",   "string"},
		{"str_",      "string"},
		{"num_",      "number"},
		{"int_",      "number"},
		{"digit_",    "number"},
		{"type_",     "type"},
	};
	for (auto& p : prefixes) {
		size_t plen = std::strlen(p.prefix);
		if (name.size() >= plen && name.compare(0, plen, p.prefix) == 0)
			return type_index(p.type);
	}

	// Exact matches.
	struct { const char* exact; const char* type; } exacts[] = {
		{"str",        "string"},
		{"q_str",      "string"},
		{"identifier", "variable"},
		{"name",       "variable"},
		{"sym",        "variable"},
		{"symbol",     "variable"},
		{"variable",   "variable"},
		{"var",        "variable"},
		{"id",         "variable"},
		{"ident",      "variable"},
		{"null",       "keyword"},
		// Common structural nonterminal names that carry semantic type meaning.
		{"object",     "type"},
		{"array",      "type"},
		{"tuple",      "type"},
		{"struct",     "type"},
		{"enum",       "type"},
		{"union",      "type"},
		{"class",      "type"},
		{"interface",  "type"},
		{"record",     "type"},
		{"message",    "type"},
		{"field",      "variable"},
		{"member",     "variable"},
		{"property",   "variable"},
		{"attribute",  "variable"},
		{"key",        "variable"},
		{"value",      "variable"},
		{"pair",       "variable"},
		{"entry",      "variable"},
		{"item",       "variable"},
		{"element",    "variable"},
		{"expr",       "variable"},
		{"term",       "variable"},
		{"factor",     "variable"},
		{"statement",  "variable"},
		{"block",      "variable"},
		{"body",       "variable"},
		{"start",      "keyword"},
		{"main",       "keyword"},
		{"program",    "keyword"},
	};
	for (auto& e : exacts) {
		if (name == e.exact) return type_index(e.type);
	}

	return no_type;
}

inline void token_classifier::init(const grammar<char, char>& g,
	const nonterminals<char, char>& nts, bool heuristics,
	std::string& diagnostics)
{
	nt_to_type_.clear();
	char_class_nts_.clear();
	digit_class_nts_.clear();
	transparent_nts_.clear();
	patterns_.clear();

	static const std::set<std::string> cc_names = {
		"alnum", "alpha", "blank", "cntrl", "digit", "graph",
		"lower", "printable", "punct", "space", "upper", "xdigit",
		"eof", "any", "ascii"
	};
	static const std::set<std::string> digit_cc_names = {
		"digit", "xdigit"
	};

	// Pass 1, structure, always: membership only, no types assigned.
	for (size_t i = 0; i < nts.size(); ++i) {
		std::string name = nts.get(i);
		if (cc_names.count(name)) char_class_nts_.insert(i);
		if (digit_cc_names.count(name)) digit_class_nts_.insert(i);
		if (name.size() >= 4 && name[0] == '_' && name[1] == '_'
			&& (name[2] == 'E' || name[2] == 'B'
				|| name[2] == 'N')
			&& name[3] == '_')
			transparent_nts_.insert(i);
	}

	// Pass 2, heuristics, only when enabled.
	if (heuristics) {
		for (size_t i = 0; i < nts.size(); ++i) {
			std::string name = nts.get(i);
			if (name.empty()) continue;
			if (char_class_nts_.count(i)) {
				nt_to_type_[i] = digit_class_nts_.count(i)
					? type_index("number") : default_type();
				continue;
			}
			uint32_t ty = classify_by_name(name);
			if (ty == no_type) ty = classify_by_content(g, i);
			nt_to_type_[i] = (ty == no_type) ? default_type() : ty;
		}
	}

	// Pass 3, @highlight, always, last: author intent beats heuristics.
	auto is_name_glob = [](const std::string& s) {
		if (s.empty()) return false;
		if (!(std::isalpha((unsigned char)s[0])
			|| s[0] == '_' || s[0] == '*'))
			return false;
		for (char c : s)
			if (!(std::isalnum((unsigned char)c)
				|| c == '_' || c == '*'))
				return false;
		return true;
	};
	for (auto& hl : g.opt.highlights) {
		uint32_t ti = type_index(hl.first);
		if (ti == no_type) continue;
		for (auto& pat : hl.second) {
			if (is_name_glob(pat)) {
				for (size_t i = 0; i < nts.size(); ++i) {
					std::string nm = nts.get(i);
					if (nm.empty()) continue;
					if (glob_match(pat, nm))
						nt_to_type_[i] = ti;
				}
				continue;
			}
			auto m = treemr::matcher_for<char, char>(nts, pat);
			if (!m.has_value()) {
				diagnostics += "invalid treemr pattern: "
					+ pat + "\n";
				continue;
			}
			patterns_.emplace_back(
				std::move(m).value(), ti);
		}
	}
}

inline void token_classifier::apply_patterns(tref root,
	std::unordered_map<tref, uint32_t>& overrides) const
{
	if (!root) return;
	for (auto& pm : patterns_) {
		auto matches = pm.first.search_all(root);
		for (auto& m : matches) {
			if (m.captures.empty()) {
				overrides[m.root] = pm.second;
				continue;
			}
			for (size_t i = 1; i <= m.captures.size(); ++i)
				for (tref n : m.capture_nodes<
					pnode_type<char, char>>(i))
					overrides[n] = pm.second;
		}
	}
}

// --- syntax_highlighter ---

inline syntax_highlighter::syntax_highlighter(const std::string& grammar_src,
	std::optional<bool> heuristics)
{
	nts_ = std::make_unique<nonterminals<char, char>>();

	auto gr = tgf<char, char>::from_string(*nts_, grammar_src);
	std::ostringstream ss;
	gr.report().print(ss);
	diagnostics_ = ss.str();
	if (!gr.has_value()) return;
	g_ = std::make_unique<grammar<char, char>>(std::move(gr).value());
	p_ = std::make_unique<parser<char, char>>(*g_,
		default_parser_options<char, char>());

	heuristics_ = heuristics.has_value()
		? *heuristics : g_->opt.highlight_heuristics;
	classifier_.init(*g_, *nts_, heuristics_, diagnostics_);

	// Highlighting shaping disables all trimming and all user inlining.
	highlight_shaping_ = g_->opt.shaping;
	highlight_shaping_.trim_terminals = false;
	highlight_shaping_.to_trim.clear();
	highlight_shaping_.to_trim_children.clear();
	highlight_shaping_.to_trim_children_terminals.clear();
	highlight_shaping_.dont_trim_terminals_of.clear();
	highlight_shaping_.to_inline.clear();

	good_ = true;
}

inline size_t syntax_highlighter::nt_count() const {
	return nts_ ? nts_->size() : 0;
}

inline std::string syntax_highlighter::get_nt_name(size_t id) const {
	if (!nts_ || id >= nts_->size()) return "";
	return nts_->get(id);
}

inline std::string syntax_highlighter::get_nt_type(size_t id) const {
	if (!nts_ || id >= nts_->size()) return "";
	uint32_t ty = classifier_.classify(id);
	const auto& names = token_classifier::token_type_names();
	if (ty == token_classifier::no_type || ty >= names.size()) return "";
	return names[ty];
}

inline void syntax_highlighter::build_line_offsets(const std::string& src,
	std::vector<size_t>& offsets) const
{
	offsets.clear();
	offsets.push_back(0);
	for (size_t i = 0; i < src.size(); ++i)
		if (src[i] == '\n') offsets.push_back(i + 1);
}

inline bool syntax_highlighter::is_delimiter_char(char ch) {
	return ch != 0 && std::strchr(".,;:()[]{}", ch) != nullptr;
}

inline uint32_t syntax_highlighter::utf16_units(const std::string& src,
	size_t a, size_t b) const
{
	uint32_t units = 0;
	size_t i = a, n = src.size();
	if (b > n) b = n;
	while (i < b) {
		unsigned char c = static_cast<unsigned char>(src[i]);
		size_t adv; uint32_t u;
		if (c < 0x80)            { adv = 1; u = 1; } // ASCII
		else if ((c >> 5) == 0x6){ adv = 2; u = 1; } // 2-byte BMP
		else if ((c >> 4) == 0xE){ adv = 3; u = 1; } // 3-byte BMP
		else if ((c >> 3) == 0x1E){ adv = 4; u = 2; } // 4-byte: surrogate pair
		else                     { adv = 1; u = 1; } // stray byte
		if (i + adv > b) { units += static_cast<uint32_t>(b - i); break; }
		i += adv;
		units += u;
	}
	return units;
}

inline void syntax_highlighter::flush_run(uint32_t type,
	size_t start, size_t end,
	const std::string& src,
	const std::vector<size_t>& line_offsets,
	std::vector<extracted_token>& out) const
{
	if (start >= end) return;
	if (type == token_classifier::no_type) return;

	// Find line by binary search on the byte line-offset table.
	size_t lo = 0, hi = line_offsets.size();
	while (lo + 1 < hi) {
		size_t mid = (lo + hi) / 2;
		if (line_offsets[mid] <= start) lo = mid;
		else hi = mid;
	}
	size_t line = lo;

	// A token never spans a line: split at every line end and drop it,
	// "\n" or "\r\n".
	size_t pos = start;
	while (pos < end) {
		size_t line_end = src.size();
		if (line + 1 < line_offsets.size()) {
			line_end = line_offsets[line + 1] - 1;
			if (line_end > line_offsets[line]
				&& src[line_end - 1] == '\r')
				--line_end;
		}
		size_t seg_end = std::min(end, line_end);
		if (seg_end > pos) {
			uint32_t col = utf16_units(src, line_offsets[line], pos);
			uint32_t len = utf16_units(src, pos, seg_end);
			out.push_back({static_cast<uint32_t>(line), col, len,
				type, 0});
		}
		if (seg_end >= end) break;
		pos = line_offsets[line + 1]; // skip the line end, "\n" or "\r\n"
		++line;
	}
}

inline void syntax_highlighter::extract_tokens(tref root,
	const std::string& src,
	std::vector<extracted_token>& out,
	const std::unordered_map<tref, uint32_t>& overrides) const
{
	using tree = parser<char, char>::tree;

	if (!root) return;

	std::vector<size_t> line_offsets;
	build_line_offsets(src, line_offsets);

	// Non-skipped ancestors with their type; the top types the next leaf.
	struct ctx_entry { tref node; uint32_t type; };
	std::vector<ctx_entry> ctx;
	static constexpr size_t no_parent = static_cast<size_t>(-1);

	// Run-merging state: adjacent same-type terminals under the same parent join.
	struct run_state { uint32_t type; size_t start; size_t end;
		size_t parent_nt; bool active = false; } cur;

	// Strings and comments are opaque: every terminal inside one takes that type.
	const uint32_t string_idx  = token_classifier::type_index("string");
	const uint32_t comment_idx = token_classifier::type_index("comment");
	int opaque_string = 0, opaque_comment = 0;
	auto eff_type = [&]() -> uint32_t {
		if (opaque_string  > 0) return string_idx;
		if (opaque_comment > 0) return comment_idx;
		if (ctx.empty()) return token_classifier::no_type;
		return ctx.back().type;
	};
	auto parent_nt = [&]() -> size_t {
		return ctx.empty() ? no_parent
			: tree::get(ctx.back().node).value.first.n();
	};

	auto flush = [&]() {
		if (!cur.active) return;
		flush_run(cur.type, cur.start, cur.end,
			src, line_offsets, out);
		cur.active = false;
	};

	// Extend the current run if it continues one, else flush it and
	// start a new one.
	auto merge_or_start = [&](uint32_t ty, size_t s, size_t e, size_t pnt) {
		if (cur.active && cur.type == ty
			&& cur.parent_nt == pnt && cur.end == s)
			cur.end = e;
		else {
			flush();
			cur = {ty, s, e, pnt, true};
		}
	};

	// An unassigned character: bracket/separator punctuation is a
	// delimiter, else an operator.
	auto infer_from_char = [](char ch) -> uint32_t {
		if (is_delimiter_char(ch))
			return token_classifier::type_index("delimiter");
		if (!charclasses::isspace<char>(ch)
			&& !std::isalnum(static_cast<unsigned char>(ch))
			&& ch != '_')
			return token_classifier::type_index("operator");
		return token_classifier::no_type;
	};

	auto enter = [&](tref node, tref /*parent*/) -> bool {
		const auto& n = tree::get(node);
		if (n.is_nt()) {
			size_t nt = n.value.first.n();
			auto oit = overrides.find(node);
			bool overridden = oit != overrides.end();
			uint32_t ty = overridden ? oit->second
				: classifier_.classify(nt);
			// An override must reach the leaves, even under a transparent node.
			bool skipped = !overridden
				&& classifier_.is_skipped_parent(nt);
			if (!skipped) {
				ctx.push_back({node, ty});
				if (ty == string_idx)  ++opaque_string;
				else if (ty == comment_idx) ++opaque_comment;
			}
			// A childless nonterminal (e.g. a char class match)
			// emits a token for its own span.
			if (n.children_size() == 0) {
				size_t s = n.value.second[0];
				size_t e = n.value.second[1];
				if (s < e) {
					size_t pnt = parent_nt();
					uint32_t t = eff_type();
					if (t == token_classifier::no_type
						&& heuristics_)
						t = infer_from_char(src[s]);
					merge_or_start(t, s, e, pnt);
				}
			}
			return true;
		}

		// Terminal leaf.
		size_t s = n.value.second[0];
		size_t e = n.value.second[1];
		if (s >= e) return true; // skip null/epsilon

		// Whitespace is not a token, except as content inside a string/comment.
		{
			char ch = static_cast<char>(
				n.value.first.t());
			if (charclasses::isspace<char>(ch)
				&& opaque_string == 0 && opaque_comment == 0)
				return true;
		}

		size_t pnt = parent_nt();
		uint32_t ty = token_classifier::no_type;
		auto oit = overrides.find(node);
		if (oit != overrides.end()) ty = oit->second;
		else ty = eff_type();

		// With heuristics on and no ancestor type, infer from the character itself.
		if (ty == token_classifier::no_type && heuristics_)
			ty = infer_from_char(
				static_cast<char>(n.value.first.t()));

		merge_or_start(ty, s, e, pnt);
		return true;
	};

	auto leave = [&](tref node, tref) {
		const auto& n = tree::get(node);
		if (n.is_nt()) {
			size_t nt = n.value.first.n();
			auto oit = overrides.find(node);
			bool overridden = oit != overrides.end();
			uint32_t ty = overridden ? oit->second
				: classifier_.classify(nt);
			bool skipped = !overridden
				&& classifier_.is_skipped_parent(nt);
			// Adjacent same-type siblings still merge across this boundary in enter().
			if (!skipped) {
				if (!ctx.empty()) ctx.pop_back();
				if (ty == string_idx && opaque_string > 0)
					--opaque_string;
				else if (ty == comment_idx
					&& opaque_comment > 0)
					--opaque_comment;
			}
		}
	};

	auto all_pass = [](tref) { return true; };
	auto noop     = [](tref, tref) {};

	pre_order<pnode_type<char, char>>(root).visit(
		enter, all_pass, leave, noop);
	flush();
}

inline void syntax_highlighter::fallback_tokens(const std::string& src,
	size_t offset,
	const std::vector<size_t>& line_offsets,
	std::vector<extracted_token>& out) const
{
	size_t i = offset;
	size_t n = src.size();

	while (i < n) {
		unsigned char c = static_cast<unsigned char>(src[i]);

		// Whitespace.
		if (charclasses::isspace<char>(static_cast<char>(c))) {
			++i;
			continue;
		}

		// Quote -> string: span to the matching quote, or to end of
		// line if unterminated.
		if (c == '"' || c == '\'') {
			char q = static_cast<char>(c);
			size_t start = i;
			++i;
			while (i < n && src[i] != q && src[i] != '\n') {
				if (src[i] == '\\' && i + 1 < n
					&& src[i + 1] != '\n') ++i;
				++i;
			}
			if (i < n && src[i] == q) ++i; // closing quote
			flush_run(token_classifier::type_index("string"),
				start, i, src, line_offsets, out);
			continue;
		}

		// Digits -> number.
		if (std::isdigit(c)) {
			size_t start = i;
			while (i < n && std::isdigit(
				static_cast<unsigned char>(src[i]))) ++i;
			flush_run(token_classifier::type_index("number"), start, i,
				src, line_offsets, out);
			continue;
		}

		// Alpha or underscore -> variable (the neutral default).
		if (std::isalpha(c) || c == '_') {
			size_t start = i;
			while (i < n && (std::isalnum(
				static_cast<unsigned char>(src[i]))
				|| src[i] == '_')) ++i;
			flush_run(token_classifier::default_type(), start, i,
				src, line_offsets, out);
			continue;
		}

		// Punctuation: separators and brackets -> delimiter, the rest -> operator.
		size_t start = i;
		char pc = static_cast<char>(c);
		++i;
		flush_run(token_classifier::type_index(
			is_delimiter_char(pc) ? "delimiter" : "operator"),
			start, i, src, line_offsets, out);
	}
}

inline std::vector<uint32_t> syntax_highlighter::get_tokens(
	const std::string& src)
{
	std::vector<extracted_token> tokens;

	if (!good_ || src.empty()) return {};

	// Merge adjacent same-type, same-modifiers tokens on the same line
	// with no gap.
	auto merge_pass = [](std::vector<extracted_token>& toks) {
		if (toks.empty()) return;
		std::vector<extracted_token> merged;
		merged.push_back(toks[0]);
		for (size_t i = 1; i < toks.size(); ++i) {
			auto& last = merged.back();
			auto& cur = toks[i];
			if (last.token_type == cur.token_type
				&& last.modifiers == cur.modifiers
				&& last.line == cur.line
				&& last.col + last.length == cur.col)
			{
				last.length += cur.length;
			} else {
				merged.push_back(cur);
			}
		}
		toks = std::move(merged);
	};

	auto result = p_->parse(src.c_str(), src.size());

	if (!result.found) {
		// Only advance the fallback start past a prefix that actually parsed.
		const size_t err_loc = result.parse_error.loc;
		size_t suffix_start = 0;
		// An error at the end of the input would just repeat the failed parse.
		if (err_loc > 0 && err_loc < src.size()) {
			auto prefix_result = p_->parse(src.c_str(), err_loc);
			if (prefix_result.found) {
				tref tree = prefix_result.get_shaped_tree2(
					highlight_shaping_);
				std::unordered_map<tref, uint32_t> overrides;
				classifier_.apply_patterns(tree, overrides);
				extract_tokens(tree, src, tokens, overrides);
				suffix_start = err_loc;
			}
		}
		// Fallback for everything past the parsed prefix.
		std::vector<size_t> line_offsets;
		build_line_offsets(src, line_offsets);
		fallback_tokens(src, suffix_start,
			line_offsets, tokens);
	} else {
		tref tree = result.get_shaped_tree2(highlight_shaping_);
		std::unordered_map<tref, uint32_t> overrides;
		classifier_.apply_patterns(tree, overrides);
		extract_tokens(tree, src, tokens, overrides);
	}

	// Post-extraction: merge adjacent same-type tokens.
	merge_pass(tokens);

	// Delta-encode into the VS Code wire format.
	std::vector<uint32_t> data;
	uint32_t prev_line = 0, prev_col = 0;
	for (auto& t : tokens) {
		uint32_t dline = t.line - prev_line;
		uint32_t dcol  = (dline == 0)
			? (t.col - prev_col) : t.col;
		data.push_back(dline);
		data.push_back(dcol);
		data.push_back(t.length);
		data.push_back(t.token_type);
		data.push_back(t.modifiers);
		prev_line = t.line;
		prev_col  = t.col;
	}
	return data;
}

} // namespace idni

#endif // __IDNI__PARSER__SYNTAX_HIGHLIGHTER_TMPL_H__
