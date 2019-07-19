CFLAGS=-Wall -Werror -g

OBJS = fake6502.o memory.o disasm.o main.o

all: $(OBJS)
	cc -o x16emu $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm *.o x16emu
