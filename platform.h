#ifndef SVNCLEAN_PLATFORM_H
#define SVNCLEAN_PLATFORM_H

////////////////////////////////////////////////////////////////////////////////

#include <cstdint>
#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#include <memory>
#include <windows.h>

using string_type = std::wstring;
using char_type   = wchar_t;

#define STR_LITERAL(s) L##s

static inline string_type convert_string(bool is_utf8, const char* s)
{
	const UINT codepage = (is_utf8 ? CP_UTF8 : CP_ACP);

	const int wstr_size = MultiByteToWideChar(codepage, 0, s, -1, NULL, 0);
	if (wstr_size == 0)
		return L"";

	std::unique_ptr<wchar_t[]> wstr(new wchar_t[wstr_size]);
	if (MultiByteToWideChar(codepage, 0, s, -1, wstr.get(), wstr_size) == 0)
		return L"";

	return wstr.get();
}

static inline string_type convert_string(bool is_utf8, std::string&& s)
{
	return convert_string(is_utf8, s.c_str());
}

#else

using string_type = std::string;
using char_type   = char;

#define STR_LITERAL(s) s

static inline string_type convert_string(bool is_utf8, const char* s)
{
	return s;
}

static inline string_type convert_string(bool is_utf8, std::string&& s)
{
	return std::move(s);
}

#endif

////////////////////////////////////////////////////////////////////////////////

extern const char_type g_directory_sep;

bool platform_init(void);
void platform_deinit(void);

string_type get_full_path(const string_type& path);
std::vector<uint8_t> get_cmd_output(const char_type* dir, const char_type* p_cmd);
void remove_files(const std::vector<string_type>& files);

////////////////////////////////////////////////////////////////////////////////

#endif
