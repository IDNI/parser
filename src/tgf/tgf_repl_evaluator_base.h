// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__TGF__TGF_REPL_EVALUATOR_BASE_H__
#define __IDNI__PARSER__TGF__TGF_REPL_EVALUATOR_BASE_H__

#include <string>

#include "utility/diagnostics.h"
#include "utility/repl_evaluator_base.h"
#include "utility/repl_json.h"
#include "format/json/json.h"
#include "tgf_repl_parser.generated.h"

namespace idni {

/// The names of the tgf JSON protocol: the fields of a request, of an
/// answer and of its data, and the requests that are not REPL commands.
struct tgf_protocol : repl_json_keys {
	static constexpr const char* session = "session";
	static constexpr const char* input = "input";
	static constexpr const char* file = "file";
	static constexpr const char* name = "name";
	static constexpr const char* source = "source";
	static constexpr const char* option = "option";
	static constexpr const char* options = "options";
	static constexpr const char* text = "text";
	static constexpr const char* value = "value";
	static constexpr const char* symbol = "symbol";
	static constexpr const char* command = "command";
	static constexpr const char* init = "init";
	static constexpr const char* max_line = "max_line";
	static constexpr const char* grammar = "grammar";
	static constexpr const char* start = "start";
	static constexpr const char* char_classes = "char_classes";
	static constexpr const char* fixed_grammar = "fixed_grammar";
	static constexpr const char* license = "license";
	static constexpr const char* loaded = "loaded";
	static constexpr const char* changed = "changed";
	static constexpr const char* productions = "productions";
	static constexpr const char* production_ids = "production_ids";
	static constexpr const char* internal_grammar = "internal_grammar";
	static constexpr const char* index = "index";
	static constexpr const char* head = "head";
	static constexpr const char* body = "body";
	static constexpr const char* guard = "guard";
	static constexpr const char* conjunctive = "conjunctive";
	static constexpr const char* terminals = "terminals";
	static constexpr const char* tml_rules = "tml_rules";
	static constexpr const char* tml_facts = "tml_facts";
	static constexpr const char* tree = "tree";
	static constexpr const char* ambiguous = "ambiguous";
	static constexpr const char* trees = "trees";
	static constexpr const char* nodes = "nodes";
	static constexpr const char* alternatives = "alternatives";
	static constexpr const char* children = "children";
	static constexpr const char* range = "range";
	// requests, and hello also names the field of the hello answer
	static constexpr const char* new_session = "new";
	static constexpr const char* attach = "attach";
	static constexpr const char* end_session = "end-session";
};

/// The options of a REPL evaluator, local or remote.
struct tgf_repl_options : repl_base_options {
	bool print_input        = false;
	bool print_terminals    = true;
	bool print_graphs       = true;
	bool print_ambiguity    = true;
	bool tml_rules          = false;
	bool tml_facts          = false;
	/// True when the evaluator refuses commands that read a file path.
	bool no_server_paths    = false;
};

/// The part of a tgf REPL evaluator that does not depend on where a
/// statement runs: the REPL language, the prompt and the text of an answer.
/// repl<> and repl_ftxui<> run a derived class.
struct tgf_repl_evaluator_base : repl_evaluator_base<tgf_repl_parser,
	tgf_repl_parser::statement, tgf_repl_options>
{
	explicit tgf_repl_evaluator_base(options o);

	/// The option name of a get, set, add, delete or toggle statement.
	static std::string option_name(const trv& s);
	/// The option value text of a set, add or delete statement.
	static std::string option_text(const trv& s);
	/// Decode the quoted TGF string @p s. A text without quotes stays as
	/// it is. A bad escape goes to @p rep and gives an empty text.
	static std::string unquote(const std::string& s,
		diagnostics::report& rep);

protected:
	/// The {"grammar","start","char_classes"} state of the session.
	virtual tref state() const = 0;
	/// The status of state() and "tgf>".
	std::string prompt_text() const override;
};

} // namespace idni

#endif // __IDNI__PARSER__TGF__TGF_REPL_EVALUATOR_BASE_H__
