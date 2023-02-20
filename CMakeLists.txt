cmake_minimum_required(VERSION 3.16)
message(STATUS "CMake Version: ${CMAKE_VERSION}")
message(STATUS "CMake Generator: ${CMAKE_GENERATOR}")
message(STATUS "CMake Source Dir: ${CMAKE_SOURCE_DIR}")
message(STATUS "CMake Binary Dir: ${CMAKE_BINARY_DIR}")

# Not allowing mixing of the CMake source and build directories.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
	message(FATAL_ERROR "The source tree and build tree cannot be the same.")
endif()

# Allow usage of folders if supported.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(svn-clean VERSION 0.1.0 LANGUAGES CXX)

# This project requires at least C++11.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add options specifically for building with Visual Studio. Most of these were
# obtained by porting over the default settings that Visual Studio uses. These
# are set here in order to override what CMake sets as the default flags. For
# example, CMake hard-codes /Zi for builds with debugging information, however
# I want to use what VS uses -- /ZI.
if(MSVC)
	set(CMAKE_CXX_FLAGS "/W3 /WX- /sdl /EHsc /GS /permissive- /Zc:inline")
	set(CMAKE_CXX_FLAGS_DEBUG "/ZI /Od /RTC1")
	set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Oi /GL /Gy")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "/O1 /Oi /GL /Gy")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/Zi ${CMAKE_CXX_FLAGS_RELEASE}")
	set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG         OFF)
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE        ON)
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL     ON)
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)

	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/DEBUG")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/OPT:REF /OPT:ICF")
	set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
	set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/DEBUG ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
	set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
	set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
	set(CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL}")
	set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO}")
endif()

# Define the appropriate debug macros for every platform.
string(APPEND CMAKE_CXX_FLAGS_DEBUG          " /D_DEBUG ")
string(APPEND CMAKE_CXX_FLAGS_RELEASE        " /DNDEBUG ")
string(APPEND CMAKE_CXX_FLAGS_MINSIZEREL     " /DNDEBUG ")
string(APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO " /DNDEBUG ")

if(WIN32)
	# Target Windows 7 at a minimum.
	string(APPEND CMAKE_CXX_FLAGS " /D_WIN32_WINNT=0x0601")
	# Ensure the WIN32 macro is defined.
	string(APPEND CMAKE_CXX_FLAGS " /DWIN32")
	# Put the Win32 API onto a diet.
	string(APPEND CMAKE_CXX_FLAGS " /DWIN32_LEAN_AND_MEAN /DSTRICT /DSTRICT_TYPED_ITEMIDS /DNOMINMAX")
	# Ensure we're using Unicode APIs when building for Windows.
	string(APPEND CMAKE_CXX_FLAGS " /DUNICODE /D_UNICODE")
endif()

set(
	HEADERS
	src/platform.h
	src/version.h
)

set(SOURCES src/svn-clean.cpp)

if(WIN32)
	list(APPEND SOURCES src/platform_win32.cpp)
else()
	list(APPEND SOURCES src/platform_posix.cpp)
endif()

add_executable(svn-clean ${SOURCES} ${HEADERS})
target_compile_definitions(svn-clean PRIVATE PUGIXML_HEADER_ONLY)

if(WIN32)
	target_link_libraries(svn-clean PRIVATE shlwapi)
endif()
