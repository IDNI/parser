// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include "line_client.h"

#include <string>
#include <string_view>
#include <utility>

#include "parser_strings.h"

namespace idni {

namespace {

namespace json = format::json;

} // namespace

diagnostics::result<std::unique_ptr<line_transport>> line_client_open(
	const connect_options& opt, std::string_view cmd, htref& hello)
{
	using messages = parser_strings::messages;
	// A hello can be large, so the first answer has no line limit.
	line_limits first = opt.limits;
	first.max_line = 0;
	first.timeout_seconds = opt.limits.connect_timeout_seconds;
	auto connected = line_connect(opt.target, first);
	if (!connected.has_value()) return connected;
	diagnostics::result<std::unique_ptr<line_transport>> r;
	std::unique_ptr<line_transport> link = std::move(connected.value());
	r.merge(std::move(connected));

	std::string line = "{\"cmd\":"
		+ json::tree::get(json::tree::build_string(cmd)).to_str();
	const std::string fields = json::tree::get(opt.open_fields).to_str();
	if (fields.size() > 2) {
		line += ',';
		line.append(fields, 1, fields.size() - 2);
	}
	line += '}';
	const bool sent = link->send_line(line);

	std::string answer;
	const read_status got = sent ? link->read_line(answer)
		: read_status::closed;
	if (got != read_status::ok) {
		link->close();
		if (got == read_status::timeout)
			return r.with_error(diagnostics::code::io_error,
				messages::net_read_timeout);
		if (got == read_status::too_long)
			return r.with_error(diagnostics::code::out_of_range,
				messages::net_line_too_long);
		return r.with_error(diagnostics::code::io_error,
			messages::net_connection_lost);
	}
	auto parsed = json::tree::get(std::string_view(answer));
	if (!parsed.has_value()) {
		link->close();
		r.merge(std::move(parsed));
		return r;
	}
	hello = json::tree::geth(parsed.value());
	link->set_limits(opt.limits);
	return r.with_value(std::move(link));
}

} // namespace idni
