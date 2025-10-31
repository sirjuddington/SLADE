# ------------------------------------------------------------------------------
# CMake to build SLADE in Windows/MSVC (using vcpkg for dependencies)
# ------------------------------------------------------------------------------

# Build Settings ---------------------------------------------------------------

# Static linking
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Enable big objects and utf8
add_compile_options(/bigobj /utf-8)

# Define a SLADE_DEBUG macro
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DSLADE_DEBUG")


# Dependencies -----------------------------------------------------------------

# wxWidgets
if (NOT BUILD_WX)
	find_package(wxWidgets CONFIG REQUIRED)
endif ()
set(WX_LIBS wx::core wx::base wx::stc wx::aui wx::gl wx::propgrid wx::net)

# FreeType
find_package(Freetype REQUIRED)

# Lua
if (NOT NO_LUA)
	find_package(Lua REQUIRED)
endif ()

# FluidSynth
if (NOT NO_FLUIDSYNTH)
	find_package(FluidSynth CONFIG REQUIRED)
endif ()

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

# WebP
if (NOT BUILD_WX)
	find_package(WebP CONFIG REQUIRED)
endif ()

# Other
find_package(MPG123 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)


# Include Search Paths ---------------------------------------------------------

include_directories(
	${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include
	.
	..
	../thirdparty/glad/include
	./Application
)


# Sources ----------------------------------------------------------------------

# Extra MSVC-specific files to build
set(SLADE_SOURCES ${SLADE_SOURCES} "../msvc/SLADE.rc" "../msvc/SLADE.manifest")

# External libraries are compiled separately to enable unity builds
add_subdirectory(../thirdparty external)


# Build ------------------------------------------------------------------------

add_executable(slade WIN32
	${SLADE_SOURCES}
	${SLADE_HEADERS}
)

if (NOT SLADE_EXE_NAME)
	set(SLADE_EXE_NAME SLADE)
endif ()

if (NOT SLADE_EXE_DIR)
	set(SLADE_EXE_DIR dist)
endif ()

# Properties
set_target_properties(slade
	PROPERTIES
	LINK_FLAGS "/subsystem:windows"
	OUTPUT_NAME "${SLADE_EXE_NAME}"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/${SLADE_EXE_DIR}"
)

# Precompiled Header
target_precompile_headers(slade PRIVATE "Application/Main.h")

# Link
target_link_libraries(slade
	${BZIP2_LIBRARIES}
	${EXTERNAL_LIBRARIES}
	${FREETYPE_LIBRARIES}
	${OPENGL_LIBRARIES}
	${SFML_LIBRARIES}
	${WX_LIBS}
	${ZLIB_LIBRARY}
	MPG123::libmpg123
	glm::glm
)

if (NOT NO_LUA)
	target_link_libraries(slade ${LUA_LIBRARIES})
endif ()

if (NOT NO_FLUIDSYNTH)
	target_link_libraries(slade FluidSynth::libfluidsynth)
endif ()

if (NOT BUILD_WX)
	target_link_libraries(slade
		WebP::webp
		WebP::webpdecoder
		WebP::webpdemux
	)
endif ()
