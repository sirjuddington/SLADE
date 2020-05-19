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
	*.cpp
	*.cxx
	dumb/*.c
	lua/*.c
	lzma/C/LzmaDec.c
	fmt/*.cc
	${SLADE_HEADERS}
	)

add_library(external STATIC ${EXTERNAL_SOURCES})
target_link_libraries(external ${ZLIB_LIBRARY})
set(EXTERNAL_LIBRARIES external PARENT_SCOPE)
