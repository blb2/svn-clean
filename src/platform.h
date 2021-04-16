/*
 * Copyright (C) 2020-2021, Brian Brice. All rights reserved.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SVNCLEAN_PLATFORM_H
#define SVNCLEAN_PLATFORM_H

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cstdint>
#include <string>
#include <vector>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#include <memory>
#include <windows.h>

using string_type = std::wstring;
using char_type   = wchar_t;

#define STR_LITERAL(s) L##s
#define STR_FMT "%S"

static inline std::wstring convert_string(bool is_utf8, const char* p_str)
{
	UINT codepage = is_utf8 ? CP_UTF8 : CP_ACP;
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
#define STR_FMT "%s"

static inline string_type convert_string(bool is_utf8, const char* p_str)
{
	return p_str;
}

static inline string_type convert_string(bool is_utf8, std::string&& str)
{
	return std::move(str);
}

#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern const char_type g_directory_sep;
extern const char_type g_directory_sep_other;

bool platform_init(void);
void platform_deinit(void);

string_type get_full_path(const string_type& path);
bool run_cmd(const char_type* p_dir, const char_type* p_cmd, std::vector<uint8_t>* p_output = nullptr);
void remove_files(const std::vector<string_type>& files);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif
