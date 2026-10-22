// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__TGF__REPL_CLIENT_EVALUATOR_H__
#define __IDNI__PARSER__TGF__REPL_CLIENT_EVALUATOR_H__

#include <istream>
#include <memory>
#include <ostream>
#include <string>

#include "net/line_client.h"
#include "net/line_transport.h"
#include "tgf_repl_evaluator_base.h"
#include "utility/diagnostics.h"
#include "format/json/json.h"

namespace idni {

/// The evaluator of repl<> and repl_ftxui<> for a client of tgf serve. It
/// parses each REPL line, runs each statement in a server session and
/// writes the answer as text, or as JSON lines with the json option.
struct repl_client_evaluator : tgf_repl_evaluator_base {
	repl_client_evaluator(std::ostream& out, options o);

	/// Connect to @p client.target and start a new session, or attach to
	/// the session that the member "session" of client.open_fields names.
	/// An attach first prints the buffered answers. On failure the error
	/// is in the report of take_report().
	bool open_session(const connect_options& client);
	/// The id of the session, for the attach-again message.
	const std::string& session_id() const noexcept;
	/// Read the grammar file @p path and load its text into the session.
	/// The answer prints like the answer of `load`.
	cmd_status load_grammar(const std::string& path);
	/// Send each line of @p in as one request and write each answer as one
	/// line. Returns at the end of @p in or after a quit answer.
	int run_json(std::istream& in);

private:
	tref state() const override;
	cmd_result run_statement(const trv& s) override;

	/// Send one request and print its answer. @p gone is set when the
	/// connection fails.
	cmd_status request(tref req, bool& gone);
	/// Render one response object as text and return its command status.
	/// @p fallback_cmd names the command when the response carries none.
	cmd_status render_response(tref resp, const std::string& fallback_cmd);
	/// One structured JSON request for one parsed REPL statement. A bad
	/// escape in a quoted text goes to @p rep.
	static tref request_of(const trv& statement, diagnostics::report& rep);

	std::ostream& out;
	std::unique_ptr<line_transport> link;
	std::string session_id_;
	htref state_ = format::json::tree::geth(
		format::json::tree::build_object({}));
};

} // namespace idni

#endif // __IDNI__PARSER__TGF__REPL_CLIENT_EVALUATOR_H__
