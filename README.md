# Commander X16 Emulator

This is an emulator for the Commander X16 computer system. It only depends on SDL2 and should compile on all modern operating systems.

## Starting

You can start `x16emu`/`x16emu.exe` either by double-clicking it, or from the command line. The latter allows you to specify additional arguments.

* When starting `x16emu` without arguments, it will pick up the system ROM (`rom.bin`) and the character ROM (`chargen.bin`) from the executable's directory.
* The system ROM and character ROM filenames/paths can be overridden with the `-rom` and `-char` command line arguments.
* `-sdcard` lets you specify an SD card image (partition table + FAT32).
* `-prg` leys you specify a `.prg` file that gets injected into RAM after start.
* When compiled with `#define TRACE`, `-trace` will enable an instruction trace on stdout.

Run `x16emu -h` to see all command line options.

## Functions while running

* Cmd + R will reset the computer.
* Cmd + S will save a memory dump (40 KB main RAM + 2 MB bankable RAM) to disk.

(These shortcuts currently only work on macOS.)

## Host Filesystem Interface

If the system ROM contains any version of the KERNAL, the LOAD (`$FFD5`) and SAVE (`$FFD8`) KERNAL calls are intercepted by the emulator if the device is 1 (which is the default). So the BASIC statements

      LOAD"$
      LOAD"FOO.PRG
      LOAD"IMAGE.PRG",1,1
      SAVE"BAR.PRG

will target the host computer's local filesystem.

## Using the KERNAL/BASIC environment

Please see the KERNAL/BASIC documentation.

## Features

* CPU: Full 6502 instruction set ("fake6502")
* VERA
	* Mostly cycle exact emulation
	* Supports almost all features: composer, two layers, sprites, progressive/interlaced
* VIA
	* ROM/RAM banking
	* PS/2 keyboard
	* SD card (SPI)
* A 60 Hz interrupt is injected independently of the VIA settings.


## Missing Features

* CPU
	* Only supports the 6502 instruction set, not the 65C02 additions.
* VERA
	* Does not support IRQs
	* Does not support the "CURRENT_FIELD" bit
	* Does not sprite z-depth, collisions or limitations
	* Only supports the first 16 sprites
	* Interlaced modes (NTSC/RGB) don't render at the full horizontal fidelity
* VIA
	* Does not support counters/timers/IRQs
	* Does not support game controllers
* Sound
	* No support


## License

Copyright (c) 2019 Michael Steil &lt;mist64@mac.com&gt;, [www.pagetable.com](https://www.pagetable.com/).
All rights reserved. License: 2-clause BSD

## Release Notes

### Release 27

* Command line overhaul. Supports `-rom`, `-char`, `-sdcard` and `-prg`.
* ROM and char filename defaults, so x16emu can be started without arguments.
* Host Filesystem Interface supports LOAD"$"
* macOS and Windows packaging logic in Makefile

### Release 26

* better sprite support (clipping, palette offset, flipping)
* better border support
* KERNAL can set up interlaced NTSC mode with scaling and borders (compile time option)

### Release 25

* sdcard: fixed LOAD,x,1 to load to the correct addressg
* sdcard: all temp data will be on bank #255; current bank will remain unchanged
* DOS: support for DOS commands ("UI", "I", "V", ...) and more status messages (e.g. 26,WRITE PROTECT ON,00,00)
* BASIC: "DOS" command. Without argument: print disk status; with "$" argument: show directory; with "8" or "9" argument: switch default drive; otherwise: send DOS command; also accessible through F7/F8
* Vera: cycle exact rendering, NTSC, interlacing, border

### Release 24

* SD card support
	* pass path to SD card image as third argument
	* access SD card as drive 8
	* the local PC/Mac disk is still drive 1
	* modulo debugging, this would work on a real X16 with the SD card (plus level shifters) hooked up to VIA#2PB as described in sdcard.c in the emulator surce

### Release 23

* Updated emulator and ROM to spec 0.6 – the ROM image should work on a real X16 with VERA 0.6 now.

### Release 22

SYS65375 (SWAPPER) now also clears the screen, avoid ing side effects.

### Release 21

* support for $ and % number prefixes in BASIC
* support for C128 KERNAL APIs LKUPLA, LKUPSA and CLOSE_ALL

### Release 20

* Toggle fullscreen using Cmd+F or Cmd+return
* new BASIC instructions and functions:
	* MON: enter monitor; no more SYS65280 required
	* VPEEK(bank, address)
	* VPOKE bank, address, value
example: VPOKE4,0,VPEEK(4,0) OR 32 [for 256 color BASIC]

### Release 19

* fixed cursor trail bug
* fixed f7 key in PS/2 driver
* f keys are assigned with shortcuts now:
F1: LIST
F2: &lt;enter monitor&gt;
F3: RUN
F4: &lt;switch 40/80&gt;
F5: LOAD
F6: SAVE"
F7: DOS"$ &lt;doesn't work yet&gt;
F8: DOS &lt;doesn't work yet&gt; 

### Release 18

* Fixed scrolling in 40x30 mode when there are double lines on the screen.

### Release 17

* video RAM support in the monitor (SYS65280)
* 40x30 screen support (SYS65375 to toggle)

### Release 16

* Integrated monitor, start with SYS65280
rom.bin is now 3*8 KB:
	* 0: BASIC (bank 0 at $C000)
	* 1: KERNAL ($E000)
	* 2: UTIL (bank 1 at $C000)

### Release 15

* correct text mode video RAM layout both in emulator and KERNAL

### Release 14

* KERNAL: fast scrolling
* KERNAL: upper/lower switching using CHR$($0E)/CHR$($8E)
* KERNAL: banking init
* KERNAL: new PS/2 driver
* Emulator: VERA updates (more modes, second data port)
* Emulator: RAM and ROM banks start out as all 1 bits

### Release 13

* Supports mode 7 (8bpp bitmap).

### Release 12

* Supports 8bpp tile mode (mode 4)

### Release 11

* The emulator and the KERNAL now speak the bit-level PS/2 protocol over VIA#2 PA0/PA1. The system behaves the same, but keyboard input in the ROM should work on a real device. 

### Release 10

updated KERNAL with proper power-on message

### Release 9

* LOAD and SAVE commands are intercepted by the emulator, can be used to access local file system, like this:

      LOAD"TETRIS.PRG
      SAVE"TETRIS.PRG

* No device number is necessary. Loading absolute works like this:

      LOAD"FILE.PRG",1,1

### Release 8

* New optional override load address for PRG files:

      ./x64emu rom.bin chargen.bin basic.prg,0401

### Release 7

* Now with banking. POKE40801,n to switch the RAM bank at $A000. POKE40800,n to switch the ROM bank at $C000. The ROM file at the command line can be up to 72 KB now (layout: 0: bank 0, 1: KERNAL, 2: bank 1, 3: bank 2 etc.), and the RAM that Cmd+S saves is 2088KB ($0000-$9F00: regular RAM, $9F00-$9FFF: unused, $A000+: extra banks)

### Release 6

* Vera emulation now matches the complete spec dated 2019-07-06: correct video address space layout, palette format, redefinable character set

### Release 5

* BASIC now starts at $0401 (39679 BASIC BYTES FREE)

### Release 4

* Cmd+S now saves all of memory (linear 64 KB for now, including ROM) to "memory.bin", "memory-1.bin", "memory-2.bin", etc. You can extract parts of it with Unix "dd", like: dd if=memory.bin of=basic.bin bs=1 skip=2049 count=38655

### Release 3

* Supports PRG file as third argument, which is injected after "READY.", so BASIC programs work as well.

### Release 2

* STOP key support

### Release 1

* 6502 core, fake PS/2 keyboard emulation (PS/2 data bytes appear at VIA#1 PB) and text mode Vera emulation
* KERNAL/BASIC modified for memory layout, missing VIC, Vera text mode and PS/2 keyboard
