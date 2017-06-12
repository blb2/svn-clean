#include "targetver.h"
#include <cassert>
#include <cstdint>
#include <algorithm>
#include <array>
#include <string>
#include <vector>
#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <atlbase.h>

////////////////////////////////////////////////////////////////////////////////

extern const wchar_t g_directory_sep = L'\\';

////////////////////////////////////////////////////////////////////////////////

bool platform_init(void)
{
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

	std::replace(full_path.begin(), full_path.end(), L'/', g_directory_sep);

	while (!full_path.empty() && full_path.back() == g_directory_sep)
		full_path.pop_back();

	return full_path;
}

std::vector<uint8_t> get_cmd_output(const wchar_t* dir, const wchar_t* p_cmd)
{
	std::vector<uint8_t> output;

	if (SetCurrentDirectory(dir)) {
		SECURITY_ATTRIBUTES stdout_sec_attrs = {};
		stdout_sec_attrs.nLength = sizeof(SECURITY_ATTRIBUTES);
		stdout_sec_attrs.bInheritHandle = TRUE;

		HANDLE h_stdout_read = INVALID_HANDLE_VALUE, h_stdout_write = INVALID_HANDLE_VALUE;
		if (CreatePipe(&h_stdout_read, &h_stdout_write, &stdout_sec_attrs, 0)) {
			SetHandleInformation(h_stdout_read, HANDLE_FLAG_INHERIT, 0);

			STARTUPINFO si = { sizeof(STARTUPINFO) };
			si.dwFlags = STARTF_USESTDHANDLES;
			si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
			si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
			si.hStdOutput = h_stdout_write;

			std::wstring cmd = p_cmd;
			std::array<uint8_t, BUFSIZ> read_block;

			PROCESS_INFORMATION pi = {};
			if (CreateProcess(nullptr, const_cast<wchar_t*>(cmd.c_str()), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
				CloseHandle(h_stdout_write);
				h_stdout_write = INVALID_HANDLE_VALUE;

				DWORD num_bytes_read;
				while (ReadFile(h_stdout_read, read_block.data(), static_cast<DWORD>(read_block.size()), &num_bytes_read, nullptr) && num_bytes_read != 0)
					output.insert(output.end(), read_block.begin(), read_block.begin() + num_bytes_read);

				WaitForSingleObject(pi.hProcess, INFINITE);

				DWORD exit_code;
				GetExitCodeProcess(pi.hProcess, &exit_code);
				assert(exit_code == 0);

				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
			} else {
				// TODO: error executing command
			}

			if (h_stdout_write != INVALID_HANDLE_VALUE)
				CloseHandle(h_stdout_write);

			if (h_stdout_read != INVALID_HANDLE_VALUE)
				CloseHandle(h_stdout_read);
		}
	} else {
		// TODO: error changing working directory
	}

	return output;
}

void remove_files(const std::vector<std::wstring>& files)
{
	std::vector<PIDLIST_ABSOLUTE> items;
	items.reserve(files.size());

	for (auto& file : files) {
		PIDLIST_ABSOLUTE item = ILCreateFromPath(file.c_str());
		if (item)
			items.push_back(item);
	}

	if (items.empty())
		return;

	CComPtr<IShellItemArray> p_items;
	if (SUCCEEDED(SHCreateShellItemArrayFromIDLists(items.size(), items.data(), &p_items))) {
		CComPtr<IFileOperation> p_file_op;
		if (SUCCEEDED(p_file_op.CoCreateInstance(CLSID_FileOperation)))
			if (SUCCEEDED(p_file_op->DeleteItems(p_items)))
				p_file_op->PerformOperations();
	}

	for (auto& item : items)
		ILFree(item);
}

////////////////////////////////////////////////////////////////////////////////
