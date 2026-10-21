// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include <cstddef>
#include <cstdint>
#include <string>

#include "net/line_reader.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
	std::string line;
	bool dropping = false;
	size_t lines = 0, long_lines = 0;
	// the limit comes from the first byte, so the fuzzer reaches every branch
	const size_t limit = size ? 1 + data[0] % 64 : 16;
	idni::feed_lines(line, dropping, limit,
		reinterpret_cast<const char*>(data), size,
		[&](std::string& l) { ++lines; (void)l; },
		[&] { ++long_lines; });
	return 0;
}
