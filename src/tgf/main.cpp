// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#include <vector>
#include <string>
#include "utility/characters.h"
#endif

namespace idni { int tgf_run(int argc, char** argv); }

#ifdef _WIN32
static int tgf_main_windows() {
	int wargc = 0;
	LPWSTR* wargv = ::CommandLineToArgvW(::GetCommandLineW(), &wargc);
	if (!wargv) return idni::tgf_run(0, nullptr);

	std::vector<std::string> args;
	args.reserve(static_cast<size_t>(wargc));
	for (int i = 0; i < wargc; ++i)
		args.emplace_back(idni::wide_to_utf8(std::wstring(wargv[i])));
	::LocalFree(wargv);

	std::vector<char*> argv_utf8;
	argv_utf8.reserve(args.size() + 1);
	for (auto& s : args) argv_utf8.push_back(s.data());
	argv_utf8.push_back(nullptr);

	return idni::tgf_run(static_cast<int>(args.size()), argv_utf8.data());
}
#endif

int main(int argc, char** argv) {
#ifdef _WIN32
	::SetConsoleOutputCP(CP_UTF8);
	::SetConsoleCP(CP_UTF8);
	(void)argc; (void)argv;
	return tgf_main_windows();
#else
	return idni::tgf_run(argc, argv);
#endif
}
