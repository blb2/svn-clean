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

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <array>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ftw.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern const char g_directory_sep       = '/';
extern const char g_directory_sep_other = '\\';

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool platform_init(void)
{
	return true;
}

void platform_deinit(void)
{
}

std::string get_full_path(const std::string& path)
{
	std::string full_path;

	char* p_full_path = realpath(path.c_str(), nullptr);
	if (p_full_path) {
		full_path = p_full_path;
		free(p_full_path);
	} else {
		full_path = path;
	}

	std::replace(full_path.begin(), full_path.end(), '\\', '/');

	while (!full_path.empty() && full_path.back() == '/')
		full_path.pop_back();

	return full_path;
}

bool run_cmd(const char* p_dir, const char* p_cmd, std::vector<uint8_t>* p_output)
{
	bool status = false;

	int fd[2];
	if (p_output && pipe(fd) < 0)
		return status;

	std::string cmd = p_cmd;
	std::vector<char*> argv = { const_cast<char*>(cmd.c_str()) };

	size_t pos = cmd.find(' ');
	while (pos != std::string::npos) {
		cmd[pos++] = '\0';

		while (pos != cmd.length() && cmd[pos] == ' ')
			cmd[pos++] = '\0';

		if (pos == cmd.length())
			break;

		if (cmd[pos] == '"') {
			argv.push_back(const_cast<char*>(cmd.c_str() + ++pos));
			pos = cmd.find('"', pos);
		} else {
			argv.push_back(const_cast<char*>(cmd.c_str() + pos));
			pos = cmd.find(' ', pos);
		}
	}

	argv.push_back(nullptr);

	pid_t cmd_pid = fork();
	if (cmd_pid == 0) {
		if (p_output) {
			if (dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO)
				exit(EXIT_FAILURE);

			close(fd[0]);
			close(fd[1]);
		}

		if (chdir(p_dir) == 0)
			execvp(cmd.c_str(), argv.data());

		exit(EXIT_FAILURE);
	} else if (cmd_pid < 0) {
		if (p_output) {
			close(fd[0]);
			close(fd[1]);
		}
	} else {
		if (p_output) {
			close(fd[1]);

			std::array<uint8_t, BUFSIZ> read_block;
			while (true) {
				ssize_t nbytes = read(fd[0], read_block.data(), read_block.size());

				if (nbytes < 0) {
					if (errno == EINTR)
						continue;

					break;
				} else if (nbytes == 0) {
					break;
				}

				p_output->insert(p_output->end(), read_block.begin(), read_block.begin() + nbytes);
			}
		}

		int exit_code;
		waitpid(cmd_pid, &exit_code, 0);

		if (p_output)
			close(fd[0]);

		status = (exit_code == 0);
		assert(status);
	}

	return status;
}

int nftw_cb(const char* p_path, const struct stat* p_stat, int flags, struct FTW* p_ftw)
{
	if (flags == FTW_D || flags == FTW_DP) {
		printf("rmdir %s\n", p_path);
		return rmdir(p_path);
	}

	if (flags == FTW_F || flags == FTW_SL) {
		printf("unlink %s\n", p_path);
		return unlink(p_path);
	}

	return -1;
}

void remove_files(const std::vector<std::string>& files)
{
	for (auto& file : files)
		if (nftw(file.c_str(), nftw_cb, 64, FTW_DEPTH | FTW_PHYS) < 0 && errno == ENOTDIR)
			nftw_cb(file.c_str(), nullptr, FTW_F, nullptr);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
