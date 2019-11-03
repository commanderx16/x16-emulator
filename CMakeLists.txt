cmake_minimum_required(VERSION 3.1)
project(x16emu LANGUAGES C VERSION 35)

option(BUILD_DOCS "Build documentation")
option(WITH_YM2151 "Enable YM2151")
option(X16EMU_RELEASE "This is a x16-emu release")
set(X16ROM_PATH "" CACHE PATH "Path to x16-rom")
set(X16DOCS_PATH "" CACHE PATH "Path to x16-docs")

if(X16EMU_RELEASE)
	message(STATUS "This is a release build")
	set(BUILD_DOCS ON FORCE)
	set(WITH_YM2151 ON FORCE)
else()
	message(STATUS "Development build")
endif()

if(NOT X16ROM_PATH)
	set(X16ROM_PATH "${PROJECT_SOURCE_DIR}/../x16-rom")
endif()
if(NOT X16DOCS_PATH)
	set(X16DOCS_PATH "${PROJECT_SOURCE_DIR}/../x16-docs")
endif()

if(NOT IS_DIRECTORY "${X16ROM_PATH}")
	message(WARNING "Path to x16-rom directory is incorrect. (X16ROM_PATH)")
endif()
if(NOT IS_DIRECTORY "${X16DOCS_PATH}")
	message(WARNING "Path to x16-docs directory is incorrect. (X16DOCS_PATH)")
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS OFF)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
	add_compile_options(-O3 -Wall -Werror)
	if(WIN32)
		# CMake 3.13 added add_link_options
		set(CMAKE_EXE_LINKER_FLAGS "-Wl,--subsystem,console")
	endif()
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
set(SDL2_BUILDING_LIBRARY ON)
find_package(SDL2 REQUIRED)
find_package(PythonInterp REQUIRED)
if(MSVC)
	include_directories("${PROJECT_SOURCE_DIR}/extern/include/msvc")
endif()

if(PYTHONINTERP_FOUND)
	set_property(DIRECTORY PROPERTY CLEAN_NO_CUSTOM ON)
	add_custom_command(OUTPUT "${PROJECT_SOURCE_DIR}/cpu/tables.h" "${PROJECT_SOURCE_DIR}/cpu/mnemonics.h"
		COMMAND "${PYTHON_EXECUTABLE}" buildtables.py
		DEPENDS "${PROJECT_SOURCE_DIR}/cpu/buildtables.py" "${PROJECT_SOURCE_DIR}/cpu/6502.opcodes" "${PROJECT_SOURCE_DIR}/cpu/65c02.opcodes"
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/cpu"
		COMMENT "Generating cpu instruction tables: tables.h mnemonics.h"
		VERBATIM)
endif()

find_file(ROM_LABELS rom_labels.h PATHS ${PROJECT_SOURCE_DIR} NO_DEFAULT_PATH)

set(SOURCES
	cpu/fake6502.c
	debugger.c
	disasm.c
	joystick.c
	keyboard.c
	loadsave.c
	main.c
	memory.c
	ps2.c
	rendertext.c
	sdcard.c
	spi.c
	vera_spi.c
	via.c
	video.c
	$<$<BOOL:${WITH_YM2151}>:extern/src/ym2151.c>
	$<$<BOOL:${EMSCRIPTEN}>:javascript_interface.c>
)
set(HEADERS
	cpu/fake6502.h
	cpu/tables.h
	cpu/mnemonics.h
	debugger.h
	disasm.h
	glue.h
	joystick.h
	keyboard.h
	loadsave.h
	memory.h
	ps2.h
	rendertext.h
	sdcard.h
	spi.h
	utf8.h
	vera_spi.h
	via.h
	video.h
	extern/include/gif.h
	$<$<BOOL:${WITH_YM2151}>:extern/include/ym2151.h>
	$<$<BOOL:${ROM_LABELS}>:${ROM_LABELS}>
)

add_executable(x16emu ${SOURCES} ${HEADERS})
target_link_libraries(x16emu PRIVATE SDL2::SDL2)
target_include_directories(x16emu PRIVATE "extern/include")
if(NOT MSVC)
	target_link_libraries(x16emu PRIVATE m)
endif()
target_compile_definitions(x16emu PRIVATE $<$<BOOL:${WITH_YM2151}>:WITH_YM2151>)
install(TARGETS x16emu RUNTIME DESTINATION ".")
if(X16EMU_RELEASE OR EXISTS "${X16ROM_PATH}/rom.bin")
	add_custom_command(TARGET x16emu POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy "${X16ROM_PATH}/rom.bin" "${CMAKE_CURRENT_BINARY_DIR}/rom.bin"
		COMMENT "Copying rom.bin to build directory" VERBATIM)
	install(FILES "${X16ROM_PATH}/rom.bin" "${X16ROM_PATH}/rom.sym" DESTINATION ".")
endif()
if(EMSCRIPTEN)
	set_target_properties(x16emu PROPERTIES SUFFIX ".html")

	set(WASM_TEMPLATE "${PROJECT_SOURCE_DIR}/webassembly/x16emu-template.html")
	set(ROM_PATH "${X16ROM_PATH}/rom.bin")
	# CMake 3.13 added target_link_options
	string(CONCAT X16EMU_LINK_FLAGS
		"--shell-file ${WASM_TEMPLATE}"
		" --preload-file ${ROM_PATH}@rom.bin"
		" -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=1"
		" -s EXPORTED_FUNCTIONS=[_j2c_reset,_j2c_paste,_j2c_start_audio,_main]"
		" -s EXTRA_EXPORTED_RUNTIME_METHODS=[ccall,cwrap]")
	set_property(TARGET x16emu APPEND_STRING PROPERTY LINK_FLAGS "${X16EMU_LINK_FLAGS}")
	set_property(TARGET x16emu APPEND PROPERTY LINK_DEPENDS "${WASM_TEMPLATE}" "${ROM_PATH}")
	set(X16EMU_EMSCRIPTEN_EXTRA_GENERATED_FILES
		"${CMAKE_CURRENT_BINARY_DIR}/x16emu.data"
		"${CMAKE_CURRENT_BINARY_DIR}/x16emu.js"
		"${CMAKE_CURRENT_BINARY_DIR}/x16emu.wasm")
	set_property(TARGET x16emu APPEND PROPERTY ADDITIONAL_CLEAN_FILES ${X16EMU_EMSCRIPTEN_EXTRA_GENERATED_FILES})
	get_target_property(CC x16emu ADDITIONAL_CLEAN_FILES)
	add_custom_command(TARGET x16emu POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy_directory "${PROJECT_SOURCE_DIR}/webassembly" "${CMAKE_CURRENT_BINARY_DIR}/webassembly"
		COMMENT "Copying webassembly directory to build directory" VERBATIM)
	install(FILES
		${X16EMU_EMSCRIPTEN_EXTRA_GENERATED_FILES}
		DESTINATION ".")
	install(FILES
		"${PROJECT_SOURCE_DIR}/webassembly/main.js" "${PROJECT_SOURCE_DIR}/webassembly/styles.css"
		DESTINATION "webassembly")
endif()

if(BUILD_DOCS)
	find_package(pandoc REQUIRED)
	set(DOCS_CREATED)

	add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/docs"
		COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/docs")
	add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/docs/github-pandoc.css"
		COMMAND "${CMAKE_COMMAND}" -E copy "${PROJECT_SOURCE_DIR}/github-pandoc.css" "${CMAKE_CURRENT_BINARY_DIR}/docs/github-pandoc.css")
	set(DOCS_CREATED "${DOCS_CREATED};${CMAKE_CURRENT_BINARY_DIR}/docs/github-pandoc.css")

	macro(add_documentation INPUT OUTPUT TITLE)
		pandoc_gfm_to_html_generate(STANDALONE INPUT "${INPUT}" OUTPUT "${OUTPUT}" TITLE "${TITLE}" CSS "github-pandoc.css")
		set(DOCS_CREATED "${DOCS_CREATED};${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT}")
	endmacro()

	add_documentation("README.md" "docs/README.html" "Commander X16 Emulator")
	add_documentation("${X16ROM_PATH}/README.md" "docs/KERNAL-BASIC.html" "Commander X16 KERNAL/BASIC/DOS ROM")
	add_documentation("${X16DOCS_PATH}/Commander X16 Programmer's Reference Guide.md" "docs/Programmer's Reference Guide.html" "Commander X16 Programmer's Reference Guide")
	add_documentation("${X16DOCS_PATH}/VERA Programmer's Reference.md" "docs/VERA Programmer's Reference.html" "VERA Programmer's Reference")

	add_custom_target(docs ALL DEPENDS ${DOCS_CREATED})
	install(FILES ${DOCS_CREATED} DESTINATION "doc")
endif()

set(PACKAGE_PLATFORM "-${CMAKE_SYSTEM_NAME}")
if(EMSCRIPTEN)
	set(PACKAGE_ARCH "")
else()
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(PACKAGE_ARCH "-x86_64")
	else()
		set(PACKAGE_ARCH "-x86")
	endif()
endif()
set(PACKAGE_VERSION "")
if(X16EMU_RELEASE)
	set(PACKAGE_VERSION "-v${PROJECT_VERSION}")
endif()

set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}${PACKAGE_PLATFORM}${PACKAGE_ARCH}${PACKAGE_VERSION}")
include(CPack)
