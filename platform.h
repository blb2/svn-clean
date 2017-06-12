#ifndef SVNCLEAN_PLATFORM_H
#define SVNCLEAN_PLATFORM_H

////////////////////////////////////////////////////////////////////////////////

#include <cstdint>
#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////

std::string get_full_path(const char* path);
std::vector<uint8_t> get_cmd_output(const char* dir, const char* p_cmd);
void remove_files(const std::vector<std::string>& files);

////////////////////////////////////////////////////////////////////////////////

#endif
