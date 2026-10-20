// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "utility/filesystem.h"

#include <filesystem>
#include <string>
#include <system_error>

using namespace idni;

namespace {

namespace sfs = std::filesystem;

// A fresh folder of the test, removed again when the case ends.
struct temp_folder {
	sfs::path dir;
	explicit temp_folder(const std::string& name) {
		std::error_code ec;
		dir = sfs::current_path(ec) / name;
		sfs::remove_all(dir, ec);
		sfs::create_directory(dir, ec);
	}
	~temp_folder() { std::error_code ec; sfs::remove_all(dir, ec); }
	fs::path file(const std::string& name) const {
		return fs::to_path((dir / name).string());
	}
};

std::string read_text(const fs::path& p) {
	auto r = fs::read(p);
	REQUIRE(r.has_value());
	return r.value();
}

// True when the report holds an io_error node.
bool has_io_error(const diagnostics::report& r) {
	for (const auto& n : r.nodes())
		if (n.tag == diagnostics::code::io_error) return true;
	return false;
}

} // namespace

TEST_SUITE("fs::read and fs::open_stream") {

TEST_CASE("read gives back each byte that write wrote") {
	temp_folder t("fs_read_bytes");
	const fs::path p = t.file("data.bin");
	// more than one read buffer, with a NUL and a byte 0xFF
	std::string content(3 * (1 << 16) + 5, 'a');
	content[7] = '\0';
	content[(1 << 16) + 1] = '\xFF';
	REQUIRE(fs::write(p, content).has_value());
	CHECK(read_text(p) == content);
}

TEST_CASE("read of an empty file gives an empty text") {
	temp_folder t("fs_read_empty");
	const fs::path p = t.file("empty.txt");
	REQUIRE(fs::write(p, "").has_value());
	CHECK(read_text(p).empty());
}

TEST_CASE("read of a missing file is an io_error") {
	temp_folder t("fs_read_missing");
	auto r = fs::read(t.file("missing.txt"));
	CHECK_FALSE(r.has_value());
	CHECK(has_io_error(r.report()));
}

TEST_CASE("open_stream opens a file in the given mode") {
	temp_folder t("fs_open_stream");
	const fs::path p = t.file("text.txt");
	std::FILE* w = fs::open_stream(p, "wb");
	REQUIRE(w != nullptr);
	CHECK(std::fputs("one line\n", w) >= 0);
	CHECK(std::fclose(w) == 0);
	CHECK(read_text(p) == "one line\n");
	CHECK(fs::open_stream(t.file("no_folder/x.txt"), "rb") == nullptr);
}

} // TEST_SUITE
