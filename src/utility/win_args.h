// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__WIN_ARGS_H__
#define __IDNI__PARSER__UTILITY__WIN_ARGS_H__

// The Windows command-line helpers: a UTF-8 to UTF-16 conversion and the
// quoting one argument needs. They are empty on other platforms, so
// including this header is always safe.

#include <string>

#ifdef _WIN32
// Keep windows.h from pulling in the older winsock.h, which conflicts with
// the winsock2.h that Boost.Asio includes.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace idni {

// UTF-8 to UTF-16 for a Windows API call.
inline std::wstring win_widen(const std::string& s) {
	if (s.empty()) return {};
	int n = ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(),
		static_cast<int>(s.size()), nullptr, 0);
	if (n <= 0) return {};
	std::wstring out(static_cast<size_t>(n), L'\0');
	::MultiByteToWideChar(CP_UTF8, 0, s.c_str(),
		static_cast<int>(s.size()), out.data(), n);
	return out;
}

// Quote one Windows command-line argument: wrap it in double quotes when
// it is empty or holds a space, a tab or a quote, and escape backslashes
// that precede a quote.
inline std::wstring win_quote_arg(const std::wstring& a) {
	bool need = a.empty()
		|| a.find_first_of(L" \t\"") != std::wstring::npos;
	if (!need) return a;
	std::wstring out = L"\"";
	for (size_t i = 0; i != a.size(); ++i) {
		size_t run = 0;
		while (i != a.size() && a[i] == L'\\') { ++run; ++i; }
		if (i == a.size()) {
			out.append(run * 2, L'\\');
			break;
		}
		if (a[i] == L'"') {
			out.append(run * 2 + 1, L'\\');
			out.push_back(L'"');
		} else {
			out.append(run, L'\\');
			out.push_back(a[i]);
		}
	}
	out.push_back(L'"');
	return out;
}

} // namespace idni

#endif // _WIN32

#endif // __IDNI__PARSER__UTILITY__WIN_ARGS_H__
