find_program(PANDOC_BIN pandoc
	HINTS PANDOC_ROOT ENV $ENV{PANDOC_ROOT}
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(pandoc
	REQUIRED_VARS PANDOC_BIN
)

include(CMakeParseArguments)
function(pandoc_gfm_to_html_generate)
	cmake_parse_arguments(PDG "STANDALONE" "INPUT;OUTPUT;CSS;TITLE" "" ${ARGN})
	if(PDG_UNPARSED_ARGUMENTS)
		message(FATAL_ERROR "Unknown argument: ${PDG_UNPARSED_ARGUMENTS}")
	endif()
	if(NOT PDG_INPUT OR NOT PDG_OUTPUT OR NOT PDG_TITLE OR NOT PDG_CSS)
		message(FATAL_ERROR "Missing required option")
	endif()
	if(NOT IS_ABSOLUTE "${PDG_INPUT}")
		set(PDG_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/${PDG_INPUT}")
	endif()
	if(NOT IS_ABSOLUTE "${PDG_OUTPUT}")
		set(PDG_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${PDG_OUTPUT}")
	endif()

	add_custom_command(OUTPUT "${PDG_OUTPUT}"
		COMMAND "${PANDOC_BIN}" --from gfm --to html --standalone --css "${PDG_CSS}"
			--metadata pagetitle="${PDG_TITLE}" "${PDG_INPUT}" --output "${PDG_OUTPUT}"
		VERBATIM
	)
endfunction()
