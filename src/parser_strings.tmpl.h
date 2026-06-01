// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include "utility/diagnostics.h"

namespace idni::parser_strings {

#ifdef TAU_PARSER_MEASURE_COUNTERS
inline void flush_counters(idni::diagnostics::report& r, const counters& c) {
#define FLUSH(nm, str) if (c.nm) r.count(label::nm, c.nm);
	PARSER_DIAG_COUNTERS(FLUSH)
#undef FLUSH
}
#else
inline void flush_counters(idni::diagnostics::report&, const counters&) {}
#endif // TAU_PARSER_MEASURE_COUNTERS

} // namespace idni::parser_strings
