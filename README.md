# Commander X16 Emulator

This is an emulator for the Commander X16 computer system. It only depends on SDL2 and should compile on all modern operating systems.

## Binaries & Compiling

Binary releases for macOS, Windows and x86_64 Linux are available on the [releases page](https://github.com/commanderx16/x16-emulator/releases).

For all other systems, make sure the development version of SDL2 is installed and type `make` for build the source. When building it from source, you will need the system ROM (`rom.bin`) and the character ROM (`chargen.bin`). It is best to take both files from the *latest* binary release. You can also build the system ROM yourself from the [X16 ROM source](https://github.com/commanderx16/x16-rom).

## Starting

You can start `x16emu`/`x16emu.exe` either by double-clicking it, or from the command line. The latter allows you to specify additional arguments.

* When starting `x16emu` without arguments, it will pick up the system ROM (`rom.bin`) and the character ROM (`chargen.bin`) from the executable's directory.
* The system ROM and character ROM filenames/paths can be overridden with the `-rom` and `-char` command line arguments.
* `-keymap` tells the KERNAL to switch to a specific keyboard layout. Use it without an argument to view the supported layouts.
* `-sdcard` lets you specify an SD card image (partition table + FAT32).
* `-prg` lets you specify a `.prg` file that gets injected into RAM after start.
* `-bas` lets you specify a BASIC program in ASCII format that automatically typed in (and tokenized).
* `-run` executes the application specified through `-prg` or `-bas` using `RUN` or `SYS`, depending on the load address.
* `-echo` causes all KERNAL/BASIC output to be printed to the host's terminal. Enable this and use the BASIC command "LIST" to convert a BASIC program to ASCII (detokenize).
* `-log` enables one or more types of logging (e.g. `-log KS`):
	* `K`: keyboard (key-up and key-down events)
	* `S`: speed (CPU load, frame misses)
	* `V`: video I/O reads and writes
* `-debug` enables the debugger.
* When compiled with `#define TRACE`, `-trace` will enable an instruction trace on stdout.

Run `x16emu -h` to see all command line options.

## Keyboard Layout

The X16 uses a PS/2 keyboard, and the ROM currently supports several different layouts. The following table shows their names, and what keys produce different characters than expected:

|Name  |Description 	       |Differences|
|------|------------------------|-------|
|en-us |US		       |[`] ⇒ [←], [~] ⇒ [π], [&#92;] ⇒ [£]|
|en-gb |United Kingdom	       |[`] ⇒ [←], [~] ⇒ [π]|
|de    |German		       |[§] ⇒ [£], [´] ⇒ [^], [^] ⇒ [←], [°] ⇒ [π]|
|nordic|Nordic                 |key left of [1] ⇒ [←],[π]|
|it    |Italian		       |[&#92;] ⇒ [←], [&vert;] ⇒ [π]|
|pl    |Polish (Programmers)   |[`] ⇒ [←], [~] ⇒ [π], [&#92;] ⇒ [£]|
|hu    |Hungarian	       |[&#92;] ⇒ [←], [&vert;] ⇒ [π], [§] ⇒ [£]|
|es    |Spanish		       |[&vert;] ⇒ π, &#92; ⇒ [←], Alt + [<] ⇒ [£]|
|fr    |French		       |[²] ⇒ [←], [§] ⇒ [£]|
|de-ch |Swiss German	       |[^] ⇒ [←], [°] ⇒ [π]|
|fr-be |Belgian French	       |[²] ⇒ [←], [³] ⇒ [π]|
|fi    |Finnish		       |[§] ⇒ [←], [½] ⇒ [π]|
|pt-br |Portuguese (Brazil ABNT)|[&#92;] ⇒ [←], [&vert;] ⇒ [π]|

Keys that produce international characters (like [ä] or [ç]) will not produce any character.

Since the emulator tells the computer the *position* of keys that are pressed, you need to configure the layout for the computer independently of the keyboard layout you have configured on the host.

**Use the F9 key to cycle through the layouts, or set the keyboard layout at startup using the `-keymap` command line argument.**

The following keys can be used for controlling games:

|Keyboard Key  | NES Equivalent |
|--------------|----------------|
|Ctrl          | A 		|
|Alt 	       | B		|
|Space         | SELECT         |
|Enter         | START		|
|Cursor Up     | UP		|
|Cursor Down   | DOWN		|
|Cursor Left   | LEFT		|
|Cursor Right  | RIGHT		|

## Functions while running

* Ctrl + R will reset the computer.
* Ctrl + V will paste the clipboard by injecting key presses.
* Ctrl + S will save a memory dump (40 KB main RAM + 2 MB bankable RAM) to disk.
* Ctrl + Return will toggle full screen mode.

On the Mac, use the Cmd key instead.

## BASIC and the Screen Editor

On startup, the X16 presents direct mode of BASIC V2. You can enter BASIC statements, or line numbers with BASIC statements and `RUN` the program, just like on Commodore computers.

* To stop execution of a BASIC program, hit the RUN/STOP key (Esc in the emulator), or Ctrl + C.
* To insert a character, press Shift + Backspace.
* To clear the screen, press Shift + Home.
* The X16 does not have a STOP+RESTORE function.

## Host Filesystem Interface

If the system ROM contains any version of the KERNAL, the LOAD (`$FFD5`) and SAVE (`$FFD8`) KERNAL calls are intercepted by the emulator if the device is 1 (which is the default). So the BASIC statements

      LOAD"$
      LOAD"FOO.PRG
      LOAD"IMAGE.PRG",1,1
      SAVE"BAR.PRG

will target the host computer's local filesystem.

The emulator will interpret filesnames relative to the directory it was started in. Note that on macOS, when double-clicking the executable, this is the home directory.

To avoid incompatibility problems between the PETSCII and ASCII encodings, use lower case filenames on the host side, and unshifted filenames on the X16 side.

## Dealing with BASIC Programs

BASIC programs are encoded in a tokenized form, they are not simply ASCII files. If you want to edit BASIC programs on the host's text editor, you need to convert it between tokenized BASIC form and ASCII.

* To convert ASCII to BASIC, reboot the machine and paste the ASCII text using Ctrl + V (Mac: Cmd + V). You can now run the program, or use the `SAVE` BASIC command to write the tokenized version to disk.
* To convert BASIC to ASCII, start x16emu with the -echo argument, `LOAD` the BASIC file, and type `LIST`. Now copy the ASCII version from the terminal.

## Using the KERNAL/BASIC environment

Please see the KERNAL/BASIC documentation.

## Debugger 

The debugger requires `-debug` to start. Without it it is effectively disabled.

There are 2 panels you can control. The code panel, the top left half, and the data panel, the bottom half of the screen. The displayed address can be changed using keys 0-9 and A-F, in a 'shift and roll' manner (easier to see than explain). To change the data panel address, press the shift key and type 0-9 A-F. The top write panel is fixed.

The debugger keys are similar to the Microsoft Debugger shortcut keys, and work as follows

|Key|Description 																			|
|---|---------------------------------------------------------------------------------------|
|F1 |resets the shown code position to the current PC										|
|F2 |resets the 65C02 CPU but not any of the hardware.										|
|F5 |is used to return to Run mode, the emulator should run as normal.						|
|F9 |sets the breakpoint to the currently code position.									|
|F10|steps 'over' routines - if the next instruction is JSR it will break on return.		|
|F11|steps 'into' routines.																	|
|F12|is used to break back into the debugger. This does not happen if you do not have -debug|
|TAB|when stopped, or single stepping, hides the debug information when pressed 			|

When -debug is selected the No-Operation $FF will break into the debugger automatically.

Effectively keyboard routines only work when the debugger is running normally. Single stepping through keyboard code will not work at present.

## Features

* CPU: Full 65C02 instruction set (improved "fake6502")
* VERA
	* Mostly cycle exact emulation
	* Supports almost all features: composer, two layers, sprites, progressive/interlaced
* VIA
	* ROM/RAM banking
	* PS/2 keyboard
	* SD card (SPI)
* A 60 Hz interrupt is injected independently of the VIA settings.


## Missing Features

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

## Known Issues

* Emulator: LOAD"$ (and LOAD"$",1) will show host uppercase filenames as garbage. Use Ctrl+N to switch to the X16 upper/lower character set for a workaround.
* BASIC: $ and % prefixes don't work for DATA entries
* RND(0) always returns 0

## Release Notes

### Release 30

Emulator:
* VERA can now generate VSYNC interrupts
* added -keymap for setting the keyboard layout
* added -scale for integer scaling of the window [Stephen Horn]
* added -log to enable various logging features (can also be enabled at runtime (POKE $9FB0+) [Randall Bohn])
* changed -run to be an option to -prg and -bas
* emulator detection: read $9FBE/$9FBF, must read 0x31 and 0x36
* fix: -prg and -run no longer corrupt BASIC programs.
* fix: LOAD,1 into RAM bank [Stephen Horn]
* fix: 2bpp and 4bpp drawing [Stephen Horn]
* fix: 4bpp sprites [MonstersGoBoom]
* fix: build on Linux/ARM

### Release 29

* better keyboard support: if you pretend you have a US keyboard layout when typing, all keys should now be reachable [Paul Robson]
* -debug will enable the new debugger [Paul Robson]
* runs at the correct speed (was way too slow on most machines)
* keyboard shortcuts work on Windows/Linux: Ctrl + F/R/S/V
* Ctrl + V pastes the clipboard as keypresses
* -bas file.txt loads a BASIC program in ASCII encoding
* -echo prints all BASIC/KERNAL output to the terminal, use it with LIST to convert a BASIC program to ASCII
* -run acts like -prg, but also autostarts the program
* JMP $FFFF and SYS 65535 exit the emulator and save memory the host's storage
* the packages now contain the current version of the Programmer's Reference Guide (HTML)
* fix: on Windows, some file load/saves may be been truncated

### Release 28

* support for 65C02 opcodes [Paul Robson]
* keep aspect ratio when resizing window [Sebastian Voges]
* updated sprite logic to VERA 0.7 – **the layout of the sprite data registers has changed, you need to change your code!**


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
