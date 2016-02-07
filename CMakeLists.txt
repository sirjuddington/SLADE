cmake_minimum_required(VERSION 2.4)
project(Slade3)

if(COMMAND cmake_policy)
	cmake_policy( SET CMP0012 NEW )
endif(COMMAND cmake_policy)

if(COMMAND cmake_policy)
	cmake_policy( SET CMP0003 NEW )
endif(COMMAND cmake_policy)

if(COMMAND cmake_policy)
	cmake_policy( SET CMP0005 NEW )
endif(COMMAND cmake_policy)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

set(SLADE_OUTPUT_DIR ${CMAKE_BINARY_DIR} CACHE PATH "Directory where slade will be created.")

find_package(ZLIB)
find_package(BZip2)
find_package(OpenGL)

if(NOT ZLIB_FOUND)
	set(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/zlib)
	add_subdirectory(src/zlib)
	set(ZLIB_LIBRARY z)
endif(NOT ZLIB_FOUND)

if(NOT BZIP2_FOUND)
	set(BZIP2_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bzip2)
	add_subdirectory(src/bzip2)
	set(BZIP2_LIBRARIES bz2)
endif(NOT BZIP2_FOUND)

OPTION(USE_WEBVIEW_STARTPAGE "Use wxWebView to display the start page" OFF)
OPTION(USE_SFML_RENDERWINDOW "Use SFML RenderWindow for OpenGL displays" OFF)

add_subdirectory(src)
add_subdirectory(dist)
