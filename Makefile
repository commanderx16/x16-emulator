
# the mingw32 path on macOS installed through homebrew
MINGW32=/usr/local/Cellar/mingw-w64/6.0.0_2/toolchain-i686/i686-w64-mingw32
# the Windows SDL2 path on macOS installed through ./configure --prefix=... && make && make install
WIN_SDL2=~/tmp/sdl2-win32

ifeq ($(CROSS_COMPILE_WINDOWS),1)
	SDL2CONFIG=$(WIN_SDL2)/bin/sdl2-config
else
	SDL2CONFIG=sdl2-config
endif

CFLAGS=-O2 -Werror -g $(shell $(SDL2CONFIG) --cflags)
LDFLAGS=$(shell $(SDL2CONFIG) --libs)

ifeq ($(MAC_STATIC),1)
	LDFLAGS=/usr/local/lib/libSDL2.a -lm -liconv -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox -Wl,-framework,ForceFeedback -lobjc -Wl,-framework,CoreVideo -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,IOKit -Wl,-weak_framework,QuartzCore -Wl,-weak_framework,Metal
endif

#ifeq ($(LINUX_STATIC),1)
#	LDFLAGS+=-Wl,-Bstatic -lSDL2 -L/lib/x86_64-linux-gnu/
#endif

ifeq ($(CROSS_COMPILE_WINDOWS),1)
	LDFLAGS+=-L$(MINGW32)/lib
	# this enables printf() to show, but also forces a console window
	LDFLAGS+=-Wl,--subsystem,console 
	CC=i686-w64-mingw32-gcc
endif

OBJS = cpu/fake6502.o memory.o disasm.o video.o ps2.o via.o loadsave.o sdcard.o main.o debugger.o
HEADERS = disasm.h cpu/fake6502.h glue.h memory.h video.h ps2.h via.h loadsave.h

ifneq ("$(wildcard ./rom_labels.h)","")
HEADERS+=rom_labels.h
endif

all: $(OBJS) $(HEADERS)
	$(CC) -o x16emu $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

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
LINUX_BASE_DIR = /mnt/Documents/git/x16emu

package: package_mac package_win package_linux
	make clean

package_mac:
	(cd ../x16-rom/; ./build.sh)
	MAC_STATIC=1 make clean all
	rm -rf ~x16emu-package x16emu_mac.zip
	mkdir ~x16emu-package
	cp x16emu ~x16emu-package
	cp ../x16-rom/rom.bin ~x16emu-package
	cp -p ~/tmp/chargen ~x16emu-package/chargen.bin
	pandoc --from gfm --to html --standalone --metadata pagetitle="X16 Emulator" README.md --output ~x16emu-package/README.html
	pandoc --from gfm --to html --standalone --metadata pagetitle="X16 KERNAL/BASIC/DOS ROM"  ../x16-rom/README.md --output ~x16emu-package/KERNAL-BASIC.html
	(cd ~x16emu-package/; zip "../x16emu_mac.zip" *)
	rm -rf ~x16emu-package

package_win:
	(cd ../x16-rom/; ./build.sh)
	CROSS_COMPILE_WINDOWS=1 make clean all
	rm -rf ~x16emu-package x16emu_win.zip
	mkdir ~x16emu-package
	cp x16emu.exe ~x16emu-package
	cp $(MINGW32)/lib/libgcc_s_sjlj-1.dll ~x16emu-package/
	cp $(MINGW32)/bin/libwinpthread-1.dll ~x16emu-package/
	cp $(WIN_SDL2)/bin/SDL2.dll ~x16emu-package/
	cp ../x16-rom/rom.bin ~x16emu-package
	cp -p ~/tmp/chargen ~x16emu-package/chargen.bin
	pandoc --from gfm --to html --standalone --metadata pagetitle="X16 Emulator" README.md --output ~x16emu-package/README.html
	pandoc --from gfm --to html --standalone --metadata pagetitle="X16 KERNAL/BASIC/DOS ROM"  ../x16-rom/README.md --output ~x16emu-package/KERNAL-BASIC.html
	(cd ~x16emu-package/; zip "../x16emu_win.zip" *)
	rm -rf ~x16emu-package

package_linux:
	(cd ../x16-rom/; ./build.sh)
	ssh $(LINUX_COMPILE_HOST) "cd $(LINUX_BASE_DIR); make clean all"
	rm -rf ~x16emu-package x16emu_linux.zip
	mkdir ~x16emu-package
	cp x16emu ~x16emu-package
	cp ../x16-rom/rom.bin ~x16emu-package
	cp -p ~/tmp/chargen ~x16emu-package/chargen.bin
	pandoc --from gfm --to html --standalone --metadata pagetitle="X16 Emulator" README.md --output ~x16emu-package/README.html
	pandoc --from gfm --to html --standalone --metadata pagetitle="X16 KERNAL/BASIC/DOS ROM"  ../x16-rom/README.md --output ~x16emu-package/KERNAL-BASIC.html
	(cd ~x16emu-package/; zip "../x16emu_linux.zip" *)
	rm -rf ~x16emu-package

clean:
	rm -f *.o cpu/*.o x16emu x16emu.exe
