# Use system packages if available
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)

# Set in parent scope
set(ZLIB_LIBRARY ${ZLIB_LIBRARY} PARENT_SCOPE)
set(BZIP2_LIBRARIES ${BZIP2_LIBRARIES} PARENT_SCOPE)

option(USE_SYSTEM_DUMB "Use system DUMB library" OFF)
if (USE_SYSTEM_DUMB)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(DUMB REQUIRED IMPORTED_TARGET dumb>=1.0)
else ()
	file(GLOB_RECURSE DUMB_SOURCES dumb/*.c)
endif ()

# Roll the rest up into a big ball and compile it by itself
set(EXTERNAL_SOURCES
)
file(GLOB_RECURSE EXTERNAL_SOURCES
	lzma/C/LzmaDec.c
	glad/src/*.c
	libdrawtext/*.c
	${DUMB_SOURCES}
	${SLADE_HEADERS}
	xxhash/*.c
)

# Add fmt
option(USE_SYSTEM_FMT "Use system fmt library" OFF)
option(FMT_INSTALL OFF)
if (USE_SYSTEM_FMT)
	find_package(fmt REQUIRED)
else ()
	add_subdirectory(fmt)
	#target_include_directories(fmt INTERFACE ${CMAKE_CURRENT_LIST_DIR}/fmt/include)
endif ()

# Required for libdrawtext or we get multiple definition linker errors
if (NOT MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcommon")
endif ()

# Add SQLiteCpp
add_subdirectory(sqlitecpp)

add_library(external STATIC ${EXTERNAL_SOURCES})
target_link_libraries(external ${ZLIB_LIBRARY} fmt::fmt ${CMAKE_DL_LIBS} SQLiteCpp sqlite3)
set(EXTERNAL_LIBRARIES external PARENT_SCOPE)

if (USE_SYSTEM_DUMB)
	target_link_libraries(external PkgConfig::DUMB)
else ()
	target_include_directories(external PUBLIC ${CMAKE_CURRENT_LIST_DIR}/dumb)
endif ()
