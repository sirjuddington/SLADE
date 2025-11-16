cmake_minimum_required(VERSION 3.15) # Minimum CMake 3.15 for precompiled header support
project(SLADE VERSION 3.2.0)

# Additional cmake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Release build by default
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif ()

# Build options
OPTION(NO_WEBVIEW "Disable wxWebview usage (for start page and documentation)" OFF)
OPTION(NO_LUA "Disable Lua/Scripting features to reduce compile time" OFF)
OPTION(NO_FLUIDSYNTH "Disable FluidSynth MIDI playback" OFF)
OPTION(BUILD_PK3 "Build the SLADE pk3 file from dist/res" ON)
OPTION(BUILD_WX "Download and build wxWidgets instead of using system-installed libs" OFF)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

# c++17 is required to compile
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Enable AddressSanitizer if requested
if (ENABLE_ASAN)
	add_compile_options(-fsanitize=address)
	add_link_options(-fsanitize=address)
	add_compile_definitions(_DISABLE_VECTOR_ANNOTATION _DISABLE_STRING_ANNOTATION)
endif ()

if (BUILD_WX)
	set(wxWidgets_USE_STATIC 1)
	set(wxBUILD_SHARED OFF)
	set(wxBUILD_USE_STATIC_RUNTIME ON)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

	include(FetchContent)
	FetchContent_Declare(
		wxWidgets
		GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
		GIT_TAG 08b10e373e8de34ec3f7439615e16a2288df0b35 # master as of 2025-11-10
		#GIT_TAG v3.3.1
	)
	FetchContent_GetProperties(wxwidgets)
	FetchContent_MakeAvailable(wxwidgets)
endif ()

add_subdirectory(src)

if (BUILD_PK3)
	add_subdirectory(dist)
endif (BUILD_PK3)
