# Use system packages if available
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)

# Set in parent scope
set(ZLIB_LIBRARY ${ZLIB_LIBRARY} PARENT_SCOPE)
set(BZIP2_LIBRARIES ${BZIP2_LIBRARIES} PARENT_SCOPE)

# Roll the rest up into a big ball and compile it by itself
set(EXTERNAL_SOURCES
)
file(GLOB_RECURSE EXTERNAL_SOURCES
	dumb/*.c
	email/*.cpp
	lzma/C/LzmaDec.c
	mus2mid/mus2mid.cpp
	zreaders/*.cpp
	${SLADE_HEADERS}
	)

# Add fmt
add_subdirectory(fmt)

# Add lunasvg
add_library(lunasvg STATIC)
add_subdirectory(lunasvg/include)
add_subdirectory(lunasvg/source)
add_subdirectory(lunasvg/3rdparty/software)
add_subdirectory(lunasvg/3rdparty/plutovg)

add_library(external STATIC ${EXTERNAL_SOURCES})
target_link_libraries(external ${ZLIB_LIBRARY} lunasvg fmt)
set(EXTERNAL_LIBRARIES external PARENT_SCOPE)
