// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include "line_support.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#include <sddl.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#if defined(__linux__)
#include <sys/random.h>
#endif
#endif

#include "utility/filesystem.h"

namespace idni {

using jt = format::json::tree;

namespace line_log {

std::string timestamp() {
	using namespace std::chrono;
	auto now = system_clock::now();
	auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
	std::time_t secs = system_clock::to_time_t(now);
#ifdef _MSC_VER
	// the MSVC CRT marks gmtime unsafe and asks for gmtime_s
	std::tm tm_buf{};
	std::tm* tmv = gmtime_s(&tm_buf, &secs) == 0 ? &tm_buf : nullptr;
#else
	std::tm* tmv = std::gmtime(&secs);
#endif
	char buf[32] = {};
	if (tmv) std::strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%S", tmv);
	char msbuf[8] = {};
	std::snprintf(msbuf, sizeof msbuf, ".%03dZ",
		static_cast<int>(ms.count()));
	return std::string(buf) + msbuf;
}

std::string start_time() {
	std::time_t now = std::time(nullptr);
#ifdef _MSC_VER
	// the MSVC CRT marks localtime unsafe and asks for localtime_s
	std::tm tm_buf{};
	std::tm* tmv = localtime_s(&tm_buf, &now) == 0 ? &tm_buf : nullptr;
#else
	std::tm* tmv = std::localtime(&now);
#endif
	char buf[32] = {};
	if (tmv) std::strftime(buf, sizeof buf, "%Y%m%d-%H%M%S", tmv);
	return buf;
}

tref text(const char* dir, const std::string& line) {
	return jt::build_object({
		jt::build_pair("t", jt::build_string(timestamp())),
		jt::build_pair("dir", jt::build_string(dir)),
		jt::build_pair("line", jt::build_string(line)) });
}

trefs event_base(std::string_view event) {
	return { jt::build_pair("t", jt::build_string(timestamp())),
		jt::build_pair("dir", jt::build_string("event")),
		jt::build_pair("event", jt::build_string(event)) };
}

tref session_event(std::string_view event, const std::string& session) {
	trefs ev = event_base(event);
	ev.push_back(jt::build_pair("session", jt::build_string(session)));
	return jt::build_object(ev);
}

bool append(fs::file_ostream& os, tref entry) {
	if (!os.is_open()) return false;
	os << jt::get(entry) << '\n';
	os.flush();
	return os.good();
}

diagnostics::result<fs::file_ostream> open_session(const std::string& dir,
	const std::string& session)
{
	return fs::open_private_append(fs::to_path(dir + "/" + start_time()
		+ "-" + session + ".log"));
}

} // namespace line_log

bool os_random_bytes(unsigned char* out, size_t n) {
#ifdef _WIN32
	return ::BCryptGenRandom(nullptr, out, static_cast<ULONG>(n),
		BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
#else
#if defined(__linux__)
	// getrandom is the direct Linux source. A missing syscall falls
	// through to /dev/urandom; any other failure is a hard error.
	{
		size_t got = 0;
		while (got != n) {
			ssize_t r = ::getrandom(out + got, n - got, 0);
			if (r < 0) {
				if (errno == EINTR) continue;
				if (errno != ENOSYS) return false;
				break;
			}
			got += static_cast<size_t>(r);
		}
		if (got == n) return true;
		if (got != 0) return false;
	}
#endif
	int fd = ::open("/dev/urandom", O_RDONLY);
	if (fd < 0) return false;
	size_t got = 0;
	while (got != n) {
		ssize_t r = ::read(fd, out + got, n - got);
		if (r <= 0) {
			if (r < 0 && errno == EINTR) continue;
			::close(fd);
			return false;
		}
		got += static_cast<size_t>(r);
	}
	::close(fd);
	return true;
#endif
}

std::optional<std::string> new_session_id() {
	std::array<unsigned char, 16> bytes{};
	if (!os_random_bytes(bytes.data(), bytes.size()))
		return std::nullopt;
	std::string out;
	out.reserve(32);
	for (unsigned char b : bytes) {
		out.push_back("0123456789abcdef"[b >> 4]);
		out.push_back("0123456789abcdef"[b & 0xf]);
	}
	return out;
}

#ifdef _WIN32
user_only_security::user_only_security() {
	HANDLE token = nullptr;
	if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token))
		return;
	DWORD len = 0;
	::GetTokenInformation(token, TokenUser, nullptr, 0, &len);
	std::vector<unsigned char> buf(len);
	bool ok = len != 0 && ::GetTokenInformation(token, TokenUser,
		buf.data(), len, &len);
	::CloseHandle(token);
	if (!ok) return;
	LPWSTR sid = nullptr;
	if (!::ConvertSidToStringSidW(
			reinterpret_cast<TOKEN_USER*>(buf.data())->User.Sid, &sid))
		return;
	std::wstring sddl = L"D:(A;;GA;;;" + std::wstring(sid) + L")";
	::LocalFree(sid);
	PSECURITY_DESCRIPTOR d = nullptr;
	if (::ConvertStringSecurityDescriptorToSecurityDescriptorW(
			sddl.c_str(), SDDL_REVISION_1, &d, nullptr))
		sd = d;
}

user_only_security::~user_only_security() {
	if (sd) ::LocalFree(sd);
}
#endif

} // namespace idni
