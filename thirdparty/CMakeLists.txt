# Use system packages if available
find_package(ZLIB)
find_package(BZip2)

if(NOT ZLIB_FOUND)
	set(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/zlib)
	add_subdirectory(zlib)
	set(ZLIB_LIBRARY z)
endif(NOT ZLIB_FOUND)

if(NOT BZIP2_FOUND)
	set(BZIP2_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bzip2)
	add_subdirectory(bzip2)
	set(BZIP2_LIBRARIES bz2)
endif(NOT BZIP2_FOUND)

# Set in parent scope
set(ZLIB_LIBRARY ${ZLIB_LIBRARY} PARENT_SCOPE)
set(BZIP2_LIBRARIES ${BZIP2_LIBRARIES} PARENT_SCOPE)

# Roll the rest up into a big ball and compile it by itself
set(EXTERNAL_SOURCES
)
file(GLOB_RECURSE EXTERNAL_SOURCES
	*.cpp
	*.cxx
	dumb/*.c
	lua/*.c
	lzma/C/LzmaDec.c
	${SLADE_HEADERS}
	)

add_library(external ${EXTERNAL_SOURCES})
target_link_libraries(external ${ZLIB_LIBRARY})
set(EXTERNAL_LIBRARIES external PARENT_SCOPE)
