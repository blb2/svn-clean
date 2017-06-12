#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include "externals/rapidxml/rapidxml.hpp"
#include "platform.h"

#ifdef _WIN32
#define strcasecmp _strnicmp
#else
#include <strings.h>
#endif

////////////////////////////////////////////////////////////////////////////////

template <bool name_compare=false>
inline bool is_equal(const rapidxml::xml_base<>* p_xml_obj, const char* p_str)
{
	if (name_compare)
		return strncmp(p_xml_obj->name(), p_str, p_xml_obj->name_size()) == 0;
	else
		return strcasecmp(p_xml_obj->value(), p_str, p_xml_obj->value_size()) == 0;
}

int main(int argc, char* argv[])
{
	bool dry_run = false;
	bool ignore_externals = false;

	if (!platform_init())
		return EXIT_FAILURE;

	int i;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-')
			break;

		if (strcmp(argv[i], "-n") == 0)
			dry_run = true;
		else if (strcmp(argv[i], "-x") == 0)
			ignore_externals = true;
	}

	string_type svn_cmd = STR_LITERAL("svn status --xml --no-ignore");
	if (ignore_externals)
		svn_cmd += STR_LITERAL(" --ignore-externals");

	std::vector<string_type> dirs;
	for (i; i < argc; i++)
		dirs.push_back(convert_string(false, argv[i]));

	if (dirs.empty())
		dirs.emplace_back(STR_LITERAL("."));

	for (auto& dir : dirs) {
		const string_type working_dir = get_full_path(dir);

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

		std::vector<string_type> files;

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
								files.push_back(working_dir + g_directory_sep + convert_string(true, std::string(p_path_attr->value(), p_path_attr->value() + p_path_attr->value_size())));
						}
					}

					p_entry_node = p_entry_node->next_sibling("entry");
				}
			}
		}

		if (dry_run) {
			for (auto& file : files)
				std::wcout << "Would remove file: " << file << '\n';
		} else {
			remove_files(files);
		}
	}

	platform_deinit();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
