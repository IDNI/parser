// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__NET__LINE_TRANSPORT_H__
#define __IDNI__PARSER__NET__LINE_TRANSPORT_H__

#include <cstddef>
#include <memory>
#include <string>

#include "net/line_address.h"
#include "utility/diagnostics.h"

namespace idni {

/// Why read_line gave no line.
enum class read_status { ok, closed, timeout, too_long };

/// Limits of a line connection.
struct line_limits {
	/// Longest line or WebSocket message that read_line takes. 0 is no limit.
	size_t max_line = 16u * 1024 * 1024;
	/// Read and write timeout. 0 waits without end.
	size_t timeout_seconds = 0;
	/// The time of the name lookup, the connect and the WebSocket
	/// handshake. 0 waits without end.
	size_t connect_timeout_seconds = 30;
	/// The time that a WebSocket close waits for the close frame of the
	/// peer.
	size_t close_timeout_seconds = 5;
};

/// One line connection on any transport. A line is UTF-8 text. On a
/// socket or TCP it ends with one '\n'. On WebSocket it is one text
/// message.
struct line_transport {
	virtual ~line_transport() = default;
	/// Send @p line. False when the connection is gone.
	virtual bool send_line(const std::string& line) = 0;
	/// Read one line without its line end. A trailing carriage return is dropped.
	/// ok gives a line. Each other value names the one cause.
	virtual read_status read_line(std::string& line) = 0;
	virtual void close() = 0;
	/// Take new limits for the next reads and writes. A buffered part of
	/// the next line stays.
	virtual void set_limits(const line_limits& limits) = 0;
};

/// Open a connection to @p target. For tcp and websocket it tries each
/// address of the host. A transport clears its read buffer at close.
diagnostics::result<std::unique_ptr<line_transport>> line_connect(
	const connect_target& target, const line_limits& limits = {});

} // namespace idni

#endif // __IDNI__PARSER__NET__LINE_TRANSPORT_H__
