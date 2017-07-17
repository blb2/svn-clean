#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include "external/pugixml/src/pugixml.hpp"
#include "platform.h"

////////////////////////////////////////////////////////////////////////////////

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

std::vector<string_type> parse_for_files(const string_type& working_dir, const pugi::xml_document& xdoc)
{
	std::vector<string_type> files;

	const char* xpath_query = "/status/target/entry[./wc-status/@item=\"unversioned\" or ./wc-status/@item=\"ignored\"]/@path";
	for (auto& node : xdoc.select_nodes(xpath_query))
		if (node.attribute())
			files.push_back(working_dir + g_directory_sep + convert_string(true, std::string(node.attribute().value())));

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

	pugi::xml_document xdoc;
	for (auto& dir : dirs) {
		const string_type working_dir = get_full_path(dir);

		std::vector<uint8_t> svn_xml = get_cmd_output(working_dir.c_str(), svn_cmd.c_str());
		if (svn_xml.empty())
			continue;

		if (svn_xml.back() != '\0')
			svn_xml.push_back('\0');

		if (!xdoc.load_buffer_inplace(svn_xml.data(), svn_xml.size()))
			xdoc.reset();

		if (debug) {
			xml_writer writer;
			xdoc.save(writer, "  ");

			printf("dir: " STR_FMT "\n", working_dir.c_str());
			printf("cmd: " STR_FMT "\n", svn_cmd.c_str());
			printf("%s\n", writer.str().c_str());
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
