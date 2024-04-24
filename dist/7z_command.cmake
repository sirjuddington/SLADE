set(7ZIP_COMMAND "${ZIPTOOL_7Z_EXECUTABLE}" a -tzip -mtc- -mcu+ -mx=9 -r "${PK3_DESTINATION}/slade.pk3" .)

if(EXISTS "${PK3_DESTINATION}/slade.pk3")
	if(WIN32)
		set(PLATFORM_GREP_COMMAND findstr "/c:SHA256 for data and names:")
	else()
		set(PLATFORM_GREP_COMMAND grep "SHA256 for data and names:")
	endif()
	execute_process(COMMAND  "${ZIPTOOL_7Z_EXECUTABLE}" h -scrcsha256 .
		COMMAND  ${PLATFORM_GREP_COMMAND}
		OUTPUT_VARIABLE FOLDER_SHA256
	)
	execute_process(COMMAND  "${ZIPTOOL_7Z_EXECUTABLE}" t -scrcsha256 "${PK3_DESTINATION}/slade.pk3"
		COMMAND  ${PLATFORM_GREP_COMMAND}
		OUTPUT_VARIABLE PK3_SHA256
	)
	if(FOLDER_SHA256 STREQUAL PK3_SHA256)
		message(STATUS "slade.pk3 is already up to date.")
	else()
		file(REMOVE "${PK3_DESTINATION}/slade.pk3")
		message(STATUS "Regenerating slade.pk3...")
		execute_process(COMMAND ${7ZIP_COMMAND})
	endif()
else()
	execute_process(COMMAND ${7ZIP_COMMAND})
endif()
