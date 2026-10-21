// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "quiet_doctest.h"
#include "utility/filesystem.h"

#include <atomic>
#include <filesystem>
#include <functional>
#include <thread>
#include <sstream>
#include <string>
#include <system_error>

#ifndef _WIN32
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

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

// True when the folder of @p p holds a temp file of fs::replace.
bool temp_left(const fs::path& p) {
	std::error_code ec;
	const sfs::path dir = sfs::path(p).parent_path();
	if (!sfs::is_directory(dir, ec)) return false;
	for (const auto& e : sfs::directory_iterator(dir, ec))
		if (e.path().filename().string().find(".tmp.")
				!= std::string::npos)
			return true;
	return false;
}

// True when the report holds an io_error node.
bool has_io_error(const diagnostics::report& r) {
	for (const auto& n : r.nodes())
		if (n.tag == diagnostics::code::io_error) return true;
	return false;
}

#ifndef _WIN32
// The permission bits of @p p.
mode_t mode_of(const fs::path& p) {
	struct stat st{};
	REQUIRE(::stat(p.c_str(), &st) == 0);
	return st.st_mode & 0777;
}
#endif

} // namespace

TEST_SUITE("fs::replace") {

TEST_CASE("replaces an existing file") {
	temp_folder t("doctest_filesystem_existing.tmp");
	const fs::path p = t.file("data.txt");
	REQUIRE(fs::write(p, "old content").has_value());
	auto r = fs::replace(p, "new content");
	REQUIRE(r.has_value());
	CHECK(read_text(p) == "new content");
	CHECK_FALSE(temp_left(p));
}

TEST_CASE("creates a missing file") {
	temp_folder t("doctest_filesystem_missing.tmp");
	const fs::path p = t.file("data.txt");
	REQUIRE_FALSE(fs::exists(p));
	auto r = fs::replace(p, "first content");
	REQUIRE(r.has_value());
	CHECK(read_text(p) == "first content");
	CHECK_FALSE(temp_left(p));
}

TEST_CASE("a missing folder is an error that names the path") {
	temp_folder t("doctest_filesystem_no_folder.tmp");
	const fs::path p = t.file("absent/data.txt");
	auto r = fs::replace(p, "content");
	CHECK_FALSE(r.has_value());
	std::ostringstream os;
	r.report().print(os);
	CHECK(os.str().find("absent") != std::string::npos);
	// a system error number is no exit code of a process
	CHECK(os.str().find("errno=") != std::string::npos);
	CHECK(os.str().find("exit code=") == std::string::npos);
	CHECK_FALSE(temp_left(p));
}

#ifndef _WIN32
TEST_CASE("a new file gets mode 0600") {
	temp_folder t("doctest_filesystem_new_mode.tmp");
	const fs::path p = t.file("data.txt");
	REQUIRE(fs::replace(p, "content").has_value());
	CHECK(mode_of(p) == 0600);
}

TEST_CASE("a file with mode 0644 gets mode 0600") {
	temp_folder t("doctest_filesystem_old_mode.tmp");
	const fs::path p = t.file("data.txt");
	REQUIRE(fs::write(p, "old content").has_value());
	REQUIRE(::chmod(p.c_str(), 0644) == 0);
	REQUIRE(mode_of(p) == 0644);
	REQUIRE(fs::replace(p, "new content").has_value());
	CHECK(mode_of(p) == 0600);
	CHECK(read_text(p) == "new content");
}
#endif

TEST_CASE("a file with a temp-like name stays") {
	temp_folder t("doctest_filesystem_stale.tmp");
	const fs::path p = t.file("data.txt");
	const fs::path own = p + fs::to_path(".tmp");
	REQUIRE(fs::write(own, "a file of the user").has_value());
	REQUIRE(fs::replace(p, "new").has_value());
	CHECK(read_text(p) == "new");
	CHECK(read_text(own) == "a file of the user");
	CHECK_FALSE(temp_left(p));
}

// Windows refuses the rename while the reader holds the file open.
#if !defined(_WIN32) \
	&& (!defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__))
TEST_CASE("two writers never mix their content") {
	temp_folder t("doctest_filesystem_writers.tmp");
	const fs::path p = t.file("data.txt");
	const std::string a(256 * 1024, 'a'), b(256 * 1024, 'b');
	REQUIRE(fs::replace(p, a).has_value());
	std::atomic<bool> failed{ false }, done{ false };
	auto writer = [&](const std::string& content) {
		for (int i = 0; i != 40; ++i)
			if (!fs::replace(p, content).has_value()) failed = true;
	};
	std::thread reader([&] {
		while (!done) {
			auto r = fs::read(p);
			if (r.has_value() && r.value() != a && r.value() != b)
				failed = true;
		}
	});
	std::thread wa(writer, std::cref(a)), wb(writer, std::cref(b));
	wa.join(), wb.join();
	done = true;
	reader.join();
	CHECK_FALSE(failed.load());
	const std::string last = read_text(p);
	CHECK((last == a || last == b));
	CHECK_FALSE(temp_left(p));
}
#endif

#ifndef _WIN32
TEST_CASE("a symlink at the path is replaced and its target stays") {
	temp_folder t("doctest_filesystem_replace_link.tmp");
	const fs::path target = t.file("target.txt");
	const fs::path p = t.file("data.txt");
	REQUIRE(fs::write(target, "target content").has_value());
	REQUIRE(::symlink(target.c_str(), p.c_str()) == 0);
	REQUIRE(fs::replace(p, "new").has_value());
	CHECK(read_text(target) == "target content");
	CHECK(read_text(p) == "new");
	std::error_code ec;
	CHECK_FALSE(sfs::is_symlink(sfs::path(p), ec));
}
#endif

TEST_CASE("an empty content gives an empty file") {
	temp_folder t("doctest_filesystem_empty.tmp");
	const fs::path p = t.file("data.txt");
	REQUIRE(fs::write(p, "old content").has_value());
	REQUIRE(fs::replace(p, "").has_value());
	CHECK(fs::exists(p));
	CHECK(read_text(p).empty());
}

TEST_CASE("a content of 3 MiB and 7 bytes comes back the same") {
	temp_folder t("doctest_filesystem_large.tmp");
	const fs::path p = t.file("data.bin");
	std::string content(3u * 1024 * 1024 + 7, '\0');
	for (size_t i = 0; i != content.size(); ++i)
		content[i] = static_cast<char>((i * 131 + 7) % 251);
	REQUIRE(fs::replace(p, content).has_value());
	CHECK(read_text(p) == content);
	CHECK_FALSE(temp_left(p));
}

TEST_CASE("a folder as the target is an io_error and the folder stays") {
	temp_folder t("doctest_filesystem_folder.tmp");
	const sfs::path dir = t.dir / "target";
	std::error_code ec;
	REQUIRE(sfs::create_directory(dir, ec));
	const fs::path p = fs::to_path(dir.string());
	auto r = fs::replace(p, "content");
	CHECK_FALSE(r.has_value());
	CHECK(has_io_error(r.report()));
	CHECK(sfs::is_directory(dir, ec));
	CHECK_FALSE(temp_left(p));
}

TEST_CASE("the error of a missing folder is an io_error") {
	temp_folder t("doctest_filesystem_no_folder_code.tmp");
	const fs::path p = t.file("absent/data.txt");
	auto r = fs::replace(p, "content");
	CHECK_FALSE(r.has_value());
	CHECK(has_io_error(r.report()));
}

}

TEST_SUITE("fs::create_private_dirs") {
#ifndef _WIN32
TEST_CASE("each new part of the path gets mode 0700") {
	temp_folder t("doctest_filesystem_private_dirs.tmp");
	const fs::path outer = t.file("a");
	const fs::path inner = t.file("a/b");
	auto r = fs::create_private_dirs(inner);
	REQUIRE(r.has_value());
	CHECK(mode_of(outer) == 0700);
	CHECK(mode_of(inner) == 0700);
}

TEST_CASE("an existing folder keeps its mode") {
	temp_folder t("doctest_filesystem_kept_dir.tmp");
	const fs::path outer = t.file("a");
	REQUIRE(fs::create_private_dirs(outer).has_value());
	REQUIRE(::chmod(outer.c_str(), 0755) == 0);
	const fs::path inner = t.file("a/b");
	REQUIRE(fs::create_private_dirs(inner).has_value());
	CHECK(mode_of(outer) == 0755);
	CHECK(mode_of(inner) == 0700);
}
#endif

TEST_CASE("a file in the way is an error that names the path") {
	temp_folder t("doctest_filesystem_dir_blocked.tmp");
	REQUIRE(fs::write(t.file("plain"), "x").has_value());
	const fs::path p = t.file("plain/sub");
	auto r = fs::create_private_dirs(p);
	CHECK_FALSE(r.has_value());
	CHECK(has_io_error(r.report()));
	std::ostringstream os;
	r.report().print(os);
	CHECK(os.str().find("plain") != std::string::npos);
}
}

TEST_SUITE("fs::open_private_append") {
#ifndef _WIN32
TEST_CASE("a new file gets mode 0600") {
	temp_folder t("doctest_filesystem_append_new.tmp");
	const fs::path p = t.file("log.txt");
	auto r = fs::open_private_append(p);
	REQUIRE(r.has_value());
	CHECK(mode_of(p) == 0600);
}

TEST_CASE("a file with mode 0644 gets mode 0600 and keeps its lines") {
	temp_folder t("doctest_filesystem_append_old.tmp");
	const fs::path p = t.file("log.txt");
	REQUIRE(fs::write(p, "first\n").has_value());
	REQUIRE(::chmod(p.c_str(), 0644) == 0);
	{
		auto r = fs::open_private_append(p);
		REQUIRE(r.has_value());
		fs::file_ostream os = std::move(r).value();
		os << "second\n";
	}
	CHECK(mode_of(p) == 0600);
	CHECK(read_text(p) == "first\nsecond\n");
}

TEST_CASE("a symlink at the path is an error and its target stays") {
	temp_folder t("doctest_filesystem_append_link.tmp");
	const fs::path target = t.file("target.txt");
	const fs::path p = t.file("log.txt");
	REQUIRE(fs::write(target, "first\n").has_value());
	REQUIRE(::chmod(target.c_str(), 0644) == 0);
	REQUIRE(::symlink(target.c_str(), p.c_str()) == 0);
	auto r = fs::open_private_append(p);
	CHECK_FALSE(r.has_value());
	CHECK(has_io_error(r.report()));
	CHECK(mode_of(target) == 0644);
	CHECK(read_text(target) == "first\n");
}

TEST_CASE("two streams append to the same file") {
	temp_folder t("doctest_filesystem_append_two.tmp");
	const fs::path p = t.file("log.txt");
	{
		auto first = fs::open_private_append(p);
		auto second = fs::open_private_append(p);
		REQUIRE(first.has_value());
		REQUIRE(second.has_value());
		fs::file_ostream a = std::move(first).value();
		fs::file_ostream b = std::move(second).value();
		a << "one\n" << std::flush;
		b << "two\n" << std::flush;
		a << "three\n" << std::flush;
	}
	CHECK(read_text(p) == "one\ntwo\nthree\n");
}
#endif

TEST_CASE("a missing folder is an error that names the path") {
	temp_folder t("doctest_filesystem_append_missing.tmp");
	const fs::path p = t.file("absent/log.txt");
	auto r = fs::open_private_append(p);
	CHECK_FALSE(r.has_value());
	CHECK(has_io_error(r.report()));
	std::ostringstream os;
	r.report().print(os);
	CHECK(os.str().find("absent") != std::string::npos);
}

TEST_CASE("a child program does not inherit the stream") {
	temp_folder t("doctest_filesystem_append_inherit.tmp");
	const fs::path p = t.file("log.txt");
	auto r = fs::open_private_append(p);
	REQUIRE(r.has_value());
	const fs::file_ostream os = std::move(r).value();
	REQUIRE(os.is_open());
#ifdef _WIN32
	DWORD flags = 0;
	REQUIRE(::GetHandleInformation(os.native_handle(), &flags));
	CHECK((flags & HANDLE_FLAG_INHERIT) == 0);
// Emscripten runs no child program, and its F_GETFD gives 0
#elif !defined(__EMSCRIPTEN__)
	CHECK((::fcntl(os.native_handle(), F_GETFD) & FD_CLOEXEC) != 0);
#endif
}

TEST_CASE("a write error shows in the stream state") {
	temp_folder t("doctest_filesystem_append_error.tmp");
	const fs::path p = t.file("read_only.txt");
	REQUIRE(fs::write(p, "first\n").has_value());
	// a read-only handle refuses each write
	auto opened = fs::open(p, fs::MMAP_READ);
	REQUIRE(opened.has_value());
	fs::file_ostream os(opened.value());
	CHECK(os.good());
	os << "second\n" << std::flush;
	CHECK(os.bad());
	CHECK(read_text(p) == "first\n");
}
}

TEST_SUITE("fs::temp_folder") {
TEST_CASE("two calls make two different empty folders") {
	auto a = fs::temp_folder("doctest_fs");
	auto b = fs::temp_folder("doctest_fs");
	REQUIRE(a.has_value());
	REQUIRE(b.has_value());
	const sfs::path pa(a.value()), pb(b.value());
	CHECK(pa != pb);
	CHECK(sfs::is_directory(pa));
	CHECK(sfs::is_directory(pb));
	CHECK(sfs::is_empty(pa));
	CHECK(pa.filename().string().starts_with("doctest_fs_"));
#ifndef _WIN32
	struct stat st{};
	REQUIRE(::stat(pa.c_str(), &st) == 0);
	CHECK((st.st_mode & 0777) == 0700);
#endif
	std::error_code ec;
	sfs::remove(pa, ec);
	sfs::remove(pb, ec);
}

TEST_CASE("a prefix with a path separator is an invalid_argument") {
	for (const char* p : { "a/b", "a\\b" }) {
		CAPTURE(p);
		auto r = fs::temp_folder(p);
		CHECK_FALSE(r.has_value());
		CHECK(r.report().first_error_code()
			== diagnostics::code::invalid_argument);
	}
}
}

TEST_SUITE("fs::open and fs::create") {
TEST_CASE("an open file closes in a child program") {
	temp_folder t("doctest_filesystem_cloexec.tmp");
	const fs::path p = t.file("data.bin");
	auto created = fs::create(p, fs::MMAP_WRITE);
	REQUIRE(created.has_value());
#ifdef _WIN32
	DWORD flags = 0;
	REQUIRE(::GetHandleInformation(created.value(), &flags));
	CHECK((flags & HANDLE_FLAG_INHERIT) == 0);
// Emscripten runs no child program, and its F_GETFD gives 0
#elif !defined(__EMSCRIPTEN__)
	CHECK((::fcntl(created.value(), F_GETFD) & FD_CLOEXEC) != 0);
#endif
	REQUIRE(fs::close(created.value()).has_value());
	auto opened = fs::open(p, fs::MMAP_READ);
	REQUIRE(opened.has_value());
#ifdef _WIN32
	REQUIRE(::GetHandleInformation(opened.value(), &flags));
	CHECK((flags & HANDLE_FLAG_INHERIT) == 0);
#elif !defined(__EMSCRIPTEN__)
	CHECK((::fcntl(opened.value(), F_GETFD) & FD_CLOEXEC) != 0);
#endif
	REQUIRE(fs::close(opened.value()).has_value());
}
}

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

TEST_SUITE("fs errors name the system error number") {

// The printed report of @p rep, with no colour.
std::string printed(const diagnostics::report& rep) {
	std::ostringstream os;
	rep.print(os);
	return os.str();
}

TEST_CASE("size and unlink of a missing file give errno, not an exit code") {
	temp_folder t("fs_errno_missing");
	const fs::path p = t.file("missing.txt");
	auto sz = fs::size(p);
	REQUIRE_FALSE(sz.has_value());
	CHECK(printed(sz.report()).find("errno=") != std::string::npos);
	CHECK(printed(sz.report()).find("exit code=") == std::string::npos);
	auto rm = fs::unlink(p);
	REQUIRE_FALSE(rm.has_value());
	CHECK(printed(rm.report()).find("errno=") != std::string::npos);
	CHECK(printed(rm.report()).find("exit code=") == std::string::npos);
}

TEST_CASE("open of a missing file gives errno, not an exit code") {
	temp_folder t("fs_errno_open");
	auto f = fs::open(t.file("missing.bin"), fs::MMAP_READ);
	REQUIRE_FALSE(f.has_value());
	CHECK(printed(f.report()).find("errno=") != std::string::npos);
	CHECK(printed(f.report()).find("exit code=") == std::string::npos);
}

} // TEST_SUITE
