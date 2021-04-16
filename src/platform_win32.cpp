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

#include "targetver.h"
#include <cassert>
#include <cstdint>
#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>
#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <atlbase.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern const wchar_t g_directory_sep = L'\\';
extern const wchar_t g_directory_sep_other = L'/';

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool platform_init(void)
{
#ifndef NDEBUG
	_set_error_mode(_OUT_TO_MSGBOX);
#endif

	return SUCCEEDED(CoInitialize(nullptr));
}

void platform_deinit(void)
{
	CoUninitialize();
}

std::wstring get_full_path(const std::wstring& path)
{
	std::wstring full_path;

	if (PathIsRelative(path.c_str())) {
		std::vector<wchar_t> path_buf;

		do {
			path_buf.resize(path_buf.size() + MAX_PATH);
			GetFullPathName(path.c_str(), static_cast<DWORD>(path_buf.size()), path_buf.data(), nullptr);
		} while (GetLastError() == ERROR_SUCCESS && path_buf[0] == L'\0');

		full_path = path_buf.data();
	} else {
		full_path = path;
	}

	std::replace(full_path.begin(), full_path.end(), L'/', L'\\');

	while (!full_path.empty() && full_path.back() == L'\\')
		full_path.pop_back();

	return full_path;
}

bool run_cmd(const wchar_t* p_dir, const wchar_t* p_cmd, std::vector<uint8_t>* p_output)
{
	bool status = false;

	HANDLE h_stdout_read = INVALID_HANDLE_VALUE, h_stdout_write = INVALID_HANDLE_VALUE;
	if (p_output) {
		SECURITY_ATTRIBUTES stdout_sec_attrs = { };
		stdout_sec_attrs.nLength = sizeof(SECURITY_ATTRIBUTES);
		stdout_sec_attrs.bInheritHandle = TRUE;

		if (!CreatePipe(&h_stdout_read, &h_stdout_write, &stdout_sec_attrs, 0))
			return status;

		SetHandleInformation(h_stdout_read, HANDLE_FLAG_INHERIT, 0);
	}

	STARTUPINFO si = { sizeof(STARTUPINFO) };
	si.dwFlags     = STARTF_USESTDHANDLES;
	si.hStdInput   = GetStdHandle(STD_INPUT_HANDLE);
	si.hStdError   = GetStdHandle(STD_ERROR_HANDLE);
	si.hStdOutput  = p_output ? h_stdout_write : GetStdHandle(STD_OUTPUT_HANDLE);

	std::wstring cmd = p_cmd;

	PROCESS_INFORMATION pi = { };
	if (CreateProcess(nullptr, const_cast<wchar_t*>(cmd.c_str()), nullptr, nullptr, TRUE, 0, nullptr, p_dir, &si, &pi)) {
		if (h_stdout_write != INVALID_HANDLE_VALUE) {
			CloseHandle(h_stdout_write);
			h_stdout_write = INVALID_HANDLE_VALUE;
		}

		if (p_output) {
			std::array<uint8_t, BUFSIZ> read_block;

			DWORD nbytes;
			while (ReadFile(h_stdout_read, read_block.data(), static_cast<DWORD>(read_block.size()), &nbytes, nullptr) && nbytes != 0)
				p_output->insert(p_output->end(), read_block.begin(), read_block.begin() + nbytes);
		}

		WaitForSingleObject(pi.hProcess, INFINITE);

		DWORD exit_code;
		GetExitCodeProcess(pi.hProcess, &exit_code);

		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);

		status = (exit_code == 0);
		assert(status);
	}

	if (h_stdout_write != INVALID_HANDLE_VALUE)
		CloseHandle(h_stdout_write);

	if (h_stdout_read != INVALID_HANDLE_VALUE)
		CloseHandle(h_stdout_read);

	return status;
}

void remove_files(const std::vector<std::wstring>& files)
{
	std::vector<PIDLIST_ABSOLUTE> items;
	items.reserve(files.size());

	for (auto& file : files) {
		PIDLIST_ABSOLUTE item;
		if (SUCCEEDED(SHParseDisplayName(file.c_str(), nullptr, &item, 0, nullptr)))
			items.push_back(item);
	}

	if (items.empty())
		return;

	CComPtr<IShellItemArray> p_items;
	if (SUCCEEDED(SHCreateShellItemArrayFromIDLists((UINT)items.size(), items.data(), &p_items))) {
		CComPtr<IFileOperation> p_file_op;
		if (SUCCEEDED(p_file_op.CoCreateInstance(CLSID_FileOperation)))
			if (SUCCEEDED(p_file_op->DeleteItems(p_items)))
				p_file_op->PerformOperations();
	}

	for (auto& item : items)
		CoTaskMemFree(item);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
