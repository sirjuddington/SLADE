find_package(wxWidgets COMPONENTS std aui gl stc richtext propgrid media REQUIRED)
include(${wxWidgets_USE_FILE})

set(SFML_FIND_COMPONENTS system audio window)

find_package (PkgConfig REQUIRED)
pkg_check_modules (GTK2 REQUIRED gtk+-2.0)

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
include_directories(${FLUIDSYNTH_INCLUDE_DIR} ${FREEIMAGE_INCLUDE_DIR} ${SFML_INCLUDE_DIR} ${FTGL_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${GLEW_INCLUDE_PATH} ${GTK2_INCLUDE_DIRS} .)

set(SLADE_SOURCES
)
file(GLOB_RECURSE SLADE_SOURCES *.cpp)
set(LUA_SOURCES
)
file(GLOB LUA_SOURCES lua/*.c)

add_executable(slade WIN32
	lzma/C/LzmaDec.c
	mus2mid/mus2mid.cpp
	zreaders/ancientzip.cpp
	zreaders/files.cpp
	zreaders/m_alloc.cpp
	cl_notebook_art/cl_aui_notebook_art.cpp
	sfMod/sfMod.cpp
	${LUA_SOURCES}
	${SLADE_SOURCES}
)

target_link_libraries(slade ${ZLIB_LIBRARY} ${BZIP2_LIBRARIES} ${wxWidgets_LIBRARIES} ${FLUIDSYNTH_LIBRARIES} ${FREEIMAGE_LIBRARIES} ${SFML_LIBRARY} ${FTGL_LIBRARY} ${OPENGL_LIBRARIES} ${FREETYPE_LIBRARIES} ${GLEW_LIBRARY} ${GTK2_LIBRARIES} modplug)
set_target_properties(slade PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${SLADE_OUTPUT_DIR})

if(APPLE)
add_custom_command(TARGET slade
	POST_BUILD
	COMMAND mkdir -p ${SLADE_OUTPUT_DIR}/SLADE.app/Contents/
	COMMAND mkdir -p ${SLADE_OUTPUT_DIR}/SLADE.app/Contents/MacOS/
	COMMAND mkdir -p ${SLADE_OUTPUT_DIR}/SLADE.app/Contents/Resources/
	COMMAND cp ${SLADE_OUTPUT_DIR}/../Info.plist ${SLADE_OUTPUT_DIR}/SLADE.app/Contents/
	COMMAND cp ${SLADE_OUTPUT_DIR}/../SLADE-osx.icns ${SLADE_OUTPUT_DIR}/SLADE.app/Contents/Resources/
	COMMAND mv ${SLADE_OUTPUT_DIR}/slade ${SLADE_OUTPUT_DIR}/SLADE.app/Contents/MacOS/
	COMMAND rsync -aC ${SLADE_OUTPUT_DIR}/res/ ${SLADE_OUTPUT_DIR}/SLADE.app/Contents/Resources
)
endif()
