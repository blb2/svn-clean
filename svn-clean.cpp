#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include "externals/rapidxml/rapidxml.hpp"
#include "externals/rapidxml/rapidxml_print.hpp"
#include "platform.h"

#ifdef _WIN32
#define strncasecmp _strnicmp
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
		return strncasecmp(p_xml_obj->value(), p_str, p_xml_obj->value_size()) == 0;
}

template <typename T>
std::vector<string_type> parse_for_files(const string_type& working_dir, rapidxml::xml_document<T>& xdoc)
{
	std::vector<string_type> files;

	rapidxml::xml_node<T>* p_root_node = xdoc.first_node("status");
	if (p_root_node) {
		rapidxml::xml_node<T>* p_target_node = p_root_node->first_node("target");

		if (p_target_node) {
			rapidxml::xml_node<T>* p_entry_node = p_target_node->first_node("entry");

			while (p_entry_node) {
				rapidxml::xml_node<T>* p_status_node = p_entry_node->first_node("wc-status");

				if (p_status_node) {
					rapidxml::xml_attribute<T>* p_item_attr = p_status_node->first_attribute("item");

					if (p_item_attr && (is_equal(p_item_attr, "unversioned") || is_equal(p_item_attr, "ignored"))) {
						rapidxml::xml_attribute<T>* p_path_attr = p_entry_node->first_attribute("path");

						if (p_path_attr && p_path_attr->value_size() != 0)
							files.push_back(working_dir + g_directory_sep + convert_string(true, std::string(p_path_attr->value(), p_path_attr->value() + p_path_attr->value_size())));
					}
				}

				p_entry_node = p_entry_node->next_sibling("entry");
			}
		}
	}

	return files;
}

int main(int argc, char* argv[])
{
	bool debug = false;
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
		else if (strcmp(argv[i], "-d") == 0)
			debug = true;
	}

	string_type svn_cmd = STR_LITERAL("svn status --xml --no-ignore");
	if (ignore_externals)
		svn_cmd += STR_LITERAL(" --ignore-externals");

	std::vector<string_type> dirs;
	while (i < argc)
		dirs.push_back(convert_string(false, argv[i++]));

	if (dirs.empty())
		dirs.emplace_back(STR_LITERAL("."));

	rapidxml::xml_document<> xdoc;
	for (auto& dir : dirs) {
		const string_type working_dir = get_full_path(dir);

		std::vector<uint8_t> svn_xml = get_cmd_output(working_dir.c_str(), svn_cmd.c_str());
		if (svn_xml.empty())
			continue;

		if (svn_xml.back() != '\0')
			svn_xml.push_back('\0');

		try {
			xdoc.parse<rapidxml::parse_fastest>(reinterpret_cast<char*>(svn_xml.data()));
		} catch (...) {
			xdoc.clear();
		}

		if (debug) {
			std::string xml;
			rapidxml::print(std::back_inserter(xml), xdoc);
			printf("dir: " STR_FMT "\n", working_dir.c_str());
			printf("cmd: " STR_FMT "\n", svn_cmd.c_str());
			printf("%s\n", xml.c_str());
			continue;
		}

		std::vector<string_type> files = parse_for_files(working_dir, xdoc);

		if (dry_run) {
			for (auto& file : files)
				printf(STR_FMT "\n", file.c_str());
		} else {
			remove_files(files);
		}
	}

	platform_deinit();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
