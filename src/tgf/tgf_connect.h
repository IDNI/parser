// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__TGF__TGF_CONNECT_H__
#define __IDNI__PARSER__TGF__TGF_CONNECT_H__

#include <cstdint>
#include <memory>
#include <ostream>
#include <set>
#include <string>

#include "tgf_cli.h"
#include "utility/diagnostics.h"

namespace idni {

/// Options of the `tgf connect` subcommand. The client never sees a grammar
/// file: the session of the server holds the grammar.
struct connect_options {
	std::string host = "127.0.0.1";
	uint16_t port = 0;
	/// Attach to this session instead of starting a new one. Empty starts
	/// a new session.
	std::string session{};
	/// True uses the legacy terminal REPL instead of FTXUI.
	bool legacy_repl = false;
};

struct tgf_remote_client;

/// The evaluator of `repl<evaluator_t>` and `repl_ftxui<evaluator_t>` that
/// runs each command in a server session and renders the answer as text.
struct tgf_remote_evaluator {
	explicit tgf_remote_evaluator(std::ostream& out, bool colors = false);
	~tgf_remote_evaluator();

	tgf_remote_evaluator(const tgf_remote_evaluator&) = delete;
	tgf_remote_evaluator& operator=(const tgf_remote_evaluator&) = delete;

	// The REPL front ends link themselves here.
	repl<tgf_remote_evaluator>* r = nullptr;
#ifdef TAU_PARSER_HAS_FTXUI
	repl_ftxui<tgf_remote_evaluator>* r_ftx = nullptr;
#endif

	std::ostream& out;

	/// Connect to @p host:@p port and pick a session. A non-empty
	/// @p session attaches to it; an empty one starts a new session. On
	/// failure the error is in the report of take_report().
	bool connect(const std::string& host, uint16_t port,
		const std::string& session);

	/// The id of the session, for the attach-again message.
	const std::string& session_id() const noexcept { return session_id_; }

	/// Turn terminal colors on or off for the rendered text.
	void set_colors(bool on) { TC.set(on); }

	diagnostics::result<int> eval(const std::string& src);
	void reprompt();

	/// Move the accumulated report out and clear it.
	diagnostics::report take_report();

private:
	std::unique_ptr<tgf_remote_client> client;
	std::string session_id_;
	term::colors TC;
	bool state_grammar_set = false;
	std::string state_grammar;
	bool state_start_set = false;
	std::string state_start;
	/// Character class nonterminals of the session grammar, from state.
	std::set<size_t> state_char_classes;
	diagnostics::report report;

	void apply_state(const format::json::value& state);
	/// Render one response object as text and return its command status.
	/// @p fallback_cmd names the command when the response carries none.
	cmd_status render_response(const format::json::value& resp,
		const std::string& fallback_cmd);
	// Send one structured request and render the answer. 0 continues, 1 is
	// quit and 2 is incomplete.
	int send_statement(format::json::value req);
};

/// Run `tgf connect`: connect, run the text REPL with a remote evaluator and
/// return when the client quits. The returned report carries every error.
diagnostics::result<int> tgf_connect_run(const connect_options& opt);

} // namespace idni

#endif // __IDNI__PARSER__TGF__TGF_CONNECT_H__
