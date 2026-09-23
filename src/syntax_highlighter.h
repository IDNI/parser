// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__SYNTAX_HIGHLIGHTER_H__
#define __IDNI__PARSER__SYNTAX_HIGHLIGHTER_H__

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <optional>
#include <utility>
#include <unordered_map>

#include "parser.h"
#include "format/treemr/treemr.h"

namespace idni {

// A single semantic token with its position and classification.
struct extracted_token
{
	uint32_t line;         // 0-based line in the document
	uint32_t col;          // column in the document's position unit
	uint32_t length;       // length of the token in position units
	uint32_t token_type;   // index into the legend
	uint32_t modifiers;    // bitmask, currently 0
};

// Maps nonterminal ids to token type indices via structure, heuristics and @highlight.
struct token_classifier
{
	// Build the classifier from a compiled grammar and its nonterminals.
	void init(const grammar<char, char>& g,
		const nonterminals<char, char>& nts, bool heuristics,
		std::string& diagnostics);

	// Return the token type index for a nonterminal id, or no_type.
	uint32_t classify(size_t nt_id) const;

	// A later pattern overwrites an earlier one on the same node.
	void apply_patterns(tref root,
		std::unordered_map<tref, uint32_t>& overrides) const;

	// Return true if the nonterminal is a character class function.
	bool is_char_class_nt(size_t nt_id) const;

	// Return true if the nonterminal is transparent.
	bool is_transparent_nt(size_t nt_id) const;

	// Return true if a terminal under this nonterminal should classify via its own ancestor instead.
	bool is_skipped_parent(size_t nt_id) const {
		return is_transparent_nt(nt_id) || classify(nt_id) == no_type;
	}

	// True if name matches pat, where pat may contain '*' wildcards.
	static bool glob_match(const std::string& pat, const std::string& name);

	// Look up a token type index by name (e.g. "keyword" -> 0), or no_type.
	static uint32_t type_index(const std::string& name);
	// The neutral fallback type index ("variable").
	static uint32_t default_type();

	// Sentinel returned when nothing classified the nonterminal.
	static constexpr uint32_t no_type = 0xffffffffu;

	// Token type names in legend order (index -> name).
	static const std::vector<std::string>& token_type_names();
	static const std::vector<std::string>& token_modifier_names();

private:
	std::map<size_t, uint32_t> nt_to_type_;
	std::set<size_t> char_class_nts_;
	std::set<size_t> digit_class_nts_;
	std::set<size_t> transparent_nts_;

	// Compiled treemr patterns in declaration order, each with its type.
	std::vector<std::pair<treemr::matcher<pnode_type<char, char>>,
		uint32_t>> patterns_;

	// Classify by nonterminal name (suffix/prefix/exact patterns), or no_type.
	uint32_t classify_by_name(const std::string& name) const;

	// Classify by what the nonterminal's productions actually produce, or no_type.
	uint32_t classify_by_content(const grammar<char, char>& g,
		size_t nt_id) const;

	// True if nt_id produces a digit/xdigit character class.
	bool produces_digit_class(const grammar<char, char>& g,
		size_t nt_id, std::set<size_t>& visited) const;

	// Quote char that delimits l, if l is a quote terminal or an all-quote nonterminal, else 0.
	char boundary_quote(const grammar<char, char>& g,
		const lit<char, char>& l) const;
};

// Owns a compiled TGF grammar and parser and extracts semantic tokens from source text.
struct syntax_highlighter
{
	// Compile a TGF grammar; heuristics overrides the grammar's own @highlight auto setting.
	explicit syntax_highlighter(const std::string& grammar_src,
		std::optional<bool> heuristics = std::nullopt);

	bool good() const { return good_; }
	const std::string& diagnostics() const { return diagnostics_; }

	// Parse source text and return delta-encoded semantic tokens (VS Code wire format).
	std::vector<uint32_t> get_tokens(const std::string& src);

	// Legend-defining ordered lists.
	static const std::vector<std::string>& get_token_types()
		{ return token_classifier::token_type_names(); }
	static const std::vector<std::string>& get_token_modifiers()
		{ return token_classifier::token_modifier_names(); }

	// Number of nonterminals in the compiled grammar.
	size_t nt_count() const;

	// Debug: nonterminal name for an id.
	std::string get_nt_name(size_t id) const;

	// Debug: token type name for a nonterminal id, or "" for no_type.
	std::string get_nt_type(size_t id) const;

private:
	std::unique_ptr<nonterminals<char, char>> nts_;
	std::unique_ptr<grammar<char, char>> g_;
	std::unique_ptr<parser<char, char>> p_;
	token_classifier classifier_;
	std::string diagnostics_;
	bool good_ = false;
	bool heuristics_ = false;

	// Shaping for highlighting: all trimming and all inlining are disabled.
	shaping_options highlight_shaping_;

	// Build a line-start offset table (byte offsets) over the source.
	void build_line_offsets(const std::string& src,
		std::vector<size_t>& offsets) const;

	// Count UTF-16 code units in the byte range src[a, b).
	uint32_t utf16_units(const std::string& src,
		size_t a, size_t b) const;

	// Append a token for a byte span, converted to line/col/length; drops a no_type run.
	void flush_run(uint32_t type, size_t start, size_t end,
		const std::string& src,
		const std::vector<size_t>& line_offsets,
		std::vector<extracted_token>& out) const;

	// Walk the shaped parse tree and extract tokens, honoring overrides.
	void extract_tokens(tref root, const std::string& src,
		std::vector<extracted_token>& out,
		const std::unordered_map<tref, uint32_t>& overrides) const;

	// Character-class-based tokenization for an unparsed suffix.
	void fallback_tokens(const std::string& src, size_t offset,
		const std::vector<size_t>& line_offsets,
		std::vector<extracted_token>& out) const;

	// True for one of the delimiter punctuation characters .,;:()[]{}.
	static bool is_delimiter_char(char ch);
};

} // namespace idni

#include "syntax_highlighter.tmpl.h"

#endif // __IDNI__PARSER__SYNTAX_HIGHLIGHTER_H__
