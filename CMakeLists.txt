project(x16emu)
cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(SDL2 REQUIRED)
if(WIN32)
	include_directories("${CMAKE_CURRENT_LIST_DIR}/windows")
endif()

set(SOURCES 
	cpu/fake6502.c
	debugger.c
	disasm.c
	loadsave.c
	main.c
	memory.c
	ps2.c
	sdcard.c
	spi.c
	vera_spi.c
	via.c
	video.c
)
set(HEADERS
	cpu/fake6502.h
	debugger.h
	disasm.h
	glue.h
	loadsave.h
	memory.h
	ps2.h
	sdcard.h
	spi.h
	utf8.h
	vera_spi.h
	via.h
	video.h
)
find_file(ROM_LABELS rom_labels.h PATHS ${CMAKE_CURRENT_SOURCE_DIR} NO_DEFAULT_PATH)
if(ROM_LABELS)
	list(APPEND HEADERS ${ROM_LABELS})
endif()
add_executable(x16emu ${SOURCES} ${HEADERS})
if(NOT MSVC)
	target_link_libraries(x16emu m)
endif()
target_link_libraries(x16emu PUBLIC SDL2::SDL2)
