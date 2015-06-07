if (USE_WEBVIEW_STARTPAGE)
find_package(wxWidgets 3.0 COMPONENTS std aui gl stc richtext propgrid media webview REQUIRED)
ADD_DEFINITIONS(-DUSE_WEBVIEW_STARTPAGE)
else (USE_WEBVIEW_STARTPAGE)
find_package(wxWidgets 3.0 COMPONENTS std aui gl stc richtext propgrid media html REQUIRED)
endif (USE_WEBVIEW_STARTPAGE)
include(${wxWidgets_USE_FILE})

if (USE_SFML_RENDERWINDOW)
set(SFML_FIND_COMPONENTS system audio window graphics)
ADD_DEFINITIONS(-DUSE_SFML_RENDERWINDOW)
else (USE_SFML_RENDERWINDOW)
set(SFML_FIND_COMPONENTS system audio window)
endif(USE_SFML_RENDERWINDOW)

find_package (PkgConfig REQUIRED)
if (APPLE)
	# There is no need to have GTK2 installed on OS X
	# Although, GLib is required by FluidSynth
	pkg_check_modules (GLib REQUIRED glib-2.0)
else (APPLE)
	pkg_check_modules (GTK2 REQUIRED gtk+-2.0)
endif (APPLE)

include(../FindFluidSynth.cmake)
include(../FindFreeImage.cmake)
include(../FindSFML.cmake)
include(../FindFTGL.cmake)
include(../FindGLEW.cmake)
include(FindFreetype)
if(NOT ${FLUIDSYNTH_FOUND})
	message(SEND_ERROR "Fluidsynth required.")
endif()
if(NOT ${FREEIMAGE_FOUND})
	message(SEND_ERROR "FreeImage required.")
endif()
if(NOT ${SFML_FOUND})
	message(SEND_ERROR "SFML required.")
endif()
if(NOT ${FTGL_FOUND})
	message(SEND_ERROR "FTGL required.")
endif()
if(NOT ${FREETYPE_FOUND})
	message(SEND_ERROR "Freetype required.")
endif()
if(NOT ${GLEW_FOUND})
	message(SEND_ERROR "GLEW required.")
endif()
include_directories(${FLUIDSYNTH_INCLUDE_DIR} ${FREEIMAGE_INCLUDE_DIR} ${SFML_INCLUDE_DIR} ${FTGL_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${GLEW_INCLUDE_PATH} ${GTK2_INCLUDE_DIRS} . ./dumb)

set(SLADE_SOURCES
)
file(GLOB_RECURSE SLADE_SOURCES *.cpp)
set(LUA_SOURCES
)
file(GLOB LUA_SOURCES lua/*.c)
set(DUMB_SOURCES
)
file(GLOB_RECURSE DUMB_SOURCES dumb/*.c)

if(APPLE)
	set(OSX_ICON "${CMAKE_SOURCE_DIR}/SLADE-osx.icns")
	set(OSX_PLIST "${CMAKE_SOURCE_DIR}/Info.plist")

	set(SLADE_SOURCES ${SLADE_SOURCES} ${OSX_ICON} ${OSX_PLIST})

	set_source_files_properties(${OSX_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif(APPLE)

add_executable(slade WIN32 MACOSX_BUNDLE
	lzma/C/LzmaDec.c
	mus2mid/mus2mid.cpp
	zreaders/ancientzip.cpp
	zreaders/files.cpp
	zreaders/m_alloc.cpp
	zreaders/i_music.cpp
	zreaders/music_hmi_midiout.cpp
	zreaders/music_midistream.cpp
	zreaders/music_mus_midiout.cpp
	zreaders/music_smf_midiout.cpp
	zreaders/music_xmi_midiout.cpp
	${LUA_SOURCES}
	${DUMB_SOURCES}
	${SLADE_SOURCES}
)

target_link_libraries(slade ${ZLIB_LIBRARY} ${BZIP2_LIBRARIES} ${wxWidgets_LIBRARIES} ${FLUIDSYNTH_LIBRARIES} ${FREEIMAGE_LIBRARIES} ${SFML_LIBRARY} ${FTGL_LIBRARY} ${OPENGL_LIBRARIES} ${FREETYPE_LIBRARIES} ${GLEW_LIBRARY} ${GTK2_LIBRARIES})
set_target_properties(slade PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${SLADE_OUTPUT_DIR})

if(APPLE)
	set_target_properties(slade PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${OSX_PLIST})

	add_custom_command(TARGET slade POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SLADE_OUTPUT_DIR}/slade.pk3" "$<TARGET_FILE_DIR:slade>/slade.pk3"
	)
endif(APPLE)
