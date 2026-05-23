// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include "utility/diagnostics.h"

namespace idni::parser_strings {

#ifdef TAU_PARSER_MEASURE_COUNTERS
inline void flush_counters(idni::diagnostics::report& r,
	const counters& c, const keys& k)
{
#define PARSER_DIAG_COUNTER_FLUSH(id, label) \
	if (c.id) r.count(k.id, c.id);
	PARSER_DIAG_COUNTERS(PARSER_DIAG_COUNTER_FLUSH)
#undef PARSER_DIAG_COUNTER_FLUSH
}
#else
inline void flush_counters(idni::diagnostics::report&,
	const counters&, const keys&) {}
#endif // TAU_PARSER_MEASURE_COUNTERS

} // namespace idni::parser_strings
