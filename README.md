# Commander X16 Emulator

This is an emulator for the Commander X16 computer.

## Building
 The build requires a `make`, C compiler and SDL2 (Mac: `brew install sdl2`, Ubuntu: `apt-get install libsdl2-dev`). Build like this:

 	make

## Usage
	Usage: ./x16emu <rom.bin> <chargen.bin> [<app.prg>[,<load_addr>]]

	<rom.bin>:     ROM file:
			 $0000-$1FFF bank #0 of banked ROM (BASIC)
			 $2000-$3FFF fixed ROM at $E000-$FFFF (KERNAL)
			 $4000-$5FFF bank #1 of banked ROM
			 $6000-$7FFF bank #2 of banked ROM
			 ...
		       The file needs to be at least $4000 bytes in size.

	<chargen.bin>: Character ROM file:
			 $0000-$07FF upper case/graphics
			 $0800-$0FFF lower case

	<app.prg>:     Application PRG file (with 2 byte start address header)

	<load_addr>:   Override load address (hex, no prefix)

* The BASIC commands `LOAD` and `SAVE` are trapped by the emulator and will be directed to PRG files on the local filesystem. The device number is not required. "Absolute load" (to the original address) is possible with `LOAD"FILE.PRG",8,1`.
* Cmd+S (Mac only) saves all of RAM into a file "memory.bin" (or "memory-2.bin" etc.) in the current directory.

## Status

### Implemented Hardware Features
* 6502 core
* memory layout and RAM/ROM mapping
* complete Vera video emulation (spec dated 2019-07-06)
* PS/2 keyboard emulation
* hardcoded 50 Hz IRQ

### Missing Hardware Features
* 65C02 extensions
* VIA timers and interrupts
* audio

## Authors
Michael Steil, <mist64@mac.com>
