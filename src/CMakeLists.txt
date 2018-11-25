if (NOT NO_COTIRE)
	include(cotire)
endif()

# wxWidgets version minimum 3.0
if(NOT WX_VERSION)
	SET(WX_VERSION 3.0)
endif(NOT WX_VERSION)

# wxWidgets libs
if (WITH_WXPATH)
    set(ENV{PATH} ${WITH_WXPATH}:$ENV{PATH})
endif()
unset(WITH_WXPATH CACHE)

set( CL_WX_CONFIG wx-config )

if (UNIX OR MINGW)
    execute_process(COMMAND which ${CL_WX_CONFIG} OUTPUT_VARIABLE WX_TOOL OUTPUT_STRIP_TRAILING_WHITESPACE)
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
endif (UNIX OR MINGW)

if (WX_GTK3)
	set(wxWidgets_CONFIG_OPTIONS --toolkit=gtk3)
endif (WX_GTK3)

SET(WX_LIBS std aui gl stc richtext propgrid media)
if (NO_WEBVIEW)
	SET(WX_LIBS ${WX_LIBS} html)
else (NO_WEBVIEW)
	SET(WX_LIBS ${WX_LIBS} webview)
	ADD_DEFINITIONS(-DUSE_WEBVIEW_STARTPAGE)
endif (NO_WEBVIEW)
find_package(wxWidgets ${WX_VERSION} COMPONENTS ${WX_LIBS} REQUIRED)
include(${wxWidgets_USE_FILE})

# SFML
if (USE_SFML_RENDERWINDOW)
set(SFML_FIND_COMPONENTS system audio window graphics network)
ADD_DEFINITIONS(-DUSE_SFML_RENDERWINDOW)
else (USE_SFML_RENDERWINDOW)
set(SFML_FIND_COMPONENTS system audio window network)
endif(USE_SFML_RENDERWINDOW)

# Fluidsynth
if (NO_FLUIDSYNTH)
ADD_DEFINITIONS(-DNO_FLUIDSYNTH)
endif(NO_FLUIDSYNTH)

if (CMAKE_INSTALL_PREFIX)
ADD_DEFINITIONS(-DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
endif(CMAKE_INSTALL_PREFIX)

find_package (PkgConfig REQUIRED)
if (APPLE)
	# There is no need to have GTK2 installed on OS X
	# Although, GLib is required by FluidSynth
	pkg_check_modules (GLib REQUIRED glib-2.0)
else (APPLE)
if (WX_GTK3)
	pkg_check_modules (GTK3 REQUIRED gtk+-3.0)
else (WX_GTK3)
	pkg_check_modules (GTK2 REQUIRED gtk+-2.0)
endif (WX_GTK3)
endif (APPLE)

if(NOT NO_FLUIDSYNTH)
	find_package(FluidSynth REQUIRED)
else(NO_FLUIDSYNTH)
	message(STATUS "Fluidsynth support is disabled.")
endif()

find_package(FreeImage REQUIRED)
find_package(SFML COMPONENTS ${SFML_FIND_COMPONENTS} REQUIRED)
find_package(FTGL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(Freetype REQUIRED)
find_package(CURL REQUIRED)
include_directories(
	${FREEIMAGE_INCLUDE_DIR}
	${SFML_INCLUDE_DIR}
	${FTGL_INCLUDE_DIR}
	${FREETYPE_INCLUDE_DIRS}
	${GLEW_INCLUDE_PATH}
	${CURL_INCLUDE_DIR}
	.
	./External/dumb
	./Application
	)

if (WX_GTK3)
	include_directories(${GTK3_INCLUDE_DIRS})
else (WX_GTK3)
	include_directories(${GTK2_INCLUDE_DIRS})
endif (WX_GTK3)

if (NOT NO_FLUIDSYNTH)
	include_directories(${FLUIDSYNTH_INCLUDE_DIR})
endif()

set(SLADE_SOURCES
)
# Don't include external libraries here as they should be compiled separately
file(GLOB_RECURSE SLADE_SOURCES
	Application/*.cpp
	Archive/*.cpp
	Audio/*.cpp
	Dialogs/*.cpp
	Game/*.cpp
	General/*.cpp
	Graphics/*.cpp
	MainEditor/*.cpp
	MapEditor/*.cpp
	OpenGL/*.cpp
	UI/*.cpp
	Utility/*.cpp
	Scripting/*.cpp
	TextEditor/*.cpp
	)
set(SLADE_HEADERS
)
file(GLOB_RECURSE SLADE_HEADERS *.h *.hpp)

if(APPLE)
	set(OSX_ICON "${CMAKE_SOURCE_DIR}/SLADE-osx.icns")
	set(OSX_PLIST "${CMAKE_SOURCE_DIR}/Info.plist")

	set(SLADE_SOURCES ${SLADE_SOURCES} ${OSX_ICON} ${OSX_PLIST})

	set_source_files_properties(${OSX_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif(APPLE)

# enable SSE instructions for dumb library
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_USE_SSE -msse")

# External libraries are compiled separately to enable unity builds
add_subdirectory(External)

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
	${FTGL_LIBRARIES}
	${OPENGL_LIBRARIES}
	${FREETYPE_LIBRARIES}
	${GLEW_LIBRARY}
	${CURL_LIBRARIES}
)

if (WX_GTK3)
	target_link_libraries(slade ${GTK3_LIBRARIES})
else(WX_GTK3)
	target_link_libraries(slade ${GTK2_LIBRARIES})
endif(WX_GTK3)

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

		install(FILES "${SLADE_OUTPUT_DIR}/slade.pk3"
			DESTINATION share/slade3
			)
		
		install(FILES "${PROJECT_SOURCE_DIR}/dist/res/logo_icon.png"
			DESTINATION share/icons/
			RENAME net.mancubus.SLADE.png
			)

		install(FILES "${PROJECT_SOURCE_DIR}/net.mancubus.SLADE.desktop"
			DESTINATION share/applications/
			)

		install(FILES "${PROJECT_SOURCE_DIR}/net.mancubus.SLADE.appdata.xml"
			DESTINATION share/appdata/
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
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()

if (NOT NO_COTIRE)
	set_target_properties(slade PROPERTIES
		COTIRE_CXX_PREFIX_HEADER_INIT "common.h"
		# Enable multithreaded unity builds by default
		# because otherwise probably no one would realize how
		COTIRE_UNITY_SOURCE_MAXIMUM_NUMBER_OF_INCLUDES -j
		# Fixes macro definition bleedout
		COTIRE_UNITY_SOURCE_PRE_UNDEFS "Bool"
		)
	cotire(slade)
endif()
