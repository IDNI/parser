// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__FILESYSTEM_TMPL_H__
#define __IDNI__PARSER__UTILITY__FILESYSTEM_TMPL_H__

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <system_error>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif

#include "filesystem.h"
#include "characters.h"
#include "../parser_strings.h"

namespace idni::fs {

#ifdef _WIN32
inline path to_path(const std::string& utf8) { return idni::utf8_to_wide(utf8); }
inline std::string to_utf8(const path& p) { return idni::wide_to_utf8(p); }
#else
inline path to_path(const std::string& utf8) { return utf8; }
inline std::string to_utf8(const path& p) { return p; }
#endif

#ifdef _WIN32
inline bool exists(const path& p) {
	return ::GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES;
}
#else
inline bool exists(const path& p) {
	struct stat s;
	return ::stat(p.c_str(), &s) == 0;
}
#endif

#ifdef _WIN32
inline result<size_t> size(const path& p) {
	using label = idni::parser_strings::label;
	result<size_t> r;
	WIN32_FILE_ATTRIBUTE_DATA data;
	if (!::GetFileAttributesExW(p.c_str(), GetFileExInfoStandard, &data))
		return r.with_error(diagnostics::code::io_error,
			"failed to get file size",
			{{label::path, to_utf8(p)},
				{label::exit_code, static_cast<int_t>(::GetLastError())}});
	LARGE_INTEGER sz;
	sz.HighPart = static_cast<LONG>(data.nFileSizeHigh);
	sz.LowPart = data.nFileSizeLow;
	return r.with_value(static_cast<size_t>(sz.QuadPart));
}
#else
inline result<size_t> size(const path& p) {
	using label = idni::parser_strings::label;
	result<size_t> r;
	struct stat s;
	if (::stat(p.c_str(), &s) != 0)
		return r.with_error(diagnostics::code::io_error,
			"failed to get file size",
			{{label::path, p},
				{label::exit_code, static_cast<int_t>(errno)}});
	return r.with_value(static_cast<size_t>(s.st_size));
}
#endif

#ifdef _WIN32
inline result<bool> unlink(const path& p) {
	using label = idni::parser_strings::label;
	result<bool> r;
	if (!::DeleteFileW(p.c_str()))
		return r.with_error(diagnostics::code::io_error,
			"failed to delete file",
			{{label::path, to_utf8(p)},
				{label::exit_code, static_cast<int_t>(::GetLastError())}});
	return r.with_value(true);
}
#else
inline result<bool> unlink(const path& p) {
	using label = idni::parser_strings::label;
	result<bool> r;
	if (::unlink(p.c_str()) != 0)
		return r.with_error(diagnostics::code::io_error,
			"failed to delete file",
			{{label::path, p},
				{label::exit_code, static_cast<int_t>(errno)}});
	return r.with_value(true);
}
#endif

inline FILE* open_stream(const path& p, const char* mode) {
#ifdef _WIN32
	const std::wstring m(mode, mode + std::strlen(mode));
#ifdef _MSC_VER
	FILE* f = nullptr;
	::_wfopen_s(&f, p.c_str(), m.c_str());
	return f;
#else
	return ::_wfopen(p.c_str(), m.c_str());
#endif
#else
	return std::fopen(p.c_str(), mode);
#endif
}

inline result<std::string> read(const path& p) {
	using label = idni::parser_strings::label;
	result<std::string> r;
	FILE* f = open_stream(p, "rb");
	if (!f) {
		return r.with_error(diagnostics::code::io_error,
			idni::parser_strings::messages::cannot_open_file,
			{{label::path, to_utf8(p)}});
	}
	std::string text;
	// a wasm stack holds 64 KB by default, so the buffer goes on the heap
	std::vector<char> buf(1 << 16);
	for (size_t n; (n = std::fread(buf.data(), 1, buf.size(), f)) != 0; )
		text.append(buf.data(), n);
	const bool failed = std::ferror(f) != 0;
	std::fclose(f);
	if (failed) {
		return r.with_error(diagnostics::code::io_error,
			"failed to read the file", {{label::path, to_utf8(p)}});
	}
	return r.with_value(std::move(text));
}

inline result<bool> write(const path& p, std::string_view content) {
	using label = idni::parser_strings::label;
	result<bool> r;
	FILE* f = open_stream(p, "wb");
	if (!f) {
		return r.with_error(diagnostics::code::io_error,
			"failed to open the file for writing",
			{{label::path, to_utf8(p)}});
	}
	size_t written = std::fwrite(content.data(), 1, content.size(), f);
	bool short_write = written != content.size();
#ifdef _WIN32
	// fclose only hands the bytes to the system cache, not to the disk.
	const bool synced = !short_write && std::fflush(f) == 0
		&& ::FlushFileBuffers(reinterpret_cast<HANDLE>(
			::_get_osfhandle(::_fileno(f))));
	const DWORD sync_error = synced ? 0 : ::GetLastError();
#endif
	bool flush_failed = std::fclose(f) != 0;
	int flush_errno = errno;
	if (short_write) {
		return r.with_error(diagnostics::code::io_error,
			"failed to write the file",
			{{label::path, to_utf8(p)}});
	}
#ifdef _WIN32
	if (!synced) {
		return r.with_error(diagnostics::code::io_error,
			"failed to write the file to the disk",
			{{label::path, to_utf8(p)},
				{label::error_number, static_cast<int_t>(sync_error)}});
	}
#endif
	if (flush_failed) {
		return r.with_error(diagnostics::code::io_error,
			"failed to flush the file",
			{{label::path, to_utf8(p)},
				{label::exit_code, static_cast<int_t>(flush_errno)}});
	}
	return r.with_value(true);
}

// One definition for every translation unit, so no unnamed namespace.
namespace detail {

// The temp file of fs::replace is <path><temp_infix><unique part>.
inline constexpr std::string_view temp_infix = ".tmp.";

#ifndef _WIN32
// The whole of @p text, also after a short write or a signal.
inline bool write_all(int fd, std::string_view text) {
	while (!text.empty()) {
		const ssize_t n = ::write(fd, text.data(), text.size());
		if (n < 0 && errno == EINTR) continue;
		if (n <= 0) return false;
		text.remove_prefix(static_cast<size_t>(n));
	}
	return true;
}

inline bool sync_fd(int fd) {
#ifdef F_FULLFSYNC
	// fsync on macOS leaves the bytes in the disk cache. A file system
	// without F_FULLFSYNC falls back to fsync.
	if (::fcntl(fd, F_FULLFSYNC) == 0) return true;
#endif
	int rc;
	do rc = ::fsync(fd); while (rc != 0 && errno == EINTR);
	return rc == 0;
}

// A new temp file with a unique name next to @p p, mode 0600 from its
// open. O_EXCL never opens an existing file or follows a symlink, so
// another writer or a planted link cannot take it.
inline bool write_private_temp(const path& p, std::string_view text,
	path& temp)
{
	std::vector<char> name(p.begin(), p.end());
	name.insert(name.end(), temp_infix.begin(), temp_infix.end());
	for (char c : std::string_view("XXXXXX")) name.push_back(c);
	name.push_back('\0');
	const int fd = ::mkstemp(name.data());
	if (fd < 0) return false;
	temp.assign(name.data());
	::fcntl(fd, F_SETFD, FD_CLOEXEC);
	const bool ok = write_all(fd, text) && sync_fd(fd);
	const int saved = errno;
	const bool closed = ::close(fd) == 0;
	if (!ok) errno = saved;
	return closed && ok;
}

// A rename is durable only after the folder entry reaches the disk.
inline bool sync_folder(const path& dir) {
	const int fd = ::open(dir.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
	if (fd < 0) return false;
	const bool ok = sync_fd(fd);
	const int saved = errno;
	const bool closed = ::close(fd) == 0;
	if (!ok) errno = saved;
	return closed && ok;
}
#else
// A new temp file with a unique name next to @p p, with the whole of
// @p text flushed to the disk before the handle closes. CREATE_NEW never
// opens a file of another writer.
inline bool write_flushed_temp(const path& p, std::string_view text,
	path& temp)
{
	static std::atomic<unsigned> counter{ 0 };
	HANDLE h = INVALID_HANDLE_VALUE;
	for (int attempt = 0; attempt != 100; ++attempt) {
		wchar_t unique[40];
		::swprintf(unique, 40, L"%lx.%x.%llx", ::GetCurrentProcessId(),
			counter.fetch_add(1),
			static_cast<unsigned long long>(::GetTickCount64()));
		temp = p + path(temp_infix.begin(), temp_infix.end()) + unique;
		h = ::CreateFileW(temp.c_str(), GENERIC_WRITE, 0, nullptr,
			CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (h != INVALID_HANDLE_VALUE
				|| ::GetLastError() != ERROR_FILE_EXISTS)
			break;
	}
	if (h == INVALID_HANDLE_VALUE) { temp.clear(); return false; }
	bool ok = true;
	while (ok && !text.empty()) {
		const DWORD chunk = static_cast<DWORD>(
			std::min<size_t>(text.size(), 1u << 30));
		DWORD n = 0;
		ok = ::WriteFile(h, text.data(), chunk, &n, nullptr) && n != 0;
		if (ok) text.remove_prefix(n);
	}
	ok = ok && ::FlushFileBuffers(h);
	const DWORD saved = ::GetLastError();
	const bool closed = ::CloseHandle(h) != 0;
	if (!ok) ::SetLastError(saved);
	return closed && ok;
}
#endif

} // namespace detail

inline result<bool> replace(const path& p, std::string_view content) {
	using label = idni::parser_strings::label;
	result<bool> r;
	path temp;
#ifdef _WIN32
	if (!detail::write_flushed_temp(p, content, temp)) {
		const DWORD err = ::GetLastError();
		if (!temp.empty() && exists(temp)) r.merge(unlink(temp));
		return r.with_error(diagnostics::code::io_error,
			"failed to write the temp file to the disk",
			{{label::path, to_utf8(temp.empty() ? p : temp)},
				{label::error_number, static_cast<int_t>(err)}});
	}
	if (!::MoveFileExW(temp.c_str(), p.c_str(),
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
	{
		const DWORD err = ::GetLastError();
		if (exists(temp)) r.merge(unlink(temp));
		return r.with_error(diagnostics::code::io_error,
			"failed to replace the file",
			{{label::path, to_utf8(p)},
				{label::error_number, static_cast<int_t>(err)}});
	}
#else
	if (!detail::write_private_temp(p, content, temp)) {
		const int err = errno;
		if (!temp.empty() && exists(temp)) r.merge(unlink(temp));
		return r.with_error(diagnostics::code::io_error,
			"failed to write the temp file to the disk",
			{{label::path, to_utf8(temp.empty() ? p : temp)},
				{label::error_number, static_cast<int_t>(err)}});
	}
	std::error_code ec;
	std::filesystem::rename(std::filesystem::path(temp),
		std::filesystem::path(p), ec);
	if (ec) {
		if (exists(temp)) r.merge(unlink(temp));
		return r.with_error(diagnostics::code::io_error,
			"failed to replace the file",
			{{label::path, to_utf8(p)},
				{label::error_number, static_cast<int_t>(ec.value())}});
	}
	// The new content is in place. Only the durability of the rename failed.
	std::string dir = std::filesystem::path(p).parent_path().string();
	if (dir.empty()) dir = ".";
	if (!detail::sync_folder(dir))
		return r.with_error(diagnostics::code::io_error,
			"failed to write the folder of the file to the disk",
			{{label::path, dir},
				{label::error_number, static_cast<int_t>(errno)}});
#endif
	return r.with_value(true);
}

inline result<path> temp_dir() {
	result<path> r;
	std::error_code ec;
	auto d = std::filesystem::temp_directory_path(ec);
#ifdef _WIN32
	auto is_dir = [](const path& p) {
		DWORD attr = ::GetFileAttributesW(p.c_str());
		return attr != INVALID_FILE_ATTRIBUTES
			&& (attr & FILE_ATTRIBUTE_DIRECTORY);
	};
	if (!ec && !d.empty() && is_dir(d.native()))
		return r.with_value(d.native());

	using label = idni::parser_strings::label;
	wchar_t dir[MAX_PATH];
	DWORD dr = ::GetTempPathW(MAX_PATH, dir);
	if (dr > MAX_PATH)
		return r.with_error(diagnostics::code::io_error,
			"the temporary directory path is too long",
			{{label::size, static_cast<size_t>(dr)},
				{label::limit, static_cast<size_t>(MAX_PATH)}});
	if (dr > 0 && is_dir(path(dir)))
		return r.with_value(path(dir));

	// Git bash scripts that name a tempfile shell var TMP/TEMP export that
	// path into Win32 children; GetTempPathW prefers TMP and then points at
	// a file. Fall back to LOCALAPPDATA\Temp when the result is not a dir.
	wchar_t la[MAX_PATH];
	DWORD n = ::GetEnvironmentVariableW(L"LOCALAPPDATA", la, MAX_PATH);
	if (!n || n >= MAX_PATH)
		return r.with_error(diagnostics::code::io_error,
			"failed to determine the temporary directory path");
	path fallback(la);
	fallback += L"\\Temp";
	if (!is_dir(fallback))
		return r.with_error(diagnostics::code::io_error,
			"failed to determine the temporary directory path");
	return r.with_value(fallback);
#else
	if (!ec && !d.empty()) return r.with_value(d.native());
	return r.with_value(path("/tmp"));
#endif
}

#ifdef _WIN32
inline result<path> temp_filename(const std::string& prefix) {
	using label = idni::parser_strings::label;
	result<path> r;
	if (!idni::is_valid_utf8(prefix, &r.report())) return r;

	path w = to_path(prefix);

	// GetTempFileNameW reads only the first three UTF-16 units of the
	// prefix; cut on a unit boundary, not in the middle of a surrogate
	// pair.
	size_t cut = w.size();
	bool truncated = false;
	if (cut > 3) {
		truncated = true;
		cut = (w[2] >= 0xD800 && w[2] <= 0xDBFF) ? 2 : 3;
	}
	if (truncated)
		r.warning("the temporary file prefix was cut to three characters",
			{{label::value, prefix}});
	path wprefix = w.substr(0, cut);

	auto dir = r.merge_take(temp_dir());
	if (!dir) return r;

	wchar_t name[MAX_PATH];
	if (!::GetTempFileNameW(dir->c_str(), wprefix.c_str(), 0, name))
		return r.with_error(diagnostics::code::io_error,
			"failed to reserve a temporary file name");

	return r.with_value(path(name));
}
#else
inline result<path> temp_filename(const std::string& prefix) {
	using label = idni::parser_strings::label;
	result<path> r;
	if (!idni::is_valid_utf8(prefix, &r.report())) return r;
	if (prefix.find('/') != std::string::npos ||
		prefix.find('\0') != std::string::npos)
		return r.with_error(diagnostics::code::invalid_argument,
			"the temporary file prefix must not contain a path separator",
			{{label::value, prefix}});

	auto dir = r.merge_take(temp_dir());
	if (!dir) return r;

	std::string tmpl = *dir + "/" + prefix + "_XXXXXX";
	std::vector<char> buf(tmpl.begin(), tmpl.end());
	buf.push_back('\0');
	int fd = ::mkstemp(buf.data());
	if (fd < 0) {
		return r.with_error(diagnostics::code::io_error,
			"failed to create a temporary file",
			{{label::path, tmpl}});
	}
	::close(fd);
	return r.with_value(std::string(buf.data()));
}
#endif

#ifdef _WIN32
inline result<file_handle> open(const path& p, mmap_mode m) {
	using label = idni::parser_strings::label;
	result<file_handle> r;
	// A read of a missing file must fail instead of creating an empty file.
	// Sharing delete lets another handle remove a file this one holds open.
	file_handle f = ::CreateFileW(p.c_str(),
		m == MMAP_READ ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		0, m == MMAP_READ ? OPEN_EXISTING : OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, 0);
	if (f == invalid_file_handle)
		return r.with_error(diagnostics::code::io_error,
			"failed to open file",
			{{label::path, to_utf8(p)},
				{label::exit_code, static_cast<int_t>(::GetLastError())}});
	return r.with_value(f);
}
inline result<file_handle> create(const path& p, mmap_mode m) {
	// Only a write mode creates a missing file, so a read mode maps to it.
	return open(p, m == MMAP_READ ? MMAP_WRITE : m);
}
inline result<bool> close(file_handle f) {
	using label = idni::parser_strings::label;
	result<bool> r;
	if (!::CloseHandle(f))
		return r.with_error(diagnostics::code::io_error,
			"failed to close file",
			{{label::exit_code, static_cast<int_t>(::GetLastError())}});
	return r.with_value(true);
}
inline result<size_t> size(file_handle f) {
	using label = idni::parser_strings::label;
	result<size_t> r;
	LARGE_INTEGER sz;
	if (!::GetFileSizeEx(f, &sz))
		return r.with_error(diagnostics::code::io_error,
			"failed to get file size",
			{{label::exit_code, static_cast<int_t>(::GetLastError())}});
	return r.with_value(static_cast<size_t>(sz.QuadPart));
}
inline result<bool> resize(file_handle f, size_t n) {
	using label = idni::parser_strings::label;
	result<bool> r;
	LARGE_INTEGER pos; pos.QuadPart = static_cast<LONGLONG>(n);
	if (!::SetFilePointerEx(f, pos, nullptr, FILE_BEGIN) ||
		!::SetEndOfFile(f))
		return r.with_error(diagnostics::code::io_error,
			"failed to resize file",
			{{label::exit_code, static_cast<int_t>(::GetLastError())}});
	return r.with_value(true);
}
inline result<size_t> write(file_handle f, std::string_view content) {
	using label = idni::parser_strings::label;
	result<size_t> r;
	DWORD written = 0;
	if (!::WriteFile(f, content.data(),
		static_cast<DWORD>(content.size()), &written, nullptr))
		return r.with_error(diagnostics::code::io_error,
			"failed to write to file",
			{{label::exit_code, static_cast<int_t>(::GetLastError())}});
	return r.with_value(static_cast<size_t>(written));
}
inline result<mapping> map(file_handle f, size_t n, mmap_mode m) {
	using label = idni::parser_strings::label;
	result<mapping> r;
	// the size is two DWORDs, so a file over 4 GB keeps its high bits
	const std::uint64_t size = n;
	HANDLE mh = ::CreateFileMappingW(f, 0, m == MMAP_READ ?
		PAGE_READONLY : PAGE_READWRITE, static_cast<DWORD>(size >> 32),
		static_cast<DWORD>(size & 0xFFFFFFFFu), 0);
	if (mh == nullptr) // CreateFileMappingW fails with NULL, not INVALID_HANDLE_VALUE
		return r.with_error(diagnostics::code::io_error,
			"failed to create file mapping",
			{{label::exit_code, static_cast<int_t>(::GetLastError())}});
	void* data = ::MapViewOfFile(mh, m == MMAP_READ ? FILE_MAP_READ
		: FILE_MAP_WRITE, 0, 0, n);
	if (data == nullptr) {
		auto ec = static_cast<int_t>(::GetLastError());
		::CloseHandle(mh);
		return r.with_error(diagnostics::code::io_error,
			"failed to map file", {{label::exit_code, ec}});
	}
	mapping map_;
	map_.data = data;
	map_.size = n;
	map_.mh = mh;
	return r.with_value(map_);
}
inline result<bool> sync(file_handle f, const mapping& r_map) {
	using label = idni::parser_strings::label;
	result<bool> r;
	if (::FlushViewOfFile(r_map.data, r_map.size) == 0 ||
		::FlushFileBuffers(f) == 0)
		return r.with_error(diagnostics::code::io_error,
			"failed to sync mapped memory",
			{{label::exit_code, static_cast<int_t>(::GetLastError())}});
	return r.with_value(true);
}
inline result<bool> unmap(mapping& r_map) {
	using label = idni::parser_strings::label;
	result<bool> r;
	if (!::UnmapViewOfFile(r_map.data))
		return r.with_error(diagnostics::code::io_error,
			"failed to unmap file",
			{{label::exit_code, static_cast<int_t>(::GetLastError())}});
	::CloseHandle(r_map.mh);
	r_map = mapping{};
	return r.with_value(true);
}
#else
inline result<file_handle> open(const path& p, mmap_mode m) {
	using label = idni::parser_strings::label;
	result<file_handle> r;
	int fd = ::open(p.c_str(), m == MMAP_READ ? O_RDONLY : O_RDWR, 0600);
	if (fd == -1)
		return r.with_error(diagnostics::code::io_error,
			"failed to open file",
			{{label::path, p}, {label::exit_code, static_cast<int_t>(errno)}});
	return r.with_value(fd);
}
inline result<file_handle> create(const path& p, mmap_mode m) {
	// Creating a file always opens it for reading and writing; m
	// only matters on Windows.
	(void)m;
	using label = idni::parser_strings::label;
	result<file_handle> r;
	int fd = ::open(p.c_str(), O_CREAT|O_RDWR, 0600);
	if (fd == -1)
		return r.with_error(diagnostics::code::io_error,
			"failed to create file",
			{{label::path, p}, {label::exit_code, static_cast<int_t>(errno)}});
	return r.with_value(fd);
}
inline result<bool> close(file_handle f) {
	using label = idni::parser_strings::label;
	result<bool> r;
	if (::close(f) != 0)
		return r.with_error(diagnostics::code::io_error,
			"failed to close file",
			{{label::exit_code, static_cast<int_t>(errno)}});
	return r.with_value(true);
}
inline result<size_t> size(file_handle f) {
	using label = idni::parser_strings::label;
	result<size_t> r;
	struct stat s;
	if (::fstat(f, &s) != 0)
		return r.with_error(diagnostics::code::io_error,
			"failed to get file size",
			{{label::exit_code, static_cast<int_t>(errno)}});
	return r.with_value(static_cast<size_t>(s.st_size));
}
inline result<bool> resize(file_handle f, size_t n) {
	using label = idni::parser_strings::label;
	result<bool> r;
	if (::ftruncate(f, static_cast<off_t>(n)) == -1)
		return r.with_error(diagnostics::code::io_error,
			"failed to resize file",
			{{label::exit_code, static_cast<int_t>(errno)}});
	return r.with_value(true);
}
inline result<size_t> write(file_handle f, std::string_view content) {
	using label = idni::parser_strings::label;
	result<size_t> r;
	ssize_t written = ::write(f, content.data(), content.size());
	if (written == -1)
		return r.with_error(diagnostics::code::io_error,
			"failed to write to file",
			{{label::exit_code, static_cast<int_t>(errno)}});
	return r.with_value(static_cast<size_t>(written));
}
inline result<mapping> map(file_handle f, size_t n, mmap_mode m) {
	using label = idni::parser_strings::label;
	result<mapping> r;
	void* data = ::mmap(0, n, m == MMAP_READ ? PROT_READ :
		PROT_READ|PROT_WRITE, MAP_SHARED, f, 0);
	if (data == MAP_FAILED)
		return r.with_error(diagnostics::code::io_error,
			"failed to map file",
			{{label::exit_code, static_cast<int_t>(errno)}});
	mapping map_;
	map_.data = data;
	map_.size = n;
	return r.with_value(map_);
}
inline result<bool> sync(file_handle, const mapping& r_map) {
	using label = idni::parser_strings::label;
	result<bool> r;
	if (::msync(r_map.data, r_map.size, MS_SYNC) == -1)
		return r.with_error(diagnostics::code::io_error,
			"failed to sync mapped memory",
			{{label::exit_code, static_cast<int_t>(errno)}});
	return r.with_value(true);
}
inline result<bool> unmap(mapping& r_map) {
	using label = idni::parser_strings::label;
	result<bool> r;
	if (::munmap(r_map.data, r_map.size) == -1)
		return r.with_error(diagnostics::code::io_error,
			"failed to unmap file",
			{{label::exit_code, static_cast<int_t>(errno)}});
	r_map = mapping{};
	return r.with_value(true);
}
#endif

} // namespace idni::fs

#endif // __IDNI__PARSER__UTILITY__FILESYSTEM_TMPL_H__
