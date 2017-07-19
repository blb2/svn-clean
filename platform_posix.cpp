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

////////////////////////////////////////////////////////////////////////////////

extern const char g_directory_sep = '/';
extern const char g_directory_sep_other = '\\';

////////////////////////////////////////////////////////////////////////////////

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

std::vector<uint8_t> get_cmd_output(const char* p_dir, const char* p_cmd)
{
	std::vector<uint8_t> output;

	int fd[2];
	if (pipe(fd) == 0) {
		std::string cmd = p_cmd;
		std::vector<char*> argv = { const_cast<char*>(cmd.c_str()) };

		size_t space_pos = cmd.find(' ');
		while (space_pos != std::string::npos) {
			cmd[space_pos++] = '\0';
			argv.push_back(const_cast<char*>(cmd.c_str() + space_pos));
			space_pos = cmd.find(' ', space_pos);
		}

		argv.push_back(nullptr);

		pid_t svn_pid = fork();
		if (svn_pid == 0) {
			if (dup2(fd[1], STDOUT_FILENO) == STDOUT_FILENO) {
				close(fd[0]);
				close(fd[1]);

				if (chdir(p_dir) == 0)
					execvp(cmd.c_str(), argv.data());
			}

			exit(EXIT_FAILURE);
		} else if (svn_pid < 0) {
			// TODO: error forking process
			close(fd[0]);
			close(fd[1]);
		} else {
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

				output.insert(output.end(), read_block.begin(), read_block.begin() + nbytes);
			}

			waitpid(svn_pid, nullptr, 0);
			close(fd[0]);
		}
	} else {
		// TODO: error creating pipe
	}

	return output;
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

////////////////////////////////////////////////////////////////////////////////
