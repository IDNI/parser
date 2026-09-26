// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__FILESYSTEM_H__
#define __IDNI__PARSER__UTILITY__FILESYSTEM_H__

#include <cstddef>
#include <string>
#include <string_view>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "diagnostics.h"

/** @brief A thin portable layer over the platform file API. */
namespace idni::fs {

/**
 * @brief The encoding the platform file API takes.
 * On Windows this is UTF-16. Everywhere else it is raw bytes. A
 * caller holding a UTF-8 string converts it with @ref to_path.
 */
#ifdef _WIN32
using path = std::wstring;
#else
using path = std::string;
#endif

/**
 * @brief The result type this namespace returns.
 * @tparam T the value type a successful call holds.
 * This alias exists so a caller needs no diagnostics spelling.
 */
template <typename T>
using result = diagnostics::result<T>;

/**
 * @brief Converts a UTF-8 string to the platform path encoding.
 * @param utf8 the string to convert.
 * @return the same text as a path. POSIX keeps it as is, and
 * Windows widens it to UTF-16.
 * @see to_utf8
 */
path to_path(const std::string& utf8);

/**
 * @brief Converts a path in the platform encoding to UTF-8.
 * @param p the path to convert.
 * @return the same text as a UTF-8 string. POSIX keeps it as is,
 * and Windows narrows it from UTF-16.
 * @see to_path
 */
std::string to_utf8(const path& p);

/**
 * @brief Checks whether a path names an existing file.
 * @param p the path to check.
 * @return true when the file exists, and false otherwise. A missing
 * file is not an error, so this returns a plain bool instead of a
 * result<T>.
 */
bool exists(const path& p);

/**
 * @brief Reads the size of a file.
 * @param p the path to the file.
 * @return on success, the size in bytes. On failure, an error with
 * code::io_error, the path in label::path, and the OS error code in
 * label::exit_code.
 */
result<size_t> size(const path& p);

/**
 * @brief Deletes a file.
 * @param p the path to the file.
 * @return on success, true. On failure, an error with code::io_error,
 * the path in label::path, and the OS error code in label::exit_code.
 */
result<bool> unlink(const path& p);

/**
 * @brief Writes content to a file, creating or overwriting it.
 * @param p the path to the file.
 * @param content the bytes to write.
 * @return on success, true. On failure, an error with code::io_error
 * and the failing path in label::path.
 */
result<bool> write(const path& p, std::string_view content);

/**
 * @brief Finds the directory for temporary files.
 * @return on success, the directory path. On failure, an error with
 * code::io_error.
 * Asks std::filesystem first. On Windows, it then falls back to a
 * native temporary-path lookup, which can still fail. On POSIX, it
 * falls back to "/tmp" and never fails.
 * @see temp_filename
 */
result<path> temp_dir();

/**
 * @brief Reserves a fresh, uniquely named temporary file.
 * @param prefix a UTF-8 name to seed the file name.
 * @return on success, the native path of the file this call just
 * created. On failure, an error. A malformed or unsafe prefix gets
 * code::invalid_argument, with the prefix in label::value. A failing
 * OS call gets code::io_error, with the attempted path in
 * label::path where one exists.
 * @post The file exists on disk and is empty, once this call
 * returns a value.
 * @note This call creates the file. It does not only choose a name.
 * @note On Windows, it keeps only the first three UTF-16 units of
 * the prefix, and records a warning when it must cut the prefix.
 * @note On POSIX, it rejects a prefix that holds a path separator
 * or a NUL.
 * @see temp_dir
 */
result<path> temp_filename(const std::string& prefix);

/**
 * @brief A native file handle.
 * This is a raw handle, not an owning type. A caller that opens one
 * closes it, through @ref close.
 */
#ifdef _WIN32
using file_handle = HANDLE;
#else
using file_handle = int;
#endif

/// The value that never names an open file: INVALID_HANDLE_VALUE on
/// Windows, -1 elsewhere.
inline const file_handle invalid_file_handle =
#ifdef _WIN32
	INVALID_HANDLE_VALUE;
#else
	-1;
#endif

/**
 * @brief The access mode for a mapped file.
 * The enumerators sit directly in idni::fs, not behind mmap_mode::,
 * because this is a plain enum.
 * MMAP_NONE opens no file and maps nothing. A memory_map_allocator
 * in this mode falls back to plain std::allocator.
 * MMAP_READ opens and maps the file read-only.
 * MMAP_WRITE opens and maps the file for reading and writing, and
 * creates the file first when it does not exist.
 */
enum mmap_mode { MMAP_NONE, MMAP_READ, MMAP_WRITE };

/**
 * @brief One mapped region, with the handle the platform needs to
 * unmap it.
 * @warning A default-constructed mapping names no memory. Only pass
 * a mapping that @ref map returned.
 */
struct mapping {
	void* data = nullptr;   ///< The mapped memory, or null when unmapped.
	size_t size = 0;        ///< The mapped size, in bytes.
#ifdef _WIN32
	HANDLE mh = nullptr;    ///< The mapping object CreateFileMappingW returns.
#endif
};

/**
 * @brief Opens an existing file.
 * @param p the path to the file.
 * @param m the access mode. MMAP_READ opens it read-only, and any
 * other mode opens it for reading and writing.
 * @return on success, an open file_handle. On failure, an error
 * with code::io_error, the path in label::path, and the OS error
 * code in label::exit_code.
 * @post The caller closes the returned handle, through @ref close.
 */
result<file_handle> open(const path& p, mmap_mode m);

/**
 * @brief Creates a file, or opens it if it already exists.
 * @param p the path to the file.
 * @param m the access mode. Ignored on POSIX, where creating a file
 * always opens it for reading and writing.
 * @return on success, an open file_handle. On failure, an error
 * with code::io_error, the path in label::path, and the OS error
 * code in label::exit_code.
 * @post The caller closes the returned handle, through @ref close.
 * @note On Windows, this opens the file with write access, because only
 * a write-open creates a missing file.
 */
result<file_handle> create(const path& p, mmap_mode m);

/**
 * @brief Closes a file handle.
 * @param f the handle to close.
 * @return on success, true. On failure, an error with
 * code::io_error and the OS error code in label::exit_code.
 * @post @p f no longer names an open file.
 */
result<bool> close(file_handle f);

/**
 * @brief Reads the size of an open file.
 * @param f the open file.
 * @return on success, the size in bytes. On failure, an error with
 * code::io_error and the OS error code in label::exit_code. Unlike
 * size(const path&), this overload has no path to report.
 */
result<size_t> size(file_handle f);

/**
 * @brief Grows or shrinks an open file to an exact size.
 * @param f the open file.
 * @param n the new size, in bytes.
 * @return on success, true. On failure, an error with
 * code::io_error and the OS error code in label::exit_code.
 * @post The file is exactly @p n bytes long.
 */
result<bool> resize(file_handle f, size_t n);

/**
 * @brief Writes to an open file at its current position.
 * @param f the open file.
 * @param content the bytes to write.
 * @return on success, the number of bytes written, which can be
 * less than content.size(). On failure, an error with
 * code::io_error and the OS error code in label::exit_code.
 * @warning Unlike write(const path&, ...), this overload never
 * checks for a short write. A caller compares the returned count
 * against content.size() itself.
 */
result<size_t> write(file_handle f, std::string_view content);

/**
 * @brief Maps an open file into memory.
 * @param f the open file.
 * @param n the number of bytes to map.
 * @param m the access mode. MMAP_READ maps it read-only, and any
 * other mode maps it for reading and writing.
 * @return on success, the mapping. On failure, an error with
 * code::io_error and the OS error code in label::exit_code.
 * @pre @p f stays open for the life of the mapping. @ref sync
 * reads it back on Windows.
 * @post A caller undoes this with @ref unmap.
 */
result<mapping> map(file_handle f, size_t n, mmap_mode m);

/**
 * @brief Flushes a mapped region to disk.
 * @param f the file that map used to produce @p r. Windows flushes
 * the file handle as well as the mapped view, so sync needs it. A
 * POSIX msync needs only the mapping.
 * @param r the mapping to flush.
 * @return on success, true. On failure, an error with
 * code::io_error and the OS error code in label::exit_code.
 */
result<bool> sync(file_handle f, const mapping& r);

/**
 * @brief Unmaps a mapped region.
 * @param r the mapping to undo.
 * @return on success, true. On failure, an error with
 * code::io_error and the OS error code in label::exit_code.
 * @post On success, r no longer names any memory: its fields reset
 * to their default state.
 */
result<bool> unmap(mapping& r);

} // namespace idni::fs

#include "filesystem.tmpl.h"

#endif // __IDNI__PARSER__UTILITY__FILESYSTEM_H__
