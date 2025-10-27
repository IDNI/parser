// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.txt

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "parser.h"

using namespace std;
using emscripten::optional_override;

EMSCRIPTEN_BINDINGS(tauparser) {
	emscripten::function("version", optional_override([]() -> string {
		return string("tauparser-js " + string(idni::tauparser::version));
	}));
	
	emscripten::function("test", optional_override([]() -> bool {
		return true;
	}));
};
