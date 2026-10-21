// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__NET__LINE_SUPPORT_H__
#define __IDNI__PARSER__NET__LINE_SUPPORT_H__

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include "utility/diagnostics.h"
#include "utility/filesystem.h"
#include "format/json/json.h"

namespace idni {

/// The log lines, the session id and the session log file of the line
/// server, for a program that keeps the session logs itself.
namespace line_log {

/// UTC ISO 8601 with milliseconds, for example "2026-09-27T10:15:03.123Z".
std::string timestamp();

/// The local start time in a file name, for example "20260927-101503".
std::string start_time();

/// The log entry {"t","dir","line"} of a line in or out.
tref text(const char* dir, const std::string& line);

/// The members {"t","dir":"event","event"} of an event entry. The caller
/// appends its own members and builds the object.
trefs event_base(std::string_view event);

/// The event entry @p event of the session @p session.
tref session_event(std::string_view event, const std::string& session);

/// Append one JSON line and flush it. False when the write fails.
bool append(fs::file_ostream& os, tref entry);

/// Open the log of the session @p session in @p dir, readable by the
/// owner only. The name starts with the start time.
diagnostics::result<fs::file_ostream> open_session(const std::string& dir,
	const std::string& session);

} // namespace line_log

/// @p n bytes from the entropy source of the system. False is a hard
/// failure.
bool os_random_bytes(unsigned char* out, size_t n);

/// 128 random bits as 32 lowercase hex characters. No value when the
/// entropy source fails.
std::optional<std::string> new_session_id();

#ifdef _WIN32
/// The security descriptor "D:(A;;GA;;;<user SID>)": full access for the
/// user of the process and no access for anyone else.
struct user_only_security {
	user_only_security();
	~user_only_security();
	user_only_security(const user_only_security&) = delete;
	user_only_security& operator=(const user_only_security&) = delete;
	/// A PSECURITY_DESCRIPTOR, null when the build of it failed.
	void* descriptor() const noexcept { return sd; }
private:
	void* sd = nullptr;
};
#endif

} // namespace idni

#endif // __IDNI__PARSER__NET__LINE_SUPPORT_H__
