find_program(ZIPTOOL_ZIP_EXECUTABLE zip)
find_program(ZIPTOOL_7Z_EXECUTABLE 7z "$ENV{ProgramFiles}/7-Zip")

if(ZIPTOOL_7Z_EXECUTABLE)
	set(ZIP_COMMAND "${ZIPTOOL_7Z_EXECUTABLE}" u -tzip -mtc- -mcu+ -mx=9 -r "${CMAKE_BINARY_DIR}/slade.pk3" .)
elseif(ZIPTOOL_ZIP_EXECUTABLE)
	set(ZIP_COMMAND "${ZIPTOOL_ZIP_EXECUTABLE}" -X -UN=UTF8 -9 -r "${CMAKE_BINARY_DIR}/slade.pk3" .)
else()
	message(STATUS "no zip executable, slade.pk3 won't build")
endif()

if(ZIP_COMMAND)
	message(STATUS "run `pk3` target to build slade.pk3")
	add_custom_target(pk3 ALL
		COMMAND ${ZIP_COMMAND}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/res
		VERBATIM)
endif()

if(APPLE)
	# To be able to put .pk3 into the application bundle
	add_dependencies(slade pk3)
endif(APPLE)
