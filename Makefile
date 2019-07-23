CFLAGS=-Wall -Werror -g $(shell sdl2-config --cflags)
LDFLAGS=$(shell sdl2-config --libs)

#
# use this for static linking on macOS (after "brew install sdl2")
#
#LDFLAGS=/usr/local/lib/libSDL2.a -lm -liconv -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox -Wl,-framework,ForceFeedback -lobjc -Wl,-framework,CoreVideo -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,IOKit -Wl,-weak_framework,QuartzCore -Wl,-weak_framework,Metal

OBJS = fake6502.o memory.o disasm.o video.o ps2.o via.o main.o
HEADERS = disasm.h fake6502.h glue.h memory.h rom_labels.h video.h ps2.h via.h

all: $(OBJS) $(HEADERS)
	cc $(LDFLAGS) -o x16emu $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o x16emu
