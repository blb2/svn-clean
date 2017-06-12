#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////

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
	// TODO: execute command and capture its output
	return std::vector<uint8_t>();
}

void remove_files(const std::vector<std::string>& files)
{
	// TODO: delete unversioned files
}

////////////////////////////////////////////////////////////////////////////////
