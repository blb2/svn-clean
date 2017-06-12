#include <cstring>
#include <iostream>
#include <string>
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

		if (dry_run) {
			for (auto& file : files)
				std::cout << "Would remove file: " << file << '\n';
		} else {
			remove_files(files);
		}
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
