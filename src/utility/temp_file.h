// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__TEMP_FILE_H__
#define __IDNI__PARSER__UTILITY__TEMP_FILE_H__

#include <string>

#include "diagnostics.h"

namespace idni::fs {

/**
 * @brief An RAII temporary file.
 *
 * Construction through @ref create writes the content once, and the
 * file exists on disk from that point on. Destruction deletes the
 * file once. A moved-from temp_file owns nothing, so its destructor
 * deletes nothing. A failing call reports through a
 * diagnostics::result, never through an exception.
 */
class temp_file {
public:
	/// Default-constructs an empty, ownerless temp_file.
	temp_file() = default;

	/**
	 * @brief Move-constructs from another temp_file.
	 * @param o the source.
	 * @post o owns nothing afterward, so its destructor deletes
	 * nothing.
	 */
	temp_file(temp_file&& o) noexcept;

	/**
	 * @brief Move-assigns from another temp_file.
	 * @param o the source.
	 * @return a reference to this object.
	 * @post This object first deletes any file of its own, then
	 * takes over the file that o owned. o owns nothing afterward,
	 * so its destructor deletes nothing.
	 */
	temp_file& operator=(temp_file&& o) noexcept;

	/// Copying is disabled. Two temp_file objects must not delete
	/// the same file.
	temp_file(const temp_file&) = delete;

	/// Copy assignment is disabled, for the same reason as the copy
	/// constructor.
	temp_file& operator=(const temp_file&) = delete;

	/**
	 * @brief Deletes the file this object owns, if any.
	 * @post The file no longer exists on disk. A default-constructed
	 * or moved-from object owns no file, so this deletes nothing.
	 */
	~temp_file();

	/**
	 * @brief The path to the file this object owns.
	 * @return the path, valid for the lifetime of this object.
	 * @warning The string holds UTF-8, not the native path encoding.
	 */
	const std::string& path() const;

	/**
	 * @brief Creates a temporary file and writes content into it.
	 * @param prefix a UTF-8 name to seed the file name, validated
	 * and possibly cut the same way as @ref temp_filename.
	 * @param content the bytes to write into the file.
	 * @return on success, a temp_file that owns the new file. On
	 * failure, an error from @ref temp_filename or @ref write.
	 * @post The file exists on disk with @p content written into
	 * it, once this call returns a value.
	 * @see temp_filename
	 * @see write
	 */
	static diagnostics::result<temp_file> create(const std::string& prefix,
		const std::string& content);

private:
	void remove_owned();

	std::string path_;
	bool owns_ = false;
};

} // namespace idni::fs

#include "temp_file.tmpl.h"

#endif // __IDNI__PARSER__UTILITY__TEMP_FILE_H__
