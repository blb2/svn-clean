#ifdef _WIN32
#include "targetver.h"
#endif

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <array>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>

#define strcasecmp _strnicmp
#else
#include <strings.h>
#endif

#include "externals/rapidxml/rapidxml.hpp"

////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

std::string get_full_path(const char* path)
{
	std::string full_path;

	if (PathIsRelative(path)) {
		std::vector<char> path_buf;

		do {
			path_buf.resize(path_buf.size() + MAX_PATH);
			GetFullPathName(path, static_cast<DWORD>(path_buf.size()), path_buf.data(), nullptr);
		} while (GetLastError() == ERROR_SUCCESS && path_buf[0] == '\0');

		full_path = path_buf.data();
	} else {
		full_path = path;
	}

	std::replace(full_path.begin(), full_path.end(), '/', '\\');

	while (!full_path.empty() && full_path.back() == '\\')
		full_path.pop_back();

	return full_path;
}

std::vector<uint8_t> get_cmd_output(const char* dir, const char* p_cmd)
{
	std::vector<uint8_t> output;

	if (SetCurrentDirectory(dir)) {
		SECURITY_ATTRIBUTES stdout_sec_attrs = { };
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

			std::string cmd = p_cmd;
			std::array<uint8_t, BUFSIZ> read_block;

			PROCESS_INFORMATION pi = { };
			if (CreateProcess(nullptr, const_cast<char*>(cmd.c_str()), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
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

#else

std::string get_full_path(const char* path)
{
	std::string full_path;

	char* p_full_path = realpath(path);
	if (p_full_path) {
		full_path = p_full_path;
		free(p_full_path);
	} else {
		full_path = path;
	}

	std::replace(full_path.begin(), full_path.end(), '/', '\\');

	while (!full_path.empty() && full_path.back() == '\\')
		full_path.pop_back();

	return full_path;
}

std::vector<uint8_t> get_cmd_output(const char* dir, const char* p_cmd)
{
	return std::vector<uint8_t>();
}

#endif

template <bool name_compare=false>
static inline bool is_equal(const rapidxml::xml_base<>* p_xml_obj, const char* b)
{
	if (name_compare)
		return strcasecmp(p_xml_obj->name(), b, p_xml_obj->name_size()) == 0;
	else
		return strcasecmp(p_xml_obj->value(), b, p_xml_obj->value_size()) == 0;
}

int main(int argc, char* argv[])
{
	bool dry_run = false;
	bool ignore_externals = false;

	int i;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-')
			break;

		if (strcmp(argv[i], "-n") == 0)
			dry_run = true;
		else if (strcmp(argv[i], "-x") == 0)
			ignore_externals = true;
	}

	std::string svn_cmd = "svn status --xml --no-ignore";
	if (ignore_externals)
		svn_cmd += " --ignore-externals";

	std::vector<const char*> dirs(argv + i, argv + argc);
	if (dirs.empty())
		dirs.emplace_back(".");

	for (auto& dir : dirs) {
		const std::string working_dir = get_full_path(dir);

		std::vector<uint8_t> svn_out = get_cmd_output(working_dir.c_str(), svn_cmd.c_str());
		if (svn_out.empty())
			continue;

		if (svn_out.back() != '\0')
			svn_out.push_back('\0');

		rapidxml::xml_document<> xml;
		try {
			xml.parse<rapidxml::parse_fastest>(reinterpret_cast<char*>(svn_out.data()));
		} catch (...) {
			xml.clear();
		}

		std::vector<std::string> files;

		rapidxml::xml_node<>* p_root_node = xml.first_node("status");
		if (p_root_node) {
			rapidxml::xml_node<>* p_target_node = p_root_node->first_node("target");

			if (p_target_node) {
				rapidxml::xml_node<>* p_entry_node = p_target_node->first_node("entry");

				while (p_entry_node) {
					rapidxml::xml_node<>* p_status_node = p_entry_node->first_node("wc-status");

					if (p_status_node) {
						rapidxml::xml_attribute<>* p_item_attr = p_status_node->first_attribute("item");

						if (p_item_attr && (is_equal(p_item_attr, "unversioned") || is_equal(p_item_attr, "ignored"))) {
							rapidxml::xml_attribute<>* p_path_attr = p_entry_node->first_attribute("path");

							if (p_path_attr)
								files.emplace_back(p_path_attr->value(), p_path_attr->value() + p_path_attr->value_size());
						}
					}

					p_entry_node = p_entry_node->next_sibling("entry");
				}
			}
		}

		for (auto& file : files) {
			if (dry_run) {
				std::cout << "Would remove file: " << file << '\n';
			} else {
				std::cout << "Removed file: " << file << '\n';
			}
		}

		// TODO: delete unversioned files
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
