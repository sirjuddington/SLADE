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

if(COMMAND cmake_policy)
	cmake_policy( SET CMP0011 NEW )
endif(COMMAND cmake_policy)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

set(SLADE_OUTPUT_DIR ${CMAKE_BINARY_DIR} CACHE PATH "Directory where slade will be created.")

OPTION(NO_WEBVIEW "Disable wxWebview usage (for start page and documentation)" OFF)
OPTION(USE_SFML_RENDERWINDOW "Use SFML RenderWindow for OpenGL displays" OFF)

add_subdirectory(src)
add_subdirectory(dist)
