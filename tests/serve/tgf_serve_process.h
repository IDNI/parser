// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// A child process with pipes for the serve tests. POSIX starts it with
// posix_spawn and reads with poll; Windows starts it with CreateProcessW
// and reads with PeekNamedPipe. No test forks.

#ifndef __IDNI__PARSER__TESTS__TGF_SERVE_PROCESS_H__
#define __IDNI__PARSER__TESTS__TGF_SERVE_PROCESS_H__

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#ifdef _WIN32
// Keep windows.h from pulling in winsock.h, which conflicts with the
// winsock2.h that Boost.Asio includes.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
// The shared Windows command-line helpers.
#include "../../src/utility/win_args.h"
#else
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <poll.h>
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif

namespace tgf_serve_test {

#ifdef _WIN32
// The test environment gives a host path. Wine maps the Unix root to Z:,
// and a native build passes a path that already names its drive.
inline std::string process_host_path(const std::string& p) {
	if (p.find(':') != std::string::npos) return p;
	if (p.empty() || p[0] != '/') return p;
	std::string out = "Z:";
	for (char c : p) out.push_back(c == '/' ? '\\' : c);
	return out;
}
#endif

#ifndef _WIN32
// One text line from @p fd, waiting no longer than @p timeout_ms. Bytes
// already read stay in @p buf.
inline bool process_read_line(int fd, std::string& buf, std::string& line,
	int timeout_ms)
{
	using clock = std::chrono::steady_clock;
	auto deadline = clock::now() + std::chrono::milliseconds(timeout_ms);
	for (;;) {
		size_t nl = buf.find('\n');
		if (nl != std::string::npos) {
			line = buf.substr(0, nl);
			buf.erase(0, nl + 1);
			if (!line.empty() && line.back() == '\r')
				line.pop_back();
			return true;
		}
		int remaining = static_cast<int>(
			std::chrono::duration_cast<std::chrono::milliseconds>(
				deadline - clock::now()).count());
		if (remaining < 0) remaining = 0;
		struct pollfd pfd{};
		pfd.fd = fd;
		pfd.events = POLLIN;
		int pr = ::poll(&pfd, 1, remaining);
		if (pr == 0) return false;
		if (pr < 0) {
			if (errno == EINTR) continue;
			return false;
		}
		char tmp[4096];
		ssize_t n = ::read(fd, tmp, sizeof tmp);
		if (n <= 0) return false;
		buf.append(tmp, static_cast<size_t>(n));
	}
}
#else
// One text line from @p h, waiting no longer than @p timeout_ms. Bytes
// already read stay in @p buf.
inline bool process_read_line(HANDLE h, std::string& buf,
	std::string& line, int timeout_ms)
{
	using clock = std::chrono::steady_clock;
	auto deadline = clock::now() + std::chrono::milliseconds(timeout_ms);
	for (;;) {
		size_t nl = buf.find('\n');
		if (nl != std::string::npos) {
			line = buf.substr(0, nl);
			buf.erase(0, nl + 1);
			if (!line.empty() && line.back() == '\r')
				line.pop_back();
			return true;
		}
		DWORD avail = 0;
		if (!::PeekNamedPipe(h, nullptr, 0, nullptr, &avail,
				nullptr))
			return false;
		if (avail != 0) {
			char tmp[4096];
			DWORD want = avail < sizeof tmp ? avail
				: static_cast<DWORD>(sizeof tmp);
			DWORD got = 0;
			if (!::ReadFile(h, tmp, want, &got, nullptr))
				return false;
			buf.append(tmp, got);
			continue;
		}
		if (clock::now() >= deadline) return false;
		::Sleep(2);
	}
}
#endif

/// A child process with pipes on stdin, stdout and stderr. The helper
/// writes whole lines to stdin and reads whole lines from stdout or
/// stderr with a timeout. The destructor stops the process.
struct child_process {
	child_process() = default;
	~child_process() { stop(); }
	child_process(const child_process&) = delete;
	child_process& operator=(const child_process&) = delete;

	/// Start @p program with @p args. The child keeps only the three
	/// pipes, not the handles of the parent.
	bool start(const std::string& program,
		const std::vector<std::string>& args)
	{
#ifdef _WIN32
		SECURITY_ATTRIBUTES sa{};
		sa.nLength = sizeof sa;
		sa.bInheritHandle = TRUE;
		HANDLE in_r = nullptr, in_w = nullptr;
		HANDLE out_r = nullptr, out_w = nullptr;
		HANDLE err_r = nullptr, err_w = nullptr;
		if (!::CreatePipe(&in_r, &in_w, &sa, 0)) return false;
		if (!::CreatePipe(&out_r, &out_w, &sa, 0)) {
			::CloseHandle(in_r);
			::CloseHandle(in_w);
			return false;
		}
		if (!::CreatePipe(&err_r, &err_w, &sa, 0)) {
			::CloseHandle(in_r);
			::CloseHandle(in_w);
			::CloseHandle(out_r);
			::CloseHandle(out_w);
			return false;
		}
		::SetHandleInformation(in_w, HANDLE_FLAG_INHERIT, 0);
		::SetHandleInformation(out_r, HANDLE_FLAG_INHERIT, 0);
		::SetHandleInformation(err_r, HANDLE_FLAG_INHERIT, 0);
		std::wstring cmd = idni::win_quote_arg(
			idni::win_widen(process_host_path(program)));
		for (const auto& a : args)
			cmd += L" " + idni::win_quote_arg(idni::win_widen(a));
		STARTUPINFOW si{};
		si.cb = sizeof si;
		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdInput = in_r;
		si.hStdOutput = out_w;
		si.hStdError = err_w;
		PROCESS_INFORMATION pi{};
		BOOL ok = ::CreateProcessW(nullptr, cmd.data(), nullptr,
			nullptr, TRUE, 0, nullptr, nullptr, &si, &pi);
		::CloseHandle(in_r);
		::CloseHandle(out_w);
		::CloseHandle(err_w);
		if (!ok) {
			::CloseHandle(in_w);
			::CloseHandle(out_r);
			::CloseHandle(err_r);
			return false;
		}
		::CloseHandle(pi.hThread);
		process_ = pi.hProcess;
		in_w_ = in_w;
		out_r_ = out_r;
		err_r_ = err_r;
		return true;
#else
		int p2c[2] = { -1, -1 };
		int c2p[2] = { -1, -1 };
		int e2p[2] = { -1, -1 };
		if (::pipe(p2c) != 0) return false;
		if (::pipe(c2p) != 0) {
			::close(p2c[0]);
			::close(p2c[1]);
			return false;
		}
		if (::pipe(e2p) != 0) {
			::close(p2c[0]);
			::close(p2c[1]);
			::close(c2p[0]);
			::close(c2p[1]);
			return false;
		}
		posix_spawn_file_actions_t fa;
		if (posix_spawn_file_actions_init(&fa) != 0) {
			::close(p2c[0]);
			::close(p2c[1]);
			::close(c2p[0]);
			::close(c2p[1]);
			::close(e2p[0]);
			::close(e2p[1]);
			return false;
		}
		posix_spawn_file_actions_adddup2(&fa, p2c[0], 0);
		posix_spawn_file_actions_adddup2(&fa, c2p[1], 1);
		posix_spawn_file_actions_adddup2(&fa, e2p[1], 2);
		// The child keeps only the pipe ends it reads or writes.
		posix_spawn_file_actions_addclose(&fa, p2c[0]);
		posix_spawn_file_actions_addclose(&fa, c2p[1]);
		posix_spawn_file_actions_addclose(&fa, e2p[1]);
		posix_spawn_file_actions_addclose(&fa, p2c[1]);
		posix_spawn_file_actions_addclose(&fa, c2p[0]);
		posix_spawn_file_actions_addclose(&fa, e2p[0]);
		std::vector<std::string> owned;
		owned.push_back(program);
		for (const auto& a : args) owned.push_back(a);
		std::vector<char*> argv;
		for (auto& s : owned) argv.push_back(s.data());
		argv.push_back(nullptr);
		pid_t pid = -1;
		int rc = posix_spawn(&pid, program.c_str(), &fa, nullptr,
			argv.data(), environ);
		posix_spawn_file_actions_destroy(&fa);
		if (rc != 0) {
			::close(p2c[0]);
			::close(p2c[1]);
			::close(c2p[0]);
			::close(c2p[1]);
			::close(e2p[0]);
			::close(e2p[1]);
			return false;
		}
		::close(p2c[0]);
		::close(c2p[1]);
		::close(e2p[1]);
		pid_ = pid;
		in_fd_ = p2c[1];
		out_fd_ = c2p[0];
		err_fd_ = e2p[0];
		return true;
#endif
	}

	bool read_stdout_line(std::string& line, int timeout_ms) {
#ifdef _WIN32
		return process_read_line(out_r_, out_buf_, line, timeout_ms);
#else
		return process_read_line(out_fd_, out_buf_, line, timeout_ms);
#endif
	}

	bool read_stderr_line(std::string& line, int timeout_ms) {
#ifdef _WIN32
		return process_read_line(err_r_, err_buf_, line, timeout_ms);
#else
		return process_read_line(err_fd_, err_buf_, line, timeout_ms);
#endif
	}

	bool write_stdin(const std::string& line) {
		std::string l = line + "\n";
#ifdef _WIN32
		if (!in_w_) return false;
		DWORD written = 0;
		return ::WriteFile(in_w_, l.data(),
			static_cast<DWORD>(l.size()), &written, nullptr)
			&& written == l.size();
#else
		if (in_fd_ < 0) return false;
		size_t off = 0;
		while (off < l.size()) {
			ssize_t n = ::write(in_fd_, l.data() + off,
				l.size() - off);
			if (n <= 0) {
				if (errno == EINTR) continue;
				return false;
			}
			off += static_cast<size_t>(n);
		}
		return true;
#endif
	}

	void stop() {
#ifdef _WIN32
		if (process_) {
			::TerminateProcess(process_, 1);
			::WaitForSingleObject(process_, 5000);
			::CloseHandle(process_);
			process_ = nullptr;
		}
		if (in_w_) { ::CloseHandle(in_w_); in_w_ = nullptr; }
		if (out_r_) { ::CloseHandle(out_r_); out_r_ = nullptr; }
		if (err_r_) { ::CloseHandle(err_r_); err_r_ = nullptr; }
#else
		if (pid_ > 0) {
			::kill(pid_, SIGTERM);
			int status = 0;
			// A bounded wait, so a stuck child never blocks the test.
			for (int i = 0; i != 500; ++i) {
				if (::waitpid(pid_, &status, WNOHANG) == pid_) {
					pid_ = -1;
					break;
				}
				::usleep(10000);
			}
			if (pid_ > 0) {
				::kill(pid_, SIGKILL);
				::waitpid(pid_, &status, 0);
				pid_ = -1;
			}
		}
		if (in_fd_ >= 0) { ::close(in_fd_); in_fd_ = -1; }
		if (out_fd_ >= 0) { ::close(out_fd_); out_fd_ = -1; }
		if (err_fd_ >= 0) { ::close(err_fd_); err_fd_ = -1; }
#endif
	}

private:
	std::string out_buf_;
	std::string err_buf_;
#ifdef _WIN32
	HANDLE process_ = nullptr;
	HANDLE in_w_ = nullptr;
	HANDLE out_r_ = nullptr;
	HANDLE err_r_ = nullptr;
#else
	pid_t pid_ = -1;
	int in_fd_ = -1;
	int out_fd_ = -1;
	int err_fd_ = -1;
#endif
};

} // namespace tgf_serve_test

#endif // __IDNI__PARSER__TESTS__TGF_SERVE_PROCESS_H__
