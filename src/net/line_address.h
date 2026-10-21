// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__NET__LINE_ADDRESS_H__
#define __IDNI__PARSER__NET__LINE_ADDRESS_H__

#include <cstdint>
#include <string>
#include <string_view>

#include "utility/diagnostics.h"

namespace idni {

/// The three transports of the line server and client.
enum class line_transport_kind {
	/// A Unix domain socket, or a named pipe on Windows. The default,
	/// because the server can check the peer user.
	local_socket,
	/// TCP, by default on 127.0.0.1. Only the token checks a client.
	tcp,
	/// WebSocket, by default on 127.0.0.1, for a browser. One request in
	/// each text message.
	websocket,
};

/// One address that the server listens on.
struct listen_address {
	line_transport_kind kind = line_transport_kind::local_socket;
	/// local_socket: the socket file path, created with mode 0600. On
	/// Windows the pipe name, for example `\\.\pipe\<name>`. A path over
	/// the system limit (about 104 to 108 bytes) is an error that names it.
	std::string path{};
	/// tcp and websocket. 0 lets the system pick a port.
	uint16_t port = 0;
	/// websocket: the HTTP path of the upgrade request, for example "/".
	std::string ws_path = "/";
	/// tcp and websocket: the IPv4 or IPv6 address to bind, with no
	/// brackets. An address other than the loopback opens the server to
	/// the network, so the caller chooses it.
	std::string host = "127.0.0.1";
};

/// The address of a server, for a client.
struct connect_target {
	line_transport_kind kind = line_transport_kind::local_socket;
	/// local_socket: the socket path or the pipe name.
	std::string path{};
	/// tcp and websocket: host name or IP address, without brackets.
	std::string host{};
	uint16_t port = 0;
	/// websocket: the HTTP path.
	std::string ws_path = "/";
};

/// The address rules of one program.
struct connect_address_rules {
	/// Scheme without "://", for example "tgf".
	std::string_view scheme;
	/// The socket path with no "~" left, for a target with no address.
	std::string default_socket_path;
	std::string_view default_host;
	uint16_t default_port;
};

/// Parse a connect argument:
///   "<scheme>+unix://<path>" or a path that starts with "/" or "~",
///   and on Windows a pipe name that starts with two backslashes
///   "<scheme>://<host>[:<port>][/]", "<host>[:<port>]", "[<ipv6>][:<port>]"
///   "ws://<host>[:<port>][/<path>]"
/// An empty text gives the default socket. Other forms are an error.
diagnostics::result<connect_target> parse_connect_address(
	std::string_view text, const connect_address_rules& rules);

} // namespace idni

#include "line_address.tmpl.h"

#endif // __IDNI__PARSER__NET__LINE_ADDRESS_H__
