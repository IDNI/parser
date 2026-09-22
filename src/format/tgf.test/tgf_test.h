// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__FORMAT__TGF_TEST__TGF_TEST_H__
#define __IDNI__PARSER__FORMAT__TGF_TEST__TGF_TEST_H__
#include <map>
#include <optional>
#include <string>

#include "tgf_test_parser.generated.h"
#include "grammar_inspector.h"
#include "format/treemr/treemr.h"

namespace idni {

// Local alias for tgf_test parser nonterminals - disambiguates from the
// host parser's nested types/fields with the same identifiers.
using tgf_test_nt = ::tgf_test_parser_nonterminals;

template <typename C = char, typename T = C>
struct tgf_test {
	using tree = tgf_test_parser::tree;
	using trv  = tree::traverser;

	// Default ambiguity mode when an item and its enclosing entries
	// carry no @forbid/@unique/@all/@any word.
	static constexpr treemr::ambig_mode default_ambig_mode =
		treemr::ambig_mode::FORBID;

	struct result {
		int    ret    = 0;
		size_t passed = 0;
		size_t failed = 0;
	};

	template <typename Parser>
	static result run_from_string(Parser& p, const std::basic_string<C>& s);

	template <typename Parser>
	static result run_from_file(Parser& p, const std::string& filename);

private:
	// Shared per-run state: grammar_inspector and compiled matcher
	// cache, both built once and reused across an entry's items.
	struct run_ctx {
		grammar_inspector<C, T> gi;
		std::map<std::string, treemr::compiled_pattern> compiled;
		size_t passed = 0, failed = 0;
		explicit run_ctx(grammar<C, T>& g) : gi(g) {}
	};

	// The ambiguity-mode/raw-tree words attached to an entry or item.
	struct words_info {
		std::optional<treemr::ambig_mode> mode;
		bool raw      = false;
		bool conflict = false;
	};

	template <typename Parser>
	static int run_entry(Parser& p, run_ctx& ctx, const trv& entry,
		const std::string& parent_text, size_t start_nt,
		treemr::ambig_mode inherited_mode, bool inherited_raw,
		bool inherited_negate);

	template <typename Parser>
	static int run_item(Parser& p, run_ctx& ctx, const trv& item,
		const std::string& entry_text, size_t start_nt,
		treemr::ambig_mode entry_mode, bool entry_raw,
		bool entry_negate);

	static words_info parse_words(const trv& words_trv);

	static std::string matcher_text(const trv& matcher_trv);
	static std::optional<std::string> leading_name(
		const std::string& text);

	template <typename Parser>
	static std::optional<size_t> resolve_nt(Parser& p, run_ctx& ctx,
		const std::string& name);

	static std::string decode_quoted_string(const trv& qs,
		std::string& err);

	static std::string decode_escape(const std::string& raw,
		std::string& err);

	static std::string read_input(const trv& in_trv, std::string& err);
};

} // namespace idni

#include "tgf_test.tmpl.h"

#endif // __IDNI__PARSER__FORMAT__TGF_TEST__TGF_TEST_H__
