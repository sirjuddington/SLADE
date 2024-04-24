cmake_minimum_required(VERSION 3.15) # Minimum CMake 3.15 for precompiled header support
project(SLADE VERSION 3.2.0)

# Additional cmake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Release build by default
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

# Build options
OPTION(NO_LUA "Disable Lua/Scripting features to reduce compile time" OFF)
OPTION(NO_FLUIDSYNTH "Disable FluidSynth MIDI playback" OFF)
OPTION(BUILD_PK3 "Build the SLADE pk3 file from dist/res" ON)

# c++17 is required to compile
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_subdirectory(src)

if (BUILD_PK3)
	add_subdirectory(dist)
endif (BUILD_PK3)
