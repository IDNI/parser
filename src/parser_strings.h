// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__PARSER_STRINGS_H__
#define __IDNI__PARSER__PARSER_STRINGS_H__

#include <array>
#include <string>
#include <string_view>

#include "defs.h"

namespace idni::diagnostics { class report; }

namespace idni::parser_strings {

using id_t = idni::int_t;

#define PARSER_DIAG_COUNTERS(X) \
	X(inner_loop_iterations, "inner loop iterations") \
	X(inner_loop_iterations_max, "inner loop iterations max") \
	X(complete_calls, "complete calls") \
	X(predict_calls, "predict calls") \
	X(predict_inserts, "predict inserts") \
	X(scan_calls, "scan calls") \
	X(scan_cc_calls, "scan cc calls") \
	X(s_max_per_pos, "s max per pos") \
	X(t_size_peak, "t size peak") \
	X(c_size_peak, "c size peak") \
	X(completion_deps_size_peak, "completion deps size peak") \
	X(refi_increments, "refi increments") \
	X(refi_decrements, "refi decrements") \
	X(refi_zero_hits, "refi zero hits") \
	X(refi_size_peak, "refi size peak") \
	X(gcready_skipped_refcounted, "gcready skipped refcounted") \
	X(remove_item_calls, "remove item calls") \
	X(remove_item_scan_total, "remove item scan total") \
	X(conjunction_attempts, "conjunction attempts") \
	X(conjunction_failures, "conjunction failures") \
	X(cascade_uncomplete_calls, "cascade uncomplete calls") \
	X(cascade_dep_total, "cascade dep total") \
	X(fromS_writes, "fromS writes") \
	X(fromS_reads, "fromS reads") \
	X(s_total_remaining, "S total remaining") \
	X(gc_collected, "gc collected") \
	X(gcready_size_final, "gcready size final") \
	X(refi_size_final, "refi size final")

#define PARSER_STATIC_LABELS(X) \
	X(root, "root") \
	X(grammar_load, "grammar load") \
	X(parse, "parse") \
	X(parsing, "parsing") \
	X(tgf_parse, "tgf parse") \
	X(tgf_build, "tgf build") \
	X(preprocess, "preprocess") \
	X(build_bintree, "build bintree") \
	X(build_forest, "build forest") \
	X(reconstruct_forest, "reconstruct forest") \
	X(reconstruct_bintree, "reconstruct bintree") \
	X(line, "line") \
	X(col, "col") \
	X(name, "name") \
	X(loc, "loc") \
	X(value, "value") \
	PARSER_DIAG_COUNTERS(X) \
	X(rss_before, "rss before") \
	X(rss_after, "rss after") \
	X(peak_rss_before, "peak rss before") \
	X(peak_rss_after, "peak rss after") \
	X(input_length, "input length") \
	X(position_count, "position count") \
	X(shaped_tree, "shaped tree") \
	X(extract_graph, "extract graph") \
	X(inline_grammar, "inline grammar") \
	X(extract_tree2, "extract tree2") \
	X(trim, "trim") \
	X(inline_nodes, "inline nodes") \
	X(inline_paths, "inline paths") \
	X(trim_terminals, "trim terminals")

// Human-readable labels for idni::diagnostics::code values. Each entry's
// identifier matches the corresponding `code::<id>` enum value; the
// generated static_key entry is `code_<id>` (via token-paste) to avoid
// colliding with PARSER_STATIC_LABELS entries that share short names
// (e.g. `warning`, `info`). Labels are sentence form: capital first
// word, trailing period — emitted by code_name() and the JSON tag_name
// field.
#define PARSER_CODE_LABELS(X) \
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

struct names {
	using sv = std::string_view;
#define PARSER_STATIC_LABEL_NAME(id, label) static constexpr sv id = label;
#define PARSER_CODE_LABEL_NAME(id, label) \
	static constexpr sv code_##id = label;
	PARSER_STATIC_LABELS(PARSER_STATIC_LABEL_NAME)
	PARSER_CODE_LABELS(PARSER_CODE_LABEL_NAME)
#undef PARSER_STATIC_LABEL_NAME
#undef PARSER_CODE_LABEL_NAME
};

enum class static_key : id_t {
	none = 0,
#define PARSER_STATIC_LABEL_KEY(id, label) id,
#define PARSER_CODE_LABEL_KEY(id, label) code_##id,
	PARSER_STATIC_LABELS(PARSER_STATIC_LABEL_KEY)
	PARSER_CODE_LABELS(PARSER_CODE_LABEL_KEY)
#undef PARSER_STATIC_LABEL_KEY
#undef PARSER_CODE_LABEL_KEY
	count
};

inline constexpr id_t key_id(static_key k) {
	return static_cast<id_t>(k);
}

// THREAD-SAFETY: the diagnostics/parser label dictionary is immutable after
// function-local static initialization. Positive ids are reserved for the
// well-known labels listed in PARSER_STATIC_LABELS. Unknown/user labels are
// not inserted here; diagnostics::report stores them as report-local dynamic
// strings with negative ids.
inline const auto& dict_strings() {
	static const std::array<std::string,
		static_cast<size_t>(static_key::count)> v{{
		std::string{},
#define PARSER_STATIC_LABEL_STRING(id, label) std::string{label},
		PARSER_STATIC_LABELS(PARSER_STATIC_LABEL_STRING)
		PARSER_CODE_LABELS(PARSER_STATIC_LABEL_STRING)
#undef PARSER_STATIC_LABEL_STRING
	}};
	return v;
}

inline id_t dict(std::string_view sv) {
	if (sv.empty()) return 0;
	const auto& strings = dict_strings();
	for (size_t i = 1; i < strings.size(); ++i)
		if (std::string_view{strings[i]} == sv)
			return static_cast<id_t>(i);
	return 0;
}

inline id_t dict(const char* s) {
	return (!s || !*s) ? 0 : dict(std::string_view{s});
}

inline id_t dict(const std::string& s) {
	return s.empty() ? 0 : dict(std::string_view{s});
}

inline const std::string& dict(id_t id) {
	const auto& strings = dict_strings();
	return id > 0 && static_cast<size_t>(id) < strings.size()
		? strings[static_cast<size_t>(id)]
		: strings[0];
}

inline id_t dict_size() {
	return static_cast<id_t>(dict_strings().size());
}

inline void dict_reset(id_t) {
	// Compatibility no-op: the static label dictionary is immutable.
}

struct messages {
	using sv = std::string_view;
	static constexpr sv cannot_open_file = "cannot open file: ";
};

// Materializes ids for every well-known name. The ids are compile-time constants
// into the immutable static dictionary above, so constructing keys does not
// mutate process-global state and is safe across threads.
struct keys {
	keys() = default;
	id_t grammar_load        = key_id(static_key::grammar_load);
	id_t parse               = key_id(static_key::parse);
	id_t parsing             = key_id(static_key::parsing);
	id_t tgf_parse           = key_id(static_key::tgf_parse);
	id_t tgf_build           = key_id(static_key::tgf_build);
	id_t preprocess          = key_id(static_key::preprocess);
	id_t build_bintree       = key_id(static_key::build_bintree);
	id_t build_forest        = key_id(static_key::build_forest);
	id_t reconstruct_forest  = key_id(static_key::reconstruct_forest);
	id_t reconstruct_bintree = key_id(static_key::reconstruct_bintree);
	id_t line                = key_id(static_key::line);
	id_t col                 = key_id(static_key::col);
	id_t name                = key_id(static_key::name);
	id_t loc                 = key_id(static_key::loc);
	id_t value               = key_id(static_key::value);
#define PARSER_DIAG_COUNTER_FIELD(id, label) \
	id_t id = key_id(static_key::id);
	PARSER_DIAG_COUNTERS(PARSER_DIAG_COUNTER_FIELD)
#undef PARSER_DIAG_COUNTER_FIELD
	id_t rss_before          = key_id(static_key::rss_before);
	id_t rss_after           = key_id(static_key::rss_after);
	id_t peak_rss_before     = key_id(static_key::peak_rss_before);
	id_t peak_rss_after      = key_id(static_key::peak_rss_after);
	id_t input_length        = key_id(static_key::input_length);
	id_t position_count      = key_id(static_key::position_count);
	id_t shaped_tree         = key_id(static_key::shaped_tree);
	id_t extract_graph       = key_id(static_key::extract_graph);
	id_t inline_grammar      = key_id(static_key::inline_grammar);
	id_t extract_tree2       = key_id(static_key::extract_tree2);
	id_t trim                = key_id(static_key::trim);
	id_t inline_nodes        = key_id(static_key::inline_nodes);
	id_t inline_paths        = key_id(static_key::inline_paths);
	id_t trim_terminals      = key_id(static_key::trim_terminals);
};

// Global constexpr handle for the well-known keys. Construction is a
// compile-time fold over key_id(static_key::...) values, so `KEYS.line`
// is just an integer literal at the use site — no per-call lookup.
inline constexpr keys KEYS{};

struct counters {
#define PARSER_DIAG_COUNTER_FIELD(id, label) size_t id = 0;
	PARSER_DIAG_COUNTERS(PARSER_DIAG_COUNTER_FIELD)
#undef PARSER_DIAG_COUNTER_FIELD
};

void flush_counters(idni::diagnostics::report& r,
	const counters& c, const keys& k);

} // namespace idni::parser_strings


#include "parser_strings.tmpl.h"

#endif // __IDNI__PARSER__PARSER_STRINGS_H__
