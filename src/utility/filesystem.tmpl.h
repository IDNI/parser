// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__FILESYSTEM_TMPL_H__
#define __IDNI__PARSER__UTILITY__FILESYSTEM_TMPL_H__

#include <cerrno>
#include <cstdio>
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

inline result<bool> write(const path& p, std::string_view content) {
	using label = idni::parser_strings::label;
	result<bool> r;
#ifdef _WIN32
	FILE* f = ::_wfopen(p.c_str(), L"wb");
#else
	FILE* f = std::fopen(p.c_str(), "wb");
#endif
	if (!f) {
		return r.with_error(diagnostics::code::io_error,
			"failed to open the file for writing",
			{{label::path, to_utf8(p)}});
	}
	size_t written = std::fwrite(content.data(), 1, content.size(), f);
	bool short_write = written != content.size();
	bool flush_failed = std::fclose(f) != 0;
	int flush_errno = errno;
	if (short_write) {
		return r.with_error(diagnostics::code::io_error,
			"failed to write the file",
			{{label::path, to_utf8(p)}});
	}
	if (flush_failed) {
		return r.with_error(diagnostics::code::io_error,
			"failed to flush the file",
			{{label::path, to_utf8(p)},
				{label::exit_code, static_cast<int_t>(flush_errno)}});
	}
	return r.with_value(true);
}

inline result<path> temp_dir() {
	result<path> r;
	std::error_code ec;
	auto d = std::filesystem::temp_directory_path(ec);
	if (!ec && !d.empty()) return r.with_value(d.native());
#ifdef _WIN32
	using label = idni::parser_strings::label;
	wchar_t dir[MAX_PATH];
	DWORD dr = ::GetTempPathW(MAX_PATH, dir);
	if (dr > MAX_PATH)
		return r.with_error(diagnostics::code::io_error,
			"the temporary directory path is too long",
			{{label::size, static_cast<size_t>(dr)},
				{label::limit, static_cast<size_t>(MAX_PATH)}});
	if (dr == 0)
		return r.with_error(diagnostics::code::io_error,
			"failed to determine the temporary directory path");
	return r.with_value(path(dir));
#else
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
	file_handle f = ::CreateFileW(p.c_str(),
		m == MMAP_READ ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (f == invalid_file_handle)
		return r.with_error(diagnostics::code::io_error,
			"failed to open file",
			{{label::path, to_utf8(p)},
				{label::exit_code, static_cast<int_t>(::GetLastError())}});
	return r.with_value(f);
}
inline result<file_handle> create(const path& p, mmap_mode m) {
	// OPEN_ALWAYS already creates a missing file, so create is open.
	return open(p, m);
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
	HANDLE mh = ::CreateFileMappingW(f, 0, m == MMAP_READ ?
		PAGE_READONLY : PAGE_READWRITE, 0, n, 0);
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
