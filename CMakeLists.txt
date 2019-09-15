cmake_minimum_required(VERSION 3.0)
project(x16emu LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(SDL2 REQUIRED)
if(MSVC)
	include_directories("${CMAKE_CURRENT_LIST_DIR}/extern/include/msvc")
endif()
include_directories(
	"${CMAKE_CURRENT_LIST_DIR}/extern/include"
	"${CMAKE_CURRENT_LIST_DIR}/extern/src"
)

set(SOURCES
	cpu/fake6502.c
	debugger.c
	disasm.c
	javascript_interface.c
	joystick.c
	keyboard.c
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
	joystick.h
	keyboard.h
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
if(WITH_YM2151)
	list(APPEND SOURCES extern/src/ym2151.c)
	list(APPEND HEADERS extern/src/ym2151.h)
	add_definitions(-DWITH_YM2151)
endif()
find_file(ROM_LABELS rom_labels.h PATHS ${CMAKE_CURRENT_SOURCE_DIR} NO_DEFAULT_PATH)
if(ROM_LABELS)
	list(APPEND HEADERS ${ROM_LABELS})
endif()
add_executable(x16emu ${SOURCES} ${HEADERS})
if(NOT MSVC)
	target_link_libraries(x16emu PRIVATE m)
endif()
target_link_libraries(x16emu PRIVATE SDL2::SDL2)
