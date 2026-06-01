// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__PARSER_STRINGS_H__
#define __IDNI__PARSER__PARSER_STRINGS_H__

#include <string_view>

#include "defs.h"

namespace idni::diagnostics { struct report; }

namespace idni::parser_strings {

using id_t = idni::int_t;

#define PARSER_DIAG_COUNTERS(X) \
	X(inner_loop_iterations,      "inner loop iterations") \
	X(inner_loop_iterations_max,  "inner loop iterations max") \
	X(complete_calls,             "complete calls") \
	X(predict_calls,              "predict calls") \
	X(predict_inserts,            "predict inserts") \
	X(scan_calls,                 "scan calls") \
	X(scan_cc_calls,              "scan cc calls") \
	X(s_max_per_pos,              "s max per pos") \
	X(t_size_peak,                "t size peak") \
	X(c_size_peak,                "c size peak") \
	X(completion_deps_size_peak,  "completion deps size peak") \
	X(refi_increments,            "refi increments") \
	X(refi_decrements,            "refi decrements") \
	X(refi_zero_hits,             "refi zero hits") \
	X(refi_size_peak,             "refi size peak") \
	X(gcready_skipped_refcounted, "gcready skipped refcounted") \
	X(remove_item_calls,          "remove item calls") \
	X(remove_item_scan_total,     "remove item scan total") \
	X(conjunction_attempts,       "conjunction attempts") \
	X(conjunction_failures,       "conjunction failures") \
	X(cascade_uncomplete_calls,   "cascade uncomplete calls") \
	X(cascade_dep_total,          "cascade dep total") \
	X(fromS_writes,               "fromS writes") \
	X(fromS_reads,                "fromS reads") \
	X(s_total_remaining,          "S total remaining") \
	X(gc_collected,               "gc collected") \
	X(gcready_size_final,         "gcready size final") \
	X(refi_size_final,            "refi size final")

// Well-known diagnostic label strings. Positive ids are O(1) lookups via
// str(label). Unknown/dynamic labels are stored per-report with negative ids.
#define LABELS(X) \
	X(root,               "root") \
	X(grammar_load,       "grammar load") \
	X(parse,              "parse") \
	X(parsing,            "parsing") \
	X(tgf_parse,          "tgf parse") \
	X(tgf_build,          "tgf build") \
	X(preprocess,         "preprocess") \
	X(build_bintree,      "build bintree") \
	X(build_forest,       "build forest") \
	X(reconstruct_forest, "reconstruct forest") \
	X(reconstruct_bintree,"reconstruct bintree") \
	X(line,               "line") \
	X(col,                "col") \
	X(name,               "name") \
	X(loc,                "loc") \
	X(value,              "value") \
	PARSER_DIAG_COUNTERS(X) \
	X(rss_before,         "rss before") \
	X(rss_after,          "rss after") \
	X(peak_rss_before,    "peak rss before") \
	X(peak_rss_after,     "peak rss after") \
	X(input_length,       "input length") \
	X(position_count,     "position count") \
	X(shaped_tree,        "shaped tree") \
	X(extract_graph,      "extract graph") \
	X(inline_grammar,     "inline grammar") \
	X(extract_tree2,      "extract tree2") \
	X(trim,               "trim") \
	X(inline_nodes,       "inline nodes") \
	X(inline_paths,       "inline paths") \
	X(trim_terminals,     "trim terminals")

// Human-readable labels for idni::diagnostics::code values.
// Sentence form: capital first word, trailing period.
#define CODE_LABELS(X) \
	X(unknown,                  "Unknown.") \
	X(invalid_argument,         "Invalid argument.") \
	X(internal_error,           "Internal error.") \
	X(io_error,                 "I/O error.") \
	X(type_error,               "Type error.") \
	X(unsupported_operation,    "Unsupported operation.") \
	X(invalid_state,            "Invalid state.") \
	X(not_found,                "Not found.") \
	X(missing_type_information, "Missing type information.") \
	X(invalid_input_stream,     "Invalid input stream.") \
	X(invalid_output_stream,    "Invalid output stream.") \
	X(invalid_memory_access,    "Invalid memory access.") \
	X(out_of_range,             "Out of range.") \
	X(unsat,                    "Unsatisfiable.") \
	X(solver_error,             "Solver error.") \
	X(parse_error,              "Parse error.") \
	X(runtime_error,            "Runtime error.") \
	X(unknown_char_class,       "Unknown character class.") \
	X(warning,                  "Warning.") \
	X(info,                     "Info.") \
	X(info_micros,              "Time.") \
	X(info_kb,                  "Memory.") \
	X(info_count,               "Count.")

struct label {
	enum : id_t { none = 0,
#define E(nm, s) nm,
		LABELS(E)
#undef E
		count };
};

struct code_label {
	enum : id_t { none = 0,
#define E(nm, s) nm,
		CODE_LABELS(E)
#undef E
		count };
};

// id_t → string_view for well-known labels (used by diagnostics::report).
inline constexpr std::string_view str(id_t i) {
	constexpr std::string_view table[] = { "",
#define S(nm, s) s,
		LABELS(S)
#undef S
	};
	return static_cast<size_t>(i) < std::size(table) ? table[i] : table[0];
}

// id_t → string_view for diagnostic code labels.
inline constexpr std::string_view str_code(id_t i) {
	constexpr std::string_view table[] = { "",
#define S(nm, s) s,
		CODE_LABELS(S)
#undef S
	};
	return static_cast<size_t>(i) < std::size(table) ? table[i] : table[0];
}

// Linear scan: string → label id. Used only for interning at report boundaries.
inline id_t label_of(std::string_view sv) {
	if (sv.empty()) return 0;
	for (id_t i = 1; i < label::count; ++i)
		if (str(i) == sv) return i;
	return 0;
}

struct messages {
	using sv = std::string_view;
	static constexpr sv cannot_open_file            = "Cannot open file: ";
	static constexpr sv loading_grammars_unavailable =
		"Loading grammars is not available in a specialized REPL";
	static constexpr sv unproductive_nonterminal    = "Unproductive nonterminal: ";
};

struct counters {
#define PARSER_DIAG_COUNTER_FIELD(id, str) size_t id = 0;
	PARSER_DIAG_COUNTERS(PARSER_DIAG_COUNTER_FIELD)
#undef PARSER_DIAG_COUNTER_FIELD
};

void flush_counters(idni::diagnostics::report& r, const counters& c);

} // namespace idni::parser_strings


#include "parser_strings.tmpl.h"

#endif // __IDNI__PARSER__PARSER_STRINGS_H__
