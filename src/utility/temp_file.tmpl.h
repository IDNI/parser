// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__TEMP_FILE_TMPL_H__
#define __IDNI__PARSER__UTILITY__TEMP_FILE_TMPL_H__

#include <string>
#include <utility>

#include "temp_file.h"
#include "filesystem.h"

namespace idni::fs {

inline temp_file::temp_file(temp_file&& o) noexcept
	: path_(std::move(o.path_)), owns_(o.owns_) { o.owns_ = false; }

inline temp_file& temp_file::operator=(temp_file&& o) noexcept {
	if (this != &o) {
		remove_owned();
		path_ = std::move(o.path_);
		owns_ = o.owns_;
		o.owns_ = false;
	}
	return *this;
}

inline temp_file::~temp_file() { remove_owned(); }

inline const std::string& temp_file::path() const { return path_; }

inline void temp_file::remove_owned() {
	// A destructor cannot return a report, so a failing unlink here goes unreported.
	if (owns_) { unlink(to_path(path_)); owns_ = false; }
}

inline diagnostics::result<temp_file> temp_file::create(
	const std::string& prefix, const std::string& content)
{
	return temp_filename(prefix).and_then(
		[&content](fs::path p) -> diagnostics::result<temp_file>
	{
		// Owns the file as soon as temp_filename creates it, so a
		// failing write still leaves the destructor of t to delete it.
		temp_file t;
		t.path_ = to_utf8(p);
		t.owns_ = true;
		return write(p, content).and_then(
			[&t](bool) -> diagnostics::result<temp_file>
		{
			diagnostics::result<temp_file> r;
			return r.with_value(std::move(t));
		});
	});
}

} // namespace idni::fs

#endif // __IDNI__PARSER__UTILITY__TEMP_FILE_TMPL_H__
