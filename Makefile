CFLAGS=-Wall -Werror -g $(shell sdl2-config --cflags)
LDFLAGS=$(shell sdl2-config --libs)

OBJS = fake6502.o memory.o disasm.o video.o main.o
HEADERS = disasm.h fake6502.h glue.h memory.h rom_labels.h video.h

all: $(OBJS) $(HEADERS)
	cc $(LDFLAGS) -o x16emu $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o x16emu
