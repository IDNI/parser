// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__MEMORY_MAP_H__
#define __IDNI__PARSER__UTILITY__MEMORY_MAP_H__
#include <string>
#include <memory>

#include "filesystem.h"

namespace idni::fs {

/**
 * @brief A file mapped into memory.
 *
 * Opening the file and mapping it into memory are separate steps,
 * moved through by @ref open and @ref map, and undone in turn by
 * @ref unmap and @ref close. Every such call returns a result<bool>:
 * true on success, and on failure a report that carries the reason.
 * This object also keeps that report, so a later caller can inspect
 * a past failure through @ref has_error and @ref report.
 *
 * A redundant close or unmap is not a failure. It records a warning
 * instead of an error, returns false, and leaves @ref has_error
 * false.
 */
class memory_map {
public:
	/// Default-constructs a memory_map with no file and no mapping.
	memory_map();

	/**
	 * @brief Constructs a memory_map, and optionally opens and maps
	 * it.
	 * @param filename the path to the file. An empty path means a
	 * temporary file, which close() deletes.
	 * @param s the size to map, in bytes. Zero, the default, means
	 * autodetect the size from the file once it is open.
	 * @param m the access mode. Defaults to MMAP_READ.
	 * @param do_open true to open the file during construction.
	 * True is the default.
	 * @param do_map true to map the file during construction. True
	 * is the default.
	 * @post Opens the file when do_open or do_map is true. Maps the
	 * file when do_map is true and the open succeeds. Does neither
	 * when m is MMAP_NONE. A failing open or map records into the
	 * report, readable through @ref report once construction ends.
	 */
	memory_map(path filename, size_t s=0, mmap_mode m = MMAP_READ,
		bool do_open=1, bool do_map=1);

	/**
	 * @brief Closes the file, unmapping it first if needed.
	 * @post Same effect as calling close(). A temporary file no
	 * longer exists on disk.
	 */
	~memory_map();

	/**
	 * @brief The mapped size.
	 * @return the size in bytes, the value the constructor
	 * autodetected when the caller passed zero.
	 */
	size_t size() const;

	/**
	 * @brief The path to the mapped file.
	 * @return the path, in the native path encoding.
	 */
	const path& file_name() const;

	/**
	 * @brief Checks the accumulated report for an error.
	 * @return true after any past call on this object fails.
	 */
	bool has_error() const;

	/**
	 * @brief The report this object accumulates across every call.
	 * @return the report, carrying every past error and warning in
	 * the order the calls made them.
	 */
	const diagnostics::report& report() const;

	/**
	 * @brief The mapped memory.
	 * @return a pointer to the mapped memory, or null outside the
	 * MAPPED state.
	 */
	void* data();

	/**
	 * @brief Opens the file, creating a temporary one when the path
	 * is empty.
	 * @return on success, true. On failure, an error with
	 * code::invalid_state for a precondition, or code::io_error for
	 * a failing system call, with the path in label::path and the
	 * OS error code in label::exit_code.
	 * @pre Needs the CLOSED state.
	 * @post On success, moves to the UNMAPPED state. size() then
	 * holds the file size, autodetected when the caller passed zero.
	 * On failure, state_ stays CLOSED, no handle stays open, and no
	 * temporary file this call created stays on disk.
	 */
	result<bool> open();

	/**
	 * @brief Maps the open file into memory.
	 * @return on success, true. On failure, an error with
	 * code::invalid_state for a precondition, or code::io_error for
	 * a failing system call, with the OS error code in
	 * label::exit_code.
	 * @pre Needs the UNMAPPED state.
	 * @post On success, moves to the MAPPED state, and data()
	 * returns the mapped memory.
	 */
	result<bool> map();

	/**
	 * @brief Flushes the mapped memory to disk.
	 * @return on success, true. On failure, an error with
	 * code::io_error and the OS error code in label::exit_code.
	 */
	result<bool> sync();

	/**
	 * @brief Unmaps the file, syncing it to disk first.
	 * @return true on success. On a call outside the MAPPED state,
	 * false, with a warning in the report. On a failing sync or a
	 * failing system call, an error with code::io_error and the OS
	 * error code in label::exit_code.
	 * @post On success, moves to the UNMAPPED state, and data()
	 * returns null. On the redundant-call case, nothing changes.
	 */
	result<bool> unmap();

	/**
	 * @brief Deletes the file from disk.
	 * @return on success, true. On failure, an error with
	 * code::invalid_state for a precondition, or the error fs::unlink
	 * reports for a failing deletion.
	 * @pre Needs the CLOSED state.
	 */
	result<bool> unlink();

	/**
	 * @brief Unmaps the file if needed, then closes it.
	 * @return true on success. On a call on an already CLOSED
	 * object, false, with a warning in the report. On a failing
	 * unmap, the error unmap reports.
	 * @post Moves to the CLOSED state. A temporary file no longer
	 * exists on disk. A failing delete of a temporary file still
	 * records into the report, but does not fail this call.
	 */
	result<bool> close();

	/**
	 * @brief Reads one byte of the mapped memory.
	 * @param i the byte offset.
	 * @return the byte at offset @p i.
	 * @pre Needs the MAPPED state.
	 * @warning This never checks that @p i is within size(). An
	 * out-of-range i causes undefined behavior.
	 */
	char operator[](const size_t i) noexcept;
private:
	mmap_mode mode_;
	/// The three states: CLOSED, UNMAPPED, and MAPPED.
	enum { CLOSED, UNMAPPED, MAPPED } state_;
	path filename_;
	size_t size_;
	bool temporary_ = false;
	diagnostics::report report_;
	file_handle fh_ = invalid_file_handle;
	mapping map_;
#ifndef _WIN32
	result<bool> truncate();
	result<bool> seek_beginning();
	result<bool> fill();
	result<bool> create();
#endif
	result<bool> create_temp();
	result<size_t> file_size();
	/// Closes a handle open() opened, and deletes a temp file it
	/// created, before a failing open() returns @p r. A cleanup
	/// failure appends after the original error in @p r, so the
	/// first failure stays the reason.
	result<bool> cleanup_failed_open(result<bool> r);
	/// Records @p message into the report, and returns a failed
	/// result<bool> carrying the same error.
	result<bool> err(diagnostics::code c, std::string_view message,
		std::initializer_list<diagnostics::attr_in> extra = {});
	/// Records @p message into the report as a warning, and returns
	/// a result<bool> holding false: the call did nothing.
	result<bool> warn(std::string_view message,
		std::initializer_list<diagnostics::attr_in> extra = {});
};

/**
 * @brief A std::allocator that backs its memory with a memory_map.
 * @tparam T the element type to allocate.
 *
 * MMAP_NONE falls back to a plain std::allocator<T>, instead of
 * mapping a file. Any other mode maps a fresh memory_map for every
 * call to allocate, and closes it on the matching deallocate.
 */
template <typename T>
class memory_map_allocator {
public:
	/// The element type this allocator allocates.
	typedef T value_type;

	/// Default-constructs an allocator in MMAP_NONE mode.
	memory_map_allocator();

	/**
	 * @brief Constructs an allocator that maps a file.
	 * @param fn the path to the file. An empty path means a
	 * temporary file.
	 * @param m the access mode. Defaults to MMAP_WRITE.
	 */
	memory_map_allocator(path fn, mmap_mode m = MMAP_WRITE);

	/**
	 * @brief Copies the file path and the access mode.
	 * @param a the allocator to copy from.
	 * @note A copy starts unmapped, even when a is already mapped.
	 */
	memory_map_allocator(const memory_map_allocator<T>& a);

	/**
	 * @brief Allocates n elements of type T.
	 * @param n the element count.
	 * @return a pointer to n*sizeof(T) bytes. In MMAP_NONE mode,
	 * this comes from std::allocator<T>. In a mapping mode, this
	 * comes from a fresh memory_map, or null when n is zero.
	 * @post In a mapping mode, this object owns the new memory_map,
	 * replacing any it owned before.
	 * @warning In a mapping mode, this returns null when the
	 * mapping fails, the same as memory_map::data() outside the
	 * MAPPED state. This call reports no error of its own. A caller
	 * that gets null reads the reason from the held memory_map.
	 */
	T* allocate(size_t n);

	/**
	 * @brief Releases memory obtained from allocate.
	 * @param p the pointer allocate returned.
	 * @param n the element count passed to allocate.
	 * @post In MMAP_NONE mode, this forwards to std::allocator<T>.
	 * In a mapping mode, a null p, a zero n, or holding no
	 * memory_map is a no-op, and otherwise this closes the
	 * memory_map allocate created.
	 */
	void deallocate(T* p, size_t n);

	/**
	 * @brief Compares the file path and the access mode.
	 * @param t the allocator to compare against.
	 * @return true when the path and the mode compare equal. This
	 * ignores the held memory_map, so two allocators still meet the
	 * Allocator requirements once one of them allocates.
	 */
	bool operator==(const memory_map_allocator& t) const;

	/**
	 * @brief Returns the opposite of operator==.
	 * @param t the allocator to compare against.
	 * @return true when any of the compared fields differ.
	 */
	bool operator!=(const memory_map_allocator& t) const;
private:
	path fn;
	mmap_mode m;
	std::unique_ptr<memory_map> mm;
	std::allocator<T> nommap;
};

} // namespace idni::fs

#include "memory_map.tmpl.h"

#endif // __IDNI__PARSER__UTILITY__MEMORY_MAP_H__
