
# the mingw32 path on macOS installed through homebrew
MINGW32=/usr/local/Cellar/mingw-w64/7.0.0_2/toolchain-i686/i686-w64-mingw32
# the Windows SDL2 path on macOS installed through ./configure --prefix=... && make && make install
WIN_SDL2=~/tmp/sdl2-win32

ifeq ($(CROSS_COMPILE_WINDOWS),1)
	SDL2CONFIG=$(WIN_SDL2)/bin/sdl2-config
else
	SDL2CONFIG=sdl2-config
endif

CFLAGS=-std=c99 -O3 -Wall -Werror -g $(shell $(SDL2CONFIG) --cflags) -Isrc/extern/include -Isrc/extern/src
LDFLAGS=$(shell $(SDL2CONFIG) --libs) -lm

ODIR = build
SDIR = src


ifdef TRACE
	CFLAGS+=-D TRACE
endif

OUTPUT=x16emu

ifeq ($(MAC_STATIC),1)
	LDFLAGS=/usr/local/lib/libSDL2.a -lm -liconv -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox -Wl,-framework,ForceFeedback -lobjc -Wl,-framework,CoreVideo -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,IOKit -Wl,-weak_framework,QuartzCore -Wl,-weak_framework,Metal
endif

ifeq ($(CROSS_COMPILE_WINDOWS),1)
	LDFLAGS+=-L$(MINGW32)/lib
	# this enables printf() to show, but also forces a console window
	LDFLAGS+=-Wl,--subsystem,console
	CC=i686-w64-mingw32-gcc
endif

ifdef EMSCRIPTEN
	LDFLAGS+=--shell-file webassembly/x16emu-template.html --preload-file rom.bin -s TOTAL_MEMORY=32MB -s ASSERTIONS=1 -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=1
	# To the Javascript runtime exported functions
	LDFLAGS+=-s EXPORTED_FUNCTIONS='["_j2c_reset", "_j2c_paste", "_j2c_start_audio", _main]' -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]'

	OUTPUT=x16emu.html
endif

_OBJS = cpu/fake6502.o memory.o disasm.o video.o ps2.o i2c.o smc.o rtc.o via.o loadsave.o vera_spi.o audio.o vera_pcm.o vera_psg.o sdcard.o main.o debugger.o javascript_interface.o joystick.o rendertext.o keyboard.o icon.o timing.o

_HEADERS = audio.h cpu/65c02.h cpu/fake6502.h cpu/instructions.h cpu/mnemonics.h cpu/modes.h cpu/support.h cpu/tables.h debugger.h disasm.h extern/include/gif.h extern/src/ym2151.h glue.h i2c.h icon.h joystick.h keyboard.h loadsave.h memory.h ps2.h rendertext.h rom_symbols.h rtc.h sdcard.h smc.h timing.h utf8.h utf8_encode.h vera_pcm.h vera_psg.h vera_spi.h version.h via.h video.h

_OBJS += extern/src/ym2151.o
_HEADERS += extern/src/ym2151.h

OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))
HEADERS = $(patsubst %,$(SDIR)/%,$(_HEADERS))

ifneq ("$(wildcard ./src/rom_labels.h)","")
HEADERS+=src/rom_labels.h
endif


all: $(OBJS) $(HEADERS)
	$(CC) -o $(OUTPUT) $(OBJS) $(LDFLAGS)
$(ODIR)/%.o: $(SDIR)/%.c
	@mkdir -p $$(dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@

cpu/tables.h cpu/mnemonics.h: cpu/buildtables.py cpu/6502.opcodes cpu/65c02.opcodes
	cd cpu && python buildtables.py


# WebASssembly/emscripten target
#
# See webassembly/WebAssembly.md
wasm:
	emmake make

#
# PACKAGING
#
# Packaging is tricky and partially depends on Michael's specific setup. :/
#
# * The Mac build is done on a Mac.
# * The Windows build is cross-compiled on the Mac using mingw. For more info, see:
#   https://blog.wasin.io/2018/10/21/cross-compile-sdl2-library-and-app-on-windows-from-macos.html
# * The Linux build is done by sshing into a VMware Ubuntu machine that has the same
#   directory tree mounted. Since unlike on Windows and Mac, there are 0 libraries guaranteed
#   to be present, a static build would mean linking everything that is not the kernel. And since
#   there are always 3 ways of doing something on Linux, it would mean including three graphics
#   and three sounds backends. Therefore, the Linux build uses dynamic linking, requires libsdl2
#   to be installed and might only work on the version of Linux I used for building, which is the
#   current version of Ubuntu.
# * For converting the documentation from Markdown to HTML, pandoc is required:
#   brew install pandoc
#

# hostname of the Linux VM
LINUX_COMPILE_HOST = ubuntu.local
# path to the equivalent of `pwd` on the Mac
LINUX_BASE_DIR = /mnt/Documents/git/x16-emulator

TMPDIR_NAME=TMP-x16emu-package

define add_extra_files_to_package
	# ROMs
	cp ../x16-rom/build/x16/rom.bin $(TMPDIR_NAME)
	cp ../x16-rom/build/x16/kernal.sym  $(TMPDIR_NAME)
	cp ../x16-rom/build/x16/keymap.sym  $(TMPDIR_NAME)
	cp ../x16-rom/build/x16/dos.sym     $(TMPDIR_NAME)
	cp ../x16-rom/build/x16/geos.sym    $(TMPDIR_NAME)
	cp ../x16-rom/build/x16/basic.sym   $(TMPDIR_NAME)
	cp ../x16-rom/build/x16/monitor.sym $(TMPDIR_NAME)
	cp ../x16-rom/build/x16/charset.sym $(TMPDIR_NAME)

	# Empty SD card image
	cp sdcard.img.zip $(TMPDIR_NAME)

	# Documentation
	mkdir $(TMPDIR_NAME)/docs
	pandoc --from gfm --to html -c github-pandoc.css --standalone --metadata pagetitle="Commander X16 Emulator" README.md --output $(TMPDIR_NAME)/docs/README.html
	pandoc --from gfm --to html -c github-pandoc.css --standalone --metadata pagetitle="Commander X16 KERNAL/BASIC/DOS ROM"  ../x16-rom/README.md --output $(TMPDIR_NAME)/docs/KERNAL-BASIC.html
	pandoc --from gfm --to html -c github-pandoc.css --standalone --metadata pagetitle="Commander X16 Programmer's Reference Guide"  ../x16-docs/Commander\ X16\ Programmer\'s\ Reference\ Guide.md --output $(TMPDIR_NAME)/docs/Programmer\'s\ Reference\ Guide.html --lua-filter=mdtohtml.lua
	pandoc --from gfm --to html -c github-pandoc.css --standalone --metadata pagetitle="VERA Programmer's Reference.md"  ../x16-docs/VERA\ Programmer\'s\ Reference.md --output $(TMPDIR_NAME)/docs/VERA\ Programmer\'s\ Reference.html
	cp github-pandoc.css $(TMPDIR_NAME)/docs
endef

package: package_mac package_win package_linux
	make clean

package_mac:
	(cd ../x16-rom/; make clean all)
	MAC_STATIC=1 make clean all
	rm -rf $(TMPDIR_NAME) x16emu_mac.zip
	mkdir $(TMPDIR_NAME)
	cp x16emu $(TMPDIR_NAME)
	$(call add_extra_files_to_package)
	(cd $(TMPDIR_NAME)/; zip -r "../x16emu_mac.zip" *)
	rm -rf $(TMPDIR_NAME)

package_win:
	(cd ../x16-rom/; make clean all)
	CROSS_COMPILE_WINDOWS=1 make clean all
	rm -rf $(TMPDIR_NAME) x16emu_win.zip
	mkdir $(TMPDIR_NAME)
	cp x16emu.exe $(TMPDIR_NAME)
	cp $(MINGW32)/lib/libgcc_s_sjlj-1.dll $(TMPDIR_NAME)/
	cp $(MINGW32)/bin/libwinpthread-1.dll $(TMPDIR_NAME)/
	cp $(WIN_SDL2)/bin/SDL2.dll $(TMPDIR_NAME)/
	$(call add_extra_files_to_package)
	(cd $(TMPDIR_NAME)/; zip -r "../x16emu_win.zip" *)
	rm -rf $(TMPDIR_NAME)

package_linux:
	(cd ../x16-rom/; make clean all)
	ssh $(LINUX_COMPILE_HOST) "cd $(LINUX_BASE_DIR); make clean all"
	rm -rf $(TMPDIR_NAME) x16emu_linux.zip
	mkdir $(TMPDIR_NAME)
	cp x16emu $(TMPDIR_NAME)
	$(call add_extra_files_to_package)
	(cd $(TMPDIR_NAME)/; zip -r "../x16emu_linux.zip" *)
	rm -rf $(TMPDIR_NAME)

clean:
	rm -rf $(ODIR) x16emu x16emu.exe x16emu.js x16emu.wasm x16emu.data x16emu.worker.js x16emu.html x16emu.html.mem
