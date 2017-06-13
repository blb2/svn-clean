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

static inline std::wstring convert_string(bool is_utf8, const char* p_str)
{
	UINT codepage = (is_utf8 ? CP_UTF8 : CP_ACP);
	int  wstr_len = MultiByteToWideChar(codepage, 0, p_str, -1, nullptr, 0);

	std::wstring wstr;

	if (wstr_len > 0) {
		std::unique_ptr<wchar_t[]> p_wstr(new wchar_t[wstr_len]);

		wstr_len = MultiByteToWideChar(codepage, 0, p_str, -1, p_wstr.get(), wstr_len);
		assert(wstr_len != 0);

		wstr = p_wstr.get();
	}

	assert(*p_str == '\0' || !wstr.empty());
	return wstr;
}

static inline std::wstring convert_string(bool is_utf8, std::string&& str)
{
	return convert_string(is_utf8, str.c_str());
}

#else

using string_type = std::string;
using char_type   = char;

#define STR_LITERAL(s) s

static inline string_type convert_string(bool is_utf8, const char* p_str)
{
	return p_str;
}

static inline string_type convert_string(bool is_utf8, std::string&& str)
{
	return std::move(str);
}

#endif

////////////////////////////////////////////////////////////////////////////////

extern const char_type g_directory_sep;

bool platform_init(void);
void platform_deinit(void);

string_type get_full_path(const string_type& path);
std::vector<uint8_t> get_cmd_output(const char_type* p_dir, const char_type* p_cmd);
void remove_files(const std::vector<string_type>& files);

////////////////////////////////////////////////////////////////////////////////

#endif
