// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__NET__LINE_ADDRESS_TMPL_H__
#define __IDNI__PARSER__NET__LINE_ADDRESS_TMPL_H__

#include <cctype>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "line_address.h"
#include "parser_strings.h"

namespace idni {

namespace line_address_detail {

inline diagnostics::result<connect_target> bad(
	diagnostics::result<connect_target>& res, std::string_view text)
{
	res.error(diagnostics::code::invalid_argument,
		parser_strings::messages::net_bad_address,
		{ { parser_strings::label::value, std::string(text) } });
	return std::move(res);
}

// "<host>[:<port>]" or "[<ipv6>][:<port>]" into @p t. False on a bad
// form or a missing host. A missing port keeps the port of @p t.
inline bool parse_host_port(std::string_view rest, connect_target& t) {
	// a path, a query, a fragment or user info
	if (rest.empty() || rest.find_first_of("/?#@") != std::string_view::npos)
		return false;
	std::string_view port_text;
	bool has_port = false;
	if (rest.front() == '[') {
		auto close = rest.find(']');
		if (close == std::string_view::npos || close == 1) return false;
		auto host = rest.substr(1, close - 1);
		for (char c : host)
			if (!std::isxdigit(static_cast<unsigned char>(c))
					&& c != ':' && c != '.')
				return false;
		t.host = std::string(host);
		auto after = rest.substr(close + 1);
		if (!after.empty()) {
			if (after.front() != ':') return false;
			port_text = after.substr(1);
			has_port = true;
		}
	} else {
		if (auto colon = rest.rfind(':');
				colon != std::string_view::npos) {
			// an IPv6 address needs its brackets
			if (rest.substr(0, colon).find(':')
					!= std::string_view::npos)
				return false;
			port_text = rest.substr(colon + 1);
			has_port = true;
			rest = rest.substr(0, colon);
		}
		if (rest.empty() || rest.find_first_of("[]")
				!= std::string_view::npos)
			return false;
		for (char ch : rest) {
			const auto c = static_cast<unsigned char>(ch);
			if (c <= 0x20 || c == 0x7f) return false;
		}
		t.host = std::string(rest);
	}
	if (!has_port) return true;
	if (port_text.empty()) return false;
	unsigned long p = 0;
	for (char c : port_text) {
		if (c < '0' || c > '9') return false;
		p = p * 10 + static_cast<unsigned long>(c - '0');
		if (p > 65535) return false;
	}
	if (p == 0) return false;
	t.port = static_cast<uint16_t>(p);
	return true;
}

} // namespace line_address_detail

/// @p path with a leading "~" or "~/" replaced by the home folder:
/// HOME, or USERPROFILE on Windows. "~<user>" is an error.
inline diagnostics::result<std::string> expand_home(std::string_view path) {
	diagnostics::result<std::string> res;
	if (path.empty() || path.front() != '~')
		return res.with_value(std::string(path));
	if (path.size() > 1 && path[1] != '/' && path[1] != '\\') {
		res.error(diagnostics::code::invalid_argument,
			parser_strings::messages::net_bad_address,
			{ { parser_strings::label::value, std::string(path) } });
		return res;
	}
#ifdef _WIN32
	constexpr const char* home_var = "USERPROFILE";
#else
	constexpr const char* home_var = "HOME";
#endif
#ifdef _MSC_VER
	// the MSVC CRT marks getenv unsafe and asks for _dupenv_s
	char* raw = nullptr;
	std::size_t raw_size = 0;
	const bool read = _dupenv_s(&raw, &raw_size, home_var) == 0;
	const std::unique_ptr<char, decltype(&std::free)> owned(raw,
		&std::free);
	const char* home = read ? owned.get() : nullptr;
#else
	const char* home = std::getenv(home_var);
#endif
	if (!home || !*home) {
		res.error(diagnostics::code::runtime_error,
			parser_strings::messages::option_env_read_failed,
			{ { parser_strings::label::name, std::string(home_var) } });
		return res;
	}
	return res.with_value(std::string(home)
		+ std::string(path.substr(1)));
}

inline diagnostics::result<connect_target> parse_connect_address(
	std::string_view text, const connect_address_rules& rules)
{
	using line_address_detail::bad;
	diagnostics::result<connect_target> res;
	connect_target t;
	t.port = rules.default_port;
	auto local = [&res, &t, text](std::string_view path) {
		if (path.empty()) return bad(res, text);
		auto p = res.merge_take(expand_home(path));
		if (!p) return std::move(res);
		t.kind = line_transport_kind::local_socket;
		t.path = std::move(*p);
		return res.with_value(std::move(t));
	};
	if (text.empty()) {
		if (rules.default_socket_path.empty()) return bad(res, text);
		t.kind = line_transport_kind::local_socket;
		t.path = rules.default_socket_path;
		return res.with_value(std::move(t));
	}
	if (text.front() == '/' || text.front() == '~') return local(text);
#ifdef _WIN32
	if (text.starts_with("\\\\")) return local(text);
#endif
	std::string_view rest = text;
	t.kind = line_transport_kind::tcp;
	if (auto sep = rest.find("://"); sep != std::string_view::npos) {
		const auto scheme = rest.substr(0, sep);
		rest.remove_prefix(sep + 3);
		if (scheme == std::string(rules.scheme) + "+unix")
			return local(rest);
		if (scheme == "ws") {
			t.kind = line_transport_kind::websocket;
			if (auto slash = rest.find('/');
					slash != std::string_view::npos) {
				const auto path = rest.substr(slash);
				for (char ch : path) {
					const auto c = static_cast<unsigned char>(ch);
					if (c <= 0x20 || c == 0x7f || c == '#')
						return bad(res, text);
				}
				t.ws_path = std::string(path);
				rest = rest.substr(0, slash);
			}
		} else if (scheme != rules.scheme) return bad(res, text);
		else if (!rest.empty() && rest.back() == '/') rest.remove_suffix(1);
	}
	if (!line_address_detail::parse_host_port(rest, t)) return bad(res, text);
	return res.with_value(std::move(t));
}

} // namespace idni

#endif // __IDNI__PARSER__NET__LINE_ADDRESS_TMPL_H__
