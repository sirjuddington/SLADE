cmake_minimum_required(VERSION 3.15) # Minimum CMake 3.15 for precompiled header support
project(SLADE VERSION 3.2.0)

# Additional cmake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Release build by default
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

# Build options
OPTION(NO_WEBVIEW "Disable wxWebview usage (for start page and documentation)" OFF)
OPTION(NO_LUA "Disable Lua/Scripting features to reduce compile time" OFF)
OPTION(NO_FLUIDSYNTH "Disable FluidSynth MIDI playback" OFF)
OPTION(BUILD_PK3 "Build the SLADE pk3 file from dist/res" ON)
OPTION(BUILD_WX "Download and build wxWidgets instead of using system-installed libs" OFF)

# c++17 is required to compile
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if(BUILD_WX)
	set(wxWidgets_USE_STATIC 1)
	set(wxBUILD_SHARED OFF)
	set(wxBUILD_USE_STATIC_RUNTIME ON)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

	include(FetchContent)
	FetchContent_Declare(
	   wxWidgets
	   GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
	   GIT_TAG 5edd516add0051d6f51ec87b8001999b6c4035a6 # master (3.3.0) as of 2024-11-26
	)
	FetchContent_GetProperties(wxwidgets)
	if(NOT wxwidgets_POPULATED)
	   FetchContent_Populate(wxwidgets)
	   add_subdirectory(${wxwidgets_SOURCE_DIR} ${wxwidgets_BUILD_DIR})
	endif()
endif()

add_subdirectory(src)

if (BUILD_PK3)
	add_subdirectory(dist)
endif (BUILD_PK3)
