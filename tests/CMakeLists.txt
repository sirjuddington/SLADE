add_executable(map-script-cli-fixture EXCLUDE_FROM_ALL MapScriptCliFixture.cpp)

set(MAP_SCRIPT_CLI_TEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/map-script-cli")
set(MAP_SCRIPT_CLI_INPUT "${MAP_SCRIPT_CLI_TEST_DIR}/input.wad")
set(MAP_SCRIPT_CLI_OUTPUT "${MAP_SCRIPT_CLI_TEST_DIR}/output.wad")
set(MAP_SCRIPT_CLI_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/map-script-cli/create_line.lua")

add_test(
	NAME MapScriptCli.CreateFixture
	COMMAND $<TARGET_FILE:map-script-cli-fixture> create "${MAP_SCRIPT_CLI_INPUT}"
)
set_tests_properties(MapScriptCli.CreateFixture PROPERTIES FIXTURES_SETUP map_script_cli)

add_test(
	NAME MapScriptCli.RunGeometry
	COMMAND $<TARGET_FILE:slade-map-script>
		--input "${MAP_SCRIPT_CLI_INPUT}"
		--map MAP01
		--script "${MAP_SCRIPT_CLI_SCRIPT}"
		--output "${MAP_SCRIPT_CLI_OUTPUT}"
		--validate
)
set_tests_properties(MapScriptCli.RunGeometry PROPERTIES
	FIXTURES_REQUIRED map_script_cli
	# The CLI must not depend on a display server being available.
	ENVIRONMENT "DISPLAY="
)

add_test(
	NAME MapScriptCli.VerifyGeometry
	COMMAND $<TARGET_FILE:map-script-cli-fixture> verify "${MAP_SCRIPT_CLI_OUTPUT}"
)
set_tests_properties(MapScriptCli.VerifyGeometry PROPERTIES FIXTURES_REQUIRED map_script_cli)

add_custom_target(check-map-script-cli
	COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CMAKE_BINARY_DIR}" --output-on-failure -R "^MapScriptCli\\."
	DEPENDS slade-map-script map-script-cli-fixture
	USES_TERMINAL
)
