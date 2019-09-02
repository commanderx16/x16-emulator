# Use
#   CROSS_COMPILE_WINDOWS=1 make
# to cross-compile for Windows on macOS. See
# https://blog.wasin.io/2018/10/21/cross-compile-sdl2-library-and-app-on-windows-from-macos.html
# for details on how to install mingw and libsdl2.

ifeq ($(CROSS_COMPILE_WINDOWS),1)
	SDL2CONFIG=~/tmp/sdl2-win32/bin/sdl2-config
else
	SDL2CONFIG=sdl2-config
endif

CFLAGS=-O3 -Wall -Werror -g $(shell $(SDL2CONFIG) --cflags)
LDFLAGS=$(shell $(SDL2CONFIG) --libs)

ifeq ($(MAC_STATIC),1)
	LDFLAGS=/usr/local/lib/libSDL2.a -lm -liconv -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox -Wl,-framework,ForceFeedback -lobjc -Wl,-framework,CoreVideo -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,IOKit -Wl,-weak_framework,QuartzCore -Wl,-weak_framework,Metal
endif

ifeq ($(CROSS_COMPILE_WINDOWS),1)
	LDFLAGS+=-L/usr/local/Cellar/mingw-w64/6.0.0_2/toolchain-i686/i686-w64-mingw32/lib -Wl,--subsystem,console
	CC=i686-w64-mingw32-gcc
endif

OBJS = fake6502.o memory.o disasm.o video.o ps2.o via.o loadsave.o sdcard.o main.o
HEADERS = disasm.h fake6502.h glue.h memory.h rom_labels.h video.h ps2.h via.h loadsave.h

all: $(OBJS) $(HEADERS)
	$(CC) -o x16emu $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

package_mac:
	(cd ../x16-kernalbasic/; ./build.sh)
	MAC_STATIC=1 make clean all
	rm -rf ~x16emu-package
	mkdir ~x16emu-package
	cp x16emu ~x16emu-package
	cp ../x16-kernalbasic/rom.bin ~x16emu-package
	cp -p ~/tmp/chargen ~x16emu-package/chargen.bin
	(cd ~x16emu-package/; zip "../x16emu_mac.zip" *)
	rm -rf ~x16emu-package

package_win:
	(cd ../x16-kernalbasic/; ./build.sh)
	CROSS_COMPILE_WINDOWS=1 make clean all
	rm -rf ~x16emu-package
	mkdir ~x16emu-package
	cp x16emu.exe ~x16emu-package
	cp /usr/local/Cellar/mingw-w64/6.0.0_2/toolchain-i686/i686-w64-mingw32/lib/libgcc_s_sjlj-1.dll ~x16emu-package/
	cp /usr/local/Cellar/mingw-w64/6.0.0_2/toolchain-i686/i686-w64-mingw32/bin/libwinpthread-1.dll ~x16emu-package/
	cp ~/tmp/sdl2-win32/bin/SDL2.dll ~x16emu-package/
	cp ../x16-kernalbasic/rom.bin ~x16emu-package
	cp -p ~/tmp/chargen ~x16emu-package/chargen.bin
	(cd ~x16emu-package/; zip "../x16emu_win.zip" *)
	rm -rf ~x16emu-package

clean:
	rm -f *.o x16emu x16emu.exe
