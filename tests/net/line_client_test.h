// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__TESTS__NET__LINE_CLIENT_TEST_H__
#define __IDNI__PARSER__TESTS__NET__LINE_CLIENT_TEST_H__

#include <chrono>
#include <string>
#include <thread>

// The dep script ships headers only, so Boost.System must stay header-only.
#ifndef BOOST_ERROR_CODE_HEADER_ONLY
#define BOOST_ERROR_CODE_HEADER_ONLY
#endif
#ifndef BOOST_SYSTEM_HEADER_ONLY
#define BOOST_SYSTEM_HEADER_ONLY
#endif

#include <boost/asio.hpp>

namespace line_client_test {

namespace ba = boost::asio;
using boost::system::error_code;

// One client with one line at a time. Every read has a deadline, so a
// server that stops answering fails the test instead of hanging it.
template <typename Protocol>
struct basic_client {
	explicit basic_client(const typename Protocol::endpoint& ep) {
		error_code ec;
		sock.connect(ep, ec);
		REQUIRE_FALSE(ec);
		sock.non_blocking(true, ec);
		REQUIRE_FALSE(ec);
	}

	void send(const std::string& line) {
		send_raw(line + "\n");
	}

	// Write the exact bytes in one stream of writes, so the server can
	// read several pipelined lines in one read.
	void send_raw(const std::string& bytes) {
		size_t off = 0;
		auto deadline = std::chrono::steady_clock::now()
			+ std::chrono::milliseconds(10000);
		while (off != bytes.size()) {
			error_code ec;
			size_t n = sock.write_some(ba::buffer(
				bytes.data() + off, bytes.size() - off), ec);
			if (!ec) { off += n; continue; }
			if (ec == ba::error::would_block
					|| ec == ba::error::try_again) {
				if (std::chrono::steady_clock::now() >= deadline)
					break;
				std::this_thread::sleep_for(
					std::chrono::milliseconds(2));
				continue;
			}
			break;
		}
		REQUIRE(off == bytes.size());
	}

	// One line, or "" when the server sends nothing before the deadline.
	std::string read_line(int timeout_ms = 10000) {
		auto deadline = std::chrono::steady_clock::now()
			+ std::chrono::milliseconds(timeout_ms);
		while (in_buf.find('\n') == std::string::npos)
			if (!read_until(deadline)) return {};
		size_t nl = in_buf.find('\n');
		std::string line = in_buf.substr(0, nl);
		in_buf.erase(0, nl + 1);
		if (!line.empty() && line.back() == '\r') line.pop_back();
		return line;
	}

	// True when the peer closed the connection. A deadline that passes
	// with no byte means the connection is still open.
	bool at_eof(int timeout_ms = 2000) {
		if (!in_buf.empty()) return false;
		if (eof_seen) return true;
		auto deadline = std::chrono::steady_clock::now()
			+ std::chrono::milliseconds(timeout_ms);
		if (read_until(deadline)) return false;
		return eof_seen;
	}

	void close() {
		error_code ec;
		sock.close(ec);
	}

	// Read once, waiting no longer than @p deadline for a byte. False on
	// the deadline or a closed peer; a closed peer sets eof_seen.
	bool read_until(std::chrono::steady_clock::time_point deadline) {
		for (;;) {
			error_code ec;
			char tmp[4096];
			size_t n = sock.read_some(ba::buffer(tmp), ec);
			if (!ec) {
				if (n == 0) { eof_seen = true; return false; }
				in_buf.append(tmp, n);
				return true;
			}
			if (ec == ba::error::would_block
					|| ec == ba::error::try_again) {
				if (std::chrono::steady_clock::now() >= deadline)
					return false;
				std::this_thread::sleep_for(
					std::chrono::milliseconds(2));
				continue;
			}
			eof_seen = true;
			return false;
		}
	}

	ba::io_context io;
	typename Protocol::socket sock{ io };
	std::string in_buf;
	bool eof_seen = false;
};

using tcp_client = basic_client<ba::ip::tcp>;
#ifndef _WIN32
using local_client = basic_client<ba::local::stream_protocol>;
#endif

} // namespace line_client_test

#endif // __IDNI__PARSER__TESTS__NET__LINE_CLIENT_TEST_H__
