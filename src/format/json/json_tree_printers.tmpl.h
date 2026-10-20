// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__FORMAT__JSON_TREE_PRINTERS_TMPL_H__
#define __IDNI__PARSER__FORMAT__JSON_TREE_PRINTERS_TMPL_H__

#include <algorithm>
#include <charconv>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "json.h"

namespace idni::format::json {

inline print_options print_options::minified() { return {}; }

inline print_options print_options::normal() {
	return { .ascii_only = true, .item_separator = ", ",
		.key_separator = ": ", .floats = float_format::repr,
		.lone_surrogate = lone_surrogate_policy::escape };
}

inline print_options print_options::pretty(size_t n) {
	return { .ascii_only = true, .item_separator = ",",
		.key_separator = ": ", .indent = n,
		.floats = float_format::repr,
		.lone_surrogate = lone_surrogate_policy::escape };
}

inline std::string float_repr(double d) {
	if (std::isnan(d)) return "nan";
	if (std::isinf(d)) return d < 0 ? "-inf" : "inf";
	// the shortest round trip digits, as d.ddde+XX
	char buf[32];
	auto r = std::to_chars(buf, buf + sizeof(buf), d,
		std::chars_format::scientific);
	std::string_view s(buf, static_cast<size_t>(r.ptr - buf));
	std::string out;
	if (s.front() == '-') out += '-', s.remove_prefix(1);
	const size_t at = s.find('e');
	std::string digits(1, s[0]);
	if (at > 1) digits += s.substr(2, at - 2);
	std::string_view es = s.substr(at + 1);
	const bool neg_exp = es.front() == '-';
	es.remove_prefix(1);
	int e = 0;
	std::from_chars(es.data(), es.data() + es.size(), e);
	if (neg_exp) e = -e;
	const size_t n = digits.size();
	if (e < -4 || e >= 16) {
		out += digits[0];
		if (n > 1) out += '.', out.append(digits, 1);
		const int mag = neg_exp ? -e : e;
		out += neg_exp ? "e-" : "e+";
		if (mag < 10) out += '0';
		return out + std::to_string(mag);
	}
	if (e < 0) return out + "0." + std::string(static_cast<size_t>(-e - 1),
		'0') + digits;
	const size_t point = static_cast<size_t>(e) + 1;
	if (n <= point) return out + digits + std::string(point - n, '0') + ".0";
	return out + digits.substr(0, point) + "." + digits.substr(point);
}

inline std::ostream& tree::print(std::ostream& os) const {
	return print(os, print_options{});
}

inline std::string tree::to_str() const { return to_str(print_options{}); }

inline std::ostream& tree::print(std::ostream& os, const print_options& o)
	const
{
	using policy = print_options::lone_surrogate_policy;
	auto prof = o.ascii_only ? idni::escapes::json_ascii
		: idni::escapes::json;
	prof.escape_delete = o.escape_delete || o.ascii_only;
	prof.wtf8_surrogates = o.lone_surrogate != policy::keep_bytes;
	auto text = [&](const std::string& s) {
		os << '"' << idni::escapes::encode(s, prof) << '"';
	};
	// A container on the way down and its next child: the lcrs sibling
	// cursor, or the index into its keys when they print sorted. A stack,
	// so a deep document fits.
	struct frame {
		tref n;
		tref cursor;
		trefs sorted{};
		size_t at = 0;
		bool started = false;
	};
	std::vector<frame> open;
	// The open arrays and objects. A pair frame adds no level.
	size_t depth = 0;
	auto new_line = [&](size_t level) {
		if (!o.indent) return;
		os << '\n';
		for (size_t i = 0; i != level * o.indent; ++i) os << ' ';
	};
	// Write the opening of n and push its frame when it has children.
	auto enter = [&](tref n) {
		const tree& t = get(n);
		switch (t.get_type()) {
		case json_parser::null_sym:  os << "null"; return;
		case json_parser::true_sym:  os << "true"; return;
		case json_parser::false_sym: os << "false"; return;
		case json_parser::number: {
			if (!t.value.str.empty()) { os << t.value.str; return; }
			const double d = t.get_number();
			if (!std::isfinite(d)) {
				if (o.nonfinite) os << (std::isnan(d) ? "NaN"
					: d < 0 ? "-Infinity" : "Infinity");
				else os << "null";
				return;
			}
			if (o.floats == print_options::float_format::repr) {
				os << float_repr(d);
				return;
			}
			char buf[32];
			auto r = std::to_chars(buf, buf + sizeof(buf), d);
			os.write(buf, r.ptr - buf);
			return;
		}
		case json_parser::str: text(t.get_string()); return;
		case json_parser::arr: os << '['; break;
		case json_parser::object: os << '{'; break;
		case json_parser::object_pair:
			text(t.get_string());
			os << o.key_separator;
			break;
		default: DBG(assert(false);) return;
		}
		if (t.get_type() != json_parser::object_pair) ++depth;
		if (!o.sorted_keys || !t.is_object()) {
			open.push_back({ n, t.left_child() });
			return;
		}
		trefs keys = t.get_children();
		std::sort(keys.begin(), keys.end(), [](tref a, tref b) {
			return get(a).get_string() < get(b).get_string();
		});
		open.push_back({ n, nullptr, std::move(keys) });
	};
	// the root can be an element with a right sibling, which is not printed
	enter(get());
	while (!open.empty()) {
		frame& f = open.back();
		tref c = nullptr;
		if (f.at != f.sorted.size()) c = f.sorted[f.at++];
		else if ((c = f.cursor)) f.cursor = get(c).right_sibling();
		const tree& ft = get(f.n);
		const bool pair = ft.get_type() == json_parser::object_pair;
		if (c != nullptr) {
			if (!pair) {
				if (f.started) os << o.item_separator;
				new_line(depth);
			}
			f.started = true;
			enter(c);
			continue;
		}
		if (!pair) {
			--depth;
			if (f.started) new_line(depth);
			os << (ft.is_array() ? ']' : '}');
		}
		open.pop_back();
	}
	return os;
}

inline std::string tree::to_str(const print_options& o) const {
	std::ostringstream os;
	print(os, o);
	return os.str();
}

inline std::ostream& tree::print_tree(std::ostream& os, size_t s) const {
	return base_t::print(os, s);
}

inline std::ostream& tree::dump(std::ostream& os, bool subtree) const {
	return base_t::dump(os, subtree);
}

inline std::string tree::dump_to_str(bool subtree) const {
	return base_t::dump_to_str(subtree);
}

inline std::ostream& operator<<(std::ostream& os, const node& n) {
	os << n.name();
	if (n.nt == json_parser::number && !n.str.empty())
		os << ' ' << n.str;
	else if (n.nt == json_parser::number)
		os << ' ' << std::bit_cast<double>(n.data);
	else if (n.nt == json_parser::str || n.nt == json_parser::object_pair)
		escape(os << ' ', n.str);
	return os;
}

inline std::ostream& operator<<(std::ostream& os, const tree& t) {
	return t.print(os);
}

inline std::ostream& operator<<(std::ostream& os, const tt& t) {
	if (!t) return os << "(no value)";
	for (size_t i = 0; i != t.size(); ++i)
		os << (i ? "\n" : "") << tree::get(t.values()[i]);
	return os;
}

inline std::ostream& write_line(tref v, std::ostream& os) {
#ifdef _WIN32
	if (&os == &std::cout) ::_setmode(::_fileno(stdout), _O_BINARY);
#endif
	return tree::get(v).print(os) << '\n' << std::flush;
}

inline std::ostream& print(const diagnostics::report& r, std::ostream& os,
	bool print_names)
{
	return tree::get(tree::build_report(r, print_names)).print(os);
}

} // namespace idni::format::json

#endif // __IDNI__PARSER__FORMAT__JSON_TREE_PRINTERS_TMPL_H__
