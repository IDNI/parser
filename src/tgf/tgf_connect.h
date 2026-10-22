// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__TGF__TGF_CONNECT_H__
#define __IDNI__PARSER__TGF__TGF_CONNECT_H__

#include <string>
#include <string_view>

#include "net/line_address.h"
#include "net/line_client.h"
#include "repl_client_evaluator.h"
#include "tgf_cli_options.h"
#include "utility/diagnostics.h"
#include "utility/options.h"

namespace idni {

/// The address rules of tgf: the scheme tgf, no default socket, so
/// an empty address is an error.
inline connect_address_rules tgf_address_rules() {
	return { "tgf", "", tgf_default_host, tgf_default_port };
}

/// Parse a `tgf connect` argument with @ref tgf_address_rules. A bad
/// address is an error that names @p iri.
diagnostics::result<connect_target> parse_connect_iri(std::string_view iri);

/// The client settings of tgf: tcp on the default host and port.
inline connect_options tgf_default_client() {
	connect_options c;
	c.target = { line_transport_kind::tcp, "", tgf_default_host,
		tgf_default_port };
	return c;
}

/// Options of the `tgf connect` subcommand. The server binds 127.0.0.1
/// only; port forwarding or a tunnel can make another host reach it.
struct tgf_connect_options {
	/// The server, the first line, json, colors and legacy_repl. The
	/// member "session" of open_fields attaches to that session.
	connect_options client = tgf_default_client();
	/// A grammar file the client reads and loads into the session after
	/// the connect. Empty loads nothing.
	std::string grammar_file{};
	/// Show the status part of the prompt. The print options belong to
	/// the session on the server.
	bool status = true;
};

/// Run `tgf connect`: connect, load the grammar file of the options, then
/// run the text REPL or, with `json`, the JSON loop on stdin with a client
/// evaluator. Returns when the client quits; a failed load returns 1. The
/// returned report carries every error.
diagnostics::result<int> tgf_connect_run(const tgf_connect_options& opt);

/// Run `tgf connect` from the parsed command line @p args and the options:
/// the server comes from the one input of @p args or from --host and
/// --port. @p grammar_file is loaded after the connect when it is not
/// empty. Returns the exit code.
int tgf_connect_command(const parsed_args& args,
	const std::string& grammar_file);

} // namespace idni

#endif // __IDNI__PARSER__TGF__TGF_CONNECT_H__
