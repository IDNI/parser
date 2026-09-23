// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <string>
#include <vector>

#include "syntax_highlighter.h"

namespace em = emscripten;

using highlighter_type = idni::syntax_highlighter;

static std::vector<std::string> get_token_types() {
	return highlighter_type::get_token_types();
}
static std::vector<std::string> get_token_modifiers() {
	return highlighter_type::get_token_modifiers();
}

EMSCRIPTEN_BINDINGS(syntax_highlighter) {
	em::class_<highlighter_type>("syntax_highlighter")
		.constructor<const std::string&>()
		.function("good",         &highlighter_type::good)
		.function("diagnostics",  &highlighter_type::diagnostics)
		.function("get_tokens",   &highlighter_type::get_tokens)
		.function("nt_count",     &highlighter_type::nt_count)
		.function("get_nt_name",  &highlighter_type::get_nt_name)
		.function("get_nt_type",  &highlighter_type::get_nt_type)
		;

	em::function("get_token_types",     &get_token_types);
	em::function("get_token_modifiers", &get_token_modifiers);

	em::register_vector<uint32_t>("vector<uint32_t>");
	em::register_vector<std::string>("vector<string>");
}
