// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__FORMAT__JSON_H__
#define __IDNI__PARSER__FORMAT__JSON_H__

#include <iostream>
#include <string_view>
#include <cstdio>

namespace idni::format::json {

inline void escape(std::ostream& os, std::string_view s) {
	os << '"';
	for (char c : s) {
		switch (c) {
		case '"':  os << "\\\""; break;
		case '\\': os << "\\\\"; break;
		case '\b': os << "\\b";  break;
		case '\f': os << "\\f";  break;
		case '\n': os << "\\n";  break;
		case '\r': os << "\\r";  break;
		case '\t': os << "\\t";  break;
		default:
			if (static_cast<unsigned char>(c) < 0x20) {
				char buf[8];
				std::snprintf(buf, sizeof(buf),
					"\\u%04x",
					static_cast<unsigned>(
						static_cast<unsigned char>(c)));
				os << buf;
			} else {
				os << c;
			}
		}
	}
	os << '"';
}
	
}

#endif // __IDNI__PARSER__FORMAT__JSON_H__
