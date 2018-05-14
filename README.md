svn-clean
=========
A utility for cleaning a SVN checkout by removing files that are not part of
the repository.

The current project page is located here:
<https://github.com/bbrice/svn-clean>

Building
--------
For building on Windows, a project file for Visual Studio 2017 is provided.

While building on a POSIX platform, such as Linux or macOS, the `make` command
along with any C++11 compliant compiler should do.  Here's an example of
building on a Linux system.

	gaia:~/svn-clean$ make
	g++ -std=c++11 -O3 -DNDEBUG -DPUGIXML_HEADER_ONLY -MMD -c -o obj/svn-clean.o svn-clean.cpp
	g++ -std=c++11 -O3 -DNDEBUG -DPUGIXML_HEADER_ONLY -MMD -c -o obj/platform_posix.o platform_posix.cpp
	g++ -std=c++11 -O3 -DNDEBUG -DPUGIXML_HEADER_ONLY -o svn-clean obj/svn-clean.o obj/platform_posix.o

After running `make`, the `svn-clean` executable is placed at the root of the
build directory.

If you'd like to override the compiler or compiler options, `CXX` or
`CXXFLAGS` can be specified for `make`:

	gaia:~/svn-clean$ make CXX=clang CXXFLAGS=-O2

If you'd like to do a debug build, `DEBUG` can be specified for `make`:

	gaia:~/svn-clean$ make DEBUG=1

Usage
-----
This application can take directory paths as arguments.  If no path is given,
then the current directory is used.

	gaia:~$ svn-clean svn-project1 svn-project2

On Windows, the shell is used to delete files.  Unfortunately, this means the
list of files being deleted will not be printed out.  However, the delete
process is actually just placing the files into the Recycle Bin.  On Linux or
macOS, the list of directories and files are printed out as they're deleted.

If you'd like to see a test run, or dry run, of what the command would do
without actually doing it, you can use the `-n` option:

	gaia:~$ svn-clean -n
	/home/user/project/Makefile~
	/home/user/project/README~

To revert as well as remove unversioned files, one can specify the `-r` option:

	gaia:~$ svn-clean -r

License
-------
`svn-clean` is licensed under the BSD license. See
[LICENSE.txt](LICENSE.txt) for more info.
