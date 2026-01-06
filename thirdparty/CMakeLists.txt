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
	lzma/C/LzmaDec.c
	glad/src/*.c
	libdrawtext/*.c
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

# Add JSON (nlohmann)
include(FetchContent)
FetchContent_Declare(json
	URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
	DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(json)

add_library(external STATIC ${EXTERNAL_SOURCES})
target_link_libraries(external
	${ZLIB_LIBRARY}
	fmt::fmt
	${CMAKE_DL_LIBS}
	SQLiteCpp
	sqlite3
	nlohmann_json::nlohmann_json
)
set(EXTERNAL_LIBRARIES external PARENT_SCOPE)
