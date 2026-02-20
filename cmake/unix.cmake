# ------------------------------------------------------------------------------
# CMake to build SLADE in Linux/MacOS using system-installed dependencies
# ------------------------------------------------------------------------------

# Additional Find* cmake modules for linux/macos dependencies
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/find_modules)

# Where to put the built slade binary and pk3 file
set(SLADE_OUTPUT_DIR ${CMAKE_BINARY_DIR} CACHE PATH "Directory where slade will be created.")

# wxWidgets
SET(WX_LIBS std aui gl stc propgrid)
find_package(wxWidgets ${WX_VERSION} COMPONENTS ${WX_LIBS} REQUIRED)
include(${wxWidgets_USE_FILE})

# SFML
set(SFML_FIND_COMPONENTS System Audio Window Network)
list(TRANSFORM SFML_FIND_COMPONENTS TOLOWER OUTPUT_VARIABLE SFML2_FIND_COMPONENTS)
find_package(SFML 2 QUIET COMPONENTS ${SFML2_FIND_COMPONENTS})
if (SFML_FOUND)
	list(TRANSFORM SFML2_FIND_COMPONENTS PREPEND sfml- OUTPUT_VARIABLE SFML_LIBRARIES)
else ()
	list(TRANSFORM SFML_FIND_COMPONENTS PREPEND SFML:: OUTPUT_VARIABLE SFML_LIBRARIES)
	find_package(SFML 3 COMPONENTS ${SFML_FIND_COMPONENTS} REQUIRED)
endif ()

# Fluidsynth
if (NO_FLUIDSYNTH)
	ADD_DEFINITIONS(-DNO_FLUIDSYNTH)
endif (NO_FLUIDSYNTH)

if (CMAKE_INSTALL_PREFIX)
	ADD_DEFINITIONS(-DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
endif (CMAKE_INSTALL_PREFIX)

if (NOT NO_FLUIDSYNTH)
	find_package(FluidSynth REQUIRED)
else (NO_FLUIDSYNTH)
	message(STATUS "Fluidsynth support is disabled.")
endif ()

find_package(OpenGL REQUIRED)
if (NOT NO_LUA)
	find_package(Lua REQUIRED)
endif ()
find_package(MPG123 REQUIRED)
find_package(WebP REQUIRED)
find_package(PNG REQUIRED)
find_package(glm REQUIRED)
find_package(Freetype REQUIRED)
find_package(libxmp CONFIG REQUIRED)

# Cpptrace
include(FetchContent)
FetchContent_Declare(
	cpptrace
	GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
	GIT_TAG v1.0.4
)
FetchContent_MakeAvailable(cpptrace)

include_directories(
	${FREETYPE_INCLUDE_DIRS}
	${LUA_INCLUDE_DIR}
	${MPG123_INCLUDE_DIR}
	${WebP_INCLUDE_DIRS}
	.
	..
	../thirdparty/glad/include
	./Application
)

if (NOT NO_FLUIDSYNTH)
	include_directories(${FLUIDSYNTH_INCLUDE_DIR})
endif ()

if (APPLE)
	set(OSX_ICON "${CMAKE_SOURCE_DIR}/SLADE-osx.icns")
	set(OSX_PLIST "${CMAKE_SOURCE_DIR}/Info.plist")

	set(SLADE_SOURCES ${SLADE_SOURCES} ${OSX_ICON} ${OSX_PLIST})

	set_source_files_properties(${OSX_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif (APPLE)

# Enable SSE instructions for dumb library
include(CheckCCompilerFlag)
check_c_compiler_flag(-msse HAVE_SSE)
if (HAVE_SSE)
	add_compile_options(-msse)
	add_definitions(-D_USE_SSE)
endif ()

# Define a SLADE_DEBUG macro
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DSLADE_DEBUG")

# (Optionally) Enable debug symbols for glib (so gdb debugging works properly with strings etc.)
OPTION(GLIB_DEBUG "Enable glib debug symbols" OFF)
if (GLIB_DEBUG)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG")
endif (GLIB_DEBUG)

# Enable AddressSanitizer if option enabled
if (USE_SANITIZER)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif (USE_SANITIZER)

# External libraries are compiled separately to enable unity builds
add_subdirectory(../thirdparty external)

add_executable(slade WIN32 MACOSX_BUNDLE
	${SLADE_SOURCES}
	${SLADE_HEADERS}
)

target_link_libraries(slade
	${ZLIB_LIBRARY}
	${BZIP2_LIBRARIES}
	${EXTERNAL_LIBRARIES}
	${wxWidgets_LIBRARIES}
	${SFML_LIBRARIES}
	${FREETYPE_LIBRARIES}
	${OPENGL_LIBRARIES}
	${LUA_LIBRARIES}
	${MPG123_LIBRARIES}
	${WebP_LIBRARIES}
	glm::glm
	cpptrace::cpptrace
	PNG::PNG
	libxmp::xmp_shared
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION LESS 9)
	target_link_libraries(slade -lstdc++fs)
endif ()

if (NOT NO_FLUIDSYNTH)
	target_link_libraries(slade ${FLUIDSYNTH_LIBRARIES})
endif ()

set_target_properties(slade PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${SLADE_OUTPUT_DIR})

# TODO: Installation targets for APPLE
if (APPLE)
	set_target_properties(slade PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${OSX_PLIST})

	add_custom_command(TARGET slade POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SLADE_OUTPUT_DIR}/slade.pk3" "$<TARGET_FILE_DIR:slade>/slade.pk3"
	)
else (APPLE)
	if (UNIX)
		install(TARGETS slade
			RUNTIME DESTINATION bin
		)

		if (BUILD_PK3)
			install(FILES "${SLADE_OUTPUT_DIR}/slade.pk3"
				DESTINATION share/slade3
			)
		endif ()

		install(FILES "${PROJECT_SOURCE_DIR}/dist/res/icons/general/logo.svg"
			DESTINATION share/icons/hicolor/scalable/apps/
			RENAME net.mancubus.SLADE.svg
		)

		install(FILES "${PROJECT_SOURCE_DIR}/net.mancubus.SLADE.desktop"
			DESTINATION share/applications/
		)

		install(FILES "${PROJECT_SOURCE_DIR}/net.mancubus.SLADE.metainfo.xml"
			DESTINATION share/metainfo/
		)
	endif (UNIX)
endif (APPLE)

# uninstall target
if (NOT TARGET uninstall)
	configure_file(
		"${PROJECT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
		"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
		IMMEDIATE @ONLY)

	add_custom_target(uninstall
		COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake COMMENT "Uninstall the project...")
endif ()

# Precompiled Header
target_precompile_headers(slade PRIVATE "Application/Main.h")
