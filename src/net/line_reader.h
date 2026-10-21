// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__NET__LINE_READER_H__
#define __IDNI__PARSER__NET__LINE_READER_H__

#include <cstddef>

namespace idni {

/// Split @p data into lines. A line over @p limit is reported once through
/// @p on_too_long, then its tail is dropped up to the next newline, so the
/// kept bytes stay bounded by the limit.
template <typename String, typename OnLine, typename OnTooLong>
inline void feed_lines(String& line, bool& dropping, size_t limit,
	const char* data, size_t n, OnLine on_line, OnTooLong on_too_long)
{
	for (size_t i = 0; i != n; ++i) {
		char c = data[i];
		if (dropping) {
			if (c == '\n') dropping = false;
			continue;
		}
		if (c == '\n') {
			on_line(line);
			line.clear();
			continue;
		}
		if (limit != 0 && line.size() >= limit) {
			line.clear();
			dropping = true;
			on_too_long();
			continue;
		}
		line.push_back(c);
	}
}

} // namespace idni

#endif // __IDNI__PARSER__NET__LINE_READER_H__
