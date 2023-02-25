/*
 * Copyright (C) 2017-2023, Brian Brice. All rights reserved.
 *
 * This file is part of svn-clean.
 *
 * svn-clean is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * svn-clean is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with svn-clean.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <vector>
#include "../external/pugixml/src/pugixml.hpp"
#include "platform.h"
#include "version.h"

struct xml_writer : public pugi::xml_writer {
	const std::string& str(void) const
	{
		return m_xml;
	}

	virtual void write(const void* data, size_t size) override
	{
		m_xml.append(reinterpret_cast<const char*>(data), size);
	}

private:
	std::string m_xml;
};

std::vector<string_type> find_unversioned_files(const string_type& working_dir, const pugi::xml_document& xdoc)
{
	std::vector<string_type> files;

	const char* xpath_query = "/status/target/entry[./wc-status/@item=\"unversioned\" or ./wc-status/@item=\"ignored\"]/@path";
	for (auto& node : xdoc.select_nodes(xpath_query)) {
		if (node.attribute()) {
			string_type path = working_dir + g_directory_sep + convert_string(true, std::string(node.attribute().value()));

			if (g_directory_sep != g_directory_sep_other)
				std::replace(path.begin(), path.end(), g_directory_sep_other, g_directory_sep);

			files.push_back(std::move(path));
		}
	}

	return files;
}

std::vector<string_type> find_externals(const pugi::xml_document& xdoc)
{
	std::vector<string_type> files;

	const char* xpath_query = "/status/target/entry[./wc-status/@item=\"external\"]/@path";
	for (auto& node : xdoc.select_nodes(xpath_query)) {
		if (node.attribute()) {
			string_type path = convert_string(true, std::string(node.attribute().value()));

			if (g_directory_sep != g_directory_sep_other)
				std::replace(path.begin(), path.end(), g_directory_sep_other, g_directory_sep);

			files.push_back(std::move(path));
		}
	}

	return files;
}

int main(int argc, char* argv[])
{
	puts("svn-clean v" SVNCLEAN_VERSION_SLIM_STR "\n");

	bool debug   = false;
	bool dry_run = false;
	bool revert  = false;

	if (!platform_init())
		return EXIT_FAILURE;

	int i;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-')
			break;

		if (strcmp(argv[i], "-n") == 0)
			dry_run = true;
		else if (strcmp(argv[i], "-d") == 0)
			debug = true;
		else if (strcmp(argv[i], "-r") == 0)
			revert = true;
	}

	std::vector<string_type> dirs;
	while (i < argc)
		dirs.push_back(convert_string(false, argv[i++]));

	if (dirs.empty())
		dirs.emplace_back(STR_LITERAL("."));

	pugi::xml_document xdoc;
	for (auto& dir : dirs) {
		const string_type svn_status_cmd = STR_LITERAL("svn status --xml --no-ignore");
		const string_type working_dir    = get_full_path(dir);

		std::vector<uint8_t> svn_status_xml;
		if (!run_cmd(working_dir.c_str(), svn_status_cmd.c_str(), &svn_status_xml) || svn_status_xml.empty())
			continue;

		if (svn_status_xml.back() != '\0')
			svn_status_xml.push_back('\0');

		if (!xdoc.load_buffer_inplace(svn_status_xml.data(), svn_status_xml.size()))
			xdoc.reset();

		if (debug) {
			xml_writer writer;
			xdoc.save(writer, "  ");

			printf("dir: " STR_FMT "\n", working_dir.c_str());
			printf("cmd: " STR_FMT "\n", svn_status_cmd.c_str());
			printf("revert: %s\n", (revert ? "true" : "false"));
			printf("%s\n", writer.str().c_str());
			continue;
		}

		std::vector<string_type> unversioned_files = find_unversioned_files(working_dir, xdoc);

		if (dry_run) {
			for (auto& file : unversioned_files)
				printf(STR_FMT "\n", file.c_str());
		} else {
			if (revert) {
				const string_type svn_revert_cmd_base = STR_LITERAL("svn revert --recursive ");

				std::vector<string_type> externals = find_externals(xdoc);
				for (auto& external : externals)
					run_cmd(working_dir.c_str(), (svn_revert_cmd_base + STR_LITERAL('"') + external + STR_LITERAL('"')).c_str());

				run_cmd(working_dir.c_str(), (svn_revert_cmd_base + STR_LITERAL(".")).c_str());
			}

			remove_files(unversioned_files);
		}
	}

	platform_deinit();
	return 0;
}
