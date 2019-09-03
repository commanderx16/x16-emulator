# Use
#   CROSS_COMPILE_WINDOWS=1 make
# to cross-compile for Windows on macOS. See
# https://blog.wasin.io/2018/10/21/cross-compile-sdl2-library-and-app-on-windows-from-macos.html
# for details on how to install mingw and libsdl2.

# the mingw32 path on macOS installed through homebrew
MINGW32=/usr/local/Cellar/mingw-w64/6.0.0_2/toolchain-i686/i686-w64-mingw32
# the Windows SDL2 path on macOS installed through ./configure --prefix=... && make && make install
WIN_SDL2=~/tmp/sdl2-win32

ifeq ($(CROSS_COMPILE_WINDOWS),1)
	SDL2CONFIG=$(WIN_SDL2)/bin/sdl2-config
else
	SDL2CONFIG=sdl2-config
endif

CFLAGS=-O3 -Wall -Werror -g $(shell $(SDL2CONFIG) --cflags)
LDFLAGS=$(shell $(SDL2CONFIG) --libs)

ifeq ($(MAC_STATIC),1)
	LDFLAGS=/usr/local/lib/libSDL2.a -lm -liconv -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox -Wl,-framework,ForceFeedback -lobjc -Wl,-framework,CoreVideo -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,IOKit -Wl,-weak_framework,QuartzCore -Wl,-weak_framework,Metal
endif

ifeq ($(CROSS_COMPILE_WINDOWS),1)
	LDFLAGS+=-L$(MINGW32)/lib
	# this enables printf() to show, but also forces a console window
	LDFLAGS+=-Wl,--subsystem,console 
	CC=i686-w64-mingw32-gcc
endif

OBJS = fake6502.o memory.o disasm.o video.o ps2.o via.o loadsave.o sdcard.o main.o
HEADERS = disasm.h fake6502.h glue.h memory.h rom_labels.h video.h ps2.h via.h loadsave.h

all: $(OBJS) $(HEADERS)
	$(CC) -o x16emu $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


# Packaging requires pandoc:
#   brew install pandoc

package: package_mac package_win
	make clean

package_mac:
	(cd ../x16-kernalbasic/; ./build.sh)
	MAC_STATIC=1 make clean all
	rm -rf ~x16emu-package x16emu_mac.zip
	mkdir ~x16emu-package
	cp x16emu ~x16emu-package
	cp ../x16-kernalbasic/rom.bin ~x16emu-package
	cp -p ~/tmp/chargen ~x16emu-package/chargen.bin
	pandoc --from gfm --to html --standalone README.md --output ~x16emu-package/README.html
	pandoc --from gfm --to html --standalone ../x16-kernalbasic/README.md --output ~x16emu-package/KERNAL-BASIC.html
	(cd ~x16emu-package/; zip "../x16emu_mac.zip" *)
	rm -rf ~x16emu-package

package_win:
	(cd ../x16-kernalbasic/; ./build.sh)
	CROSS_COMPILE_WINDOWS=1 make clean all
	rm -rf ~x16emu-package x16emu_win.zip
	mkdir ~x16emu-package
	cp x16emu.exe ~x16emu-package
	cp $(MINGW32)/lib/libgcc_s_sjlj-1.dll ~x16emu-package/
	cp $(MINGW32)/bin/libwinpthread-1.dll ~x16emu-package/
	cp $(WIN_SDL2)/bin/SDL2.dll ~x16emu-package/
	cp ../x16-kernalbasic/rom.bin ~x16emu-package
	cp -p ~/tmp/chargen ~x16emu-package/chargen.bin
	pandoc --from gfm --to html --standalone README.md --output ~x16emu-package/README.html
	pandoc --from gfm --to html --standalone ../x16-kernalbasic/README.md --output ~x16emu-package/KERNAL-BASIC.html
	(cd ~x16emu-package/; zip "../x16emu_win.zip" *)
	rm -rf ~x16emu-package

clean:
	rm -f *.o x16emu x16emu.exe
