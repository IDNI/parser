// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__FORMAT__JSON_TREE_PRINTERS_TMPL_H__
#define __IDNI__PARSER__FORMAT__JSON_TREE_PRINTERS_TMPL_H__

#include <algorithm>
#include <charconv>
#include <sstream>
#include <string>
#include <vector>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "json.h"

namespace idni::format::json {

inline std::ostream& tree::print(std::ostream& os) const {
	return print(os, print_options{});
}

inline std::string tree::to_str() const { return to_str(print_options{}); }

inline std::ostream& tree::print(std::ostream& os, const print_options& o)
	const
{
	auto prof = o.ascii_only ? idni::escapes::json_ascii
		: idni::escapes::json;
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
			os << ':';
			break;
		default: DBG(assert(false);) return;
		}
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
			if (!pair && f.started) os << ',';
			f.started = true;
			enter(c);
			continue;
		}
		if (!pair) os << (ft.is_array() ? ']' : '}');
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
