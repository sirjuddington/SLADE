# ------------------------------------------------------------------------------
# CMake to build SLADE in Linux/MacOS using system-installed dependencies
# ------------------------------------------------------------------------------

# Additional Find* cmake modules for linux/macos dependencies
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/find_modules)

set(SLADE_OUTPUT_DIR ${CMAKE_BINARY_DIR} CACHE PATH "Directory where slade will be created.")

if(NOT APPLE)
	OPTION(WX_GTK3 "Use GTK3 (if wx is built with it)" ON)
endif(NOT APPLE)

if (NOT NO_COTIRE)
	include(cotire)
endif()

# wxWidgets libs
if (WITH_WXPATH)
    set(ENV{PATH} ${WITH_WXPATH}:$ENV{PATH})
endif()
unset(WITH_WXPATH CACHE)

set(CL_WX_CONFIG wx-config CACHE STRING "")

if (UNIX OR MINGW)
    if(NOT wxWidgets_CONFIG_EXECUTABLE)
        execute_process(COMMAND which ${CL_WX_CONFIG} OUTPUT_VARIABLE WX_TOOL OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
        set(WX_TOOL ${wxWidgets_CONFIG_EXECUTABLE})
    endif()
    if (NOT WX_TOOL)
        message(FATAL_ERROR
"\nNo functional wx_config script was found in your PATH.\nIs the wxWidgets development package installed?\nIf you built wxWidgets yourself, you can specify the path to your built wx-config executable via WITH_WXPATH\neg. -DWITH_WXPATH=\"/path/to/wx-config/\""
             )
    else()
        execute_process(COMMAND sh ${WX_TOOL} --version OUTPUT_VARIABLE WX_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
        string(SUBSTRING "${WX_VERSION}" "0" "1" wxMAJOR_VERSION)
        string(SUBSTRING "${WX_VERSION}" "2" "1" wxMINOR_VERSION)
        string(SUBSTRING "${WX_VERSION}" "4" "1" wxRELEASE_NUMBER)
        if ( wxMAJOR_VERSION LESS 3 )
        message(FATAL_ERROR
"\nBuilding SLADE requires at least wxWidgets-3.0.0"
             )
        endif()
        if (MINGW)
          execute_process(COMMAND sh ${WX_TOOL} --debug=no --rescomp OUTPUT_VARIABLE WX_RC_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
          string(REGEX REPLACE "windres" "" WX_RC_FLAGS ${WX_RC_FLAGS})
          set (CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} ${WX_RC_FLAGS}")
          add_definitions(-D__WXMSW__)
        endif (MINGW)
    endif()
    message("-- wx-config used is: ${WX_TOOL}")
    message("-- wxWidgets version is: ${WX_VERSION}")
    if (NOT APPLE AND NOT MINGW)
        # Is the wx we are using built on gtk2 or 3?
        execute_process(COMMAND ${WX_TOOL} --selected_config OUTPUT_VARIABLE WX_GTK_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
        string(SUBSTRING "${WX_GTK_VERSION}" "3" "1" GTK_VERSION)
        message("-- gtk version is: ${GTK_VERSION}")
    endif()
	set(wxWidgets_CONFIG_EXECUTABLE "${WX_TOOL}")
endif (UNIX OR MINGW)

if (WX_GTK3)
	set(wxWidgets_CONFIG_OPTIONS --toolkit=gtk3)
endif (WX_GTK3)

SET(WX_LIBS std aui gl stc richtext propgrid)
find_package(wxWidgets ${WX_VERSION} COMPONENTS ${WX_LIBS} REQUIRED)
include(${wxWidgets_USE_FILE})

# SFML
if (USE_SFML_RENDERWINDOW)
set(SFML_FIND_COMPONENTS system audio window graphics network)
ADD_DEFINITIONS(-DUSE_SFML_RENDERWINDOW)
else (USE_SFML_RENDERWINDOW)
set(SFML_FIND_COMPONENTS system audio window network)
find_package(Freetype REQUIRED)
endif(USE_SFML_RENDERWINDOW)

# Fluidsynth
if (NO_FLUIDSYNTH)
ADD_DEFINITIONS(-DNO_FLUIDSYNTH)
endif(NO_FLUIDSYNTH)

if (CMAKE_INSTALL_PREFIX)
ADD_DEFINITIONS(-DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
endif(CMAKE_INSTALL_PREFIX)

if(NOT NO_FLUIDSYNTH)
	find_package(FluidSynth REQUIRED)
else(NO_FLUIDSYNTH)
	message(STATUS "Fluidsynth support is disabled.")
endif()

find_package(FreeImage REQUIRED)
find_package(SFML COMPONENTS ${SFML_FIND_COMPONENTS} REQUIRED)
find_package(OpenGL REQUIRED)
if (NOT NO_LUA)
	find_package(Lua REQUIRED)
endif()
find_package(MPG123 REQUIRED)
find_package(glm REQUIRED)
include_directories(
	${FREEIMAGE_INCLUDE_DIR}
	${SFML_INCLUDE_DIR}
	${FREETYPE_INCLUDE_DIRS}
	${LUA_INCLUDE_DIR}
	${MPG123_INCLUDE_DIR}
	.
	..
	../thirdparty/glad/include
	./Application
	)

if (NOT NO_FLUIDSYNTH)
	include_directories(${FLUIDSYNTH_INCLUDE_DIR})
endif()

if(APPLE)
	set(OSX_ICON "${CMAKE_SOURCE_DIR}/SLADE-osx.icns")
	set(OSX_PLIST "${CMAKE_SOURCE_DIR}/Info.plist")

	set(SLADE_SOURCES ${SLADE_SOURCES} ${OSX_ICON} ${OSX_PLIST})

	set_source_files_properties(${OSX_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif(APPLE)

# Enable SSE instructions for dumb library
include(CheckCCompilerFlag)
check_c_compiler_flag(-msse HAVE_SSE)
if(HAVE_SSE)
	add_compile_options(-msse)
	add_definitions(-D_USE_SSE)
endif()

# Enable debug symbols for glib (so gdb debugging works properly with strings etc.)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG")

if(USE_SANITIZER)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif(USE_SANITIZER)

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
	${FREEIMAGE_LIBRARIES}
	${SFML_LIBRARY}
	${FREETYPE_LIBRARIES}
	${OPENGL_LIBRARIES}
	${LUA_LIBRARIES}
	${MPG123_LIBRARIES}
	glm::glm
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION LESS 9)
	target_link_libraries(slade -lstdc++fs)
endif()

if (NOT NO_FLUIDSYNTH)
	target_link_libraries(slade ${FLUIDSYNTH_LIBRARIES})
endif()

set_target_properties(slade PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${SLADE_OUTPUT_DIR})

# TODO: Installation targets for APPLE
if(APPLE)
	set_target_properties(slade PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${OSX_PLIST})

	add_custom_command(TARGET slade POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SLADE_OUTPUT_DIR}/slade.pk3" "$<TARGET_FILE_DIR:slade>/slade.pk3"
	)
else(APPLE)
	if(UNIX)
		install(TARGETS slade
			RUNTIME DESTINATION bin
			)

if (BUILD_PK3)
		install(FILES "${SLADE_OUTPUT_DIR}/slade.pk3"
			DESTINATION share/slade3
			)
endif()

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
	endif(UNIX)
endif(APPLE)

# uninstall target
if(NOT TARGET uninstall)
    configure_file(
        "${PROJECT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake COMMENT "Uninstall the project...")
endif()

if (NOT NO_COTIRE)
	set_target_properties(slade PROPERTIES
		COTIRE_CXX_PREFIX_HEADER_INIT "Application/Main.h"
		# Enable multithreaded unity builds by default
		# because otherwise probably no one would realize how
		COTIRE_UNITY_SOURCE_MAXIMUM_NUMBER_OF_INCLUDES -j
		# Fixes macro definition bleedout
		COTIRE_UNITY_SOURCE_PRE_UNDEFS "Bool"
		)
	cotire(slade)
endif()
